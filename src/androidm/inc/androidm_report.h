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

#ifndef ANDROIDM_REPORT_H_INCLUDED
#define ANDROIDM_REPORT_H_INCLUDED

#include <ev.h>

#define ANDROIDM_RPT_INTERVAL_NODE_KEY "report_interval"

typedef struct androidm_collector
{
    time_t mqtt_interval;
    time_t mqtt_periodic_ts;
    char mqtt_topic[256];
    union
    {
        char apps[256][256];
        bool is_one_time;
    } u;
} androidm_collector_t;

#define INIT_STREAMING_COLLECTOR(collector) \
    do \
    { \
        (collector).mqtt_periodic_ts = time(NULL); \
        (collector).mqtt_interval = 0; \
        strcpy((collector).mqtt_topic, ""); \
        for (int i = 0; i < 256; i++) \
        { \
            memset((collector).u.apps[i], 0, sizeof((collector).u.apps[i])); \
        } \
    } while (0)

#define INIT_APPUSAGE_COLLECTOR(collector) \
    do \
    { \
        (collector).mqtt_periodic_ts = time(NULL); \
        (collector).mqtt_interval = 0; \
        strcpy((collector).mqtt_topic, ""); \
        (collector).u.is_one_time = false; \
    } while (0)

/**
 * @brief Container of protobuf serialization output
 *
 * Contains the information related to a serialized protobuf
 */
struct report_packed_buffer
{
    size_t len;  // length of the serialized protobuf
    void *buf;   // dynamically allocated pointer to serialized data
};

#endif /* ANDROIDM_REPORT_H_INCLUDED */
