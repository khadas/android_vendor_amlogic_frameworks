package com.droidlogic;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.Service;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.os.SystemProperties;
import android.util.Log;

import java.lang.Runnable;
import java.lang.Thread;
import java.util.List;

public class Optimization extends Service {
    private static String TAG = "Optimization";
    private Context mContext;

    static {
        System.loadLibrary("optimization");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (SystemProperties.getBoolean("ro.app.optimization", true)) {
            new Thread(runnable).start();
        }
        else {
            Log.i(TAG, "Optimization service not start");
        }

        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private Runnable runnable = new Runnable() {
        public void run() {
            int retProc = -1;
            int retPkg = -1;
            boolean isInProcOpt = false;
            boolean isInPkgOpt = false;
            ActivityManager am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);

            while (true) {
                try {
                    if (!isInProcOpt) {
                        List< ActivityManager.RunningTaskInfo > task = am.getRunningTasks (1);
                        if (!task.isEmpty()) {
                            ComponentName cn = task.get (0).topActivity;
                            String pkg = cn.getPackageName();
                            String cls = cn.getClassName();

                            retPkg = nativeOptimization(pkg, cls);//bench match
                            if (retPkg == 0) {//0:PKG_BENCH, -4:PKG_SAME
                                isInPkgOpt = true;
                            }
                            else if (retPkg != -4) {
                                isInPkgOpt = false;
                            }
                        }
                    }

                    if (!isInPkgOpt) {
                        List< ActivityManager.RunningAppProcessInfo> apInfo = am.getRunningAppProcesses();
                        int len = apInfo.size();
                        //Log.i(TAG, "apInfo.size():" + len);
                        String [] proc = new String[len];
                        for (int i = 0; i < len; i++) {
                            //Log.i(TAG, "apInfo[" + i + "] processName:" + apInfo.get(i).processName);
                            proc[i] = apInfo.get(i).processName;
                        }
                        retProc = nativeOptimization(proc);
                        if (retProc == 0) {
                            isInProcOpt = true;
                        }
                        else if (retProc != -4) {
                            isInProcOpt = false;
                        }
                    }

                    Thread.sleep(50);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    };

    private native int nativeOptimization(String pkg, String cls);
    private native int nativeOptimization(String[] proc);
}
