/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/09/20
 *  @par function description:
 *  - 1 system control interface
 */

#ifndef ANDROID_SYSTEM_CONTROL_SERVICE_H
#define ANDROID_SYSTEM_CONTROL_SERVICE_H

#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Mutex.h>
#include <ISystemControlService.h>

#include "SysWrite.h"
#include "common.h"
#include "DisplayMode.h"
#include "Dimension.h"
#include <string>
#include <vector>

#include "ubootenv/Ubootenv.h"

#include <vendor/amlogic/hardware/droidvold/1.0/IDroidVold.h>
using ::vendor::amlogic::hardware::droidvold::V1_0::IDroidVold;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;

extern "C" int vdc_loop(int argc, char **argv);

namespace android {
// ----------------------------------------------------------------------------

class SystemControlService
{
public:
    SystemControlService(const char *path);
    virtual ~SystemControlService();

    bool getSupportDispModeList(std::vector<std::string> *supportDispModes);
    bool getActiveDispMode(std::string *activeDispMode);
    bool setActiveDispMode(std::string& activeDispMode);
    //read write property and sysfs
    bool getProperty(const std::string &key, std::string *value);
    bool getPropertyString(const std::string &key, std::string *value, const std::string &def);
    int32_t getPropertyInt(const std::string &key, int32_t def);
    int64_t getPropertyLong(const std::string &key, int64_t def);
    bool getPropertyBoolean(const std::string& key, bool def);
    void setProperty(const std::string& key, const std::string& value);
    bool readSysfs(const std::string& path, std::string& value);
    bool writeSysfs(const std::string& path, const std::string& value);
    bool writeSysfs(const std::string& path, const char *value, const int size);
    int32_t readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused)));
    bool writeHdcpRX22Key(const char *value, const int size);
    int32_t readHdcpRX14Key(char *value __attribute__((unused)), int size __attribute__((unused)));
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHdcpRXImg(const std::string& path);
    //set or get uboot env
    bool getBootEnv(const std::string& key, std::string& value);
    void setBootEnv(const std::string& key, const std::string& value);
    void getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);
    void loopMountUnmount(int isMount, const std::string& path);
    void setSourceOutputMode(const std::string& mode);
    void setSinkOutputMode(const std::string& mode);
    void setDigitalMode(const std::string& mode);
    void setOsdMouseMode(const std::string& mode);
    void setOsdMousePara(int x, int y, int w, int h);
    void setPosition(int left, int top, int width, int height);
    void getPosition(const std::string& mode, int &x, int &y, int &w, int &h);
    void setDolbyVisionEnable(int state);
    bool isTvSupportDolbyVision(std::string& mode);
    void isHDCPTxAuthSuccess(int &status);
    void saveDeepColorAttr(const std::string& mode, const std::string& dcValue);
    void getDeepColorAttr(const std::string& mode, std::string& value);
    void setHdrMode(const std::string& mode);
    void setSdrMode(const std::string& mode);
    int64_t resolveResolutionValue(const std::string& mode);
    void setListener(const sp<SystemControlNotify>& listener);

    //3D
    int32_t set3DMode(const std::string& mode3d);
    void init3DSetting(void);
    int32_t getVideo3DFormat(void);
    int32_t getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setDisplay3DFormat(int format);
    int32_t getDisplay3DFormat(void);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox();
    //3D end
    static SystemControlService* instantiate(const char *cfgpath);

    virtual status_t dump(int fd, const Vector<String16>& args);

    int getLogLevel();

private:
    int permissionCheck();
    void setLogLevel(int level);
    void traceValue(const std::string& type, const std::string& key, const std::string& value);
    void traceValue(const std::string& type, const std::string& key, const int size);
    int getProcName(pid_t pid, String16& procName);

    mutable Mutex mLock;

    int mLogLevel;

    SysWrite *pSysWrite;
    DisplayMode *pDisplayMode;
    Dimension *pDimension;
    Ubootenv *pUbootenv;

    struct DroidVoldDeathRecipient : public android::hardware::hidl_death_recipient  {
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    };
    sp<DroidVoldDeathRecipient> mDeathRecipient = nullptr;

    sp<IDroidVold> mDroidVold;
};

// ----------------------------------------------------------------------------

} // namespace android
#endif // ANDROID_SYSTEM_CONTROL_SERVICE_H
