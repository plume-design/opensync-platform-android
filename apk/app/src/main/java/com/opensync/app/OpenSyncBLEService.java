package com.opensync.app;

import android.app.Service;
import android.content.Context;

import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;

import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattService;

import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;

import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelUuid;

import android.util.Log;

import java.util.UUID;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;

public class OpenSyncBLEService {

    private static final String TAG = "OpenSync Bluetooth Service";
    private static final String CHARACTERISTIC = "0000FE71-0000-1000-8000-00805F9B34FB";

    private static final String BLUETOOTH_SERVICE = Context.BLUETOOTH_SERVICE;

    private BleAdvertisingData advData;
    private final Context context;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeAdvertiser bluetoothLeAdvertiser;
    private BluetoothGattServer mGattServer;
    private BluetoothGattServerCallback mGattServerCallback;

    public class BleAdvertisingData {
        private String msgType;
        private String msg;

        /* Enable advertising or not */
        private boolean mode = false;

        /* Connectable mode is not possible if the pairing token cannot be generated */
        private boolean connectable = false;
        private int intervalMillis = -1;

        /* APIs */
        public void setMode(boolean modeState) {
            mode = modeState;
        }

        public void setConnectable(boolean connState) {
            connectable = connState;
        }

        public void setInterval(int interval) {
            intervalMillis = interval;
        }

        public void setMsgType(String buff) {
            msgType = buff;
        }

        public void setMsg(String buff) {
            msg = buff;
        }

        public boolean getMode() {
            return mode;
        }

        public boolean getConnectable() {
            return connectable;
        }

        public int getInterval() {
            return intervalMillis;
        }

        private byte[] convertToByteArray(String input) {
            int len = input.length();
            ByteBuffer buffer = ByteBuffer.allocate(len);
            for (int i = 0; i < len; i++) {
                char c = input.charAt(i);
                buffer.put((byte) c);
            }
            return buffer.array();
        }

        private byte[] hexStringToByteArray(String hexString) {
            int len = hexString.length();
            byte[] byteArray = new byte[len / 2];

            for (int i = 0; i < len; i += 2) {
                byteArray[i / 2] = (byte) ((Character.digit(hexString.charAt(i), 16) << 4)
                        + Character.digit(hexString.charAt(i + 1), 16));
            }

            return byteArray;
        }

        private byte[] getManufacturerDataBytes() {
            ByteBuffer buffer = ByteBuffer.allocate(20).order(ByteOrder.LITTLE_ENDIAN);

            buffer.put((byte) 0x05);  // Version

            OpenSyncPlatformAPI openSyncPlatformAPI = new OpenSyncPlatformAPI(context);
            byte[] serialNum = convertToByteArray(openSyncPlatformAPI.UnitIdGet());
            for (byte serialByte : serialNum) {
                buffer.put(serialByte);
            }

            byte[] msgTypeByteArray = hexStringToByteArray(msgType);
            buffer.put(msgTypeByteArray);  // MsgType

            byte[] msgByteArray = hexStringToByteArray(msg);
            for (byte msgByte : msgByteArray) {
                buffer.put(msgByte);
            }

            return buffer.array();
        }
    }

    public void enableBLEAdvertising(JSONObject jsonObject) throws JSONException {
        boolean modeState = false;
        boolean connState = false;
        int interval = -1;

        try {
            modeState = jsonObject.getBoolean("mode");
            advData.setMode(modeState);

            connState = jsonObject.getBoolean("connectable");
            advData.setConnectable(connState);

            interval = jsonObject.getInt("interval_millis");
            advData.setInterval(interval);
        } catch (JSONException e) {
            e.printStackTrace();
            jsonObject.put("errCode", "500");
            jsonObject.put("errMsg", "JSON parsing error");
        }

        if (advData.getMode()) {
            stopBLEAdvertising();
            startBLEAdvertising();
        } else {
            stopBLEAdvertising();
        }
    }

    public void setBLEAdvertisingData(JSONObject jsonObject) throws JSONException {
        String msgType = null;
        String msg = null;
        Boolean connectable = advData.getConnectable();
        int intervalMillis = advData.getInterval();

        try {
            msgType = jsonObject.getString("msgType");
            advData.setMsgType(msgType);

            msg = jsonObject.getString("msg");
            advData.setMsg(msg);
        } catch (JSONException e) {
            e.printStackTrace();
            jsonObject.put("errCode", "500");
            jsonObject.put("errMsg", "JSON parsing error");
        }

        if (advData.getMode()) {
            stopBLEAdvertising();
            startBLEAdvertising();
        } else {
            stopBLEAdvertising();
        }

    }

    private AdvertiseCallback advertiseCallback = new AdvertiseCallback() {
        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            super.onStartSuccess(settingsInEffect);
            Log.i(TAG, "Advertising started successfully");

        }

        @Override
        public void onStartFailure(int errorCode) {
            super.onStartFailure(errorCode);
            String errMsg = null;

            switch (errorCode) {
                case AdvertiseCallback.ADVERTISE_FAILED_FEATURE_UNSUPPORTED:
                    errMsg = "Feature unsupported";
                    break;
                case AdvertiseCallback.ADVERTISE_FAILED_TOO_MANY_ADVERTISERS:
                    errMsg = "Too many advertisers";
                    break;
                case AdvertiseCallback.ADVERTISE_FAILED_ALREADY_STARTED:
                    errMsg = "Already started";
                    break;
                case AdvertiseCallback.ADVERTISE_FAILED_DATA_TOO_LARGE:
                    errMsg = "Data too large";
                    break;
                case AdvertiseCallback.ADVERTISE_FAILED_INTERNAL_ERROR:
                    errMsg = "Internal error";
                    break;
                default:
                    errMsg = "Unknown error";
            }
            Log.e(TAG, "Advertising failed with error (" + errorCode  + ") errMsg: " + errMsg);
        }
    };

    private void startBLEAdvertising() {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (bluetoothAdapter == null || !bluetoothAdapter.isMultipleAdvertisementSupported() || !bluetoothAdapter.isEnabled()) {
            Log.e(TAG, "Not Support BLE ! ");
            return;
        }

        bluetoothLeAdvertiser = bluetoothAdapter.getBluetoothLeAdvertiser();

         AdvertiseSettings settings = new AdvertiseSettings.Builder()
             .setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
             .setConnectable(true)  // different from pairing connectable
             .setTimeout(0)
             .build();

         AdvertiseData data = new AdvertiseData.Builder()
             .setIncludeDeviceName(false)
             .addServiceUuid(new ParcelUuid(UUID.fromString(CHARACTERISTIC)))
             .addManufacturerData(0x0A17, advData.getManufacturerDataBytes())
             .build();

        if (bluetoothLeAdvertiser != null) {
            // start advertising
            bluetoothLeAdvertiser.startAdvertising(settings, data, advertiseCallback);
        }

    }

    public void startGattServer() {
        mGattServerCallback = new BluetoothGattServerCallback() {
            @Override
            public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
                super.onConnectionStateChange(device, status, newState);

                if (device == null) {
                    return;
                }

                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    Log.d(TAG, "Device connected: " + device.getAddress());
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    Log.d(TAG, "Device disconnected: " + device.getAddress());
                    closeGattServer();
                }
            }

            @Override
            public void onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset,
                    BluetoothGattCharacteristic characteristic) {
                super.onCharacteristicReadRequest(device, requestId, offset, characteristic);
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                        characteristic.getValue());
            }

            @Override
            public void onDescriptorReadRequest(BluetoothDevice device, int requestId, int offset,
                    BluetoothGattDescriptor descriptor) {
                super.onDescriptorReadRequest(device, requestId, offset, descriptor);
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                        descriptor.getValue());
            }
        };

        mGattServer = ((BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE)).openGattServer(context, mGattServerCallback);
        if (mGattServer != null) {
            setupGattServer();
        } else {
            Log.e(TAG, "Unable to initialize BluetoothGattServer.");

        }
    }

    private void setupGattServer() {
        // Add GATT SERVICE UUID
        BluetoothGattService plumeService = new BluetoothGattService(
                UUID.fromString("0000FE71-0000-1000-8000-00805f9b34fb"),
                BluetoothGattService.SERVICE_TYPE_PRIMARY
        );

        // JSON CHARACTERISTIC
        BluetoothGattCharacteristic jsonCharacteristic = new BluetoothGattCharacteristic(
                UUID.fromString("E5E01504-FE71-4A90-9898-376FA622DE9C"),
                BluetoothGattCharacteristic.PROPERTY_READ | BluetoothGattCharacteristic.PROPERTY_WRITE | BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                BluetoothGattCharacteristic.PERMISSION_READ | BluetoothGattCharacteristic.PERMISSION_WRITE
        );

        plumeService.addCharacteristic(jsonCharacteristic);

        mGattServer.addService(plumeService);
    }



    public OpenSyncBLEService(Context context) {
        this.context = context;
        this.advData = new BleAdvertisingData();
        startGattServer();
    }

    private void closeGattServer() {
        if (mGattServer != null) {
            mGattServer.close();
            mGattServer = null;
        }
    }

    private void stopBLEAdvertising() {
        Log.d(TAG, "BLE advertising stopped.");
        if (bluetoothAdapter == null || advertiseCallback == null) {
            // advertising not started
            return;
        }

        if (bluetoothLeAdvertiser != null) {
            bluetoothLeAdvertiser.stopAdvertising(advertiseCallback);
            bluetoothLeAdvertiser = null;
        }

        // close Gatt Server
        closeGattServer();
    }
}

