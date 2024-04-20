package com.opensync.app;

import android.util.Log;
import android.os.Build;
import android.app.ActivityManager;
import android.content.ComponentName;
import android.content.Context;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.media.MediaMetadata;
import android.os.Build;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.TrafficStats;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;

import org.json.JSONException;
import org.json.JSONArray;
import org.json.JSONObject;

public class OpenSyncStreaming {
    private static final String TAG = "OpenSyncStreaming";
    private static Context context;

    private MediaSessionManager mediaSessionManager;
    private MediaController mediaController;

    public OpenSyncStreaming(Context context) {
        OpenSyncStreaming.context = context;
        mediaSessionManager = (MediaSessionManager) context.getSystemService(Context.MEDIA_SESSION_SERVICE);
    }

    public void osandroidStreamingGet(JSONObject jsonObject) throws JSONException {
        String appName = getPlayingVideoAppName(context);

        Log.i(TAG, "APP: " + appName);
        packResponseJson(jsonObject, "app_name", appName);

        mediaController = isAppPlayingVideo(appName);
        if (mediaController != null) {
            MediaController.PlaybackInfo playbackInfo = mediaController.getPlaybackInfo();
            PlaybackState playbackState = mediaController.getPlaybackState();
            MediaMetadata metaData = mediaController.getMetadata();

            if (metaData != null) {
                /* Title/Channel */
                String title = metaData.getString(MediaMetadata.METADATA_KEY_TITLE);
                Log.d(TAG, "Title/Channel: " + title);
                packResponseJson(jsonObject, "title_channel", title);

                /* Playback duration seconds */
                long total_duration = metaData.getLong(MediaMetadata.METADATA_KEY_DURATION);
                Log.d(TAG, "Total Video Duration(ms): " + total_duration);
                packResponseJson(jsonObject, "total_duration", total_duration);
            } else {
                Log.w(TAG, "MediaMetadata is null");
            }

            /* Video Resolution */
            int resol_width = (int) parseAmlVdecStatus("frame width", false);
            int resol_height = (int) parseAmlVdecStatus("frame height", false);
            Log.d(TAG, "Video Resolution: " + resol_width + "*" + resol_height);
            packResponseJson(jsonObject, "resol_width", resol_width);
            packResponseJson(jsonObject, "resol_height", resol_height);

            /* Video frame rate */
            int frame_rate = (int) parseAmlVdecStatus("frame rate", false);
            Log.d(TAG, "Frame Rate: " + frame_rate);
            packResponseJson(jsonObject, "frame_rate", frame_rate);

            /* Frames Count */
            int frame_count = (int) parseAmlVdecStatus("frame count", false);
            Log.d(TAG, "Frame Count: " + frame_count);
            packResponseJson(jsonObject, "frame_count", frame_count);

            /* Dropped Frames */
            int dropped_frames = (int) parseAmlVdecStatus("drop count", false);
            Log.d(TAG, "Dropped Frames: " + dropped_frames);
            packResponseJson(jsonObject, "dropped_frames", dropped_frames);

            /* Error Frames */
            int error_frames = (int) parseAmlVdecStatus("fra err count", false);
            Log.d(TAG, "Error Frames: " + error_frames);
            packResponseJson(jsonObject, "error_frames", error_frames);

            /* Render Time (ms) */

            /* Startup Time */
            long startupTime = OpenSyncStreamingEvent.mStartupTime;
            Log.d(TAG, "Startup Time: " + startupTime);
            packResponseJson(jsonObject, "start_time", startupTime);

            /* TX-RX Byte */
            PackageManager packageManager = context.getPackageManager();
            try {
                ApplicationInfo appInfo = packageManager.getApplicationInfo(appName, 0);
                long txBytes = TrafficStats.getUidTxBytes(appInfo.uid);
                long rxBytes = TrafficStats.getUidRxBytes(appInfo.uid);
                Log.d(TAG, "txBytes: " + txBytes);
                Log.d(TAG, "rxBytes: " + rxBytes);
                packResponseJson(jsonObject, "tx_bytes", txBytes);
                packResponseJson(jsonObject, "rx_bytes", rxBytes);
            } catch (NameNotFoundException e) {
                e.printStackTrace();
            }

            /* Audio bit rate */

            /* Video bit rate */
            int video_bit_rate = (int) parseAmlVdecStatus("bit rate", false);
            Log.d(TAG, "Video Bitrate: " + video_bit_rate);
            packResponseJson(jsonObject, "video_bit_rate", video_bit_rate);

            /* Audio Sample Rate */

            /* Codec */
            String codec = (String) parseAmlVdecStatus("device name", true);
            Log.d(TAG, "Codec: " + codec);
            packResponseJson(jsonObject, "codec", codec);

            /* Playback */
            if (playbackInfo != null) {
                Log.d(TAG, "Playback Info: " + playbackInfo.toString());
                /* Volume */
                int max_vol = playbackInfo.getMaxVolume();
                int cur_vol = playbackInfo.getCurrentVolume();
                int vol_percent = (cur_vol / max_vol) * 100;
                Log.d(TAG, "Current Volume(%): " + vol_percent);
                packResponseJson(jsonObject, "volume", vol_percent);
            }

            if (playbackState != null) {
                Log.d(TAG, "Playback State: " + playbackState.toString());
                /* Video playback speed */
                float speed = playbackState.getPlaybackSpeed();
                Log.d(TAG, "Playback Speed: " + speed);
                packResponseJson(jsonObject, "play_speed", speed);

                /* State Flag */
                int pb_state = playbackState.getState();
                Log.d(TAG, "Playback State Flag: " + pb_state);
                packResponseJson(jsonObject, "state", pb_state);

                /* Realtime Duration */
                long duration = playbackState.getPosition();
                Log.d(TAG, "Realtime Duration(ms): " + duration);
                packResponseJson(jsonObject, "duration", duration);

                /* Video Buffering time (ms) */
                long buffering_time = playbackState.getBufferedPosition();
                Log.d(TAG, "Playback Buffering time: " + buffering_time);
                packResponseJson(jsonObject, "buffering_time", buffering_time);

                /* Error */
                CharSequence error = playbackState.getErrorMessage();
                if (error != null) {
                    Log.d(TAG, "Playback Error: " + error);
                    packResponseJson(jsonObject, "error", error);
                }
                fillError(jsonObject, "200", "");
            } else {
                Log.w(TAG, "PlaybackState is null");
            }
        } else {
            packResponseJson(jsonObject, "state", PlaybackState.STATE_NONE);
            fillError(jsonObject, "404", "APP is not playing back.");
            packResponseJson(jsonObject, "error", "APP is not playing back.");
            return;
        }
    }

    private static Object parseAmlVdecStatus(String key, boolean asString) {
        String filePath = "/sys/class/vdec/vdec_status";
        try (BufferedReader br = new BufferedReader(new FileReader(filePath))) {
            String line;
            while ((line = br.readLine()) != null) {
                if (line.contains(key)) {
                    String[] parts = line.split(":");
                    if (parts.length == 2) {
                        String value = parts[1].trim();
                        if (asString) {
                            return value;
                        } else {
                            value = value.replaceAll("[^0-9]", "");
                            if (!value.isEmpty()) {
                                return Integer.parseInt(value);
                            } else {
                                Log.e(TAG, "AmlVdec parse failed: " + value);
                                return 0;
                            }
                        }
                    }
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return asString ? null : 0;
    }

    public static JSONObject packResponseJson(JSONObject inputJson, String key, Object value) throws JSONException {
        try {
            JSONArray paramsArray = inputJson.optJSONArray("params");
            if (paramsArray == null) {
                paramsArray = new JSONArray();
                inputJson.put("params", paramsArray);
            }

            JSONObject streamingInfoObject = null;
            if (paramsArray.length() > 0) {
                streamingInfoObject = paramsArray.optJSONObject(0).optJSONObject("streaming_info");
            } else {
                streamingInfoObject = new JSONObject();
                paramsArray.put(new JSONObject().put("streaming_info", streamingInfoObject));
            }

            streamingInfoObject.put(key, value);

            return inputJson;
        } catch (JSONException e) {
            e.printStackTrace();
            inputJson.put("errCode", "500");
            inputJson.put("errMsg", e.getMessage());
            return inputJson;
        }
    }

    private void fillError(JSONObject jsonObject, String code, String reason) throws JSONException {
        jsonObject.put("errCode", code);
        jsonObject.put("errMsg", reason);
        return;
    }

    private static String getPlayingVideoAppName(Context context) {
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        if (am != null) {
            List<ActivityManager.RunningTaskInfo> runningTasks = am.getRunningTasks(1);
            if (runningTasks != null && !runningTasks.isEmpty()) {
                return runningTasks.get(0).topActivity.getPackageName();
            }
        }
        return null;
    }

    private MediaController isAppPlayingVideo(String packageName) {
        if (mediaSessionManager != null) {
            List<MediaController> controllers = mediaSessionManager
                    .getActiveSessions(new ComponentName(context, OpenSyncStreaming.class));

            for (MediaController controller : controllers) {
                if (controller != null && controller.getPackageName().equals(packageName)) {
                    Log.d(TAG, "Media Session: " + controller.getPackageName());
                    PlaybackState pbs = controller.getPlaybackState();
                    if (pbs != null) {
                        int state = pbs.getState();
                        if (state != PlaybackState.STATE_NONE &&
                                state != PlaybackState.STATE_STOPPED) {
                            Log.d(TAG, "PlaybackState is " + state);
                            return controller;
                        }
                    } else {
                        Log.w(TAG, "PlaybackState is null");
                        return controller;
                    }
                }
            }
        }
        return null;
    }
}
