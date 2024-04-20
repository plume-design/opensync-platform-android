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

#include "log.h"
#include <jansson.h>
#include <stdio.h>

#include "target_util.h"

void util_wifi_getter(const char *buf, int len, target_home_wifi_t *wifi_home)
{
    json_error_t error;
    json_t *root = json_loads(buf, 0, &error);

    if (!root)
    {
        LOGE("Error parsing JSON: %s\n", error.text);
        return;
    }

    json_t *params = json_object_get(root, "params");
    if (!json_is_array(params))
    {
        LOGE("Error: 'params' is not an array.");
        json_decref(root);
        return;
    }

    size_t index;
    json_t *value;
    json_array_foreach(params, index, value)
    {
        if (json_is_object(value))
        {
            json_t *ssid_obj = json_object_get(value, "ssid");
            if (json_is_string(ssid_obj))
            {
                wifi_home->ssid = (char *)json_string_value(ssid_obj);
            }

            json_t *bssid_obj = json_object_get(value, "bssid");
            if (json_is_string(bssid_obj))
            {
                wifi_home->bssid = (char *)json_string_value(bssid_obj);
            }

            json_t *mac_obj = json_object_get(value, "mac");
            if (json_is_string(mac_obj))
            {
                wifi_home->mac = (char *)json_string_value(mac_obj);
            }
        }
    }

    json_decref(root);
}
