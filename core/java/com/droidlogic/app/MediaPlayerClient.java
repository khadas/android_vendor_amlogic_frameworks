/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.droidlogic.app;

import android.os.IBinder;
import android.os.Parcel;
import android.util.Log;

import com.droidlogic.app.MediaPlayerExt;

/** @hide */
public class MediaPlayerClient extends IMediaPlayerClient.Stub {
    private static final String TAG = "MediaPlayerClient";
    private static final boolean DEBUG = false;
    private MediaPlayerExt mMp;

    public MediaPlayerClient(MediaPlayerExt mp) {
        mMp = mp;
    }

    static IBinder create(MediaPlayerExt mp) {
        return (new MediaPlayerClient(mp)).asBinder();
    }

    public void notify(int msg, int ext1, int ext2, Parcel parcel) {
        if (DEBUG) Log.i(TAG, "[notify] msg:" + msg + ", ext1:" + ext1 + ", ext2:" + ext2 + ", parcel:" + parcel);
        mMp.postEvent(msg, ext1, ext2, parcel);
    }
}
