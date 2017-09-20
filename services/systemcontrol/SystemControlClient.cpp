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

#define LOG_TAG "SystemControlClient"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <android-base/logging.h>

#include <SystemControlClient.h>

namespace android {

SystemControlClient::SystemControlClient() {
    mSysCtrl = ISystemControl::getService();

    mDeathRecipient = new SystemControlDeathRecipient();
    Return<bool> linked = mSysCtrl->linkToDeath(mDeathRecipient, /*cookie*/ 0);
    if (!linked.isOk()) {
        LOG(ERROR) << "Transaction error in linking to system service death: " << linked.description().c_str();
    } else if (!linked) {
        LOG(ERROR) << "Unable to link to system service death notifications";
    } else {
        LOG(INFO) << "Link to system service death notification successful";
    }
}

bool SystemControlClient::getProperty(const std::string& key, std::string& value) {
    mSysCtrl->getProperty(key, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

bool SystemControlClient::getPropertyString(const std::string& key, std::string& value, std::string& def) {
    mSysCtrl->getPropertyString(key, def, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

int32_t SystemControlClient::getPropertyInt(const std::string& key, int32_t def) {
    int32_t result;
    mSysCtrl->getPropertyInt(key, def, [&result](const Result &ret, const int32_t& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

int64_t SystemControlClient::getPropertyLong(const std::string& key, int64_t def) {
    int64_t result;
    mSysCtrl->getPropertyLong(key, def, [&result](const Result &ret, const int64_t& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

bool SystemControlClient::getPropertyBoolean(const std::string& key, bool def) {
    bool result;
    mSysCtrl->getPropertyBoolean(key, def, [&result](const Result &ret, const bool& v) {
        if (Result::OK == ret) {
            result = v;
        }
    });
    return result;
}

void SystemControlClient::setProperty(const std::string& key, const std::string& value) {
    mSysCtrl->setProperty(key, value);
}

bool SystemControlClient::readSysfs(const std::string& path, std::string& value) {
    mSysCtrl->readSysfs(path, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });

    return true;
}

bool SystemControlClient::writeSysfs(const std::string& path, const std::string& value) {
    Result rtn = mSysCtrl->writeSysfs(path, value);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::writeSysfs(const std::string& path, const char *value, const int size) {
    std::string writeValue(value, size);
    Result rtn = mSysCtrl->writeSysfs(path, writeValue);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

int32_t SystemControlClient::readHdcpRX22Key(char *value, int size) {
    std::string key;
    int32_t len;
    mSysCtrl->readHdcpRX22Key(size, [&key, &len](const Result &ret, const hidl_string& v, const int32_t& l) {
        if (Result::OK == ret) {
            key = v;
            len = l;
        }
    });

    memcpy(value, key.c_str(), len);
    return len;
}

bool SystemControlClient::writeHdcpRX22Key(const char *value, const int size) {
    std::string key(value, size);
    Result rtn = mSysCtrl->writeHdcpRX22Key(key);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

int32_t SystemControlClient::readHdcpRX14Key(char *value, int size) {
    std::string key;
    int32_t len;
    mSysCtrl->readHdcpRX14Key(size, [&key, &len](const Result &ret, const hidl_string& v, const int32_t& l) {
        if (Result::OK == ret) {
            key = v;
            len = l;
        }
    });

    memcpy(value, key.c_str(), len);
    return len;
}

bool SystemControlClient::writeHdcpRX14Key(const char *value, const int size) {
    std::string key(value, size);
    Result rtn = mSysCtrl->writeHdcpRX14Key(key);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::writeHdcpRXImg(const std::string& path) {
    Result rtn = mSysCtrl->writeHdcpRXImg(path);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

bool SystemControlClient::getBootEnv(const std::string& key, std::string& value) {
    mSysCtrl->getBootEnv(key, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return true;
}

void SystemControlClient::setBootEnv(const std::string& key, const std::string& value) {
    mSysCtrl->setBootEnv(key, value);
}

void SystemControlClient::getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
    int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
    int &fb1w, int &fb1h, int &fb1bits, int &fb1trip) {

    /*mSysCtrl->getDroidDisplayInfo([&](const Result &ret, const DroidDisplayInfo& info) {
        if (Result::OK == ret) {
            type = info.type;
            socType = info.socType;
            defaultUI = info.defaultUI;
            fb0w = info.fb0w;
            fb0h = info.fb0h;
            fb0bits = info.fb0bits;
            fb0trip = info.fb0trip;
            fb1w = info.fb1w;
            fb1h = info.fb1h;
            fb1bits = info.fb1bits;
            fb1trip = info.fb1trip;
        }
    });*/
}

void SystemControlClient::loopMountUnmount(int &isMount, const std::string& path)  {
    mSysCtrl->loopMountUnmount(isMount, path);
}

void SystemControlClient::setMboxOutputMode(const std::string& mode) {
    mSysCtrl->setSourceOutputMode(mode);
}

void SystemControlClient::setSinkOutputMode(const std::string& mode) {
    mSysCtrl->setSinkOutputMode(mode);
}

void SystemControlClient::setDigitalMode(const std::string& mode) {
    mSysCtrl->setDigitalMode(mode);
}

void SystemControlClient::setOsdMouseMode(const std::string& mode) {
    mSysCtrl->setOsdMouseMode(mode);
}

void SystemControlClient::setOsdMousePara(int x, int y, int w, int h) {
    mSysCtrl->setOsdMousePara(x, y, w, h);
}

void SystemControlClient::setPosition(int left, int top, int width, int height)  {
    mSysCtrl->setPosition(left, top, width, height);
}

void SystemControlClient::getPosition(const std::string& mode, int &outx, int &outy, int &outw, int &outh) {
    mSysCtrl->getPosition(mode, [&outx, &outy, &outw, &outh](const Result &ret,
        const int32_t& x, const int32_t& y, const int32_t& w, const int32_t& h) {
        if (Result::OK == ret) {
            outx = x;
            outy = y;
            outw = w;
            outh = h;
        }
    });
}

void SystemControlClient::saveDeepColorAttr(const std::string& mode, const std::string& dcValue) {
    mSysCtrl->saveDeepColorAttr(mode, dcValue);
}

void SystemControlClient::getDeepColorAttr(const std::string& mode, std::string& value) {
    mSysCtrl->getDeepColorAttr(mode, [&value](const Result &ret, const hidl_string& v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
}

void SystemControlClient::setDolbyVisionEnable(int state) {
    mSysCtrl->setDolbyVisionState(state);
}

bool SystemControlClient::isTvSupportDolbyVision(std::string& mode) {
    bool supported = false;
    mSysCtrl->sinkSupportDolbyVision([&mode, &supported](const Result &ret, const hidl_string& sinkMode, const bool &isSupport) {
        if (Result::OK == ret) {
            mode = sinkMode;
            supported = isSupport;
        }
    });

    return supported;
}

int64_t SystemControlClient::resolveResolutionValue(const std::string& mode) {
    int64_t value = 0;
    mSysCtrl->resolveResolutionValue(mode, [&value](const Result &ret, const int64_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

void SystemControlClient::setHdrMode(const std::string& mode) {
    mSysCtrl->setHdrMode(mode);
}

void SystemControlClient::setSdrMode(const std::string& mode) {
    mSysCtrl->setSdrMode(mode);
}

void SystemControlClient::setListener(const sp<ISystemControlCallback> callback) {
    Return<void> ret = mSysCtrl->setCallback(callback);
}

bool SystemControlClient::getSupportDispModeList(std::vector<std::string>& supportDispModes) {
    mSysCtrl->getSupportDispModeList([&supportDispModes](const Result &ret, const hidl_vec<hidl_string> list) {
        if (Result::OK == ret) {
            for (size_t i = 0; i < list.size(); i++) {
                supportDispModes.push_back(list[i]);
            }
        } else {
            supportDispModes.clear();
        }
    });

    if (supportDispModes.empty()) {
        LOG(ERROR) << "syscontrol::readEdidList FAIL.";
        return false;
    }

    return true;
}

bool SystemControlClient::getActiveDispMode(std::string& activeDispMode) {
    mSysCtrl->getActiveDispMode([&activeDispMode](const Result &ret, const hidl_string& mode) {
        if (Result::OK == ret)
            activeDispMode = mode.c_str();
        else
            activeDispMode.clear();
    });

    if (activeDispMode.empty()) {
        LOG(ERROR) << "system control client getActiveDispMode FAIL.";
        return false;
    }

    return true;
}

bool SystemControlClient::setActiveDispMode(std::string& activeDispMode) {
    Result rtn = mSysCtrl->setActiveDispMode(activeDispMode);
    if (rtn == Result::OK) {
        return true;
    }
    return false;
}

void SystemControlClient::isHDCPTxAuthSuccess(int &status) {
    Result rtn = mSysCtrl->isHDCPTxAuthSuccess();
    if (rtn == Result::OK) {
        status = 1;
    }
    else {
        status = 0;
    }
}

//3D
int32_t SystemControlClient::set3DMode(const std::string& mode3d) {
    mSysCtrl->set3DMode(mode3d);
    return 0;
}

void SystemControlClient::init3DSetting(void) {
    mSysCtrl->init3DSetting();
}

int SystemControlClient::getVideo3DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getVideo3DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

int SystemControlClient::getDisplay3DTo2DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getDisplay3DTo2DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

bool SystemControlClient::setDisplay3DTo2DFormat(int format) {
    mSysCtrl->setDisplay3DTo2DFormat(format);
    return true;
}

bool SystemControlClient::setDisplay3DFormat(int format) {
    mSysCtrl->setDisplay3DFormat(format);
    return true;
}

int SystemControlClient::getDisplay3DFormat(void) {
    int32_t value = 0;
    mSysCtrl->getDisplay3DFormat([&value](const Result &ret, const int32_t &v) {
        if (Result::OK == ret) {
            value = v;
        }
    });
    return value;
}

bool SystemControlClient::setOsd3DFormat(int format) {
    mSysCtrl->setOsd3DFormat(format);
    return true;
}

bool SystemControlClient::switch3DTo2D(int format) {
    mSysCtrl->switch3DTo2D(format);
    return true;
}

bool SystemControlClient::switch2DTo3D(int format) {
    mSysCtrl->switch2DTo3D(format);
    return true;
}

void SystemControlClient::autoDetect3DForMbox() {
    mSysCtrl->autoDetect3DForMbox();
}
//3D end

void SystemControlClient::SystemControlDeathRecipient::serviceDied(uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    LOG(ERROR) << "system control service died. need release some resources";
}

}; // namespace android
