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
#include <FormatColorDepth.h>
#include <common.h>
#include "ubootenv.h"

//this is prior selected list  of 4k2k50hz, 4k2k60hz
static const char* COLOR_ATTRIBUTE_LIST1[] = {
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of smpte50hz, smpte60hz
static const char* COLOR_ATTRIBUTE_LIST2[] = {
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list  of other display mode
static const char* COLOR_ATTRIBUTE_LIST3[] = {
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

FormatColorDepth::FormatColorDepth(int dispayType) {
        this->mDisplayType = dispayType;
}

FormatColorDepth::~FormatColorDepth() {
}

void FormatColorDepth::initColorAttribute(char* supportedColorList, int lenth) {
        int count = 0;
        if (lenth < MAX_STR_LEN) {
            SYS_LOGE("supportedColorList init lenth is %d, not enough !\n", lenth);
            return;
        }

        if (supportedColorList != NULL)
            memset(supportedColorList, 0, lenth);

        while (true) {
            mSysWrite.readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
            if (strlen(supportedColorList) > 0)
                break;

            if (count >= 5) {
                strcpy(supportedColorList, "null data");
                break;
            }
            count++;
            usleep(500000);
        }
}

void FormatColorDepth::getHdmiColorAttribute(char* outputmode, char* colorAttribute, int state) {
    char supportedColorList[MAX_STR_LEN];
    initColorAttribute(supportedColorList, MAX_STR_LEN);
    if (strstr(supportedColorList, "null") != NULL) {
        mSysWrite.getPropertyString(PROP_DEFAULT_COLOR, colorAttribute, COLOR_YCBCR444_8BIT);
    }
    char curDisplayMode[MODE_LEN] = {0};
    char outputMode[MODE_LEN] = {0};
    char colorData[MODE_LEN] = {0};
    strcpy(outputMode, outputmode);
    char* end = strstr(outputMode, "hz");
    if (end == NULL)//cvbs
       return;
    char* p = NULL;
    p = end +2;
    if (*p != 0) {
        memset(p, 0, strlen(p));
    }
    mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
    SYS_LOGI("the outputMode is: %s , and support_color_list is: %s\n", outputMode, supportedColorList);
    if (mSysWrite.getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (isBestOutputmode()) {//1.auto complete select
            SYS_LOGI("support best outputmode\n");
            getBestHdmiColorArrtibute(outputMode, supportedColorList, colorAttribute);
        } else {
            if (state == OUPUT_MODE_STATE_SWITCH ) {
                SYS_LOGI("outputmode or deepcolor will be switch\n");
                if (strncmp(outputMode, curDisplayMode, strlen(outputMode))) {
                    SYS_LOGI("only change outputmode\n");
                    getBestHdmiColorArrtibute(outputMode, supportedColorList, colorAttribute);
               } else if (!strncmp(outputMode, curDisplayMode, strlen(outputMode))) {
                   SYS_LOGI("only change deepcolor\n");
                   getBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
              }
            } else {
                SYS_LOGI("not outputmode_state_switch\n");
                getBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
                char saveOutputMode[MODE_LEN] = {0};
                getBootEnv(UBOOTENV_HDMIMODE, saveOutputMode);
                if (strlen(colorAttribute) == 0 || strncmp(outputMode, saveOutputMode, strlen(outputMode))) {//2.this case:plug into defferernt display device while not power off
                    SYS_LOGI("getBestHdmiColorAttribute when BootEnv is null\n");
                    getBestHdmiColorArrtibute(outputMode, supportedColorList, colorAttribute);
                }
           }
        }
    }
    setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
    SYS_LOGI("set HDMI color Attribute to %s\n", colorAttribute);
}

void FormatColorDepth::getBestHdmiColorArrtibute(const char* outputmode, char* supportedColorList, char* colorAttribute) {
    if (!strcmp(outputmode, MODE_4K2K60HZ) ||!strcmp(outputmode, MODE_4K2K50HZ)) {
        searchBestHdmiColorArrtibute(outputmode, supportedColorList, COLOR_ATTRIBUTE_LIST1, (sizeof(COLOR_ATTRIBUTE_LIST1)/sizeof(COLOR_ATTRIBUTE_LIST1[0])), colorAttribute);
    } else if (!strcmp(outputmode, MODE_4K2KSMPTE60HZ)||!strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
        searchBestHdmiColorArrtibute(outputmode, supportedColorList, COLOR_ATTRIBUTE_LIST2, (sizeof(COLOR_ATTRIBUTE_LIST2)/sizeof(COLOR_ATTRIBUTE_LIST2[0])), colorAttribute);
    } else {
        searchBestHdmiColorArrtibute(outputmode, supportedColorList, COLOR_ATTRIBUTE_LIST3, (sizeof(COLOR_ATTRIBUTE_LIST3)/sizeof(COLOR_ATTRIBUTE_LIST3[0])), colorAttribute);
    }
    SYS_LOGI("get BestHdmi color Attribute %s\n", colorAttribute);
}

void FormatColorDepth::searchBestHdmiColorArrtibute(const char* outputmode, char* supportedColorList,  const char* selectList[], int listLenth, char* colorAttribute) {
    char *pos = NULL;
    if (strstr(supportedColorList, "null") != NULL || (strlen(supportedColorList) == 0)) {
        mSysWrite.getPropertyString(PROP_DEFAULT_COLOR, colorAttribute, COLOR_YCBCR444_8BIT);
        SYS_LOGI("supportedColorList is null and return !\n");
        return;
    }
    for (int i = 0; i < listLenth; i++) {
        if ((pos = strstr(supportedColorList, selectList[i])) != NULL) {
            int isSupport = 1;
            char valueStr[10] = {0};
            char buf[MODE_LEN] = {0};
            strcpy(buf, outputmode);
            strcat(buf, selectList[i]);
            mSysWrite.writeSysfs(DISPLAY_HDMI_VALID_MODE, buf);
            mSysWrite.readSysfs(DISPLAY_HDMI_VALID_MODE, valueStr);
            isSupport = atoi(valueStr);
            SYS_LOGI("the echomode is:%s[%d]\n", buf, isSupport);
            if (isSupport) {
                strcpy(colorAttribute, selectList[i]);
                return;
            }
	}
    }
}

bool FormatColorDepth::getBootEnv(const char* key, char* value) {
    const char* p_value = bootenv_get(key);
    SYS_LOGI("getBootEnv key:%s value:%s", key, p_value);
    if (p_value) {
        strcpy(value, p_value);
        return true;
    }
    return false;
}

void FormatColorDepth::setBootEnv(const char* key, char* value) {
    SYS_LOGI("setBootEnv key:%s value:%s", key, value);
    bootenv_update(key, value);
}

bool FormatColorDepth::isBestOutputmode() {
    char isBestMode[MODE_LEN] = {0};
    if (mDisplayType == TYPE_DISPLAY_TV) {
        return false;
    }
    return !getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) || strcmp(isBestMode, "true") == 0;
}