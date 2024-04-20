package com.opensync.app;

import android.content.Context;
import android.hardware.display.DeviceProductInfo;
import android.hardware.display.DisplayManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.HashSet;
import java.util.Set;


public class OpenSyncConnectDevice {
    private static final String TAG = "OpenSync ConnectDevice";
    private final Context context;
    private Map<String, List<DeviceInfo>> deviceInfoMap;  // phy_iface and DeviceInfo

    public class DeviceInfo {
        private String name;
        private String displayName;
        private String phyIface;
        private String pnpId;
        private String mac;  // BLE
        private String resolution;
        private String refreshRate;
        private boolean isAdded = false;

        public String getName() {
            return name;
        }

        public String getPhyIface() {
            return phyIface;
        }

        public String getPnpId() {
            return pnpId;
        }

        public String getDisplayName() {
            return displayName;
        }

        public boolean getIsAdded() {
            return isAdded;
        }

        public String getMac() {
            return mac;
        }

        public String getResolution() {
            return resolution;
        }

        public String getRefreshRate() {
            return refreshRate;
        }

        public void setName(String name) {
            this.name = (name != null && !name.isEmpty()) ? name : "";
        }

        public void setPhyIface(String phyIface) {
            this.phyIface = (phyIface != null && !phyIface.isEmpty()) ? phyIface : "";
        }

        public void setPnpId(String pnpId) {
            this.pnpId = (pnpId != null) ? pnpId : "";
        }

        public void setDisplayName(String displayName) {
            this.displayName = (displayName != null && !displayName.isEmpty()) ? displayName : "";
        }

        public void setIsAdded(boolean isAdded) {
            this.isAdded = isAdded;
        }

        public void setMac(String mac) {
            this.mac = (mac != null && !mac.isEmpty()) ? mac : "";
        }

        public void setResolution(String resolution) {
            this.resolution = (resolution != null && !resolution.isEmpty()) ? resolution : "";
        }

        public void setRefreshRate(String rate) {
            this.refreshRate = (rate != null && !rate.isEmpty()) ? rate : "";
        }
    }


    public OpenSyncConnectDevice(Context context) {
        this.context = context;
        this.deviceInfoMap = new HashMap<String, List<DeviceInfo>>();

        try {
            // delay 2 seconds to avoid ANDROIDM up late
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public void BLEDeviceUpdate(JSONObject json, BluetoothDevice device) {
        DeviceInfo BLEdevice = new DeviceInfo();

        if (device == null) {
            return;
        }

        String deviceName = device.getName();
        String deviceAddress = device.getAddress();

        List<DeviceInfo> BLEDeviceList = new ArrayList<>();

        BLEdevice.setName(deviceName);
        BLEdevice.setDisplayName(deviceName);
        BLEdevice.setPhyIface("Bluetooth");
        BLEdevice.setPnpId("N/A");
        BLEdevice.setMac(deviceAddress);
        BLEDeviceList.add(BLEdevice);

        if (!BLEDeviceList.isEmpty()) {
            deviceInfoMap.put("Bluetooth", BLEDeviceList);
        }
    }

    public void initializeDisplayInfo() {
        DisplayManager displayManager = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
        Display[] displays = displayManager.getDisplays();
        List<DeviceInfo> HDMIDeviceList;
        HDMIDeviceList = new ArrayList<>();

        if (HDMIDeviceList == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && displays.length > 0) {
            for (Display display : displays) {
                String displayName = display.getName();
                DeviceInfo HDMIdevice = new DeviceInfo();

                DeviceProductInfo displayProdInfo = display.getDeviceProductInfo();

                DisplayMetrics metrics = new DisplayMetrics();
                display.getMetrics(metrics);
                String resolution = metrics.widthPixels + "x" + metrics.heightPixels;

                if (null != displayProdInfo) {
                    HDMIdevice.setName(displayProdInfo.getName());
                    HDMIdevice.setDisplayName(displayProdInfo.getProductId());
                    HDMIdevice.setPhyIface("HDMI");
                    HDMIdevice.setPnpId(displayProdInfo.getManufacturerPnpId());
                    HDMIdevice.setResolution(resolution);
                    HDMIdevice.setRefreshRate(Integer.toString((int)display.getRefreshRate()));
                    HDMIDeviceList.add(HDMIdevice);
                }
            }
        }

        if (!HDMIDeviceList.isEmpty()) {
            deviceInfoMap.put("HDMI", HDMIDeviceList);
        }
    }

    public void initializeUSBInfo() {
        UsbManager mUsbManager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
        List<DeviceInfo> USBDeviceList = new ArrayList<>();

        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
        while (((Iterator<?>) deviceIterator).hasNext()) {
            UsbDevice device = deviceIterator.next();
            DeviceInfo USBdevice = new DeviceInfo();

            USBdevice.setName(device.getProductName());
            USBdevice.setDisplayName(device.getManufacturerName());
            USBdevice.setPhyIface("USB");
            USBdevice.setPnpId(String.valueOf(device.getProductId()));

            USBDeviceList.add(USBdevice);
        }

        if (!USBDeviceList.isEmpty()) {
            deviceInfoMap.put("USB", USBDeviceList);
        }

    }

    public void osandroidGetConnectionDeviceCount() throws JSONException {
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.put("api", "osandroid_get_connection_device_count");
            JSONObject paramsObject = new JSONObject();
            paramsObject.put("count", deviceInfoMap.size());
            jsonObject.put("params", paramsObject);

            jsonObject.put("errCode", "200");
            jsonObject.put("errMsg", "");
        } catch (JSONException e) {
            e.printStackTrace();
            jsonObject.put("errCode", "500");
            jsonObject.put("errMsg", e.getMessage());
        }
    }

    public void BLEJsonPayload(JSONObject json, BluetoothDevice device, boolean isConnected) throws JSONException {

        String phyIface = "Bluetooth";

        if (true == isAdded(phyIface, device.getAddress())) {
            Log.d(TAG, phyIface + " device " + device.getName() + "is added, skipped it");
            return;
        }

        try {
            JSONArray paramsArray = new JSONArray();

            JSONObject param1 = new JSONObject();
            JSONObject schemaAndroidConnectedDevice = new JSONObject();
            param1.put("schema_Peripheral_Device", schemaAndroidConnectedDevice);

            JSONObject product_info = new JSONObject();
            JSONArray map = new JSONArray();

            JSONObject displayName = new JSONObject();
            JSONObject mac = new JSONObject();

            JSONObject param2 = new JSONObject();

            schemaAndroidConnectedDevice.put("name", device.getName());
            schemaAndroidConnectedDevice.put("physical_interface", phyIface);

            displayName.put("displayName", device.getName());
            mac.put("mac", device.getAddress());
            if (isConnected == true) {
                param2.put("associated", true);
            } else {
                param2.put("associated", false);
            }

            map.put(displayName);
            map.put(mac);

            product_info.put("map", map);

            schemaAndroidConnectedDevice.put("product_info", product_info);
            paramsArray.put(param1);
            paramsArray.put(param2);

            json.put("params", paramsArray);
            json.put("errCode", "200");
            json.put("errMsg", "");

        } catch (JSONException e) {
            e.printStackTrace();
            json.put("errCode", "500");
            json.put("errMsg", e.getMessage());
        }
    }

    private boolean isAdded (String phyIface, String key) {
        if (phyIface.equals("Bluetooth") && deviceInfoMap.containsKey("Bluetooth") && !key.isEmpty()) {
            List<DeviceInfo> BLEDeviceList = deviceInfoMap.get("Bluetooth");
            for (DeviceInfo deviceInfo : BLEDeviceList) {
                if (deviceInfo.getName().equals(key) && true == deviceInfo.getIsAdded()) {
                    Log.d(TAG, " BLE device " + deviceInfo.getName() + " is added, we do not send JSON again");
                    return true;
                }
            }
        } else if ((phyIface.equals("HMDI") || phyIface.equals("USB")) && deviceInfoMap.containsKey(phyIface)) {
            List<DeviceInfo> DeviceList = deviceInfoMap.get(phyIface);
            for (DeviceInfo deviceInfo : DeviceList) {
                if (true == deviceInfo.getIsAdded()) {
                    Log.d(TAG, phyIface + " device " + deviceInfo.getName() + " is added, we do not send JSON again");
                    return true;
                }
            }
        }

        return false;
    }

    public void osandroidConnectedDeviceUpdate(JSONObject json, String phyIface) throws JSONException {
        // To avoid duplicated creation
        if (true == isAdded(phyIface, null)) {
            Log.i(TAG, phyIface + " device is added, Skipped it");
            return;
        }

        try {
            JSONArray paramsArray = new JSONArray();

            JSONObject param1 = new JSONObject();
            JSONObject schemaAndroidConnectedDevice = new JSONObject();
            param1.put("schema_Peripheral_Device", schemaAndroidConnectedDevice);

            JSONObject  product_info = new JSONObject();
            JSONArray map = new JSONArray();

            JSONObject pnpId = new JSONObject();
            JSONObject displayName = new JSONObject();
            JSONObject resolution = new JSONObject();
            JSONObject refreshRate = new JSONObject();

            JSONObject param2 = new JSONObject();

            if (deviceInfoMap != null && !deviceInfoMap.isEmpty() && deviceInfoMap.containsKey(phyIface)) {
                List<DeviceInfo> deviceList = deviceInfoMap.get(phyIface);
                for (DeviceInfo deviceInfo : deviceList) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        assert deviceInfo != null;
                        schemaAndroidConnectedDevice.put("name", deviceInfo.getName());
                    }

                    schemaAndroidConnectedDevice.put("physical_interface", deviceInfo.getPhyIface());

                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        pnpId.put("pnpId", deviceInfo.getPnpId());
                        displayName.put("displayName", deviceInfo.getDisplayName());
                        resolution.put("resolution", deviceInfo.getResolution());
                        refreshRate.put("refreshRate", deviceInfo.getRefreshRate());
                    }

                    param2.put("associated", true);
                }
            } else {
                schemaAndroidConnectedDevice.put("name", "");
                schemaAndroidConnectedDevice.put("physical_interface", phyIface);

                pnpId.put("pnpId", "");
                displayName.put("displayName", "");
                resolution.put("resolution", "");
                refreshRate.put("refreshRate", "");
                param2.put("associated", false);
            }

            map.put(pnpId);
            map.put(displayName);
            map.put(resolution);
            map.put(refreshRate);
            product_info.put("map", map);

            schemaAndroidConnectedDevice.put("product_info", product_info);
            paramsArray.put(param1);
            paramsArray.put(param2);

            json.put("params", paramsArray);
            json.put("errCode", "200");
            json.put("errMsg", "");
        } catch (JSONException e) {
            e.printStackTrace();
            json.put("errCode", "500");
            json.put("errMsg", e.getMessage());
        }
    }

    // FIXME: USB/HDMI specific
    public void deleteDevice(String phyIface) {
        if (deviceInfoMap != null && deviceInfoMap.containsKey(phyIface)) {
            deviceInfoMap.remove(phyIface);
            Log.d(TAG, "Delete " + phyIface);
        } else {
            Log.d(TAG, "No " + phyIface + " exist");
        }
    }

    public void deleteBLEdevice(String phyIface, String pnpId) {
        if (deviceInfoMap.containsKey(phyIface) && (phyIface.equals("Bluetooth"))) {
            List<DeviceInfo> deviceList = deviceInfoMap.get(phyIface);
            if (deviceList != null) {
                Iterator<DeviceInfo> iterator = deviceList.iterator();
                while (iterator.hasNext()) {
                    DeviceInfo deviceInfo = iterator.next();
                    if (deviceInfo.getPnpId().equals(pnpId)) {
                        iterator.remove();
                    }
                }
            } else {
                Log.d(TAG, phyIface + " list is NULL");
            }
        }
    }
}
