package com.opensync.app;

import android.content.Intent;
import android.content.BroadcastReceiver;
import android.util.Log;
import android.content.Context;
import android.os.Build;

public class OpenSyncAutoStart extends BroadcastReceiver {
    private static final String TAG = "OpenSync AutoStart";

    public void onReceive(Context context, Intent arg1) {
        Intent intent = new Intent(context, OpenSyncService.class);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(intent);
        } else {
            context.startService(intent);
        }
        Log.i(TAG, "OpenSync started");
    }
}
