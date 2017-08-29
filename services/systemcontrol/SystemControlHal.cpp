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

#define LOG_TAG "SystemControlHal"

#include <vendor/amlogic/hardware/systemcontrol/1.0/ISystemControl.h>
#include <vendor/amlogic/hardware/systemcontrol/1.0/types.h>
#include <log/log.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <inttypes.h>
#include <string>
#include <vector>

#include <utils/KeyedVector.h>
#include <utils/String8.h>

#include "SystemControlHal.h"

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

SystemControlHal::SystemControlHal(SystemControl * control)
    : mSysControl(control) {
}

SystemControlHal::~SystemControlHal() {
}

Return<void> SystemControlHal::getSupportDispModeList(getSupportDispModeList_cb _hidl_cb) {
    std::vector<std::string> supportModes;
    mSysControl->getSupportDispModeList(&supportModes);

    hidl_vec<hidl_string> hidlList;
    hidlList.resize(supportModes.size());
    for (size_t i = 0; i < supportModes.size(); ++i) {
        hidlList[i] = supportModes[i];

        ALOGI("getSupportDispModeList index:%ld mode :%s", (unsigned long)i, supportModes[i].c_str());
    }

    _hidl_cb(Result::OK, hidlList);
    return Void();
}

Return<void> SystemControlHal::getActiveDispMode(getActiveDispMode_cb _hidl_cb) {
    std::string mode;
    mSysControl->getActiveDispMode(&mode);

    ALOGI("getActiveDispMode mode :%s", mode.c_str());
    _hidl_cb(Result::OK, mode);
    return Void();
}

Return<Result> SystemControlHal::setActiveDispMode(const hidl_string &activeDispMode) {
    std::string mode = activeDispMode;

    ALOGI("setActiveDispMode mode :%s", mode.c_str());
    mSysControl->setActiveDispMode(mode);
    return Result::OK;
}

Return<Result> SystemControlHal::isHDCPTxAuthSuccess() {
    int status;
    mSysControl->isHDCPTxAuthSuccess(status);

    ALOGI("isHDCPTxAuthSuccess status :%d", status);
    return (status==1)?Result::OK:Result::FAIL;
}

//ISystemControl* HIDL_FETCH_ISystemControl(const char* /* name */) {
//    return new SystemControlHal();
//}
}  // namespace implementation
}  // namespace V1_0
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor