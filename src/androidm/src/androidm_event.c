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

#include "androidm.h"
#include "kconfig.h"
#include "log.h"

#ifdef CONFIG_ANDROIDM_APP_REPORT
#include "androidm_app_usage.h"
#endif

#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
#include "androidm_streaming.h"
#endif

/******************************************************************************
 * Event
 *****************************************************************************/
#define MODULE_ID LOG_MODULE_ID_EVENT

static void androidm_mqtt_periodic(time_t now, android_mgr_t *mgr)
{
#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
    /* Build Streaming Report */
    time_t stream_mqtt_periodic_ts = mgr->streaming_collector.mqtt_periodic_ts;
    time_t stream_mqtt_interval = mgr->streaming_collector.mqtt_interval;

    if (((now - stream_mqtt_periodic_ts) >= stream_mqtt_interval) && stream_mqtt_interval)
    {
        int res = androidm_streaming_build_mqtt_report(now);
        LOGI("%s: streaming report mqtt_interval[%d]", __func__, (int)stream_mqtt_interval);

        if (!res)
        {
            LOGE("%s: androidm_streaming_build_mqtt_report: failed, res=%d", __func__, res);
        }
        mgr->streaming_collector.mqtt_periodic_ts = now;
    }
#endif /* CONFIG_ANDROIDM_STREAMING_REPORT */

#ifdef CONFIG_ANDROIDM_APP_REPORT
    /* Build APP Report */
    time_t appusage_mqtt_periodic_ts = mgr->app_usage_collector.mqtt_periodic_ts;
    time_t appusage_mqtt_interval = mgr->app_usage_collector.mqtt_interval;
    bool is_one_time = mgr->app_usage_collector.u.is_one_time;

    if ((((now - appusage_mqtt_periodic_ts) >= appusage_mqtt_interval) && appusage_mqtt_interval)
        || (0 == appusage_mqtt_interval && true == is_one_time))
    {
        int res = androidm_app_usage_build_mqtt_report(now);
        if (true == is_one_time)
        {
            LOGI("%s: Android app usage data is sent immediately.", __func__);
            mgr->app_usage_collector.u.is_one_time = false;
        }
        else
        {
            LOGI("%s: app usage report mqtt_interval[%d]", __func__, (int)appusage_mqtt_interval);
        }

        if (!res)
        {
            LOGE("%s: androidm_app_usage_build_mqtt_report: failed, res=%d", __func__, res);
        }
        mgr->app_usage_collector.mqtt_periodic_ts = now;
    }
#endif /* CONFIG_ANDROIDM_APP_REPORT */
}

/**
 * @brief General periodic routine
 */
static void androidm_event_cb(struct ev_loop *loop, ev_timer *watcher, int revents)
{
    android_mgr_t *mgr;
    time_t now;

    (void)loop;
    (void)watcher;
    (void)revents;

    mgr = androidm_get_mgr();

    now = time(NULL);
    androidm_mqtt_periodic(now, mgr);

    mgr->periodic_ts = now;
}

/**
 * @brief Periodic timer initialization
 */
void androidm_event_init()
{
    android_mgr_t *mgr = androidm_get_mgr();

    LOGI("Initializing Androidm event");

    ev_timer_init(&mgr->timer, androidm_event_cb, ANDROIDM_TIMER_INTERVAL, ANDROIDM_TIMER_INTERVAL);
    mgr->timer.data = NULL;
    mgr->periodic_ts = time(NULL);

    if (kconfig_enabled(CONFIG_ANDROIDM_STREAMING_REPORT))
    {
        INIT_STREAMING_COLLECTOR(mgr->streaming_collector);
    }

    if (kconfig_enabled(CONFIG_ANDROIDM_APP_REPORT))
    {
        INIT_APPUSAGE_COLLECTOR(mgr->app_usage_collector);
    }

    ev_timer_start(mgr->loop, &mgr->timer);
}

