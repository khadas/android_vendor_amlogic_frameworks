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

import android.content.Context;
import android.content.Intent;
import android.util.Log;

//this event from native system control service
public class SystemControlEvent extends ISystemControlNotify.Stub {
    private static final String TAG                             = "SystemControlEvent";

    public static final String ACTION_SYSTEM_CONTROL_EVENT      = "android.intent.action.SYSTEM_CONTROL_EVENT";
    public static final String EVENT_TYPE                       = "event";

    //must sync with DisplayMode.h
    public static final int EVENT_OUTPUT_MODE_CHANGE            = 0;
    public static final int EVENT_DIGITAL_MODE_CHANGE           = 1;

    private Context mContext = null;

    public SystemControlEvent(Context context) {
        mContext = context;
    }

    @Override
    public void onEvent(int event) {
        Log.i(TAG, "system control callback event: " + event);

        Intent intent = new Intent(ACTION_SYSTEM_CONTROL_EVENT);
        intent.putExtra(EVENT_TYPE, event);
        mContext.sendStickyBroadcastAsUser(intent, android.os.UserHandle.ALL);
    }
}
