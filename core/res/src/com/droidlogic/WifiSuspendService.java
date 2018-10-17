package com.droidlogic;

import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.util.Log;

import com.droidlogic.app.SystemControlManager;

public class WifiSuspendService extends Service {
    private static final String TAG = "WifiSuspendService";
    private boolean mWifiDisableWhenSuspend = false;

    private BroadcastReceiver mReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "action:" + action);
            WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                if (mWifiDisableWhenSuspend == true) {
                    try {
                        wm.setWifiEnabled(true);
                        mWifiDisableWhenSuspend = false;
                    } catch (Exception e) {
                        /* ignore - local call */
                    }
                }
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                int wifiState = wm.getWifiState();
                if (wifiState == WifiManager.WIFI_STATE_ENABLING
                        || wifiState == WifiManager.WIFI_STATE_ENABLED) {
                    try {
                        wm.setWifiEnabled(false);
                        mWifiDisableWhenSuspend = true;
                    } catch (Exception e) {
                        /* ignore - local call */
                    }
                    try {
                        Thread.sleep(2300);
                    } catch (InterruptedException ignore) {
                    }
                }
            }
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_SCREEN_ON);
        registerReceiver (mReceiver, filter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}

