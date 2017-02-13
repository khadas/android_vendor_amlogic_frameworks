package com.droidlogic.app.tv;

import android.content.Context;
import android.provider.Settings;
import android.os.SystemClock;

import java.util.Date;

import com.droidlogic.app.SystemControlManager;

/**
 *TV时间管理
 */
public class TVTime{
    private long diff = 0;
    private Context mContext;
    private SystemControlManager mSystemControlManager;

    private final static String TV_KEY_TVTIME = "dtvtime";
    private final static String PROP_SET_SYSTIME_ENABLED = "persist.sys.getdtvtime.isneed";

    /**
     *创建时间管理器
     */
    public TVTime(Context context){
        mContext = context;
        mSystemControlManager = new SystemControlManager(mContext);
    }

    /**
     *设定当前时间
     *@param time 当前时间（毫秒单位）
     */
    public synchronized void setTime(long time){
        Date sys = new Date();

        diff = time - sys.getTime();
        if (mSystemControlManager.getPropertyBoolean(PROP_SET_SYSTIME_ENABLED, false)
                && (Math.abs(diff) > 1000)) {
            SystemClock.setCurrentTimeMillis(time);
            diff = 0;
        }

        Settings.System.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, diff);
    }

    /**
     *取得当前时间
     *@return 返回当前时间
     */
    public synchronized long getTime(){
        Date sys = new Date();
        diff = Settings.System.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);

        return sys.getTime() + diff;
    }

    /**
     *取得TDT/STT与系统时间的差值
     *@return 返回差值时间
     */
    public synchronized long getDiffTime(){
        return Settings.System.getLong(mContext.getContentResolver(), TV_KEY_TVTIME, 0);
    }

    /**
     *设置TDT/STT与系统时间的差值
     */
    public synchronized void setDiffTime(long diff){
        this.diff = diff;
        Settings.System.putLong(mContext.getContentResolver(), TV_KEY_TVTIME, this.diff);
    }
}

