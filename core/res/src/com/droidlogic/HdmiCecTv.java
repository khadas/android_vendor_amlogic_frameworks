
package com.droidlogic;

import android.content.Context;
import android.content.ContentResolver;
import android.provider.Settings.Global;
import android.database.ContentObserver;
import android.os.Handler;
import com.droidlogic.app.HdmiCecManager;
import android.util.Log;
import android.net.Uri;
import android.os.Message;
import android.os.UserHandle;
//import android.os.ServiceManager;

public class HdmiCecTv {
    private static final String TAG = "HdmiCecTv";
    private static final int DISABLED = 0;
    private static final int ENABLED = 1;
    private static final String HDMI_CONTROL_ENABLED = "hdmi_control_enabled";
    private Context mContext;
    private SettingsObserver mSettingsObserver;
    private Handler mHandler = new Handler();
    public HdmiCecTv(Context ctx) {
        mContext = ctx;
        mSettingsObserver = new SettingsObserver(mHandler);
        updatehdmirx0(true);
        /*only system process has the permisson to observe the changes of settings db*/
        //registerContentObserver();
    }

   /* private void registerContentObserver() {
        ContentResolver resolver = mContext.getContentResolver();
        String[] settings = new String[] {
                Global.HDMI_CONTROL_ENABLED
        };
        for (String s : settings) {
            resolver.registerContentObserver(Global.getUriFor(s), false, mSettingsObserver,
                    UserHandle.USER_ALL);
        }
    }*/

    private void updatehdmirx0(boolean started) {
        ContentResolver cr = mContext.getContentResolver();
        //==> hdmi_control_enabled==HDMI_CONTROL_ENABLED
        boolean enable = Global.getInt(cr, HDMI_CONTROL_ENABLED, ENABLED) == ENABLED;
        HdmiCecManager mm = new HdmiCecManager(mContext);
        if (!enable) {
            mm.setCecEnable("0");
        }else if(started) {
            mm.setCecEnable("2");
        }else {
            mm.setCecEnable("1");
        }
    }
    private class SettingsObserver extends ContentObserver {
        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            String option = uri.getLastPathSegment();
            Log.d(TAG, "onChange, option = " + option);
            if (option.equals(HDMI_CONTROL_ENABLED)) {
                updatehdmirx0(false);
            }
        }
    }
}
