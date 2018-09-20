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
import vendor.amlogic.hardware.systemcontrol.V1_0.SourceInputParam;

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

    public int[] paddingBuffer(int[] src, int def, int len) {
        int[] data;
        data = new int[len];
        synchronized (mLock) {
            try {
                int i;
                for (i = 0; i < def; ++i) {
                    data[i] = src[i];
                }
                for (; i < len; ++i) {
                    data[i] = 0;
                }
            } catch (Exception e) {
                Log.e(TAG, "padding_buffer:" + e);
            }
        }

        return data;
    }

    public boolean writeSysFs(String path, int[] val, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > 4096) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(val, def, 4096);
                mProxy.writeSysfsBin(path, data, def);
            } catch (RemoteException e) {
                Log.e(TAG, "writeSysFs:" + e);
            }
        }

        return true;
    }

    public int readHdcpRX22Key(int[] val, int size) {
        /*
        synchronized (mLock) {
            Mutable<Integer> lenVal = new Mutable<>();
            try {
                mProxy.readHdcpRX22Key(size, (int ret, final int[] v, int len) -> {
                                if (Result.OK == ret) {
                                    for (int i = 0; i < len; i++)
                                        val[i] = v[i];
                                    lenVal.value = len;
                                }
                            });
                return lenVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readHdcpRX22Key:" + e);
            }
        }
        */

        return 0;
    }

    public boolean writeHdcpRX22Key(int[] val, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > 4096) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(val, def, 4096);
                mProxy.writeHdcpRX22Key(data, def);
            } catch (RemoteException e) {
                Log.e(TAG, "writeHdcpRX22Key:" + e);
            }
        }

        return true;
    }

    public boolean writeAttestationKey(String node, String name, int[] key, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > 10240) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(key, def, 10240);
                mProxy.writeAttestationKey(node, name, data);
            } catch (RemoteException e) {
                Log.e(TAG, "writeAttestationKey:" + e);
            }
        }

        return true;
    }

    public int readAttestationKey(String node, String name, int[] val, int size) {
        synchronized (mLock) {
            Mutable<Integer> lenVal = new Mutable<>();
            try {
                mProxy.readAttestationKey(node, name, size, (int ret, final int[] v, int len) -> {
                                if (Result.OK == ret) {
                                    for (int i = 0; i < len; i++)
                                        val[i] = v[i];
                                    lenVal.value = len;
                                }
                            });
                return lenVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readAttestationKey:" + e);
            }
        }

        return 0;
    }

    public String readUnifyKey(String path) {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.readUnifyKey(path, (int ret, String v) -> {
                                if (Result.OK == ret) {
                                    resultVal.value = v;
                                }
                            });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readUnifyKey:" + e);
            }
        }

        return "";
    }

    public void writeUnifyKey(String prop, String val) {
        synchronized (mLock) {
            try {
                mProxy.writeUnifyKey(prop, val);
            } catch (RemoteException e) {
                Log.e(TAG, "setBootenv:" + e);
            }
        }
    }

    public int readPlayreadyKey(String path, int[] val, int size) {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.readPlayreadyKey(path, size, (int ret, final int[] v, int len) -> {
                    if (Result.OK == ret) {
                        for (int i = 0; i < len; i++)
                            val[i] = v[i];
                        resultVal.value = len;
                    }
                });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readUnifyKey:" + e);
            }
        }
        return 0;
    }

    public boolean writePlayreadyKey(String path, int[] val, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > 4096) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(val, def, 4096);
                mProxy.writePlayreadyKey(path, data, def);
            } catch (RemoteException e) {
                Log.e(TAG, "writeUnifyKey:" + e);
            }
        }
        return true;
    }

    public int readHdcpRX14Key(int[] val, int size) {
        /*
        synchronized (mLock) {
            Mutable<Integer> lenVal = new Mutable<>();
            try {
                mProxy.readHdcpRX14Key(size, (int ret, final int[] v, int len) -> {
                                if (Result.OK == ret) {
                                    for (int i = 0; i < len; i++)
                                        val[i] = v[i];
                                    lenVal.value = len;
                                }
                            });
                return lenVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "readHdcpRX14Key:" + e);
            }
        }
        */
        return 0;
    }

    public boolean writeHdcpRX14Key(int[] val, int def) {
        synchronized (mLock) {
            try {
                int[] data;
                if (def > 4096) {
                    Log.e(TAG, "The data len is too long, it cannot exceed " + (String.format("%d", def)));
                    return false;
                }
                data = paddingBuffer(val, def, 4096);
                mProxy.writeHdcpRX14Key(data, def);
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
    public int getDolbyVisionType() {
        synchronized (mLock) {
            Mutable<Integer> resultVal = new Mutable<>();
            try {
                mProxy.getDolbyVisionType((int ret, int v) -> {
                    if (Result.OK == ret) {
                        resultVal.value = v;
                    }
                });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getDolbyVisionType:" + e);
            }
        }

        return 0;
    }

    public void setGraphicsPriority(String mode) {
        synchronized (mLock) {
            try {
                mProxy.setGraphicsPriority(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "setGraphicsPriority:" + e);
            }
        }
    }

    public String getGraphicsPriority() {
        synchronized (mLock) {
            Mutable<String> resultVal = new Mutable<>();
            try {
                mProxy.getGraphicsPriority((int ret, String v) -> {
                    if (Result.OK == ret) {
                        resultVal.value = v;
                    }
                });
                return resultVal.value;
            } catch (RemoteException e) {
                Log.e(TAG, "getGraphicsPriority:" + e);
            }
        }
        return "";
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

    //PQ moudle
    public enum SourceInput {
        TV(0),
        AV1(1),
        AV2(2),
        YPBPR1(3),
        YPBPR2(4),
        HDMI1(5),
        HDMI2(6),
        HDMI3(7),
        HDMI4(8),
        VGA(9),
        XXXX(10),//not use MPEG source
        DTV(11),
        SVIDEO(12),
        IPTV(13),
        DUMMY(14),
        SOURCE_SPDIF(15),
        ADTV(16),
        AUX(17),
        ARC(18),
        MAX(19);
        private int val;

        SourceInput(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

   public enum PQMode {
        PQ_MODE_STANDARD(0),
        PQ_MODE_BRIGHT(1),
        PQ_MODE_SOFTNESS(2),
        PQ_MODE_USER(3),
        PQ_MODE_MOVIE(4),
        PQ_MODE_COLORFUL(5),
        PQ_MODE_MONITOR(6),
        PQ_MODE_GAME(7),
        PQ_MODE_SPORTS(8),
        PQ_MODE_SONY(9),
        PQ_MODE_SAMSUNG(10),
        PQ_MODE_SHARP(11);

        private int val;

        PQMode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    public int LoadPQSettings(SourceInputParam srcInputParam) {
        synchronized (mLock) {
            try {
                return mProxy.loadPQSettings(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "LoadPQSettings:" + e);
            }
        }

        return -1;

    }


    public int LoadCpqLdimRegs() {
        synchronized (mLock) {
            try {
                return mProxy.loadCpqLdimRegs();
            } catch (RemoteException e) {
                Log.e(TAG, "LoadCpqLdimRegs:" + e);
            }
        }

        return -1;
    }

        /**
     * @Function: SetPQMode
     * @Description: Set current source picture mode
     * @Param: value mode refer to enum Pq_Mode, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetPQMode(int pq_mode, int is_save, int is_autoswitch) {
        synchronized (mLock) {
            try {
                return mProxy.setPQmode(pq_mode, is_save, is_autoswitch);
            } catch (RemoteException e) {
                Log.e(TAG, "SetPQMode:" + e);
            }
        }

        return -1;

    }

        /**
     * @Function: GetPQMode
     * @Description: Get current source picture mode
     * @Param: source refer to enum SourceInput
     * @Return: picture mode refer to enum Pq_Mode
     */
    public int GetPQMode() {
        synchronized (mLock) {
            try {
                return mProxy.getPQmode();
            } catch (RemoteException e) {
                Log.e(TAG, "getDisplay3DFormat:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: SavePQMode
     * @Description: Save current source picture mode
     * @Param: picture mode refer to enum Pq_Mode, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SavePQMode(int pq_mode) {
        synchronized (mLock) {
            try {
                return mProxy.savePQmode(pq_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SavePQMode:" + e);
            }
        }
        return -1;
    }

   public enum color_temperature {
        COLOR_TEMP_STANDARD(0),
        COLOR_TEMP_WARM(1),
        COLOR_TEMP_COLD(2),
        COLOR_TEMP_MAX(3);
        private int val;
        color_temperature(int val) {
            this.val = val;
        }
        public int toInt() {
            return this.val;
        }
    }

        /**
     * @Function: SetColorTemperature
     * @Description: Set current source color temperature mode
     * @Param: value mode refer to enum color_temperature, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetColorTemperature(int mode, int is_save) {
       synchronized (mLock) {
           try {
               return mProxy.setColorTemperature(mode, is_save);
           } catch (RemoteException e) {
               Log.e(TAG, "SavePQMode:" + e);
           }
       }
       return -1;

    }

        /**
     * @Function: GetColorTemperature
     * @Description: Get current source color temperature mode
     * @Param: source refer to enum SourceInput
     * @Return: color temperature refer to enum color_temperature
     */
    public int GetColorTemperature() {
        synchronized (mLock) {
            try {
                return mProxy.getColorTemperature();
            } catch (RemoteException e) {
                Log.e(TAG, "GetColorTemperature:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: SaveColorTemperature
     * @Description: Save current source color temperature mode
     * @Param: color temperature mode refer to enum color_temperature, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveColorTemperature(int mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveColorTemperature(mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveColorTemperature:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: SetBrightness
     * @Description: Set current source brightness value
     * @Param: value brightness, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetBrightness(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setBrightness(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetBrightness:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: GetBrightness
     * @Description: Get current source brightness value
     * @Param: source refer to enum SourceInput
     * @Return: value brightness
     */
    public int GetBrightness() {
          synchronized (mLock) {
          try {
              return mProxy.getBrightness();
          } catch (RemoteException e) {
              Log.e(TAG, "GetBrightness:" + e);
          }
      }
      return -1;

    }

    /**
     * @Function: SaveBrightness
     * @Description: Save current source brightness value
     * @Param: value brightness, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveBrightness(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveBrightness(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveBrightness:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: SetContrast
     * @Description: Set current source contrast value
     * @Param: value contrast, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetContrast(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setContrast(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetContrast:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetContrast
     * @Description: Get current source contrast value
     * @Param: source refer to enum SourceInput
     * @Return: value contrast
     */
    public int GetContrast() {
        synchronized (mLock) {
            try {
                return mProxy.getContrast();
            } catch (RemoteException e) {
                Log.e(TAG, "GetContrast:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveContrast
     * @Description: Save current source contrast value
     * @Param: value contrast, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveContrast(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveContrast(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveContrast:" + e);
            }
        }
        return -1;

    }

        /**
     * @Function: SetSatuation
     * @Description: Set current source saturation value
     * @Param: value saturation, source refer to enum SourceInput, fmt current fmt refer to tvin_sig_fmt_e, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetSaturation(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setSaturation(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetBrightness:" + e);
            }
        }
        return -1;

    }

    /**
       * @Function: GetSatuation
       * @Description: Get current source saturation value
       * @Param: source refer to enum SourceInput
       * @Return: value saturation
       */
      public int GetSaturation() {
        synchronized (mLock) {
            try {
                return mProxy.getSaturation();
            } catch (RemoteException e) {
                Log.e(TAG, "GetSaturation:" + e);
            }
        }
        return -1;

      }

          /**
     * @Function: SaveSaturation
     * @Description: Save current source saturation value
     * @Param: value saturation, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveSaturation(int value) {
            synchronized (mLock) {
              try {
                  return mProxy.saveSaturation(value);
              } catch (RemoteException e) {
                  Log.e(TAG, "SaveSaturation:" + e);
              }
          }
          return -1;

    }

    /**
     * @Function: SetHue
     * @Description: Set current source hue value
     * @Param: value saturation, source refer to enum SourceInput, fmt current fmt refer to tvin_sig_fmt_e, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetHue(int value, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setHue(value, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetHue:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetHue
     * @Description: Get current source hue value
     * @Param: source refer to enum SourceInput
     * @Return: value hue
     */
    public int GetHue() {
        synchronized (mLock) {
            try {
                return mProxy.getHue();
            } catch (RemoteException e) {
                Log.e(TAG, "GetHue:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveHue
     * @Description: Save current source hue value
     * @Param: value hue, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveHue(int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveHue(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveHue:" + e);
            }
        }
        return -1;


    }

    /**
     * @Function: SetSharpness
     * @Description: Set current source sharpness value
     * @Param: value saturation, source_type refer to enum SourceInput, is_enable set 1 as default
     * @Param: status_3d refer to enum Tvin_3d_Status, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetSharpness(int value, int is_enable, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setSharpness(value, is_enable, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetSharpness:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetSharpness
     * @Description: Get current source sharpness value
     * @Param: source refer to enum SourceInput
     * @Return: value sharpness
     */
    public int GetSharpness() {
        synchronized (mLock) {
            try {
                return mProxy.getSharpness();
            } catch (RemoteException e) {
                Log.e(TAG, "GetSharpness:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: SaveSharpness
     * @Description: Save current source sharpness value
     * @Param: value sharpness, source refer to enum SourceInput, isEnable set 1 enable as default
     * @Return: 0 success, -1 fail
     */
    public int SaveSharpness(int value, int isEnable) {
          synchronized (mLock) {
            try {
                return mProxy.saveSharpness(value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveHue:" + e);
            }
        }
        return -1;

    }

    public enum Noise_Reduction_Mode {
        REDUCE_NOISE_CLOSE(0),
        REDUCE_NOISE_WEAK(1),
        REDUCE_NOISE_MID(2),
        REDUCE_NOISE_STRONG(3),
        REDUCTION_MODE_AUTO(4);

        private int val;

        Noise_Reduction_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    /**
     * @Function: SetNoiseReductionMode
     * @Description: Set current source noise reduction mode
     * @Param: noise reduction mode refer to enum Noise_Reduction_Mode, source refer to enum SourceInput, is_save 1 to save
     * @Return: 0 success, -1 fail
     */
    public int SetNoiseReductionMode(int nr_mode, int is_save) {
          synchronized (mLock) {
            try {
                return mProxy.setNoiseReductionMode(nr_mode, is_save);
            } catch (RemoteException e) {
                Log.e(TAG, "SetNoiseReductionMode:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: GetNoiseReductionMode
     * @Description: Get current source noise reduction mode
     * @Param: source refer to enum SourceInput
     * @Return: noise reduction mode refer to enum Noise_Reduction_Mode
     */
    public int GetNoiseReductionMode() {
        synchronized (mLock) {
            try {
                return mProxy.getNoiseReductionMode();
            } catch (RemoteException e) {
                Log.e(TAG, "GetNoiseReductionMode:" + e);
            }
        }
        return -1;

    }

        /**
     * @Function: SaveNoiseReductionMode
     * @Description: Save current source noise reduction mode
     * @Param: noise reduction mode refer to enum Noise_Reduction_Mode, source refer to enum SourceInput
     * @Return: 0 success, -1 fail
     */
    public int SaveNoiseReductionMode(int nr_mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveNoiseReductionMode(nr_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveNoiseReductionMode:" + e);
            }
        }
        return -1;

    }

    public int SetEyeProtectionMode(int inputtSrc, int enable, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setEyeProtectionMode(inputtSrc, enable, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetEyeProtectionMode:" + e);
            }
        }
        return -1;
    }

    public int GetEyeProtectionMode(int inputtSrc) {
         synchronized (mLock) {
            try {
                return mProxy.getEyeProtectionMode(inputtSrc);
            } catch (RemoteException e) {
                Log.e(TAG, "GetEyeProtectionMode:" + e);
            }
        }
        return -1;
    }

    public int SetGammaValue(int curve, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setGammaValue(curve, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetGammaValue:" + e);
            }
        }
        return -1;

    }

    public int GetGammaValue() {
          synchronized (mLock) {
            try {
                return mProxy.getGammaValue();
            } catch (RemoteException e) {
                Log.e(TAG, "GetGammaValue:" + e);
            }
        }
        return -1;

    }

    public enum Display_Mode {
        DISPLAY_MODE_169(0),
        DISPLAY_MODE_PERSON(1),
        DISPLAY_MODE_MOVIE(2),
        DISPLAY_MODE_CAPTION(3),
        DISPLAY_MODE_MODE43(4),
        DISPLAY_MODE_FULL(5),
        DISPLAY_MODE_NORMAL(6),
        DISPLAY_MODE_NOSCALEUP(7),
        DISPLAY_MODE_CROP_FULL(8),
        DISPLAY_MODE_CROP(9),
        DISPLAY_MODE_ZOOM(10),
        DISPLAY_MODE_MAX(11);
        private int val;

        Display_Mode(int val) {
            this.val = val;
        }

        public int toInt() {
            return this.val;
        }
    }

    public int SetDisplayMode(int inputtSrc, Display_Mode mode, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setDisplayMode(inputtSrc, mode.toInt(), isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetDisplayMode:" + e);
            }
        }
        return -1;
    }

    public int GetDisplayMode(int inputtSrc) {
          synchronized (mLock) {
            try {
                return mProxy.getDisplayMode(inputtSrc);
            } catch (RemoteException e) {
                Log.e(TAG, "GetDisplayMode:" + e);
            }
        }
        return -1;
    }

    public int SaveDisplayMode(int inputtSrc, Display_Mode mode) {
          synchronized (mLock) {
            try {
                return mProxy.saveDisplayMode(inputtSrc, mode.toInt());
            } catch (RemoteException e) {
                Log.e(TAG, "SaveDisplayMode:" + e);
            }
        }
        return -1;
    }

    public int SetBacklight(int inputtSrc, int value, int isSave) {
          synchronized (mLock) {
            try {
                return mProxy.setBacklight(inputtSrc, value, isSave);
            } catch (RemoteException e) {
                Log.e(TAG, "SetBacklight:" + e);
            }
        }
        return -1;
    }

    public int GetBacklight(int inputtSrc) {
          synchronized (mLock) {
            try {
                return mProxy.getBacklight(inputtSrc);
            } catch (RemoteException e) {
                Log.e(TAG, "GetBacklight:" + e);
            }
        }
        return -1;
    }

    public int SaveBacklight(int inputtSrc, int value) {
          synchronized (mLock) {
            try {
                return mProxy.saveBacklight(inputtSrc, value);
            } catch (RemoteException e) {
                Log.e(TAG, "SaveBacklight:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryResetPQMode
     * @Description: Reset all values of PQ mode for factory menu conctrol
     * @Param:
     * @Return: 0 success, -1 fail
     */
    public int FactoryResetPQMode() {
          synchronized (mLock) {
            try {
                return mProxy.factoryResetPQMode();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryResetPQMode:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: FactoryResetColorTemp
     * @Description: Reset all values of color temperature mode for factory menu conctrol
     * @Param:
     * @Return: 0 success, -1 fail
     */
    public int FactoryResetColorTemp() {
          synchronized (mLock) {
            try {
                return mProxy.factoryResetColorTemp();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryResetColorTemp:" + e);
            }
        }
        return -1;
    }

    public int FactorySetPQParam(SourceInputParam srcInputParam, int mode, int id, int value) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetPQParam(srcInputParam, mode, id, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetPQParam:" + e);
            }
        }
        return -1;

    }

    public int FactoryGetPQParam(SourceInputParam srcInputParam, int mode, int id) {
          synchronized (mLock) {
          Mutable<Integer> resultVal = new Mutable<>();
            try {
                return mProxy.factoryGetPQParam(srcInputParam, mode, id);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetPQParam:" + e);
            }
        }
        return -1;

    }

    public int FactorySetColorTemperatureParam(int colortemperature_mode, int id, int value) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetColorTemperatureParam(colortemperature_mode, id, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetColorTemperatureParam:" + e);
            }
        }
        return -1;

    }

    public int FactoryGetColorTemperatureParam(int colortemperature_mode, int id)  {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetColorTemperatureParam(colortemperature_mode, id);
            } catch (RemoteException e) {
                Log.e(TAG, "factoryGetColorTemperatureParam:" + e);
            }
        }
        return -1;


    }

    public int FactorySaveColorTemperatureParam(int colortemperature_mode, int id, int value) {
          synchronized (mLock) {
            try {
                return mProxy.factorySaveColorTemperatureParam(colortemperature_mode, id, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySaveColorTemperatureParam:" + e);
            }
        }
        return -1;

    }

   public int FactorySetOverscan(SourceInputParam srcInputParam, int he_value, int hs_value, int ve_value, int vs_value) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetOverscan(srcInputParam, he_value, hs_value, ve_value, vs_value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySaveColorTemperatureParam:" + e);
            }
        }
        return -1;

   }

   public int FactoryGetOverscan(SourceInputParam srcInputParam, int id) {
         synchronized (mLock) {
         Mutable<Integer> resultVal = new Mutable<>();
           try {
               return mProxy.factoryGetOverscan(srcInputParam, id);
           } catch (RemoteException e) {
               Log.e(TAG, "FactoryGetOverscan:" + e);
           }
       }
       return -1;

   }

   public class noline_params_t {
       public int osd0;
       public int osd25;
       public int osd50;
       public int osd75;
       public int osd100;
   }

   public enum NOLINE_PARAMS_TYPE {
       NOLINE_PARAMS_TYPE_BRIGHTNESS(0),
       NOLINE_PARAMS_TYPE_CONTRAST(1),
       NOLINE_PARAMS_TYPE_SATURATION(2),
       NOLINE_PARAMS_TYPE_HUE(3),
       NOLINE_PARAMS_TYPE_SHARPNESS(4),
       NOLINE_PARAMS_TYPE_VOLUME(5),
       NOLINE_PARAMS_TYPE_MAX(6);

       private int val;

       NOLINE_PARAMS_TYPE(int val) {
           this.val = val;
       }

       public int toInt() {
           return this.val;
       }
   }

   public int FactorySetNolineParams(SourceInputParam srcInputParam, int type, int osd0_value, int osd25_value,
                            int osd50_value, int osd75_value, int osd100_value) {
         synchronized (mLock) {
           try {
               return mProxy.factorySetNolineParams(srcInputParam, type, osd0_value, osd25_value, osd50_value, osd75_value, osd100_value);
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySetNolineParams:" + e);
           }
       }
       return -1;
   }

   public int FactoryGetNolineParams(SourceInputParam srcInputParam, int type, int id) {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetNolineParams(srcInputParam, type, id);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetNolineParams:" + e);
            }
        }
        return -1;

   }

   public int FactorySetParamsDefault() {
         synchronized (mLock) {
           try {
               return mProxy.factorySetParamsDefault();
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySetParamsDefault:" + e);
           }
       }
       return -1;

   }

   public int FactorySSMRestore() {
         synchronized (mLock) {
           try {
               return mProxy.factorySSMRestore();
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySSMRestore:" + e);
           }
       }
       return -1;

   }

   public int FactoryResetNonlinear() {
         synchronized (mLock) {
           try {
               return mProxy.factoryResetNonlinear();
           } catch (RemoteException e) {
               Log.e(TAG, "FactoryResetNonlinear:" + e);
           }
       }
       return -1;
    }

   public int FactorySetGamma(int gamma_r, int gamma_g, int gamma_b) {
         synchronized (mLock) {
           try {
               return mProxy.factorySetGamma(gamma_r, gamma_g, gamma_b);
           } catch (RemoteException e) {
               Log.e(TAG, "FactorySetGamma:" + e);
           }
       }
       return -1;
    }

    public int SysSSMReadNTypes(int id, int data_len, int offset) {
         synchronized (mLock) {
           try {
               return mProxy.sysSSMReadNTypes(id, data_len, offset);
           } catch (RemoteException e) {
               Log.e(TAG, "SysSSMReadNTypes:" + e);
           }
       }
       return -1;

    }

    public int SysSSMWriteNTypes(int id, int data_len, int data_buf, int offset) {
          synchronized (mLock) {
            try {
                return mProxy.sysSSMWriteNTypes(id, data_len, data_buf, offset);
            } catch (RemoteException e) {
                Log.e(TAG, "SysSSMWriteNTypes:" + e);
            }
        }
        return -1;

    }

    public int GetActualAddr(int id) {
          synchronized (mLock) {
          Mutable<Integer> resultVal = new Mutable<>();
            try {
                return mProxy.getActualAddr(id);
            } catch (RemoteException e) {
                Log.e(TAG, "GetActualAddr:" + e);
            }
        }
        return -1;

    }

    public int GetActualSize(int id) {
          synchronized (mLock) {
            try {
                return mProxy.getActualSize(id);
            } catch (RemoteException e) {
                Log.e(TAG, "GetActualSize:" + e);
            }
        }
        return -1;

    }

    public int SSMRecovery() {
          synchronized (mLock) {
            try {
                return mProxy.SSMRecovery();
            } catch (RemoteException e) {
                Log.e(TAG, "SSMRecovery:" + e);
            }
        }
        return -1;

    }

    public int SetPLLValues(SourceInputParam srcInputParam) {
          synchronized (mLock) {
            try {
                return mProxy.setPLLValues(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "SetPLLValues:" + e);
            }
        }
        return -1;

    }

    public int SetCVD2Values(SourceInputParam srcInputParam) {
          synchronized (mLock) {
            try {
                return mProxy.setCVD2Values(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "SetCVD2Values:" + e);
            }
        }
        return -1;

    }

   public int setPQConfig(int id, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setPQConfig(id, value);
            } catch (RemoteException e) {
                Log.e(TAG, "setPQConfig:" + e);
            }
        }
        return -1;
    }

    public int GetSSMStatus() {
         synchronized (mLock) {
           try {
               return mProxy.getSSMStatus();
           } catch (RemoteException e) {
               Log.e(TAG, "GetSSMStatus:" + e);
           }
       }
       return -1;
    }

    public int ResetLastPQSettingsSourceType() {
          synchronized (mLock) {
            try {
                return mProxy.resetLastPQSettingsSourceType();
            } catch (RemoteException e) {
                Log.e(TAG, "ResetLastPQSettingsSourceType:" + e);
            }
        }
        return -1;
    }

    public int SetCurrentSourceInfo(SourceInputParam srcInputParam) {
          synchronized (mLock) {
            try {
                return mProxy.setCurrentSourceInfo(srcInputParam);
            } catch (RemoteException e) {
                Log.e(TAG, "SetCurrentSourceInfo:" + e);
            }
        }
        return -1;
    }

    public int[] GetCurrentSourceInfo() {
          int CurrentSourceInfo[] = {0, 0, 0, 0, 0, 0};
          synchronized (mLock) {
              Mutable<SourceInputParam> srcInputParam = new Mutable<>();
              try {
                  mProxy.getCurrentSourceInfo((int ret, SourceInputParam tmpSrcInputParam)-> {
                                                 if (Result.OK == ret) {
                                                     srcInputParam.value = tmpSrcInputParam;
                                                 }
                                             });
                CurrentSourceInfo[0] = srcInputParam.value.sourceInput;
                CurrentSourceInfo[1] = srcInputParam.value.sourceType;
                CurrentSourceInfo[2] = srcInputParam.value.sourcePort;
                CurrentSourceInfo[3] = srcInputParam.value.sigFmt;
                CurrentSourceInfo[4] = srcInputParam.value.transFmt;
                CurrentSourceInfo[5] = srcInputParam.value.is3d;
                return CurrentSourceInfo;
            } catch (RemoteException e) {
                Log.e(TAG, "GetCurrentSourceInfo:" + e);
            }
        }
        return CurrentSourceInfo;
    }
    public int GetAutoSwitchPCModeFlag() {
          synchronized (mLock) {
            try {
                return mProxy.getAutoSwitchPCModeFlag();
            } catch (RemoteException e) {
                Log.e(TAG, "GetAutoSwitchPCModeFlag:" + e);
            }
        }
        return -1;
    }

        /**
     * @Function: Read the red gain with specified souce and color temperature
     * @Param:
     * @ Return value: the red gain value
     * */
    public int FactoryWhiteBalanceSetRedGain(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceGainRed(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetRedGain:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetGreenGain(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceGainGreen(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetGreenGain:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetBlueGain(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceGainBlue(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetBlueGain:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceGetRedGain(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceGainRed(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetBlueGain:" + e);
            }
        }
        return -1;

    }

    public int FactoryWhiteBalanceGetGreenGain(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceGainGreen(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetRedGain:" + e);
            }
        }
        return -1;

    }

    public int FactoryWhiteBalanceGetBlueGain(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceGainBlue(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetBlueGain:" + e);
            }
        }
        return -1;

    }

    public int FactoryWhiteBalanceSetRedOffset(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceOffsetRed(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetRedOffset:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetGreenOffset(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceOffsetGreen(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetGreenOffset:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetBlueOffset(int sourceType, int colorTemp_mode, int value) {
          synchronized (mLock) {
            try {
                return mProxy.setwhiteBalanceOffsetBlue(sourceType, colorTemp_mode, value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetBlueOffset:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceGetRedOffset(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceOffsetRed(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetBlueOffset:" + e);
            }
        }
        return -1;

    }

    public int FactoryWhiteBalanceGetGreenOffset(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceOffsetGreen(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetGreenOffset:" + e);
            }
        }
        return -1;

    }

    public int FactoryWhiteBalanceGetBlueOffset(int sourceType, int colorTemp_mode) {
          synchronized (mLock) {
            try {
                return mProxy.getwhiteBalanceOffsetBlue(sourceType, colorTemp_mode);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetBlueOffset:" + e);
            }
        }
        return -1;

    }

        /**
     * @Function: Save the white balance data to fbc or g9
     * @Param:
     * @Return value: save OK: 0 , else -1
     *
     * */
    public int FactoryWhiteBalanceSaveParameters(int sourceType, int colorTemp_mode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset) {
          synchronized (mLock) {
            try {
                return mProxy.saveWhiteBalancePara(sourceType, colorTemp_mode, r_gain, g_gain, b_gain, r_offset, g_offset, b_offset);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSaveParameters:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryGetRGBScreen
     * @Description: get rgb screen pattern
     * @Return: rgb(0xrrggbb)
     */
    public int FactoryGetRGBScreen() {
          synchronized (mLock) {
            try {
                return mProxy.getRGBPattern();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetRGBScreen:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactorySetRGBScreen
     * @Description: set test pattern with rgb.
     * @Param r,g,b int 0~255
     * @Return: -1 failed, otherwise success
     */
    public int FactorySetRGBScreen(int r, int g, int b) {
          synchronized (mLock) {
            try {
                return mProxy.setRGBPattern(r, g, b);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetRGBScreen:" + e);
            }
        }
        return -1;

    }

    /**
     * @Function: FactorySetDDRSSC
     * @Description: Set ddr ssc level for factory menu conctrol
     * @Param: step ddr ssc level
     * @Return: 0 success, -1 fail
     */
    public int FactorySetDDRSSC(int step) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetDDRSSC(step);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetDDRSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryGetDDRSSC
     * @Description: Get ddr ssc level for factory menu conctrol
     * @Param:
     * @Return: ddr ssc level
     */
    public int FactoryGetDDRSSC() {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetDDRSSC();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetDDRSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactorySetLVDSSSC
     * @Description: Set lvds ssc level for factory menu conctrol
     * @Param: step lvds ssc level
     * @Return: 0 success, -1 fail
     */
    public int FactorySetLVDSSSC(int step) {
          synchronized (mLock) {
            try {
                return mProxy.factorySetLVDSSSC(step);
            } catch (RemoteException e) {
                Log.e(TAG, "FactorySetLVDSSSC:" + e);
            }
        }
        return -1;
    }

    /**
     * @Function: FactoryGetLVDSSSC
     * @Description: Get lvds ssc level for factory menu conctrol
     * @Param:
     * @Return: lvds ssc level
     */
    public int FactoryGetLVDSSSC() {
          synchronized (mLock) {
            try {
                return mProxy.factoryGetLVDSSSC();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryGetLVDSSSC:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceOpenGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternOpen();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceOpenGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceCloseGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternClose();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceCloseGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceSetGrayPattern(int value) {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternSet(value);
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceSetGrayPattern:" + e);
            }
        }
        return -1;
    }

    public int FactoryWhiteBalanceGetGrayPattern() {
          synchronized (mLock) {
            try {
                return mProxy.whiteBalanceGrayPatternGet();
            } catch (RemoteException e) {
                Log.e(TAG, "FactoryWhiteBalanceGetGrayPattern:" + e);
            }
        }
        return -1;
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
