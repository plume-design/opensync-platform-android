/*
Copyright (c) 2023, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <errno.h>
#include <ev.h>
#include <getopt.h>
#include <jansson.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "ds.h"
#include "json_util.h"
#include "log.h"
#include "memutil.h"
#include "module.h"
#include "os.h"
#include "ovsdb.h"
#include "ovsdb_cache.h"
#include "ovsdb_sync.h"
#include "ovsdb_table.h"
#include "ovsdb_update.h"
#include "qm_conn.h"
#include "schema.h"
#include "target.h"
#include "util.h"

#include "androidm.h"
#include "osandroid_ipc.h"
#include "streaming.pb-c.h"

#define MODULE_ID LOG_MODULE_ID_MAIN

static bool androidm_streaming_send_report(char *topic, struct report_packed_buffer *pb)
{
#ifndef ARCH_X86
    qm_response_t res;
    bool ret = false;
#endif

    if (!topic)
    {
        LOGE("%s: topic NULL", __func__);
        return false;
    }

    if (!pb)
    {
        LOGE("%s: pb NULL", __func__);
        return false;
    }

    if (!pb->buf)
    {
        LOGE("%s: pb->buf NULL", __func__);
        return false;
    }

    LOGI("%s: msg len: %zu, topic: %s", __func__, pb->len, topic);

#ifndef ARCH_X86
    ret = qm_conn_send_direct(QM_REQ_COMPRESS_IF_CFG, topic, pb->buf, pb->len, &res);
    if (!ret)
    {
        LOGE("Error sending mqtt with topic %s", topic);
        return false;
    }
#endif

    return true;
}

bool androidm_streaming_update_apps(struct schema_Node_Config *node_cfg)
{
    android_mgr_t *mgr = androidm_get_mgr();

    char *token = strtok(node_cfg->value, "|");
    char(*apps)[256] = mgr->streaming_collector.u.apps;
    int i = 0;

    memset(apps, 0, sizeof(mgr->streaming_collector.u.apps));
    while (token != NULL && i < 256)
    {
        STRSCPY(apps[i], token);
        apps[i][sizeof(apps[i]) - 1] = '\0';

        token = strtok(NULL, "|");
        i++;
    }

    for (int j = 0; j < i; j++)
    {
        LOGI("%s: updated apps[%d]: %s", __func__, j, apps[j]);
    }

    return true;
}

/******************************************************************************
 * MQTT
 *****************************************************************************/
static Streaming__StreamingReport *androidm_streaming_set_pb_report(streaming_info_t *info)
{
    android_mgr_t *mgr = androidm_get_mgr();
    Streaming__BitRate *bitrate;
    Streaming__Resolution *resolution;
    Streaming__StreamingInfo *pb_info;
    Streaming__VideoInfo *video_info;
    Streaming__PlaybackInfo *playback_info;
    Streaming__StreamingReport *pb;

    /* Allocate the protobuf structure */
    pb = CALLOC(1, sizeof(*pb));
    pb_info = CALLOC(1, sizeof(*pb_info));
    video_info = CALLOC(1, sizeof(*video_info));
    playback_info = CALLOC(1, sizeof(*playback_info));
    resolution = CALLOC(1, sizeof(*resolution));
    bitrate = CALLOC(1, sizeof(*bitrate));

    /* Initialize the protobuf structure */
    streaming__streaming_report__init(pb);
    streaming__streaming_info__init(pb_info);
    streaming__video_info__init(video_info);
    streaming__playback_info__init(playback_info);
    streaming__resolution__init(resolution);
    streaming__bit_rate__init(bitrate);

    resolution->height = info->video_info.resol_height;
    resolution->width = info->video_info.resol_width;

    bitrate->audio = info->video_info.audio_bit_rate;
    bitrate->video = info->video_info.video_bit_rate;

    video_info->titlechannel = info->video_info.title_channel;
    video_info->totalduration = info->video_info.total_duration;
    video_info->resolution = resolution;
    video_info->frameratefps = info->video_info.frame_rate;
    video_info->framescount = info->video_info.frames_count;
    video_info->droppedframes = info->video_info.dropped_frames;
    video_info->errorframes = info->video_info.error_frames;
    video_info->rendertime = info->video_info.render_time;
    video_info->bitrate = bitrate;
    video_info->sampleratehz = info->video_info.sample_rate_hz;
    video_info->codec = info->video_info.codec;
    video_info->starttime = info->video_info.start_time;
    video_info->txbytes = info->video_info.tx_bytes;
    video_info->rxbytes = info->video_info.rx_bytes;
    video_info->bufferingtime = info->video_info.buffering_time;

    playback_info->duration = info->playback_info.duration;
    playback_info->volume = info->playback_info.volume;
    playback_info->playspeed = info->playback_info.play_speed;
    playback_info->state = (Streaming__PlaybackState)info->playback_info.state;

    pb_info->appname = info->app_name;
    pb_info->videoinfo = video_info;
    pb_info->playbackinfo = playback_info;

    pb->reportedat = time(NULL);
    pb->nodeid = mgr->node_id;
    pb->streaminginfo = pb_info;
    pb->error = info->error;

    return pb;
}

static void androidm_streaming_report_free_pb_report(Streaming__StreamingReport *pb)
{
    FREE(pb->streaminginfo->videoinfo->resolution);
    FREE(pb->streaminginfo->videoinfo->bitrate);
    FREE(pb->streaminginfo->videoinfo);
    FREE(pb->streaminginfo->playbackinfo);
    FREE(pb->streaminginfo);
    FREE(pb);
}

static bool androidm_serialize_streaming_report(streaming_info_t *info)
{
    bool res = false;
    android_mgr_t *mgr = androidm_get_mgr();
    char *mqtt_topic = mgr->streaming_collector.mqtt_topic;

    struct report_packed_buffer *serialized;
    Streaming__StreamingReport *pb;
    size_t len;
    void *buf;

    /* Allocate serialization output structure */
    serialized = CALLOC(1, sizeof(*serialized));

    pb = androidm_streaming_set_pb_report(info);

    /* Get serialization length */
    len = streaming__streaming_report__get_packed_size(pb);

    /* Allocate space for the serialized buffer */
    buf = MALLOC(len);

    serialized->len = streaming__streaming_report__pack(pb, buf);
    serialized->buf = buf;

    /* Free the protobuf structure */
    androidm_streaming_report_free_pb_report(pb);

    if (mqtt_topic[0])
    {
        res = androidm_streaming_send_report(mqtt_topic, serialized);
        LOGD("%s: AWLAN topic[%s]", __func__, mqtt_topic);
    }
    else
    {
        LOGI("%s: AWLAN topic: not set, mqtt report not sent", __func__);
        FREE(buf);
        return -1;
    }

    FREE(buf);
    return res;
}

bool androidm_streaming_build_mqtt_report(time_t now)
{
    streaming_info_t *info;
    bool res = 0;

    info = CALLOC(1, sizeof(streaming_info_t));

    res = osandroid_streaming_get(info);
    if (!res)
    {
        LOGW("%s: osandroid_streaming_get() failed", __func__);
    }

    res = androidm_serialize_streaming_report(info);
    if (!res)
    {
        LOGW("%s: androidm_serialize_streaming_report: failed", __func__);
    }

    FREE(info);
    return res;
}

void androidm_streaming_event_cb(const char *message)
{
    streaming_info_t *info;
    info = CALLOC(1, sizeof(streaming_info_t));

    osandroid_streaming_json_parse(info, message);
    LOGI("%s: event: %s", __func__, message);
    androidm_streaming_build_mqtt_report(time(NULL));

    FREE(info);
}

static void androidm_streaming_init(void *data)
{
    /* Android event registration */
    osandroid_subscriber_register(ANDROID_EVENT_STREAMING, androidm_streaming_event_cb);
}

static void androidm_streaming_fini(void *data)
{
    LOGI("Finishing androidm streaming Module.");
}

MODULE(androidm_streaming, androidm_streaming_init, androidm_streaming_fini)
