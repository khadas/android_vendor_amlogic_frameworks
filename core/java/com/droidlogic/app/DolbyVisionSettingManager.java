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
 *  - This is Dolby Vision Manager, control DV feature, sysFs, proc env.
 */
package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class DolbyVisionSettingManager {
    private static final String TAG                 = "DolbyVisionSettingManager";

    private static final String KEY_DV_ENABLE        = "key_dv_enable";
    private static final String KEY_DV_POLICY        = "key_dv_policy";
    private static final String KEY_DV_MODE          = "key_dv_mode";

    private static final String PROP_DOLBY_VISION_ENABLE  = "persist.sys.dolbyvision.enable";

    private static final String SYSF_DV_ENABLE       = "/sys/module/am_vecm/parameters/dolby_vision_enable";
    private static final String SYSF_DV_POLICY       = "/sys/module/am_vecm/parameters/dolby_vision_policy";
    private static final String SYSF_DV_MODE         = "/sys/class/amvecm/dv_mode";

    private static final String DV_ENABLE            = "Y";
    private static final String DV_DISABLE           = "N";

    public static final int DOLBY_VISION_FOLLOW_SINK       = 0;
    public static final int DOLBY_VISION_FOLLOW_SOURCE     = 1;
    public static final int DOLBY_VISION_FORCE_OUTPUT_MODE = 2;

    public static final int DOLBY_VISION_OUTPUT_MODE_BYPASS      = 0;
  //public static final int DOLBY_VISION_OUTPUT_MODE_IPT         = 1;
    public static final int DOLBY_VISION_OUTPUT_MODE_IPT_TUNNEL  = 2;
    public static final int DOLBY_VISION_OUTPUT_MODE_HDR10       = 3;
  //public static final int DOLBY_VISION_OUTPUT_MODE_SDR10       = 4;
    public static final int DOLBY_VISION_OUTPUT_MODE_SDR8        = 5;

    private Context mContext;
    private SystemControlManager mSystenControl;

    public DolbyVisionSettingManager(Context context){
        mContext = context;
        mSystenControl = new SystemControlManager(context);
    }

    /**
     * if DV ENABLE: set policy value of database to sysFs. and need to init mode value.
     * if DV DISABLE: set DOLBY_VISION_FORCE_OUTPUT_MODE to policy sysFs, DOLBY_VISION_OUTPUT_MODE_BYPASS to mode sysFs.
     */
    public void initDolbyVision() {
        String flag = (getDolbyVisionEnable() == true) ? DV_ENABLE:DV_DISABLE;
        if (getDolbyVisionEnable() == true) {
            initDolbyVisionPolicy(getDolbyVisionPolicy());
            setDolbyVisionEnable(true);
            initDolbyVisionMode(getDolbyVisionMode());
        } else {
            initDolbyVisionPolicy(DOLBY_VISION_FORCE_OUTPUT_MODE);
            initDolbyVisionMode(DOLBY_VISION_OUTPUT_MODE_BYPASS);
            setDolbyVisionEnable(false);
        }
    }

    private void initDolbyVisionPolicy(int policy) {
        mSystenControl.writeSysFs(SYSF_DV_POLICY, Integer.toString(policy));
        Settings.System.putInt(mContext.getContentResolver(), KEY_DV_POLICY, policy);
    }

    private void initDolbyVisionMode(int mode) {
        mSystenControl.writeSysFs(SYSF_DV_MODE, "0x" + Integer.toString(mode));
        Settings.System.putInt(mContext.getContentResolver(), KEY_DV_MODE, mode);
    }

    public void setDolbyVisionEnable(boolean enable) {
        String flag = (enable == true) ? DV_ENABLE:DV_DISABLE;
        Settings.System.putString(mContext.getContentResolver(), KEY_DV_ENABLE, flag);
        try {
                Thread.sleep(100);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        mSystenControl.writeSysFs(SYSF_DV_ENABLE, flag);
    }

    public void setDolbyVisionPolicy(int policy) {
        if ((policy >= DOLBY_VISION_FOLLOW_SINK) && (policy <=DOLBY_VISION_FORCE_OUTPUT_MODE)) {
            if (policy != getDolbyVisionPolicy()) {
                mSystenControl.writeSysFs(SYSF_DV_POLICY, Integer.toString(policy));
                Settings.System.putInt(mContext.getContentResolver(), KEY_DV_POLICY, policy);
            }
        }
    }

    public void setDolbyVisionMode(int mode) {
        if ((mode >= DOLBY_VISION_OUTPUT_MODE_BYPASS) && (mode <= DOLBY_VISION_OUTPUT_MODE_SDR8)) {
            if (mode != getDolbyVisionMode()) {
                mSystenControl.writeSysFs(SYSF_DV_MODE, "0x" + Integer.toString(mode));
                Settings.System.putInt(mContext.getContentResolver(), KEY_DV_MODE, mode);
            }
        }
    }

    public boolean getDolbyVisionEnable() {
        String flag = Settings.System.getString(mContext.getContentResolver(), KEY_DV_ENABLE);
        if ("Y".equals(flag)) {
            return true;
        }
        return false;
    }

    /* *
     * @Description: Enable/Disable Dolby Vision
     * @params: state: 1:Enable  DV
     *                 0:Disable DV
     */
    public void setDolbyVisionEnable(int state) {
        mSystenControl.setDolbyVisionEnable(state);
    }

    /* *
     * @Description: Determine Whether TV support Dolby Vision
     * @return: if TV support Dolby Vision
     *              return the Highest resolution Tv supported.
     *          else
     *              return ""
     */
    public String isTvSupportDolbyVision() {
        return mSystenControl.isTvSupportDolbyVision();
    }

    /* *
     * @Description: get current state of Dolby Vision
     * @return: if DV is Enable  return true
     *                   Disable return false
     */
    public boolean isDolbyVisionEnable() {
        return mSystenControl.getPropertyBoolean(PROP_DOLBY_VISION_ENABLE, false);
    }

    public long resolveResolutionValue(String mode) {
        return mSystenControl.resolveResolutionValue(mode);
    }

    public int getDolbyVisionPolicy() {
        int policy = Settings.System.getInt(mContext.getContentResolver(), KEY_DV_POLICY, DOLBY_VISION_FORCE_OUTPUT_MODE);
        return policy;
    }

    public int getDolbyVisionMode() {
        int mode = Settings.System.getInt(mContext.getContentResolver(), KEY_DV_MODE, DOLBY_VISION_OUTPUT_MODE_BYPASS);
        return mode;
    }
}
