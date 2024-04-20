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

#ifndef ANDROIDM_H_INCLUDED
#define ANDROIDM_H_INCLUDED

#include <ev.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "androidm_app_usage.h"
#include "androidm_report.h"
#include "androidm_streaming.h"
#include "ds_tree.h"

// Intervals and timeouts in seconds
#define ANDROIDM_TIMER_INTERVAL 1
#define ANDROIDM_MQTT_INTERVAL 60
#define ANDROIDM_MODEM_INFO_INTERVAL 60

typedef enum
{
    MQTT_NO_HEADER = -1,
    MQTT_HEADER_LOCATION_ID = 0,
    MQTT_HEADER_NODE_ID = 1,
    MQTT_NUM_HEADER_IDS = 2,
} mqtt_header_ids_t;

/**
 * @brief Manager's global context
 *
 * The manager keeps track of the various sessions.
 */
typedef struct androidm_mgr
{
    char location_id[64];
    char node_id[64];

    struct ev_loop *loop;
    ev_timer timer;          // manager's general event timer
    time_t periodic_ts;      // periodic timestamp
    char pid[16];            // manager's pid
    struct sysinfo sysinfo;  // system information
    time_t qm_backoff;       // backoff interval on qm connection errors

    androidm_collector_t streaming_collector;
    androidm_collector_t app_usage_collector;
} android_mgr_t;

android_mgr_t *androidm_get_mgr(void);
void androidm_event_init(void);

#endif /* ANDROIDM_H_INCLUDED */
