
package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Settings;
import android.content.ContentResolver;
import android.util.Log;
import android.media.AudioManager;
//import android.media.AudioSystem;
import android.provider.Settings;

//import android.view.IWindowManager;
//import android.os.ServiceManager;
//import android.app.KeyguardManager;
//import android.app.KeyguardManager.KeyguardLock;

import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
//import com.android.internal.policy.IKeyguardExitCallback;
//import com.android.internal.policy.IKeyguardService;
import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;


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
    private static final String DECRYPT_STATE = "encrypted";
    private static final String DECRYPT_TYPE = "file";
    private static final String DROID_SETTINGS_PACKAGE = "com.droidlogic.tv.settings";
    private static final String DROID_SETTINGS_ENCRYPTKEEPERFBE = "com.droidlogic.tv.settings.CryptKeeperFBE";

    //IKeyguardService mService = null;
    //RemoteServiceConnection mConnection;
    private SystemControlEvent sce =  null;
    AudioManager mAudioManager;
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            final ContentResolver resolver = context.getContentResolver();
            final SystemControlManager sm =  SystemControlManager.getInstance();
            //register system control callback
            sce = new SystemControlEvent(context);
            sm.setListener(sce);

            mAudioManager = (AudioManager) context.getSystemService(context.AUDIO_SERVICE);
            final OutputModeManager outputModeManager = new OutputModeManager(context);

            if (SettingsPref.getFirstRun(context)) {
                Log.i(TAG, "first running: " + context.getPackageName());

                //workround for O-MR1 GTVS used for P
                //Settings.Global.putString(resolver, Settings.Global.HIDDEN_API_BLACKLIST_EXEMPTIONS, "*");

                /*try {
                    Settings.Global.putInt(resolver,
                            OutputModeManager.DIGITAL_SOUND, OutputModeManager.IS_PCM);
                    Settings.Global.putInt(resolver,
                            OutputModeManager.DRC_MODE, OutputModeManager.IS_DRC_LINE);
                    Settings.Global.putInt(resolver, Settings.Global.CAPTIVE_PORTAL_DETECTION_ENABLED, 0);
                    //set default show_ime_with_hard_keyboard 1, then first boot can show the ime.
                    Settings.Secure.putInt(resolver, Settings.Secure.SHOW_IME_WITH_HARD_KEYBOARD, 1);
                    Settings.Secure.putInt(resolver, Settings.Secure.TV_USER_SETUP_COMPLETE, 1);
                    Settings.Global.putInt(resolver, Settings.Global.REQUIRE_PASSWORD_TO_DECRYPT, 0);
                } catch (NumberFormatException e) {
                    Log.e(TAG, "could not find hard keyboard ", e);
                }*/

                SettingsPref.setFirstRun(context, false);
            }

           /* final int digitalSoundValue = Settings.Global.getInt(resolver,
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
            }*/

            /*setThisValue for dts scale*/
            outputModeManager.setDtsDrcScaleSysfs();

            //DroidVoldManager
            //new DroidVoldManager(context);

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

            if (sm.getPropertyBoolean("ro.vendor.platform.has.tvuimode", false))
                context.startService(new Intent(context, EsmService.class));

            Intent gattServiceIntent = new Intent(context, DialogBluetoothService.class);
            context.startService(gattServiceIntent);

            /*  AML default rotation config, cannot use with shipping_api_level=28
            String rotProp = sm.getPropertyString("persist.vendor.sys.app.rotation", "");
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
            */

            /* @hidden-api-issue-start
            Log.d(TAG,"setWireDeviceConnectionState");
            //simulate DEVPATH=/devices/virtual/amhdmitx/amhdmitx0/hdmi_audio uevent funtion
            audioManager.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_HDMI, (outputModeManager.isHDMIPlugged() == true) ? 1 : 0, "", "");
            @hidden-api-issue-end */
            setWiredDeviceConnectionState(SystemControlEvent.DEVICE_OUT_AUX_DIGITAL, (outputModeManager.isHDMIPlugged() == true) ? 1 : 0, "", "");
            //bindKeyguardService(context);

            // Dissmiss keyguard first.
            /* used for quick bootup
            final IWindowManager wm = IWindowManager.Stub
                    .asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
            try {
                //wm.dismissKeyguard(null);
            } catch (Exception e) {
                // ignore it
            }

            KeyguardManager km= (KeyguardManager)context.getSystemService(Context.KEYGUARD_SERVICE);
            KeyguardLock kl = km.newKeyguardLock("unLock");
            kl.disableKeyguard();
            */

            enableCryptKeeperComponent(context);
        }
    }

        private void setWiredDeviceConnectionState(int type, int state, String address, String name) {
        try {
            Class<?> audioManager = Class.forName("android.media.AudioManager");
            Method setwireState = audioManager.getMethod("setWiredDeviceConnectionState",
                                    int.class, int.class, String.class, String.class);
        Log.d(TAG,"setWireDeviceConnectionState "+setwireState);

            setwireState.invoke(mAudioManager, type, state, address, name);

        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    private void enableCryptKeeperComponent(Context context) {
        String state = SystemProperties.get("ro.crypto.state");
        String type = SystemProperties.get("ro.crypto.type");
        boolean isMultiUser = UserManager.supportsMultipleUsers();
        if (("".equals(state) || !DECRYPT_STATE.equals(state) || !DECRYPT_TYPE.equals(type)) || !isMultiUser) {
            return;
        }

        PackageManager pm = context.getPackageManager();
        ComponentName name = new ComponentName(DROID_SETTINGS_PACKAGE, DROID_SETTINGS_ENCRYPTKEEPERFBE);
        Log.d(TAG, "enableCryptKeeperComponent " + name);
        try {
            pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                    PackageManager.DONT_KILL_APP);
        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
    }

    private boolean needCecExtend(SystemControlManager sm, Context context) {
        //return sm.getPropertyInt("ro.hdmi.device_type", -1) == HdmiDeviceInfo.DEVICE_PLAYBACK;
        return true;
    }

    /*
    class KeyguardExitCallback extends IKeyguardExitCallback.Stub {
        @Override
        public void onKeyguardExitResult(final boolean success) throws RemoteException {
            Log.i(TAG, "onKeyguardExitResult: " + success);
        }
    };
    */

    private boolean needCecTv(SystemControlManager sm, Context context) {
        return sm.getPropertyInt("ro.hdmi.device_type",-1) == HdmiDeviceInfo.DEVICE_TV;
    }

/*
    private class RemoteServiceConnection implements ServiceConnection {
        public void onServiceConnected(ComponentName className, IBinder service) {
            Log.v(TAG, "onServiceConnected()");
            mService = IKeyguardService.Stub.asInterface(service);
            try {
                mService.asBinder().linkToDeath(new IBinder.DeathRecipient() {
                    @Override
                    public void binderDied() {
                    }
                }, 0);

               mService.verifyUnlock(new KeyguardExitCallback());

            } catch (RemoteException e) {
                Log.w(TAG, "Couldn't linkToDeath");
                e.printStackTrace();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            Log.v(TAG, "onServiceDisconnected()");
            mService = null;
        }
    };

    private void bindKeyguardService(Context ctx) {
        if (mConnection == null) {
            mConnection = new RemoteServiceConnection();
            Intent intent = new Intent();
            intent.setClassName("com.droidlogic", "com.droidlogic.StubKeyguardService");
            Log.v(TAG, "BINDING SERVICE: " + "com.droidlogic.StubKeyguardService");
            if (!ctx.getApplicationContext().bindService(intent, mConnection, Context.BIND_AUTO_CREATE)) {
                Log.v(TAG, "FAILED TO BIND TO KEYGUARD!");
            }
        } else {
            Log.v(TAG, "Service already bound");
        }
    }
*/
}
