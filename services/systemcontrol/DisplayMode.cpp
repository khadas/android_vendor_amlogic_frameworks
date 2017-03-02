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
 *  @date     2014/10/23
 *  @par function description:
 *  - 1 set display mode
 */

#define LOG_TAG "SystemControl"
//#define LOG_NDEBUG 0

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <cutils/properties.h>
#include "ubootenv.h"
#include "DisplayMode.h"
#include "SysTokenizer.h"

#ifndef RECOVERY_MODE
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

using namespace android;
#endif

static const char* DISPLAY_MODE_LIST[DISPLAY_MODE_TOTAL] = {
    MODE_480I,
    MODE_480P,
    MODE_480CVBS,
    MODE_576I,
    MODE_576P,
    MODE_576CVBS,
    MODE_720P,
    MODE_720P50HZ,
    MODE_1080P24HZ,
    MODE_1080I50HZ,
    MODE_1080P50HZ,
    MODE_1080I,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K50HZ420,
    MODE_4K2K50HZ422,
    MODE_4K2K60HZ,
    MODE_4K2K60HZ420,
    MODE_4K2K60HZ422,
    MODE_4K2KSMPTE,
    MODE_4K2KSMPTE30HZ,
    MODE_4K2KSMPTE50HZ,
    MODE_4K2KSMPTE50HZ420,
    MODE_4K2KSMPTE60HZ,
    MODE_4K2KSMPTE60HZ420,
};

/**
 * strstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 */
char *_strstr(const char *s1, const char *s2)
{
    size_t l1, l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;
    l1 = strlen(s1);
    while (l1 >= l2) {
        l1--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

static void copy_if_gt0(uint32_t *src, uint32_t *dst, unsigned cnt)
{
    do {
        if ((int32_t) *src > 0)
            *dst = *src;
        src++;
        dst++;
    } while (--cnt);
}

static void copy_changed_values(
            struct fb_var_screeninfo *base,
            struct fb_var_screeninfo *set)
{
    //if ((int32_t) set->xres > 0) base->xres = set->xres;
    //if ((int32_t) set->yres > 0) base->yres = set->yres;
    //if ((int32_t) set->xres_virtual > 0)   base->xres_virtual = set->xres_virtual;
    //if ((int32_t) set->yres_virtual > 0)   base->yres_virtual = set->yres_virtual;
    copy_if_gt0(&set->xres, &base->xres, 4);

    if ((int32_t) set->bits_per_pixel > 0) base->bits_per_pixel = set->bits_per_pixel;
    //copy_if_gt0(&set->bits_per_pixel, &base->bits_per_pixel, 1);

    //if ((int32_t) set->pixclock > 0)       base->pixclock = set->pixclock;
    //if ((int32_t) set->left_margin > 0)    base->left_margin = set->left_margin;
    //if ((int32_t) set->right_margin > 0)   base->right_margin = set->right_margin;
    //if ((int32_t) set->upper_margin > 0)   base->upper_margin = set->upper_margin;
    //if ((int32_t) set->lower_margin > 0)   base->lower_margin = set->lower_margin;
    //if ((int32_t) set->hsync_len > 0) base->hsync_len = set->hsync_len;
    //if ((int32_t) set->vsync_len > 0) base->vsync_len = set->vsync_len;
    //if ((int32_t) set->sync > 0)  base->sync = set->sync;
    //if ((int32_t) set->vmode > 0) base->vmode = set->vmode;
    copy_if_gt0(&set->pixclock, &base->pixclock, 9);
}

DisplayMode::DisplayMode(const char *path)
    :mDisplayType(DISPLAY_TYPE_MBOX),
    mFb0Width(-1),
    mFb0Height(-1),
    mFb0FbBits(-1),
    mFb0TripleEnable(true),
    mFb1Width(-1),
    mFb1Height(-1),
    mFb1FbBits(-1),
    mFb1TripleEnable(true),
    mNativeWinX(0), mNativeWinY(0), mNativeWinW(0), mNativeWinH(0),
    mDisplayWidth(FULL_WIDTH_1080),
    mDisplayHeight(FULL_HEIGHT_1080),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    if (NULL == path) {
        pConfigPath = DISPLAY_CFG_FILE;
    }
    else {
        pConfigPath = path;
    }

    SYS_LOGI("display mode config path: %s", pConfigPath);
    pSysWrite = new SysWrite();
}

DisplayMode::~DisplayMode() {
    delete pSysWrite;
}

void DisplayMode::init() {
    parseConfigFile();

    SYS_LOGI("display mode init type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
        mDisplayType, mSocType, mDefaultUI);
    if (DISPLAY_TYPE_TABLET == mDisplayType) {
        setTabletDisplay();
    }
    else if (DISPLAY_TYPE_MBOX == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        setMboxDisplay(NULL, OUPUT_MODE_STATE_INIT);
    }
    else if (DISPLAY_TYPE_TV == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        pRxAuth = new HDCPRxAuth(pTxAuth);

        setTVDisplay(true);
    }
}

void DisplayMode::reInit() {
    char boot_type[MODE_LEN] = {0};
    /*
     * boot_type would be "normal", "fast", "snapshotted", or "instabooting"
     * "normal": normal boot, the boot_type can not be it here;
     * "fast": fast boot;
     * "snapshotted": this boot contains instaboot image making;
     * "instabooting": doing the instabooting operation, the boot_type can not be it here;
     * for fast boot, need to reinit the display, but for snapshotted, reInit display would make a screen flicker
     */
    pSysWrite->readSysfs(SYSFS_BOOT_TYPE, boot_type);
    if (strcmp(boot_type, "snapshotted")) {
        SYS_LOGI("display mode reinit type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s, default UI:%s",
            mDisplayType, mSocType, mDefaultUI);
        if (DISPLAY_TYPE_TABLET == mDisplayType) {
            setTabletDisplay();
        }
        else if (DISPLAY_TYPE_MBOX == mDisplayType) {
            setMboxDisplay(NULL, OUPUT_MODE_STATE_POWER);
        }
        else if (DISPLAY_TYPE_TV == mDisplayType) {
            setTVDisplay(false);
        }
    }

    SYS_LOGI("open osd0 and disable video\n");
    pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_AUTO_ENABLE);
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

HDCPTxAuth *DisplayMode:: geTxAuth() {
    return pTxAuth;
}

void DisplayMode:: getDisplayInfo(int &type, char* socType, char* defaultUI) {
    type = mDisplayType;
    if (NULL != socType)
        strcpy(socType, mSocType);

    if (NULL != defaultUI)
        strcpy(defaultUI, mDefaultUI);
}

void DisplayMode:: getFbInfo(int &fb0w, int &fb0h, int &fb0bits, int &fb0trip,
        int &fb1w, int &fb1h, int &fb1bits, int &fb1trip) {
    fb0w = mFb0Width;
    fb0h = mFb0Height;
    fb0bits = mFb0FbBits;
    fb0trip = mFb0TripleEnable?1:0;

    fb1w = mFb1Width;
    fb1h = mFb1Height;
    fb1bits = mFb1FbBits;
    fb1trip = mFb1TripleEnable?1:0;
}

void DisplayMode::setLogLevel(int level){
    mLogLevel = level;
}

bool DisplayMode::getBootEnv(const char* key, char* value) {
    const char* p_value = bootenv_get(key);

    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("getBootEnv key:%s value:%s", key, p_value);

	if (p_value) {
        strcpy(value, p_value);
        return true;
	}
    return false;
}

void DisplayMode::setBootEnv(const char* key, char* value) {
    if (mLogLevel > LOG_LEVEL_1)
        SYS_LOGI("setBootEnv key:%s value:%s", key, value);

    bootenv_update(key, value);
}

int DisplayMode::parseConfigFile(){
    const char* WHITESPACE = " \t\r";

    SysTokenizer* tokenizer;
    int status = SysTokenizer::open(pConfigPath, &tokenizer);
    if (status) {
        SYS_LOGE("Error %d opening display config file %s.", status, pConfigPath);
    } else {
        while (!tokenizer->isEof()) {

            if(mLogLevel > LOG_LEVEL_1)
                SYS_LOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());

            tokenizer->skipDelimiters(WHITESPACE);

            if (!tokenizer->isEol() && tokenizer->peekChar() != '#') {

                char *token = tokenizer->nextToken(WHITESPACE);
                if(!strcmp(token, DEVICE_STR_MID)){
                    mDisplayType = DISPLAY_TYPE_TABLET;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb0Width = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb0Height = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb0FbBits = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb0TripleEnable = (0 == atoi(tokenizer->nextToken(WHITESPACE)))?false:true;

                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb1Width = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb1Height = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb1FbBits = atoi(tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    mFb1TripleEnable = (0 == atoi(tokenizer->nextToken(WHITESPACE)))?false:true;

                } else if (!strcmp(token, DEVICE_STR_MBOX)) {
                    mDisplayType = DISPLAY_TYPE_MBOX;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                } else if (!strcmp(token, DEVICE_STR_TV)) {
                    mDisplayType = DISPLAY_TYPE_TV;

                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mSocType, tokenizer->nextToken(WHITESPACE));
                    tokenizer->skipDelimiters(WHITESPACE);
                    strcpy(mDefaultUI, tokenizer->nextToken(WHITESPACE));
                }else {
                    SYS_LOGE("%s: Expected keyword, got '%s'.", tokenizer->getLocation(), token);
                    break;
                }
            }

            tokenizer->nextLine();
        }
        delete tokenizer;
    }
    return status;
}

void DisplayMode::setTabletDisplay() {
    struct fb_var_screeninfo var_set;

    var_set.xres = mFb0Width;
	var_set.yres = mFb0Height;
	var_set.xres_virtual = mFb0Width;
    if(mFb0TripleEnable)
	    var_set.yres_virtual = 3*mFb0Height;
    else
        var_set.yres_virtual = 2*mFb0Height;
	var_set.bits_per_pixel = mFb0FbBits;
    setFbParameter(DISPLAY_FB0, var_set);

    pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
    var_set.xres = mFb1Width;
	var_set.yres = mFb1Height;
	var_set.xres_virtual = mFb1Width;
    if (mFb1TripleEnable)
	    var_set.yres_virtual = 3*mFb1Height;
    else
        var_set.yres_virtual = 2*mFb1Height;
	var_set.bits_per_pixel = mFb1FbBits;
    setFbParameter(DISPLAY_FB1, var_set);

    char axis[512] = {0};
    sprintf(axis, "%d %d %d %d %d %d %d %d",
        0, 0, mFb0Width, mFb0Height, 0, 0, mFb1Width, mFb1Height);

    pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "panel");
    pSysWrite->writeSysfs(SYSFS_DISPLAY_AXIS, axis);

    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

void DisplayMode::setMboxDisplay(char* hpdstate, output_mode_state state) {
    hdmi_data_t data;
    char outputmode[MODE_LEN] = {0};
    memset(&data, 0, sizeof(hdmi_data_t));

    if (mDisplayType == DISPLAY_TYPE_TV) {
        pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_DISABLE);
    }

    initHdmiData(&data, hpdstate);
    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (!strcmp(data.hpd_state, "1")) {
            if ((!strcmp(data.current_mode, MODE_480CVBS) || !strcmp(data.current_mode, MODE_576CVBS))
                    && (OUPUT_MODE_STATE_INIT == state)) {
                pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
                pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
            }

            getHdmiOutputMode(outputmode, &data);
        } else {
            getBootEnv(UBOOTENV_CVBSMODE, outputmode);
        }
    } else {
        getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    }

    //if the tv don't support current outputmode,then switch to best outputmode
    if (strcmp(data.hpd_state, "1")) {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "3");
        if (strcmp(outputmode, MODE_480CVBS) && strcmp(outputmode, MODE_576CVBS)) {
            strcpy(outputmode, MODE_576CVBS);
        }
    } else {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "0");
    }

    SYS_LOGI("init mbox display hpdstate:%s, old outputmode:%s, new outputmode:%s\n",
            data.hpd_state,
            data.current_mode,
            outputmode);
    if (strlen(outputmode) == 0)
        strcpy(outputmode, DEFAULT_OUTPUT_MODE);

    if (OUPUT_MODE_STATE_INIT == state) {
        if (!strncmp(mDefaultUI, "720", 3)) {
            mDisplayWidth= FULL_WIDTH_720;
            mDisplayHeight = FULL_HEIGHT_720;
            //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_720P);
            //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1280");
            //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "720");
        } else if (!strncmp(mDefaultUI, "1080", 4)) {
            mDisplayWidth = FULL_WIDTH_1080;
            mDisplayHeight = FULL_HEIGHT_1080;
            //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_1080P);
            //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1920");
            //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "1080");
        } else if (!strncmp(mDefaultUI, "4k2k", 4)) {
            mDisplayWidth = FULL_WIDTH_4K2K;
            mDisplayHeight = FULL_HEIGHT_4K2K;
            //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_2160P);
            //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "3840");
            //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "2160");
        }
    }

    //output mode not the same
    if (strcmp(data.current_mode, outputmode)) {
        if (OUPUT_MODE_STATE_INIT == state) {
            //when change mode, need close uboot logo to avoid logo scaling wrong
            pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
            pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
            pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        }
    }
    setMboxOutputMode(outputmode, state);
}

void DisplayMode::setMboxOutputMode(const char* outputmode){
    setMboxOutputMode(outputmode, OUPUT_MODE_STATE_SWITCH);
}

void DisplayMode::setMboxOutputMode(const char* outputmode, output_mode_state state) {
    char value[MAX_STR_LEN] = {0};
    char finalMode[MODE_LEN] = {0};
    int outputx = 0;
    int outputy = 0;
    int outputwidth = 0;
    int outputheight = 0;
    int position[4] = { 0, 0, 0, 0 };
    bool cvbsMode = false;

    strcpy(finalMode, outputmode);
    addSuffixForMode(finalMode, state);
    pSysWrite->readSysfs(HDMI_TX_FRAMRATE_POLY, value);
    if (state == OUPUT_MODE_STATE_SWITCH  && (strcmp(value, "0") == 0)) {
        char curDisplayMode[MODE_LEN] = {0};
        pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
        if (!strcmp(finalMode, curDisplayMode)) {
            return;
        }
    }

    if (OUPUT_MODE_STATE_INIT != state) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "1");
        if (OUPUT_MODE_STATE_POWER != state) {
            usleep(50000);//50ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_HDCP_MODE, "-1");
            usleep(100000);//100ms
            pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "0"); /* Turn off TMDS PHY */
            usleep(50000);//50ms
        }
    }

    getPosition(finalMode, position);
    outputx = position[0];
    outputy = position[1];
    outputwidth = position[2];
    outputheight = position[3];


    char curDisplayMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
    if (OUPUT_MODE_STATE_INIT != state && !strcmp(finalMode, curDisplayMode)) {
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
    }
    if (state == OUPUT_MODE_STATE_SWITCH_ADAPTER) {
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLY, "1");
    }
    else {
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLY, "0");
    }

    if ((!strcmp(finalMode, MODE_480I) || !strcmp(finalMode, MODE_576I)) &&
            (pSysWrite->getPropertyBoolean(PROP_HAS_CVBS_MODE, false))) {
        const char *mode = "";
        if (!strcmp(finalMode, MODE_480I)) {
            mode = MODE_480CVBS;
        }
        else if (!strcmp(finalMode, MODE_576I)) {
            mode = MODE_576CVBS;
        }

        cvbsMode = true;

        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, mode);
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE2, "null");
    }
    else {
        if (!strcmp(finalMode, MODE_480CVBS) || !strcmp(finalMode, MODE_576CVBS)) {
            //close deepcolor if HDMI not plugged in, because the next TV maybe not support deepcolor
            pSysWrite->setProperty(PROP_DEEPCOLOR, "false");
            cvbsMode = true;
        }

        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, finalMode);
    }

    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, mDisplayWidth - 1, mDisplayHeight - 1);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_AXIS, axis);

    sprintf(axis, "%d %d %d %d",
            outputx, outputy, outputx + outputwidth - 1, outputy + outputheight -1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);
    setVideoPlayingAxis();

    SYS_LOGI("setMboxOutputMode cvbsMode = %d\n", cvbsMode);
    if (OUPUT_MODE_STATE_INIT != state) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn off TMDS PHY */
   }
    pTxAuth->stop();
    //only HDMI mode need HDCP authenticate
    if (!cvbsMode) {
        pTxAuth->start();
    }

    if (OUPUT_MODE_STATE_INIT == state) {
        startBootanimDetectThread();
    } else {
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
        pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
        setOsdMouse(finalMode);
    }

#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    //audio
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

    if (OUPUT_MODE_STATE_INIT != state) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
    }

    setBootEnv(UBOOTENV_OUTPUTMODE, finalMode);
    if (strstr(finalMode, "cvbs") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, finalMode);
    } else {
        setBootEnv(UBOOTENV_HDMIMODE, finalMode);
    }
    SYS_LOGI("set output mode:%s done\n", finalMode);
}

void DisplayMode::setDigitalMode(const char* mode) {
    if (mode == NULL) return;

    if (!strcmp("PCM", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "0");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("SPDIF passthrough", mode))  {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "1");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    } else if (!strcmp("HDMI passthrough", mode)) {
        pSysWrite->writeSysfs(AUDIO_DSP_DIGITAL_RAW, "2");
        pSysWrite->writeSysfs(AV_HDMI_CONFIG, "audio_on");
    }
}

void DisplayMode::setNativeWindowRect(int x, int y, int w, int h) {
    mNativeWinX = x;
    mNativeWinY = y;
    mNativeWinW = w;
    mNativeWinH = h;
}

void DisplayMode::setVideoPlayingAxis() {
    char currMode[MODE_LEN] = {0};
    int currPos[4] = {0};
    char videoPlaying[MODE_LEN] = {0};

    pSysWrite->readSysfs(SYSFS_VIDEO_LAYER_STATE, videoPlaying);
    if (videoPlaying[0] == '0') {
        SYS_LOGI("video is not playing, don't need set video axis\n");
        return;
    }

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, currMode);
    getPosition(currMode, currPos);

    SYS_LOGD("set video playing axis currMode:%s\n", currMode);
    //need base as display width and height
    float scaleW = (float)currPos[2]/mDisplayWidth;
    float scaleH = (float)currPos[3]/mDisplayHeight;

    //scale down or up the native window position
    int outputx = currPos[0] + mNativeWinX*scaleW;
    int outputy = currPos[1] + mNativeWinY*scaleH;
    int outputwidth = mNativeWinW*scaleW;
    int outputheight = mNativeWinH*scaleH;

    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            outputx, outputy, outputx + outputwidth - 1, outputy + outputheight - 1);
    SYS_LOGD("write %s: %s\n", SYSFS_VIDEO_AXIS, axis);
    pSysWrite->writeSysfs(SYSFS_VIDEO_AXIS, axis);
}

int DisplayMode::readHdcpRX22Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.getHdcpRX22key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX22Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 2.2 key \n");
    HDCPRxKey key22(HDCP_RX_22_KEY);
    int ret = key22.setHdcpRX22key(value, size);
    if (ret == 0)
        return true;
    else
        return false;
}

int DisplayMode::readHdcpRX14Key(char *value, int size) {
    SYS_LOGI("read HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.getHdcpRX14key(value, size);
    return ret;
}

bool DisplayMode::writeHdcpRX14Key(const char *value, const int size) {
    SYS_LOGI("write HDCP rx 1.4 key \n");
    HDCPRxKey key14(HDCP_RX_14_KEY);
    int ret = key14.setHdcpRX14key(value,size);
    if (ret == 0)
        return true;
    else
        return false;
}

bool DisplayMode::writeHdcpRXImg(const char *path) {
    SYS_LOGI("write HDCP key from Img \n");
    int ret = setImgPath(path);
    if (ret == 0)
        return true;
    else
        return false;
}


//get the best hdmi mode by edid
void DisplayMode::getBestHdmiMode(char* mode, hdmi_data_t* data) {
    char* pos = strchr(data->edid, '*');
    if (pos != NULL) {
        char* findReturn = pos;
        while (*findReturn != 0x0a && findReturn >= data->edid) {
            findReturn--;
        }
        //*pos = 0;
        //strcpy(mode, findReturn + 1);

        findReturn = findReturn + 1;
        strncpy(mode, findReturn, pos - findReturn);
        SYS_LOGI("set HDMI to best edid mode: %s\n", mode);
    }

    if (strlen(mode) == 0) {
        pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
    }

  /*
    char* arrayMode[MAX_STR_LEN] = {0};
    char* tmp;

    int len = strlen(data->edid);
    tmp = data->edid;
    int i = 0;

    do {
        if (strlen(tmp) == 0)
            break;
        char* pos = strchr(tmp, 0x0a);
        *pos = 0;

        arrayMode[i] = tmp;
        tmp = pos + 1;
        i++;
    } while (tmp <= data->edid + len -1);

    for (int j = 0; j < i; j++) {
        char* pos = strchr(arrayMode[j], '*');
        if (pos != NULL) {
            *pos = 0;
            strcpy(mode, arrayMode[j]);
            break;
        }
    }*/
}

//get the highest hdmi mode by edid
void DisplayMode::getHighestHdmiMode(char* mode, hdmi_data_t* data) {
    const char PMODE = 'p';
    const char IMODE = 'i';
    const char* FREQ = "hz";
    int lenmode = 0, intmode = 0, higmode = 0;
    char value[MODE_LEN] = {0};
    char* type;
    char* start;
    char* pos = data->edid;
    do {
        pos = strstr(pos, FREQ);
        if (pos == NULL) break;
        start = pos;
        while (*start != '\n' && start >= data->edid) {
            start--;
        }
        start++;
        int len = pos - start;
        strncpy(value, start, len);
        pos = strstr(pos, "\n");

        if ((type = strchr(value, PMODE)) != NULL && type - value >= 3) {
            value[type - value] = '1';
        } else if ((type = strchr(value, IMODE)) != NULL) {
            value[type - value] = '0';
        } else {
            continue;
        }
        value[len] = '\0';

        if ((intmode = atoi(value)) >= higmode) {
            len = pos - start;
            if (intmode == higmode && lenmode >= len) continue;
            lenmode = len;
            higmode = intmode;
            strncpy(mode, start, len);
            if (mode[len - 1] == '*')  mode[len - 1] = '\0';
            else mode[len] = '\0';
        }
    } while (strlen(pos) > 0);

    if (higmode == 0) {
        pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
    }

    SYS_LOGI("set HDMI to highest edid mode: %s\n", mode);
}

//check if the edid support current hdmi mode
void DisplayMode::filterHdmiMode(char* mode, hdmi_data_t* data) {
    char *pCmp = data->edid;
    while ((pCmp - data->edid) < (int)strlen(data->edid)) {
        char *pos = strchr(pCmp, 0x0a);
        if (NULL == pos)
            break;

        int step = 1;
        if (*(pos - 1) == '*') {
            pos -= 1;
            step += 1;
        }
        if (!strncmp(pCmp, data->ubootenv_hdmimode, pos - pCmp)) {
            strcpy(mode, data->ubootenv_hdmimode);
            return;
        }
        pCmp = pos + step;
    }
    if (DISPLAY_TYPE_TV == mDisplayType) {
        #ifdef TEST_UBOOT_MODE
            getBootEnv(UBOOTENV_TESTMODE, mode);
            if (strlen(mode) != 0)
               return;
        #endif
    }
    //old mode is not support in this TV, so switch to best mode.
#ifdef USE_BEST_MODE
    getBestHdmiMode(mode, data);
#else
    getHighestHdmiMode(mode, data);
#endif
}

void DisplayMode::standardMode(char* mode) {
    char* p;
    if ((p = strstr(mode, SUFFIX_10BIT)) != NULL) {
    } else if ((p = strstr(mode, SUFFIX_12BIT)) != NULL) {
    } else if ((p = strstr(mode, SUFFIX_14BIT)) != NULL) {
    } else if ((p = strstr(mode, SUFFIX_RGB)) != NULL) {
    }

    if (p != NULL) {
        memset(p, 0, strlen(p));
    }
}

void DisplayMode::addSuffixForMode(char* mode, output_mode_state state) {
    char save_mode[MODE_LEN] = {0};

    strcpy(save_mode, mode);
    standardMode(mode);
    int index = modeToIndex(mode);
    //only support 4 modes for 10bit now
    switch (index) {
        case DISPLAY_MODE_4K2K24HZ:
        case DISPLAY_MODE_4K2K30HZ:
        case DISPLAY_MODE_4K2K50HZ420:
        case DISPLAY_MODE_4K2K60HZ420:
            if (OUPUT_MODE_STATE_INIT != state)
                pSysWrite->writeSysfs(DISPLAY_HDMI_VIC, "0");
            if (isDeepColor()) {
#if 0
                char deepColor[MAX_STR_LEN];
                pSysWrite->readSysfsOriginal(DISPLAY_HDMI_DEEP_COLOR, deepColor);
                char *pCmp = deepColor;
                while ((pCmp - deepColor) < (int)strlen(deepColor)) {
                    char *pos = strchr(pCmp, 0x0a);
                    if (NULL == pos) {
                        break;
                    } else {
                        *pos = 0;
                    }
                    if ((strstr(pCmp, "420") != NULL && strstr(pCmp, SUFFIX_10BIT) != NULL && strstr(mode, "420") != NULL)
                            ||(strstr(pCmp, "422") != NULL && strstr(pCmp, SUFFIX_10BIT) != NULL && strstr(mode, "422") != NULL)
                            ||(strstr(pCmp, "444") != NULL && strstr(pCmp, SUFFIX_10BIT) != NULL)) {
                        strcat(mode, SUFFIX_10BIT);
                        break;
                    }
                    *pos = 0x0a;
                    pCmp = pos + 1;
                }
#else
                strcat(mode, SUFFIX_10BIT);
#endif
            }
            break;
      case DISPLAY_MODE_4K2K50HZ422:
            strcat(mode, SUFFIX_10BIT);
            break;
      case DISPLAY_MODE_4K2K60HZ422:
            strcat(mode, SUFFIX_10BIT);
            break;
    }
}

void DisplayMode::getHdmiOutputMode(char* mode, hdmi_data_t* data) {
    if (strstr(data->edid, "null") != NULL) {
        pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
        return;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (isBestOutputmode()) {
        #ifdef USE_BEST_MODE
            getBestHdmiMode(mode, data);
        #else
            getHighestHdmiMode(mode, data);
        #endif
        } else {
            filterHdmiMode(mode, data);
        }
    }
    SYS_LOGI("set HDMI mode to %s\n", mode);
}

void DisplayMode::initHdmiData(hdmi_data_t* data, char* hpdstate){
    if (hpdstate == NULL) {
        pSysWrite->readSysfs(DISPLAY_HPD_STATE, data->hpd_state);
    } else {
        strcpy(data->hpd_state, hpdstate);
    }

    if (!strcmp(data->hpd_state, "1")) {
        int count = 0;
        while (true) {
            pSysWrite->readSysfsOriginal(DISPLAY_HDMI_EDID, data->edid);
            if (strlen(data->edid) > 0)
                break;

            if (count >= 5) {
                strcpy(data->edid, "null edid");
                break;
            }
            count++;
            usleep(500000);
        }
    }
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, data->current_mode);
    getBootEnv(UBOOTENV_HDMIMODE, data->ubootenv_hdmimode);
    standardMode(data->ubootenv_hdmimode);
}

void DisplayMode::startBootanimDetectThread() {
    pthread_t id;
    int ret = pthread_create(&id, NULL, bootanimDetect, this);
    if (ret != 0) {
        SYS_LOGE("Create BootanimDetect error!\n");
    }
}

//if detected bootanim is running, then close uboot logo
void* DisplayMode::bootanimDetect(void* data) {
    DisplayMode *pThiz = (DisplayMode*)data;
    char bootanimState[MODE_LEN] = {"stopped"};
    char fs_mode[MODE_LEN] = {0};
    char outputmode[MODE_LEN] = {0};
    char bootvideo[MODE_LEN] = {0};

    pThiz->pSysWrite->getPropertyString(PROP_FS_MODE, fs_mode, "android");
    pThiz->pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, outputmode);

    //not in the recovery mode
    if (strcmp(fs_mode, "recovery")) {
        //some boot videos maybe need 2~3s to start playing, so if the bootamin property
        //don't run after about 4s, exit the loop.
        int timeout = 40;
        while (timeout > 0) {
            //init had started boot animation, will set init.svc.* running
            pThiz->pSysWrite->getPropertyString(PROP_BOOTANIM, bootanimState, "stopped");
            if (!strcmp(bootanimState, "running"))
                break;

            usleep(100000);
            timeout--;
        }

        int delayMs = pThiz->pSysWrite->getPropertyInt(PROP_BOOTANIM_DELAY, 100);
        usleep(delayMs * 1000);
        usleep(1000 * 1000);
    }

    pThiz->pSysWrite->writeSysfs(DISPLAY_LOGO_INDEX, "-1");
    pThiz->pSysWrite->getPropertyString(PROP_BOOTVIDEO_SERVICE, bootvideo, "0");
    SYS_LOGI("boot animation detect boot video:%s\n", bootvideo);
    if ((!strcmp(fs_mode, "recovery")) || (!strcmp(bootvideo, "1"))) {
        //recovery or bootvideo mode
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        //need close fb1, because uboot logo show in fb1
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
        //not boot video running
        if (strcmp(bootvideo, "1")) {
            //open fb0, let bootanimation show in it
            pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
        }
    } else {
        pThiz->pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_SWTICH, "0x10001");
    }

    pThiz->setOsdMouse(outputmode);
    pThiz->pTxAuth->setBootAnimFinished(true);
    return NULL;
}

//get edid crc value to check edid change
bool DisplayMode::isEdidChange() {
    char edid[MAX_STR_LEN] = {0};
    char crcvalue[MAX_STR_LEN] = {0};
    unsigned int crcheadlength = strlen(DEFAULT_EDID_CRCHEAD);
    pSysWrite->readSysfs(DISPLAY_EDID_VALUE, edid);
    char *p = strstr(edid, DEFAULT_EDID_CRCHEAD);
    if (p != NULL && strlen(p) > crcheadlength) {
        p += crcheadlength;
        if (!getBootEnv(UBOOTENV_EDIDCRCVALUE, crcvalue) || strncmp(p, crcvalue, strlen(p))) {
            setBootEnv(UBOOTENV_EDIDCRCVALUE, p);
            return true;
        }
    }
    return false;
}

bool DisplayMode::isBestOutputmode() {
    char isBestMode[MODE_LEN] = {0};
    if (DISPLAY_TYPE_TV == mDisplayType) {
        return false;
    }
    return !getBootEnv(UBOOTENV_ISBESTMODE, isBestMode) || strcmp(isBestMode, "true") == 0;
}

bool DisplayMode::isDeepColor() {
    char isDeepColor[MODE_LEN] = {0};
    return pSysWrite->getProperty(PROP_DEEPCOLOR, isDeepColor) && strcmp(isDeepColor, "true") == 0;
}

//this function only running in bootup time
void DisplayMode::setTVOutputMode(const char* outputmode, bool initState) {
    int outputx = 0;
    int outputy = 0;
    int outputwidth = 0;
    int outputheight = 0;
    int position[4] = { 0, 0, 0, 0 };

    getPosition(outputmode, position);
    outputx = position[0];
    outputy = position[1];
    outputwidth = position[2];
    outputheight = position[3];

    pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, outputmode);
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, mDisplayWidth - 1, mDisplayHeight - 1);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_AXIS, axis);

    sprintf(axis, "%d %d %d %d",
            outputx, outputy, outputx + outputwidth - 1, outputy + outputheight -1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);

    if (initState)
        startBootanimDetectThread();
    else {
        pSysWrite->writeSysfs(DISPLAY_LOGO_INDEX, "-1");
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        //need close fb1, because uboot logo show in fb1
        pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
        pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
        setOsdMouse(outputmode);
    }
}

void DisplayMode::setTVDisplay(bool initState) {
    char current_mode[MODE_LEN] = {0};
    char outputmode[MODE_LEN] = {0};

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, current_mode);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    SYS_LOGD("init tv display old outputmode:%s, outputmode:%s\n", current_mode, outputmode);

    if (strlen(outputmode) == 0)
        strcpy(outputmode, mDefaultUI);

    if (!strncmp(mDefaultUI, "720", 3)) {
        mDisplayWidth= FULL_WIDTH_720;
        mDisplayHeight = FULL_HEIGHT_720;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_720P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1280");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "720");
    } else if (!strncmp(mDefaultUI, "1080", 4)) {
        mDisplayWidth = FULL_WIDTH_1080;
        mDisplayHeight = FULL_HEIGHT_1080;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_1080P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1920");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "1080");
    } else if (!strncmp(mDefaultUI, "4k2k", 4)) {
        mDisplayWidth = FULL_WIDTH_1080;
        mDisplayHeight = FULL_HEIGHT_1080;
        //pSysWrite->setProperty(PROP_LCD_DENSITY, DESITY_1080P);
        //pSysWrite->setProperty(PROP_WINDOW_WIDTH, "1920");
        //pSysWrite->setProperty(PROP_WINDOW_HEIGHT, "1080");
    }
    if (strcmp(current_mode, outputmode)) {
        //when change mode, need close uboot logo to avoid logo scaling wrong
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
    }

    setTVOutputMode(outputmode, initState);
}

void DisplayMode::setFbParameter(const char* fbdev, struct fb_var_screeninfo var_set) {
    struct fb_var_screeninfo var_old;

    int fh = open(fbdev, O_RDONLY);
    ioctl(fh, FBIOGET_VSCREENINFO, &var_old);

    copy_changed_values(&var_old, &var_set);
    ioctl(fh, FBIOPUT_VSCREENINFO, &var_old);
    close(fh);
}

int DisplayMode::getBootenvInt(const char* key, int defaultVal) {
    int value = defaultVal;
    const char* p_value = bootenv_get(key);
    if (p_value) {
        value = atoi(p_value);
    }
    return value;
}

void DisplayMode::setOsdMouse(const char* curMode) {
    //SYS_LOGI("set osd mouse mode: %s", curMode);

    int position[4] = { 0, 0, 0, 0 };
    getPosition(curMode, position);
    setOsdMouse(position[0], position[1], position[2], position[3]);
}

void DisplayMode::setOsdMouse(int x, int y, int w, int h) {
    SYS_LOGI("set osd mouse x:%d y:%d w:%d h:%d", x, y, w, h);

    const char* displaySize = "1920 1080";
    int display_w, display_h;
    if (!strncmp(mDefaultUI, "720", 3)) {
        displaySize = "1280 720";
    } else if (!strncmp(mDefaultUI, "1080", 4)) {
        displaySize = "1920 1080";
    } else if (!strncmp(mDefaultUI, "4k2k", 4)) {
        displaySize = "3840 2160";
    }

    char cur_mode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, cur_mode);
    if (!strcmp(cur_mode, MODE_480I) || !strcmp(cur_mode, MODE_576I) ||
            !strcmp(cur_mode, MODE_480CVBS) || !strcmp(cur_mode, MODE_576CVBS) ||
            !strcmp(cur_mode, MODE_1080I50HZ) || !strcmp(cur_mode, MODE_1080I)) {
        y /= 2;
        h /= 2;
    }

    char axis[512] = {0};
    sprintf(axis, "%d %d %s %d %d 18 18", x, y, displaySize, x, y);
    pSysWrite->writeSysfs(SYSFS_DISPLAY_AXIS, axis);

    sprintf(axis, "%s %d %d", displaySize, w, h);
    sscanf(displaySize,"%d %d",&display_w,&display_h);
    pSysWrite->writeSysfs(DISPLAY_FB1_SCALE_AXIS, axis);
    if ((display_w != w) || (display_h != h)) {
        pSysWrite->writeSysfs(DISPLAY_FB1_SCALE, "0x10001");
    } else {
        pSysWrite->writeSysfs(DISPLAY_FB1_SCALE, "0");
    }
}

void DisplayMode::getPosition(const char* curMode, int *position) {
    char std_mode[MODE_LEN] = {0};
    strcpy(std_mode, curMode);
    standardMode(std_mode);

    int index = modeToIndex(std_mode);
    switch (index) {
        case DISPLAY_MODE_480I:
        case DISPLAY_MODE_480CVBS: // 480cvbs
            position[0] = getBootenvInt(ENV_480I_X, 0);
            position[1] = getBootenvInt(ENV_480I_Y, 0);
            position[2] = getBootenvInt(ENV_480I_W, FULL_WIDTH_480);
            position[3] = getBootenvInt(ENV_480I_H, FULL_HEIGHT_480);
            break;
        case DISPLAY_MODE_480P: // 480p
            position[0] = getBootenvInt(ENV_480P_X, 0);
            position[1] = getBootenvInt(ENV_480P_Y, 0);
            position[2] = getBootenvInt(ENV_480P_W, FULL_WIDTH_480);
            position[3] = getBootenvInt(ENV_480P_H, FULL_HEIGHT_480);
            break;
        case DISPLAY_MODE_576I: // 576i
        case DISPLAY_MODE_576CVBS: // 576cvbs
            position[0] = getBootenvInt(ENV_576I_X, 0);
            position[1] = getBootenvInt(ENV_576I_Y, 0);
            position[2] = getBootenvInt(ENV_576I_W, FULL_WIDTH_576);
            position[3] = getBootenvInt(ENV_576I_H, FULL_HEIGHT_576);
            break;
        case DISPLAY_MODE_576P: // 576p
            position[0] = getBootenvInt(ENV_576P_X, 0);
            position[1] = getBootenvInt(ENV_576P_Y, 0);
            position[2] = getBootenvInt(ENV_576P_W, FULL_WIDTH_576);
            position[3] = getBootenvInt(ENV_576P_H, FULL_HEIGHT_576);
            break;
        case DISPLAY_MODE_720P: // 720p
        case DISPLAY_MODE_720P50HZ: // 720p50hz
            position[0] = getBootenvInt(ENV_720P_X, 0);
            position[1] = getBootenvInt(ENV_720P_Y, 0);
            position[2] = getBootenvInt(ENV_720P_W, FULL_WIDTH_720);
            position[3] = getBootenvInt(ENV_720P_H, FULL_HEIGHT_720);
            break;
        case DISPLAY_MODE_1080I: // 1080i
        case DISPLAY_MODE_1080I50HZ: // 1080i50hz
            position[0] = getBootenvInt(ENV_1080I_X, 0);
            position[1] = getBootenvInt(ENV_1080I_Y, 0);
            position[2] = getBootenvInt(ENV_1080I_W, FULL_WIDTH_1080);
            position[3] = getBootenvInt(ENV_1080I_H, FULL_HEIGHT_1080);
            break;
        case DISPLAY_MODE_1080P: // 1080p
        case DISPLAY_MODE_1080P50HZ: // 1080p50hz
        case DISPLAY_MODE_1080P24HZ://1080p24hz
            position[0] = getBootenvInt(ENV_1080P_X, 0);
            position[1] = getBootenvInt(ENV_1080P_Y, 0);
            position[2] = getBootenvInt(ENV_1080P_W, FULL_WIDTH_1080);
            position[3] = getBootenvInt(ENV_1080P_H, FULL_HEIGHT_1080);
            break;
        case DISPLAY_MODE_4K2K24HZ: // 4k2k24hz
            position[0] = getBootenvInt(ENV_4K2K24HZ_X, 0);
            position[1] = getBootenvInt(ENV_4K2K24HZ_Y, 0);
            position[2] = getBootenvInt(ENV_4K2K24HZ_W, FULL_WIDTH_4K2K);
            position[3] = getBootenvInt(ENV_4K2K24HZ_H, FULL_HEIGHT_4K2K);
            break;
        case DISPLAY_MODE_4K2K25HZ: // 4k2k25hz
            position[0] = getBootenvInt(ENV_4K2K25HZ_X, 0);
            position[1] = getBootenvInt(ENV_4K2K25HZ_Y, 0);
            position[2] = getBootenvInt(ENV_4K2K25HZ_W, FULL_WIDTH_4K2K);
            position[3] = getBootenvInt(ENV_4K2K25HZ_H, FULL_HEIGHT_4K2K);
            break;
        case DISPLAY_MODE_4K2K30HZ: // 4k2k30hz
            position[0] = getBootenvInt(ENV_4K2K30HZ_X, 0);
            position[1] = getBootenvInt(ENV_4K2K30HZ_Y, 0);
            position[2] = getBootenvInt(ENV_4K2K30HZ_W, FULL_WIDTH_4K2K);
            position[3] = getBootenvInt(ENV_4K2K30HZ_H, FULL_HEIGHT_4K2K);
            break;
        case DISPLAY_MODE_4K2K50HZ: // 4k2k50hz
        case DISPLAY_MODE_4K2K50HZ420: // 4k2k50hz420
        case DISPLAY_MODE_4K2K50HZ422: // 4k2k50hz422
            position[0] = getBootenvInt(ENV_4K2K50HZ_X, 0);
            position[1] = getBootenvInt(ENV_4K2K50HZ_Y, 0);
            position[2] = getBootenvInt(ENV_4K2K50HZ_W, FULL_WIDTH_4K2K);
            position[3] = getBootenvInt(ENV_4K2K50HZ_H, FULL_HEIGHT_4K2K);
            break;
        case DISPLAY_MODE_4K2K60HZ: // 4k2k60hz
        case DISPLAY_MODE_4K2K60HZ420: // 4k2k60hz420
        case DISPLAY_MODE_4K2K60HZ422: // 4k2k60hz422
            position[0] = getBootenvInt(ENV_4K2K60HZ_X, 0);
            position[1] = getBootenvInt(ENV_4K2K60HZ_Y, 0);
            position[2] = getBootenvInt(ENV_4K2K60HZ_W, FULL_WIDTH_4K2K);
            position[3] = getBootenvInt(ENV_4K2K60HZ_H, FULL_HEIGHT_4K2K);
            break;
        case DISPLAY_MODE_4K2KSMPTE: // 4k2ksmpte
        case DISPLAY_MODE_4K2KSMPTE30HZ: // 4k2ksmpte30hz
        case DISPLAY_MODE_4K2KSMPTE50HZ: // 4k2ksmpte50hz
        case DISPLAY_MODE_4K2KSMPTE50HZ420: // 4k2ksmpte50hz420
        case DISPLAY_MODE_4K2KSMPTE60HZ: // 4k2ksmpte60hz
        case DISPLAY_MODE_4K2KSMPTE60HZ420: // 4k2ksmpte60hz320
            position[0] = getBootenvInt(ENV_4K2KSMPTE_X, 0);
            position[1] = getBootenvInt(ENV_4K2KSMPTE_Y, 0);
            position[2] = getBootenvInt(ENV_4K2KSMPTE_W, FULL_WIDTH_4K2KSMPTE);
            position[3] = getBootenvInt(ENV_4K2KSMPTE_H, FULL_HEIGHT_4K2KSMPTE);
            break;
        default: //1080p
            position[0] = getBootenvInt(ENV_1080P_X, 0);
            position[1] = getBootenvInt(ENV_1080P_Y, 0);
            position[2] = getBootenvInt(ENV_1080P_W, FULL_WIDTH_1080);
            position[3] = getBootenvInt(ENV_1080P_H, FULL_HEIGHT_1080);
            break;
    }
}
void DisplayMode::isHDCPTxAuthSuccess(int *status) {

    pTxAuth->isAuthSuccess(status);
}
void DisplayMode::setPosition(int left, int top, int width, int height) {
    char x[512] = {0};
    char y[512] = {0};
    char w[512] = {0};
    char h[512] = {0};
    sprintf(x, "%d", left);
    sprintf(y, "%d", top);
    sprintf(w, "%d", width);
    sprintf(h, "%d", height);

    char curMode[MODE_LEN] = {0};
    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curMode);
    standardMode(curMode);
    int index = modeToIndex(curMode);
    switch (index) {
        case DISPLAY_MODE_480I: // 480i
        case DISPLAY_MODE_480CVBS: //480cvbs
            setBootEnv(ENV_480I_X, x);
            setBootEnv(ENV_480I_Y, y);
            setBootEnv(ENV_480I_W, w);
            setBootEnv(ENV_480I_H, h);
            break;
        case DISPLAY_MODE_480P: // 480p
            setBootEnv(ENV_480P_X, x);
            setBootEnv(ENV_480P_Y, y);
            setBootEnv(ENV_480P_W, w);
            setBootEnv(ENV_480P_H, h);
            break;
        case DISPLAY_MODE_576I: // 576i
        case DISPLAY_MODE_576CVBS:    //576cvbs
            setBootEnv(ENV_576I_X, x);
            setBootEnv(ENV_576I_Y, y);
            setBootEnv(ENV_576I_W, w);
            setBootEnv(ENV_576I_H, h);
            break;
        case DISPLAY_MODE_576P: // 576p
            setBootEnv(ENV_576P_X, x);
            setBootEnv(ENV_576P_Y, y);
            setBootEnv(ENV_576P_W, w);
            setBootEnv(ENV_576P_H, h);
            break;
        case DISPLAY_MODE_720P: // 720p
        case DISPLAY_MODE_720P50HZ: // 720p50hz
            setBootEnv(ENV_720P_X, x);
            setBootEnv(ENV_720P_Y, y);
            setBootEnv(ENV_720P_W, w);
            setBootEnv(ENV_720P_H, h);
            break;
        case DISPLAY_MODE_1080I: // 1080i
        case DISPLAY_MODE_1080I50HZ: // 1080i50hz
            setBootEnv(ENV_1080I_X, x);
            setBootEnv(ENV_1080I_Y, y);
            setBootEnv(ENV_1080I_W, w);
            setBootEnv(ENV_1080I_H, h);
            break;
        case DISPLAY_MODE_1080P: // 1080p
        case DISPLAY_MODE_1080P50HZ: // 1080p50hz
        case DISPLAY_MODE_1080P24HZ: //1080p24hz
            setBootEnv(ENV_1080P_X, x);
            setBootEnv(ENV_1080P_Y, y);
            setBootEnv(ENV_1080P_W, w);
            setBootEnv(ENV_1080P_H, h);
            break;
        case DISPLAY_MODE_4K2K24HZ:      //4k2k24hz
            setBootEnv(ENV_4K2K24HZ_X, x);
            setBootEnv(ENV_4K2K24HZ_Y, y);
            setBootEnv(ENV_4K2K24HZ_W, w);
            setBootEnv(ENV_4K2K24HZ_H, h);
            break;
        case DISPLAY_MODE_4K2K25HZ:    //4k2k25hz
            setBootEnv(ENV_4K2K25HZ_X, x);
            setBootEnv(ENV_4K2K25HZ_Y, y);
            setBootEnv(ENV_4K2K25HZ_W, w);
            setBootEnv(ENV_4K2K25HZ_H, h);
            break;
        case DISPLAY_MODE_4K2K30HZ:    //4k2k30hz
            setBootEnv(ENV_4K2K30HZ_X, x);
            setBootEnv(ENV_4K2K30HZ_Y, y);
            setBootEnv(ENV_4K2K30HZ_W, w);
            setBootEnv(ENV_4K2K30HZ_H, h);
            break;
        case DISPLAY_MODE_4K2K50HZ:    //4k2k50hz
        case DISPLAY_MODE_4K2K50HZ420: //4k2k50hz420
        case DISPLAY_MODE_4K2K50HZ422: //4k2k50hz422
            setBootEnv(ENV_4K2K50HZ_X, x);
            setBootEnv(ENV_4K2K50HZ_Y, y);
            setBootEnv(ENV_4K2K50HZ_W, w);
            setBootEnv(ENV_4K2K50HZ_H, h);
            break;
        case DISPLAY_MODE_4K2K60HZ:    //4k2k60hz
        case DISPLAY_MODE_4K2K60HZ420: //4k2k60hz420
        case DISPLAY_MODE_4K2K60HZ422: //4k2k60hz422
            setBootEnv(ENV_4K2K60HZ_X, x);
            setBootEnv(ENV_4K2K60HZ_Y, y);
            setBootEnv(ENV_4K2K60HZ_W, w);
            setBootEnv(ENV_4K2K60HZ_H, h);
            break;
        case DISPLAY_MODE_4K2KSMPTE:    //4k2ksmpte
        case DISPLAY_MODE_4K2KSMPTE30HZ: // 4k2ksmpte30hz
        case DISPLAY_MODE_4K2KSMPTE50HZ: // 4k2ksmpte50hz
        case DISPLAY_MODE_4K2KSMPTE50HZ420: // 4k2ksmpte50hz420
        case DISPLAY_MODE_4K2KSMPTE60HZ: // 4k2ksmpte60hz
        case DISPLAY_MODE_4K2KSMPTE60HZ420: // 4k2ksmpte60hz320
            setBootEnv(ENV_4K2KSMPTE_X, x);
            setBootEnv(ENV_4K2KSMPTE_Y, y);
            setBootEnv(ENV_4K2KSMPTE_W, w);
            setBootEnv(ENV_4K2KSMPTE_H, h);
            break;

        default:
            break;
    }
}

int DisplayMode::modeToIndex(const char *mode) {
    int index = DISPLAY_MODE_1080P;
    for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
        if (!strcmp(mode, DISPLAY_MODE_LIST[i])) {
            index = i;
            break;
        }
    }

    //SYS_LOGI("modeToIndex mode:%s index:%d", mode, index);
    return index;
}

void DisplayMode::onTxEvent (char* hpdstate, int outputState) {

    SYS_LOGI("onTxEvent hpdstate:%s state: %d\n", hpdstate, outputState);
    setMboxDisplay(hpdstate, (output_mode_state)outputState);
}

void DisplayMode::onDispModeSyncEvent (const char* outputmode, int state) {
    SYS_LOGI("onDispModeSyncEvent outputmode:%s state: %d\n", outputmode, state);
    setMboxOutputMode(outputmode, (output_mode_state)state);
}

//for debug
void DisplayMode::hdcpSwitch() {
    SYS_LOGI("hdcpSwitch for debug hdcp authenticate\n");
}

#ifndef RECOVERY_MODE
void DisplayMode::notifyEvent(int event) {
    if (mNotifyListener != NULL) {
        mNotifyListener->onEvent(event);
    }
}

void DisplayMode::setListener(const sp<ISystemControlNotify>& listener) {
    mNotifyListener = listener;
}
#endif

int DisplayMode::dump(char *result) {
    if (NULL == result)
        return -1;

    char buf[2048] = {0};
    sprintf(buf, "\ndisplay type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s\n", mDisplayType, mSocType);
    strcat(result, buf);

    if (DISPLAY_TYPE_TABLET == mDisplayType) {
        sprintf(buf, "fb0 width:%d height:%d fbbits:%d triple buffer enable:%d\n",
            mFb0Width, mFb0Height, mFb0FbBits, (int)mFb0TripleEnable);
        strcat(result, buf);

        sprintf(buf, "fb1 width:%d height:%d fbbits:%d triple buffer enable:%d\n",
            mFb1Width, mFb1Height, mFb1FbBits, (int)mFb1TripleEnable);
        strcat(result, buf);
    }

    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        sprintf(buf, "default ui:%s\n", mDefaultUI);
        strcat(result, buf);
    }
    return 0;
}

