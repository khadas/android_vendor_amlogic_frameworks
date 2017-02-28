package com.droidlogic.app.tv;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;
import java.util.Comparator;

import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicHdmiCecManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TVInSignalInfo;

import com.droidlogic.app.SystemControlManager;

import android.provider.Settings;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.media.tv.TvInputManager.HardwareCallback;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.media.tv.TvContract.Channels;
import android.app.ActivityManager;
import android.view.Surface;
import android.net.Uri;
import com.android.internal.os.SomeArgs;
import android.os.Handler;
import android.os.Message;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiTvClient;
import android.provider.Settings.Global;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.hdmi.HdmiTvClient.SelectCallback;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParserException;

import android.provider.Settings;

public class DroidLogicTvInputService extends TvInputService implements
        TVInSignalInfo.SigInfoChangeListener, TvControlManager.StorDBEventListener,
        TvControlManager.ScanningFrameStableListener {
    private static final String TAG = DroidLogicTvInputService.class.getSimpleName();
    private static final boolean DEBUG = true;

    private SparseArray<TvInputInfo> mInfoList = new SparseArray<>();

    private TvInputBaseSession mSession;
    private String mCurrentInputId;
    public Hardware mHardware;
    public TvStreamConfig[] mConfigs;
    private int mDeviceId = -1;
    private int mSourceType = DroidLogicTvUtils.SOURCE_TYPE_OTHER;
    private String mChildClassName;
    private SurfaceHandler mSessionHandler;
    private static final int MSG_DO_TUNE = 0;
    private static final int MSG_DO_SET_SURFACE = 3;
    private static final int RETUNE_TIMEOUT = 20; // 1 second
    private static int mSelectPort = -1;
    private int timeout = RETUNE_TIMEOUT;
    private Surface mSurface;
    protected int ACTION_FAILED = -1;
    protected int ACTION_SUCCESS = 1;
    private Context mContext;
    private int mCurrentSessionId = 0;

    private TvInputManager mTvInputManager;
    private TvControlManager mTvControlManager;

    private TvStoreManager mTvStoreManager;

    private HardwareCallback mHardwareCallback = new HardwareCallback(){
        @Override
        public void onReleased() {
            if (DEBUG)
                Log.d(TAG, "onReleased");

            mHardware = null;
        }

        @Override
        public void onStreamConfigChanged(TvStreamConfig[] configs) {
            if (DEBUG)
                Log.d(TAG, "onStreamConfigChanged");
            mConfigs = configs;
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mTvInputManager = (TvInputManager)this.getSystemService(Context.TV_INPUT_SERVICE);
    }

    /**
     * inputId should get from subclass which must invoke {@link super#onCreateSession(String)}
     */
    @Override
    public Session onCreateSession(String inputId) {
        mContext = getApplicationContext();
        mCurrentInputId = inputId;
        mSessionHandler = new SurfaceHandler();
        return null;
    }

    protected void initInputService(int sourceType, String className) {
        mSourceType = sourceType;
        mChildClassName = className;
    }

    protected void acquireHardware(TvInputInfo info){
        mDeviceId = getHardwareDeviceId(info.getId());
        mHardware = mTvInputManager.acquireTvInputHardware(mDeviceId, mHardwareCallback,info);
        Log.d(TAG, "acquireHardware , mHardware: " + mHardware);
    }

    protected void releaseHardware(){
        Log.d(TAG, "releaseHardware , mHardware: " + mHardware);
        if (mHardware != null && mDeviceId != -1) {
            mConfigs = null;
            mHardware = null;
        }

        //if hdmi signal is unstable from stable, disconnect cec.
        if (mDeviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1
                && mDeviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI3) {
            DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
            if (hdmi_cec.getInputSourceType() == mDeviceId)
                selectHdmiDevice(0);
        }
    }

    /**
     * get session has been created by {@code onCreateSession}, and input id of session.
     * @param session {@link HdmiInputSession} or {@link AVInputSession}
     */
    protected void registerInputSession(TvInputBaseSession session) {
        Log.d(TAG, "registerInputSession");
        mSession = session;
        mCurrentSessionId = session.mId;
        Log.d(TAG, "inputId["+mCurrentInputId+"]");

        SystemControlManager mSystemControlManager = new SystemControlManager(mContext);
        int channel_number_start = mSystemControlManager.getPropertyInt("tv.channel.number.start", 1);
        mTvStoreManager = new TvStoreManager(this, mCurrentInputId, channel_number_start) {
                @Override
                public void onEvent(String eventType, Bundle eventArgs) {
                    mSession.notifySessionEvent(eventType, eventArgs);
                }
                @Override
                public void onUpdateCurrent(ChannelInfo channel, boolean store) {
                    onUpdateCurrentChannel(channel, store);
                }
                @Override
                public void onDtvNumberMode(String mode) {
                    Settings.System.putString(DroidLogicTvInputService.this.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_NUMBER_MODE, "lcn");
                }
                public void onScanEnd() {
                    mTvControlManager.DtvStopScan();
                }
            };
        mTvControlManager = TvControlManager.getInstance();
        mTvControlManager.SetSigInfoChangeListener(this);
        mTvControlManager.setScanningFrameStableListener(this);
        resetScanStoreListener();
    }

    /**
     * update {@code mInfoList} when hardware device is added or removed.
     * @param hInfo {@linkHardwareInfo} get from HAL.
     * @param info {@link TvInputInfo} will be added or removed.
     * @param isRemoved {@code true} if you want to remove info. {@code false} otherwise.
     */
    protected void updateInfoListIfNeededLocked(TvInputHardwareInfo hInfo,
            TvInputInfo info, boolean isRemoved) {
        updateInfoListIfNeededLocked(hInfo.getDeviceId(), info, isRemoved);
    }

    protected void updateInfoListIfNeededLocked(int Id, TvInputInfo info,
            boolean isRemoved) {
        if (isRemoved) {
            mInfoList.remove(Id);
        } else {
            mInfoList.put(Id, info);
        }

        if (DEBUG)
            Log.d(TAG, "size of mInfoList is " + mInfoList.size());
    }

    protected boolean hasInfoExisted(TvInputHardwareInfo hInfo) {
        return mInfoList.get(hInfo.getDeviceId()) == null ? false : true;
    }

    protected TvInputInfo getTvInputInfo(TvInputHardwareInfo hardwareInfo) {
        return mInfoList.get(hardwareInfo.getDeviceId());
    }

    protected TvInputInfo getTvInputInfo(int devId) {
        return mInfoList.get(devId);
    }

    protected int getHardwareDeviceId(String input_id) {
        int id = 0;
        for (int i = 0; i < mInfoList.size(); i++) {
            if (input_id.equals(mInfoList.valueAt(i).getId())) {
                id = mInfoList.keyAt(i);
                break;
            }
        }

        if (DEBUG)
            Log.d(TAG, "device id is " + id);
        return id;
    }

    protected String getTvInputInfoLabel(int device_id) {
        String label = null;
        switch (device_id) {
        case DroidLogicTvUtils.DEVICE_ID_ATV:
            label = ChannelInfo.LABEL_ATV;
            break;
        case DroidLogicTvUtils.DEVICE_ID_DTV:
            label = ChannelInfo.LABEL_DTV;
            break;
        case DroidLogicTvUtils.DEVICE_ID_AV1:
            label = ChannelInfo.LABEL_AV1;
            break;
        case DroidLogicTvUtils.DEVICE_ID_AV2:
            label = ChannelInfo.LABEL_AV2;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI1:
            label = ChannelInfo.LABEL_HDMI1;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI2:
            label = ChannelInfo.LABEL_HDMI2;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI3:
            label = ChannelInfo.LABEL_HDMI3;
            break;
        case DroidLogicTvUtils.DEVICE_ID_SPDIF:
            label = ChannelInfo.LABEL_SPDIF;
            break;
        default:
            break;
        }
        return label;
    }

    protected ResolveInfo getResolveInfo(String cls_name) {
        if (TextUtils.isEmpty(cls_name))
            return null;
        ResolveInfo ret_ri = null;
        PackageManager pm = getApplicationContext().getPackageManager();
        List<ResolveInfo> services = pm.queryIntentServices(
                new Intent(TvInputService.SERVICE_INTERFACE),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        for (ResolveInfo ri : services) {
            ServiceInfo si = ri.serviceInfo;
            if (!android.Manifest.permission.BIND_TV_INPUT.equals(si.permission)) {
                continue;
            }

            if (DEBUG)
                Log.d(TAG, "cls_name = " + cls_name + ", si.name = " + si.name);

            if (cls_name.equals(si.name)) {
                ret_ri = ri;
                break;
            }
        }
        return ret_ri;
    }

    protected void stopTv() {
        Log.d(TAG, "stop tv, mCurrentInputId =" + mCurrentInputId);
        mTvControlManager.StopTv();
    }

    protected void releasePlayer() {
        mTvControlManager.StopPlayProgram();
    }

    private String getInfoLabel() {
        return mTvInputManager.getTvInputInfo(mCurrentInputId).loadLabel(this).toString();
    }

    @Override
    public void onSigChange(TVInSignalInfo signal_info) {
        TVInSignalInfo.SignalStatus status = signal_info.sigStatus;

        if (DEBUG)
            Log.d(TAG, "onSigChange" + status.ordinal() + status.toString());

        if (status == TVInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NOSIG
                || status == TVInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NULL
                || status == TVInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NOTSUP) {
            mSession.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
        } else if (status == TVInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
            int device_id = mSession.getDeviceId();
            if ((device_id != DroidLogicTvUtils.DEVICE_ID_DTV)
                || (signal_info.reserved == 1))
                mSession.notifyVideoAvailable();

            String[] strings;
            Bundle bundle = new Bundle();
            switch (device_id) {
            case DroidLogicTvUtils.DEVICE_ID_HDMI1:
            case DroidLogicTvUtils.DEVICE_ID_HDMI2:
            case DroidLogicTvUtils.DEVICE_ID_HDMI3:
                if (DEBUG)
                    Log.d(TAG, "signal_info.fmt.toString() for hdmi=" + signal_info.sigFmt.toString());

                strings = signal_info.sigFmt.toString().split("_");
                TVInSignalInfo.SignalFmt fmt = signal_info.sigFmt;
                if (fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_60HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_120HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_240HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_2880X480I_60HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_2880X480I_60HZ) {
                    strings[4] = "480I";
                } else if (fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_50HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_100HZ
                        || fmt == TVInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_200HZ) {
                    strings[4] = "576I";
                }

                bundle.putInt(DroidLogicTvUtils.SIG_INFO_TYPE, DroidLogicTvUtils.SIG_INFO_TYPE_HDMI);
                bundle.putString(DroidLogicTvUtils.SIG_INFO_LABEL, getInfoLabel());
                if (strings != null && strings.length <= 4)
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, " ");
                else if (mTvControlManager.IsDviSignal()) {
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, "DVI "+strings[4]
                            + "_" + signal_info.reserved + "HZ");
                }else
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, strings[4]
                            + "_" + signal_info.reserved + "HZ");

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, bundle);
                break;
            case DroidLogicTvUtils.DEVICE_ID_AV1:
            case DroidLogicTvUtils.DEVICE_ID_AV2:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for av=" + signal_info.sigFmt.toString());

                strings = signal_info.sigFmt.toString().split("_");
                bundle.putInt(DroidLogicTvUtils.SIG_INFO_TYPE, DroidLogicTvUtils.SIG_INFO_TYPE_AV);
                bundle.putString(DroidLogicTvUtils.SIG_INFO_LABEL, getInfoLabel());
                if (strings != null && strings.length <= 4)
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, "");
                else
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, strings[4]);
                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, bundle);
                break;
            case DroidLogicTvUtils.DEVICE_ID_ATV:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for atv=" + signal_info.sigFmt.toString());

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, null);
                break;
            case DroidLogicTvUtils.DEVICE_ID_DTV:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for dtv=" + signal_info.sigFmt.toString());

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, null);
                break;
            default:
                break;
            }
        }
    }

    @Override
    public void StorDBonEvent(TvControlManager.ScannerEvent event) {
        mTvStoreManager.onStoreEvent(event);
    }

    public void resetScanStoreListener() {
        mTvControlManager.setStorDBListener(this);
    }

    @Override
    public void onFrameStable(TvControlManager.ScanningFrameStableEvent event) {
        Log.d(TAG, "scanning frame stable!");
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FREQ, event.CurScanningFrq);
        mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_C_SCANNING_FRAME_STABLE_EVENT, bundle);
    }

    public void onUpdateCurrentChannel(ChannelInfo channel, boolean store) {}

    protected  boolean setSurfaceInService(Surface surface, TvInputBaseSession session ) {
        Log.d(TAG, "SetSurface");
        Message message = mSessionHandler.obtainMessage();
        message.what = MSG_DO_SET_SURFACE;

        SomeArgs args = SomeArgs.obtain();
        args.arg1 = surface;
        args.arg2 = session;

        message.obj = args;
        mSessionHandler.sendMessage(message);
        return false;
    }

    protected  boolean doTuneInService(Uri channelUri, int sessionId) {
        if (DEBUG)
            Log.d(TAG, "onTune, channelUri=" + channelUri);

        mSessionHandler.obtainMessage(MSG_DO_TUNE, sessionId, 0, channelUri).sendToTarget();
        return false;
    }

    private final class SurfaceHandler extends Handler {
        @Override
        public void handleMessage(Message message) {
        if (DEBUG)
            Log.d(TAG, "handleMessage, msg.what=" + message.what);
        switch (message.what) {
            case MSG_DO_TUNE:
                mSessionHandler.removeMessages(MSG_DO_TUNE);
                doTune((Uri)message.obj, message.arg1);
                break;
            case MSG_DO_SET_SURFACE:
                SomeArgs args = (SomeArgs) message.obj;
                doSetSurface((Surface)args.arg1, (TvInputBaseSession)args.arg2);
                break;
            }
        }
    }
    private void doSetSurface(Surface surface, TvInputBaseSession session) {
        Log.d(TAG, "doSetSurface inputId=" + mCurrentInputId + " number=" + session.mId + " surface=" + surface);
        timeout = RETUNE_TIMEOUT;

        if (surface != null && !surface.isValid()) {
            Log.d(TAG, "onSetSurface get invalid surface");
            return;
        } else if (surface != null) {
            if (mHardware != null && mSurface != null
                && (mSourceType >= DroidLogicTvUtils.DEVICE_ID_HDMI1)
                && (mSourceType <= DroidLogicTvUtils.DEVICE_ID_HDMI3)) {
                stopTvPlay(mSession.mId);
            }
            registerInputSession(session);
            setCurrentSessionById(mSession.mId);
            mSurface = surface;
        }

        if (surface == null && mHardware != null && session.mId == mSession.mId) {
            Log.d(TAG, "surface is null, so stop TV play");
            mSurface = null;
            stopTvPlay(session.mId);
        }
    }

    public int doTune(Uri uri, int sessionId) {
        Log.d(TAG, "doTune, uri = " + uri);
        if (mConfigs == null || startTvPlay() == ACTION_FAILED) {
            Log.d(TAG, "doTune failed, timeout=" + timeout + ", retune 50ms later ...");

            if (timeout > 0) {
                Message msg = mSessionHandler.obtainMessage(MSG_DO_TUNE, uri);
                mSessionHandler.sendMessageDelayed(msg, 50);
                timeout--;
                return ACTION_FAILED;
            }
        }
        doTuneFinish(ACTION_SUCCESS, uri, sessionId);
        return ACTION_SUCCESS;
    }

    private int startTvPlay() {
        Log.d(TAG, "startTvPlay inputId=" + mCurrentInputId + " surface=" + mSurface);
        if (mHardware != null && mSurface != null && mSurface.isValid()) {
            mHardware.setSurface(mSurface, mConfigs[0]);
            selectHdmiDevice(mDeviceId);
            return ACTION_SUCCESS;
        }
        return ACTION_FAILED;
    }

    private int stopTvPlay(int sessionId) {
        if (mHardware != null) {
            mHardware.setSurface(null, mConfigs[0]);
            tvPlayStopped(sessionId);
        }
        disconnectHdmiCec(mDeviceId);
        return ACTION_SUCCESS;
    }
    public void setCurrentSessionById(int sessionId){}
    public void doTuneFinish(int result, Uri uri, int sessionId){};
    public void tvPlayStopped(int sessionId){};

    protected int getCurrentSessionId() {
        return mCurrentSessionId;
    }

    private boolean isInTvApp() {
        ActivityManager am = (ActivityManager) this.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningTaskInfo> runTaskInfos = am.getRunningTasks(1);
        if (runTaskInfos == null)
            return false;
        ActivityManager.RunningTaskInfo runningTaskInfo = runTaskInfos.get(0);
        return runningTaskInfo.topActivity.getPackageName().equals("com.droidlogic.tvsource");
    }

    /**
     * select hdmi cec device.
     * @param port the hardware device id of hdmi need to be selected.
     */
    public void selectHdmiDevice(final int port) {
        if (!isInTvApp())
            return;
        DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
        hdmi_cec.selectHdmiDevice(port);
    }

    /**
     * reset the status for hdmi cec device.
     * @param port the hardware device id of hdmi need to be selected.
     */
    public void disconnectHdmiCec(int port) {
        DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
        hdmi_cec.disconnectHdmiCec(port);
    }

    private int getHdmiPortIndex(int phyAddr) {
        /* TODO: consider of tuner */
        return ((phyAddr & 0xf000) >> 12) - 1;
    }

    @Override
    public TvInputInfo onHdmiDeviceAdded(HdmiDeviceInfo deviceInfo) {
        if (deviceInfo == null) {
            return null;
        }
        int phyaddr = deviceInfo.getPhysicalAddress();
        int sourceType = getHdmiPortIndex(phyaddr) + DroidLogicTvUtils.DEVICE_ID_HDMI1;

        if (sourceType < DroidLogicTvUtils.DEVICE_ID_HDMI1
                || sourceType > DroidLogicTvUtils.DEVICE_ID_HDMI3
                || sourceType != mSourceType)
            return null;
        Log.d(TAG, "onHdmiDeviceAdded, sourceType = " + sourceType + ", mSourceType = " + mSourceType);

        if (getTvInputInfo(phyaddr) != null) {
            Log.d(TAG, "onHdmiDeviceAdded, phyaddr:" + phyaddr + " already add");
            return null;
        }

        String parentId = null;
        TvInputInfo info = null;
        TvInputInfo parent = getTvInputInfo(sourceType);
        if (parent != null) {
            parentId = parent.getId();
        } else {
            Log.d(TAG, "onHdmiDeviceAdded, can't found parent");
            return null;
        }
        Log.d(TAG, "onHdmiDeviceAdded, phyaddr:" + phyaddr +
                    ", port:" + sourceType + ", parentID:" + parentId);
        ResolveInfo rInfo = getResolveInfo(mChildClassName);
        if (rInfo != null) {
            try {
                info = TvInputInfo.createTvInputInfo(
                        getApplicationContext(),
                        rInfo,
                        deviceInfo,
                        parentId,
                        deviceInfo.getDisplayName(),
                        null);
            } catch (XmlPullParserException e) {
                // TODO: handle exception
            }catch (IOException e) {
                // TODO: handle exception
            }
        } else {
            return null;
        }
        Log.d(TAG, "createTvInputInfo, id:" + info.getId());
        updateInfoListIfNeededLocked(phyaddr, info, false);
        selectHdmiDevice(sourceType);

        return info;
    }

    @Override
    public String onHdmiDeviceRemoved(HdmiDeviceInfo deviceInfo) {
        if (deviceInfo == null)
            return null;
        int phyaddr = deviceInfo.getPhysicalAddress();
        int sourceType = getHdmiPortIndex(phyaddr) + DroidLogicTvUtils.DEVICE_ID_HDMI1;

        if (sourceType < DroidLogicTvUtils.DEVICE_ID_HDMI1
                || sourceType > DroidLogicTvUtils.DEVICE_ID_HDMI3
                || sourceType != mSourceType)
            return null;

        Log.d(TAG, "onHdmiDeviceRemoved, sourceType = " + sourceType + ", mSourceType = " + mSourceType);
        TvInputInfo info = getTvInputInfo(phyaddr);
        if (info == null)
            return null;

        String id = info.getId();
        Log.d(TAG, "onHdmiDeviceRemoved, id:" + id);
        updateInfoListIfNeededLocked(phyaddr, info, true);
        disconnectHdmiCec(sourceType);

        return id;
    }
}
