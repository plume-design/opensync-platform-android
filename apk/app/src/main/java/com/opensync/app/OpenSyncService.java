package com.opensync.app;

import android.app.Service;
import android.app.Notification;
import android.content.BroadcastReceiver;
import androidx.core.app.NotificationCompat;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.Build;
import android.graphics.Color;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

import java.util.*;

import android.hardware.display.DisplayManager;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;


import org.zeromq.SocketType;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZContext;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.media.session.PlaybackState;
import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

public class OpenSyncService extends Service {
    private static final String TAG = "OpenSync Service";
    private static final String EVENT_TAG = "OpenSync Event Service";
    private BroadcastReceiver eventReceiver;
    public  ZContext zContext;
    Socket publisher;
    Thread ipcServerThread;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        OpenSyncBootStrap opensyncCoreExec = new OpenSyncBootStrap(this);
        startForeground();

        /* Bootstrapping OpenSync Core mamangers */
        opensyncCoreExec.preStart();
        opensyncCoreExec.StartProcessHelper();

        /* Start OpenSync IPC Server */
        ipcServerThread = new Thread(new OpenSyncIPCServer(getApplicationContext()));
        ipcServerThread.start();

        /* Event Service */
        zContext = new ZContext();
        publisher = zContext.createSocket(SocketType.PUB);
        publisher.bind("tcp://127.0.0.1:10087");

        eventReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                String api = "";
                String ssid = "";
                String bssid = "";
                String phyIface = "";
                String mac = "";

                if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
                    NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                    if (networkInfo.isConnected()) {
                        WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                        WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                        ssid = wifiInfo.getSSID().replace("\"", "");
                        mac = wifiInfo.getMacAddress();

                        api = "osandroid_sta_connected";
                        if (wifiInfo == null) {
                            Log.i(EVENT_TAG, "Connected to WiFi network but WifiInfo is NULL");
                        } else {
                            Log.i(EVENT_TAG, "Connected to WiFi network: " + ssid);
                            bssid = wifiInfo.getBSSID();
                        }
                    } else {
                        api = "osandroid_sta_disconnected";
                        Log.i(EVENT_TAG, "Disconnected from WiFi network");
                    }

                    try {
                        JSONObject json = new JSONObject();
                        json.put("api", api);
                        JSONArray params = new JSONArray();

                        JSONObject param1 = new JSONObject();
                        param1.put("ssid", ssid);
                        params.put(param1);

                        JSONObject param2 = new JSONObject();
                        param2.put("bssid", bssid);
                        params.put(param2);

                        JSONObject param3 = new JSONObject();
                        param3.put("mac", mac);
                        params.put(param3);

                        json.put("params", params);

                        publisher.send(json.toString(), 0);
                    } catch (JSONException e) {
                        Log.e(EVENT_TAG, "Build JSON failed");
                    }
                }

                if (    action.equals("android.intent.action.HDMI_PLUGGED") ||
                        action.equals("android.intent.action.HDMISTATUS_CHANGED") ) {
                    boolean plugged = intent.getBooleanExtra("state", false);
                    OpenSyncConnectDevice ConnectDevice = new OpenSyncConnectDevice(context);
                    ConnectDevice.initializeDisplayInfo();
                    if (plugged) {
                        Log.i(EVENT_TAG, "HDMI device connected");
                    } else {
                       Log.i(EVENT_TAG, "HDMI device disconnected");
                       ConnectDevice.deleteDevice("HDMI");
                    }

                    api = "osandroid_peripheral_device_update";
                    phyIface = "HDMI";
                    try {
                        JSONObject hdmiJson = new JSONObject();
                        hdmiJson.put("api", api);
                        ConnectDevice.osandroidConnectedDeviceUpdate(hdmiJson, phyIface);

                        if (!hdmiJson.toString().isEmpty()) {
                            publisher.send(hdmiJson.toString(), 0);
                        }
                    } catch (JSONException e) {
                        Log.e(EVENT_TAG, "Build JSON failed");
                    }
                }

                if (    action.equals("android.hardware.usb.action.USB_STATE") ||
                        action.equals("android.hardware.usb.action.USB_DEVICE_ATTACHED") ||
                        action.equals("android.hardware.usb.action.USB_DEVICE_DETACHED") ) {

                    OpenSyncConnectDevice ConnectDevice = new OpenSyncConnectDevice(context);
                    ConnectDevice.initializeUSBInfo();
                    if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED)) {
                        Log.i(EVENT_TAG, "USB device connected");
                    } else if (action.equals(UsbManager.ACTION_USB_DEVICE_DETACHED)) {
                        ConnectDevice.deleteDevice("USB");
                        Log.i(EVENT_TAG, "USB device disconnected");
                    }

                    api = "osandroid_peripheral_device_update";
                    phyIface = "USB";
                    try {
                        JSONObject usbJson = new JSONObject();
                        usbJson.put("api", api);

                        ConnectDevice.osandroidConnectedDeviceUpdate(usbJson, phyIface);
                        if (!usbJson.toString().isEmpty()) {
                            publisher.send(usbJson.toString(), 0);
                        }
                    } catch (JSONException e) {
                        Log.e(EVENT_TAG, "Build JSON failed");
                    }
                }

                if (    action.equals("android.bluetooth.device.action.BOND_STATE_CHANGED") ||
                        action.equals("android.bluetooth.device.action.ACL_DISCONNECTED") ||
                        action.equals("android.bluetooth.device.action.ACL_CONNECTED") ) {
                    OpenSyncConnectDevice ConnectDevice = new OpenSyncConnectDevice(context);
                    api = "osandroid_peripheral_device_update";
                    phyIface = "Bluetooth";
                    JSONObject BLEJson = new JSONObject();
                    if (BluetoothDevice.ACTION_ACL_CONNECTED.equals(action)) {
                        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                        Log.i(EVENT_TAG, "Bluetooth device connected: " + device.getName() + " (" + device.getAddress() + ")");
                        BLEJson = new JSONObject();
                        try {
                            BLEJson.put("api", api);
                            ConnectDevice.BLEDeviceUpdate(BLEJson, device);
                            ConnectDevice.BLEJsonPayload(BLEJson, device, true);
                        } catch (JSONException e) {
                            Log.e(EVENT_TAG, "Build JSON failed");
                        }
                    } else if (BluetoothDevice.ACTION_ACL_DISCONNECTED.equals(action)) {
                        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                        Log.i(EVENT_TAG, "Bluetooth device disconnected: " + device.getName() + " (" + device.getAddress() + ")");
                        try {
                            BLEJson = new JSONObject();
                            BLEJson.put("api", api);
                            ConnectDevice.deleteBLEdevice(phyIface, device.getAddress());
                            ConnectDevice.BLEJsonPayload(BLEJson, device, false);
                        } catch (JSONException e) {
                            Log.e(EVENT_TAG, "Build JSON failed");
                        }
                    }

                    if (!BLEJson.toString().isEmpty() || BLEJson.toString().length() > 5) {
                        publisher.send(BLEJson.toString(), 0);
                    }
                }
            }
        };

        IntentFilter filter = new IntentFilter();
        filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);

        // HDMI
        filter.addAction("android.intent.action.HDMI_PLUGGED");
        filter.addAction("android.intent.action.HDMISTATUS_CHANGED");

        // USB
        filter.addAction("android.hardware.usb.action.USB_STATE");
        filter.addAction("android.hardware.usb.action.USB_DEVICE_ATTACHED");
        filter.addAction("android.hardware.usb.action.USB_DEVICE_DETACHED");

        // BLE
        filter.addAction("android.bluetooth.device.action.ACL_CONNECTED");
        filter.addAction("android.bluetooth.device.action.ACL_DISCONNECTED");
        filter.addAction("android.bluetooth.device.action.BOND_STATE_CHANGED");

        try {
            Thread.sleep(10000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        registerReceiver(eventReceiver, filter);

        OpenSyncStreamingEvent openSyncStreamingEvent = new OpenSyncStreamingEvent(this);
        EventBus.getDefault().register(this);
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onEventBusPlaybackState(PlaybackState state) {
        Log.d(EVENT_TAG, "PlaybackState: " + state);
        OpenSyncStreaming openSyncStreaming = new OpenSyncStreaming(this);

        JSONObject streamingEvent = new JSONObject();;
        try {
            streamingEvent.put("api", "osandroid_streaming_event");
            openSyncStreaming.osandroidStreamingGet(streamingEvent);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        publisher.send(streamingEvent.toString(), 0);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (ipcServerThread != null) {
            ipcServerThread.interrupt();
        }

        if (publisher != null) {
            publisher.close();
        }

        if (zContext != null) {
            zContext.close();
        }

        if (eventReceiver != null) {
            unregisterReceiver(eventReceiver);
        }

        EventBus.getDefault().unregister(this);
    }

    private void startForeground() {
        String channelId = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            channelId = createNotificationChannel("opensync", "ForegroundService");
        } else {
            channelId = "";
        }
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, channelId);
        Notification notification = builder.setOngoing(true)
                .setSmallIcon(R.mipmap.opensync_launcher)
                .setPriority(NotificationManager.IMPORTANCE_MIN)
                .setCategory(Notification.CATEGORY_SERVICE)
                .build();
        startForeground(1, notification);
    }

    private String createNotificationChannel(String channelId, String channelName) {
        NotificationChannel chan = new NotificationChannel(channelId,
                channelName, NotificationManager.IMPORTANCE_NONE);
        chan.setLightColor(Color.BLUE);
        chan.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
        NotificationManager service = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        service.createNotificationChannel(chan);
        return channelId;
    }
}
