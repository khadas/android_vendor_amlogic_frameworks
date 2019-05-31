/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "systemcontrol-jni"
#include "com_droidlogic_app_SystemControlManager.h"
static sp<SystemControlClient> spSysctrl = NULL;
sp<EventCallback> spEventCB;
static jobject SysCtrlObject;
static jmethodID notifyCallback;
static jmethodID FBCUpgradeCallback;
static JavaVM   *g_JavaVM = NULL;

const sp<SystemControlClient>& getSystemControlClient()
{
    if (spSysctrl == NULL)
        spSysctrl = new SystemControlClient();
    return spSysctrl;
}

static JNIEnv* getJniEnv(bool *needDetach) {
    int ret = -1;
    JNIEnv *env = NULL;
    ret = g_JavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (ret < 0) {
        ret = g_JavaVM->AttachCurrentThread(&env, NULL);
        if (ret < 0) {
            ALOGE("Can't attach thread ret = %d", ret);
            return NULL;
        }
        *needDetach = true;
    }
    return env;
}

static void DetachJniEnv() {
    int result = g_JavaVM->DetachCurrentThread();
    if (result != JNI_OK) {
        ALOGE("thread detach failed: %#x", result);
    }
}


void EventCallback::notify (const int event) {
    ALOGD("eventcallback notify event = %d", event);
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && SysCtrlObject != NULL) {
        env->CallVoidMethod(SysCtrlObject, notifyCallback, event);
    } else {
        ALOGE("env or SysCtrlObject is NULL");
    }
    if (needDetach) {
        DetachJniEnv();
    }
}

void EventCallback::notifyFBCUpgrade(int state, int param) {
    bool needDetach = false;
    JNIEnv *env = getJniEnv(&needDetach);
    if (env != NULL && SysCtrlObject != NULL) {
        env->CallVoidMethod(SysCtrlObject, FBCUpgradeCallback, state, param);
    } else {
        ALOGE("g_env or SysCtrlObject is NULL");
    }
    if (needDetach) {
        DetachJniEnv();
    }
}

static void ConnectSystemControl(JNIEnv *env, jclass clazz __unused, jobject obj)
{
    ALOGI("Connect System Control");
    const sp<SystemControlClient>& scc = getSystemControlClient();
    //spSysctrl = SystemControlClient::GetInstance();
    if (scc != NULL) {
        spEventCB = new EventCallback();
        scc->setListener(spEventCB);
        SysCtrlObject = env->NewGlobalRef(obj);
    }
}

static jstring GetProperty(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->getProperty(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else {
        return env->NewStringUTF("");
    }
}

static jstring GetPropertyString(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *def = env->GetStringUTFChars(jdef, nullptr);
        std::string val;
        scc->getPropertyString(key, val, def);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jdef, def);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jint GetPropertyInt(JNIEnv* env, jclass clazz __unused, jstring jkey, jint jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        signed int result = scc->getPropertyInt(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static jlong GetPropertyLong(JNIEnv* env, jclass clazz __unused, jstring jkey, jlong jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        jlong result = scc->getPropertyLong(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static jboolean GetPropertyBoolean(JNIEnv* env, jclass clazz __unused, jstring jkey, jboolean jdef) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        unsigned char result = scc->getPropertyBoolean(key, jdef);
        env->ReleaseStringUTFChars(jkey, key);
        return result;
    } else
        return -1;
}

static void SetProperty(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        scc->setProperty(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
    }
}

static jstring ReadSysfs(JNIEnv* env, jclass clazz __unused, jstring jpath) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        std::string val;
        scc->readSysfs(path, val);
        env->ReleaseStringUTFChars(jpath, path);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jboolean WriteSysfs(JNIEnv* env, jclass clazz __unused, jstring jpath, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        unsigned char result = scc->writeSysfs(path, val);
        env->ReleaseStringUTFChars(jpath, path);
        env->ReleaseStringUTFChars(jval, val);
        return result;
    } else
        return false;
}

static jboolean WriteSysfsSize(JNIEnv* env, jclass clazz __unused, jstring jpath, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        jint* jints = env->GetIntArrayElements(jval, NULL);
        int len = env->GetArrayLength(jval);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        unsigned char result = scc->writeSysfs(path, buf, size);
        env->ReleaseStringUTFChars(jpath, path);
        env->ReleaseIntArrayElements(jval, jints, 0);
        return result;
    } else {
        return false;
    }
}

static jboolean WriteUnifyKey(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        unsigned char result = scc->writeUnifyKey(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
        return result;
    } else
        return false;
}

static jstring ReadUnifyKey(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->readUnifyKey(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jboolean WritePlayreadyKey(JNIEnv* env, jclass clazz __unused, jstring jkey, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        jint* jints = env->GetIntArrayElements(jval, NULL);
        int len = env->GetArrayLength(jval);
        char buf[len+1];
        memset(buf,0,len + 1);
        for (int i = 0; i < len; i++) {
            buf[i] = jints[i];
        }
        buf[len] = '\0';
        unsigned char result = scc->writePlayreadyKey(key, buf, size);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseIntArrayElements(jval, jints, 0);
        return result;
    } else
        return false;
}

static jint ReadPlayreadyKey(JNIEnv* env, jclass clazz __unused, jstring jpath, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *path = env->GetStringUTFChars(jpath, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readPlayreadyKey(path, buf, size);
         env->ReleaseStringUTFChars(jpath, path);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jint ReadAttestationKey(JNIEnv* env, jclass clazz __unused, jstring jnode, jstring jname, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *node = env->GetStringUTFChars(jnode, nullptr);
         const char *name = env->GetStringUTFChars(jname, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readAttestationKey(node, name, buf, size);
         env->ReleaseStringUTFChars(jnode, node);
         env->ReleaseStringUTFChars(jname, name);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteAttestationKey(JNIEnv* env, jclass clazz __unused, jstring jnode, jstring jname, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         const char *node = env->GetStringUTFChars(jnode, nullptr);
         const char *name = env->GetStringUTFChars(jname, nullptr);
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeAttestationKey(node, name, buf, size);
         env->ReleaseStringUTFChars(jnode, node);
         env->ReleaseStringUTFChars(jname, name);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jint CheckAttestationKey(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         int result = scc->checkAttestationKey();
         return result;
     } else
         return -1;
}

static jint ReadHdcpRX22Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readHdcpRX22Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteHdcpRX22Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeHdcpRX22Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jint ReadHdcpRX14Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         int result = scc->readHdcpRX14Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return -1;
}

static jboolean WriteHdcpRX14Key(JNIEnv* env, jclass clazz __unused, jintArray jval, jint size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
     if (scc != NULL) {
         jint* jints = env->GetIntArrayElements(jval, NULL);
         int len = env->GetArrayLength(jval);
         char buf[len+1];
         memset(buf,0,len + 1);
         for (int i = 0; i < len; i++) {
             buf[i] = jints[i];
         }
         buf[len] = '\0';
         unsigned char result = scc->writeHdcpRX14Key(buf, size);
         env->ReleaseIntArrayElements(jval, jints, 0);
         return result;
     } else
         return false;
}

static jboolean WriteHdcpRXImg(JNIEnv* env, jclass clazz __unused, jstring jpath) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        unsigned char result = scc->writeHdcpRXImg(path);
        env->ReleaseStringUTFChars(jpath, path);
        return result;
    } else
        return false;
}

static jstring GetBootEnv(JNIEnv* env, jclass clazz __unused, jstring jkey) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        std::string val;
        scc->getBootEnv(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static void SetBootEnv(JNIEnv* env, jclass clazz __unused, jstring jkey, jstring jval) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *key = env->GetStringUTFChars(jkey, nullptr);
        const char *val = env->GetStringUTFChars(jval, nullptr);
        scc->setBootEnv(key, val);
        env->ReleaseStringUTFChars(jkey, key);
        env->ReleaseStringUTFChars(jval, val);
    }
}

static void LoopMountUnmount(JNIEnv* env, jclass clazz __unused, jboolean isMount, jstring jpath)  {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *path = env->GetStringUTFChars(jpath, nullptr);
        scc->loopMountUnmount(isMount, path);
        env->ReleaseStringUTFChars(jpath, path);
    }
}

static void SetMboxOutputMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setMboxOutputMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetSinkOutputMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setSinkOutputMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetDigitalMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setDigitalMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetOsdMouseMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setOsdMouseMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetOsdMousePara(JNIEnv* env __unused, jclass clazz __unused, jint x, jint y, jint w, jint h) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setOsdMousePara(x, y, w, h);
    }
}

static void SetPosition(JNIEnv* env __unused, jclass clazz __unused, jint left, jint top, jint width, jint height)  {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setPosition(left, top, width, height);
    }
}

static jintArray GetPosition(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jintArray result;
    result = env->NewIntArray(4);
    jint curPosition[4] = { 0, 0, 1280, 720 };
    env->SetIntArrayRegion(result, 0, 4, curPosition);
    if (scc != NULL) {
        int outx = 0;
        int outy = 0;
        int outw = 0;
        int outh = 0;
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->getPosition(mode, outx, outy, outw, outh);
        curPosition[0] = outx;
        curPosition[1] = outy;
        curPosition[2] = outw;
        curPosition[3] = outh;
        env->SetIntArrayRegion(result, 0, 4, curPosition);
        env->ReleaseStringUTFChars(jmode, mode);
    }
    return result;
}

static jstring GetDeepColorAttr(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        std::string val;
        scc->getDeepColorAttr(mode, val);
        env->ReleaseStringUTFChars(jmode, mode);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static jlong ResolveResolutionValue(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    jlong value = 0;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        value = scc->resolveResolutionValue(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
    return value;
}

static jstring IsTvSupportDolbyVision(JNIEnv* env, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        std::string mode;
        scc->isTvSupportDolbyVision(mode);
        return env->NewStringUTF(mode.c_str());
    } else
        return env->NewStringUTF("");
}

static void InitDolbyVision(JNIEnv* env __unused, jclass clazz __unused, jint state) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->initDolbyVision(state);
    }
}

static void SetDolbyVisionEnable(JNIEnv* env __unused, jclass clazz __unused, jint state) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        scc->setDolbyVisionEnable(state);
    }
}

static void SaveDeepColorAttr(JNIEnv* env, jclass clazz __unused, jstring jmode, jstring jdcValue) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        const char *dcValue = env->GetStringUTFChars(jdcValue, nullptr);
        scc->saveDeepColorAttr(mode, dcValue);
        env->ReleaseStringUTFChars(jmode, mode);
        env->ReleaseStringUTFChars(jdcValue, dcValue);
    }
}

static void SetHdrMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setHdrMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static void SetSdrMode(JNIEnv* env, jclass clazz __unused, jstring jmode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode = env->GetStringUTFChars(jmode, nullptr);
        scc->setSdrMode(mode);
        env->ReleaseStringUTFChars(jmode, mode);
    }
}

static jint GetDolbyVisionType(JNIEnv* env __unused, jclass clazz __unused) {
    jint result = -1;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL)
        result = scc->getDolbyVisionType();
    return result;
}

static void SetGraphicsPriority(JNIEnv* env, jclass clazz __unused, jstring jmode) {
   const sp<SystemControlClient>& scc = getSystemControlClient();
   if (scc != NULL) {
       const char *mode = env->GetStringUTFChars(jmode, nullptr);
       scc->setGraphicsPriority(mode);
       env->ReleaseStringUTFChars(jmode, mode);
   }
}

static jstring GetGraphicsPriority(JNIEnv* env, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        std::string val;
        scc->getGraphicsPriority(val);
        return env->NewStringUTF(val.c_str());
    } else
        return env->NewStringUTF("");
}

static void SetAppInfo(JNIEnv* env, jclass clazz __unused, jstring jpkg, jstring jcls, jobjectArray jproc) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    std::vector<std::string> proc;
    if (scc != NULL && jproc != NULL) {
        const char *pkg = env->GetStringUTFChars(jpkg, nullptr);
        const char *cls = env->GetStringUTFChars(jcls, nullptr);
        size_t count = env->GetArrayLength(jproc);
        proc.resize(count);
        for (size_t i = 0; i < count; ++i) {
            jstring jstr = (jstring)env->GetObjectArrayElement(jproc, i);
            const char *cString = env->GetStringUTFChars(jstr, nullptr);
            proc[i] = cString;
            env->ReleaseStringUTFChars(jstr, cString);
        }

        scc->setAppInfo(pkg, cls, proc);
        env->ReleaseStringUTFChars(jpkg, pkg);
        env->ReleaseStringUTFChars(jcls, cls);
    }
}

//3D
static jint Set3DMode(JNIEnv* env, jclass clazz __unused, jstring jmode3d) {
    int result = -1;
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *mode3d = env->GetStringUTFChars(jmode3d, nullptr);
        result = scc->set3DMode(mode3d);
        env->ReleaseStringUTFChars(jmode3d, mode3d);
    }
    return result;
}

static void Init3DSetting(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL)
        scc->init3DSetting();
}

static jint GetVideo3DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getVideo3DFormat();
    return result;
}

static jint GetDisplay3DTo2DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL)
        result = scc->getDisplay3DTo2DFormat();
    return result;
}

static jboolean SetDisplay3DTo2DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setDisplay3DTo2DFormat(format);
    return result;
}

static jboolean SetDisplay3DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setDisplay3DFormat(format);
    return result;
}

static jint GetDisplay3DFormat(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDisplay3DFormat();
    return result;
}

static jboolean SetOsd3DFormat(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->setOsd3DFormat(format);
    return result;
}

static jboolean Switch3DTo2D(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->switch3DTo2D(format);
    return result;
}

static jboolean Switch2DTo3D(JNIEnv* env __unused, jclass clazz __unused, jint format) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jboolean result = false;
    if (scc != NULL)
        result = scc->switch2DTo3D(format);
    return result;
}

static jint SetPQmode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave, jint is_autoswitch) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setPQmode(mode, isSave, is_autoswitch);
    return result;
}

static jint GetPQmode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getPQmode();
    return result;
}

static jint SavePQmode(JNIEnv* env __unused, jclass clazz __unused, jint mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->savePQmode(mode);
    return result;
}

static jint SetColorTemperature(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setColorTemperature(mode, isSave);
    return result;
}

static jint GetColorTemperature(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getColorTemperature();
    return result;
}

static jint SaveColorTemperature(JNIEnv* env __unused, jclass clazz __unused, jint mode) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveColorTemperature(mode);
    return result;
}

static jint SetBrightness(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setBrightness(value, isSave);
    return result;
}

static jint GetBrightness(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getBrightness();
    return result;
}

static jint SaveBrightness(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveBrightness(value);
    return result;
}

static jint SetContrast(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setContrast(value, isSave);
    return result;
}

static jint GetContrast(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getContrast();
    return result;
}

static jint SaveContrast(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveContrast(value);
    return result;
}

static jint SetSaturation(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setSaturation(value, isSave);
    return result;
}

static jint GetSaturation(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSaturation();
    return result;
}

static jint SaveSaturation(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveSaturation(value);
    return result;
}

static jint SetHue(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setHue(value, isSave);
    return result;
}

static jint GetHue(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getHue();
    return result;
}

static jint SaveHue(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveHue(value);
    return result;
}

static jint SetSharpness(JNIEnv* env __unused, jclass clazz __unused, jint value, jint is_enable, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setSharpness(value, is_enable, isSave);
    return result;
}

static jint GetSharpness(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSharpness();
    return result;
}

static jint SaveSharpness(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveSharpness(value);
    return result;
}

static jint SetNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setNoiseReductionMode(value, isSave);
    return result;
}

static jint GetNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getNoiseReductionMode();
    return result;
}

static jint SaveNoiseReductionMode(JNIEnv* env __unused, jclass clazz __unused, jint value) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveNoiseReductionMode(value);
    return result;
}

static jint SetEyeProtectionMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint enable, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setEyeProtectionMode(source_input, enable, isSave);
    return result;
}

static jint GetEyeProtectionMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getEyeProtectionMode(source_input);
    return result;
}

static jint SetGammaValue(JNIEnv* env __unused, jclass clazz __unused, jint gamma_curve, jint isSave) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setGammaValue(gamma_curve, isSave);
    return result;
}

static jint GetGammaValue(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getGammaValue();
    return result;
}

static jint SetDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setDisplayMode(source_input, mode, isSave);
    return result;
}

static jint GetDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDisplayMode(source_input);
    return result;
}

static jint SaveDisplayMode(JNIEnv* env __unused, jclass clazz __unused, jint source_input, jint mode)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveDisplayMode(source_input, mode);
    return result;
}

static jint SetBacklight(JNIEnv* env __unused, jclass clazz __unused, jint value, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setBacklight(value, isSave);
    return result;
}

static jint GetBacklight(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getBacklight();
    return result;
}

static jint SaveBacklight(JNIEnv* env __unused, jclass clazz __unused, jint value)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->saveBacklight(value);
    return result;
}

static jint CheckLdimExist(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->checkLdimExist();
    return result;
}

static jint SetDynamicBacklight(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setDynamicBacklight(mode, isSave);
    return result;
}

static jint GetDynamicBacklight(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getDynamicBacklight();
    return result;
}

static jint SetLocalContrastMode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
   // if (scc != NULL)
   //     result = scc->setLocalContrastMode(mode, isSave);
    return result;
}

static jint GetLocalContrastMode(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
 //   if (scc != NULL)
 //       result = scc->getLocalContrastMode();
    return result;
}

static jint SetColorBaseMode(JNIEnv* env __unused, jclass clazz __unused, jint mode, jint isSave)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
   // if (scc != NULL)
   //     result = scc->setColorBaseMode(mode, isSave);
    return result;
}

static jint GetColorBaseMode(JNIEnv* env __unused, jclass clazz __unused)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
   // if (scc != NULL)
    //    result = scc->getColorBaseMode();
    return result;
}

static jint SysSSMReadNTypes(JNIEnv* env __unused, jclass clazz __unused, jint id, jint data_len, jint offset) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->sysSSMReadNTypes(id, data_len, offset);
    return result;
}

static jint SysSSMWriteNTypes(JNIEnv* env __unused, jclass clazz __unused, jint id, jint data_len, jint data_buf, jint offset) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->sysSSMWriteNTypes(id, data_len, data_buf, offset);
    return result;
}

static jint GetActualAddr(JNIEnv* env __unused, jclass clazz __unused, jint id) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getActualAddr(id);
    return result;
}

static jint GetActualSize(JNIEnv* env __unused, jclass clazz __unused, jint id) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getActualSize(id);
    return result;
}

static jint SSMRecovery(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->SSMRecovery();
    return result;
}

static jint SetCVD2Values(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setCVD2Values();
    return result;
}

static jint GetSSMStatus(JNIEnv* env __unused, jclass clazz __unused) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->getSSMStatus();
    return result;
}

static jint SetCurrentSourceInfo(JNIEnv* env __unused, jclass clazz __unused, jint sourceInput, jint sigFmt, jint transFmt) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    jint result = -1;
    if (scc != NULL)
        result = scc->setCurrentSourceInfo(sourceInput, sigFmt, transFmt);
    return result;
}

static void GetCurrentSourceInfo(JNIEnv* env __unused, jclass clazz __unused, jint sourceInput, jint sigFmt, jint transFmt)
{
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL)
       scc->getCurrentSourceInfo(sourceInput, sigFmt, transFmt);
}

static void GetPQDatabaseInfo(JNIEnv* env, jclass clazz __unused, jint dataBaseName, jstring jToolVersion, jstring
jProjectVersion, jstring jGenerateTime) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    if (scc != NULL) {
        const char *ToolVersion = env->GetStringUTFChars(jToolVersion, nullptr);
        const char *ProjectVersion = env->GetStringUTFChars(jProjectVersion, nullptr);
        const char *GenerateTime = env->GetStringUTFChars(jGenerateTime, nullptr);
        std::string tmpTv = const_cast<char *>(ToolVersion);
        std::string tmpPv = const_cast<char *>(ProjectVersion);
        std::string tmpGt = const_cast<char *>(GenerateTime);
       // spSysctrl->getPQDatabaseInfo(dataBaseName, tmpTv, tmpPv, tmpGt);
        env->ReleaseStringUTFChars(jToolVersion, ToolVersion);
        env->ReleaseStringUTFChars(jProjectVersion, ProjectVersion);
        env->ReleaseStringUTFChars(jGenerateTime, GenerateTime);
    }
}

//FBC
static jint StartUpgradeFBC(JNIEnv* env, jclass clazz __unused, jstring jfile_name, jint mode, jint upgrade_blk_size) {
    const sp<SystemControlClient>& scc = getSystemControlClient();
    int result = -1;
    if (scc != NULL) {
        const char *file_name = env->GetStringUTFChars(jfile_name, nullptr);
       // result = spSysctrl->StartUpgradeFBC(file_name, mode, upgrade_blk_size);
        env->ReleaseStringUTFChars(jfile_name, file_name);
    }
    return result;
}


static JNINativeMethod SystemControl_Methods[] = {
{"native_ConnectSystemControl", "(Lcom/droidlogic/app/SystemControlManager;)V", (void *) ConnectSystemControl },
{"native_GetProperty", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetProperty },
{"native_GetPropertyString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void *) GetPropertyString },
{"native_GetPropertyInt", "(Ljava/lang/String;I)I", (void *) GetPropertyInt },
{"native_GetPropertyLong", "(Ljava/lang/String;J)J", (void *) GetPropertyLong },
{"native_GetPropertyBoolean", "(Ljava/lang/String;Z)Z", (void *) GetPropertyBoolean },
{"native_SetProperty", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SetProperty },
{"native_ReadSysfs", "(Ljava/lang/String;)Ljava/lang/String;", (void *) ReadSysfs },
{"native_WriteSysfs", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) WriteSysfs },
{"native_WriteSysfsSize", "(Ljava/lang/String;[II)Z", (void *) WriteSysfsSize },
{"native_WriteUnifyKey", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) WriteUnifyKey },
{"native_ReadUnifyKey", "(Ljava/lang/String;)Ljava/lang/String;", (void *) ReadUnifyKey },
{"native_WritePlayreadyKey", "(Ljava/lang/String;[II)Z", (void *) WritePlayreadyKey },
{"native_ReadPlayreadyKey", "(Ljava/lang/String;[II)I", (void *) ReadPlayreadyKey },
{"native_ReadAttestationKey", "(Ljava/lang/String;Ljava/lang/String;[II)I", (void *) ReadAttestationKey },
{"native_WriteAttestationKey", "(Ljava/lang/String;Ljava/lang/String;[II)Z", (void *) WriteAttestationKey },
{"native_CheckAttestationKey", "()I", (void *) CheckAttestationKey },
{"native_ReadHdcpRX22Key", "([II)I", (void *) ReadHdcpRX22Key },
{"native_WriteHdcpRX22Key", "([II)Z", (void *) WriteHdcpRX22Key },
{"native_ReadHdcpRX14Key", "([II)I", (void *) ReadHdcpRX14Key },
{"native_WriteHdcpRX14Key", "([II)Z", (void *) WriteHdcpRX14Key },
{"native_WriteHdcpRXImg", "(Ljava/lang/String;)Z", (void *) WriteHdcpRXImg },
{"native_GetBootEnv", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetBootEnv },
{"native_SetBootEnv", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SetBootEnv },
{"native_LoopMountUnmount", "(ZLjava/lang/String;)V", (void *) LoopMountUnmount },
{"native_SetMboxOutputMode", "(Ljava/lang/String;)V", (void *) SetMboxOutputMode },
{"native_SetSinkOutputMode", "(Ljava/lang/String;)V", (void *) SetSinkOutputMode },
{"native_SetDigitalMode", "(Ljava/lang/String;)V", (void *) SetDigitalMode },
{"native_SetOsdMouseMode", "(Ljava/lang/String;)V", (void *) SetOsdMouseMode },
{"native_SetOsdMousePara", "(IIII)V", (void *) SetOsdMousePara },
{"native_SetPosition", "(IIII)V", (void *) SetPosition },
{"native_GetPosition", "(Ljava/lang/String;)[I", (void *) GetPosition },
{"native_GetDeepColorAttr", "(Ljava/lang/String;)Ljava/lang/String;", (void *) GetDeepColorAttr },
{"native_ResolveResolutionValue", "(Ljava/lang/String;)J", (void *) ResolveResolutionValue },
{"native_IsTvSupportDolbyVision", "()Ljava/lang/String;", (void *) IsTvSupportDolbyVision },
{"native_InitDolbyVision", "(I)V", (void *) InitDolbyVision },
{"native_SetDolbyVisionEnable", "(I)V", (void *) SetDolbyVisionEnable },
{"native_SaveDeepColorAttr", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) SaveDeepColorAttr },
{"native_SetHdrMode", "(Ljava/lang/String;)V", (void *) SetHdrMode },
{"native_SetSdrMode", "(Ljava/lang/String;)V", (void *) SetSdrMode },
{"native_GetDolbyVisionType", "()I", (void *) GetDolbyVisionType },
{"native_SetGraphicsPriority", "(Ljava/lang/String;)V", (void *) SetGraphicsPriority },
{"native_GetGraphicsPriority", "()Ljava/lang/String;", (void *) GetGraphicsPriority },
{"native_SetAppInfo", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V", (void *) SetAppInfo },
{"native_Set3DMode", "(Ljava/lang/String;)I", (void *) Set3DMode },
{"native_Init3DSetting", "()V", (void *) Init3DSetting },
{"native_GetVideo3DFormat", "()I", (void *) GetVideo3DFormat },
{"native_GetDisplay3DTo2DFormat", "()I", (void *) GetDisplay3DTo2DFormat },
{"native_SetDisplay3DTo2DFormat", "(I)Z", (void *) SetDisplay3DTo2DFormat },
{"native_SetDisplay3DFormat", "(I)Z", (void *) SetDisplay3DFormat },
{"native_GetDisplay3DFormat", "()I", (void *) GetDisplay3DFormat },
{"native_SetOsd3DFormat", "(I)Z", (void *) SetOsd3DFormat },
{"native_Switch3DTo2D", "(I)Z", (void *) Switch3DTo2D },
{"native_Switch2DTo3D", "(I)Z", (void *) Switch2DTo3D },
{"native_SetPQmode", "(III)I", (void *) SetPQmode },
{"native_GetPQmode", "()I", (void *) GetPQmode },
{"native_SavePQmode", "(I)I", (void *) SavePQmode },
{"native_SetColorTemperature", "(II)I", (void *) SetColorTemperature },
{"native_GetColorTemperature", "()I", (void *) GetColorTemperature },
{"native_SaveColorTemperature", "(I)I", (void *) SaveColorTemperature },
{"native_SetBrightness", "(II)I", (void *) SetBrightness },
{"native_GetBrightness", "()I", (void *) GetBrightness },
{"native_SaveBrightness", "(I)I", (void *) SaveBrightness },
{"native_SetContrast", "(II)I", (void *) SetContrast },
{"native_GetContrast", "()I", (void *) GetContrast },
{"native_SaveContrast", "(I)I", (void *) SaveContrast },
{"native_SetSaturation", "(II)I", (void *) SetSaturation },
{"native_GetSaturation", "()I", (void *) GetSaturation },
{"native_SaveSaturation", "(I)I", (void *) SaveSaturation },
{"native_SetHue", "(II)I", (void *) SetHue },
{"native_GetHue", "()I", (void *) GetHue },
{"native_SaveHue", "(I)I", (void *) SaveHue },
{"native_SetSharpness", "(III)I", (void *) SetSharpness },
{"native_GetSharpness", "()I", (void *) GetSharpness },
{"native_SaveSharpness", "(I)I", (void *) SaveSharpness },
{"native_SetNoiseReductionMode", "(II)I", (void *) SetNoiseReductionMode },
{"native_GetNoiseReductionMode", "()I", (void *) GetNoiseReductionMode },
{"native_SaveNoiseReductionMode", "(I)I", (void *) SaveNoiseReductionMode },
{"native_SetEyeProtectionMode", "(III)I", (void *) SetEyeProtectionMode },
{"native_GetEyeProtectionMode", "(I)I", (void *) GetEyeProtectionMode },
{"native_SetGammaValue", "(II)I", (void *) SetGammaValue },
{"native_GetGammaValue", "()I", (void *) GetGammaValue },
{"native_SetDisplayMode", "(III)I", (void *) SetDisplayMode },
{"native_GetDisplayMode", "(I)I", (void *) GetDisplayMode },
{"native_SaveDisplayMode", "(II)I", (void *) SaveDisplayMode },
{"native_SetBacklight", "(II)I", (void *) SetBacklight },
{"native_GetBacklight", "()I", (void *) GetBacklight },
{"native_SaveBacklight", "(I)I", (void *) SaveBacklight },
{"native_CheckLdimExist", "()I", (void *) CheckLdimExist },
{"native_SetDynamicBacklight", "(II)I", (void *) SetDynamicBacklight },
{"native_GetDynamicBacklight", "()I", (void *) GetDynamicBacklight },
{"native_SetLocalContrastMode", "(II)I", (void *) SetLocalContrastMode },
{"native_GetLocalContrastMode", "()I", (void *) GetLocalContrastMode },
{"native_SetColorBaseMode", "(II)I", (void *) SetColorBaseMode },
{"native_GetColorBaseMode", "()I", (void *) GetColorBaseMode },
{"native_SysSSMReadNTypes", "(III)I", (void *) SysSSMReadNTypes },
{"native_SysSSMWriteNTypes", "(IIII)I", (void *) SysSSMWriteNTypes },
{"native_GetActualAddr", "(I)I", (void *) GetActualAddr },
{"native_GetActualSize", "(I)I", (void *) GetActualSize },
{"native_SSMRecovery", "()I", (void *) SSMRecovery },
{"native_SetCVD2Values", "()I", (void *) SetCVD2Values },
{"native_GetSSMStatus", "()I", (void *) GetSSMStatus },
{"native_SetCurrentSourceInfo", "(III)I", (void *) SetCurrentSourceInfo },
{"native_GetCurrentSourceInfo", "(III)V", (void *) GetCurrentSourceInfo },
{"native_GetPQDatabaseInfo", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", (void *) GetPQDatabaseInfo },
{"native_StartUpgradeFBC", "(Ljava/lang/String;II)I", (void *) StartUpgradeFBC },


};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_app_SystemControlManager(JNIEnv *env)
{
    static const char *const kClassPathName = "com/droidlogic/app/SystemControlManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, SystemControl_Methods, NELEM(SystemControl_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    GET_METHOD_ID(notifyCallback, clazz, "notifyCallback", "(I)V");
    GET_METHOD_ID(FBCUpgradeCallback, clazz, "FBCUpgradeCallback", "(II)V");
    return rc;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);
    g_JavaVM = vm;

    if (register_com_droidlogic_app_SystemControlManager(env) < 0)
    {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
