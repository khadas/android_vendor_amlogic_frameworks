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
 *  - 1 HDMI deep color attribute
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
#include "DisplayMode.h"
#include "FormatColorDepth.h"
#include "common.h"
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

//this is prior selected list  of 4k2k50hz420, 4k2k60hz420, smpte50hz420, smpte60hz420
static const char* COLOR_ATTRIBUTE_LIST4[] = {
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
};

FormatColorDepth::FormatColorDepth() {
}

FormatColorDepth::~FormatColorDepth() {
}

bool FormatColorDepth::initColorAttribute(char* supportedColorList, int len) {
    int count = 0;
    bool result = false;

    if (supportedColorList != NULL)
        memset(supportedColorList, 0, len);

    while (true) {
        //mSysWrite.readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        mSysWrite.readSysfs(DISPLAY_HDMI_DEEP_COLOR, supportedColorList);
        if (strlen(supportedColorList) > 0) {
            result = true;
            break;
        }

        if (count++ >= 5) {
            break;
        }
        usleep(500000);
    }

    return result;
}

void FormatColorDepth::getHdmiColorAttribute(const char* outputmode, char* colorAttribute, int state) {
    char supportedColorList[MAX_STR_LEN];

    if (!initColorAttribute(supportedColorList, MAX_STR_LEN)) {
        mSysWrite.getPropertyString(PROP_DEFAULT_COLOR, colorAttribute,
                (strstr(outputmode, "420") != NULL) ? COLOR_YCBCR420_8BIT : COLOR_YCBCR444_8BIT);
        SYS_LOGE("Error!!! Do not find sink color list, use default color attribute:%s\n", colorAttribute);
        return;
    }

    if (mSysWrite.getPropertyBoolean(PROP_HDMIONLY, true)) {
        char curMode[MODE_LEN] = {0};
        mSysWrite.readSysfs(SYSFS_DISPLAY_MODE, curMode);

        if ((state == OUPUT_MODE_STATE_SWITCH) && (!strcmp(curMode, outputmode))) {
            //note: "outputmode" should be the second parameter of "strcmp", because it maybe prefix of "curMode".
            SYS_LOGI("Only modify deep color mode, get colorAttr from ubootenv.var.colorattribute\n");
            getBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
        } else {
            getBestHdmiColorArrtibute(outputmode, supportedColorList, colorAttribute);
        }
    }

    SYS_LOGI("get hdmi color attribute : [%s], outputmode is: [%s] , and support color list is: [%s]\n",
        colorAttribute, outputmode, supportedColorList);
}

void FormatColorDepth::getBestHdmiColorArrtibute(const char* outputmode, char* supportedColorList, char* colorAttribute) {
    char *pos = NULL;
    const char **colorList = NULL;
    int length = 0;
    char ubootvar[MODE_LEN] = {0};
    char tmpValue[MODE_LEN] = {0};
    char isBestMode[MODE_LEN] = {0};

    if (getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) && strcmp(isBestMode, "false") == 0) {
        SYS_LOGI("get color attr from ubootenv.var.%s_deepcolor When is not best mode\n", outputmode);
        sprintf(ubootvar, "ubootenv.var.%s_deepcolor", outputmode);
        if (getBootEnv(ubootvar, tmpValue) && isModeSupportDeepColorAttr(outputmode, tmpValue)) {
            strcpy(colorAttribute, tmpValue);
            return;
        }
    }

    if (!strcmp(outputmode, MODE_4K2K60HZ) || !strcmp(outputmode, MODE_4K2K50HZ)) {
        colorList = COLOR_ATTRIBUTE_LIST1;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
    }
    else if (!strcmp(outputmode, MODE_4K2KSMPTE60HZ) || !strcmp(outputmode, MODE_4K2KSMPTE50HZ)) {
        colorList = COLOR_ATTRIBUTE_LIST2;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
    } else if (!strcmp(outputmode, MODE_4K2K50HZ420) || !strcmp(outputmode, MODE_4K2K60HZ420) ||
        !strcmp(outputmode, MODE_4K2KSMPTE50HZ420) || !strcmp(outputmode, MODE_4K2KSMPTE60HZ420)) {
        colorList = COLOR_ATTRIBUTE_LIST4;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST4);
    }
    else {
        colorList = COLOR_ATTRIBUTE_LIST3;
        length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST3);
    }

    for (int i = 0; i < length; i++) {
        if ((pos = strstr(supportedColorList, colorList[i])) != NULL) {
            if (isModeSupportDeepColorAttr(outputmode, colorList[i])) {
                SYS_LOGI("support current mode:[%s], deep color:[%s]\n", outputmode, colorList[i]);

                strcpy(colorAttribute, colorList[i]);
                break;
            }
        }
    }

    //SYS_LOGI("get best hdmi color attribute %s\n", colorAttribute);
}

bool FormatColorDepth::isModeSupportDeepColorAttr(const char *mode, const char * color) {
    char valueStr[10] = {0};
    char outputmode[MODE_LEN] = {0};
    strcpy(outputmode, mode);
    strcat(outputmode, color);
    //try support or not
    mSysWrite.writeSysfs(DISPLAY_HDMI_VALID_MODE, outputmode);
    mSysWrite.readSysfs(DISPLAY_HDMI_VALID_MODE, valueStr);
    return atoi(valueStr) ? true : false;
}

bool FormatColorDepth::getBootEnv(const char* key, char* value) {
    const char* p_value = bootenv_get(key);
    //SYS_LOGI("getBootEnv key:%s value:%s", key, p_value);
    if (p_value) {
        strcpy(value, p_value);
        return true;
    }
    return false;
}
