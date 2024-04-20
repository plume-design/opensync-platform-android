package com.opensync.app;

import android.content.Context;
import android.net.Network;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiNetworkSpecifier;

import android.os.Build;
import android.net.MacAddress;

import android.util.Log;
import android.os.Build;
import java.util.List;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;

public class OpenSyncTargetAPI {
    private static final String TAG = "OpenSync Target API";

    private Context context;
    private WifiManager wifiManager;
    private LogUtils logUtils;
    private String osyncOBSSID;

    public OpenSyncTargetAPI(Context context) {
        this.context = context;
        this.osyncOBSSID = BuildConfig.OSYNC_OBSSID;
        wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
    }

    public void targetVifConfigSet2(JSONObject jsonObject) throws JSONException {
        String ssid = null;
        String bssid = null;
        String onboard_psk = null;
        String wpa_psk = null;
        String freqBand = null;
        int channel = 0;

        try {
            JSONArray paramsArray = jsonObject.getJSONArray("params");
            for (int i = 0; i < paramsArray.length(); i++) {
                JSONObject configObject = paramsArray.getJSONObject(i);

                if (configObject.has("wifi_vif_config")) {
                    JSONObject wifiVifConfig = configObject.getJSONObject("wifi_vif_config");
                    ssid = wifiVifConfig.getString("ssid");
                    bssid = wifiVifConfig.getString("parent");
                    onboard_psk = wifiVifConfig.getString("security");
                    wpa_psk = wifiVifConfig.getString("wpa_psks");
                }

                if (configObject.has("wifi_radio_config")) {
                    JSONObject wifiRadioConfig = configObject.getJSONObject("wifi_radio_config");
                    freqBand = wifiRadioConfig.getString("freq_band");
                    channel = wifiRadioConfig.getInt("channel");
                }
            }

            /* Skip onboarding if device already has Internet */
            if (osyncOBSSID != null && ssid.equals(osyncOBSSID)) {
                ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);

                if (connectivityManager != null) {
                    NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
                    if ((activeNetworkInfo != null) && (activeNetworkInfo.isConnected())) {
                        Log.i(TAG, "Skip onboarding since device already has Internet");
                        jsonObject.put("errCode", "200");
                        jsonObject.put("errMsg", "");
                        return;
                    }
                }
            }

            WifiUtils wifiConfig = new WifiUtils(context);
            if (!onboard_psk.isEmpty()) {
                wifiConfig.connectToWifi(ssid, onboard_psk, bssid, true);
            } else {
                wifiConfig.connectToWifi(ssid, wpa_psk, bssid, false);

                List<WifiConfiguration> list = wifiManager.getConfiguredNetworks();
                for (WifiConfiguration i : list) {
                    if (osyncOBSSID != null && i.SSID.equals("\"" + osyncOBSSID + "\"")) {
                        Log.i(TAG, "Removed OpenSync onboard network");
                        wifiManager.removeNetwork(i.networkId);
                    }
                }
            }
            jsonObject.put("errCode", "200");
            jsonObject.put("errMsg", "");
        } catch (JSONException e) {
            e.printStackTrace();
            jsonObject.put("errCode", "500");
            jsonObject.put("errMsg", "JSON parsing error");
        }
    }

    public void targetLogPullExt(JSONObject jsonObject) throws JSONException {
        String uploadLocation = null;
        String uploadToken = null;
        logUtils = new LogUtils(context);

        try {
            JSONObject paramsObject = jsonObject.getJSONObject("params");
            if (paramsObject.has("upload_location")) {
                uploadLocation = paramsObject.getString("upload_location");
                logUtils.setUploadLocation(uploadLocation);
            }

            if (paramsObject.has("upload_token")) {
                uploadToken = paramsObject.getString("upload_token");
                logUtils.setUploadToken(uploadToken);
            }

            // Start logpull
            logUtils.doLogpull();

            jsonObject.put("errCode", "200");
            jsonObject.put("errMsg", "");
        } catch (JSONException e) {
            e.printStackTrace();
            jsonObject.put("errCode", "500");
            jsonObject.put("errMsg", "JSON parsing error");
        }
    }
}
