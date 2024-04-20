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
#include <zmq.h>

/* internal */
#include "ds_list.h"
#include "json_util.h"
#include "memutil.h"
#include "os.h"
#include "util.h"

#include "osandroid_ipc.h"

#define IPC_LOG_TAG "AndroidIPC"
#define randof(num) (int)((float)(num) * random() / (RAND_MAX + 1.0))

static void *req_context = NULL;
static void *requester = NULL;
static char identity[10];

static void *sub_context = NULL;
static void *subscriber = NULL;

static ds_list_t osandroid_ipc_cb_list = DS_LIST_INIT(osandroid_ipc_cb_t, node);

bool osandroid_ipc_requester_init()
{
    int connect_timeout = OSANDROID_IPC_CONNECT_TIMEOUT;
    int recv_timeout = OSANDROID_IPC_RECV_TIMEOUT;
    if (!req_context)
    {
        req_context = zmq_ctx_new();
    }
    else
    {
        LOGI(IPC_LOG_TAG " REQ/REP IPC context already initialized");
    }

    if (!requester)
    {
        requester = zmq_socket(req_context, ZMQ_REQ);

        zmq_setsockopt(requester, ZMQ_CONNECT_TIMEOUT, &connect_timeout, sizeof(connect_timeout));
        zmq_setsockopt(requester, ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

        srand(getpid() + time(NULL));
        sprintf(identity, "%04X-%04X", randof(0x10000), randof(0x10000));
        zmq_setsockopt(requester, ZMQ_IDENTITY, identity, strlen(identity));

        int rc = zmq_connect(requester, CONFIG_ANDROID_ZMQ_REQREP_SOCK_ADDR);
        if (rc == -1)
        {
            LOGE(IPC_LOG_TAG " REQ/REP IPC socket connect failed (%d)", rc);
            return false;
        }
        LOGI(IPC_LOG_TAG " REQ/REP IPC requester initialized successfully ID %s", identity);
    }
    else
    {
        LOGI(IPC_LOG_TAG " REQ/REP IPC requester already initialized");
    }

    return true;
}

static bool osandroid_subscriber_cb(void *arg)
{
    char message[ZMQ_MSG_MAX_LEN] = "";
    int rc;
    osandroid_ipc_cb_t *data;

    if (!sub_context) sub_context = zmq_ctx_new();
    subscriber = zmq_socket(sub_context, ZMQ_SUB);
    while (1)
    {
        rc = zmq_connect(subscriber, CONFIG_ANDROID_ZMQ_PUBSUB_SOCK_ADDR);
        if (rc == 0)
        {
            LOGI(IPC_LOG_TAG " PUB/SUB IPC socket connect successfully");
            break;
        }
        else
        {
            LOGI(IPC_LOG_TAG " Publisher not ready. Need to Zzzz ...");
            sleep(1);
        }
    }

    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    while (1)
    {
        memset(message, 0, ZMQ_MSG_MAX_LEN);
        rc = zmq_recv(subscriber, message, ZMQ_MSG_MAX_LEN, 0);
        if (rc > 0)
        {
            ds_list_foreach(&osandroid_ipc_cb_list, data)
            {
                if (strstr(message, data->api_name))
                {
                    data->callback(message);
                }
            }
        }
    }

    zmq_close(subscriber);
    return true;
}

bool osandroid_subscriber_register(const char *api_name, void (*callback)(const char *))
{
    osandroid_ipc_cb_t *cb = NULL;

    cb = CALLOC(1, sizeof(*cb));
    cb->callback = callback;
    cb->api_name = api_name;
    ds_list_insert_head(&osandroid_ipc_cb_list, cb);

    return true;
}

static bool osandroid_ipc_subscriber_init()
{
    task_id_t tsk_id;
    task_entry_point_t osandroid_subscriber_cb;

    return task_create(&tsk_id, "OSAndroid Subscriber", osandroid_subscriber_cb, NULL);
}

bool osandroid_ipc_init()
{
    bool ret = false;

    ret = osandroid_ipc_requester_init();
    ret &= osandroid_ipc_subscriber_init();

    return ret;
}

bool osandroid_ipc_deinit()
{
    ds_list_iter_t iter;
    osandroid_ipc_cb_t *data;

    if (!req_context) zmq_ctx_destroy(req_context);
    if (!sub_context) zmq_ctx_destroy(sub_context);

    if (!requester) zmq_close(requester);
    if (!subscriber) zmq_close(subscriber);

    for (data = ds_list_ifirst(&iter, &osandroid_ipc_cb_list); data != NULL; data = ds_list_inext(&iter))
    {
        ds_list_iremove(&iter);
        FREE(data);
    }

    return true;
}

bool osandroid_ipc_request(json_t *req, char *rep_buf, size_t rep_len)
{
    if (!req_context)
    {
        LOGE(IPC_LOG_TAG " REQ/REP IPC is not initialized");
        return false;
    }

    if (!req)
    {
        LOGE(IPC_LOG_TAG " Request JSON is NULL");
        return false;
    }

    char *buf = json_dumps(req, 0);
    int len = strlen(buf);
    if (!buf || !len)
    {
        LOGE(IPC_LOG_TAG " JSON request dumps nothing");
        return false;
    }

    int rc = zmq_send(requester, buf, len, 0);
    if (rc == -1)
    {
        int retry = 0;
        while (retry < CONFIG_ANDROID_ZMQ_SOCKET_MAX_RETRY_TIMES)
        {
            retry++;
            LOGW(IPC_LOG_TAG " %s send failed (%d) reason: %s, Need to retry (#%d)...",
                 identity,
                 rc,
                 zmq_strerror(zmq_errno()),
                 retry);

            usleep(CONFIG_ANDROID_ZMQ_SOCKET_MAX_BACKOFF_TIME * 1000);
            if (requester) zmq_close(requester);
            requester = NULL;
            if (osandroid_ipc_requester_init())
            {
                rc = zmq_send(requester, buf, len, 0);
                if (rc != -1)
                {
                    LOGI(IPC_LOG_TAG " %s send&recovery OK (retry %d)", identity, retry);
                    return true;
                }
            }
        }
    }

    if (!rep_buf)
    {
        LOGE(IPC_LOG_TAG " Response buffer cannot be NULL");
        return false;
    }

    if (rep_len <= 0)
    {
        LOGE(IPC_LOG_TAG " Response buffer size must be greater than 0.");
        return false;
    }

    rc = zmq_recv(requester, rep_buf, rep_len, 0);
    if (rc == -1 || rc >= (int)rep_len - 1)
    {
        LOGE(IPC_LOG_TAG " %s", rc == -1 ? "IPC receive failed" : "Buffer overflow");
        return false;
    }

    (rep_buf)[rc] = '\0';

    if (buf) json_free(buf);
    return true;
}

json_t *osandroid_ipc_build_header(const char *fun)
{
    return json_pack("{s:s}", "api", fun);
}
