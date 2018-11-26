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
 *  @author   tong.li
 *  @version  1.0
 *  @date     2018/03/16
 *  @par function description:
 *  - This is Audio Setting Manager, control surround sound state.
 */
package com.droidlogic.app;

import android.content.Context;
import android.content.ContentResolver;
import android.database.ContentObserver;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.net.Uri;
import android.os.Handler;
import android.provider.Settings;
import android.util.Log;

public class AudioSettingManager {
    private static final String TAG                 = "AudioSettingManager";

    private ContentResolver mResolver;
    private OutputModeManager mOutputModeManager;
    private SettingsObserver mSettingsObserver;

    public AudioSettingManager(Context context){
        mResolver = context.getContentResolver();
        mSettingsObserver = new SettingsObserver(new Handler());
        mOutputModeManager = new OutputModeManager(context);
        registerContentObserver();
    }

    public boolean needSyncDroidSetting(int surround) {
        int format = Settings.Global.getInt(mResolver,
                OutputModeManager.DIGITAL_AUDIO_FORMAT, -1);
        switch (surround) {
            case OutputModeManager.ENCODED_SURROUND_OUTPUT_AUTO:
                if (format == OutputModeManager.DIGITAL_AUTO)
					return false;
                break;
            case OutputModeManager.ENCODED_SURROUND_OUTPUT_NEVER:
                if (format == OutputModeManager.DIGITAL_PCM)
                    return false;
                break;
            case OutputModeManager.ENCODED_SURROUND_OUTPUT_ALWAYS:
            case OutputModeManager.ENCODED_SURROUND_OUTPUT_MANUAL:
                String subformat = Settings.Global.getString(mResolver,
                        OutputModeManager.DIGITAL_AUDIO_SUBFORMAT);
                String subsurround = getSurroundManualFormats();
                if (subsurround == null)
                    subsurround = "";
                if ((format == OutputModeManager.DIGITAL_MANUAL)
                        && subsurround.equals(subformat))
                    return false;
                else if ((format == OutputModeManager.DIGITAL_SPDIF)
                        && subsurround.equals(OutputModeManager.DIGITAL_AUDIO_SUBFORMAT_SPDIF))
                    return false;
                break;
            default:
                Log.d(TAG, "error surround format");
                break;
        }
        return true;
    }

    public String getSurroundManualFormats() {
        return Settings.Global.getString(mResolver,
                OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS);
    }

    private void registerContentObserver() {
        String[] settings = new String[] {
                OutputModeManager.ENCODED_SURROUND_OUTPUT,
                OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS,
        };
        for (String s : settings) {
            mResolver.registerContentObserver(Settings.Global.getUriFor(s),
                    false, mSettingsObserver);
        }
    }

    private class SettingsObserver extends ContentObserver {
        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            String option = uri.getLastPathSegment();
            final int esoValue = Settings.Global.getInt(mResolver,
                    OutputModeManager.ENCODED_SURROUND_OUTPUT,
                    OutputModeManager.ENCODED_SURROUND_OUTPUT_AUTO);

            if (!needSyncDroidSetting(esoValue))
                return;
            if (OutputModeManager.ENCODED_SURROUND_OUTPUT.equals(option)) {
                switch (esoValue) {
                    case OutputModeManager.ENCODED_SURROUND_OUTPUT_AUTO:
                        mOutputModeManager.setDigitalAudioFormatOut(
                                OutputModeManager.DIGITAL_AUTO);
                        break;
                    case OutputModeManager.ENCODED_SURROUND_OUTPUT_NEVER:
                        mOutputModeManager.setDigitalAudioFormatOut(
                                OutputModeManager.DIGITAL_PCM);
                        break;
                    case OutputModeManager.ENCODED_SURROUND_OUTPUT_ALWAYS:
                    case OutputModeManager.ENCODED_SURROUND_OUTPUT_MANUAL:
                        mOutputModeManager.setDigitalAudioFormatOut(
                                OutputModeManager.DIGITAL_MANUAL, getSurroundManualFormats());
                        break;
                    default:
                        break;
                }
            } else if (OutputModeManager.ENCODED_SURROUND_OUTPUT_ENABLED_FORMATS.equals(option)) {
                mOutputModeManager.setDigitalAudioFormatOut(
                        OutputModeManager.DIGITAL_MANUAL, getSurroundManualFormats());

            }
        }
    }
}
