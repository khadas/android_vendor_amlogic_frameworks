/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.droidlogic;

import android.Manifest;
import android.annotation.Nullable;
import android.app.usage.StorageStatsManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.UserInfo;
import android.content.res.Configuration;
import android.content.res.ObbInfo;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.Binder;
import android.os.Environment;
import android.os.Environment.UserEnvironment;
import android.os.FileUtils;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.storage.DiskInfo;
import android.os.storage.IStorageShutdownObserver;
import android.os.storage.StorageResultCode;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.VolumeInfo;
import android.os.storage.StorageManager;
import android.os.storage.VolumeRecord;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.ArrayMap;
import android.util.AtomicFile;
import android.util.Log;
import android.util.Pair;
import android.util.Slog;
import android.util.SparseArray;
import android.util.TimeUtils;

import com.android.internal.annotations.GuardedBy;
import com.android.internal.util.DumpUtils;
import com.android.internal.util.IndentingPrintWriter;

import libcore.io.IoUtils;
import libcore.util.EmptyArray;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;

import vendor.amlogic.hardware.droidvold.V1_0.IDroidVold;
import vendor.amlogic.hardware.droidvold.V1_0.IDroidVoldCallback;
import vendor.amlogic.hardware.droidvold.V1_0.Result;

import android.hidl.manager.V1_0.IServiceManager;
import android.hidl.manager.V1_0.IServiceNotification;
import android.os.Bundle;
import android.os.HwBinder;
import android.os.Parcel;
import android.os.Parcelable;
import java.util.NoSuchElementException;

import com.droidlogic.app.IDroidVoldManager;

import com.droidlogic.app.IDroidVoldManager;

/**
 * Service responsible for various storage media. Connects to {@code droidvold} to
 * watch for and manage dynamically added storage, such as SD cards and USB mass storage.
 */
class DroidVoldManager extends IDroidVoldManager.Stub {
    // Static direct instance pointer for the tightly-coupled idle service to use
    static DroidVoldManager sSelf = null;

    private static final boolean DEBUG = false;

    private static final String TAG = "DroidVoldManager";

    private static ArrayMap<String, String> sEnvironmentToBroadcast = new ArrayMap<>();

    public static final String ACTION_MEDIA_UNMOUNTED = "com.droidvold.action.MEDIA_UNMOUNTED";
    public static final String ACTION_MEDIA_CHECKING = "com.droidvold.action.MEDIA_CHECKING";
    public static final String ACTION_MEDIA_MOUNTED = "com.droidvold.action.MEDIA_MOUNTED";
    public static final String ACTION_MEDIA_UNMOUNTABLE = "com.droidvold.action.MEDIA_UNMOUNTABLE";
    public static final String ACTION_MEDIA_EJECT = "com.droidvold.action.MEDIA_EJECT";
    public static final String ACTION_MEDIA_BAD_REMOVAL = "com.droidvold.action.MEDIA_BAD_REMOVAL";
    public static final String ACTION_MEDIA_REMOVED = "com.droidvold.action.MEDIA_REMOVED";

    static {
        sEnvironmentToBroadcast.put(Environment.MEDIA_UNMOUNTED, ACTION_MEDIA_UNMOUNTED);
        sEnvironmentToBroadcast.put(Environment.MEDIA_CHECKING, ACTION_MEDIA_CHECKING);
        sEnvironmentToBroadcast.put(Environment.MEDIA_MOUNTED, ACTION_MEDIA_MOUNTED);
        sEnvironmentToBroadcast.put(Environment.MEDIA_MOUNTED_READ_ONLY, ACTION_MEDIA_MOUNTED);
        sEnvironmentToBroadcast.put(Environment.MEDIA_EJECTING, ACTION_MEDIA_EJECT);
        sEnvironmentToBroadcast.put(Environment.MEDIA_UNMOUNTABLE, ACTION_MEDIA_UNMOUNTABLE);
        sEnvironmentToBroadcast.put(Environment.MEDIA_REMOVED, ACTION_MEDIA_REMOVED);
        sEnvironmentToBroadcast.put(Environment.MEDIA_BAD_REMOVAL, ACTION_MEDIA_BAD_REMOVAL);
     }

    /*
     * Internal droidvold response code constants
     */
    class VoldResponseCode {
        /*
         * 100 series - Requestion action was initiated; expect another reply
         *              before proceeding with a new command.
         */
        public static final int VolumeListResult               = 110;

        /*
         * 200 series - Requestion action has been successfully completed.
         */
        public static final int ShareStatusResult              = 210;

        /*
         * 400 series - Command was accepted, but the requested action
         *              did not take place.
         */
        public static final int OpFailedNoMedia                = 401;
        public static final int OpFailedMediaBlank             = 402;
        public static final int OpFailedMediaCorrupt           = 403;
        public static final int OpFailedVolNotMounted          = 404;
        public static final int OpFailedStorageBusy            = 405;
        public static final int OpFailedStorageNotFound        = 406;

        /*
         * 600 series - Unsolicited broadcasts.
         */
        public static final int DISK_CREATED = 640;
        public static final int DISK_SIZE_CHANGED = 641;
        public static final int DISK_LABEL_CHANGED = 642;
        public static final int DISK_SCANNED = 643;
        public static final int DISK_SYS_PATH_CHANGED = 644;
        public static final int DISK_DESTROYED = 649;

        public static final int VOLUME_CREATED = 650;
        public static final int VOLUME_STATE_CHANGED = 651;
        public static final int VOLUME_FS_TYPE_CHANGED = 652;
        public static final int VOLUME_FS_UUID_CHANGED = 653;
        public static final int VOLUME_FS_LABEL_CHANGED = 654;
        public static final int VOLUME_PATH_CHANGED = 655;
        public static final int VOLUME_INTERNAL_PATH_CHANGED = 656;
        public static final int VOLUME_DESTROYED = 659;
    }


    /**
     * <em>Never</em> hold the lock while performing downcalls into vold, since
     * unsolicited events can suddenly appear to update data structures.
     */
    private final Object mLock = new Object();

    /** Map from disk ID to disk */
    @GuardedBy("mLock")
    private ArrayMap<String, DiskInfo> mDisks = new ArrayMap<>();
    /** Map from volume ID to disk */
    @GuardedBy("mLock")
    private final ArrayMap<String, VolumeInfo> mVolumes = new ArrayMap<>();

    /** Map from UUID to record */
    @GuardedBy("mLock")
    private ArrayMap<String, VolumeRecord> mRecords = new ArrayMap<>();

    private volatile int mCurrentUserId = UserHandle.USER_SYSTEM;

    private final Context mContext;

    private IDroidVold mDroidVold;

    private DroidVoldCallback mDroidVoldCallback;

    public static @Nullable String getBroadcastForEnvironment(String envState) {
        return sEnvironmentToBroadcast.get(envState);
    }

    // Handler messages
    private static final int H_SHUTDOWN = 1;
    private static final int H_VOLUME_MOUNT = 2;
    private static final int H_VOLUME_BROADCAST = 3;
    private static final int H_INTERNAL_BROADCAST = 4;
    private static final int H_PARTITION_FORGET = 5;
    private static final int H_RESET = 6;

    class DroidVoldManagerHandler extends Handler {
        public DroidVoldManagerHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case H_SHUTDOWN: {
                    final IStorageShutdownObserver obs = (IStorageShutdownObserver) msg.obj;
                    boolean success = false;
                    try {
                        int rs = mDroidVold.shutdown();
                        success = (rs == 0 ? true : false);
                    } catch (NoSuchElementException e) {
                        Slog.e(TAG, "connectToProxy: droidvold hal service not found."
                                + " Did the service fail to start?", e);
                    } catch (RemoteException e) {
                        Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
                    }

                    if (obs != null) {
                        try {
                            obs.onShutDownComplete(success ? 0 : -1);
                        } catch (RemoteException ignored) {
                        }
                    }
                    break;
                }
                case H_VOLUME_MOUNT: {
                    final VolumeInfo vol = (VolumeInfo) msg.obj;
                    if (isMountDisallowed(vol)) {
                        Slog.i(TAG, "Ignoring mount " + vol.getId() + " due to policy");
                        break;
                    }
                    try {
                        mDroidVold.mount(vol.id, vol.mountFlags, vol.mountUserId);
                    } catch (RemoteException e) {
                        Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
                    }
                    break;
                }
                case H_VOLUME_BROADCAST: {
                    final StorageVolume userVol = (StorageVolume) msg.obj;
                    final String envState = userVol.getState();
                    Slog.d(TAG, "Volume " + userVol.getId() + " broadcasting " + envState + " to "
                            + userVol.getOwner());

                    final String action = getBroadcastForEnvironment(envState);

                    StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
                    StrictMode.setVmPolicy(builder.build());
                    //builder.detectFileUriExposure();

                    if (action != null) {
                        final Intent intent = new Intent(action,
                                Uri.fromFile(userVol.getPathFile()));
                        intent.putExtra(StorageVolume.EXTRA_STORAGE_VOLUME, userVol);
                        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                                | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);

                        mContext.sendBroadcastAsUser(intent, userVol.getOwner());
                    }
                    break;
                }
                case H_INTERNAL_BROADCAST: {
                    // Internal broadcasts aimed at system components, not for
                    // third-party apps.
                    final Intent intent = (Intent) msg.obj;
                    mContext.sendBroadcastAsUser(intent, UserHandle.ALL,
                            android.Manifest.permission.WRITE_MEDIA_STORAGE);
                    break;
                }
                case H_RESET: {
                    resetIfReadyAndConnected();
                    break;
                }
            }
        }
    }

    private final Handler mHandler;

    private void resetIfReadyAndConnected() {
        final List<UserInfo> users = mContext.getSystemService(UserManager.class).getUsers();

        synchronized (mLock) {
            mDisks.clear();
            mVolumes.clear();
        }

        try {
            mDroidVold.reset();
        } catch (RemoteException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
        }
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            Slog.d(TAG, "droidvold server died");
        }
    }

    static class DroidVoldCallback extends IDroidVoldCallback.Stub {
        public DroidVoldManager mDroidVoldManager;
        // implement methods
        DroidVoldCallback() {
            super();
        }

        DroidVoldCallback(DroidVoldManager dm) {
            this.mDroidVoldManager = dm;
        }

        public static String[] unescapeArgs(String rawEvent) {
            final boolean DEBUG_ROUTINE = false;
            final String LOGTAG = "unescapeArgs";
            final ArrayList<String> parsed = new ArrayList<String>();
            final int length = rawEvent.length();
            int current = 0;
            int wordEnd = -1;
            boolean quoted = false;

            if (DEBUG_ROUTINE) Slog.e(LOGTAG, "parsing '" + rawEvent + "'");
            if (rawEvent.charAt(current) == '\"') {
                quoted = true;
                current++;
            }
            while (current < length) {
                // find the end of the word
                char terminator = quoted ? '\"' : ' ';
                wordEnd = current;
                while (wordEnd < length && rawEvent.charAt(wordEnd) != terminator) {
                    if (rawEvent.charAt(wordEnd) == '\\') {
                        // skip the escaped char
                        ++wordEnd;
                    }
                    ++wordEnd;
                }
                if (wordEnd > length) wordEnd = length;
                String word = rawEvent.substring(current, wordEnd);
                current += word.length();
                if (!quoted) {
                    word = word.trim();
                } else {
                    current++;  // skip the trailing quote
                }
                // unescape stuff within the word
                word = word.replace("\\\\", "\\");
                word = word.replace("\\\"", "\"");

                if (DEBUG_ROUTINE) Slog.e(LOGTAG, "found '" + word + "'");
                parsed.add(word);

                // find the beginning of the next word - either of these options
                int nextSpace = rawEvent.indexOf(' ', current);
                int nextQuote = rawEvent.indexOf(" \"", current);
                if (DEBUG_ROUTINE) {
                    Slog.e(LOGTAG, "nextSpace=" + nextSpace + ", nextQuote=" + nextQuote);
                }
                if (nextQuote > -1 && nextQuote <= nextSpace) {
                    quoted = true;
                    current = nextQuote + 2;
                } else {
                    quoted = false;
                    if (nextSpace > -1) {
                        current = nextSpace + 1;
                    }
                } // else we just start the next word after the current and read til the end
                if (DEBUG_ROUTINE) {
                    Slog.e(LOGTAG, "next loop - current=" + current +
                            ", length=" + length + ", quoted=" + quoted);
                }
            }
            return parsed.toArray(new String[parsed.size()]);
        }

       // @Override
        public void onEvent(int code, String message)
            throws android.os.RemoteException {
            if (DEBUG)
                Slog.d(TAG, "onEvent code=" + code + " message=" + message);

            String[] cooked = unescapeArgs(message);
            mDroidVoldManager.onEvent(code, message, unescapeArgs(message));
        }

    }

    /**
     * Callback from NativeDaemonConnector
     */
    public boolean onEvent(int code, String raw, String[] cooked) {
        synchronized (mLock) {
            return onEventLocked(code, raw, cooked);
        }
    }

    private boolean onEventLocked(int code, String raw, String[] cooked) {
        switch (code) {
            case VoldResponseCode.DISK_CREATED: {
                if (cooked.length != 2) break;
                final String id = cooked[0];
                int flags = Integer.parseInt(cooked[1]);

                mDisks.put(id, new DiskInfo(id, flags));
                break;
            }
            case VoldResponseCode.DISK_SIZE_CHANGED: {
                if (cooked.length != 2) break;
                final DiskInfo disk = mDisks.get(cooked[0]);
                if (disk != null) {
                    //disk.size = Long.parseLong(cooked[1]);
                }
                break;
            }
            case VoldResponseCode.DISK_LABEL_CHANGED: {
                final DiskInfo disk = mDisks.get(cooked[0]);
                if (disk != null) {
                    final StringBuilder builder = new StringBuilder();
                    for (int i = 1; i < cooked.length; i++) {
                        builder.append(cooked[i]).append(' ');
                    }
                    disk.label = builder.toString().trim();
                }
                break;
            }
            case VoldResponseCode.DISK_SCANNED: {
                if (cooked.length != 1) break;
                final DiskInfo disk = mDisks.get(cooked[0]);
                break;
            }
            case VoldResponseCode.DISK_SYS_PATH_CHANGED: {
                if (cooked.length != 2) break;
                final DiskInfo disk = mDisks.get(cooked[0]);
                if (disk != null) {
                    disk.sysPath = cooked[1];
                }
                break;
            }
            case VoldResponseCode.DISK_DESTROYED: {
                if (cooked.length != 1) break;
                final DiskInfo disk = mDisks.remove(cooked[0]);
                break;
            }

            case VoldResponseCode.VOLUME_CREATED: {
                final String id = cooked[0];
                final int type = Integer.parseInt(cooked[1]);
                final String diskId = TextUtils.nullIfEmpty(cooked[2]);
                final String partGuid = TextUtils.nullIfEmpty(cooked[3]);

                final DiskInfo disk = mDisks.get(diskId);
                final VolumeInfo vol = new VolumeInfo(id, type, disk, partGuid);
                mVolumes.put(id, vol);
                onVolumeCreatedLocked(vol);
                break;
            }
            case VoldResponseCode.VOLUME_STATE_CHANGED: {
                if (cooked.length != 2) break;
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    final int oldState = vol.state;
                    final int newState = Integer.parseInt(cooked[1]);
                    vol.state = newState;
                    onVolumeStateChangedLocked(vol, oldState, newState);
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_TYPE_CHANGED: {
                if (cooked.length != 2) break;
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    vol.fsType = cooked[1];
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_UUID_CHANGED: {
                if (cooked.length != 2) break;
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    vol.fsUuid = cooked[1];
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_LABEL_CHANGED: {
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    final StringBuilder builder = new StringBuilder();
                    for (int i = 1; i < cooked.length; i++) {
                        builder.append(cooked[i]).append(' ');
                    }
                    vol.fsLabel = builder.toString().trim();
                }
                break;
            }
            case VoldResponseCode.VOLUME_PATH_CHANGED: {
                if (cooked.length != 2) break;
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    vol.path = cooked[1];
                }
                break;
            }
            case VoldResponseCode.VOLUME_INTERNAL_PATH_CHANGED: {
                if (cooked.length != 2) break;
                final VolumeInfo vol = mVolumes.get(cooked[0]);
                if (vol != null) {
                    vol.internalPath = cooked[1];
                }
                break;
            }
            case VoldResponseCode.VOLUME_DESTROYED: {
                if (cooked.length != 1) break;
                mVolumes.remove(cooked[0]);
                break;
            }

            default: {
                Slog.d(TAG, "Unhandled vold event " + code);
            }
        }

        return true;
    }

    private void onVolumeCreatedLocked(VolumeInfo vol) {

        if (vol.type == VolumeInfo.TYPE_PUBLIC) {
            // Adoptable public disks are visible to apps, since they meet
            // public API requirement of being in a stable location.
            vol.mountFlags |= VolumeInfo.MOUNT_FLAG_VISIBLE;

            vol.mountUserId = mCurrentUserId;
            mHandler.obtainMessage(H_VOLUME_MOUNT, vol).sendToTarget();
        } else {
            Slog.d(TAG, "Skipping automatic mounting of " + vol);
        }
    }

    private void onVolumeStateChangedLocked(VolumeInfo vol, int oldState, int newState) {
        // Remember that we saw this volume so we're ready to accept user
        // metadata, or so we can annoy them when a private volume is ejected
        if (vol.isMountedReadable() && !TextUtils.isEmpty(vol.fsUuid)) {
            VolumeRecord rec = mRecords.get(vol.fsUuid);
            if (rec == null) {
                rec = new VolumeRecord(vol.type, vol.fsUuid);
                rec.partGuid = vol.partGuid;
                rec.createdMillis = System.currentTimeMillis();
                if (vol.type == VolumeInfo.TYPE_PRIVATE) {
                    rec.nickname = vol.disk.getDescription();
                }
                mRecords.put(rec.fsUuid, rec);
            } else {
                // Handle upgrade case where we didn't store partition GUID
                if (TextUtils.isEmpty(rec.partGuid)) {
                    rec.partGuid = vol.partGuid;
                }
            }
        }

        final String oldStateEnv = VolumeInfo.getEnvironmentForState(oldState);
        final String newStateEnv = VolumeInfo.getEnvironmentForState(newState);

        if (!Objects.equals(oldStateEnv, newStateEnv)) {
            // Kick state changed event towards all started users. Any users
            // started after this point will trigger additional
            // user-specific broadcasts.
            if (vol.path != null) {
                final StorageVolume userVol = vol.buildStorageVolume(mContext, mCurrentUserId, false);
                mHandler.obtainMessage(H_VOLUME_BROADCAST, userVol).sendToTarget();
            }
        }
    }

    private void enforcePermission(String perm) {
        mContext.enforceCallingOrSelfPermission(perm, perm);
    }

    /**
     * Decide if volume is mountable per device policies.
     */
    private boolean isMountDisallowed(VolumeInfo vol) {
        UserManager userManager = mContext.getSystemService(UserManager.class);

        boolean isUsbRestricted = false;
        if (vol.disk != null && vol.disk.isUsb()) {
            isUsbRestricted = userManager.hasUserRestriction(UserManager.DISALLOW_USB_FILE_TRANSFER,
                    Binder.getCallingUserHandle());
        }

        boolean isTypeRestricted = false;
        if (vol.type == VolumeInfo.TYPE_PUBLIC || vol.type == VolumeInfo.TYPE_PRIVATE) {
            isTypeRestricted = userManager
                    .hasUserRestriction(UserManager.DISALLOW_MOUNT_PHYSICAL_MEDIA,
                    Binder.getCallingUserHandle());
        }

        return isUsbRestricted || isTypeRestricted;
    }

    /**
     * Constructs a new StorageManagerService instance
     *
     * @param context  Binder context for this service
     */
    public DroidVoldManager(Context context) {
        sSelf = this;
        mContext = context;

        HandlerThread hthread = new HandlerThread(TAG);
        hthread.start();
        mHandler = new DroidVoldManagerHandler(hthread.getLooper());

        try {
            mDroidVold = IDroidVold.getService();
            mDroidVoldCallback = new DroidVoldCallback(this);

            if (DEBUG) Slog.d(TAG, "setCallback");
            mDroidVold.setCallback(mDroidVoldCallback);
            mDroidVold.linkToDeath(new DeathRecipient(), 0);

        } catch (NoSuchElementException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not found."
                    + " Did the service fail to start?", e);
        } catch (RemoteException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
        }

        if (DEBUG) Slog.d(TAG, "restIfReady and connected");
        resetIfReadyAndConnected();

        ServiceManager.addService("droidmount", sSelf, false);
    }

    private String findVolumeIdForPathOrThrow(String path) {
        synchronized (mLock) {
            for (int i = 0; i < mVolumes.size(); i++) {
                final VolumeInfo vol = mVolumes.valueAt(i);
                if (vol.path != null && path.startsWith(vol.path)) {
                    return vol.id;
                }
            }
        }
        throw new IllegalArgumentException("No volume found for path " + path);
    }

    private VolumeInfo findVolumeByIdOrThrow(String id) {
        synchronized (mLock) {
            final VolumeInfo vol = mVolumes.get(id);
            if (vol != null) {
                return vol;
            }
        }
        throw new IllegalArgumentException("No volume found for ID " + id);
    }

    /**
     * Exposed API calls below here
     */

    @Override
    public void shutdown(final IStorageShutdownObserver observer) {
        enforcePermission(android.Manifest.permission.SHUTDOWN);

        Slog.i(TAG, "Shutting down");
        mHandler.obtainMessage(H_SHUTDOWN, observer).sendToTarget();
    }

    @Override
    public int formatVolume(String path) {
        format(findVolumeIdForPathOrThrow(path));
        return 0;
    }

    @Override
    public int mkdirs(String callingPkg, String appPath) {
        //ignore
        /*
        final int userId = UserHandle.getUserId(Binder.getCallingUid());
        final UserEnvironment userEnv = new UserEnvironment(userId);

        File appFile = null;
        try {
            appFile = new File(appPath).getCanonicalFile();
        } catch (IOException e) {
            Slog.e(TAG, "Failed to resolve " + appPath + ": " + e);
            return -1;
        }

        // Try translating the app path into a vold path, but require that it
        // belong to the calling package.
        if (FileUtils.contains(userEnv.buildExternalStorageAppDataDirs(callingPkg), appFile) ||
                FileUtils.contains(userEnv.buildExternalStorageAppObbDirs(callingPkg), appFile) ||
                FileUtils.contains(userEnv.buildExternalStorageAppMediaDirs(callingPkg), appFile)) {
            appPath = appFile.getAbsolutePath();
            if (!appPath.endsWith("/")) {
                appPath = appPath + "/";
            }

            try {
                mConnector.execute("volume", "mkdirs", appPath);
                return 0;
            } catch (NativeDaemonConnectorException e) {
                return e.getCode();
            }
        }

        throw new SecurityException("Invalid mkdirs path: " + appFile);
        */
        return 0;
    }

    @Override
    public StorageVolume[] getVolumeList(int uid, String packageName, int flags) {
        final int userId = UserHandle.getUserId(uid);

        final boolean forWrite = (flags & StorageManager.FLAG_FOR_WRITE) != 0;
        final boolean realState = (flags & StorageManager.FLAG_REAL_STATE) != 0;
        final boolean includeInvisible = (flags & StorageManager.FLAG_INCLUDE_INVISIBLE) != 0;


        boolean foundPrimary = false;

        final ArrayList<StorageVolume> res = new ArrayList<>();
        synchronized (mLock) {
            for (int i = 0; i < mVolumes.size(); i++) {
                final VolumeInfo vol = mVolumes.valueAt(i);
                switch (vol.getType()) {
                    case VolumeInfo.TYPE_PUBLIC:
                    case VolumeInfo.TYPE_EMULATED:
                        break;
                    default:
                        continue;
                }

                boolean match = false;
                if (forWrite) {
                    match = vol.isVisibleForWrite(userId);
                } else {
                    match = vol.isVisibleForRead(userId)
                            || (includeInvisible && vol.getPath() != null);
                }
                if (!match) continue;

                boolean reportUnmounted = false;
                if ((vol.getType() == VolumeInfo.TYPE_EMULATED)) {
                    reportUnmounted = true;
                } else if (!realState) {
                    reportUnmounted = true;
                }

                final StorageVolume userVol = vol.buildStorageVolume(mContext, userId,
                        reportUnmounted);
                if (vol.isPrimary()) {
                    res.add(0, userVol);
                    foundPrimary = true;
                } else {
                    res.add(userVol);
                }
            }
        }

        if (!foundPrimary) {
            Log.w(TAG, "No primary storage defined yet; hacking together a stub");

            final boolean primaryPhysical = SystemProperties.getBoolean(
                    StorageManager.PROP_PRIMARY_PHYSICAL, false);

            final String id = "stub_primary";
            final File path = Environment.getLegacyExternalStorageDirectory();
            final String description = mContext.getString(android.R.string.unknownName);
            final boolean primary = true;
            final boolean removable = primaryPhysical;
            final boolean emulated = !primaryPhysical;
            final long mtpReserveSize = 0L;
            final boolean allowMassStorage = false;
            final long maxFileSize = 0L;
            final UserHandle owner = new UserHandle(userId);
            final String uuid = null;
            final String state = Environment.MEDIA_REMOVED;

            res.add(0, new StorageVolume(id, StorageVolume.STORAGE_ID_INVALID, path,
                    description, primary, removable, emulated, mtpReserveSize,
                    allowMassStorage, maxFileSize, owner, uuid, state));
        }

        return res.toArray(new StorageVolume[res.size()]);
    }

    @Override
    public DiskInfo[] getDisks() {
        synchronized (mLock) {
            final DiskInfo[] res = new DiskInfo[mDisks.size()];
            for (int i = 0; i < mDisks.size(); i++) {
                res[i] = mDisks.valueAt(i);
            }
            return res;
        }
    }

    @Override
    public VolumeInfo[] getVolumes(int flags) {
        synchronized (mLock) {
            final VolumeInfo[] res = new VolumeInfo[mVolumes.size()];
            for (int i = 0; i < mVolumes.size(); i++) {
                res[i] = mVolumes.valueAt(i);
            }
            return res;
        }
    }

    @Override
    public VolumeRecord[] getVolumeRecords(int flags) {
        synchronized (mLock) {
            final VolumeRecord[] res = new VolumeRecord[mRecords.size()];
            for (int i = 0; i < mRecords.size(); i++) {
                res[i] = mRecords.valueAt(i);
            }
            return res;
        }
    }

    @Override
    public void mount(String volId) {
        enforcePermission(android.Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS);

        final VolumeInfo vol = findVolumeByIdOrThrow(volId);
        if (isMountDisallowed(vol)) {
            throw new SecurityException("Mounting " + volId + " restricted by policy");
        }

        try {
            mDroidVold.mount(vol.id, vol.mountFlags, vol.mountUserId);
        } catch (RemoteException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
        }
    }

    @Override
    public void unmount(String volId) {
        enforcePermission(android.Manifest.permission.MOUNT_UNMOUNT_FILESYSTEMS);

        final VolumeInfo vol = findVolumeByIdOrThrow(volId);
        try {
            mDroidVold.unmount(vol.id);
        } catch (RemoteException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
        }
    }

    @Override
    public void format(String volId) {
        enforcePermission(android.Manifest.permission.MOUNT_FORMAT_FILESYSTEMS);

        final VolumeInfo vol = findVolumeByIdOrThrow(volId);

        try {
            mDroidVold.format(vol.id, "auto");
        } catch (RemoteException e) {
            Slog.e(TAG, "connectToProxy: droidvold hal service not responding", e);
        }
    }

    @Override
    public int getVolumeInfoDiskFlags(String mountPoint) {
        int flag = 0;
        int FLAG_SD = 1 << 2;
        int FLAG_USB = 1 << 3;

        synchronized (mLock) {
            for (int i = 0; i < mVolumes.size(); i++) {
                final VolumeInfo vol = mVolumes.valueAt(i);
                if (vol.path != null && mountPoint.startsWith(vol.path)) {
                    if (vol.getDisk().isSd())
                        flag |= FLAG_SD;
                    else if (vol.getDisk().isUsb())
                        flag |= FLAG_USB;
                    break;
                }
            }
        }

        return flag;
    }


    @Override
    protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        if (!DumpUtils.checkDumpPermission(mContext, TAG, writer)) return;

        final IndentingPrintWriter pw = new IndentingPrintWriter(writer, "  ", 160);
        synchronized (mLock) {
            pw.println("Disks:");
            pw.increaseIndent();
            for (int i = 0; i < mDisks.size(); i++) {
                final DiskInfo disk = mDisks.valueAt(i);
                disk.dump(pw);
            }
            pw.decreaseIndent();

            pw.println();
            pw.println("Volumes:");
            pw.increaseIndent();
            for (int i = 0; i < mVolumes.size(); i++) {
                final VolumeInfo vol = mVolumes.valueAt(i);
                if (VolumeInfo.ID_PRIVATE_INTERNAL.equals(vol.id)) continue;
                vol.dump(pw);
            }
            pw.decreaseIndent();

            pw.println();
            pw.println("Records:");
            pw.increaseIndent();
            for (int i = 0; i < mRecords.size(); i++) {
                final VolumeRecord note = mRecords.valueAt(i);
                note.dump(pw);
            }
            pw.decreaseIndent();

            final Pair<String, Long> pair = StorageManager.getPrimaryStoragePathAndSize();
            if (pair == null) {
                pw.println("Internal storage total size: N/A");
            } else {
                pw.print("Internal storage (");
                pw.print(pair.first);
                pw.print(") total size: ");
                pw.print(pair.second);
                pw.print(" (");
                pw.print((float) pair.second / TrafficStats.GB_IN_BYTES);
                pw.println(" GB)");
            }
            pw.println();
        }

        pw.println();
        pw.decreaseIndent();
    }
}
