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

#define LOG_TAG "SystemControl"
#define LOG_NDEBUG 0

#include <fcntl.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <private/android_filesystem_config.h>
#include <pthread.h>

#include "SystemControlService.h"

#define VIDEO_RGB_SCREEN    "/sys/class/video/rgb_screen"
#define SSC_PATH            "/sys/class/lcd/ss"
#define TEST_SCREEN         "/sys/class/video/test_screen"

namespace android {

SystemControlService* SystemControlService::instantiate(const char *cfgpath) {
    SystemControlService *sysControlIntf = new SystemControlService(cfgpath);
    return sysControlIntf;
}

SystemControlService::SystemControlService(const char *path)
    : mLogLevel(LOG_LEVEL_DEFAULT) {

    ALOGI("SystemControlService instantiate begin");
    pUbootenv = new Ubootenv();
    pSysWrite = new SysWrite();

    pDisplayMode = new DisplayMode(path, pUbootenv);
    pDisplayMode->init();

    mSSMAction = SSMAction::getInstance();

    //load PQ
    pCPQControl = CPQControl::GetInstance();

    pDimension = new Dimension(pDisplayMode, pSysWrite);

    //if ro.firstboot is true, we should clear first boot flag
    const char* firstBoot = pUbootenv->getValue("ubootenv.var.firstboot");
    if (firstBoot && (strcmp(firstBoot, "1") == 0)) {
        ALOGI("ubootenv.var.firstboot first_boot:%s, clear it to 0", firstBoot);
        if ( pUbootenv->updateValue("ubootenv.var.firstboot", "0") < 0 )
            ALOGE("set firstboot to 0 fail");
    }

    ALOGI("SystemControlService instantiate done");
}

SystemControlService::~SystemControlService() {
    delete pUbootenv;
    delete pSysWrite;
    delete pDisplayMode;
    delete pDimension;

    if (mSSMAction != NULL) {
        delete mSSMAction;
        mSSMAction = NULL;
    }

    if (pCPQControl != NULL) {
        delete pCPQControl;
        pCPQControl = NULL;
    }
}

int SystemControlService::permissionCheck() {
    /*
    // codes that require permission check
    IPCThreadState* ipc = IPCThreadState::self();
    const int pid = ipc->getCallingPid();
    const int uid = ipc->getCallingUid();

    if ((uid != AID_GRAPHICS) && (uid != AID_MEDIA) &&( uid != AID_MEDIA_CODEC) &&
            !PermissionCache::checkPermission(String16("droidlogic.permission.SYSTEM_CONTROL"), pid, uid)) {
        ALOGE("Permission Denial: "
                "can't use system control service pid=%d, uid=%d", pid, uid);
        return PERMISSION_DENIED;
    }

    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("system_control service permissionCheck pid=%d, uid=%d", pid, uid);
    }
    */

    return NO_ERROR;
}

bool SystemControlService::getSupportDispModeList(std::vector<std::string> *supportDispModes) {
    const char *delim = "\n";
    char value[MODE_LEN] = {0};
    hdmi_data_t data;

    pDisplayMode->getHdmiData(&data);
    char *ptr = strtok(data.edid, delim);
    while (ptr != NULL) {
        int len = strlen(ptr);
        if (ptr[len - 1] == '*')
            ptr[len - 1] = '\0';

        (*supportDispModes).push_back(std::string(ptr));
        ptr = strtok(NULL, delim);
    }

    return true;
}

bool SystemControlService::getActiveDispMode(std::string *activeDispMode) {
    char mode[MODE_LEN] = {0};
    bool ret = pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, mode);
    *activeDispMode = mode;
    return ret;
}

bool SystemControlService::setActiveDispMode(std::string& activeDispMode) {
    setSourceOutputMode(activeDispMode.c_str());
    return true;
}

//read write property and sysfs
bool SystemControlService::getProperty(const std::string &key, std::string *value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getProperty(key.c_str(), buf);
    *value = buf;
    return ret;
}

bool SystemControlService::getPropertyString(const std::string &key, std::string *value, const std::string &def) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    bool ret = pSysWrite->getPropertyString(key.c_str(), (char *)buf, def.c_str());
    *value = buf;
    return ret;
}

int32_t SystemControlService::getPropertyInt(const std::string &key, int32_t def) {
    return pSysWrite->getPropertyInt(key.c_str(), def);
}

int64_t SystemControlService::getPropertyLong(const std::string &key, int64_t def) {
    return pSysWrite->getPropertyLong(key.c_str(), def);
}

bool SystemControlService::getPropertyBoolean(const std::string& key, bool def) {
    return pSysWrite->getPropertyBoolean(key.c_str(), def);
}

void SystemControlService::setProperty(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        pSysWrite->setProperty(key.c_str(), value.c_str());
        traceValue("setProperty", key, value);
    }
}

bool SystemControlService::readSysfs(const std::string& path, std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readSysfs(path.c_str(), buf);
        value = buf;

        traceValue("readSysfs", path, value);
        return ret;
    }

    return false;
}

bool SystemControlService::writeSysfs(const std::string& path, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, value);

        return pSysWrite->writeSysfs(path.c_str(), value.c_str());
    }

    return false;
}

bool SystemControlService::writeSysfs(const std::string& path, const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, size);

        bool ret = pSysWrite->writeSysfs(path.c_str(), value, size);
        return ret;
    }

    return false;
}

bool SystemControlService::writeUnifyKey(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeUnifyKey", key, value);

        return pSysWrite->writeUnifyKey(key.c_str(), value.c_str());
    }

    return false;
}

bool SystemControlService::readUnifyKey(const std::string& key, std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        char buf[MAX_STR_LEN] = {0};
        bool ret = pSysWrite->readUnifyKey(key.c_str(), buf);
        value = buf;

        traceValue("readUnifyKey", key, value);
        return ret;
    }

    return false;
}

bool SystemControlService::writePlayreadyKey(const std::string& key, const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writePlayreadyKey", key, size);

        return pSysWrite->writePlayreadyKey(key.c_str(), value, size);
    }

    return false;
}

int32_t SystemControlService::readPlayreadyKey(const std::string& key, char *value, int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("readPlayreadyKey", key, size);
        int i;
        int len = pSysWrite->readPlayreadyKey(key.c_str(), value, size);

        return len;
    }

    return 0;
}


int32_t SystemControlService::readAttestationKey(const std::string& node, const std::string& name, char *value, int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("readAttestationKey", node, name);
        int i;
        int len= pSysWrite->readAttestationKey(node.c_str(), name.c_str(), value, size);

        ALOGE("come to SystemControlService::writeAttestationKey  size: %d  len = %d \n", size, len);
        for (i = 0; i < 32; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, value[i], value[i], value[i]);
        }

        for (i = 8960; i < 8992; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, value[i], value[i], value[i]);
        }

        return len;
    }
    return 0;
}

bool SystemControlService::writeAttestationKey(const std::string& node, const std::string& name, const char *buff, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeAttestationKey", node, name);
        int i;
        ALOGE("come to SystemControlService::writeAttestationKey  size: %d\n", size);
        for (i = 0; i < 32; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        for (i = 8960; i < 8992; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        for (i = 10000; i < 10016; ++i) {
            ALOGE("%d: %02x    %08x   %x", i, buff[i], buff[i], buff[i]);
        }

        bool ret = pSysWrite->writeAttestationKey(node.c_str(), name.c_str(), buff, size);
        return ret;
    }
    return false;
}

int32_t SystemControlService::readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue("readHdcpRX14Key", "", size);
        int len = pDisplayMode->readHdcpRX22Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControlService::writeHdcpRX22Key(const char *value, const int size) {
   if (NO_ERROR == permissionCheck()) {
        traceValue("writeHdcp22Key", "", size);

        bool ret = pDisplayMode->writeHdcpRX22Key(value, size);
        return ret;
    }
    return false;
}

int32_t SystemControlService::readHdcpRX14Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue("readHdcpRX14Key", "", size);
        int len = pDisplayMode->readHdcpRX14Key(value, size);
        return len;
    }*/
    return 0;
}

bool SystemControlService::writeHdcpRX14Key(const char *value, const int size) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeHdcp14Key", "", size);

        bool ret = pDisplayMode->writeHdcpRX14Key(value, size);
        return ret;
    }
    return false;
}

bool SystemControlService::writeHdcpRXImg(const std::string& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("writeSysfs", path, "");

        return pDisplayMode->writeHdcpRXImg(path.c_str());
    }

    return false;
}

//set or get uboot env
bool SystemControlService::getBootEnv(const std::string& key, std::string& value) {
    const char* p_value = pUbootenv->getValue(key.c_str());
	if (p_value) {
        value = p_value;
        return true;
	}
    return false;
}

void SystemControlService::setBootEnv(const std::string& key, const std::string& value) {
    if (NO_ERROR == permissionCheck()) {
        pUbootenv->updateValue(key.c_str(), value.c_str());
        traceValue("setBootEnv", key, value);
    }
}

void SystemControlService::getDroidDisplayInfo(int &type __unused, std::string& socType __unused, std::string& defaultUI __unused,
        int &fb0w __unused, int &fb0h __unused, int &fb0bits __unused, int &fb0trip __unused,
        int &fb1w __unused, int &fb1h __unused, int &fb1bits __unused, int &fb1trip __unused) {
    if (NO_ERROR == permissionCheck()) {
       /* char bufType[MAX_STR_LEN] = {0};
        char bufUI[MAX_STR_LEN] = {0};
        pDisplayMode->getDisplayInfo(type, bufType, bufUI);
        socType = bufType;
        defaultUI = bufUI;
        pDisplayMode->getFbInfo(fb0w, fb0h, fb0bits, fb0trip, fb1w, fb1h, fb1bits, fb1trip);*/
    }
}

void SystemControlService::DroidVoldDeathRecipient::serviceDied(uint64_t cookie,
        const ::android::wp<::android::hidl::base::V1_0::IBase>& who) {
    //LOG(ERROR) << "DroidVold service died. need release some resources";
}

void SystemControlService::loopMountUnmount(int isMount, const std::string& path) {
    if (NO_ERROR == permissionCheck()) {
        traceValue("loopMountUnmount", (isMount==1)?"mount":"unmount", path);

        /*if (isMount == 1) {
            char mountPath[MAX_STR_LEN] = {0};

            strncpy(mountPath, path.c_str(), path.size());

            const char *cmd[4] = {"vdc", "loop", "mount", mountPath};
            vdc_loop(4, (char **)cmd);
        } else {
            const char *cmd[3] = {"vdc", "loop", "unmount"};
            vdc_loop(3, (char **)cmd);
        }*/

        mDroidVold = IDroidVold::getService();
        mDeathRecipient = new DroidVoldDeathRecipient();
        Return<bool> linked = mDroidVold->linkToDeath(mDeathRecipient, 0);
        if (!linked.isOk()) {
            //LOG(ERROR) << "Transaction error in linking to system service death: " << linked.description().c_str();
        } else if (!linked) {
            //LOG(ERROR) << "Unable to link to system service death notifications";
        } else {
            //LOG(INFO) << "Link to system service death notification successful";
        }

        if (isMount == 1) {
            mDroidVold->mount(path, 0xF, 0);
        }
        else {
            mDroidVold->unmount(path);
        }
    }
}

void SystemControlService::setSourceOutputMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set output mode :%s", mode.c_str());
    }

    pDisplayMode->setSourceOutputMode(mode.c_str());
}

void SystemControlService::setSinkOutputMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set sink output mode :%s", mode.c_str());
    }

    pDisplayMode->setSinkOutputMode(mode.c_str());
}

void SystemControlService::setDigitalMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set Digital mode :%s", mode.c_str());
    }

    pDisplayMode->setDigitalMode(mode.c_str());
}

void SystemControlService::setOsdMouseMode(const std::string& mode) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse mode :%s", mode.c_str());
    }

}

void SystemControlService::setOsdMousePara(int x, int y, int w, int h) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set osd mouse parameter x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}

void SystemControlService::setPosition(int left, int top, int width, int height) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set position x:%d y:%d w:%d h:%d", left, top, width, height);
    }
    pDisplayMode->setPosition(left, top, width, height);
}

void SystemControlService::getPosition(const std::string& mode, int &x, int &y, int &w, int &h) {
    int position[4] = { 0, 0, 0, 0 };
    pDisplayMode->getPosition(mode.c_str(), position);
    x = position[0];
    y = position[1];
    w = position[2];
    h = position[3];
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get position x:%d y:%d w:%d h:%d", x, y, w, h);
    }
}

void SystemControlService::setDolbyVisionEnable(int state) {
    pDisplayMode->setDolbyVisionEnable(state);
}

bool SystemControlService::isTvSupportDolbyVision(std::string& mode) {
    char value[MAX_STR_LEN] = {0};
    bool ret = pDisplayMode->isTvSupportDolbyVision(value);
    mode = value;
    return ret;
}

int32_t SystemControlService::getDolbyVisionType() {
    return pDisplayMode->getDolbyVisionType();
}

void SystemControlService::setGraphicsPriority(const std::string& mode) {
    char value[MODE_LEN];
    strcpy(value, mode.c_str());
    pDisplayMode->setGraphicsPriority(value);
}

void SystemControlService::getGraphicsPriority(std::string& mode) {
    char value[MODE_LEN] = {0};
    pDisplayMode->getGraphicsPriority(value);
    mode = value;
}

void SystemControlService::isHDCPTxAuthSuccess(int &status) {
    int value=0;
    pDisplayMode->isHDCPTxAuthSuccess(&value);
    status = value;
}

void SystemControlService::saveDeepColorAttr(const std::string& mode, const std::string& dcValue) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set deep color attr %s\n", dcValue.c_str());
    }
    char outputmode[64];
    char value[64];
    strcpy(outputmode, mode.c_str());
    strcpy(value, dcValue.c_str());
    pDisplayMode->saveDeepColorAttr(outputmode, value);
}

void SystemControlService::getDeepColorAttr(const std::string& mode, std::string& value) {
    char buf[PROPERTY_VALUE_MAX] = {0};
    pDisplayMode->getDeepColorAttr(mode.c_str(), buf);
    value = buf;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("get deep color attr mode %s, value %s ", mode.c_str(), value.c_str());
    }
}

void SystemControlService::setHdrMode(const std::string& mode) {
    pDisplayMode->setHdrMode(mode.c_str());
}

void SystemControlService::setSdrMode(const std::string& mode) {
    pDisplayMode->setSdrMode(mode.c_str());
}

int64_t SystemControlService::resolveResolutionValue(const std::string& mode) {
    int64_t value = pDisplayMode->resolveResolutionValue(mode.c_str());
    return value;
}

void SystemControlService::setListener(const sp<SystemControlNotify>& listener) {
    pDisplayMode->setListener(listener);
}

//3D
int32_t SystemControlService::set3DMode(const std::string& mode3d) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("set 3d mode :%s", mode3d.c_str());
    }

    return pDimension->set3DMode(mode3d.c_str());
}

void SystemControlService::init3DSetting(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("init3DSetting\n");
    }

    pDimension->init3DSetting();
}

int32_t SystemControlService::getVideo3DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getVideo3DFormat\n");
    }

    return pDimension->getVideo3DFormat();
}

int32_t SystemControlService::getDisplay3DTo2DFormat(void) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("getDisplay3DTo2DFormat\n");
    }

    return pDimension->getDisplay3DTo2DFormat();
}

bool SystemControlService::setDisplay3DTo2DFormat(int format) {
    bool ret = false;
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DTo2DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DTo2DFormat(format);
}

bool SystemControlService::setDisplay3DFormat(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setDisplay3DFormat format:%d\n", format);
    }

    return pDimension->setDisplay3DFormat(format);
}

int32_t SystemControlService::getDisplay3DFormat(void) {
    return pDimension->getDisplay3DFormat();
}

bool SystemControlService::setOsd3DFormat(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("setOsd3DFormat format:%d\n", format);
    }

    return pDimension->setOsd3DFormat(format);
}

bool SystemControlService::switch3DTo2D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch3DTo2D format:%d\n", format);
    }

    return pDimension->switch3DTo2D(format);
}

bool SystemControlService::switch2DTo3D(int format) {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("switch2DTo3D format:%d\n", format);
    }

    return pDimension->switch2DTo3D(format);
}

void SystemControlService::autoDetect3DForMbox() {
    if (mLogLevel > LOG_LEVEL_1) {
        ALOGI("autoDetect3DForMbox\n");
    }

    pDimension->autoDetect3DForMbox();
}
//3D end

//PQ
int SystemControlService::loadPQSettings(source_input_param_t source_input_param)
{
    return pCPQControl->LoadPQSettings(source_input_param);
}

int SystemControlService::loadCpqLdimRegs(void)
{
    return pCPQControl->LoadCpqLdimRegs();
}

int SystemControlService::sysSSMReadNTypes(int id, int data_len, int offset)
{
    return pCPQControl->Cpq_SSMReadNTypes(id, data_len, offset);
}

int SystemControlService::sysSSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    return pCPQControl->Cpq_SSMWriteNTypes(id, data_len, data_buf, offset);
}

int SystemControlService::getActualAddr(int id)
{
    return pCPQControl->Cpq_GetSSMActualAddr(id);
}

int SystemControlService::getActualSize(int id)
{
    return pCPQControl->Cpq_GetSSMActualSize(id);
}

int SystemControlService::getSSMStatus(void)
{
    return pCPQControl->Cpq_GetSSMStatus();
}

int SystemControlService::setPQmode(int pq_mode, int is_save, int is_autoswitch)
{
    return pCPQControl->SetPQMode(pq_mode, is_save, is_autoswitch);
}

int SystemControlService::getPQmode(void)
{
    return (int)pCPQControl->GetPQMode();
}

int SystemControlService::savePQmode(int pq_mode)
{
    return pCPQControl->SavePQMode(pq_mode);
}

int SystemControlService::setColorTemperature(int temp_mode, int is_save)
{
    return pCPQControl->SetColorTemperature(temp_mode, is_save);
}

int SystemControlService::getColorTemperature(void)
{
    return pCPQControl->GetColorTemperature();
}

int SystemControlService::saveColorTemperature(int temp_mode)
{
    return pCPQControl->SaveColorTemperature(temp_mode);
}

int SystemControlService::setColorTemperatureParam(int Tempmode, tcon_rgb_ogo_t params)
{
    return pCPQControl->SetColorTemperatureParams((vpp_color_temperature_mode_t)Tempmode, params);
}

int SystemControlService::getColorTemperatureParam(int Tempmode, int id)
{
    tcon_rgb_ogo_t params;
    pCPQControl->GetColorTemperatureParams((vpp_color_temperature_mode_t)Tempmode, &params);

    switch ((pq_color_param_t)id) {
        case EN: {
            return params.en;
        }
        case POST_OFFSET_R: {
            return params.r_post_offset;
        }
        case POST_OFFSET_G: {
            return params.g_post_offset;
        }
        case POST_OFFSET_B: {
            return params.b_post_offset;
        }
        case GAIN_R: {
            return params.r_gain;
        }
        case GAIN_G: {
            return params.g_gain;
        }
        case GAIN_B: {
            return params.b_gain;
        }
        case PRE_OFFSET_R: {
            return params.r_post_offset;
        }
        case PRE_OFFSET_G: {
            return params.g_post_offset;
        }
        case PRE_OFFSET_B: {
            return params.b_post_offset;
        }
        default: {
            return -1;
        }

    }
}

int SystemControlService::saveColorTemperatureParam(int Tempmode, tcon_rgb_ogo_t params)
{
    return pCPQControl->SaveColorTemperatureParams((vpp_color_temperature_mode_t)Tempmode, params);
}

int SystemControlService::setBrightness(int value, int is_save)
{
    return pCPQControl->SetBrightness(value, is_save);
}

int SystemControlService::getBrightness(void)
{
    return pCPQControl->GetBrightness();
}

int SystemControlService::saveBrightness(int value)
{
    return pCPQControl->SaveBrightness(value);
}

int SystemControlService::setContrast(int value, int is_save)
{
    return pCPQControl->SetContrast(value, is_save);
}

int SystemControlService::getContrast(void)
{
    return pCPQControl->GetContrast();
}

int SystemControlService::saveContrast(int value)
{
    return pCPQControl->SaveContrast(value);
}

int SystemControlService::setSaturation(int value, int is_save)
{
    return pCPQControl->SetSaturation(value, is_save);
}

int SystemControlService::getSaturation(void)
{
    return pCPQControl->GetSaturation();
}

int SystemControlService::saveSaturation(int value)
{
    return pCPQControl->SaveSaturation(value);
}

int SystemControlService::setHue(int value, int is_save )
{
    return pCPQControl->SetHue(value, is_save);
}

int SystemControlService::getHue(void)
{
    return pCPQControl->GetHue();
}

int SystemControlService::saveHue(int value)
{
    return pCPQControl->SaveHue(value);
}

int SystemControlService::setSharpness(int value, int is_enable, int is_save)
{
    return pCPQControl->SetSharpness(value, is_enable, is_save);
}

int SystemControlService::getSharpness(void)
{
    return pCPQControl->GetSharpness();
}

int SystemControlService::saveSharpness(int value)
{
    return pCPQControl->SaveSharpness(value);
}

int SystemControlService::setNoiseReductionMode(int nr_mode, int is_save)
{
    return pCPQControl->SetNoiseReductionMode(nr_mode, is_save);
}

int SystemControlService::getNoiseReductionMode(void)
{
    return (int)pCPQControl->GetNoiseReductionMode();
}

int SystemControlService::saveNoiseReductionMode(int nr_mode)
{
    return pCPQControl->SaveNoiseReductionMode(nr_mode);
}

int SystemControlService::setEyeProtectionMode(int source_input, int enable, int isSave)
{
    return pCPQControl->SetEyeProtectionMode((tv_source_input_t)source_input, enable, isSave);
}

int SystemControlService::getEyeProtectionMode(int source_input)
{
    return pCPQControl->GetEyeProtectionMode((tv_source_input_t)source_input);
}

int SystemControlService::setGammaValue(int gamma_curve, int is_save)
{
    return pCPQControl->SetGammaValue((vpp_gamma_curve_t)gamma_curve, is_save);
}

int SystemControlService::getGammaValue()
{
    return pCPQControl->GetGammaValue();
}

int SystemControlService::setDisplayMode(int source_input, int mode, int isSave)
{
    return pCPQControl->SetDisplayMode((tv_source_input_t)source_input, (vpp_display_mode_t)mode, isSave);
}

int SystemControlService::getDisplayMode(int source_input)
{
    return pCPQControl->GetDisplayMode((tv_source_input_t)source_input);
}

int SystemControlService::saveDisplayMode(int source_input, int mode)
{
    return pCPQControl->SaveDisplayMode((tv_source_input_t)source_input, (vpp_display_mode_t)mode);
}

int SystemControlService::setBacklight(int source_input, int value, int isSave)
{
    return pCPQControl->SetBacklight((tv_source_input_t)source_input, value, isSave);
}

int SystemControlService::getBacklight(int source_input)
{
    return pCPQControl->GetBacklight((tv_source_input_t)source_input);
}

int SystemControlService::saveBacklight(int source_input, int value)
{
    return pCPQControl->SaveBacklight((tv_source_input_t)source_input, value);
}

int SystemControlService::factoryResetPQMode(void)
{
    return pCPQControl->FactoryResetPQMode();
}

int SystemControlService::factoryResetColorTemp(void)
{
    return pCPQControl->FactoryResetColorTemp();
}

int SystemControlService::factorySetPQParam(source_input_param_t source_input_param, int pq_mode, int id, int value)
{
    return pCPQControl->FactorySetPQParam(source_input_param, pq_mode, id, value);
}

int SystemControlService::factoryGetPQParam(source_input_param_t source_input_param, int pq_mode, int id)
{
    return pCPQControl->FactoryGetPQParam(source_input_param, pq_mode, id);
}

int SystemControlService::factorySetColorTemperatureParam(int colortemperature_mode, int id, int value)
{
    return pCPQControl->FactorySetColorTemperatureParam(colortemperature_mode, (pq_color_param_t)id, value);
}

int SystemControlService::factoryGetColorTemperatureParam(int colortemperature_mode, int id)
{
    return pCPQControl->FactoryGetColorTemperatureParam(colortemperature_mode, (pq_color_param_t)id);
}

int SystemControlService::factorySaveColorTemperatureParam(int colortemperature_mode, int id, int value)
{
    return pCPQControl->FactorySaveColorTemperatureParam(colortemperature_mode, (pq_color_param_t)id, value);
}

int SystemControlService::factorySetOverscan(source_input_param_t source_input_param, int he_value, int hs_value,
                                             int ve_value, int vs_value)
{
    tvin_cutwin_t cutwin_t;
    cutwin_t.he = he_value;
    cutwin_t.hs = hs_value;
    cutwin_t.ve = ve_value;
    cutwin_t.vs = vs_value;

    return pCPQControl->FactorySetOverscanParam(source_input_param, cutwin_t);
}

int SystemControlService::factoryGetOverscan(source_input_param_t source_input_param, int id)
{
    return pCPQControl->FactoryGetOverscanParam(source_input_param, (tvin_cutwin_param_t)id);
}

int SystemControlService::factorySetNolineParams(source_input_param_t source_input_param, int type, int osd0_value, int osd25_value,
                                                int osd50_value, int osd75_value, int osd100_value __unused)
{
    noline_params_t noline_params;
    noline_params.osd0   = osd0_value;
    noline_params.osd25  = osd25_value;
    noline_params.osd50  = osd50_value;
    noline_params.osd75  = osd75_value;
    noline_params.osd100 = osd50_value;

    return pCPQControl->FactorySetNolineParams(source_input_param, type, noline_params);
}

int SystemControlService::factoryGetNolineParams(source_input_param_t source_input_param, int type, int id)
{
    return pCPQControl->FactoryGetNolineParams(source_input_param, type, (noline_params_ID_t)id);
}

int SystemControlService::factorySetParamsDefault()
{
    return pCPQControl->FactorySetParamsDefault();
}

int SystemControlService::factorySSMRestore(void)
{
    return pCPQControl->FcatorySSMRestore();
}

int SystemControlService::factoryResetNonlinear(void)
{
    return pCPQControl->FactoryResetNonlinear();
}

int SystemControlService::factorySetGamma(int gamma_r, int gamma_g, int gamma_b)
{
    return pCPQControl->FactorySetGamma(gamma_r, gamma_g, gamma_b);
}

int SystemControlService::SSMRecovery(void)
{
    return pCPQControl->Cpq_SSMRecovery();
}

int SystemControlService::setPLLValues(source_input_param_t source_input_param)
{
    return pCPQControl->SetPLLValues(source_input_param);
}

int SystemControlService::setCVD2Values(source_input_param_t source_input_param)
{
    return pCPQControl->SetCVD2Values(source_input_param);
}

int SystemControlService::setPQConfig(Set_Flag_Cmd_t id, int value)
{
    return pCPQControl->SetFlagByCfg(id, value);
}

int SystemControlService::resetLastPQSettingsSourceType(void)
{
    return pCPQControl->resetLastSourceTypeSettings();
}

int SystemControlService::setCurrentSourceInfo(source_input_param_t source_input_param)
{
    return pCPQControl->SetCurrentSourceInputInfo(source_input_param);
}

source_input_param_t SystemControlService::getCurrentSourceInfo(void)
{
    return pCPQControl->GetCurrentSourceInputInfo();
}

int SystemControlService::getAutoSwitchPCModeFlag(void)
{
    return pCPQControl->autoSwitchMode();
}
//PQ end

int SystemControlService::setwhiteBalanceGainRed(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }

    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, GAIN_R, value);
    if (ret != -1) {
        ALOGI("save the red gain to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, GAIN_R, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceGainGreen(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, GAIN_G, value);
    if (ret != -1) {
        ALOGI("save the green gain to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, GAIN_G, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceGainBlue(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < 0) {
        value = 0;
    } else if (value > 2047) {
        value = 2047;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, GAIN_B, value);
    if (ret != -1) {
        ALOGI("save the blue gain to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, GAIN_B, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceOffsetRed(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_R, value);
    if (ret != -1) {
        ALOGI("save the red offset to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_R, value);
    }
    return ret;
}

int SystemControlService::setwhiteBalanceOffsetGreen(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_G, value);
    if (ret != -1) {
        ALOGI("save the green offset to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_G, value);
    }

    return ret;
}

int SystemControlService::setwhiteBalanceOffsetBlue(int inputSrc, int colortemp_mode, int value) {
    int ret = -1;
    if (value < -1024) {
        value = -1024;
    } else if (value > 1023) {
        value = 1023;
    }
    // not use fbc store the white balance params
    ret = pCPQControl->FactorySetColorTemperatureParam(colortemp_mode, POST_OFFSET_B, value);
    if (ret != -1) {
        ALOGI("save the green offset to flash");
        ret = pCPQControl->FactorySaveColorTemperatureParam(colortemp_mode, POST_OFFSET_B, value);
    }

    return ret;
}

int SystemControlService::getwhiteBalanceGainRed(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, GAIN_R);
}

int SystemControlService::getwhiteBalanceGainGreen(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, GAIN_G);
}

int SystemControlService::getwhiteBalanceGainBlue(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, GAIN_B);
}

int SystemControlService::getwhiteBalanceOffsetRed(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_R);
}

int SystemControlService::getwhiteBalanceOffsetGreen(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_G);
}

int SystemControlService::getwhiteBalanceOffsetBlue(int inputSrc, int colortemp_mode) {
    return pCPQControl->FactoryGetColorTemperatureParam(colortemp_mode, POST_OFFSET_B);
}

int SystemControlService::saveWhiteBalancePara(int sourceType, int colorTemp_mode, int r_gain, int g_gain, int b_gain, int r_offset, int g_offset, int b_offset) {
    int ret = 0;
    pCPQControl->SaveColorTemperature(colorTemp_mode);
    pCPQControl->FactorySaveColorTemperatureParam(colorTemp_mode, GAIN_R, r_gain);
    pCPQControl->FactorySaveColorTemperatureParam(colorTemp_mode, GAIN_G, g_gain);
    pCPQControl->FactorySaveColorTemperatureParam( colorTemp_mode, GAIN_B, b_gain);
    pCPQControl->FactorySaveColorTemperatureParam(colorTemp_mode, POST_OFFSET_R, r_offset);
    pCPQControl->FactorySaveColorTemperatureParam(colorTemp_mode, POST_OFFSET_G, g_offset);
    pCPQControl->FactorySaveColorTemperatureParam(colorTemp_mode, POST_OFFSET_B, b_offset);
    return ret;
}

int SystemControlService::getRGBPattern() {
    char value[32] = {0};
    bool ret = pSysWrite->readSysfs(VIDEO_RGB_SCREEN, value);
    return strtol(value, NULL, 10);
}

int SystemControlService::setRGBPattern(int r, int g, int b) {
    int value = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    char str[32] = {0};
    sprintf(str, "%d", value);
    bool ret = pSysWrite->writeSysfs(VIDEO_RGB_SCREEN, str, 16);
    return ret;
}

int SystemControlService::factorySetDDRSSC(int step) {
    if (step < 0 || step > 5) {
        SYS_LOGE ("%s, step = %d is too long", __FUNCTION__, step);
        return -1;
    }

    return mSSMAction->SSMSaveDDRSSC(step);
}

int SystemControlService::factoryGetDDRSSC() {
    unsigned char data = 0;
    mSSMAction->SSMReadDDRSSC(&data);
    return data;
}

int SystemControlService::setLVDSSSC(int step) {
    int ret = -1;
    if (step > 4)
        step = 4;

    FILE *fp = fopen(SSC_PATH, "w");
    if (fp != NULL) {
        fprintf(fp, "%d", step);
        fclose(fp);
    } else {
        SYS_LOGE("open /sys/class/lcd/ss ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
    return 0;
}
int SystemControlService::factorySetLVDSSSC(int step) {
    int ret = -1;
    unsigned char data[2] = {0, 0};
    char cmd_tmp_1[128];
    int value = 0, panel_idx = 0, tmp = 0;
    const char *PanelIdx;
    if (step > 4)
        step = 4;
    PanelIdx = "0";//config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" ); can't parse ini file in systemcontrol
    panel_idx = strtoul(PanelIdx, NULL, 10);
    SYS_LOGD ("%s, panel_idx = %x", __FUNCTION__, panel_idx);
    mSSMAction->SSMReadLVDSSSC(data);

    //every 2 bits represent one panel, use 2 byte store 8 panels
    value = (data[1] << 8) | data[0];
    step = step & 0x03;
    panel_idx = panel_idx * 2;
    tmp = 3 << panel_idx;
    value = (value & (~tmp)) | (step << panel_idx);
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    SYS_LOGD ("%s, tmp = %x, save value = %x", __FUNCTION__, tmp, value);

    setLVDSSSC(step);
    return mSSMAction->SSMSaveLVDSSSC(data);
}

int SystemControlService::factoryGetLVDSSSC() {
    unsigned char data[2] = {0, 0};
    int value = 0, panel_idx = 0;
    const char *PanelIdx = "0";//config_get_str ( CFG_SECTION_TV, "get.panel.index", "0" );can't parse ini file in systemcontrol

    panel_idx = strtoul(PanelIdx, NULL, 10);
    mSSMAction->SSMReadLVDSSSC(data);
    value = (data[1] << 8) | data[0];
    value = (value >> (2 * panel_idx)) & 0x03;
    SYS_LOGD ("%s, panel_idx = %x, value= %d", __FUNCTION__, panel_idx, value);
    return value;
}

int SystemControlService::whiteBalanceGrayPatternClose() {
    int ret = SetGrayPattern(0);
    return ret;
}

int SystemControlService::whiteBalanceGrayPatternOpen() {
    int ret = 0;
    return ret;
}

int SystemControlService::whiteBalanceGrayPatternSet(int value) {
    int ret = SetGrayPattern(value);
    return ret;
}

int SystemControlService::whiteBalanceGrayPatternGet() {
    int value = GetGrayPattern();
    return value;
}

int SystemControlService::SetGrayPattern(int value) {
    if (value < 0) {
        value = 0;
    } else if (value > 255) {
        value = 255;
    }
    value = value << 16 | 0x8080;

    SYS_LOGD("SetGrayPattern /sys/class/video/test_screen : %x", value);
    FILE *fp = fopen(TEST_SCREEN, "w");
    if (fp == NULL) {
        SYS_LOGE("Open /sys/classs/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    fprintf(fp, "0x%x", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

int SystemControlService::GetGrayPattern() {
    int value = 0;

    FILE *fp = fopen(TEST_SCREEN, "r+");
    if (fp == NULL) {
        SYS_LOGE("Open /sys/class/video/test_screen error(%s)!\n", strerror(errno));
        return -1;
    }

    fscanf(fp, "%x", &value);

    SYS_LOGD("GetGrayPattern /sys/class/video/test_screen %x", value);
    fclose(fp);
    fp = NULL;
    if (value < 0) {
        return 0;
    } else {
        value = value >> 16;
        if (value > 255) {
            value = 255;
        }
        return value;
    }
}

void SystemControlService::traceValue(const std::string& type, const std::string& key, const std::string& value) {
    if (mLogLevel > LOG_LEVEL_1) {
        /*
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %s ] from pid=%d, uid=%d, process name=%s",
            String8(type).string(), String8(key).string(), String8(value).string(),
            pid, uid,
            String8(procName).string());
            */
    }
}

void SystemControlService::traceValue(const std::string& type, const std::string& key, const int size) {
    if (mLogLevel > LOG_LEVEL_1) {
        /*
        String16 procName;
        int pid = IPCThreadState::self()->getCallingPid();
        int uid = IPCThreadState::self()->getCallingUid();

        getProcName(pid, procName);

        ALOGI("%s [ %s ] [ %d ] from pid=%d, uid=%d, process name=%s",
           String8(type).string(), String8(key).string(), size,
           pid, uid,
           String8(procName).string());
           */
    }
}

void SystemControlService::setLogLevel(int level) {
    if (level > (LOG_LEVEL_TOTAL - 1)) {
        ALOGE("out of range level=%d, max=%d", level, LOG_LEVEL_TOTAL);
        return;
    }

    mLogLevel = level;
    pSysWrite->setLogLevel(level);
    pDisplayMode->setLogLevel(level);
    pDimension->setLogLevel(level);
}

int SystemControlService::getLogLevel() {
    return mLogLevel;
}

int SystemControlService::getProcName(pid_t pid, String16& procName) {
    char proc_path[MAX_STR_LEN];
    char cmdline[64];
    int fd;

    strcpy(cmdline, "unknown");

    sprintf(proc_path, "/proc/%d/cmdline", pid);
    fd = open(proc_path, O_RDONLY);
    if (fd >= 0) {
        int rc = read(fd, cmdline, sizeof(cmdline)-1);
        cmdline[rc] = 0;
        close(fd);

        procName.setTo(String16(cmdline));
        return 0;
    }

    return -1;
}

status_t SystemControlService::dump(int fd, const Vector<String16>& args) {
#if 0
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    Mutex::Autolock lock(mLock);

    int len = args.size();
    for (int i = 0; i < len; i ++) {
        String16 debugLevel("-l");
        String16 bootenv("-b");
        String16 display("-d");
        String16 dimension("-dms");
        String16 hdcp("-hdcp");
        String16 help("-h");
        if (args[i] == debugLevel) {
            if (i + 1 < len) {
                String8 levelStr(args[i+1]);
                int level = atoi(levelStr.string());
                setLogLevel(level);

                result.append(String8::format("Setting log level to %d.\n", level));
                break;
            }
        }
        else if (args[i] == bootenv) {
            if ((i + 3 < len) && (args[i + 1] == String16("set"))) {
                setBootEnv(args[i+2], args[i+3]);

                result.append(String8::format("set bootenv key:[%s] value:[%s]\n",
                    String8(args[i+2]).string(), String8(args[i+3]).string()));
                break;
            }
            else if (((i + 2) <= len) && (args[i + 1] == String16("get"))) {
                if ((i + 2) == len) {
                    result.appendFormat("get all bootenv\n");
                    pUbootenv->printValues();
                }
                else {
                    String16 value;
                    getBootEnv(args[i+2], value);

                    result.append(String8::format("get bootenv key:[%s] value:[%s]\n",
                        String8(args[i+2]).string(), String8(value).string()));
                }
                break;
            }
            else {
                result.appendFormat(
                    "dump bootenv format error!! should use:\n"
                    "dumpsys system_control -b [set |get] key value \n");
            }
        }
        else if (args[i] == display) {
            /*
            String8 displayInfo;
            pDisplayMode->dump(displayInfo);
            result.append(displayInfo);*/

            char buf[4096] = {0};
            pDisplayMode->dump(buf);
            result.append(String8(buf));
            break;
        }
        else if (args[i] == dimension) {
            char buf[4096] = {0};
            pDimension->dump(buf);
            result.append(String8(buf));
            break;
        }
        else if (args[i] == hdcp) {
            pDisplayMode->hdcpSwitch();
            break;
        }
        else if (args[i] == help) {
            result.appendFormat(
                "system_control service use to control the system sysfs property and boot env \n"
                "in multi-user mode, normal process will have not system privilege \n"
                "usage: \n"
                "dumpsys system_control -l value \n"
                "dumpsys system_control -b [set |get] key value \n"
                "-l: debug level \n"
                "-b: set or get bootenv \n"
                "-d: dump display mode info \n"
                "-hdcp: stop hdcp and start hdcp tx \n"
                "-h: help \n");
        }
    }

    write(fd, result.string(), result.size());
#endif
    return NO_ERROR;
}

} // namespace android

