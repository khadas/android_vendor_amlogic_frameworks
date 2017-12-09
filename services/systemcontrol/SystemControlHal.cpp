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

SystemControlHal::SystemControlHal(SystemControlService * control)
    : mSysControl(control),
    mDeathRecipient(new DeathRecipient(this)) {

    control->setListener(this);
}

SystemControlHal::~SystemControlHal() {
}

/*
event:
EVENT_OUTPUT_MODE_CHANGE            = 0,
EVENT_DIGITAL_MODE_CHANGE           = 1,
EVENT_HDMI_PLUG_OUT                 = 2,
EVENT_HDMI_PLUG_IN                  = 3,
EVENT_HDMI_AUDIO_OUT                = 4,
EVENT_HDMI_AUDIO_IN                 = 5,
*/
void SystemControlHal::onEvent(int event) {
    int clientSize = mClients.size();

    ALOGI("onEvent event:%d, client size:%d", event, clientSize);

    for (int i = 0; i < clientSize; i++) {
        if (mClients[i] != nullptr) {
            ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
            mClients[i]->notifyCallback(event);
        }
    }
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

Return<void> SystemControlHal::getProperty(const hidl_string &key, getProperty_cb _hidl_cb) {
    std::string value;
    mSysControl->getProperty(key, &value);

    ALOGI("getProperty key :%s, value:%s", key.c_str(), value.c_str());
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::getPropertyString(const hidl_string &key, const hidl_string &def, getPropertyString_cb _hidl_cb) {
    std::string value;
    mSysControl->getPropertyString(key, &value, def);

    ALOGI("getPropertyString key :%s, value:%s", key.c_str(), value.c_str());
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::getPropertyInt(const hidl_string &key, int32_t def, getPropertyInt_cb _hidl_cb) {
    int32_t value = mSysControl->getPropertyInt(key, def);

    ALOGI("getPropertyInt key :%s, value:%d", key.c_str(), value);
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::getPropertyLong(const hidl_string &key, int64_t def, getPropertyLong_cb _hidl_cb) {
    int64_t value = mSysControl->getPropertyLong(key, def);

    ALOGI("getPropertyLong key :%s, value:%ld", key.c_str(), value);
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::getPropertyBoolean(const hidl_string &key, bool def, getPropertyBoolean_cb _hidl_cb) {
    bool value = mSysControl->getPropertyBoolean(key, def);

    ALOGI("getPropertyBoolean key :%s, value:%d", key.c_str(), value);
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<Result> SystemControlHal::setProperty(const hidl_string &key, const hidl_string &value) {
    mSysControl->setProperty(key, value);

    ALOGI("setProperty key :%s, value:%s", key.c_str(), value.c_str());
    return Result::OK;
}

Return<void> SystemControlHal::readSysfs(const hidl_string &path, readSysfs_cb _hidl_cb) {
    std::string value;
    mSysControl->readSysfs(path, value);

    ALOGI("readSysfs path :%s, value:%s", path.c_str(), value.c_str());
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<Result> SystemControlHal::writeSysfs(const hidl_string &path, const hidl_string &value) {
   ALOGI("writeSysfs path :%s, value:%s", path.c_str(), value.c_str());
    return mSysControl->writeSysfs(path, value)?Result::OK:Result::FAIL;
}

Return<void> SystemControlHal::readHdcpRX22Key(int32_t size, readHdcpRX22Key_cb _hidl_cb) {
    char *value = (char *)malloc(size);
    int len = mSysControl->readHdcpRX22Key(value, size);

    std::string valueStr(value, len);

    ALOGI("readHdcpRX22Key size :%d, value:%s", size, value);
    _hidl_cb(Result::OK, valueStr, len);
    free(value);
    return Void();
}

Return<Result> SystemControlHal::writeHdcpRX22Key(const hidl_string &key) {
    ALOGI("writeHdcpRX22Key key:%s", key.c_str());
    return mSysControl->writeHdcpRX22Key(key.c_str(), key.size())?Result::OK:Result::FAIL;
}

Return<void> SystemControlHal::readHdcpRX14Key(int32_t size, readHdcpRX14Key_cb _hidl_cb) {
    char *value = (char *)malloc(size);
    int len = mSysControl->readHdcpRX14Key(value, size);

    std::string valueStr(value, len);

    ALOGI("readHdcpRX14Key size :%d, value:%s", size, value);
    _hidl_cb(Result::OK, valueStr, len);
    free(value);
    return Void();
}

Return<Result> SystemControlHal::writeHdcpRX14Key(const hidl_string &key) {
    ALOGI("writeHdcpRX14Key key:%s", key.c_str());
    return mSysControl->writeHdcpRX14Key(key.c_str(), key.size())?Result::OK:Result::FAIL;
}

Return<Result> SystemControlHal::writeHdcpRXImg(const hidl_string &path) {
    ALOGI("writeHdcpRXImg path:%s", path.c_str());
    return mSysControl->writeHdcpRXImg(path)?Result::OK:Result::FAIL;
}

Return<void> SystemControlHal::getBootEnv(const hidl_string &key, getBootEnv_cb _hidl_cb) {
    std::string value;
    mSysControl->getBootEnv(key, value);

    ALOGI("getBootEnv key :%s, value:%s", key.c_str(), value.c_str());
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::setBootEnv(const hidl_string &key, const hidl_string &value) {
    mSysControl->setBootEnv(key, value);

    ALOGI("setBootEnv key :%s, value:%s", key.c_str(), value.c_str());
    return Void();
}

Return<void> SystemControlHal::getDroidDisplayInfo(getDroidDisplayInfo_cb _hidl_cb) {
    DroidDisplayInfo info;

    /*std::string type;
    std::string ui;
    mSysControl->getDroidDisplayInfo(info.type, type, ui,
        info.fb0w, info.fb0h, info.fb0bits, info.fb0trip,
        info.fb1w, info.fb1h, info.fb1bits, info.fb1trip);

    info.socType = type;
    info.defaultUI = ui;*/
    _hidl_cb(Result::OK, info);
    return Void();
}

Return<void> SystemControlHal::loopMountUnmount(int32_t isMount, const hidl_string& path) {
    ALOGI("loopMountUnmount isMount :%d, path:%s", isMount, path.c_str());
    mSysControl->loopMountUnmount(isMount, path);
    return Void();
}

Return<void> SystemControlHal::setSourceOutputMode(const hidl_string& mode) {
    ALOGI("setSourceOutputMode mode:%s", mode.c_str());
    mSysControl->setSourceOutputMode(mode);
    return Void();
}

Return<void> SystemControlHal::setSinkOutputMode(const hidl_string& mode) {
    ALOGI("setSinkOutputMode mode:%s", mode.c_str());
    mSysControl->setSinkOutputMode(mode);
    return Void();
}

Return<void> SystemControlHal::setDigitalMode(const hidl_string& mode) {
    ALOGI("setDigitalMode mode:%s", mode.c_str());
    mSysControl->setDigitalMode(mode);
    return Void();
}

Return<void> SystemControlHal::setOsdMouseMode(const hidl_string& mode) {
    ALOGI("setOsdMouseMode mode:%s", mode.c_str());
    mSysControl->setOsdMouseMode(mode);
    return Void();
}

Return<void> SystemControlHal::setOsdMousePara(int32_t x, int32_t y, int32_t w, int32_t h) {
    mSysControl->setOsdMousePara(x, y, w, h);
    return Void();
}

Return<void> SystemControlHal::setPosition(int32_t left, int32_t top, int32_t width, int32_t height) {
    mSysControl->setPosition(left, top, width, height);
    return Void();
}

Return<void> SystemControlHal::getPosition(const hidl_string& mode, getPosition_cb _hidl_cb) {
    int x, y, w, h;
    mSysControl->getPosition(mode, x, y, w, h);
    _hidl_cb(Result::OK, x, y, w, h);
    return Void();
}

Return<void> SystemControlHal::saveDeepColorAttr(const hidl_string& mode, const hidl_string& dcValue) {
    mSysControl->saveDeepColorAttr(mode, dcValue);
    return Void();
}

Return<void> SystemControlHal::getDeepColorAttr(const hidl_string &mode, getDeepColorAttr_cb _hidl_cb) {
    std::string value;
    mSysControl->getDeepColorAttr(mode, value);

    ALOGI("getDeepColorAttr mode :%s, value:%s", mode.c_str(), value.c_str());
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::setDolbyVisionState(int32_t state) {
    mSysControl->setDolbyVisionEnable(state);
    return Void();
}

Return<void> SystemControlHal::sinkSupportDolbyVision(sinkSupportDolbyVision_cb _hidl_cb) {
    std::string mode;
    bool support = mSysControl->isTvSupportDolbyVision(mode);

    ALOGI("getDeepColorAttr mode :%s, dv support:%d", mode.c_str(), support);
    _hidl_cb(Result::OK, mode, support);
    return Void();
}

Return<void> SystemControlHal::setHdrMode(const hidl_string& mode) {
    ALOGI("setHdrMode mode:%s", mode.c_str());
    mSysControl->setHdrMode(mode);
    return Void();
}

Return<void> SystemControlHal::setSdrMode(const hidl_string& mode) {
    ALOGI("setSdrMode mode:%s", mode.c_str());
    mSysControl->setSdrMode(mode);
    return Void();
}

Return<void> SystemControlHal::resolveResolutionValue(const hidl_string& mode, resolveResolutionValue_cb _hidl_cb) {
    int64_t value = mSysControl->resolveResolutionValue(mode);
    _hidl_cb(Result::OK, value);
    return Void();
}

Return<void> SystemControlHal::setCallback(const sp<ISystemControlCallback>& callback) {
    if (callback != nullptr) {
        int cookie = -1;
        int clientSize = mClients.size();
        for (int i = 0; i < clientSize; i++) {
            if (mClients[i] == nullptr) {
                ALOGI("%s, client index:%d had died, this id give the new client", __FUNCTION__, i);
                cookie = i;
                mClients[i] = callback;
                break;
            }
        }

        if (cookie < 0) {
            cookie = clientSize;
            mClients[clientSize] = callback;
        }

        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, cookie);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            ALOGW("Couldn't link death recipient for cookie: %d", cookie);
        }

        ALOGI("%s cookie:%d, client size:%d", __FUNCTION__, cookie, (int)mClients.size());
    }

    return Void();
}

//for 3D
Return<void> SystemControlHal::set3DMode(const hidl_string& mode) {
    ALOGI("set3DMode mode:%s", mode.c_str());
    mSysControl->set3DMode(mode);
    return Void();
}

Return<void> SystemControlHal::init3DSetting() {
    mSysControl->init3DSetting();
    return Void();
}

Return<void> SystemControlHal::getVideo3DFormat(getVideo3DFormat_cb _hidl_cb) {
    int32_t format = mSysControl->getVideo3DFormat();
    _hidl_cb(Result::OK, format);
    return Void();
}

Return<void> SystemControlHal::getDisplay3DTo2DFormat(getDisplay3DTo2DFormat_cb _hidl_cb) {
    int32_t format = mSysControl->getDisplay3DTo2DFormat();
    _hidl_cb(Result::OK, format);
    return Void();
}

Return<void> SystemControlHal::setDisplay3DTo2DFormat(int32_t format) {
    mSysControl->setDisplay3DTo2DFormat(format);
    return Void();
}

Return<void> SystemControlHal::setDisplay3DFormat(int32_t format) {
    mSysControl->setDisplay3DFormat(format);
    return Void();
}

Return<void> SystemControlHal::getDisplay3DFormat(getDisplay3DFormat_cb _hidl_cb) {
    int32_t format = mSysControl->getDisplay3DFormat();
    _hidl_cb(Result::OK, format);
    return Void();
}

Return<void> SystemControlHal::setOsd3DFormat(int32_t format) {
    mSysControl->setOsd3DFormat(format);
    return Void();
}

Return<void> SystemControlHal::switch3DTo2D(int32_t format) {
    mSysControl->switch3DTo2D(format);
    return Void();
}

Return<void> SystemControlHal::switch2DTo3D(int32_t format) {
    mSysControl->switch2DTo3D(format);
    return Void();
}

Return<void> SystemControlHal::autoDetect3DForMbox() {
    mSysControl->autoDetect3DForMbox();
    return Void();
}
//3D end

void SystemControlHal::handleServiceDeath(uint32_t cookie) {
    mClients[cookie]->unlinkToDeath(mDeathRecipient);
    mClients[cookie].clear();
}

SystemControlHal::DeathRecipient::DeathRecipient(sp<SystemControlHal> sch)
        : mSystemControlHal(sch) {}

void SystemControlHal::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("systemcontrol daemon client died cookie:%d", (int)cookie);

    uint32_t type = static_cast<uint32_t>(cookie);
    mSystemControlHal->handleServiceDeath(type);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace systemcontrol
}  // namespace hardware
}  // namespace android
}  // namespace vendor
