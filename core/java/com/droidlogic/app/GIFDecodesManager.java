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

import java.io.InputStream;

import android.graphics.Bitmap;

public class GIFDecodesManager {
    private static final String TAG = "GIFDecodesManager";

    static {
        System.loadLibrary("gifdecode_jni");
    }

    private static native void nativeDecodeStream(InputStream istream);
    private static native void nativeDestructor();
    private static native int nativeWidth();
    private static native int nativeHeight();
    private static native int nativeTotalDuration();
    private static native boolean nativeSetCurrFrame(int frameIndex);
    private static native int nativeGetFrameDuration(int frameIndex);
    private static native int nativeGetFrameCount();
    private static native Bitmap nativeGetFrameBitmap(int frameIndex);

    public static void decodeStream(InputStream is) {
        nativeDecodeStream(is);
    }

    public static void destructor() {
        nativeDestructor();
    }

    public static int width() {
        return nativeWidth();
    }

    public static int height() {
        return nativeHeight();
    }

    public static int getTotalDuration() {
        return nativeTotalDuration();
    }

    public static boolean setCurrFrame(int frameIndex) {
        return nativeSetCurrFrame(frameIndex);
    }

    public static int getFrameDuration(int frameIndex) {
        return nativeGetFrameDuration(frameIndex);
    }

    public static int getFrameCount() {
        return nativeGetFrameCount();
    }

    public static Bitmap getFrameBitmap(int frameIndex) {
        return nativeGetFrameBitmap(frameIndex);
    }
}
