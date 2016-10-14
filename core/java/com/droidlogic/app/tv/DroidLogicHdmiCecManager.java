package com.droidlogic.app.tv;

import android.content.Context;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.provider.Settings.Global;
import android.util.Log;


public class DroidLogicHdmiCecManager {
    private static final String TAG = "DroidLogicHdmiCecManager";

    private Context mContext;
    private HdmiControlManager mHdmiControlManager;
    private HdmiTvClient mTvClient;
    private static int mSelectPort = -1;

    public DroidLogicHdmiCecManager(Context context) {
        mContext = context;
        mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);

        if (mHdmiControlManager != null)
            mTvClient = mHdmiControlManager.getTvClient();
    }

    /**
     * select hdmi cec port.
     * @param deviceId defined in {@link DroidLogicTvUtils} {@code DEVICE_ID_HDMI1} {@code DEVICE_ID_HDMI2}
     * or {@code DEVICE_ID_HDMI3}.
     * @return {@value true} indicates has select device successfully, otherwise {@value false}.
     */
    public boolean selectHdmiDevice(final int deviceId) {
        Log.d(TAG, "selectHdmiDevice, deviceId = " + deviceId + ", mSelectPort = " + mSelectPort);

        int devAddr = 0;
        if (mHdmiControlManager == null || mSelectPort == deviceId)
            return false;

        boolean cecOption = (Global.getInt(mContext.getContentResolver(), Global.HDMI_CONTROL_ENABLED, 1) == 1);
        if (!cecOption || mTvClient == null)
            return false;

        devAddr = getLogicalAddress(deviceId);
        Log.d(TAG, "mSelectPort = " + mSelectPort + ", devAddr = " + devAddr);

        if (mSelectPort < 0 && devAddr == 0)
            return false;

        final int addr = devAddr;
        mTvClient.deviceSelect(devAddr, new SelectCallback() {
            @Override
            public void onComplete(int result) {
                Log.d(TAG, "select device, onComplete result = " + result);
                if (addr == 0)
                    mSelectPort = 0;
                else
                    mSelectPort = deviceId;
            }
        });
        return true;
    }

    public void disconnectHdmiCec() {
        selectHdmiDevice(0);
    }

    public int getLogicalAddress (int deviceId) {
        if (deviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1 && deviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI3) {
            int id = deviceId - DroidLogicTvUtils.DEVICE_ID_HDMI1 + 1;
            for (HdmiDeviceInfo info : mTvClient.getDeviceList()) {
                if (id == (info.getPhysicalAddress() >> 12)) {
                    return info.getLogicalAddress();
                }
            }
        }
        return 0;
    }

    public boolean hasHdmiCecDevice(int deviceId) {
        if (deviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1 && deviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI3) {
            int id = deviceId - DroidLogicTvUtils.DEVICE_ID_HDMI1 + 1;
            for (HdmiDeviceInfo info : mTvClient.getDeviceList()) {
                if (id == (info.getPhysicalAddress() >> 12)) {
                    return true;
                }
            }
        }
        return false;
    }
}
