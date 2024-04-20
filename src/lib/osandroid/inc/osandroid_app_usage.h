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

#ifndef OSANDROID_APP_USAGE_H_INCLUDED
#define OSANDROID_APP_USAGE_H_INCLUDED

#include <stdint.h>
#include <stdio.h>

/* Default data collection for the app usage over one day */
#define OSANDROIDM_APPUSAGE_TIME_DEFAULT_PERIOD 60 * 60 * 24

typedef struct app_usage_entry
{
    char *app_name;
    int launch_count;
    uint64_t foreground_time;
    uint64_t usage_rx_bytes;
    uint64_t usage_tx_bytes;
} app_usage_entry_t;

typedef struct app_usage
{
    int time_period;
    size_t cnt;
    app_usage_entry_t **app_usage_entry;
} app_usage_t;

void osandroid_app_usage_time_period_set(int period);
bool osandroid_app_usage_get(app_usage_t *usage);
bool osandroid_app_usage_json_parse(app_usage_t *usage, const char *rep_buf);

#endif /* OSANDROID_APP_USAGE_H_INCLUDED */
