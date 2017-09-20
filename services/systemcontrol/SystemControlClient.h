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
 *  @author   tellen
 *  @version  1.0
 *  @date     2017/10/18
 *  @par function description:
 *  - 1 system control apis for other vendor process
 */

#ifndef ANDROID_SYSTEMCONTROLCLIENT_H
#define ANDROID_SYSTEMCONTROLCLIENT_H

#include <utils/Errors.h>
#include "ISystemControlNotify.h"
#include <string>
#include <vector>

#include <vendor/amlogic/hardware/systemcontrol/1.0/ISystemControl.h>
using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControlCallback;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::DroidDisplayInfo;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;

namespace android {

class SystemControlClient
{
public:
    SystemControlClient();

    bool getProperty(const std::string& key, std::string& value);
    bool getPropertyString(const std::string& key, std::string& value, std::string& def);
    int32_t getPropertyInt(const std::string& key, int32_t def);
    int64_t getPropertyLong(const std::string& key, int64_t def);

    bool getPropertyBoolean(const std::string& key, bool def);
    void setProperty(const std::string& key, const std::string& value);

    bool readSysfs(const std::string& path, std::string& value);
    bool writeSysfs(const std::string& path, const std::string& value);
    bool writeSysfs(const std::string& path, const char *value, const int size);

    int32_t readHdcpRX22Key(char *value, int size);
    bool writeHdcpRX22Key(const char *value, const int size);
    int32_t readHdcpRX14Key(char *value, int size);
    bool writeHdcpRX14Key(const char *value, const int size);
    bool writeHdcpRXImg(const std::string& path);

    void setBootEnv(const std::string& key, const std::string& value);
    bool getBootEnv(const std::string& key, std::string& value);
    void getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip);

    void loopMountUnmount(int &isMount, const std::string& path);

    void setMboxOutputMode(const std::string& mode);
    void setSinkOutputMode(const std::string& mode);

    void setDigitalMode(const std::string& mode);
    void setListener(const sp<ISystemControlCallback> callback);
    void setOsdMouseMode(const std::string& mode);
    void setOsdMousePara(int x, int y, int w, int h);
    void setPosition(int left, int top, int width, int height);
    void getPosition(const std::string& mode, int &x, int &y, int &w, int &h);
    void getDeepColorAttr(const std::string& mode, std::string& value);
    void saveDeepColorAttr(const std::string& mode, const std::string& dcValue);
    int64_t resolveResolutionValue(const std::string& mode);
    void setDolbyVisionEnable(int state);
    bool isTvSupportDolbyVision(std::string& mode);
    void setHdrMode(const std::string& mode);
    void setSdrMode(const std::string& mode);

    int32_t set3DMode(const std::string& mode3d);
    void init3DSetting(void);
    int getVideo3DFormat(void);
    int getDisplay3DTo2DFormat(void);
    bool setDisplay3DTo2DFormat(int format);
    bool setDisplay3DFormat(int format);
    int getDisplay3DFormat(void);
    bool setOsd3DFormat(int format);
    bool switch3DTo2D(int format);
    bool switch2DTo3D(int format);
    void autoDetect3DForMbox(void);
    bool getSupportDispModeList(std::vector<std::string>& supportDispModes);
    bool getActiveDispMode(std::string& activeDispMode);
    bool setActiveDispMode(std::string& activeDispMode);

    void isHDCPTxAuthSuccess(int &status);

 private:
    struct SystemControlDeathRecipient : public android::hardware::hidl_death_recipient  {
        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;
    };
    sp<SystemControlDeathRecipient> mDeathRecipient = nullptr;

    sp<ISystemControl> mSysCtrl;

};

}; // namespace android

#endif // ANDROID_SYSTEMCONTROLCLIENT_H
