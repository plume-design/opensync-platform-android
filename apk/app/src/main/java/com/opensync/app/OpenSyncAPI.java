package com.opensync.app;

import android.content.Context;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONArray;
import org.json.JSONObject;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.function.Consumer;

public class OpenSyncAPI {
    private static final String TAG = "OpenSync API";
    private Map<String, Consumer<JSONObject>> apiHandlers;
    private Context context;
    private OpenSyncPlatformAPI openSyncPlatformAPI;
    private OpenSyncTargetAPI openSyncTargetAPI;
    private OpenSyncStreaming openSyncStreaming;
    private OpenSyncAPPUsage openSyncAPPUsage;
    private OpenSyncBLEService openSyncBLEService;

    public OpenSyncAPI(Context context) {
        this.context = context;
        openSyncPlatformAPI = new OpenSyncPlatformAPI(context);
        openSyncTargetAPI = new OpenSyncTargetAPI(context);
        openSyncStreaming = new OpenSyncStreaming(context);
        openSyncAPPUsage = new OpenSyncAPPUsage(context);
        openSyncBLEService = new OpenSyncBLEService(context);

        apiHandlers = new HashMap<>();

        apiHandlers.put("osp_unit_serial_get", this::ospUnitSerialGet);
        apiHandlers.put("osp_unit_id_get", this::ospUnitIdGet);
        apiHandlers.put("osp_unit_model_get", this::ospUnitModelGet);
        apiHandlers.put("osp_unit_sku_get", this::ospUnitSkuGet);
        apiHandlers.put("osp_unit_hw_revision_get", this::ospUnitHwRevisionGet);
        apiHandlers.put("osp_unit_platform_version_get", this::ospUnitPlatformVersionGet);
        apiHandlers.put("osp_unit_sw_version_get", this::ospUnitSwVersionGet);
        apiHandlers.put("osp_unit_vendor_name_get", this::ospUnitVendorNameGet);
        apiHandlers.put("osp_unit_vendor_part_get", this::ospUnitVendorPartGet);
        apiHandlers.put("osp_unit_manufacturer_get", this::ospUnitManufacturerGet);
        apiHandlers.put("osp_unit_factory_get", this::ospUnitFactoryGet);
        apiHandlers.put("osp_unit_mfg_date_get", this::ospUnitMfgDateGet);
        apiHandlers.put("osp_unit_dhcpc_hostname_get", this::ospUnitDhcpcHostnameGet);

        apiHandlers.put("target_vif_config_set2", this::targetVifConfigSet2);
        apiHandlers.put("osandroid_streaming_get", this::osandroidStreamingGet);
        apiHandlers.put("osandroid_app_usage_get", this::osandroidAppUsageGet);

        apiHandlers.put("osp_ble_set_advertising_params", this::ospBleSetAdvertisingParams);
        apiHandlers.put("osp_ble_set_advertising_data", this::ospBleSetAdvertisingData);

        apiHandlers.put("target_log_pull_ext", this::targetLogPullExt);
    }

    public String Dispatch(String jsonContent) {
        try {
            JSONObject jsonObject = new JSONObject(jsonContent);
            String apiName = jsonObject.getString("api");

            Consumer<JSONObject> handler = apiHandlers.get(apiName);

            if (handler != null) {
                handler.accept(jsonObject);
            } else {
                generateErrorResponse(jsonObject, "400", "Unknown API: " + apiName);
            }

            Log.d(TAG, "OUT: " + jsonObject.toString());
            return jsonObject.toString();
        } catch (JSONException e) {
            Log.e(TAG, "JSON Parsing Error: " + e.getMessage());
            return "{\"errCode\":\"500\",\"errMsg\":\"JSON Parsing Error:\"}";
        }
    }

    private void OspPackResponseJson(JSONObject jsonObject, String string) {
        JSONObject paramsObject = new JSONObject();
        try {
            paramsObject.put("buff", string);
        } catch (JSONException e) {
            generateErrorResponse(jsonObject, "500", "JSON Pack: " + "Error adding argv: " + e.getMessage());
        }

        JSONArray paramsArray = new JSONArray();
        paramsArray.put(paramsObject);

        try {
            jsonObject.put("params", paramsArray);
        } catch (JSONException e) {
            generateErrorResponse(jsonObject, "500", "JSON Pack" + "Error putting params into main JSON: " + e.getMessage());
        }

        try {
            jsonObject.put("errCode", "200");
            jsonObject.put("errMsg", "");
        } catch (JSONException e) {
            generateErrorResponse(jsonObject, "500", "JSON Pack" + "Error adding error code: " + e.getMessage());
        }
    }

    private void ospUnitSerialGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitSerialGet());
    }

    private void ospUnitIdGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitIdGet());
    }

    private void ospUnitModelGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitModelGet());
    }

    private void ospUnitSkuGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitSkuGet());
    }

    private void ospUnitHwRevisionGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitHwRevisionGet());
    }

    private void ospUnitPlatformVersionGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitPlatformVersionGet());
    }

    private void ospUnitSwVersionGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitSwVersionGet());
    }

    private void ospUnitVendorNameGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitVendorNameGet());
    }

    private void ospUnitVendorPartGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitVendorPartGet());
    }

    private void ospUnitManufacturerGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitManufacturerGet());
    }

    private void ospUnitFactoryGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitFactoryGet());
    }

    private void ospUnitMfgDateGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitMfgDateGet());
    }

    private void ospUnitDhcpcHostnameGet(JSONObject jsonObject) {
        OspPackResponseJson(jsonObject, openSyncPlatformAPI.UnitDhcpcHostnameGet());
    }

    private void targetVifConfigSet2(JSONObject jsonObject) {
        try {
            openSyncTargetAPI.targetVifConfigSet2(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "targetVifConfigSet2" + e.getMessage());
        }
    }

    private void osandroidStreamingGet(JSONObject jsonObject) {
        try {
            openSyncStreaming.osandroidStreamingGet(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "osandroidStreamingGet" + e.getMessage());
        }
    }

    private void osandroidAppUsageGet(JSONObject jsonObject) {
        try {
            openSyncAPPUsage.osandroidAppUsageGet(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "osandroidAppUsageGet" + e.getMessage());
        }
    }

    private void ospBleSetAdvertisingParams(JSONObject jsonObject) {
        try {
            openSyncBLEService.enableBLEAdvertising(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "ospBleSetAdvertisingParams" + e.getMessage());
        }
    }

    private void ospBleSetAdvertisingData(JSONObject jsonObject) {
        try {
            openSyncBLEService.setBLEAdvertisingData(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "ospBleSetAdvertisingData" + e.getMessage());
        }
    }

    private void targetLogPullExt(JSONObject jsonObject) {
        try {
            openSyncTargetAPI.targetLogPullExt(jsonObject);
        } catch (JSONException e) {
            Log.e(TAG, "targetLogPullExt" + e.getMessage());
        }
    }

    private void generateErrorResponse(JSONObject jsonObject, String errCode, String errMsg) {
        try {
            jsonObject.put("errCode", errCode);
            jsonObject.put("errMsg", errMsg);
        } catch (JSONException e) {
            Log.e(TAG + " JSON Pack", "Error adding error code: " + e.getMessage());
        }
    }
}
