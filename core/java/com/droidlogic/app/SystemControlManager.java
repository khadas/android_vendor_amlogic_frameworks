package com.droidlogic.app;

import android.content.Context;
import android.os.IBinder;
import android.os.HwBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;

import java.util.NoSuchElementException;

import android.hidl.manager.V1_0.IServiceManager;
import android.hidl.manager.V1_0.IServiceNotification;
import vendor.amlogic.hardware.systemcontrol.V1_0.ISystemControl;
import vendor.amlogic.hardware.systemcontrol.V1_0.ISystemControlCallback;
import vendor.amlogic.hardware.systemcontrol.V1_0.Result;
import vendor.amlogic.hardware.systemcontrol.V1_0.DroidDisplayInfo;

public class SystemControlManager {
    private static final String TAG                 = "SysControlManager";

    //must sync with DisplayMode.h
    public static final boolean USE_BEST_MODE       = false;
    public static final int DISPLAY_TYPE_NONE       = 0;
    public static final int DISPLAY_TYPE_TABLET     = 1;
    public static final int DISPLAY_TYPE_MBOX       = 2;
    public static final int DISPLAY_TYPE_TV         = 3;

    private ISystemControl mProxy = null;

    // Notification object used to listen to the start of the system control daemon.
    private final ServiceNotification mServiceNotification = new ServiceNotification();

    private static final int SYSTEM_CONTROL_DEATH_COOKIE = 1000;

    private Context mContext;
    private IBinder mIBinder = null;

    // Mutex for all mutable shared state.
    private final Object mLock = new Object();

    public SystemControlManager(Context context) {
        mContext = context;

        try {
            boolean ret = IServiceManager.getService()
                    .registerForNotifications("vendor.amlogic.hardware.systemcontrol@1.0::ISystemControl", "", mServiceNotification);
            if (!ret) {
                Log.e(TAG, "Failed to register service start notification");
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to register service start notification", e);
            return;
        }
        connectToProxy();
    }

    private void connectToProxy() {
        synchronized (mLock) {
            if (mProxy != null) {
                return;
            }

            try {
                mProxy = ISystemControl.getService();
                mProxy.linkToDeath(new DeathRecipient(), SYSTEM_CONTROL_DEATH_COOKIE);
            } catch (NoSuchElementException e) {
                Log.e(TAG, "connectToProxy: system control service not found."
                        + " Did the service fail to start?", e);
            } catch (RemoteException e) {
                Log.e(TAG, "connectToProxy: system control service not responding", e);
            }
        }
    }

    public String getProperty(String prop) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getProperty(prop, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getProperty:" + e);
            }
        }
        return "";
    }

    public String getPropertyString(String prop, String def) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyString(prop, def, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyString:" + e);
            }
        }

        return "";
    }

    public int getPropertyInt(String prop, int def) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyInt(prop, def, (int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyInt:" + e);
            }
        }

        return 0;
    }

    public long getPropertyLong(String prop, long def) {
        synchronized (mLock) {
            Mutable<Long> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyLong(prop, def, (int ret, long v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyLong:" + e);
            }
        }

        return 0;
    }

    public boolean getPropertyBoolean(String prop, boolean def) {
        synchronized (mLock) {
            Mutable<Boolean> resultVal = new Mutable<>();
            try {
                mProxy.getPropertyBoolean(prop, def, (int ret, boolean v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getPropertyBoolean:" + e);
            }
        }

        return false;
    }

    public void setProperty(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.setProperty(prop, val);
            } catch (RemoteException e) {
                Log.e(TAG, "setProperty:" + e);
            }
        }
    }

    public String readSysFs(String path) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readSysfs(path, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readSysFs:" + e);
            }
        }

        return "";
    }

    public boolean writeSysFs(String path, String val) {
        synchronized (mLock) {
            try {
                mProxy.writeSysfs(path, val);
            } catch (RemoteException e) {
                Log.e(TAG, "writeSysFs:" + e);
            }
        }

        return true;
    }

    public boolean writeSysFs(String path, String val, int def) {
        synchronized (mLock) {
            try {
                mProxy.writeSysfs(path, val);
            } catch (RemoteException e) {
                Log.e(TAG, "writeSysFs:" + e);
            }
        }

        return true;
    }

    public int readHdcpRX22Key(String val, int size) {
        /*
        synchronized (mLock) {
            Mutable<Integer> lenVal = new Mutable<>();
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readHdcpRX22Key(size, (int ret, String val, int len) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                    lenVal.value = len;
                                }
                            });
                val = resultVal.value;
                return lenVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readHdcpRX22Key:" + e);
            }
        }
        */

        return 0;
    }

    public boolean writeHdcpRX22Key(String val, int def) {
        synchronized (mLock) {
            try {
                mProxy.writeHdcpRX22Key(val);
            } catch (RemoteException e) {
                Log.e(TAG, "writeHdcpRX22Key:" + e);
            }
        }

        return true;
    }

    public int readHdcpRX14Key(String val, int size) {
        /*
        synchronized (mLock) {
            Mutable<Integer> lenVal = new Mutable<>();
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readHdcpRX14Key(size, (int ret, String val, int len) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                    lenVal.value = len;
                                }
                            });
                val = resultVal.value;
                return lenVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readHdcpRX14Key:" + e);
            }
        }
        */
        return 0;
    }

    public boolean writeHdcpRX14Key(String val, int def) {
        synchronized (mLock) {
            try {
                mProxy.writeHdcpRX14Key(val);
            } catch (RemoteException e) {
                Log.e(TAG, "writeHdcpRX14Key:" + e);
            }
        }

        return true;
    }

    public boolean writeHdcpRXImg(String path) {
        synchronized (mLock) {
            try {
                mProxy.writeHdcpRXImg(path);
            } catch (RemoteException e) {
                Log.e(TAG, "writeHdcpRXImg:" + e);
            }
        }

        return true;
    }

    public String getBootenv(String prop, String def) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getBootEnv(prop, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getBootenv:" + e);
            }
        }

        return "";
    }

    public void setBootenv(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.setBootEnv(prop, val);
            } catch (RemoteException e) {
                Log.e(TAG, "setBootenv:" + e);
            }
        }
    }

    public DisplayInfo getDisplayInfo() {
        /*DisplayInfo info = new DisplayInfo();
        synchronized (mLock) {
            Mutable<DroidDisplayInfo> resultInfo = new Mutable<>();
            try {
                mProxy.getDroidDisplayInfo((int ret, DroidDisplayInfo v) -> {
                                if (Result.OK == ret) {
                                    resultInfo.value = v;
                                }
                            });
                info.type = resultInfo.value.type;
                info.socType = resultInfo.value.socType;
                info.defaultUI = resultInfo.value.defaultUI;
                info.fb0Width = resultInfo.value.fb0w;
                info.fb0Height = resultInfo.value.fb0h;
                info.fb0FbBits = resultInfo.value.fb0bits;
                info.fb0TripleEnable = (1==resultInfo.value.fb0trip)?true:false;

                info.fb1Width = resultInfo.value.fb1w;
                info.fb1Height = resultInfo.value.fb1h;
                info.fb1FbBits = resultInfo.value.fb1bits;
                info.fb1TripleEnable = (1==resultInfo.value.fb1trip)?true:false;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplayInfo:" + e);
            }
        }*/

        return null;
    }

    public void loopMountUnmount(boolean isMount, String path){
        synchronized (mLock) {
            try {
                mProxy.loopMountUnmount(isMount?1:0, path);
            } catch (RemoteException e) {
                Log.e(TAG, "loopMountUnmount:" + e);
            }
        }
    }

    public void setMboxOutputMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setSourceOutputMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setMboxOutputMode:" + e);
            }
        }
    }

    public void setDigitalMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setDigitalMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setDigitalMode:" + e);
            }
        }
    }

    public void setOsdMouseMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setOsdMouseMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsdMouseMode:" + e);
            }
        }
    }

    public void setOsdMousePara(int x, int y, int w, int h) {
        synchronized (mLock) {
            try {
                mProxy.setOsdMousePara(x, y, w, h);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsdMousePara:" + e);
            }
        }
    }

    public void setPosition(int x, int y, int w, int h) {
        synchronized (mLock) {
            try {
                mProxy.setPosition(x, y, w, h);
            } catch (RemoteException e) {
                Log.e(TAG, "setPosition:" + e);
            }
        }
    }

    public int[] getPosition(String mode) {
        int[] curPosition = { 0, 0, 1280, 720 };
        synchronized (mLock) {
            Mutable<Integer> left = new Mutable<>();
            Mutable<Integer> top = new Mutable<>();
            Mutable<Integer> width = new Mutable<>();
            Mutable<Integer> height = new Mutable<>();
            try {
                mProxy.getPosition(mode, (int ret, int x, int y, int w, int h) -> {
                                if (Result.OK == ret) {
                                    left.value = x;
                                    top.value = y;
                                    width.value = w;
                                    height.value = h;
                                }
                            });
                curPosition[0] = left.value;
                curPosition[1] = top.value;
                curPosition[2] = width.value;
                curPosition[3] = height.value;
                return curPosition;
            } catch (RemoteException e) {
                Log.e(TAG, "getPosition:" + e);
            }
        }
        return curPosition;
    }

    public String getDeepColorAttr(String mode) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getDeepColorAttr(mode, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDeepColorAttr:" + e);
            }
        }
        return "";
    }

    public long resolveResolutionValue(String mode) {
        synchronized (mLock) {
            Mutable<Long> resultVal = new Mutable<>();
            try {
                mProxy.resolveResolutionValue(mode, (int ret, long v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "resolveResolutionValue:" + e);
            }
        }
        return -1;
    }

    public String isTvSupportDolbyVision() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.sinkSupportDolbyVision((int ret, String v, boolean support) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "isTvSupportDolbyVision:" + e);
            }
        }
        return "";
    }

    public void setDolbyVisionEnable(int state) {
        synchronized (mLock) {
            try {
                mProxy.setDolbyVisionState(state);
            } catch (RemoteException e) {
                Log.e(TAG, "setDolbyVisionEnable:" + e);
            }
        }
    }

    public void saveDeepColorAttr(String mode, String dcValue) {
        synchronized (mLock) {
            try {
                mProxy.saveDeepColorAttr(mode, dcValue);
            } catch (RemoteException e) {
                Log.e(TAG, "saveDeepColorAttr:" + e);
            }
        }
    }

    public void setHdrMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setHdrMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setHdrMode:" + e);
            }
        }
    }

    public void setSdrMode(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setSdrMode(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setSdrMode:" + e);
            }
        }
    }

    /**
     * that use by droidlogic-res.apk only, because need have one callback only
     *
     * @hide
     */
    public void setListener(ISystemControlCallback listener) {
        //Log.i(TAG, "setListener");
        synchronized (mLock) {
            try {
                mProxy.setCallback(listener);
            } catch (RemoteException e) {
                Log.e(TAG, "setCallback:" + e);
            }
        }
    }

    public int set3DMode(String mode3d) {
        Log.i(TAG, "[set3DMode]mode3d:" + mode3d);
        synchronized (mLock) {
            try {
                mProxy.set3DMode(mode3d);
            } catch (RemoteException e) {
                Log.e(TAG, "set3DMode:" + e);
            }
        }

        return 0;
    }

    /**
     * Close 3D mode, include 3D setting and OSD display setting.
     */
    public void init3DSettings() {
        synchronized (mLock) {
            try {
                mProxy.init3DSetting();
            } catch (RemoteException e) {
                Log.e(TAG, "init3DSettings:" + e);
            }
        }
    }

    /**
     * Get 3D format for current playing video, include local, streaming and HDMI input.
     * return format is setted by video parser, such as libplayer for amlogic
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getVideo3DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getVideo3DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getVideo3DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * Get display 3D format setted by setDisplay3DTo2DFormat.
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getDisplay3DTo2DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDisplay3DTo2DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DTo2DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * Set 3D format for video, this format is decided by user,
     * if LCD isn't support 3D and you wanna play a 3D file, use the api to show part picture of the video,
     * such as the left side of the 3D video source or the top side one.
     *
     * @param 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     *
     * @return set status
     */
    public boolean setDisplay3DTo2DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setDisplay3DTo2DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setDisplay3DTo2DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Set 3D format for OSD and video, this format is decided by user,
     * if LCD is support 3D, use the api to set OSD and video 3D format.
     *
     * @param 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     *
     * @return set status
     */
    public boolean setDisplay3DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setDisplay3DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setDisplay3DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Get display 3D format setted by setDisplay3DFormat.
     *
     * @return 3D format
     * FORMAT_3D_OFF
     * FORMAT_3D_AUTO
     * FORMAT_3D_SIDE_BY_SIDE
     * FORMAT_3D_TOP_AND_BOTTOM
     */
    public int getDisplay3DFormat() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDisplay3DFormat((int ret, int v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DFormat:" + e);
            }
        }
        return -1;
    }

    /**
     * for subtitle, maybe unnecessary
     */
    public boolean setOsd3DFormat(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.setOsd3DFormat(format);
            } catch (RemoteException e) {
                Log.e(TAG, "setOsd3DFormat:" + e);
            }
        }

        return true;
    }

    /**
     * Switch 3D to 2D for video, this api is used for tv platform if user wanna watch movie part of 3D files,
     * take left side or top side for example
     *
     * @param 3D format
     * FORMAT_3D_TO_2D_LEFT_EYE
     * FORMAT_3D_TO_2D_RIGHT_EYE
     *
     * @return set status
     */
    public boolean switch3DTo2D(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.switch3DTo2D(format);
            } catch (RemoteException e) {
                Log.e(TAG, "switch3DTo2D:" + e);
            }
        }

        return true;
    }

    /**
     * // TODO: haven't implemented yet
     */
    public boolean switch2DTo3D(int format) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.switch2DTo3D(format);
            } catch (RemoteException e) {
                Log.e(TAG, "switch2DTo3D:" + e);
            }
        }

        return true;
    }

    private static class Mutable<E> {
        public E value;

        Mutable() {
            value = null;
        }

        Mutable(E value) {
            this.value = value;
        }
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
        }

        @Override
        public void serviceDied(long cookie) {
            if (SYSTEM_CONTROL_DEATH_COOKIE == cookie) {
                Log.e(TAG, "system control service died cookie: " + cookie);
                synchronized (mLock) {
                    mProxy = null;
                }
            }
        }
    }

    final class ServiceNotification extends IServiceNotification.Stub {
        @Override
        public void onRegistration(String fqName, String name, boolean preexisting) {
            Log.i(TAG, "system control service started " + fqName + " " + name);
            connectToProxy();
        }
    }

    public static class DisplayInfo{
        /*//1:tablet 2:MBOX 3:TV
        public int type;
        public String socType;
        public String defaultUI;
        public int fb0Width;
        public int fb0Height;
        public int fb0FbBits;
        public boolean fb0TripleEnable;//Triple Buffer enable or not

        public int fb1Width;
        public int fb1Height;
        public int fb1FbBits;
        public boolean fb1TripleEnable;//Triple Buffer enable or not*/
    }
}
