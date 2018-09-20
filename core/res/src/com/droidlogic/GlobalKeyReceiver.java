package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import com.droidlogic.app.UsbCameraManager;

public class GlobalKeyReceiver extends BroadcastReceiver {
    private static final String TAG = "GlobalKeyReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        //Log.i(TAG, "action: " + action);
        if (action.equals(Intent.ACTION_GLOBAL_BUTTON)) {
            Intent itent = new Intent();
            ComponentName cn = new ComponentName("com.nes.blerc","com.nes.blerc.MainActivity");
            itent.setComponent(cn);
            itent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(itent);
        }

    }
}
