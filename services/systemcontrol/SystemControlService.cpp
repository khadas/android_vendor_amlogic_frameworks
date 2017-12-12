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
//#define LOG_NDEBUG 0

#include <fcntl.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <private/android_filesystem_config.h>
#include <pthread.h>

#include "SystemControlService.h"

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

int32_t SystemControlService::readHdcpRX22Key(char *value __attribute__((unused)), int size __attribute__((unused))) {
    /*if (NO_ERROR == permissionCheck()) {
        traceValue(String16("readHdcpRX22Key"), size);
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
        traceValue(String16("readHdcpRX14Key"), size);
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

void SystemControlService::getDroidDisplayInfo(int &type, std::string& socType, std::string& defaultUI,
        int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip) {
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
    char value[MODE_LEN] = {0};
    bool ret = pDisplayMode->isTvSupportDolbyVision(value);
    mode = value;
    return ret;
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

