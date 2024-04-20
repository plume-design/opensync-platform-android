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

#include "osandroid_app_usage.h"
#include "osandroid_ipc.h"

static int time_period = OSANDROIDM_APPUSAGE_TIME_DEFAULT_PERIOD;

void osandroid_app_usage_time_period_set(int period)
{
    if (period != time_period)
    {
        time_period = period;
    }
}

bool osandroid_app_usage_json_parse(app_usage_t *info, const char *rep_buf)
{
    bool ret = false;
    const char *api = NULL;
    json_error_t error;
    json_t *params = NULL, *time_period_obj = NULL, *app_usage = NULL;
    json_t *root = json_loads(rep_buf, 0, &error);

    if (!root)
    {
        LOGE("Load app_usage JSON failed. Reason:%s", error.text);
        goto cleanup;
    }

    if (json_unpack_ex(root, &error, 0, "{s:s, s:O}", "api", &api, "params", &params))
    {
        LOGE("Error unpacking app_usage JSON: %s\n", error.text);
        goto cleanup;
    }

    if (json_array_size(params) > 0)
    {
        time_period_obj = json_array_get(params, 0);
        if (json_unpack_ex(time_period_obj, &error, 0, "{s:I}", "time_period", &info->time_period) != 0)
        {
            LOGE("Error unpacking JSON: Couldn't extract 'time_period' %s", error.text);
            goto cleanup;
        }

        app_usage = json_array_get(params, 1);
        info->cnt = json_object_size(app_usage);

        json_t *app_usage_array = json_object_get(app_usage, "app_usage");
        if (json_is_array(app_usage_array))
        {
            info->cnt = json_array_size(app_usage_array);
            info->app_usage_entry = (app_usage_entry_t **)malloc(info->cnt * sizeof(app_usage_entry_t *));
            if (info->app_usage_entry == NULL)
            {
                LOGE("malloc app_usage_entry_t failed");
                goto cleanup;
            }

            for (size_t i = 0; i < info->cnt; i++)
            {
                json_t *app_entry = json_array_get(app_usage_array, i);

                info->app_usage_entry[i] = (app_usage_entry_t *)malloc(sizeof(app_usage_entry_t));
                if (info->app_usage_entry[i] == NULL)
                {
                    LOGE("Malloc app_usage_entry_t[i] failed");
                    goto cleanup;
                }

                info->app_usage_entry[i]->app_name = (char *)json_string_value(json_object_get(app_entry, "app_name"));
                info->app_usage_entry[i]->launch_count = json_integer_value(json_object_get(app_entry, "launch_count"));
                info->app_usage_entry[i]->foreground_time =
                        json_integer_value(json_object_get(app_entry, "foreground_time"));
                info->app_usage_entry[i]->usage_rx_bytes =
                        json_integer_value(json_object_get(app_entry, "usage_rx_bytes"));
                info->app_usage_entry[i]->usage_tx_bytes =
                        json_integer_value(json_object_get(app_entry, "usage_tx_bytes"));
            }
        }
    }

    ret = true;

cleanup:
    if (root)
    {
        json_decref(root);
    }
    return ret;
}

bool osandroid_app_usage_get(app_usage_t *info)
{
    bool ret = false;
    char rep_buf[ZMQ_MSG_MAX_LEN];

    json_t *js = json_pack("{s:s, s:{s:i}}", "api", __func__, "params", "time_period", time_period);
    if (!js)
    {
        LOGE("%s: %s", __func__, "APP Usage JSON request is null");
        goto end;
    }

    ret = osandroid_ipc_request(js, rep_buf, ZMQ_MSG_MAX_LEN);
    if (ret == false)
    {
        goto cleanup;
    }

    LOGI("%s: %s", __func__, rep_buf);
    ret = osandroid_app_usage_json_parse(info, rep_buf);

cleanup:
    if (js) json_decref(js);

end:
    return ret;
}
