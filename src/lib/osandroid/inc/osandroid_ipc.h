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

#ifndef OSANDROID_IPC_H_INCLUDED
#define OSANDROID_IPC_H_INCLUDED

#include "json_util.h"
#include <ds_list.h>

#define ZMQ_MSG_MAX_LEN 2048

/* IPC Timeout */
#define OSANDROID_IPC_CONNECT_TIMEOUT 2000
#define OSANDROID_IPC_RECV_TIMEOUT 30000

/* Events */
#define ANDROID_EVENT_STA "osandroid_sta"
#define ANDROID_EVENT_STA_CON "osandroid_sta_connected"
#define ANDROID_EVENT_STA_DISCON "osandroid_sta_disconnected"
#define ANDROID_EVENT_PERIPH_DEVICE_UPT "osandroid_peripheral_device_update"
#define ANDROID_EVENT_STREAMING "osandroid_streaming_event"

typedef struct osandroid_ipc_cb
{
    const char *api_name;
    void (*callback)(const char *);
    ds_list_node_t node;
} osandroid_ipc_cb_t;

bool osandroid_ipc_init();
bool osandroid_ipc_requester_init();
bool osandroid_ipc_deinit();
bool osandroid_ipc_request(json_t *req, char *rep_buf, size_t rep_len);
bool osandroid_subscriber_register(const char *api_name, void (*callback)(const char *));
json_t *osandroid_ipc_build_header(const char *fun);

#endif /* OSANDROID_IPC_H_INCLUDED */
