package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import com.droidlogic.app.SystemControlManager;

public class EthernetWifiSwitch extends BroadcastReceiver {
    private static final String TAG = "EthernetWifiSwitch";
    private static final String NETCONDITION_LED = "/sys/class/leds/led-net/brightness";
    private SystemControlManager mSystemControlManager;
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "action: " + action);
        if (ConnectivityManager.CONNECTIVITY_ACTION.equals(action)) {
            WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo ethInfo = cm.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
            NetworkInfo netInfo = (NetworkInfo)intent.getExtra(ConnectivityManager.EXTRA_NETWORK_INFO, null);
            mSystemControlManager = new SystemControlManager(context);
            if (netInfo != null) {
                if (netInfo.getState() == NetworkInfo.State.CONNECTED) {
                    Log.d ( TAG ,"TypeName " + netInfo.getTypeName()+ " connected");
                    if (mSystemControlManager != null) {
                        mSystemControlManager.writeSysFs(NETCONDITION_LED, "1");
                    }
                } else if (netInfo.getState() == NetworkInfo.State.DISCONNECTED) {
                    Log.d ( TAG ,"TypeName " + netInfo.getTypeName()+ " disconnected");
                    if (mSystemControlManager != null) {
                        mSystemControlManager.writeSysFs(NETCONDITION_LED, "0");
                    }
                }
            }
            if (ethInfo.getState() == NetworkInfo.State.CONNECTED) {
                int wifiState = wm.getWifiState();
                if ((wifiState == WifiManager.WIFI_STATE_ENABLING) ||
                    (wifiState == WifiManager.WIFI_STATE_ENABLED)) {
                    wm.setWifiEnabled(false);
                    SettingsPref.setWiFiSaveState(context, 1);
                }
            }
            else {
                if (1 == SettingsPref.getWiFiSaveState(context)) {
                    wm.setWifiEnabled(true);
                    SettingsPref.setWiFiSaveState(context, 0);
                }
            }
        }
    }
}

