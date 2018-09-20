package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class PlayBackManager {
    private static final String TAG             = "PlayBackManager";
    private static final boolean DEBUG          = false;

    private static final String KEY_HDMI_SELFADAPTION = "key_hdmi_selfadaption";
    private static final String SYSF_HDMI_SELFADAPTION = "/sys/class/tv/policy_fr_auto";

    public static final int MODE_OFF = 0;
    public static final int MODE_PART = 1;
    public static final int MODE_TOTAL = 2;

    private Context mContext;
    private SystemControlManager mSystenControl;

    public PlayBackManager(Context context){
        mContext = context;
        mSystenControl = new SystemControlManager(context);
    }

    public void initHdmiSelfadaption () {
        mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, Integer.toString(getHdmiSelfAdaptionMode()));
    }

    public int getHdmiSelfAdaptionMode() {
        return Settings.System.getInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, 0);
    }

    public void setHdmiSelfadaption(int mode) {
        mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, Integer.toString(mode));
        Settings.System.putInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, mode);
    }
}
