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

import android.content.Context;
import android.content.Intent;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;
import android.media.AudioManager;
//import android.media.AudioSystem;
import android.content.ContentResolver;
import com.droidlogic.app.SystemControlManager;

public class AudioOutputManager {
    private static final String TAG                                 = "AudioOutputManager";
    private static final boolean DEBUG                              = false;

    public static final String DIGITAL_AUDIO_FORMAT                 = "digital_audio_format";
    public static final String PARA_PCM                             = "hdmi_format=0";
    public static final String PARA_AUTO                            = "hdmi_format=5";
    public static final int DIGITAL_PCM                             = 0;
    public static final int DIGITAL_AUTO                            = 1;

    public static final String BOX_LINE_OUT                         = "box_line_out";
    public static final String PARA_BOX_LINE_OUT_OFF                = "enable_line_out=false";
    public static final String PARA_BOX_LINE_OUT_ON                 = "enable_line_out=true";
    public static final int BOX_LINE_OUT_OFF                        = 0;
    public static final int BOX_LINE_OUT_ON                         = 1;

    public static final String BOX_HDMI                             = "box_hdmi";
    public static final String PARA_BOX_HDMI_OFF                    = "Audio hdmi-out mute=1";
    public static final String PARA_BOX_HDMI_ON                     = "Audio hdmi-out mute=0";
    public static final int BOX_HDMI_OFF                            = 0;
    public static final int BOX_HDMI_ON                             = 1;

    public static final String TV_SPEAKER                           = "tv_speaker";
    public static final String PARA_TV_SPEAKER_OFF                  = "speaker_mute=1";
    public static final String PARA_TV_SPEAKER_ON                   = "speaker_mute=0";
    public static final int TV_SPEAKER_OFF                          = 0;
    public static final int TV_SPEAKER_ON                           = 1;

    public static final String TV_ARC                               = "tv_arc";
    public static final String PARA_TV_ARC_OFF                      = "HDMI ARC Switch=0";
    public static final String PARA_TV_ARC_ON                       = "HDMI ARC Switch=1";
    public static final int TV_ARC_OFF                              = 0;
    public static final int TV_ARC_ON                               = 1;

    public static final String VIRTUAL_SURROUND                     = "virtual_surround";
    public static final String PARA_VIRTUAL_SURROUND_OFF            = "enable_virtual_surround=false";
    public static final String PARA_VIRTUAL_SURROUND_ON             = "enable_virtual_surround=true";
    public static final int VIRTUAL_SURROUND_OFF                    = 0;
    public static final int VIRTUAL_SURROUND_ON                     = 1;

    public static final String SOUND_OUTPUT_DEVICE                  = "sound_output_device";
    public static final String PARA_SOUND_OUTPUT_DEVICE_SPEAKER     = "sound_output_device=speak";
    public static final String PARA_SOUND_OUTPUT_DEVICE_ARC         = "sound_output_device=arc";
    public static final int SOUND_OUTPUT_DEVICE_SPEAKER             = 0;
    public static final int SOUND_OUTPUT_DEVICE_ARC                 = 1;

    //sound effects save key in global settings
    public static final String SOUND_EFFECT_BASS                    = "sound_effect_bass";
    public static final String SOUND_EFFECT_TREBLE                  = "sound_effect_treble";
    public static final String SOUND_EFFECT_BALANCE                 = "sound_effect_balance";
    public static final String SOUND_EFFECT_DIALOG_CLARITY          = "sound_effect_dialog_clarity";
    public static final String SOUND_EFFECT_SURROUND                = "sound_effect_surround";
    public static final String SOUND_EFFECT_BASS_BOOST              = "sound_effect_bass_boost";
    public static final String SOUND_EFFECT_SOUND_MODE              = "sound_effect_sound_mode";
    public static final String SOUND_EFFECT_SOUND_MODE_TYPE         = "sound_effect_sound_mode_type";
    public static final String SOUND_EFFECT_SOUND_MODE_TYPE_DAP     = "type_dap";
    public static final String SOUND_EFFECT_SOUND_MODE_TYPE_EQ      = "type_eq";
    public static final String SOUND_EFFECT_SOUND_MODE_DAP_VALUE    = "sound_effect_sound_mode_dap";
    public static final String SOUND_EFFECT_SOUND_MODE_EQ_VALUE     = "sound_effect_sound_mode_eq";
    public static final String SOUND_EFFECT_BAND1                   = "sound_effect_band1";
    public static final String SOUND_EFFECT_BAND2                   = "sound_effect_band2";
    public static final String SOUND_EFFECT_BAND3                   = "sound_effect_band3";
    public static final String SOUND_EFFECT_BAND4                   = "sound_effect_band4";
    public static final String SOUND_EFFECT_BAND5                   = "sound_effect_band5";
    public static final String SOUND_EFFECT_AGC_ENABLE              = "sound_effect_agc_on";
    public static final String SOUND_EFFECT_AGC_MAX_LEVEL           = "sound_effect_agc_level";
    public static final String SOUND_EFFECT_AGC_ATTRACK_TIME        = "sound_effect_agc_attrack";
    public static final String SOUND_EFFECT_AGC_RELEASE_TIME        = "sound_effect_agc_release";
    public static final String SOUND_EFFECT_AGC_SOURCE_ID           = "sound_avl_source_id";

    public static final String DIGITAL_SOUND                        = "digital_sound";
    public static final String PCM                                  = "PCM";
    public static final String RAW                                  = "RAW";
    public static final String HDMI                                 = "HDMI";
    public static final String SPDIF                                = "SPDIF";
    public static final String HDMI_RAW                             = "HDMI passthrough";
    public static final String SPDIF_RAW                            = "SPDIF passthrough";
    public static final int IS_PCM                                  = 0;
    public static final int IS_SPDIF_RAW                            = 1;
    public static final int IS_HDMI_RAW                             = 2;

    public static final String DRC_MODE                             = "drc_mode";
    public static final String DTSDRC_MODE                          = "dtsdrc_mode";
    public static final String CUSTOM_0_DRCMODE                     = "0";
    public static final String CUSTOM_1_DRCMODE                     = "1";
    public static final String LINE_DRCMODE                         = "2";
    public static final String RF_DRCMODE                           = "3";
    public static final String DEFAULT_DRCMODE                      = LINE_DRCMODE;
    public static final String MIN_DRC_SCALE                        = "0";
    public static final String MAX_DRC_SCALE                        = "100";
    public static final String DEFAULT_DRC_SCALE                    = MIN_DRC_SCALE;
    public static final int IS_DRC_OFF                              = 0;
    public static final int IS_DRC_LINE                             = 1;
    public static final int IS_DRC_RF                               = 2;

    private final Context mContext;
    private final ContentResolver mResolver;
    final Object mLock = new Object[0];

    private AudioManager mAudioManager;
    private SystemControlManager mSystenControl;
    /*only system/priv process can binder HDMI_CONTROL_SERVICE*/
   // private HdmiControlManager mHdmiControlManager;

    public AudioOutputManager(Context context) {
        mContext = context;
        mResolver = mContext.getContentResolver();
        mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
    }

    public void setDigitalAudioFormatOut(int mode) {
        switch (mode) {
            case DIGITAL_PCM:
                mAudioManager.setParameters(PARA_PCM);
                Settings.Global.putInt(mResolver,
                        "encoded_surround_output"/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                        1/*Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER*/);
                break;
            case DIGITAL_AUTO:
                mAudioManager.setParameters(PARA_AUTO);
                Settings.Global.putInt(mResolver,
                        "encoded_surround_output"/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                        0/*Settings.Global.ENCODED_SURROUND_OUTPUT_AUTO*/);
                break;
            default:
                mAudioManager.setParameters(PARA_PCM);
                Settings.Global.putInt(mResolver,
                        "encoded_surround_output"/*Settings.Global.ENCODED_SURROUND_OUTPUT*/,
                        1/*Settings.Global.ENCODED_SURROUND_OUTPUT_NEVER*/);
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
        mSystenControl = SystemControlManager.getInstance();
        final boolean istv = mSystenControl.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false);
        Log.i(TAG, "init sound parameter after boot, current is:" + (istv? "TV" : "STB"));
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

    public void resetSoundParameters(boolean istv) {
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

