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
#include <SystemControl.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace systemcontrol {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::systemcontrol::V1_0::ISystemControl;
using ::vendor::amlogic::hardware::systemcontrol::V1_0::Result;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct SystemControlHal : public ISystemControl {
  public:
    SystemControlHal(SystemControl * control);
    ~SystemControlHal();

    Return<void> getSupportDispModeList(getSupportDispModeList_cb _hidl_cb) override;
    Return<void> getActiveDispMode(getActiveDispMode_cb _hidl_cb) override;
    Return<Result> setActiveDispMode(const hidl_string &activeDispMode) override;
    Return<Result> isHDCPTxAuthSuccess() override;
  private:
    SystemControl *mSysControl;
};

//extern "C" ISystemControl* HIDL_FETCH_ISystemControl(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor

#endif  // ANDROID_DROIDLOGIC_SYSTEM_CONTROL_V1_0_SYSTEM_CONTROL_HAL_H