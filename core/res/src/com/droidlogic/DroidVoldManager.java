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
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.storage.DiskInfo;
import android.os.storage.IStorageShutdownObserver;
import android.os.storage.StorageResultCode;
import android.os.storage.StorageVolume;
import android.os.storage.VolumeInfo;
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
import com.android.internal.os.SomeArgs;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.DumpUtils;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.HexDump;
import com.android.internal.util.IndentingPrintWriter;
import com.android.internal.util.Preconditions;
import com.android.internal.widget.LockPatternUtils;

import libcore.io.IoUtils;
import libcore.util.EmptyArray;

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
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Service responsible for various storage media. Connects to {@code droidvold} to
 * watch for and manage dynamically added storage, such as SD cards and USB mass storage.
 */
class DroidVoldManager implements INativeDaemonConnectorCallbacks {

    // Static direct instance pointer for the tightly-coupled idle service to use
    static DroidVoldManager sSelf = null;

    private static final boolean DEBUG = false;

    private static final String TAG = "DroidVoldManager";
    private static final String VOLD_TAG = "DroidVoldConnector";
    private static final int MAX_CONTAINERS = 250;

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

    /** Set of users that system knows are unlocked. */
    @GuardedBy("mLock")
    private int[] mSystemUnlockedUsers = EmptyArray.INT;

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

    private final NativeDaemonConnector mConnector;

    private final Thread mConnectorThread;

    private volatile boolean mDaemonConnected = false;

    // Two connectors - mConnector & mCryptConnector
    private final CountDownLatch mConnectedSignal = new CountDownLatch(1);

    public static @Nullable String getBroadcastForEnvironment(String envState) {
        return sEnvironmentToBroadcast.get(envState);
    }

    // Handler messages
    private static final int H_DAEMON_CONNECTED = 1;
    private static final int H_SHUTDOWN = 2;
    private static final int H_VOLUME_MOUNT = 3;
    private static final int H_VOLUME_BROADCAST = 4;
    private static final int H_INTERNAL_BROADCAST = 5;
    private static final int H_PARTITION_FORGET = 6;
    private static final int H_RESET = 7;

    class DroidVoldManagerHandler extends Handler {
        public DroidVoldManagerHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case H_DAEMON_CONNECTED: {
                    handleDaemonConnected();
                    break;
                }
                case H_SHUTDOWN: {
                    final IStorageShutdownObserver obs = (IStorageShutdownObserver) msg.obj;
                    boolean success = false;
                    try {
                        success = mConnector.execute("volume", "shutdown").isClassOk();
                    } catch (NativeDaemonConnectorException ignored) {
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
                        mConnector.execute("volume", "mount", vol.id, vol.mountFlags,
                                vol.mountUserId);
                    } catch (NativeDaemonConnectorException ignored) {
                    }
                    break;
                }
                case H_VOLUME_BROADCAST: {
                    final StorageVolume userVol = (StorageVolume) msg.obj;
                    final String envState = userVol.getState();
                    Slog.d(TAG, "Volume " + userVol.getId() + " broadcasting " + envState + " to "
                            + userVol.getOwner());

                    final String action = getBroadcastForEnvironment(envState);
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

    private void waitForReady() {
        waitForLatch(mConnectedSignal, "mConnectedSignal");
    }

    private void waitForLatch(CountDownLatch latch, String condition) {
        try {
            waitForLatch(latch, condition, -1);
        } catch (TimeoutException ignored) {
        }
    }

    private void waitForLatch(CountDownLatch latch, String condition, long timeoutMillis)
            throws TimeoutException {
        final long startMillis = SystemClock.elapsedRealtime();
        while (true) {
            try {
                if (latch.await(5000, TimeUnit.MILLISECONDS)) {
                    return;
                } else {
                    Slog.w(TAG, "Thread " + Thread.currentThread().getName()
                            + " still waiting for " + condition + "...");
                }
            } catch (InterruptedException e) {
                Slog.w(TAG, "Interrupt while waiting for " + condition);
            }
            if (timeoutMillis > 0 && SystemClock.elapsedRealtime() > startMillis + timeoutMillis) {
                throw new TimeoutException("Thread " + Thread.currentThread().getName()
                        + " gave up waiting for " + condition + " after " + timeoutMillis + "ms");
            }
        }
    }

    private void resetIfReadyAndConnected() {
        Slog.d(TAG, "Thinking about reset mDaemonConnected=" + mDaemonConnected);
        if (mDaemonConnected) {
            final List<UserInfo> users = mContext.getSystemService(UserManager.class).getUsers();

            final int[] systemUnlockedUsers;
            synchronized (mLock) {
                systemUnlockedUsers = mSystemUnlockedUsers;

                mDisks.clear();
                mVolumes.clear();
            }

            try {
                mConnector.execute("volume", "reset");
            } catch (NativeDaemonConnectorException e) {
                Slog.w(TAG, "Failed to reset vold", e);
            }
        }
    }

    /**
     * Callback from NativeDaemonConnector
     */
    @Override
    public void onDaemonConnected() {
        mDaemonConnected = true;
        mHandler.obtainMessage(H_DAEMON_CONNECTED).sendToTarget();
    }

    private void handleDaemonConnected() {
        resetIfReadyAndConnected();

        /*
         * Now that we've done our initialization, release
         * the hounds!
         */
        mConnectedSignal.countDown();
        if (mConnectedSignal.getCount() != 0) {
            // More daemons need to connect
            return;
        }
    }

    /**
     * Callback from NativeDaemonConnector
     */
    @Override
    public boolean onCheckHoldWakeLock(int code) {
        return false;
    }

    /**
     * Callback from NativeDaemonConnector
     */
    @Override
    public boolean onEvent(int code, String raw, String[] cooked) {
        synchronized (mLock) {
            return onEventLocked(code, raw, cooked);
        }
    }

    private boolean onEventLocked(int code, String raw, String[] cooked) {
        switch (code) {
            case VoldResponseCode.DISK_CREATED: {
                if (cooked.length != 3) break;
                final String id = cooked[1];
                int flags = Integer.parseInt(cooked[2]);

                mDisks.put(id, new DiskInfo(id, flags));
                break;
            }
            case VoldResponseCode.DISK_SIZE_CHANGED: {
                if (cooked.length != 3) break;
                final DiskInfo disk = mDisks.get(cooked[1]);
                if (disk != null) {
                    disk.size = Long.parseLong(cooked[2]);
                }
                break;
            }
            case VoldResponseCode.DISK_LABEL_CHANGED: {
                final DiskInfo disk = mDisks.get(cooked[1]);
                if (disk != null) {
                    final StringBuilder builder = new StringBuilder();
                    for (int i = 2; i < cooked.length; i++) {
                        builder.append(cooked[i]).append(' ');
                    }
                    disk.label = builder.toString().trim();
                }
                break;
            }
            case VoldResponseCode.DISK_SCANNED: {
                if (cooked.length != 2) break;
                final DiskInfo disk = mDisks.get(cooked[1]);
                break;
            }
            case VoldResponseCode.DISK_SYS_PATH_CHANGED: {
                if (cooked.length != 3) break;
                final DiskInfo disk = mDisks.get(cooked[1]);
                if (disk != null) {
                    disk.sysPath = cooked[2];
                }
                break;
            }
            case VoldResponseCode.DISK_DESTROYED: {
                if (cooked.length != 2) break;
                final DiskInfo disk = mDisks.remove(cooked[1]);
                break;
            }

            case VoldResponseCode.VOLUME_CREATED: {
                final String id = cooked[1];
                final int type = Integer.parseInt(cooked[2]);
                final String diskId = TextUtils.nullIfEmpty(cooked[3]);
                final String partGuid = TextUtils.nullIfEmpty(cooked[4]);

                final DiskInfo disk = mDisks.get(diskId);
                final VolumeInfo vol = new VolumeInfo(id, type, disk, partGuid);
                mVolumes.put(id, vol);
                onVolumeCreatedLocked(vol);
                break;
            }
            case VoldResponseCode.VOLUME_STATE_CHANGED: {
                if (cooked.length != 3) break;
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    final int oldState = vol.state;
                    final int newState = Integer.parseInt(cooked[2]);
                    vol.state = newState;
                    onVolumeStateChangedLocked(vol, oldState, newState);
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_TYPE_CHANGED: {
                if (cooked.length != 3) break;
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    vol.fsType = cooked[2];
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_UUID_CHANGED: {
                if (cooked.length != 3) break;
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    vol.fsUuid = cooked[2];
                }
                break;
            }
            case VoldResponseCode.VOLUME_FS_LABEL_CHANGED: {
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    final StringBuilder builder = new StringBuilder();
                    for (int i = 2; i < cooked.length; i++) {
                        builder.append(cooked[i]).append(' ');
                    }
                    vol.fsLabel = builder.toString().trim();
                }
                break;
            }
            case VoldResponseCode.VOLUME_PATH_CHANGED: {
                if (cooked.length != 3) break;
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    vol.path = cooked[2];
                }
                break;
            }
            case VoldResponseCode.VOLUME_INTERNAL_PATH_CHANGED: {
                if (cooked.length != 3) break;
                final VolumeInfo vol = mVolumes.get(cooked[1]);
                if (vol != null) {
                    vol.internalPath = cooked[2];
                }
                break;
            }
            case VoldResponseCode.VOLUME_DESTROYED: {
                if (cooked.length != 2) break;
                mVolumes.remove(cooked[1]);
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
            final StorageVolume userVol = vol.buildStorageVolume(mContext, mCurrentUserId, false);
            mHandler.obtainMessage(H_VOLUME_BROADCAST, userVol).sendToTarget();

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

        Slog.d(TAG, "start handle");
        HandlerThread hthread = new HandlerThread(TAG);
        hthread.start();
        mHandler = new DroidVoldManagerHandler(hthread.getLooper());

       // LocalServices.addService(StorageManagerInternal.class, mStorageManagerInternal);

        /*
         * Create the connection to vold with a maximum queue of twice the
         * amount of containers we'd ever expect to have. This keeps an
         * "asec list" from blocking a thread repeatedly.
         */

        mConnector = new NativeDaemonConnector(this, "droidvold", MAX_CONTAINERS * 2, VOLD_TAG, 25,
                null, hthread.getLooper());
        mConnector.setDebug(true);
        mConnector.setWarnIfHeld(mLock);

        mConnectorThread = new Thread(mConnector, VOLD_TAG);
        Slog.d(TAG, "start nativeNaemon");
        mConnectorThread.start();

    }

    /**
     * Exposed API calls below here
     */

    public void shutdown(final IStorageShutdownObserver observer) {
        enforcePermission(android.Manifest.permission.SHUTDOWN);

        Slog.i(TAG, "Shutting down");
        mHandler.obtainMessage(H_SHUTDOWN, observer).sendToTarget();
    }
}
