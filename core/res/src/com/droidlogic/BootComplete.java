
package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.content.ContentResolver;
import android.util.Log;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.provider.Settings;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.PlayBackManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.UsbCameraManager;
import com.droidlogic.HdmiCecExtend;
import com.droidlogic.app.DolbyVisionSettingManager;

public class BootComplete extends BroadcastReceiver {
    private static final String TAG             = "BootComplete";
    private static final String FIRST_RUN       = "first_run";
    private static final int SPEAKER_DEFAULT_VOLUME = 11;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            final ContentResolver resolver = context.getContentResolver();
            final SystemControlManager sm = new SystemControlManager(context);
            //register system control callback
            sm.setListener(new SystemControlEvent(context));

            final AudioManager audioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
            final OutputModeManager outputModeManager = new OutputModeManager(context);

            if (SettingsPref.getFirstRun(context)) {
                Log.i(TAG, "first running: " + context.getPackageName());
                try {
                    Settings.Global.putInt(resolver,
                            OutputModeManager.DIGITAL_SOUND, OutputModeManager.IS_PCM);
                    Settings.Global.putInt(resolver,
                            OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_LINE);
                    Settings.Global.putInt(resolver, Settings.Global.CAPTIVE_PORTAL_DETECTION_ENABLED, 0);
                    //set default show_ime_with_hard_keyboard 1, then first boot can show the ime.
                    Settings.Secure.putInt(resolver, Settings.Secure.SHOW_IME_WITH_HARD_KEYBOARD, 1);
                    if (AudioSystem.PLATFORM_TELEVISION == AudioSystem.getPlatformType(context)) {
                        int maxVolume = SystemProperties.getInt("ro.config.media_vol_steps", 100);
                        int streamMaxVolume = audioManager.getStreamMaxVolume(AudioSystem.STREAM_MUSIC);
                        int defaultVolume = maxVolume == streamMaxVolume ? (maxVolume * 3) / 10 : (streamMaxVolume * 3) / 4;
                        audioManager.setStreamVolume(AudioSystem.STREAM_MUSIC, defaultVolume, 0);
                    }
                } catch (NumberFormatException e) {
                    Log.e(TAG, "could not find hard keyboard ", e);
                }

                SettingsPref.setFirstRun(context, false);
            }

            int speakervalue = Settings.System.getInt(resolver,"volume_music_speaker",-1);
            if (speakervalue == -1) {
                Settings.System.putInt(resolver,"volume_music_speaker", SPEAKER_DEFAULT_VOLUME);
                if (AudioSystem.PLATFORM_TELEVISION != AudioSystem.getPlatformType(context)) {
                    int current = audioManager.getStreamVolume( AudioManager.STREAM_MUSIC);
                    audioManager.setStreamVolume(AudioManager.STREAM_MUSIC,current,0);
                }
            }

            final int digitalSoundValue = Settings.Global.getInt(resolver,
                    OutputModeManager.DIGITAL_SOUND, OutputModeManager.IS_PCM);
            switch (digitalSoundValue) {
            case OutputModeManager.IS_PCM:
            case OutputModeManager.IS_HDMI_RAW:
            default:
                AudioSystem.setDeviceConnectionState(
                        AudioSystem.DEVICE_OUT_SPDIF,
                        AudioSystem.DEVICE_STATE_UNAVAILABLE,
                        "Amlogic", "Amlogic-S/PDIF");
                break;
            case OutputModeManager.IS_SPDIF_RAW:
                AudioSystem.setDeviceConnectionState(
                        AudioSystem.DEVICE_OUT_SPDIF,
                        AudioSystem.DEVICE_STATE_AVAILABLE,
                        "Amlogic", "Amlogic-S/PDIF");
                break;
            }

            final int drcModeValue = Settings.Global.getInt(resolver,
                    OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_LINE);
            switch (drcModeValue) {
            case OutputModeManager.IS_DRC_OFF:
                outputModeManager.enableDobly_DRC(false);
                outputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                break;
            case OutputModeManager.IS_DRC_LINE:
            default:
                outputModeManager.enableDobly_DRC(true);
                outputModeManager.setDoblyMode(OutputModeManager.LINE_DRCMODE);
                break;
            case OutputModeManager.IS_DRC_RF:
                outputModeManager.enableDobly_DRC(false);
                outputModeManager.setDoblyMode(OutputModeManager.RF_DRCMODE);
                break;
            }

            //use to check whether disable camera or not
            new UsbCameraManager(context).bootReady();
            new DolbyVisionSettingManager(context).initSetDolbyVision();
            new PlayBackManager(context).initHdmiSelfadaption();

            if (needCecExtend(sm, context)) {
                new HdmiCecExtend(context);
            }

            //start optimization service
            context.startService(new Intent(context, Optimization.class));

            if (context.getPackageManager().hasSystemFeature(NetflixService.FEATURE_SOFTWARE_NETFLIX)) {
                context.startService(new Intent(context, NetflixService.class));
            }

            context.startService(new Intent(context,NtpService.class));

            if (sm.getPropertyBoolean("net.wifi.suspend", false))
                context.startService(new Intent(context, WifiSuspendService.class));

            if (sm.getPropertyBoolean("ro.platform.has.tvuimode", false))
                context.startService(new Intent(context, EsmService.class));

            Intent gattServiceIntent = new Intent(context, DialogBluetoothService.class);
            context.startService(gattServiceIntent);
            String rotProp = sm.getPropertyString("persist.sys.app.rotation", "");
            ContentResolver res = context.getContentResolver();
            int acceRotation = Settings.System.getIntForUser(res,
                Settings.System.ACCELEROMETER_ROTATION,
                0,
                UserHandle.USER_CURRENT);
            if (rotProp != null && ("middle_port".equals(rotProp) || "force_land".equals(rotProp))) {
                    if (0 != acceRotation) {
                        Settings.System.putIntForUser(res,
                            Settings.System.ACCELEROMETER_ROTATION,
                            0,
                            UserHandle.USER_CURRENT);
                    }
            }
        }
    }

    private boolean needCecExtend(SystemControlManager sm, Context context) {
        return sm.getPropertyInt("ro.hdmi.device_type", -1) == HdmiDeviceInfo.DEVICE_PLAYBACK;
    }
}
