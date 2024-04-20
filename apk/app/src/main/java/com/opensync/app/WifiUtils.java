package com.opensync.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.util.Log;

import java.util.List;

public class WifiUtils {
    private static final String TAG = "OpenSync WifiUtils";

    private Context mContext;
    private WifiManager mWifiManager;
    private WifiReceiver mWifiReceiver;

    public WifiUtils(Context context) {
        mContext = context;
        mWifiManager = (WifiManager) mContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        mWifiReceiver = new WifiReceiver();
    }

    public void start() {
        if (mWifiManager != null) {
            mContext.registerReceiver(mWifiReceiver, new IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION));
        }
    }

    public void stop() {
        if (mWifiManager != null) {
            mContext.unregisterReceiver(mWifiReceiver);
        }
    }

    public void connectToWifi(String ssid, String password, String bssid, boolean hidden) {
        if (mWifiManager != null) {
            WifiConfiguration mConfig = new WifiConfiguration();
            mConfig.allowedAuthAlgorithms.clear();
            mConfig.allowedGroupCiphers.clear();
            mConfig.allowedKeyManagement.clear();
            mConfig.allowedPairwiseCiphers.clear();
            mConfig.allowedProtocols.clear();
            mConfig.SSID = '"' + ssid + '"';
            mConfig.preSharedKey = '"' + password + '"';;
            if (!bssid.isEmpty())
                mConfig.BSSID = bssid;
            mConfig.hiddenSSID = hidden;
            mConfig.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);
            mConfig.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
            mConfig.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
            mConfig.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
            mConfig.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
            mConfig.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
            mConfig.status = WifiConfiguration.Status.ENABLED;
            int networkId = mWifiManager.addNetwork(mConfig);
            if (networkId != -1) {
                mWifiManager.disconnect();
                mWifiManager.enableNetwork(networkId, true);
                mWifiManager.reconnect();
            } else {
                Log.e(TAG, "Failed to add network configuration");
            }
        }
    }

    private class WifiReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
                NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                if (networkInfo != null && networkInfo.isConnected()) {
                    Log.d(TAG, "Connected to WiFi");
                }
            }
        }
    }
}
