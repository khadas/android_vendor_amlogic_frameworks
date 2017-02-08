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
 *  @version  2.0
 *  @date     2014/09/09
 *  @par function description:
 *  - 1 write property or sysfs in daemon
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/properties.h>
#include <stdint.h>
#include <sys/types.h>
#include <FrameRateAutoAdaption.h>
#include <common.h>

FrameRateAutoAdaption::FrameRateAutoAdaption(Callbak *cb): mCallback(cb){
}

FrameRateAutoAdaption::~FrameRateAutoAdaption() {
}

void FrameRateAutoAdaption::initHdmiData(hdmi_data_t* data){
    int count = 0;
    while (true) {
        mSysWrite.readSysfsOriginal(DISPLAY_HDMI_EDID, data->edid);
        if (strlen(data->edid) > 0)
            break;

        if (count >= 5) {
            strcpy(data->edid, "null edid");
            break;
        }
        count++;
        usleep(500000);
    }
    mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, data->current_mode);
}

//Get best match display mode and if need pull down 1/1000 as video frame duration
void FrameRateAutoAdaption::getMatchDurOutputMode (int dur, char *dispmode, bool *pulldown) {
    char value[MODE_LEN] = {0};
    char refresh[10] = {0};
    char resolution[10] = {0};
    char suffix[10] = {0};
    char first_mode[MODE_LEN] = {0};
    char second_mode[MODE_LEN] = {0};
    bool need_pull_down = false;
    char *p = NULL;
    hdmi_data_t data;
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, value);
    SYS_LOGD("duration is: %d  policy_fr_auto value is: %s\n", dur, value);
    if (strcmp(value, FRAME_RATE_HDMI_OFF) == 0) {
        //frame rate adapter off
        *pulldown = false;
    }
    else if (strcmp(value, FRAME_RATE_HDMI_CLK_PULLDOWN) == 0) {
        //frame rate adapter mode 1:only need pull down 1/1000
        *pulldown = false;
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_5994)) {
            initHdmiData(&data);
            if (strstr(data.current_mode, "smpte")) {
                return;
            }
            else if (strstr(data.current_mode, "24hz")
                || strstr(data.current_mode, "30hz")
                || strstr(data.current_mode, "60hz")) {
                *pulldown = true;
            }
        }
    }
    else if (strcmp(value, FRAME_RATE_HDMI_SWITCH_FORCE) == 0) {
        //frame rate adapter mode 2:need change display mode and pull down 1/1000
        initHdmiData(&data);
        SYS_LOGD("getMatchDurOutputMode fr: %d\n", dur);
        if (strstr(data.current_mode, "smpte")) {
            return;
        }
        p = strstr(data.current_mode, "hz");
        if (p != NULL)
            p = p - 2;
        if ((p < data.current_mode)
            || (p > data.current_mode + strlen(data.current_mode))) {
           return;
        }

        strncpy(resolution, data.current_mode, int(p-data.current_mode));
        strncpy(refresh, p, 4);
        // if need 1/1000 pull down
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_5994)) {
            need_pull_down = true;
        }

        // get first and second mode for this duration
        if ((dur == FRAME_RATE_DURATION_2397)
            || (dur == FRAME_RATE_DURATION_2398)
            || (dur == FRAME_RATE_DURATION_24)) {
            sprintf(first_mode, "%s%s", resolution, "24hz");
            sprintf(second_mode, "%s%s", resolution, "60hz");
        }
        else if ((dur == FRAME_RATE_DURATION_2997)
            || (dur == FRAME_RATE_DURATION_30)) {
            sprintf(first_mode, "%s%s", resolution, "30hz");
            sprintf(second_mode, "%s%s", resolution, "60hz");
        }
        else if ((dur == FRAME_RATE_DURATION_5994)
            || (dur == FRAME_RATE_DURATION_60)) {
            sprintf(first_mode, "%s%s", resolution, "60hz");
            sprintf(second_mode, "%s%s", resolution, "30hz");
        }
        else if (dur == FRAME_RATE_DURATION_25) {
            sprintf(first_mode, "%s%s", resolution, "25hz");
            sprintf(second_mode, "%s%s", resolution, "50hz");
        }
        else if (dur == FRAME_RATE_DURATION_50) {
            sprintf(first_mode, "%s%s", resolution, "50hz");
            sprintf(second_mode, "%s%s", resolution, "25hz");
        }
        SYS_LOGD("getMatchDurOutputMode first_mode: %s second_mode:%s\n", first_mode, second_mode);

        if (!strcmp(first_mode, data.current_mode)) {
            *pulldown = need_pull_down;
        }
        else if (strstr(data.edid, first_mode)) {
            strncpy(dispmode, first_mode, strlen(first_mode));
            *pulldown = need_pull_down;
        }
        else if (!strcmp(second_mode, data.current_mode)) {
            *pulldown = need_pull_down;
        }
        else if (strstr(data.edid, second_mode)) {
            strncpy(dispmode, second_mode, strlen(first_mode));
            *pulldown = need_pull_down;
        }
        else {
            if (strstr(data.current_mode, "24hz")
                ||strstr(data.current_mode, "30hz")
                ||strstr(data.current_mode, "60hz"))
                *pulldown = need_pull_down;
            else
                *pulldown = false;
        }
        if (!strcmp(dispmode,"2160p60hz") || !strcmp(dispmode,"2160p50hz"))
            strcat(dispmode, "420");

        SYS_LOGD("getMatchDurOutputMode dispmode: %s\n", dispmode);
    }
}
void FrameRateAutoAdaption::onTxUeventReceived(uevent_data_t* ueventData){
    char status[5] ={0};
    char new_display_mode[MODE_LEN] = {0};
    bool need_pll_pull_dow = false;
    int duration = 0;
    char curDisplayMode[MODE_LEN] = {0};
    SYS_LOGD("Video framerate switch: %s\n", ueventData->buf);
    SYS_LOGD("Video framerate switchName: %s switch_state: %s\n", ueventData->switchName, ueventData->switchState);
    mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
    mSysWrite.readSysfs(HDMI_FRAME_RATE_AUTO, status);
    if ((strstr(curDisplayMode, "cvbs") == NULL) && (strcmp(status, FRAME_RATE_HDMI_OFF) != 0)) {
        if (!strcmp(ueventData->switchName, "end_hint")) {
            SYS_LOGD("Video framerate switch end hint last mode: %s\n", mLastVideoMode);
            if (strlen(mLastVideoMode) > 0) {
                mCallback->onDispModeSyncEvent(mLastVideoMode,OUPUT_MODE_STATE_SWITCH);
                memset(mLastVideoMode, 0, sizeof(mLastVideoMode));
	     }
	 }
        else{
            sscanf(ueventData->switchName, "%d", &duration);
            if (duration > 0) {
                memset(new_display_mode, 0, sizeof(new_display_mode));
                need_pll_pull_dow = false;
                getMatchDurOutputMode(duration,new_display_mode, &need_pll_pull_dow);
            }
           SYS_LOGD("Video framerate switch: %s\n", new_display_mode);
           memset(curDisplayMode, 0, sizeof(curDisplayMode));
           mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
           SYS_LOGD("Video framerate switch hint last mode: %s\n", mLastVideoMode);
           if ((strlen(new_display_mode) != 0) && need_pll_pull_dow) {
               strcpy(mLastVideoMode, curDisplayMode);
               mCallback->onDispModeSyncEvent(new_display_mode,OUPUT_MODE_STATE_SWITCH_ADAPTER);
           }
           else if ((strlen(new_display_mode) != 0) && !need_pll_pull_dow) {
               strcpy(mLastVideoMode, curDisplayMode);
               mCallback->onDispModeSyncEvent(new_display_mode,OUPUT_MODE_STATE_SWITCH);
           }
           else if (need_pll_pull_dow) {
               strcpy(mLastVideoMode, curDisplayMode);
               mCallback->onDispModeSyncEvent(curDisplayMode,OUPUT_MODE_STATE_SWITCH_ADAPTER);
           }
        }
    }
}