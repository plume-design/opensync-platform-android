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
#include "log.h"

/* internal */
#include "osandroid_ipc.h"
#include <jansson.h>

#define MODULE_ID LOG_MODULE_ID_TARGET

bool target_log_pull_ext(const char *upload_location, const char *upload_token, const char *upload_method)
{
    json_t *json_root = NULL;
    char json_res[ZMQ_MSG_MAX_LEN];
    bool ret = false;

    osandroid_ipc_init();

    json_root = json_pack(
            "{s:s, s:{s:s, s:s}}",
            "api",
            __func__,
            "params",
            "upload_location",
            upload_location,
            "upload_token",
            upload_token,
            "upload_method",
            upload_method);

    if (!json_root)
    {
        ret = false;
        goto end;
    }

    ret = osandroid_ipc_request(json_root, json_res, ZMQ_MSG_MAX_LEN);

end:
    if (json_root) json_decref(json_root);

    return ret;
}

bool target_log_pull(const char *upload_location, const char *upload_token)
{
    return target_log_pull_ext(upload_location, upload_token, "lm-awlan");
}
