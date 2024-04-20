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
#include "app.pb-c.h"
#include "osandroid_app_usage.h"
#include "osandroid_ipc.h"

/******************************************************************************
 * MQTT
 *****************************************************************************/
void androidm_one_time_app_usage_report(bool is_one_time)
{
    android_mgr_t *mgr = androidm_get_mgr();
    mgr->app_usage_collector.u.is_one_time = is_one_time;
}

static bool androidm_appusage_send_report(char *topic, struct report_packed_buffer *pb)
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
static void androidm_app_usage_report_free_pb_report(App__APPUsageReport *pb)
{
    if (!pb)
    {
        return;
    }

    for (size_t i = 0; i < pb->n_usage; i++)
    {
        if (pb->usage[i])
        {
            FREE(pb->usage[i]);
        }
    }

    if (pb->usage)
    {
        FREE(pb->usage);
    }

    FREE(pb);
}

static App__APPUsageReport *androidm_app_usage_set_pb_report(app_usage_t *app_usage_info)
{
    android_mgr_t *mgr = androidm_get_mgr();
    App__APPUsageReport *pb_app_usage;

    /* Allocate the protobuf structure */
    pb_app_usage = (App__APPUsageReport *)CALLOC(1, sizeof(App__APPUsageReport));
    if (pb_app_usage == NULL)
    {
        LOGE("Failed to allocate memory for pb_app_usage");
        return NULL;
    }

    /* Initialize the protobuf structure */
    app__appusage_report__init(pb_app_usage);

    pb_app_usage->usage = (App__APPUsageEntry **)CALLOC(app_usage_info->cnt, sizeof(App__APPUsageEntry *));
    if (pb_app_usage->usage == NULL)
    {
        LOGE("Failed to allocate memory for pb_app_usage->usage");
        goto clean_pb_app_usage;
    }

    for (size_t i = 0; i < app_usage_info->cnt; i++)
    {
        pb_app_usage->usage[i] = (App__APPUsageEntry *)CALLOC(1, sizeof(App__APPUsageEntry));
        if (pb_app_usage->usage[i] == NULL)
        {
            goto clean_pb_app_usage;
        }

        app__appusage_entry__init(pb_app_usage->usage[i]);

        pb_app_usage->usage[i]->appname = app_usage_info->app_usage_entry[i]->app_name;
        if (pb_app_usage->usage[i]->appname == NULL)
        {
            LOGE("Failed to allocate memory for pb_app_usage->usage[%zu]->appname", i);
            goto clean_pb_app_usage;
        }

        pb_app_usage->usage[i]->launchcount = app_usage_info->app_usage_entry[i]->launch_count;
        pb_app_usage->usage[i]->timeinforeground = app_usage_info->app_usage_entry[i]->foreground_time;
        pb_app_usage->usage[i]->usagetxbytes = app_usage_info->app_usage_entry[i]->usage_tx_bytes;
        pb_app_usage->usage[i]->usagerxbytes = app_usage_info->app_usage_entry[i]->usage_rx_bytes;
    }

    pb_app_usage->reportedat = time(NULL);
    pb_app_usage->nodeid = mgr->node_id;
    pb_app_usage->n_usage = app_usage_info->cnt;
    pb_app_usage->timeperiod = app_usage_info->time_period;
    return pb_app_usage;

clean_pb_app_usage:
    androidm_app_usage_report_free_pb_report(pb_app_usage);
    return NULL;
}

static void androidm_app_usage_release(app_usage_t *info)
{
    if (info == NULL)
    {
        return;
    }

    size_t number_of_entries = info->cnt;
    for (size_t i = 0; i < number_of_entries; i++)
    {
        if (info->app_usage_entry[i] != NULL)
        {
            FREE(info->app_usage_entry[i]);
        }
    }

    if (info->app_usage_entry != NULL)
    {
        FREE(info->app_usage_entry);
    }

    FREE(info);
}

static bool androidm_serialize_app_usage_report(app_usage_t *info)
{
    bool res = false;
    android_mgr_t *mgr = androidm_get_mgr();
    char *mqtt_topic = mgr->app_usage_collector.mqtt_topic;

    struct report_packed_buffer *serialized;
    App__APPUsageReport *pb;
    size_t len;
    void *buf;

    /* Allocate serialization output structure */
    serialized = CALLOC(1, sizeof(*serialized));

    pb = androidm_app_usage_set_pb_report(info);

    /* Get serialization length */
    len = app__appusage_report__get_packed_size(pb);

    /* Allocate space for the serialized buffer */
    buf = MALLOC(len);

    serialized->len = app__appusage_report__pack(pb, buf);
    serialized->buf = buf;

    /* Free the protobuf structure */
    androidm_app_usage_report_free_pb_report(pb);

    if (mqtt_topic[0])
    {
        res = androidm_appusage_send_report(mqtt_topic, serialized);
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

bool androidm_app_usage_build_mqtt_report(time_t now)
{
    app_usage_t *info;
    bool res = 0;

    info = CALLOC(1, sizeof(app_usage_t));

    res = osandroid_app_usage_get(info);
    if (!res)
    {
        LOGE("%s: osandroid_app_usage_get() failed", __func__);
    }

    res = androidm_serialize_app_usage_report(info);
    if (!res)
    {
        LOGE("%s: androidm_serialize_app_usage_report: failed", __func__);
    }

    androidm_app_usage_release(info);

    return res;
}

