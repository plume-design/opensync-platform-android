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

#define _GNU_SOURCE
/* std libc */
#include <const.h>
#include <log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 3rd party */
#include <jansson.h>

/* internal */
#include <os.h>
#include <util.h>

#include "osandroid_ipc.h"
#include "osandroid_streaming.h"

bool osandroid_streaming_json_parse(streaming_info_t *info, const char *rep_buf)
{
    json_error_t error;
    json_t *root = json_loads(rep_buf, 0, &error);
    const char *api = NULL;
    json_t *params = NULL, *report = NULL;
    bool ret = false;

    if (!root)
    {
        LOGE("Parsing streaming info JSON failed");
        return false;
    }

    json_unpack_ex(root, &error, 0, "{s:s, s:O}", "api", &api, "params", &params);
    if (json_array_size(params) > 0)
    {
        json_unpack_ex(json_array_get(params, 0), &error, 0, "{s:O}", "streaming_info", &report);
        if (json_is_object(report))
        {
            const char *key;
            json_t *item;

            json_object_foreach(report, key, item)
            {
                if (json_is_string(item))
                {
                    if (strcmp(key, "app_name") == 0)
                    {
                        STRSCPY(info->app_name, json_string_value(item));
                    }
                    else if (strcmp(key, "title_channel") == 0)
                    {
                        STRSCPY(info->video_info.title_channel, json_string_value(item));
                    }
                    else if (strcmp(key, "codec") == 0)
                    {
                        STRSCPY(info->video_info.codec, json_string_value(item));
                    }
                    else if (strcmp(key, "error") == 0)
                    {
                        STRSCPY(info->error, json_string_value(item));
                    }

                    ret = true;
                }
                else if (json_is_number(item))
                {
                    if (strcmp(key, "total_duration") == 0)
                    {
                        info->video_info.total_duration = json_integer_value(item);
                    }
                    else if (strcmp(key, "resol_width") == 0)
                    {
                        info->video_info.resol_width = json_integer_value(item);
                    }
                    else if (strcmp(key, "resol_height") == 0)
                    {
                        info->video_info.resol_height = json_integer_value(item);
                    }
                    else if (strcmp(key, "frame_rate") == 0)
                    {
                        info->video_info.frame_rate = json_integer_value(item);
                    }
                    else if (strcmp(key, "frames_count") == 0)
                    {
                        info->video_info.frames_count = json_integer_value(item);
                    }
                    else if (strcmp(key, "dropped_frames") == 0)
                    {
                        info->video_info.dropped_frames = json_integer_value(item);
                    }
                    else if (strcmp(key, "error_frames") == 0)
                    {
                        info->video_info.error_frames = json_integer_value(item);
                    }
                    else if (strcmp(key, "render_time") == 0)
                    {
                        info->video_info.render_time = json_integer_value(item);
                    }
                    else if (strcmp(key, "audio_bit_rate") == 0)
                    {
                        info->video_info.audio_bit_rate = json_integer_value(item);
                    }
                    else if (strcmp(key, "video_bit_rate") == 0)
                    {
                        info->video_info.video_bit_rate = json_integer_value(item);
                    }
                    else if (strcmp(key, "sample_rate_hz") == 0)
                    {
                        info->video_info.sample_rate_hz = json_integer_value(item);
                    }
                    else if (strcmp(key, "tx_bytes") == 0)
                    {
                        info->video_info.tx_bytes = json_integer_value(item);
                    }
                    else if (strcmp(key, "rx_bytes") == 0)
                    {
                        info->video_info.rx_bytes = json_integer_value(item);
                    }
                    else if (strcmp(key, "start_time") == 0)
                    {
                        info->video_info.start_time = json_integer_value(item);
                    }
                    else if (strcmp(key, "connection_speed") == 0)
                    {
                        info->video_info.connection_speed = json_integer_value(item);
                    }
                    else if (strcmp(key, "buffering_time") == 0)
                    {
                        info->video_info.buffering_time = json_integer_value(item);
                    }
                    else if (strcmp(key, "duration") == 0)
                    {
                        info->playback_info.duration = json_integer_value(item);
                    }
                    else if (strcmp(key, "volume") == 0)
                    {
                        info->playback_info.volume = json_integer_value(item);
                    }
                    else if (strcmp(key, "play_speed") == 0)
                    {
                        info->playback_info.play_speed = (float)json_real_value(item);
                    }
                    else if (strcmp(key, "state") == 0)
                    {
                        info->playback_info.state = json_integer_value(item);
                    }

                    ret = true;
                }
                else
                {
                    LOGE("Parsing streaming info JSON failed");
                }
            }
        }
    }

    if (params) json_decref(params);
    if (report) json_decref(report);
    if (root) json_decref(root);

    return ret;
}

bool osandroid_streaming_get(streaming_info_t *info)
{
    char rep_buf[ZMQ_MSG_MAX_LEN];
    bool ret = false;

    json_t *msg = osandroid_ipc_build_header(__func__);
    if (!msg)
    {
        LOGE("%s: Failed to build IPC message header", __func__);
        return false;
    }

    ret = osandroid_ipc_request(msg, rep_buf, ZMQ_MSG_MAX_LEN);
    if (!ret)
    {
        LOGE("%s: IPC request failed", __func__);
        goto cleanup;
    }

    LOGI("%s: %s", __func__, rep_buf);
    ret = osandroid_streaming_json_parse(info, rep_buf);

cleanup:
    if (msg) json_decref(msg);

    return ret;
}
