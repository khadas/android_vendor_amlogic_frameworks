/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Luan.Yuan
 *  @version  1.0
 *  @date     2017/02/21
 *  @par function description:
 *  - This is SDR Manager, control SDR feature, sysFs, proc env.
 */
package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class SdrManager {
    private static final String TAG                 = "SdrManager";

    private static final String KEY_SDR_MODE        = "key_sdr_mode";
    private static final String SYSF_SDR_MODE       = "/sys/module/am_vecm/parameters/sdr_mode";

    public static final int MODE_OFF = 0;
    public static final int MODE_AUTO = 2;

    private Context mContext;
    private SystemControlManager mSystenControl;

    public SdrManager(Context context){
        mContext = context;
        mSystenControl = new SystemControlManager(context);
    }

    public void initSdrMode() {
        switch (Settings.System.getInt(mContext.getContentResolver(), KEY_SDR_MODE, MODE_OFF)) {
            case MODE_OFF:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_OFF));
                break;
            case MODE_AUTO:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_AUTO));
                break;
            default:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_OFF));
                break;
        }
    }

    public int getSdrMode() {
        switch (Settings.System.getInt(mContext.getContentResolver(), KEY_SDR_MODE, MODE_OFF)) {
            case 0:
                return MODE_OFF;
            case 2:
                return MODE_AUTO;
            default:
                return MODE_OFF;
        }
    }

    public void setSdrMode(int mode) {
        switch (mode) {
            case MODE_OFF:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_OFF));
                Settings.System.putInt(mContext.getContentResolver(), KEY_SDR_MODE, MODE_OFF);
                break;
            case MODE_AUTO:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_AUTO));
                Settings.System.putInt(mContext.getContentResolver(), KEY_SDR_MODE, MODE_AUTO);
                break;
            default:
                mSystenControl.writeSysFs(SYSF_SDR_MODE, Integer.toString(MODE_OFF));
                Settings.System.putInt(mContext.getContentResolver(), KEY_SDR_MODE, MODE_OFF);
                break;
        }
    }
}
