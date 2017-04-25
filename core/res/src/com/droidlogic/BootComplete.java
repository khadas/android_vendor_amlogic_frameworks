
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

import com.droidlogic.app.HdrManager;
import com.droidlogic.app.SdrManager;
import com.droidlogic.app.DolbyVisionSettingManager;
import com.droidlogic.app.PlayBackManager;
import com.droidlogic.app.SystemControlEvent;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.UsbCameraManager;
import com.droidlogic.HdmiCecExtend;
import com.droidlogic.app.OutputModeManager;

public class BootComplete extends BroadcastReceiver {
    private static final String TAG             = "BootComplete";
    private static final String FIRST_RUN       = "first_run";
    private static final int SPEAKER_DEFAULT_VOLUME = 11;
    private static final String DRC_MODE = "drc_mode";
    private static final int DRC_OFF = 0;
    private static final int DRC_LINE = 1;
    private static final int DRC_RF = 2;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            SystemControlManager sm = new SystemControlManager(context);
            //register system control callback
            sm.setListener(new SystemControlEvent(context));

            ContentResolver resolver = context.getContentResolver();
            AudioManager audioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
            int speakervalue = Settings.System.getInt(resolver,"volume_music_speaker",-1);
            if (speakervalue == -1) {
                Settings.System.putInt(resolver,"volume_music_speaker", SPEAKER_DEFAULT_VOLUME);
                if (AudioSystem.PLATFORM_TELEVISION != AudioSystem.getPlatformType(context)) {
                    int current = audioManager.getStreamVolume( AudioManager.STREAM_MUSIC);
                    audioManager.setStreamVolume(AudioManager.STREAM_MUSIC,current,0);
                }
            }

            OutputModeManager drcomm = new OutputModeManager(context);
            int drcvalue = Settings.Global.getInt(context.getContentResolver(), DRC_MODE, DRC_LINE);
            Log.d(TAG,"read drcmode value: "+drcvalue);
            switch (drcvalue) {
                case DRC_OFF:
                    drcomm.enableDobly_DRC(false);
                    Settings.Global.putInt(context.getContentResolver(),DRC_MODE, drcvalue);
                    break;
                case DRC_LINE:
                    drcomm.enableDobly_DRC(true);
                    drcomm.setDoblyMode(String.valueOf(DRC_LINE));
                    Settings.Global.putInt(context.getContentResolver(),DRC_MODE, drcvalue);
                    break;
                case DRC_RF:
                    drcomm.setDoblyMode(String.valueOf(DRC_RF));
                    Settings.Global.putInt(context.getContentResolver(),DRC_MODE, drcvalue);
                    break;
            }

            //set default show_ime_with_hard_keyboard 1, then first boot can show the ime.
            if (SettingsPref.getFirstRun(context)) {
                Log.i(TAG, "first running: " + context.getPackageName());
                try {
                    Settings.Secure.putInt(context.getContentResolver(),
                            Settings.Secure.SHOW_IME_WITH_HARD_KEYBOARD, 1);
                    Settings.Global.putInt(context.getContentResolver(),
                            Settings.Global.CAPTIVE_PORTAL_DETECTION_ENABLED, 0);
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

            //use to check whether disable camera or not
            new UsbCameraManager(context).bootReady();

            new PlayBackManager(context).initHdmiSelfadaption();

            if (needCecExtend(sm, context)) {
                new HdmiCecExtend(context);
            }

            new HdrManager(context).initHdrMode();

            new SdrManager(context).initSdrMode();
            new DolbyVisionSettingManager(context).initDolbyVision();
            //start optimization service
            context.startService(new Intent(context, Optimization.class));

            if (context.getPackageManager().hasSystemFeature(NetflixService.FEATURE_SOFTWARE_NETFLIX)) {
                context.startService(new Intent(context, NetflixService.class));
            }

            context.startService(new Intent(context,NtpService.class));

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

