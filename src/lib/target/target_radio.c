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
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/limits.h>
#include <linux/un.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

/* 3rd party */
#include <ev.h>
#include <zmq.h>

/* internal */
#include <jansson.h>
#include <os.h>
#include <ovsdb_table.h>
#include <schema.h>
#include <target.h>
#include <util.h>

#include "target_init.h"
#include "target_radio.h"
#include "target_util.h"

#include "osandroid_ipc.h"

struct ev_loop *target_mainloop;
struct target_radio_ops rops;

struct kvstore
{
    struct ds_dlist_node list;
    char key[64];
    char val[512];
};

static ev_timer g_util_cb_timer;
static ds_dlist_t g_kvstore_list = DS_DLIST_INIT(struct kvstore, list);

#define MODULE_ID LOG_MODULE_ID_TARGET
#define TARGET_INIT_MGR_ANDROIDM 99

/******************************************************************************
 * Key-value store
 *****************************************************************************/

static struct kvstore *util_kv_get(const char *key)
{
    struct kvstore *i;
    ds_dlist_foreach (&g_kvstore_list, i)
        if (!strcmp(i->key, key)) return i;
    return NULL;
}

static void util_kv_set(const char *key, const char *val)
{
    struct kvstore *i;

    if (!key) return;

    if (!(i = util_kv_get(key)))
    {
        i = MALLOC(sizeof(*i));
        ds_dlist_insert_tail(&g_kvstore_list, i);
    }

    if (!val)
    {
        ds_dlist_remove(&g_kvstore_list, i);
        FREE(i);
        LOGT("%s: '%s'=nil", __func__, key);
        return;
    }

    STRSCPY(i->key, key);
    STRSCPY(i->val, val);
    LOGT("%s: '%s'='%s'", __func__, key, val);
}

/******************************************************************************
 * Target delayed callback helpers
 *****************************************************************************/
bool target_vif_state_get(char *vif, struct schema_Wifi_VIF_State *vstate)
{
    const struct kvstore *kv;

    memset(vstate, 0, sizeof(*vstate));

    vstate->_partial_update = true;

    SCHEMA_SET_STR(vstate->if_name, vif);

    kv = util_kv_get("bridge");
    if (kv != NULL) SCHEMA_SET_STR(vstate->bridge, kv->val);
    kv = util_kv_get("parent");
    if (kv != NULL) SCHEMA_SET_STR(vstate->parent, kv->val);
    kv = util_kv_get("ssid");
    if (kv != NULL) SCHEMA_SET_STR(vstate->ssid, kv->val);
    kv = util_kv_get("mac");
    if (kv != NULL) SCHEMA_SET_STR(vstate->mac, kv->val);

    return true;
}

bool target_radio_state_get(char *phy, struct schema_Wifi_Radio_State *rstate)
{
    memset(rstate, 0, sizeof(*rstate));

    rstate->_partial_update = true;

    SCHEMA_SET_STR(rstate->if_name, phy);

    return true;
}

static void util_cb_vif_state_update(const char *vif)
{
    struct schema_Wifi_VIF_State vstate;

    LOGI("%s: updating state", vif);

    target_vif_state_get(WIFI_STA_IFNAME, &vstate);

    if (rops.op_vstate) rops.op_vstate(&vstate, WIFI_PHY_IFNAME);
}

static void util_cb_phy_state_update(const char *phy)
{
    struct schema_Wifi_Radio_State rstate;

    LOGI("%s: updating state", phy);

    target_radio_state_get((char *)phy, &rstate);

    if (rops.op_rstate) rops.op_rstate(&rstate);
}

static void util_cb_delayed_update_timer(struct ev_loop *loop, ev_timer *timer, int revents)
{
    const struct kvstore *kv;
    char *ifname;
    char *type;
    char *p;
    char *q;
    char *i;

    if (!(kv = util_kv_get(UTIL_CB_KV_KEY))) return;

    p = strdupa(kv->val);
    util_kv_set(UTIL_CB_KV_KEY, NULL);

    /* The ordering is intentional here. It
     * reduces the churn when vif states are
     * updated, e.g. due to channel change events
     * in which case updating phy will need to be
     * done once afterwards.
     */

    q = strdupa(p);
    while ((i = strsep(&q, " ")))
        if ((type = strsep(&i, ":")) && !strcmp(type, UTIL_CB_VIF) && (ifname = strsep(&i, "")))
            util_cb_vif_state_update(ifname);

    q = strdupa(p);
    while ((i = strsep(&q, " ")))
        if ((type = strsep(&i, ":")) && !strcmp(type, UTIL_CB_PHY) && (ifname = strsep(&i, "")))
            util_cb_phy_state_update(ifname);
}

static void util_cb_delayed_update(const char *type, const char *ifname)
{
    const struct kvstore *kv;
    char buf[512];
    char *p;
    char *i;

    if ((kv = util_kv_get(UTIL_CB_KV_KEY)))
    {
        STRSCPY(buf, kv->val);
        p = strdupa(buf);
        while ((i = strsep(&p, " ")))
            if (!strcmp(i, strfmta("%s:%s", type, ifname))) break;
        if (i)
        {
            LOGD("%s: delayed update already scheduled", ifname);
            return;
        }
    }
    else
    {
        ev_timer_init(&g_util_cb_timer, util_cb_delayed_update_timer, UTIL_CB_DELAY_SEC, 0);
        ev_timer_start(target_mainloop, &g_util_cb_timer);
    }

    LOGD("%s: scheduling delayed update '%s' += '%s:%s'", ifname, kv ? kv->val : "", type, ifname);
    STRSCAT(buf, " ");
    STRSCAT(buf, type);
    STRSCAT(buf, ":");
    STRSCAT(buf, ifname);
    util_kv_set(UTIL_CB_KV_KEY, buf);
}

static void target_wm_event_cb(const char *message)
{
    target_home_wifi_t wifi_config = {NULL, NULL, NULL};

    util_wifi_getter(message, strlen(message), &wifi_config);

    if (strstr(message, ANDROID_EVENT_STA_CON))
    {
        /* Update STA parent, channel, bridge */
        util_kv_set("ssid", wifi_config.ssid);
        util_kv_set("mac", wifi_config.mac);
        util_kv_set("parent", wifi_config.bssid);
        util_kv_set("bridge", "\0");
        util_cb_delayed_update(UTIL_CB_VIF, WIFI_STA_IFNAME);

        LOGI("Android EVENT: %s", ANDROID_EVENT_STA_CON);
        const char *cmd = "/data/user/0/com.opensync.app/files/opensync/tools/ovsh "
                          "d Connection_Manager_Uplink\n"
                          "/data/user/0/com.opensync.app/files/opensync/tools/ovsh "
                          "d Wifi_Inet_State\n"
                          "/data/user/0/com.opensync.app/files/opensync/tools/ovsh "
                          "i Connection_Manager_Uplink if_name:=eth0 "
                          "if_type:=eth "
                          "has_L2:=true "
                          "has_L3:=true "
                          "is_used:=true "
                          "ntp_state:=true "
                          "priority:=11\n"
                          "/data/user/0/com.opensync.app/files/opensync/tools/ovsh "
                          "i Wifi_Inet_State if_name:=eth0 "
                          "if_type:=eth "
                          "network:=true "
                          "enabled:=true "
                          "if_uuid:=e881c038-c17d-11ed-afa1-0242ac120002 "
                          "ip_assign_scheme:=dhcp "
                          "NAT:=true "
                          "inet_addr:=\"192.168.1.251\" "
                          "hwaddr:=\"d4:cf:f9:60:61:ab\" "
                          "netmask:=\"255.255.255.0\"";
        LOGI("Android EVENT: WR: Update WAN Info:\n %s", cmd);
        system(cmd);
    }
    else if (strstr(message, ANDROID_EVENT_STA_DISCON))
    {
        LOGI("Android EVENT: %s", ANDROID_EVENT_STA_DISCON);
        /* Clear STA parent, channel, bridge */
        util_kv_set("parent", "");
        util_kv_set("ssid", "");
        util_kv_set("mac", "");
        util_kv_set("bridge", "null");
        util_cb_delayed_update(UTIL_CB_VIF, WIFI_STA_IFNAME);
    }
}

/******************************************************************************
 * Target APIs
 *****************************************************************************/

void target_init_wm(struct ev_loop *loop)
{
    target_mainloop = loop;

    osandroid_ipc_init();
    osandroid_subscriber_register(ANDROID_EVENT_STA, target_wm_event_cb);
}

bool target_radio_init(const struct target_radio_ops *ops)
{
    rops = *ops;

    return true;
}

bool target_radio_config_set2(
        const struct schema_Wifi_Radio_Config *rconf,
        const struct schema_Wifi_Radio_Config_flags *changed)
{
    struct schema_Wifi_Radio_State rstate = {0};

    LOGI("phy: %s: configuring", rconf->if_name);

#define CPY_INT(n) SCHEMA_CPY_INT(rstate.n, rconf->n)
#define CPY_STR(n) SCHEMA_CPY_STR(rstate.n, rconf->n)
    rstate._partial_update = true;
    rstate.channel_mode_present = false;
    rstate.channel_sync_present = false;

    CPY_INT(enabled);
    CPY_INT(channel);
    CPY_INT(dfs_demo);
    CPY_INT(tx_chainmask);

    CPY_STR(country);
    CPY_STR(ht_mode);
    CPY_STR(if_name);
    CPY_STR(freq_band);
    CPY_STR(hw_mode);
#undef CPY_INT
#undef CPY_STR

    if (rops.op_rstate) rops.op_rstate(&rstate);

    return true;
}

bool target_vif_config_set2(
        const struct schema_Wifi_VIF_Config *vconf,
        const struct schema_Wifi_Radio_Config *rconf,
        const struct schema_Wifi_Credential_Config *cconfs,
        const struct schema_Wifi_VIF_Config_flags *changed,
        int num_cconfs)
{
    struct schema_Wifi_VIF_State vstate = {0};
    bool ret = false;
    json_t *json_root = NULL;
    char json_res[ZMQ_MSG_MAX_LEN];

    LOGI("vif: %s: configuring", vconf->if_name);

#define CPY_INT(n) SCHEMA_CPY_INT(vstate.n, vconf->n)
#define CPY_STR(n) SCHEMA_CPY_STR(vstate.n, vconf->n)
#define CPY_MAP(n) SCHEMA_CPY_MAP(vstate.n, vconf->n)
#define CPY_LIST(n) SCHEMA_CPY_LIST(vstate.n, vconf->n)

    vstate._partial_update = true;

    SCHEMA_SET_STR(vstate.mac, "00:11:22:33:44:55");  // FIXME
    CPY_INT(ap_bridge);
    CPY_INT(btm);
    CPY_INT(dynamic_beacon);
    CPY_INT(enabled);
    CPY_INT(ft_mobility_domain);
    CPY_INT(ft_psk);
    CPY_INT(group_rekey);
    CPY_INT(mcast2ucast);
    CPY_INT(radius_srv_port);
    CPY_INT(rrm);
    CPY_INT(wds);
    CPY_INT(wpa);
    CPY_INT(wps);
    CPY_INT(wps_pbc);
    CPY_INT(vif_radio_idx);
    CPY_STR(pmf);
    SCHEMA_SET_BOOL(vstate.rsn_pairwise_ccmp, vconf->rsn_pairwise_ccmp);
    SCHEMA_SET_BOOL(vstate.wpa_pairwise_ccmp, vconf->wpa_pairwise_ccmp);
    SCHEMA_SET_BOOL(vstate.wpa_pairwise_tkip, vconf->wpa_pairwise_tkip);
    SCHEMA_SET_BOOL(vstate.rsn_pairwise_tkip, vconf->rsn_pairwise_tkip);
    CPY_STR(dpp_connector);
    CPY_STR(dpp_csign_hex);
    CPY_STR(dpp_netaccesskey_hex);
    CPY_STR(if_name);
    CPY_STR(min_hw_mode);
    CPY_STR(mode);
    CPY_STR(multi_ap);
    CPY_STR(radius_srv_addr);
    CPY_STR(radius_srv_secret);
    CPY_STR(ssid);
    CPY_STR(ssid_broadcast);
    CPY_STR(wps_pbc_key_id);
    CPY_LIST(mac_list);
    CPY_LIST(wpa_key_mgmt);
    CPY_MAP(security);
    CPY_MAP(wpa_psks);
#undef CPY_INT
#undef CPY_STR

    if ((changed->ssid || changed->wpa_psks || changed->parent || changed->bridge || changed->security)
        && (strcmp(vconf->if_name, WIFI_STA_IFNAME) == 0))
    {
        LOGI("vif: %s@%s: configuring Android Wi-Fi Station SSID: %s", vconf->if_name, rconf->if_name, vconf->ssid);
        json_root = json_pack(
                "{s:s, s:[{s:{s:s, s:s, s:s, s:s, s:s}}, {s:{s:s, s:s, s:i}}]}",
                "api",
                __func__,
                "params",
                "wifi_vif_config",
                "if_name",
                vconf->if_name,
                "ssid",
                vconf->ssid,
                "security",
                vconf->security[1],
                "wpa_psks",
                vconf->wpa_psks[0],
                "parent",
                vconf->parent,
                "wifi_radio_config",
                "if_name",
                rconf->if_name,
                "freq_band",
                rconf->freq_band,
                "channel",
                rconf->channel);
        if (!json_root)
        {
            LOGE("%s: Failed to pack JSON data", __func__);
            return false;
        }

        ret = osandroid_ipc_request(json_root, json_res, ZMQ_MSG_MAX_LEN);
        if (!ret)
        {
            LOGE("%s: IPC request failed", __func__);
            goto end;
        }
    }

    if (rops.op_vstate) rops.op_vstate(&vstate, rconf->if_name);
    ret = true;

end:
    if (json_root) json_decref(json_root);
    return ret;
}
