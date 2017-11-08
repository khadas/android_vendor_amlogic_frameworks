/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @date     2017/8/30
 *  @par function description:
 *  - 1 system control hwbinder service
 */

#ifndef ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H
#define ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H

#include <vendor/amlogic/hardware/systemcontrol/1.0/ISystemControl.h>
#include <SystemControlNotify.h>
#include <SystemControlService.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace systemcontrol {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControlCallback;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::DroidDisplayInfo;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct SystemControlHal : public ISystemControl, public SystemControlNotify {
  public:
    SystemControlHal(SystemControlService * control);
    ~SystemControlHal();

    Return<void> getSupportDispModeList(getSupportDispModeList_cb _hidl_cb) override;
    Return<void> getActiveDispMode(getActiveDispMode_cb _hidl_cb) override;
    Return<Result> setActiveDispMode(const hidl_string &activeDispMode) override;
    Return<Result> isHDCPTxAuthSuccess() override;
    Return<void> getProperty(const hidl_string &key, getProperty_cb _hidl_cb) override;
    Return<void> getPropertyString(const hidl_string &key, const hidl_string &def, getPropertyString_cb _hidl_cb) override;
    Return<void> getPropertyInt(const hidl_string &key, int32_t def, getPropertyInt_cb _hidl_cb)  override;
    Return<void> getPropertyLong(const hidl_string &key, int64_t def, getPropertyLong_cb _hidl_cb) override;
    Return<void> getPropertyBoolean(const hidl_string &key, bool def, getPropertyBoolean_cb _hidl_cb) override;
    Return<Result> setProperty(const hidl_string &key, const hidl_string &value) override;
    Return<void> readSysfs(const hidl_string &path, readSysfs_cb _hidl_cb) override;
    Return<Result> writeSysfs(const hidl_string &path, const hidl_string &value) override;
    Return<void> readHdcpRX22Key(int32_t size, readHdcpRX22Key_cb _hidl_cb) override;
    Return<Result> writeHdcpRX22Key(const hidl_string &key) override;
    Return<void> readHdcpRX14Key(int32_t size, readHdcpRX14Key_cb _hidl_cb) override;
    Return<Result> writeHdcpRX14Key(const hidl_string &key) override;
    Return<Result> writeHdcpRXImg(const hidl_string &path) override;
    Return<void> getBootEnv(const hidl_string &key, getBootEnv_cb _hidl_cb) override;
    Return<void> setBootEnv(const hidl_string &key, const hidl_string &value) override;
    Return<void> getDroidDisplayInfo(getDroidDisplayInfo_cb _hidl_cb) override;
    Return<void> loopMountUnmount(int32_t isMount, const hidl_string& path) override;
    Return<void> setSourceOutputMode(const hidl_string& mode) override;
    Return<void> setSinkOutputMode(const hidl_string& mode) override;
    Return<void> setDigitalMode(const hidl_string& mode) override;
    Return<void> setOsdMouseMode(const hidl_string& mode) override;
    Return<void> setOsdMousePara(int32_t x, int32_t y, int32_t w, int32_t h) override;
    Return<void> setPosition(int32_t left, int32_t top, int32_t width, int32_t height) override;
    Return<void> getPosition(const hidl_string& mode, getPosition_cb _hidl_cb) override;
    Return<void> saveDeepColorAttr(const hidl_string& mode, const hidl_string& dcValue) override;
    Return<void> getDeepColorAttr(const hidl_string &mode, getDeepColorAttr_cb _hidl_cb) override;
    Return<void> setDolbyVisionState(int32_t state) override;
    Return<void> sinkSupportDolbyVision(sinkSupportDolbyVision_cb _hidl_cb) override;
    Return<void> setHdrMode(const hidl_string& mode) override;
    Return<void> setSdrMode(const hidl_string& mode) override;
    Return<void> resolveResolutionValue(const hidl_string& mode, resolveResolutionValue_cb _hidl_cb) override;
    Return<void> setCallback(const sp<ISystemControlCallback>& callback) override;

    //for 3D
    Return<void> set3DMode(const hidl_string& mode) override;
    Return<void> init3DSetting() override;
    Return<void> getVideo3DFormat(getVideo3DFormat_cb _hidl_cb) override;
    Return<void> getDisplay3DTo2DFormat(getDisplay3DTo2DFormat_cb _hidl_cb) override;
    Return<void> setDisplay3DTo2DFormat(int32_t format) override;
    Return<void> setDisplay3DFormat(int32_t format) override;
    Return<void> getDisplay3DFormat(getDisplay3DFormat_cb _hidl_cb) override;
    Return<void> setOsd3DFormat(int32_t format) override;
    Return<void> switch3DTo2D(int32_t format) override;
    Return<void> switch2DTo3D(int32_t format) override;
    Return<void> autoDetect3DForMbox() override;

    virtual void onEvent(int event);

  private:
    void handleServiceDeath(uint32_t cookie);

    SystemControlService *mSysControl;
    std::map<uint32_t, sp<ISystemControlCallback>> mClients;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<SystemControlHal> sch);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<SystemControlHal> mSystemControlHal;
    };

    sp<DeathRecipient> mDeathRecipient;

};

}  // namespace implementation
}  // namespace V1_0
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor

#endif  // ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H
