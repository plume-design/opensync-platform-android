package com.opensync.app;

import android.content.Context;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;
import android.os.Bundle;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

import java.util.*;


public class OpenSyncPlatformAPI {
    private static final String TAG = "OpenSync PlatformAPI";
    private Context context;

    public OpenSyncPlatformAPI(Context context) {
        this.context = context;
    }

    public String UnitSerialGet() {
        String serialNumber = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            serialNumber = Build.getSerial();
        } else {
            serialNumber = Settings.Secure.getString(context.getContentResolver(),
                    Settings.Secure.ANDROID_ID);
        }

        return serialNumber;
    }

    public String UnitIdGet() {
        try {
            WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (wifiManager == null) {
                throw new NullPointerException("WifiManager is null");
            }

            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if (wifiInfo == null) {
                throw new NullPointerException("WifiInfo is null");
            }

            String macAddress = wifiInfo.getMacAddress();
            if (macAddress == null) {
                throw new NullPointerException("MacAddress is null");
            }
            return macAddress.toUpperCase().replaceAll(":", "");
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public String UnitModelGet() {
        String deviceModel = Build.BRAND;
        return deviceModel;
    }

    public String UnitSkuGet() {
        String sku = Build.PRODUCT;
        return sku;
    }

    public String UnitHwRevisionGet() {
        return "N/A";
    }

    public String UnitPlatformVersionGet() {
        String androidVersion = android.os.Build.VERSION.CODENAME;
        return androidVersion;
    }

    public String UnitSwVersionGet() {
        String version = Build.VERSION.RELEASE + "." + Build.VERSION.SDK + "-" + Build.VERSION.INCREMENTAL + "-" + Build.ID;
        return version;
    }

    public String UnitVendorNameGet() {
        String manufacturer = Build.MANUFACTURER;
        return manufacturer;
    }

    public String UnitVendorPartGet() {
        return "N/A";
    }

    public String UnitManufacturerGet() {
        String manufacturer = Build.MANUFACTURER;
        return manufacturer;
    }

    public String UnitFactoryGet() {
        String factory = Build.MANUFACTURER;
        return factory;
    }

    public String UnitMfgDateGet() {
        return "N/A";
    }

    public String UnitDhcpcHostnameGet() {
        return "N/A";
    }
}
