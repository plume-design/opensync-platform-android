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

#include "ds_tree.h"
#include "json_util.h"
#include "kconfig.h"
#include "log.h"
#include "module.h"
#include "os.h"
#include "os_backtrace.h"
#include "os_socket.h"
#include "ovsdb.h"
#include "target.h"
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
#include "androidm_peripheral.h"
#include "osandroid_ipc.h"

#ifdef CONFIG_ANDROIDM_APP_REPORT
#include "androidm_app_usage.h"
#endif /* CONFIG_ANDROIDM_APP_REPORT */

#ifdef CONFIG_ANDROIDM_STREAMING_REPORT
#include "androidm_streaming.h"
#endif /* CONFIG_ANDROIDM_STREAMING_REPORT */

/*****************************************************************************/
#define MODULE_ID LOG_MODULE_ID_MAIN
#define TARGET_INIT_MGR_ANDROIDM 99
/*****************************************************************************/

static log_severity_t androidm_log_severity = LOG_SEVERITY_INFO;

/******************************************************************************
 *  PROTECTED definitions
 *****************************************************************************/
static android_mgr_t android_mgr = {0};

android_mgr_t *androidm_get_mgr(void)
{
    return &android_mgr;
}

static bool androidm_init_mgr(struct ev_loop *loop)
{
    android_mgr_t *mgr = androidm_get_mgr();

    mgr->loop = loop;

    return true;
}

/******************************************************************************
 *  PUBLIC API definitions
 *****************************************************************************/
int main(int argc, char **argv)
{
    struct ev_loop *loop = EV_DEFAULT;

    /* Parse command-line arguments */
    if (os_get_opt(argc, argv, &androidm_log_severity))
    {
        return -1;
    }

    /* enable logging */
    target_log_open("ANDROIDM", 0);
    LOGN("Starting  manager - ANDROIDM");
    log_severity_set(androidm_log_severity);

    /* Enable runtime severity updates */
    log_register_dynamic_severity(loop);

    /* Install crash handlers that dump the stack to the log file */
    backtrace_init();

    json_memdbg_init(loop);

    androidm_init_mgr(loop);
    /* Register to relevant OVSDB tables events */
    if (!ovsdb_init_loop(loop, "ANDROIDM"))
    {
        LOGE("Initializing ANDROIDM "
             "(Failed to initialize OVSDB)");
        return -1;
    }

    /* Android IPC Init */
    osandroid_ipc_init();

    /* Module Init */
    module_init();

    /* Start the event loop */
    if (kconfig_enabled(CONFIG_ANDROIDM_APP_REPORT) || kconfig_enabled(CONFIG_ANDROIDM_STREAMING_REPORT))
    {
        androidm_event_init();
    }
    ev_run(loop, 0);

    // exit:
    if (!ovsdb_stop_loop(loop))
    {
        LOGE("Stopping ANDROIDM "
             "(Failed to stop OVSDB");
    }

    target_close(TARGET_INIT_MGR_ANDROIDM, loop);

    ev_default_destroy();

    LOGN("Exiting ANDROIDM");

    return 0;
}
