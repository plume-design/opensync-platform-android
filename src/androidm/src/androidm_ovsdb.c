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
#include "androidm_report.h"
#include "osandroid_ipc.h"

#ifdef CONFIG_ANDROIDM_APP_REPORT
#include "androidm_app_usage.h"
#endif

#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
#include "androidm_streaming.h"
#endif

/******************************************************************************
 * OVSDB
 *****************************************************************************/
#define MODULE_ID LOG_MODULE_ID_OVSDB

ovsdb_table_t table_AWLAN_Node;
ovsdb_table_t table_Node_Config;

static mqtt_header_ids_t androidm_get_awlan_header_id(const char *key)
{
    int val;

    val = strcmp(key, "locationId");
    if (val == 0) return MQTT_HEADER_LOCATION_ID;

    val = strcmp(key, "nodeId");
    if (val == 0) return MQTT_HEADER_NODE_ID;

    return MQTT_NO_HEADER;
}

static void androidm_update_awlan_headers(struct schema_AWLAN_Node *awlan)
{
    android_mgr_t *mgr = androidm_get_mgr();

#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
    androidm_collector_t *streaming_collector = &mgr->streaming_collector;
#endif

#ifdef CONFIG_ANDROIDM_APP_REPORT
    androidm_collector_t *app_usage_collector = &mgr->app_usage_collector;
#endif

    int i = 0;
    LOGT("%s %d", __FUNCTION__, awlan ? awlan->mqtt_headers_len : 0);

    for (i = 0; i < awlan->mqtt_headers_len; i++)
    {
        char *key = awlan->mqtt_headers_keys[i];
        char *val = awlan->mqtt_headers[i];
        mqtt_header_ids_t id = MQTT_NO_HEADER;

        LOGT("mqtt_headers[%s]='%s'", key, val);

        id = androidm_get_awlan_header_id(key);
        if (id == MQTT_NO_HEADER)
        {
            LOG(ERR, "%s: invalid mqtt_headers key", key);
            continue;
        }
        else if (id == MQTT_HEADER_NODE_ID)
        {
            STRSCPY(mgr->node_id, awlan->mqtt_headers[i]);
            LOGI("%s: new node_id[%s]", __func__, mgr->node_id);
        }
        else if (id == MQTT_HEADER_LOCATION_ID)
        {
            STRSCPY(mgr->node_id, awlan->mqtt_headers[i]);
            LOGI("%s: new location_id[%s]", __func__, mgr->location_id);
        }
    }

    for (i = 0; i < awlan->mqtt_topics_len; i++)
    {
#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
        if (strcmp(awlan->mqtt_topics_keys[i], STREAMING_REPORT_MQTT_TOPIC) == 0)
        {
            STRSCPY(streaming_collector->mqtt_topic, awlan->mqtt_topics[i]);
            LOGI("%s: new MQTT topic[%s]", __func__, streaming_collector->mqtt_topic);
        }
#endif /* CONFIG_ANDROIDM_STREAMING_REPORT */

#ifdef CONFIG_ANDROIDM_APP_REPORT
        if (strcmp(awlan->mqtt_topics_keys[i], APPUSAGE_REPORT_MQTT_TOPIC) == 0)
        {
            STRSCPY(app_usage_collector->mqtt_topic, awlan->mqtt_topics[i]);
            LOGI("%s: new MQTT topic[%s]", __func__, app_usage_collector->mqtt_topic);
        }
#endif /* CONFIG_ANDROIDM_APP_REPORT */
    }
}

/**
 * @brief Registered callback for AWLAN_Node events
 */
void callback_AWLAN_Node(
        ovsdb_update_monitor_t *mon,
        struct schema_AWLAN_Node *old_rec,
        struct schema_AWLAN_Node *awlan)
{
    if (mon->mon_type != OVSDB_UPDATE_DEL)
    {
        androidm_update_awlan_headers(awlan);
    }
}

/**
 * @brief Registered callback for Node_Config events
 */
static void callback_Node_Config(
        ovsdb_update_monitor_t *mon,
        struct schema_Node_Config *old_rec,
        struct schema_Node_Config *node_cfg)
{
#if defined(CONFIG_ANDROIDM_STREAMING_REPORT) || defined(CONFIG_ANDROIDM_APP_REPORT)
    android_mgr_t *mgr = androidm_get_mgr();
#endif

    if ((mon->mon_type == OVSDB_UPDATE_NEW) || (mon->mon_type == OVSDB_UPDATE_DEL)
        || (mon->mon_type == OVSDB_UPDATE_MODIFY))
    {
#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
        androidm_collector_t *streaming_collector = &mgr->streaming_collector;
        if (!strcmp(node_cfg->module, STREAMING_NODE_MODULE))
        {
            if (!strcmp(node_cfg->key, ANDROIDM_RPT_INTERVAL_NODE_KEY))
            {
                if (mon->mon_type == OVSDB_UPDATE_DEL)
                {
                    if (streaming_collector->mqtt_interval != 0)
                    {
                        /* Stop streaming report */
                        streaming_collector->mqtt_interval = 0;
                        LOGI("%s: Stop streaming report.", __func__);
                    }
                }
                else
                {
                    if (atoi(node_cfg->value) >= 0)
                    {
                        streaming_collector->mqtt_interval = atoi(node_cfg->value);
                    }
                    else
                    {
                        streaming_collector->mqtt_interval = ANDROIDM_STREAMING_RPT_DEFAULT_INTERVAL;
                    }
                    LOGI("Streaming report start: mqtt_interval %d", (int)streaming_collector->mqtt_interval);
                }
            }
            else if (!strcmp(node_cfg->key, STREAMING_MONITORED_APPS_NODE_KEY))
            {
                if (mon->mon_type == OVSDB_UPDATE_DEL)
                {
                    if (streaming_collector->mqtt_interval != 0)
                    {
                        /* Stop streaming report */
                        streaming_collector->mqtt_interval = 0;
                        LOGI("%s: Stop streaming report.", __func__);
                    }
                    memset(streaming_collector->u.apps, 0, sizeof(streaming_collector->u.apps));
                }
                else
                {
                    androidm_streaming_update_apps(node_cfg);
                }
            }
        }
#endif /* CONFIG_ANDROIDM_STREAMING_REPORT */

#ifdef CONFIG_ANDROIDM_APP_REPORT
        androidm_collector_t *app_usage_collector = &mgr->app_usage_collector;
        if (!strcmp(node_cfg->module, APPUSAGE_NODE_MODULE))
        {
            if (!strcmp(node_cfg->key, ANDROIDM_RPT_INTERVAL_NODE_KEY))
            {
                int mqtt_interval = atoi(node_cfg->value);
                if (mon->mon_type == OVSDB_UPDATE_DEL)
                {
                    if (app_usage_collector->mqtt_interval != 0)
                    {
                        /* Stop app usage report */
                        app_usage_collector->mqtt_interval = 0;
                        LOGI("%s: Stop app_usage report.", __func__);
                    }
                }
                else
                {
                    if (0 < mqtt_interval)
                    {
                        app_usage_collector->mqtt_interval = mqtt_interval;
                    }
                    else if (0 == mqtt_interval)
                    {
                        /* Set one time timer */
                        androidm_one_time_app_usage_report(true);
                        app_usage_collector->mqtt_interval = 0;
                    }
                    else if (0 > mqtt_interval)
                    {
                        app_usage_collector->mqtt_interval = ANDROIDM_APPUSAGE_RPT_DEFAULT_INTERVAL;
                    }
                    LOGI("App usage report start: mqtt_interval %d", (int)app_usage_collector->mqtt_interval);
                }
            }
            else if (!strcmp(node_cfg->key, APPUSAGE_TIME_PERIOD_NODE_KEY))
            {
                int collect_period = atoi(node_cfg->value);
                if (mon->mon_type == OVSDB_UPDATE_DEL)
                {
                    LOGI("%s: Stop collecting APP_Usage from device .", __func__);
                    osandroid_app_usage_time_period_set(0);
                }
                else
                {
                    LOGD("%s: Set collecting APP_Usage time_period = %d", __func__, collect_period);
                    osandroid_app_usage_time_period_set(collect_period);
                }
            }
        }
#endif /* CONFIG_ANDROIDM_APP_REPORT */
    }
}

static void androidm_ovsdb_init(void *data)
{
    LOGI("Initializing Android ovsdb tables");

    // Initialize OVSDB tables
    OVSDB_TABLE_INIT_NO_KEY(AWLAN_Node);
    OVSDB_TABLE_INIT_NO_KEY(Node_Config);

    // Initialize OVSDB monitor callbacks
    OVSDB_TABLE_MONITOR(AWLAN_Node, false);
    OVSDB_TABLE_MONITOR(Node_Config, false);
}

static void androidm_ovsdb_fini(void *data)
{
    LOGI("Finishing androidm_ovsdb module.");
}

MODULE(androidm_ovsdb, androidm_ovsdb_init, androidm_ovsdb_fini)
