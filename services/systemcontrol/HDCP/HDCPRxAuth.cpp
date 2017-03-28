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
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP RX authenticate
 */

#define LOG_TAG "SystemControl"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "HDCPRxKey.h"
#include "HDCPRxAuth.h"
#include "UEventObserver.h"
#include "../DisplayMode.h"

HDCPRxAuth::HDCPRxAuth(HDCPTxAuth *txAuth) :
    pTxAuth(txAuth) {

    initKey();

    pthread_t id;
    int ret = pthread_create(&id, NULL, RxUenventThreadLoop, this);
    if (ret != 0) {
        SYS_LOGE("HDCP RX, Create RxUenventThreadLoop error!\n");
    }
}

HDCPRxAuth::~HDCPRxAuth() {

}

void HDCPRxAuth::initKey() {
    //init HDCP 1.4 key
    HDCPRxKey hdcpRx14(HDCP_RX_14_KEY);
    hdcpRx14.refresh();

    //init HDCP 2.2 key
    HDCPRxKey hdcpRx22(HDCP_RX_22_KEY);
    hdcpRx22.refresh();
/*
#ifndef RECOVERY_MODE
#ifdef IMPDATA_HDCP_RX_KEY//used for tcl
    if ((access(HDCP_RX_DES_FW_PATH, F_OK) || (access(HDCP_NEW_KEY_CREATED, F_OK) == F_OK)) &&
        (access(HDCP_PACKED_IMG_PATH, F_OK) == F_OK)) {
        SYS_LOGI("HDCP rx 2.2 firmware do not exist or new key come, first create it\n");
        generateHdcpFw(HDCP_FW_LE_OLD_PATH, HDCP_PACKED_IMG_PATH, HDCP_RX_DES_FW_PATH);
        remove(HDCP_NEW_KEY_CREATED);
    }
#else

    #if 0
    if (access(HDCP_RX_DES_FW_PATH, F_OK)) {
        SYS_LOGI("HDCP rx 2.2 firmware do not exist, first create it\n");
        int ret = generateHdcpFwFromStorage(HDCP_RX_SRC_FW_PATH, HDCP_RX_DES_FW_PATH);
        if (ret < 0) {
            pSysWrite->writeSysfs(HDMI_RX_KEY_COMBINE, "0");
            SYS_LOGE("HDCP rx 2.2 generate firmware fail\n");
        }
    }
    #else
    HdcpRx22Key hdcpRxFw;
    hdcpRxFw.generateHdcpRxFw();
    #endif

#endif
#endif
*/
}

void HDCPRxAuth::startVer22() {
    SysWrite write;
    write.setProperty("ctl.start", "hdcp_rx22");
}

void HDCPRxAuth::stopVer22() {
    SysWrite write;
    write.setProperty("ctl.stop", "hdcp_rx22");
}

void HDCPRxAuth::forceFlushVideoLayer() {
#ifndef RECOVERY_MODE
    SysWrite write;
    char valueStr[10] = {0};

    write.readSysfs(SYSFS_VIDEO_LAYER_STATE, valueStr);
    int curVideoState = atoi(valueStr);

    if (curVideoState != mLastVideoState) {
        SYS_LOGI("hdcp_rx Video Layer1 switch_state: %d\n", curVideoState);
        pTxAuth->sfRepaintEverything();
        mLastVideoState = curVideoState;
    }
    usleep(200*1000);//sleep 200ms
#endif
}

// HDMI RX uevent prcessed in this loop
void* HDCPRxAuth::RxUenventThreadLoop(void* data) {
    HDCPRxAuth *pThiz = (HDCPRxAuth*)data;

    SysWrite sysWrite;
    uevent_data_t ueventData;
    memset(&ueventData, 0, sizeof(uevent_data_t));

    UEventObserver ueventObserver;
    ueventObserver.addMatch(HDMI_RX_PLUG_UEVENT);
    ueventObserver.addMatch(HDMI_RX_AUTH_UEVENT);

    while (true) {
        ueventObserver.waitForNextEvent(&ueventData);
        SYS_LOGI("HDCP RX switch_name: %s ,switch_state: %s\n", ueventData.switchName, ueventData.switchState);

        if (!strcmp(ueventData.matchName, HDMI_RX_PLUG_UEVENT)) {
            if (!strcmp(ueventData.switchState, HDMI_RX_PLUG_IN)) {
                pThiz->pTxAuth->stop();
                pThiz->stopVer22();
                usleep(50*1000);
                pThiz->startVer22();
            } else if (!strcmp(ueventData.switchState, HDMI_RX_PLUG_OUT)) {
                pThiz->pTxAuth->stop();
                pThiz->stopVer22();
                pThiz->pTxAuth->start();
            }
        }
        else if (!strcmp(ueventData.matchName, HDMI_RX_AUTH_UEVENT)) {
            if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_FAIL)) {
                SYS_LOGI("HDCP RX, switch_state: %s error\n", ueventData.switchState);
                continue;
            }

            if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_HDCP14)) {
                SYS_LOGI("HDCP RX, 1.4 hdmi is plug in\n");
                pThiz->pTxAuth->setRepeaterRxVersion(REPEATER_RX_VERSION_14);
            }
            else if (!strcmp(ueventData.switchState, HDMI_RX_AUTH_HDCP22)) {
                SYS_LOGI("HDCP RX, 2.2 hdmi is plug in\n");
                pThiz->pTxAuth->setRepeaterRxVersion(REPEATER_RX_VERSION_22);
            }

            pThiz->forceFlushVideoLayer();

            char hdmiTxState[MODE_LEN] = {0};
            sysWrite.readSysfs(HDMI_TX_PLUG_STATE, hdmiTxState);
            if (!strcmp(hdmiTxState, "1")) {
                sysWrite.writeSysfs(DISPLAY_HDMI_AVMUTE, "1");

                SYS_LOGI("HDCP RX, repeater TX hdmi is plug in\n");
                pThiz->pTxAuth->stop();
                pThiz->pTxAuth->start();
            }
            else {
                SYS_LOGI("HDCP RX, repeater TX hdmi is plug out\n");
            }
        }
    }

    return NULL;
}

