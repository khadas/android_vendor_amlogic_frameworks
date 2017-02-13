package com.droidlogic.app.tv;

import android.content.Context;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.util.Log;


public class DroidLogicHdmiCecManager {
    private static final String TAG = "DroidLogicHdmiCecManager";

    private Context mContext;
    private HdmiControlManager mHdmiControlManager;
    private HdmiTvClient mTvClient;
    private int mSelectPort = -1;
    private int mSourceType = 0;

    private final Object mLock = new Object();

    private static DroidLogicHdmiCecManager mInstance = null;

    public static synchronized DroidLogicHdmiCecManager getInstance(Context context) {
        if (mInstance == null) {
            Log.d(TAG, "mInstance is null...");
            mInstance = new DroidLogicHdmiCecManager(context);
        }
        Log.d(TAG, "mInstance is not null");
        return mInstance;
    }

    public DroidLogicHdmiCecManager(Context context) {
        mContext = context;
        mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);

        if (mHdmiControlManager != null)
            mTvClient = mHdmiControlManager.getTvClient();
    }

    /**
     * select hdmi cec port.
     * @param deviceId defined in {@link DroidLogicTvUtils} {@code DEVICE_ID_HDMI1} {@code DEVICE_ID_HDMI2}
     * {@code DEVICE_ID_HDMI3} or 0(TV).
     * @return {@value true} indicates has select device successfully, otherwise {@value false}.
     */
    public boolean selectHdmiDevice(final int deviceId) {
        synchronized (mLock) {
            getInputSourceType();

            Log.d(TAG, "selectHdmiDevice"
                + ", deviceId = " + deviceId
                + ", mSelectPort = " + mSelectPort
                + ", mSourceType = " + mSourceType);

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
                    if (addr == 0 || result != HdmiControlManager.RESULT_SUCCESS)
                        mSelectPort = 0;
                    else
                        mSelectPort = deviceId;
                    Log.d(TAG, "select device, onComplete result = " + result + ", mSelectPort = " + mSelectPort);
                }
            });
            return true;
        }
    }

    /**
     * when hdmi is disconnected or switched to another source, reset the cec selected status.
     * the function will invoked by some different works as follows.
     * 1. plug out the hdmi.
     * 2. tv has stopped because of something. such as source is switched to another.
     */
    public void disconnectHdmiCec(int deviceId) {
        synchronized (mLock) {
          //only disconnect hdmi device.
            if (deviceId < DroidLogicTvUtils.DEVICE_ID_HDMI1 || deviceId > DroidLogicTvUtils.DEVICE_ID_HDMI3)
                return;

            getInputSourceType();
            Log.d(TAG, "disconnectHdmiCec, deviceId = " + deviceId
                    + ", mSourceType = " + mSourceType
                    + ", mSelectPort = " + mSelectPort);
            selectHdmiDevice(0);
        }
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

    public int getInputSourceType() {
        mSourceType = Settings.System.getInt(mContext.getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, 0);
        return mSourceType;
    }
}
