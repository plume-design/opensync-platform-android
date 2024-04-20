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
#include <errno.h>
#include <jansson.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "schema.h"
#include "target.h"
#include "util.h"

#include "androidm.h"
#include "androidm_peripheral.h"

#include "osandroid_ipc.h"

#define MODULE_ID LOG_MODULE_ID_MAIN

ovsdb_table_t table_Peripheral_Device;

static void callback_Peripheral_Device(
        ovsdb_update_monitor_t *mon,
        struct schema_Peripheral_Device *old_rec,
        struct schema_Peripheral_Device *vconf)
{
    LOGD("%s: ovsdb updated", vconf->name);
}

static void peripheral_device_parse_ipc_message(
        const char *json_str,
        int *associated,
        struct schema_Peripheral_Device *peripheral_device)
{
    json_error_t error;

    json_t *root = json_loads(json_str, 0, &error);

    if (!root)
    {
        return;
    }

    json_t *params, *schemaDevice = NULL, *productInfo = NULL, *map_array = NULL;
    const char *api = NULL, *name = NULL, *physical_interface = NULL;

    json_unpack_ex(root, &error, 0, "{s:s, s:O}", "api", &api, "params", &params);
    if (json_array_size(params) > 0)
    {
        json_unpack_ex(json_array_get(params, 1), &error, 0, "{s:b}", "associated", associated);
        json_unpack_ex(json_array_get(params, 0), &error, 0, "{s:O}", "schema_Peripheral_Device", &schemaDevice);
        if (json_is_object(schemaDevice))
        {
            json_unpack_ex(
                    schemaDevice,
                    &error,
                    0,
                    "{s:s, s:s, s:O}",
                    "name",
                    &name,
                    "physical_interface",
                    &physical_interface,
                    "product_info",
                    &productInfo);

            SCHEMA_SET_STR(peripheral_device->physical_interface, physical_interface);
            SCHEMA_SET_STR(peripheral_device->name, name);

            if (json_is_object(productInfo))
            {
                json_unpack_ex(productInfo, &error, 0, "{s:O}", "map", &map_array);
                if (json_is_array(map_array))
                {
                    size_t index;
                    json_t *map_item, *value;
                    const char *key;

                    json_array_foreach(map_array, index, map_item)
                    {
                        json_object_foreach(map_item, key, value)
                        {
                            const char *value_str = json_string_value(value);
                            SCHEMA_KEY_VAL_APPEND(peripheral_device->product_info, key, value_str);
                        }
                    }
                }
            }
        }
    }
    json_decref(root);
}

static void androidm_peripheral_event_cb(const char *message)
{
    ovsdb_table_t table_Peripheral_Device = {0};
    struct schema_Peripheral_Device peripheral_device = {0};
    int associated = 0;

    peripheral_device_parse_ipc_message(message, &associated, &peripheral_device);
    OVSDB_TABLE_INIT(Peripheral_Device, name);
    OVSDB_TABLE_INIT(Peripheral_Device, physical_interface);

    if (associated)
    {
        if (!ovsdb_table_upsert_simple(
                    &table_Peripheral_Device,
                    "name",
                    (char *)peripheral_device.name,
                    &peripheral_device,
                    false))
        {
            LOGE("Unable to update Peripheral_Device table");
        }
    }
    else
    {
        if (strlen(peripheral_device.name) > 0)
        {
            ovsdb_table_delete_simple(&table_Peripheral_Device, "name", peripheral_device.name);
        }
        else
        {
            ovsdb_table_delete_simple(
                    &table_Peripheral_Device,
                    "physical_interface",
                    peripheral_device.physical_interface);
        }
    }
}

void androidm_peripheral_init(void *data)
{
    LOGI("Initializing Peripheral Device tables");

    // Initialize OVSDB tables
    OVSDB_TABLE_INIT_NO_KEY(Peripheral_Device);

    // Initialize OVSDB monitor callbacks
    OVSDB_TABLE_MONITOR(Peripheral_Device, true);

    osandroid_subscriber_register(ANDROID_EVENT_PERIPH_DEVICE_UPT, androidm_peripheral_event_cb);
}

void androidm_peripheral_fini(void *data)
{
    LOGI("Finising peripheral device.");
}

MODULE(peripheral_device, androidm_peripheral_init, androidm_peripheral_fini)
