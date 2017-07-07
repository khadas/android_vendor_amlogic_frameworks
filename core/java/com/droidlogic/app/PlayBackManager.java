/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class PlayBackManager {
    private static final String TAG             = "PlayBackManager";
    private static final boolean DEBUG          = false;

    private static final String KEY_HDMI_SELFADAPTION = "key_hdmi_selfadaption";
    private static final String SYSF_HDMI_SELFADAPTION = "/sys/class/tv/policy_fr_auto";

    private Context mContext;
    private SystemControlManager mSystenControl;

    public PlayBackManager(Context context){
        mContext = context;
        mSystenControl = new SystemControlManager(context);
    }

    public void initHdmiSelfadaption () {
        if (Settings.System.getInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, 0) == 1) {
            mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, "1");
        } else {
            mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, "0");
        }
    }

    public boolean isHdmiSelfadaptionOn() {
        return Settings.System.getInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, 0) == 1 ? true : false;
    }

    public void setHdmiSelfadaption(boolean on) {
        if (on) {
            mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, "1");
            Settings.System.putInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, 1);
        } else {
            mSystenControl.writeSysFs(SYSF_HDMI_SELFADAPTION, "0");
            Settings.System.putInt(mContext.getContentResolver(), KEY_HDMI_SELFADAPTION, 0);
        }
    }
}
