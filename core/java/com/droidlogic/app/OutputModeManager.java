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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Method;

import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiHotplugEvent;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.media.AudioManager;
import android.media.AudioFormat;
import android.content.ContentResolver;

public class OutputModeManager {
    private static final String TAG                         = "OutputModeManager";
    private static final boolean DEBUG                      = false;

    /**
     * The saved value for Outputmode auto-detection.
     * One integer
     * @hide
     */
    public static final String DISPLAY_OUTPUTMODE_AUTO      = "display_outputmode_auto";

    /**
     *  broadcast of the current HDMI output mode changed.
     */
    public static final String ACTION_HDMI_MODE_CHANGED     = "droidlogic.intent.action.HDMI_MODE_CHANGED";

    /**
     * Extra in {@link #ACTION_HDMI_MODE_CHANGED} indicating the mode:
     */
    public static final String EXTRA_HDMI_MODE              = "mode";

    public static final String SYS_DIGITAL_RAW              = "/sys/class/audiodsp/digital_raw";
    public static final String SYS_AUDIO_CAP                = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    public static final String SYS_AUIDO_HDMI               = "/sys/class/amhdmitx/amhdmitx0/config";
    public static final String SYS_AUIDO_SPDIF              = "/sys/devices/platform/spdif-dit.0/spdif_mute";

    public static final String AUIDO_DSP_AC3_DRC            = "/sys/class/audiodsp/ac3_drc_control";
    public static final String AUIDO_DSP_DTS_DEC            = "/sys/class/audiodsp/dts_dec_control";

    public static final String HDMI_STATE                   = "/sys/class/amhdmitx/amhdmitx0/hpd_state";
    public static final String HDMI_SUPPORT_LIST            = "/sys/class/amhdmitx/amhdmitx0/disp_cap";
    public static final String HDMI_COLOR_SUPPORT_LIST      = "/sys/class/amhdmitx/amhdmitx0/dc_cap";

    public static final String COLOR_ATTRIBUTE              = "/sys/class/amhdmitx/amhdmitx0/attr";
    public static final String DISPLAY_HDMI_VALID_MODE      = "/sys/class/amhdmitx/amhdmitx0/valid_mode";//test if tv support this mode

    public static final String DISPLAY_MODE                 = "/sys/class/display/mode";
    public static final String DISPLAY_AXIS                 = "/sys/class/display/axis";

    public static final String VIDEO_AXIS                   = "/sys/class/video/axis";

    public static final String FB0_FREE_SCALE_AXIS          = "/sys/class/graphics/fb0/free_scale_axis";
    public static final String FB0_FREE_SCALE_MODE          = "/sys/class/graphics/fb0/freescale_mode";
    public static final String FB0_FREE_SCALE               = "/sys/class/graphics/fb0/free_scale";
    public static final String FB1_FREE_SCALE               = "/sys/class/graphics/fb1/free_scale";

    public static final String FB0_WINDOW_AXIS              = "/sys/class/graphics/fb0/window_axis";
    public static final String FB0_BLANK                    = "/sys/class/graphics/fb0/blank";

    public static final String ENV_CVBS_MODE                = "ubootenv.var.cvbsmode";
    public static final String ENV_HDMI_MODE                = "ubootenv.var.hdmimode";
    public static final String ENV_OUTPUT_MODE              = "ubootenv.var.outputmode";
    public static final String ENV_DIGIT_AUDIO              = "ubootenv.var.digitaudiooutput";
    public static final String ENV_IS_BEST_MODE             = "ubootenv.var.is.bestmode";
    public static final String ENV_COLORATTRIBUTE           = "ubootenv.var.colorattribute";

    public static final String PROP_BEST_OUTPUT_MODE        = "ro.vendor.platform.best_outputmode";
    public static final String PROP_HDMI_ONLY               = "ro.vendor.platform.hdmionly";
    public static final String PROP_SUPPORT_4K              = "ro.vendor.platform.support.4k";
    public static final String PROP_DEEPCOLOR               = "vendor.sys.open.deepcolor";
    public static final String PROP_DTSDRCSCALE             = "persist.vendor.sys.dtsdrcscale";
    public static final String PROP_DTSEDID                 = "persist.vendor.sys.dts.edid";

    public static final String FULL_WIDTH_480               = "720";
    public static final String FULL_HEIGHT_480              = "480";
    public static final String FULL_WIDTH_576               = "720";
    public static final String FULL_HEIGHT_576              = "576";
    public static final String FULL_WIDTH_720               = "1280";
    public static final String FULL_HEIGHT_720              = "720";
    public static final String FULL_WIDTH_1080              = "1920";
    public static final String FULL_HEIGHT_1080             = "1080";
    public static final String FULL_WIDTH_4K2K              = "3840";
    public static final String FULL_HEIGHT_4K2K             = "2160";
    public static final String FULL_WIDTH_4K2KSMPTE         = "4096";
    public static final String FULL_HEIGHT_4K2KSMPTE        = "2160";

    public static final String DIGITAL_AUDIO_FORMAT          = "digital_audio_format";
    public static final String DIGITAL_AUDIO_SUBFORMAT       = "digital_audio_subformat";
    public static final String PARA_PCM                      = "hdmi_format=0";
    public static final String PARA_SPDIF                    = "hdmi_format=4";
    public static final String PARA_AUTO                     = "hdmi_format=5";
    public static final int DIGITAL_PCM                      = 0;
    public static final int DIGITAL_SPDIF                    = 1;
    public static final int DIGITAL_AUTO                     = 2;
    public static final int DIGITAL_MANUAL                   = 3;
    // DD/DD+/DTS
    public static final String DIGITAL_AUDIO_SUBFORMAT_SPDIF  = "5,6,7";

    private static final String NRDP_EXTERNAL_SURROUND =
                                       "nrdp_external_surround_sound_enabled";
    private static final int NRDP_ENABLE            = 1;
    private static final int NRDP_DISABLE           = 0;

    public static final String BOX_LINE_OUT                          = "box_line_out";
    public static final String PARA_BOX_LINE_OUT_OFF         = "enable_line_out=false";
    public static final String PARA_BOX_LINE_OUT_ON           = "enable_line_out=true";
    public static final int BOX_LINE_OUT_OFF                        = 0;
    public static final int BOX_LINE_OUT_ON                         = 1;

    public static final String BOX_HDMI                          = "box_hdmi";
    public static final String PARA_BOX_HDMI_OFF         = "Audio hdmi-out mute=1";
    public static final String PARA_BOX_HDMI_ON           = "Audio hdmi-out mute=0";
    public static final int BOX_HDMI_OFF                               = 0;
    public static final int BOX_HDMI_ON                                 = 1;

    public static final String TV_SPEAKER                       = "tv_speaker";
    public static final String PARA_TV_SPEAKER_OFF       = "speaker_mute=1";
    public static final String PARA_TV_SPEAKER_ON         = "speaker_mute=0";
    public static final int TV_SPEAKER_OFF                     = 0;
    public static final int TV_SPEAKER_ON                       = 1;

    public static final String TV_ARC                      = "tv_arc";
    public static final String PARA_TV_ARC_OFF       = "HDMI ARC Switch=0";
    public static final String PARA_TV_ARC_ON         = "HDMI ARC Switch=1";
    public static final int TV_ARC_OFF                     = 0;
    public static final int TV_ARC_ON                       = 1;

    public static final String VIRTUAL_SURROUND                      = "virtual_surround";
    public static final String PARA_VIRTUAL_SURROUND_OFF       = "enable_virtual_surround=false";
    public static final String PARA_VIRTUAL_SURROUND_ON       = "enable_virtual_surround=true";
    public static final int VIRTUAL_SURROUND_OFF                     = 0;
    public static final int VIRTUAL_SURROUND_ON                       = 1;

    //surround sound formats, must sync with Settings.Global
    public static final String ENCODED_SURROUND_OUTPUT = "encoded_surround_output";
    public static final String ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS =
                "encoded_surround_output_enabled_formats";
    public static final int ENCODED_SURROUND_OUTPUT_AUTO = 0;
    public static final int ENCODED_SURROUND_OUTPUT_NEVER = 1;
    public static final int ENCODED_SURROUND_OUTPUT_ALWAYS = 2;
    public static final int ENCODED_SURROUND_OUTPUT_MANUAL = 3;

    public static final String SOUND_OUTPUT_DEVICE                         = "sound_output_device";
    public static final String PARA_SOUND_OUTPUT_DEVICE_SPEAKER  = "sound_output_device=speak";
    public static final String PARA_SOUND_OUTPUT_DEVICE_ARC       = "sound_output_device=arc";
    public static final int SOUND_OUTPUT_DEVICE_SPEAKER                 = 0;
    public static final int SOUND_OUTPUT_DEVICE_ARC                        = 1;

    //sound effects save key in global settings
    public static final String SOUND_EFFECT_BASS                         = "sound_effect_bass";
    public static final String SOUND_EFFECT_TREBLE                      = "sound_effect_treble";
    public static final String SOUND_EFFECT_BALANCE                   = "sound_effect_balance";
    public static final String SOUND_EFFECT_DIALOG_CLARITY       = "sound_effect_dialog_clarity";
    public static final String SOUND_EFFECT_SURROUND                 = "sound_effect_surround";
    public static final String SOUND_EFFECT_BASS_BOOST             = "sound_effect_bass_boost";
    public static final String SOUND_EFFECT_SOUND_MODE            = "sound_effect_sound_mode";
    public static final String SOUND_EFFECT_BAND1       = "sound_effect_band1";
    public static final String SOUND_EFFECT_BAND2       = "sound_effect_band2";
    public static final String SOUND_EFFECT_BAND3       = "sound_effect_band3";
    public static final String SOUND_EFFECT_BAND4       = "sound_effect_band4";
    public static final String SOUND_EFFECT_BAND5       = "sound_effect_band5";
    public static final String SOUND_EFFECT_AGC_ENABLE       = "sound_effect_agc_on";
    public static final String SOUND_EFFECT_AGC_MAX_LEVEL       = "sound_effect_agc_level";
    public static final String SOUND_EFFECT_AGC_ATTRACK_TIME       = "sound_effect_agc_attrack";
    public static final String SOUND_EFFECT_AGC_RELEASE_TIME       = "sound_effect_agc_release";

    public static final String DIGITAL_SOUND                = "digital_sound";
    public static final String PCM                          = "PCM";
    public static final String RAW                          = "RAW";
    public static final String HDMI                         = "HDMI";
    public static final String SPDIF                        = "SPDIF";
    public static final String HDMI_RAW                     = "HDMI passthrough";
    public static final String SPDIF_RAW                    = "SPDIF passthrough";
    public static final int IS_PCM                          = 0;
    public static final int IS_SPDIF_RAW                    = 1;
    public static final int IS_HDMI_RAW                     = 2;

    public static final String DRC_MODE                     = "drc_mode";
    public static final String DTSDRC_MODE                  = "dtsdrc_mode";
    public static final String CUSTOM_0_DRCMODE             = "0";
    public static final String CUSTOM_1_DRCMODE             = "1";
    public static final String LINE_DRCMODE                 = "2";
    public static final String RF_DRCMODE                   = "3";
    public static final String DEFAULT_DRCMODE              = LINE_DRCMODE;
    public static final String MIN_DRC_SCALE                = "0";
    public static final String MAX_DRC_SCALE                = "100";
    public static final String DEFAULT_DRC_SCALE            = MIN_DRC_SCALE;
    public static final int IS_DRC_OFF                      = 0;
    public static final int IS_DRC_LINE                     = 1;
    public static final int IS_DRC_RF                       = 2;

    public static final String REAL_OUTPUT_SOC              = "meson8,meson8b,meson8m2,meson9b";
    public static final String UI_720P                      = "720p";
    public static final String UI_1080P                     = "1080p";
    public static final String UI_2160P                     = "2160p";
    public static final String HDMI_480                     = "480";
    public static final String HDMI_576                     = "576";
    public static final String HDMI_720                     = "720p";
    public static final String HDMI_1080                    = "1080";
    public static final String HDMI_4K2K                    = "2160p";
    public static final String HDMI_SMPTE                   = "smpte";

    private String DEFAULT_OUTPUT_MODE                      = "720p60hz";
    private String DEFAULT_COLOR_ATTRIBUTE                  = "444,8bit";

    private static String currentColorAttribute = null;
    private static String currentOutputmode = null;
    private boolean ifModeSetting = false;
    private final Context mContext;
    private final ContentResolver mResolver;
    final Object mLock = new Object[0];

    private SystemControlManager mSystenControl;
    private AudioManager mAudioManager;
    /*only system/priv process can binder HDMI_CONTROL_SERVICE*/
   // private HdmiControlManager mHdmiControlManager;

    public OutputModeManager(Context context) {
        mContext = context;

        mSystenControl = SystemControlManager.getInstance();
        mResolver = mContext.getContentResolver();
        currentOutputmode = readSysfs(DISPLAY_MODE);
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);

       /* mHdmiControlManager = (HdmiControlManager) context.getSystemService(Context.HDMI_CONTROL_SERVICE);
        if (mHdmiControlManager != null) {
            mHdmiControlManager.addHotplugEventListener(new HdmiControlManager.HotplugEventListener() {

                @Override
                public void onReceived(HdmiHotplugEvent event) {
                    if (Settings.Global.getInt(context.getContentResolver(), DIGITAL_SOUND, IS_PCM) == IS_HDMI_RAW) {
                        autoSwitchHdmiPassthough();
                    }
                }
            });
        }*/
    }

    private void setOutputMode(final String mode) {
        setOutputModeNowLocked(mode);
    }

    public void setBestMode(String mode) {
        if (mode == null) {
            if (!isBestOutputmode()) {
                mSystenControl.setBootenv(ENV_IS_BEST_MODE, "true");
                if (SystemControlManager.USE_BEST_MODE) {
                    setOutputMode(getBestMatchResolution());
                } else {
                    setOutputMode(getHighestMatchResolution());
                }
            } else {
                mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
            }
        } else {
            mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
            setOutputModeNowLocked(mode);
        }
    }

    public void setDeepColorMode() {
        if (isDeepColor()) {
            mSystenControl.setProperty(PROP_DEEPCOLOR, "false");
        } else {
            mSystenControl.setProperty(PROP_DEEPCOLOR, "true");
        }
        setOutputModeNowLocked(getCurrentOutputMode());
    }

    public void setDeepColorAttribute(final String colorValue) {
        mSystenControl.setBootenv(ENV_IS_BEST_MODE, "false");
        mSystenControl.setBootenv(ENV_COLORATTRIBUTE, colorValue);
        setOutputModeNowLocked(getCurrentOutputMode());
    }

    public String getCurrentColorAttribute(){
       String colorValue = getBootenv(ENV_COLORATTRIBUTE, DEFAULT_COLOR_ATTRIBUTE);
       return colorValue;
    }

    public String getHdmiColorSupportList() {
        String list = readSupportList(HDMI_COLOR_SUPPORT_LIST);

        if (DEBUG)
            Log.d(TAG, "getHdmiColorSupportList :" + list);
        return list;
    }

    public boolean isModeSupportColor(final String curMode, final String curValue){
         writeSysfs(DISPLAY_HDMI_VALID_MODE, curMode+curValue);
         String isSupport = readSysfs(DISPLAY_HDMI_VALID_MODE).trim();
         Log.d("SystemControl", "In OutputModeManager, " + curMode+curValue+" is "+ " supported or not:"+isSupport);
         return isSupport.equals("1") ? true : false;
    }

    private void setOutputModeNowLocked(final String newMode){
        synchronized (mLock) {
            String oldMode = currentOutputmode;
            currentOutputmode = newMode;

            if (oldMode == null || oldMode.length() < 4) {
                Log.e(TAG, "get display mode error, oldMode:" + oldMode + " set to default " + DEFAULT_OUTPUT_MODE);
                oldMode = DEFAULT_OUTPUT_MODE;
            }

            if (DEBUG)
                Log.d(TAG, "change mode from " + oldMode + " -> " + newMode);

            mSystenControl.setMboxOutputMode(newMode);

            Intent intent = new Intent(ACTION_HDMI_MODE_CHANGED);
            //intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(EXTRA_HDMI_MODE, newMode);
            mContext.sendStickyBroadcast(intent);
        }
    }

    public void setOsdMouse(String curMode) {
        if (DEBUG)
            Log.d(TAG, "set osd mouse curMode " + curMode);
        mSystenControl.setOsdMouseMode(curMode);
    }

    public void setOsdMouse(int x, int y, int w, int h) {
        mSystenControl.setOsdMousePara(x, y, w, h);
    }

    public String getCurrentOutputMode(){
        return readSysfs(DISPLAY_MODE);
    }

    public int[] getPosition(String mode) {
        return mSystenControl.getPosition(mode);
    }

    public void savePosition(int left, int top, int width, int height) {
        mSystenControl.setPosition(left, top, width, height);
    }

    public String getHdmiSupportList() {
        String list = readSupportList(HDMI_SUPPORT_LIST).replaceAll("[*]", "");

        if (DEBUG)
            Log.d(TAG, "getHdmiSupportList :" + list);
        return list;
    }

    public String getHighestMatchResolution() {
        final String KEY = "hz";
        final String FORMAT_P = "p";
        final String FORMAT_I = "i";

        String[] supportList = null;
        String value = readSupportList(HDMI_SUPPORT_LIST);
        if (value.indexOf(HDMI_480) >= 0 || value.indexOf(HDMI_576) >= 0
            || value.indexOf(HDMI_720) >= 0 || value.indexOf(HDMI_1080) >= 0
            || value.indexOf(HDMI_4K2K) >= 0 || value.indexOf(HDMI_SMPTE) >= 0) {
            supportList = (value.substring(0, value.length()-1)).split(",");
        }

        int type = -1;
        int intMode = -1, higMode = 0, lenMode = 0;
        String outputMode = null;
        if (supportList != null) {
            for (int i = 0; i < supportList.length; i++) {
                String[] pref = supportList[i].split(KEY);
                if (pref != null) {
                    if ((type = supportList[i].indexOf(FORMAT_P)) >= 3) {          //p
                        intMode = Integer.parseInt(pref[0].replace(FORMAT_P, "1"));
                    } else if ((type = supportList[i].indexOf(FORMAT_I)) > 0) {    //i
                        intMode = Integer.parseInt(pref[0].replace(FORMAT_I, "0"));
                    } else {                                                        //other
                        continue;
                    }
                    if (intMode >= higMode) {
                        int len = supportList[i].length();
                        if (intMode == higMode && lenMode >= len) continue;
                        lenMode = len;
                        higMode = intMode;
                        if (supportList[i].contains("*"))
                            outputMode = supportList[i].substring(0, supportList[i].length()-1);
                        else
                            outputMode = supportList[i];
                    }
                }
            }
        }
        if (outputMode != null) return outputMode;
        return getPropertyString(PROP_BEST_OUTPUT_MODE, DEFAULT_OUTPUT_MODE);
    }

    public String getBestMatchResolution() {
        if (DEBUG)
            Log.d(TAG, "get best mode, if support mode contains *, that is best mode, otherwise use:" + PROP_BEST_OUTPUT_MODE);

        String[] supportList = null;
        String value = readSupportList(HDMI_SUPPORT_LIST);
        if (value.indexOf(HDMI_480) >= 0 || value.indexOf(HDMI_576) >= 0
            || value.indexOf(HDMI_720) >= 0 || value.indexOf(HDMI_1080) >= 0
            || value.indexOf(HDMI_4K2K) >= 0 || value.indexOf(HDMI_SMPTE) >= 0) {
            supportList = (value.substring(0, value.length()-1)).split(",");
        }

        if (supportList != null) {
            for (int i = 0; i < supportList.length; i++) {
                if (supportList[i].contains("*")) {
                    return supportList[i].substring(0, supportList[i].length()-1);
                }
            }
        }

        return getPropertyString(PROP_BEST_OUTPUT_MODE, DEFAULT_OUTPUT_MODE);
    }

    public String getSupportedResolution() {
        String curMode = getBootenv(ENV_HDMI_MODE, DEFAULT_OUTPUT_MODE);

        if (DEBUG)
            Log.d(TAG, "get supported resolution curMode:" + curMode);

        String value = readSupportList(HDMI_SUPPORT_LIST);
        String[] supportList = null;

        if (value.indexOf(HDMI_480) >= 0 || value.indexOf(HDMI_576) >= 0
            || value.indexOf(HDMI_720) >= 0 || value.indexOf(HDMI_1080) >= 0
            || value.indexOf(HDMI_4K2K) >= 0 || value.indexOf(HDMI_SMPTE) >= 0) {
            supportList = (value.substring(0, value.length()-1)).split(",");
        }

        if (supportList == null) {
            return curMode;
        }

        for (int i = 0; i < supportList.length; i++) {
            if (supportList[i].equals(curMode)) {
                return curMode;
            }
        }

        if (SystemControlManager.USE_BEST_MODE) {
            return getBestMatchResolution();
        }
        return getHighestMatchResolution();
    }

    private String readSupportList(String path) {
        String str = null;
        String value = "";
        try {
            BufferedReader br = new BufferedReader(new FileReader(path));
            while ((str = br.readLine()) != null) {
                if (str != null) {
                    if (!getPropertyBoolean(PROP_SUPPORT_4K, true)
                        && (str.contains("2160") || str.contains("smpte"))) {
                        continue;
                    }
                    value += str + ",";
                }
            }
            br.close();

            Log.d(TAG, "TV support list is :" + value);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return value;
    }

    public void initOutputMode(){
        if (isHDMIPlugged()) {
            setHdmiPlugged();
        } else {
            if (!currentOutputmode.contains("cvbs"))
                setHdmiUnPlugged();
        }

        //there can not set osd mouse parameter, otherwise bootanimation logo will shake
        //because set osd1 scaler will shake
    }

    public void setHdmiUnPlugged(){
        Log.d(TAG, "setHdmiUnPlugged");

        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            String cvbsmode = getBootenv(ENV_CVBS_MODE, "576cvbs");
            setOutputMode(cvbsmode);
        }
    }

    public void setHdmiPlugged() {
        boolean isAutoMode = isBestOutputmode() || readSupportList(HDMI_SUPPORT_LIST).contains("null edid");

        Log.d(TAG, "setHdmiPlugged auto mode: " + isAutoMode);
        if (getPropertyBoolean(PROP_HDMI_ONLY, true)) {
            if (isAutoMode) {
                if (SystemControlManager.USE_BEST_MODE) {
                    setOutputMode(getBestMatchResolution());
                } else {
                    setOutputMode(getHighestMatchResolution());
                }
            } else {
                String mode = getSupportedResolution();
                setOutputMode(mode);
            }
        }
    }

    public boolean isBestOutputmode() {
        String isBestOutputmode = mSystenControl.getBootenv(ENV_IS_BEST_MODE, "true");
        return Boolean.parseBoolean(isBestOutputmode.equals("") ? "true" : isBestOutputmode);
    }

    public boolean isDeepColor() {
        return getPropertyBoolean(PROP_DEEPCOLOR, false);
    }

    public boolean isHDMIPlugged() {
        String status = readSysfs(HDMI_STATE);
        if ("1".equals(status))
            return true;
        else
            return false;
    }

    public boolean ifModeIsSetting() {
        return ifModeSetting;
    }

    private void shadowScreen() {
        writeSysfs(FB0_BLANK, "1");
        Thread task = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ifModeSetting = true;
                    Thread.sleep(1000);
                    writeSysfs(FB0_BLANK, "0");
                    ifModeSetting = false;
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });
        task.start();
    }

    public String getDigitalVoiceMode(){
        return getBootenv(ENV_DIGIT_AUDIO, PCM);
    }

    public int autoSwitchHdmiPassthough () {
        String mAudioCapInfo = readSysfsTotal(SYS_AUDIO_CAP);
        if (mAudioCapInfo.contains("Dobly_Digital+")) {
            setDigitalMode(HDMI_RAW);
            return IS_HDMI_RAW;
        } else if (mAudioCapInfo.contains("AC-3")
                || (getPropertyBoolean(PROP_DTSEDID, false) && mAudioCapInfo.contains("DTS"))) {
            setDigitalMode(SPDIF_RAW);
            return IS_SPDIF_RAW;
        } else {
            setDigitalMode(PCM);
            return IS_PCM;
        }
    }

    public void setDigitalMode(String mode) {
        // value : "PCM" ,"RAW","SPDIF passthrough","HDMI passthrough"
        setBootenv(ENV_DIGIT_AUDIO, mode);
        mSystenControl.setDigitalMode(mode);
    }

    public void enableDobly_DRC (boolean enable) {
        if (enable) {       //open DRC
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0x64");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0x64");
        } else {           //close DRC
            writeSysfs(AUIDO_DSP_AC3_DRC, "drchighcutscale 0");
            writeSysfs(AUIDO_DSP_AC3_DRC, "drclowboostscale 0");
        }
    }

    public void setDoblyMode (String mode) {
        //"CUSTOM_0","CUSTOM_1","LINE","RF"; default use "LINE"
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 3) {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_AC3_DRC, "drcmode" + " " + DEFAULT_DRCMODE);
        }
    }

    public void setDtsDrcScale (String drcscale) {
        //10 one step,100 highest; default use "0"
        int i = Integer.parseInt(drcscale);
        if (i >= 0 && i <= 100) {
            setProperty(PROP_DTSDRCSCALE, drcscale);
        } else {
            setProperty(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        }
        setDtsDrcScaleSysfs();
    }

    public void setDtsDrcScaleSysfs() {
        String prop = getPropertyString(PROP_DTSDRCSCALE, DEFAULT_DRC_SCALE);
        int val = Integer.parseInt(prop);
        writeSysfs(AUIDO_DSP_DTS_DEC, String.format("0x%02x", val));
    }
    /**
    * @Deprecated
    **/
    public void setDTS_DownmixMode(String mode) {
        // 0: Lo/Ro;   1: Lt/Rt;  default 0
        int i = Integer.parseInt(mode);
        if (i >= 0 && i <= 1) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + mode);
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdmxmode" + " " + "0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_DRC_scale_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0x64");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdrcscale 0");
        }
    }
    /**
    * @Deprecated
    **/
    public void enableDTS_Dial_Norm_control (boolean enable) {
        if (enable) {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 1");
        } else {
            writeSysfs(AUIDO_DSP_DTS_DEC, "dtsdialnorm 0");
        }
    }

    private String getProperty(String key) {
        if (DEBUG)
            Log.i(TAG, "getProperty key:" + key);
        return mSystenControl.getProperty(key);
    }

    private String getPropertyString(String key, String def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyString key:" + key + " def:" + def);
        return mSystenControl.getPropertyString(key, def);
    }

    private int getPropertyInt(String key,int def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyInt key:" + key + " def:" + def);
        return mSystenControl.getPropertyInt(key, def);
    }

    private long getPropertyLong(String key,long def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyLong key:" + key + " def:" + def);
        return mSystenControl.getPropertyLong(key, def);
    }

    private boolean getPropertyBoolean(String key,boolean def) {
        if (DEBUG)
            Log.i(TAG, "getPropertyBoolean key:" + key + " def:" + def);
        return mSystenControl.getPropertyBoolean(key, def);
    }

    private void setProperty(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "setProperty key:" + key + " value:" + value);
        mSystenControl.setProperty(key, value);
    }

    private String getBootenv(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenv key:" + key + " def value:" + value);
        return mSystenControl.getBootenv(key, value);
    }

    private int getBootenvInt(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "getBootenvInt key:" + key + " def value:" + value);
        return Integer.parseInt(mSystenControl.getBootenv(key, value));
    }

    private void setBootenv(String key, String value) {
        if (DEBUG)
            Log.i(TAG, "setBootenv key:" + key + " value:" + value);
        mSystenControl.setBootenv(key, value);
    }

    private String readSysfsTotal(String path) {
        return mSystenControl.readSysFs(path).replaceAll("\n", "");
    }

    private String readSysfs(String path) {

        return mSystenControl.readSysFs(path).replaceAll("\n", "");
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return null;
        }

        String str = null;
        StringBuilder value = new StringBuilder();

        if (DEBUG)
            Log.i(TAG, "readSysfs path:" + path);

        try {
            FileReader fr = new FileReader(path);
            BufferedReader br = new BufferedReader(fr);
            try {
                while ((str = br.readLine()) != null) {
                    if (str != null)
                        value.append(str);
                };
                fr.close();
                br.close();
                if (value != null)
                    return value.toString();
                else
                    return null;
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }
        */
    }

    private boolean writeSysfs(String path, String value) {
        if (DEBUG)
            Log.i(TAG, "writeSysfs path:" + path + " value:" + value);

        return mSystenControl.writeSysFs(path, value);
        /*
        if (!new File(path).exists()) {
            Log.e(TAG, "File not found: " + path);
            return false;
        }

        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(path), 64);
            try {
                writer.write(value);
            } finally {
                writer.close();
            }
            return true;

        } catch (IOException e) {
            Log.e(TAG, "IO Exception when write: " + path, e);
            return false;
        }
        */
    }

    public void saveDigitalAudioFormatMode(int mode, String submode) {
        String tmp;
        int surround = Settings.Global.getInt(mResolver,
                ENCODED_SURROUND_OUTPUT, -1);
        switch (mode) {
            case DIGITAL_SPDIF:
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_ENABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_SPDIF);
                Settings.Global.putString(mResolver,
                        DIGITAL_AUDIO_SUBFORMAT, DIGITAL_AUDIO_SUBFORMAT_SPDIF);
                if (surround != ENCODED_SURROUND_OUTPUT_MANUAL)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_MANUAL/*Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL*/);
                tmp = Settings.Global.getString(mResolver,
                        OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
                if (!DIGITAL_AUDIO_SUBFORMAT_SPDIF.equals(tmp))
                    Settings.Global.putString(mResolver,
                            ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS,
                            DIGITAL_AUDIO_SUBFORMAT_SPDIF);
                break;
            case DIGITAL_MANUAL:
                if (submode == null)
                    submode = "";
                boolean isTv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                if (isTv) {
                    Settings.Global.putInt(mResolver,
                            DIGITAL_AUDIO_FORMAT, DIGITAL_AUTO);
                    break;
                } else
                    Settings.Global.putInt(mResolver,
                            DIGITAL_AUDIO_FORMAT, DIGITAL_MANUAL);
                Settings.Global.putString(mResolver,
                        DIGITAL_AUDIO_SUBFORMAT, submode);
                if (surround != ENCODED_SURROUND_OUTPUT_MANUAL)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_MANUAL/*Settings.Global.ENCODED_SURROUND_OUTPUT_MANUAL*/);
                tmp = Settings.Global.getString(mResolver,
                        OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
                if (!submode.equals(tmp))
                    Settings.Global.putString(mResolver,
                            ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS, submode);
                break;
            case DIGITAL_AUTO:
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_AUTO);
                if (surround != ENCODED_SURROUND_OUTPUT_AUTO)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_AUTO/*Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO*/);
                break;
            case DIGITAL_PCM:
            default:
                Settings.Global.putInt(mResolver,
                        NRDP_EXTERNAL_SURROUND, NRDP_DISABLE);
                Settings.Global.putInt(mResolver,
                        DIGITAL_AUDIO_FORMAT, DIGITAL_PCM);
                if (surround != ENCODED_SURROUND_OUTPUT_NEVER)
                    Settings.Global.putInt(mResolver,
                            ENCODED_SURROUND_OUTPUT/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                            ENCODED_SURROUND_OUTPUT_NEVER/*Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER*/);
                break;
        }
    }

    public void setDigitalAudioFormatOut(int mode) {
        setDigitalAudioFormatOut(mode, "");
    }

    public void setDigitalAudioFormatOut(int mode, String submode) {
        Log.d(TAG, "setDigitalAudioFormatOut: mode="+mode+", submode="+submode);
        saveDigitalAudioFormatMode(mode, submode);
        switch (mode) {
            case DIGITAL_SPDIF:
                mAudioManager.setParameters(PARA_SPDIF);
                break;
            case DIGITAL_AUTO:
                mAudioManager.setParameters(PARA_AUTO);
                break;
            case DIGITAL_MANUAL:
                mAudioManager.setParameters(PARA_AUTO);
                break;
            case DIGITAL_PCM:
            default:
                mAudioManager.setParameters(PARA_PCM);
                break;
        }
    }

    public void enableBoxLineOutAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_LINE_OUT_OFF);
        }
    }

    public void enableBoxHdmiAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_BOX_HDMI_ON);
        } else {
            mAudioManager.setParameters(PARA_BOX_HDMI_OFF);
        }
    }

    public void enableTvSpeakerAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_SPEAKER_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_SPEAKER_OFF);
        }
    }

    public void enableTvArcAudio(boolean value) {
        if (value) {
            mAudioManager.setParameters(PARA_TV_ARC_ON);
        } else {
            mAudioManager.setParameters(PARA_TV_ARC_OFF);
        }
    }

    public void setVirtualSurround (int value) {
        if (value == VIRTUAL_SURROUND_ON) {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_ON);
        } else {
            mAudioManager.setParameters(PARA_VIRTUAL_SURROUND_OFF);
        }
    }

    public void setSoundOutputStatus (int mode) {
        switch (mode) {
            case SOUND_OUTPUT_DEVICE_SPEAKER:
                enableTvSpeakerAudio(true);
                enableTvArcAudio(false);
                break;
            case SOUND_OUTPUT_DEVICE_ARC:
                enableTvSpeakerAudio(false);
                enableTvArcAudio(true);
                break;
        }
    }

    public void initSoundParametersAfterBoot() {
        final boolean istv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
        if (!istv) {
            final int boxlineout = Settings.Global.getInt(mResolver, BOX_LINE_OUT, BOX_LINE_OUT_OFF);
            enableBoxLineOutAudio(boxlineout == BOX_LINE_OUT_ON);
            final int boxhdmi = Settings.Global.getInt(mResolver, BOX_HDMI, BOX_HDMI_ON);
            enableBoxHdmiAudio(boxhdmi == BOX_HDMI_ON);
        } else {
            final int virtualsurround = Settings.Global.getInt(mResolver, VIRTUAL_SURROUND, VIRTUAL_SURROUND_OFF);
            setVirtualSurround(virtualsurround);
            /*int device = mAudioManager.getDevicesForStream(AudioManager.STREAM_MUSIC);
            if ((device & AudioSystem.DEVICE_OUT_SPEAKER) != 0) {
                final int soundoutput = Settings.Global.getInt(mResolver, SOUND_OUTPUT_DEVICE, SOUND_OUTPUT_DEVICE_SPEAKER);
                setSoundOutputStatus(soundoutput);
            }*/
        }
    }

    public void resetSoundParameters() {
        final boolean istv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
        if (!istv) {
            enableBoxLineOutAudio(false);
            enableBoxHdmiAudio(false);
        } else {
            enableTvSpeakerAudio(false);
            enableTvArcAudio(false);
            setVirtualSurround(VIRTUAL_SURROUND_OFF);
            setSoundOutputStatus(SOUND_OUTPUT_DEVICE_SPEAKER);
        }
    }
}

