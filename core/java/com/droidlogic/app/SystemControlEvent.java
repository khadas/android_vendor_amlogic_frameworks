package com.droidlogic.app;

import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.media.AudioManager;

//this event from native system control service
public class SystemControlEvent extends ISystemControlNotify.Stub {
    private static final String TAG                             = "SystemControlEvent";

    public static final String ACTION_SYSTEM_CONTROL_EVENT      = "android.intent.action.SYSTEM_CONTROL_EVENT";
    public final static String ACTION_HDMI_PLUGGED = "android.intent.action.HDMI_PLUGGED";
    public final static String EXTRA_HDMI_PLUGGED_STATE = "state";
    public static final String EVENT_TYPE                       = "event";

    //must sync with DisplayMode.h
    public static final int EVENT_OUTPUT_MODE_CHANGE            = 0;
    public static final int EVENT_DIGITAL_MODE_CHANGE           = 1;
    public static final int EVENT_HDMI_PLUGGED           = 2;
    public static final int EVENT_HDMI_AUDIO_OUT           = 3;
    public static final int EVENT_HDMI_AUDIO_IN           = 4;


    private Context mContext = null;
    private final AudioManager mAudioManager;

    public SystemControlEvent(Context context) {
        mContext = context;
        mAudioManager = (AudioManager)context.getSystemService(Context.AUDIO_SERVICE);
    }

    @Override
    public void onEvent(int event) {
        Log.i(TAG, "system control callback event: " + event);
        Intent intent;
        if (event == EVENT_HDMI_PLUGGED) {
            intent = new Intent(ACTION_HDMI_PLUGGED);
            int plugged =1;
            intent.putExtra(EXTRA_HDMI_PLUGGED_STATE, plugged);
        } else if (event == EVENT_HDMI_AUDIO_OUT || event == EVENT_HDMI_AUDIO_IN) {
            mAudioManager.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_HDMI, (event - EVENT_HDMI_AUDIO_OUT), "", "");
            return;
        }  else {
            intent = new Intent(ACTION_SYSTEM_CONTROL_EVENT);
            intent.putExtra(EVENT_TYPE, event);
        }
        mContext.sendStickyBroadcastAsUser(intent, android.os.UserHandle.ALL);
    }
}
