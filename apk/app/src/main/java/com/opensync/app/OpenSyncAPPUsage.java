package com.opensync.app;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

import android.net.TrafficStats;
import android.app.usage.UsageStats;
import android.app.usage.UsageStatsManager;
import android.content.Context;
import android.util.Log;

import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.*;
import org.json.*;

import android.app.usage.NetworkStats;
import android.app.usage.NetworkStatsManager;
import android.net.ConnectivityManager;

public class OpenSyncAPPUsage {
    private static final String TAG = "OpenSync APPUsage";
    private static PackageManager packageManager;
    private static Context context;
    private static final SimpleDateFormat dateFormat = new SimpleDateFormat("MM-dd-yyyy HH:mm:ss");
    private static ArrayList<AppQOEInfo> appQoeList;
    private static long AppStartTime;
    private static long AppEndTime;

    public static class AppQOEInfo {
        private String packageName;
        private long totalTimeInForeground;
        private int launchCount;
        private long usageTxBytes;
        private long usageRxBytes;

        public AppQOEInfo(String packageName, long totalTimeInForeground, int launchCount, long usageTxBytes, long usageRxBytes) {
            this.packageName = packageName;
            this.totalTimeInForeground = totalTimeInForeground;
            this.launchCount = launchCount;
            this.usageTxBytes = usageTxBytes;
            this.usageRxBytes = usageRxBytes;
        }

        public String getPackageName() {
            return packageName;
        }

        public long getTotalTimeInForeground() {
            return totalTimeInForeground;
        }

        public int getLaunchCount() {
            return launchCount;
        }

        public long getUsageRxBytes() {
            return usageRxBytes;
        }

        public long getUsageTxBytes() {
            return usageTxBytes;
        }
    }

    public OpenSyncAPPUsage(Context context) {
        this.AppStartTime = 0;
        this.AppEndTime = 0;
        this.context = context;
        this.appQoeList = new ArrayList<>();
    }

    public void osandroidAppUsageGet(JSONObject jsonObject) throws JSONException {
        int timePeriod = -1;

        // Get interval from osp
        try {
            JSONObject paramsObject = jsonObject.getJSONObject("params");
            timePeriod = paramsObject.getInt("time_period");
        } catch (Exception e) {
            e.printStackTrace();
        }
        setTime(timePeriod);

        // Get Usage and Network
        appQoeList = getAppQOEInfo();
        if (appQoeList == null) {
            Log.e(TAG, "APP Usage List is null");
            return;
        }

        try {
            jsonObject.put("api", "osandroid_app_usage_get");

            JSONArray paramsArray = new JSONArray();
            jsonObject.put("params", paramsArray);

            JSONObject timePeriodObject = new JSONObject();
            timePeriodObject.put("time_period", timePeriod);
            paramsArray.put(timePeriodObject);

            // Add entry
            JSONArray appUsageArray = new JSONArray();
            for (AppQOEInfo appUsageInfo : appQoeList) {
                JSONObject appEntry = new JSONObject();
                appEntry.put("app_name", appUsageInfo.getPackageName());
                appEntry.put("launch_count", appUsageInfo.getLaunchCount());
                appEntry.put("foreground_time", appUsageInfo.getTotalTimeInForeground());
                appEntry.put("usage_rx_bytes", appUsageInfo.getUsageRxBytes());
                appEntry.put("usage_tx_bytes", appUsageInfo.getUsageTxBytes());

                appUsageArray.put(appEntry);
            }

            JSONObject appUsageObject = new JSONObject();
            appUsageObject.put("app_usage", appUsageArray);
            paramsArray.put(appUsageObject);
            jsonObject.put("params", paramsArray);

            jsonObject.put("errCode", "200");
            jsonObject.put("errMsg", "");
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    private static void setTime(int timePeriod) {
        AppStartTime = System.currentTimeMillis() - timePeriod * 1000;
        AppEndTime = System.currentTimeMillis();
    }

    private static int getLaunchCount(UsageStats usageStats) throws NoSuchFieldException {
        Field field;
        try {
            field = UsageStats.class.getDeclaredField("mLaunchCount");
            field.setAccessible(true);
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
            throw e;
        }

        try {
            return (int) field.get(usageStats);
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            throw new NoSuchFieldException("mLaunchCount field not accessible");
        }
    }

    public static ArrayList<AppQOEInfo> getAppQOEInfo() {
        ArrayList<AppQOEInfo> list = new ArrayList<>();
        Log.d(TAG, "Range start:   " + dateFormat.format(AppStartTime)+ " AppStartTime = " + AppStartTime);
        Log.d(TAG, "Range end:" + dateFormat.format(AppEndTime) + "AppEndTime= " + AppEndTime);

        packageManager = context.getPackageManager();
        if (packageManager == null) {
            Log.e(TAG, "packageManager is empty");
            return null;
        }

        UsageStatsManager usageStatsManager = (UsageStatsManager) context.getSystemService(Context.USAGE_STATS_SERVICE);
        NetworkStatsManager networkStatsManager = (NetworkStatsManager) context.getSystemService(Context.NETWORK_STATS_SERVICE);

        if (usageStatsManager == null) {
            Log.e(TAG, "UsageStatsManager is null");
            return list;
        }

        Map<String, UsageStats> statsMap;
        try {
            statsMap = usageStatsManager.queryAndAggregateUsageStats(AppStartTime, AppEndTime);
        } catch (SecurityException e) {
            e.printStackTrace();
            return list;
        }

        if (statsMap == null) {
            Log.e(TAG, "statsMap is null");
            return list;
        }

        for (Map.Entry<String, UsageStats> entry : statsMap.entrySet()) {
            UsageStats stats = entry.getValue();
            if (stats != null && stats.getTotalTimeInForeground() > 0) {
                try {
                    ApplicationInfo appInfo = packageManager.getApplicationInfo(stats.getPackageName(), 0);

                    NetworkStats networkStats = networkStatsManager.queryDetailsForUid(
                            ConnectivityManager.TYPE_WIFI,
                            null,
                            AppStartTime,
                            AppEndTime,
                            appInfo.uid);

                    long usageTxBytes = 0;
                    long usageRxBytes = 0;
                    while (networkStats.hasNextBucket()) {
                        NetworkStats.Bucket bucket = new NetworkStats.Bucket();
                        networkStats.getNextBucket(bucket);
                        usageTxBytes += bucket.getTxBytes();
                        usageRxBytes += bucket.getRxBytes();
                    }

                    AppQOEInfo appUsageInfo = new AppQOEInfo(
                            stats.getPackageName(),
                            stats.getTotalTimeInForeground(),
                            getLaunchCount(stats),
                            usageTxBytes,
                            usageRxBytes
                    );

                    list.add(appUsageInfo);
                } catch (NoSuchFieldException | NameNotFoundException e)  {
                    e.printStackTrace();
                }
            }
        }
        return list;
    }
}
