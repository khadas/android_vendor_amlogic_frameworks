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

// Sink reference table, sorted by priority, per CDF
static const char* MODES_SINK[] = {
    "2160p60hz420",
    "2160p60hz",
    "2160p50hz420",
    "2160p50hz",
    "2160p30hz",
    "2160p25hz",
    "2160p24hz",
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
};

// Repeater reference table, sorted by priority, per CDF
static const char* MODES_REPEATER[] = {
    "1080p60hz",
    "1080p50hz",
    "1080p30hz",
    "1080p25hz",
    "1080p24hz",
    "720p60hz",
    "720p50hz",
    "480p60hz",
    "576p50hz",
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

DisplayMode::DisplayMode(const char *path) {
    DisplayMode(path, NULL);
}

DisplayMode::DisplayMode(const char *path, Ubootenv *ubootenv)
    :mDisplayType(DISPLAY_TYPE_MBOX),
    mDisplayWidth(FULL_WIDTH_1080),
    mDisplayHeight(FULL_HEIGHT_1080),
    mLogLevel(LOG_LEVEL_DEFAULT) {

    if (NULL == path) {
        pConfigPath = DISPLAY_CFG_FILE;
    }
    else {
        pConfigPath = path;
    }

    if (NULL == ubootenv)
        mUbootenv = new Ubootenv();
    else
        mUbootenv = ubootenv;

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
    if (DISPLAY_TYPE_MBOX == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
        dumpCaps();
    } else if (DISPLAY_TYPE_TV == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        pRxAuth = new HDCPRxAuth(pTxAuth);
        setSinkDisplay(true);
    } else if (DISPLAY_TYPE_REPEATER == mDisplayType) {
        pTxAuth = new HDCPTxAuth();
        pTxAuth->setUevntCallback(this);
        pTxAuth->setFRAutoAdpt(new FrameRateAutoAdaption(this));
        pRxAuth = new HDCPRxAuth(pTxAuth);
        setSourceDisplay(OUPUT_MODE_STATE_INIT);
        dumpCaps();
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
        if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
            setSourceDisplay(OUPUT_MODE_STATE_POWER);
        } else if (DISPLAY_TYPE_TV == mDisplayType) {
            setSinkDisplay(false);
        }
    }

    SYS_LOGI("open osd0 and disable video\n");
    pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_AUTO_ENABLE);
    pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
}

HDCPTxAuth *DisplayMode:: geTxAuth() {
    return pTxAuth;
}

void DisplayMode::setLogLevel(int level){
    mLogLevel = level;
}

bool DisplayMode::getBootEnv(const char* key, char* value) {
    const char* p_value = mUbootenv->getValue(key);

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

    mUbootenv->updateValue(key, value);
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
                if (!strcmp(token, DEVICE_STR_MBOX)) {
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
    //if TVSOC as Mbox, change mDisplayType to DISPLAY_TYPE_REPEATER. and it will be in REPEATER process.
    if ((DISPLAY_TYPE_TV == mDisplayType) && (pSysWrite->getPropertyBoolean(PROP_TVSOC_AS_MBOX, false))) {
        mDisplayType = DISPLAY_TYPE_REPEATER;
    }
    return status;
}

void DisplayMode::setSourceDisplay(output_mode_state state) {
    hdmi_data_t data;
    char outputmode[MODE_LEN] = {0};

    pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_DISABLE);

    memset(&data, 0, sizeof(hdmi_data_t));
    getHdmiData(&data);
    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (HDMI_SINK_TYPE_NONE != data.sinkType) {
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
    if (HDMI_SINK_TYPE_NONE == data.sinkType) {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "3");
        //if (strcmp(outputmode, MODE_480CVBS) && strcmp(outputmode, MODE_576CVBS))
        //    strcpy(outputmode, MODE_576CVBS);

        if (strlen(outputmode) == 0)
            strcpy(outputmode, "none");
    } else {
        pSysWrite->writeSysfs(H265_DOUBLE_WRITE_MODE, "0");
    }

    SYS_LOGI("display sink type:%d [0:none, 1:sink, 2:repeater], old outputmode:%s, new outputmode:%s\n",
            data.sinkType,
            data.current_mode,
            outputmode);
    if (strlen(outputmode) == 0)
        strcpy(outputmode, DEFAULT_OUTPUT_MODE);

    if (OUPUT_MODE_STATE_INIT == state) {
        updateDefaultUI();
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
    DetectDolbyVisionOutputMode(state, outputmode);
    setSourceOutputMode(outputmode, state);
}

void DisplayMode::setSourceOutputMode(const char* outputmode){
    setSourceOutputMode(outputmode, OUPUT_MODE_STATE_SWITCH);
}

void DisplayMode::setSourceOutputMode(const char* outputmode, output_mode_state state) {
    char value[MAX_STR_LEN] = {0};

    bool cvbsMode = false;

    if (!strcmp(outputmode, "auto")) {
        hdmi_data_t data;

        SYS_LOGI("outputmode is [auto] mode, need find the best mode\n");
        getHdmiData(&data);
        getHdmiOutputMode((char *)outputmode, &data);
    }

    bool deepColorEnabled = pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, false);
    pSysWrite->readSysfs(HDMI_TX_FRAMRATE_POLICY, value);
    if ((OUPUT_MODE_STATE_SWITCH == state) && (strcmp(value, "0") == 0)) {
        char curDisplayMode[MODE_LEN] = {0};

        pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, curDisplayMode);
        if (!strcmp(outputmode, curDisplayMode)) {
            //deep color disabled, only need check output mode same or not
            if (!deepColorEnabled) {
                SYS_LOGI("deep color is Disabled, and curDisplayMode is same to outputmode, return\n");
                return;
            }

            //deep color enabled, check the deep color same or not
            char curColorAttribute[MODE_LEN] = {0};
            char saveColorAttribute[MODE_LEN] = {0};
            pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, curColorAttribute);
            getBootEnv(UBOOTENV_COLORATTRIBUTE, saveColorAttribute);
            //if bestOutputmode is enable, need change deepcolor to best deepcolor.
            if (isBestOutputmode()) {
                FormatColorDepth deepColor;
                deepColor.getBestHdmiDeepColorAttr(outputmode, saveColorAttribute);
            }
            SYS_LOGI("curColorAttribute:[%s] ,saveColorAttribute: [%s]\n", curColorAttribute, saveColorAttribute);
            if (NULL != strstr(curColorAttribute, saveColorAttribute))
                return;
        }
    }
    // 1.set avmute and close phy
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

    // 2.stop hdcp tx
    pTxAuth->stop();


    //write framerate policy
    setAutoSwitchFrameRate(state);

    if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS)) {
        cvbsMode = true;
    }
    // 3. set deep color and outputmode
    updateDeepColor(cvbsMode, state, outputmode);
    pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, outputmode);

    //update free_scale_axis and window_axis
    updateFreeScaleAxis();
    updateWindowAxis(outputmode);

    initHdrSdrMode();

    if (!cvbsMode && (OUPUT_MODE_STATE_INIT == state) && isDolbyVisionEnable()) {
        setDolbyVisionEnable(DOLBY_VISION_SET_ENABLE);
    }

    if (0 == pSysWrite->getPropertyInt(PROP_BOOTCOMPLETE, 0)) {
        setVideoPlayingAxis();
    }

    SYS_LOGI("setMboxOutputMode cvbsMode = %d\n", cvbsMode);
    //4. turn on phy and clear avmute
    if (OUPUT_MODE_STATE_INIT != state && !cvbsMode) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_PHY, "1"); /* Turn on TMDS PHY */
        usleep(20000);
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "1");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AUDIO_MUTE, "0");
        pSysWrite->writeSysfs(DISPLAY_HDMI_AVMUTE, "-1");
    }

    //5. start HDMI HDCP authenticate
    if (!cvbsMode) {
        pTxAuth->start();
    }

    if (OUPUT_MODE_STATE_INIT == state) {
        startBootanimDetectThread();
    } else {
        pSysWrite->writeSysfs(SYS_DISABLE_VIDEO, VIDEO_LAYER_ENABLE);
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "0");
        pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE, "0x10001");
    }

#ifndef RECOVERY_MODE
    notifyEvent(EVENT_OUTPUT_MODE_CHANGE);
#endif

    //audio
    memset(value, 0, sizeof(0));
    getBootEnv(UBOOTENV_DIGITAUDIO, value);
    setDigitalMode(value);

    setBootEnv(UBOOTENV_OUTPUTMODE, (char *)outputmode);
    if (strstr(outputmode, "cvbs") != NULL) {
        setBootEnv(UBOOTENV_CVBSMODE, (char *)outputmode);
    } else {
        setBootEnv(UBOOTENV_HDMIMODE, (char *)outputmode);
    }
    SYS_LOGI("set output mode:%s done\n", outputmode);
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

void DisplayMode::setVideoPlayingAxis() {
    char currMode[MODE_LEN] = {0};
    int currPos[4] = {0};//x,y,w,h

    int videoPlaying = pSysWrite->getPropertyInt(PROP_BOOTVIDEO_SERVICE, 0);
    if (videoPlaying == 0) {
        SYS_LOGI("video is not playing, don't need set video axis\n");
        return;
    }

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, currMode);
    getPosition(currMode, currPos);

    SYS_LOGD("set video playing axis currMode:%s\n", currMode);

    //scale down or up the native window position
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            currPos[0], currPos[1], currPos[0] + currPos[2] - 1, currPos[1] + currPos[3] - 1);
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
    char value[MODE_LEN] = {0};
    char tempMode[MODE_LEN] = {0};

    char* startpos;
    char* destpos;

    startpos = data->edid;
    strcpy(value, DEFAULT_OUTPUT_MODE);

    while (strlen(startpos) > 0) {
        //get edid resolution to tempMode in order.
        destpos = strstr(startpos, "\n");
        if (NULL == destpos)
            break;
        memset(tempMode, 0, MODE_LEN);
        strncpy(tempMode, startpos, destpos - startpos);
        startpos = destpos + 1;
        if (!pSysWrite->getPropertyBoolean(PROP_SUPPORT_4K, true)
            &&(strstr(tempMode, "2160") || strstr(tempMode, "smpte"))) {
                SYS_LOGE("This platform not support : %s\n", tempMode);
            continue;
        }

        if (tempMode[strlen(tempMode) - 1] == '*') {
            tempMode[strlen(tempMode) - 1] = '\0';
        }

        if (resolveResolutionValue(tempMode) > resolveResolutionValue(value)) {
            memset(value, 0, MODE_LEN);
            strcpy(value, tempMode);
        }
    }
    strcpy(mode, value);
    SYS_LOGI("set HDMI to highest edid mode: %s\n", mode);
}

int64_t DisplayMode::resolveResolutionValue(const char *mode) {
    resolution_t resol_t;
    resolveResolution(mode, &resol_t);
    return resol_t.resolution_num;
}

/* *
 * @Description: convert resolution into a int binary value.
 *              priority level: resolution-->standard-->frequency-->deepcolor
 *              1080p60hz       1080           p           60          00
 *              2160p60hz420    2160           p           60          420
 *  User can select Highest resolution base this value.
 */
void DisplayMode::resolveResolution(const char *mode, resolution_t* resol_t) {
    bool validMode = false;
    if (strlen(mode) != 0) {
        for (int i = 0; i < DISPLAY_MODE_TOTAL; i++) {
            if (strcmp(mode, DISPLAY_MODE_LIST[i]) == 0) {
                validMode = true;
                break;
            }
        }
    }
    if (!validMode) {
        SYS_LOGI("the resolveResolution mode [%s] is not valid\n", mode);
        return;
    }

    resol_t->resolution = atoi(mode);
    resol_t->standard = strstr(mode, "p") == NULL ? 'i' : 'p';
    char* position = (char *)strstr(mode, resol_t->standard == 'p' ? "p" : "i");
    resol_t->frequency = atoi(position + 1);
    position = (char *)strstr(mode, "hz");
    resol_t->deepcolor = strlen(position + 2) == 0 ? 0 : atoi(position + 2);

    int i = strstr(mode, "p") == NULL ? 0 : 1;
    //[ 0:15]bit : resolution deepcolor
    //[16:27]bit : frequency
    //[28:31]bit : standard 'p' is 1, and 'i' is 0.
    //[32:63]bit : resolution
    resol_t->resolution_num = resol_t->deepcolor + (resol_t->frequency<< 16)
        + (((int64_t)i) << 28) + (((int64_t)resol_t->resolution) << 32);
}

//get the highest priority mode defined by CDF table
void DisplayMode::getHighestPriorityMode(char* mode, hdmi_data_t* data) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == data->sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == data->sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        if (strstr(data->edid, pMode[i]) != NULL) {
            strcpy(mode, pMode[i]);
            return;
        }
    }

    pSysWrite->getPropertyString(PROP_BEST_OUTPUT_MODE, mode, DEFAULT_OUTPUT_MODE);
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
    //getHighestPriorityMode(mode, data);
#endif
}

void DisplayMode::getHdmiOutputMode(char* mode, hdmi_data_t* data) {
    char edidParsing[MODE_LEN] = {0};
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    /* Fall back to 480p if EDID can't be parsed */
    if (strcmp(edidParsing, "ok")) {
        strcpy(mode, DEFAULT_OUTPUT_MODE);
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    if (pSysWrite->getPropertyBoolean(PROP_HDMIONLY, true)) {
        if (isBestOutputmode()) {
        #ifdef USE_BEST_MODE
            getBestHdmiMode(mode, data);
        #else
            getHighestHdmiMode(mode, data);
            //getHighestPriorityMode(mode, data);
        #endif
        } else {
            filterHdmiMode(mode, data);
        }
    }
    //SYS_LOGI("set HDMI mode to %s\n", mode);
}

void DisplayMode::getHdmiData(hdmi_data_t* data) {
    char sinkType[MODE_LEN] = {0};
    char edidParsing[MODE_LEN] = {0};

    //three sink types: sink, repeater, none
    pSysWrite->readSysfsOriginal(DISPLAY_HDMI_SINK_TYPE, sinkType);
    pSysWrite->readSysfs(DISPLAY_EDID_STATUS, edidParsing);

    data->sinkType = HDMI_SINK_TYPE_NONE;
    if (NULL != strstr(sinkType, "sink"))
        data->sinkType = HDMI_SINK_TYPE_SINK;
    else if (NULL != strstr(sinkType, "repeater"))
        data->sinkType = HDMI_SINK_TYPE_REPEATER;

    if (HDMI_SINK_TYPE_NONE != data->sinkType) {
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

    //filter mode defined by CDF, default disable this
    if (!strcmp(edidParsing, "ok") && false) {
        const char *delim = "\n";
        char filterEdid[MAX_STR_LEN] = {0};

        char *ptr = strtok(data->edid, delim);
        while (ptr != NULL) {
            //recommend mode or not
            bool recomMode = false;
            int len = strlen(ptr);
            if (ptr[len - 1] == '*') {
                ptr[len - 1] = '\0';
                recomMode = true;
            }

            if (modeSupport(ptr, data->sinkType)) {
                strcat(filterEdid, ptr);
                if (recomMode)
                    strcat(filterEdid, "*");
                strcat(filterEdid, delim);
            }
            ptr = strtok(NULL, delim);
        }

        //this is the real support edid filter by CDF
        strcpy(data->edid, filterEdid);

        SYS_LOGI("CDF filtered modes:\n%s", data->edid);
    }
}

bool DisplayMode::modeSupport(char *mode, int sinkType) {
    char **pMode = NULL;
    int modeSize = 0;

    if (HDMI_SINK_TYPE_SINK == sinkType) {
        pMode= (char **)MODES_SINK;
        modeSize = ARRAY_SIZE(MODES_SINK);
    }
    else if (HDMI_SINK_TYPE_REPEATER == sinkType) {
        pMode= (char **)MODES_REPEATER;
        modeSize = ARRAY_SIZE(MODES_REPEATER);
    }

    for (int i = 0; i < modeSize; i++) {
        //SYS_LOGI("modeSupport mode=%s, filerMode:%s, size:%d\n", mode, pMode[i], modeSize);
        if (!strcmp(mode, pMode[i]))
            return true;
    }

    return false;
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
    }

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

void DisplayMode::setSinkOutputMode(const char* outputmode) {
    setSinkOutputMode(outputmode, false);
}

void DisplayMode::setSinkOutputMode(const char* outputmode, bool initState) {
    SYS_LOGI("set sink output mode:%s, init state:%d\n", outputmode, initState?1:0);

    setSourceOutputMode(outputmode, initState?OUPUT_MODE_STATE_INIT:OUPUT_MODE_STATE_SWITCH);
}

void DisplayMode::setSinkDisplay(bool initState) {
    char current_mode[MODE_LEN] = {0};
    char outputmode[MODE_LEN] = {0};

    pSysWrite->readSysfs(SYSFS_DISPLAY_MODE, current_mode);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode);
    SYS_LOGD("init tv display old outputmode:%s, outputmode:%s\n", current_mode, outputmode);

    if (strlen(outputmode) == 0)
        strcpy(outputmode, mDefaultUI);

    updateDefaultUI();
    if (strcmp(current_mode, outputmode)) {
        //when change mode, need close uboot logo to avoid logo scaling wrong
        pSysWrite->writeSysfs(DISPLAY_FB0_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_BLANK, "1");
        pSysWrite->writeSysfs(DISPLAY_FB1_FREESCALE, "0");
    }

    setSinkOutputMode(outputmode, initState);
}

int DisplayMode::getBootenvInt(const char* key, int defaultVal) {
    int value = defaultVal;
    const char* p_value = mUbootenv->getValue(key);
    if (p_value) {
        value = atoi(p_value);
    }
    return value;
}

/*
 * *
 * @Description: select diff policy base on output mode state.
 * @params: outputmode state.
 * author: luan.yuan@amlogic.com
 *
 * only set 'null' to display/mode in switch adaper state.
 * auto switch frame rate need set 1 to /sys/class/amhdmitx/amhdmitx0/frac_rate_policy, to get CLK 0.1% offset.
 * But only change frac_rate_policy can not update CLOCK, unless mode and frac_rate_policy.
 * and can not set same mode to mode node. so need like 1080p60hz--->null--->1080p60hz.
 * this function will set mode to 'null', policy to 1, and set mode to previous value later.
 */
void DisplayMode::setAutoSwitchFrameRate(int state) {
    if (state == OUPUT_MODE_STATE_SWITCH_ADAPTER) {
        pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLICY, "1");
    } else {
        pSysWrite->writeSysfs(HDMI_TX_FRAMRATE_POLICY, "0");
    }
}

void DisplayMode::updateDefaultUI() {
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

void DisplayMode::updateDeepColor(bool cvbsMode, output_mode_state state, const char* outputmode) {
    if (!cvbsMode && (mDisplayType != DISPLAY_TYPE_TV)) {
        char colorAttribute[MODE_LEN] = {0};
        char mode[MODE_LEN] = {0};
        FormatColorDepth deepColor;
        if (pSysWrite->getPropertyBoolean(PROP_DEEPCOLOR, false)) {
            if (isDolbyVisionEnable() && isTvSupportDolbyVision(mode)) {
                if (!deepColor.isModeSupportDeepColorAttr(outputmode, COLOR_YCBCR444_8BIT)) {
                    strcpy((char *)outputmode, mode);
                }
                strcpy(colorAttribute, COLOR_YCBCR444_8BIT);
            } else {
                deepColor.getHdmiColorAttribute(outputmode, colorAttribute, (int)state);
            }
        } else {
            strcpy(colorAttribute, "default");
        }
        char attr[MODE_LEN] = {0};
        pSysWrite->readSysfs(DISPLAY_HDMI_COLOR_ATTR, attr);
        if (strstr(attr, colorAttribute) == NULL) {
            SYS_LOGI("set DeepcolorAttr value is different from attr sysfs value\n");
            pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
            pSysWrite->writeSysfs(DISPLAY_HDMI_COLOR_ATTR, colorAttribute);
        } else {
            SYS_LOGI("cur deepcolor attr value is equals to colorAttribute, Do not need set it\n");
        }
        SYS_LOGI("setMboxOutputMode colorAttribute = %s\n", colorAttribute);
        //save to ubootenv
        saveDeepColorAttr(outputmode, colorAttribute);
        setBootEnv(UBOOTENV_COLORATTRIBUTE, colorAttribute);
        //usleep(1000000);//100ms
        //pSysWrite->writeSysfs(SYSFS_DISPLAY_MODE, "null");
    }
}

void DisplayMode::updateFreeScaleAxis() {
    char axis[MAX_STR_LEN] = {0};
    sprintf(axis, "%d %d %d %d",
            0, 0, mDisplayWidth - 1, mDisplayHeight - 1);
    pSysWrite->writeSysfs(DISPLAY_FB0_FREESCALE_AXIS, axis);
}

void DisplayMode::updateWindowAxis(const char* outputmode) {
    char axis[MAX_STR_LEN] = {0};
    int position[4] = { 0, 0, 0, 0 };//x,y,w,h
    getPosition(outputmode, position);
    sprintf(axis, "%d %d %d %d",
            position[0], position[1], position[0] + position[2] - 1, position[1] + position[3] -1);
    pSysWrite->writeSysfs(DISPLAY_FB0_WINDOW_AXIS, axis);
}

void DisplayMode::getPosition(const char* curMode, int *position) {
    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    int defaultWidth = 0;
    int defaultHeight = 0;
    if (strstr(curMode, "480")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
        defaultWidth = FULL_WIDTH_480;
        defaultHeight = FULL_HEIGHT_480;
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
        defaultWidth = FULL_WIDTH_576;
        defaultHeight = FULL_HEIGHT_576;
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
        defaultWidth = FULL_WIDTH_720;
        defaultHeight = FULL_HEIGHT_720;
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
        defaultWidth = FULL_WIDTH_4K2K;
        defaultHeight = FULL_HEIGHT_4K2K;
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
        defaultWidth = FULL_WIDTH_4K2KSMPTE;
        defaultHeight = FULL_HEIGHT_4K2KSMPTE;
    } else {
        strcpy(keyValue, MODE_1080P_PREFIX);
        defaultWidth = FULL_WIDTH_1080;
        defaultHeight = FULL_HEIGHT_1080;
    }

    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    position[0] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    position[1] = getBootenvInt(ubootvar, 0);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    position[2] = getBootenvInt(ubootvar, defaultWidth);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    position[3] = getBootenvInt(ubootvar, defaultHeight);
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

    char keyValue[20] = {0};
    char ubootvar[100] = {0};
    if (strstr(curMode, "480")) {
        strcpy(keyValue, strstr(curMode, MODE_480P_PREFIX) ? MODE_480P_PREFIX : MODE_480I_PREFIX);
    } else if (strstr(curMode, "576")) {
        strcpy(keyValue, strstr(curMode, MODE_576P_PREFIX) ? MODE_576P_PREFIX : MODE_576I_PREFIX);
    } else if (strstr(curMode, MODE_720P_PREFIX)) {
        strcpy(keyValue, MODE_720P_PREFIX);
    } else if (strstr(curMode, MODE_1080I_PREFIX)) {
        strcpy(keyValue, MODE_1080I_PREFIX);
    } else if (strstr(curMode, MODE_1080P_PREFIX)) {
        strcpy(keyValue, MODE_1080P_PREFIX);
    } else if (strstr(curMode, MODE_4K2K_PREFIX)) {
        strcpy(keyValue, MODE_4K2K_PREFIX);
    } else if (strstr(curMode, MODE_4K2KSMPTE_PREFIX)) {
        strcpy(keyValue, "4k2ksmpte");
    }
    sprintf(ubootvar, "ubootenv.var.%s_x", keyValue);
    setBootEnv(ubootvar, x);
    sprintf(ubootvar, "ubootenv.var.%s_y", keyValue);
    setBootEnv(ubootvar, y);
    sprintf(ubootvar, "ubootenv.var.%s_w", keyValue);
    setBootEnv(ubootvar, w);
    sprintf(ubootvar, "ubootenv.var.%s_h", keyValue);
    setBootEnv(ubootvar, h);
}

void DisplayMode::saveDeepColorAttr(const char* mode, const char* dcValue) {
    char ubootvar[100] = {0};
    sprintf(ubootvar, "ubootenv.var.%s_deepcolor", mode);
    setBootEnv(ubootvar, (char *)dcValue);
}

void DisplayMode::getDeepColorAttr(const char* mode, char* value) {
    char ubootvar[100];
    sprintf(ubootvar, "ubootenv.var.%s_deepcolor", mode);
    if (!getBootEnv(ubootvar, value)) {
        //strcpy(value, (strstr(mode, "420") != NULL) ? DEFAULT_420_DEEP_COLOR_ATTR : DEFAULT_DEEP_COLOR_ATTR);
    }
    SYS_LOGI(": getDeepColorAttr [%s]", value);
}

/* *
 * @Description: Detect Whether TV support Dolby Vision
 * @return: if TV support return true, or false
 * if true, mode is the Highest resolution Tv Dolby Vision supported
 * else mode is ""
 */
bool DisplayMode::isTvSupportDolbyVision(char *mode) {
    char dv_cap[1024] = {0};
    pSysWrite->readSysfs(DOLBY_VISION_IS_SUPPORT, dv_cap);
    if (strlen(dv_cap) != 0) {
        for (int i = DISPLAY_MODE_TOTAL - 1; i >= 0; i--) {
            if (strstr(dv_cap, DISPLAY_MODE_LIST[i]) != NULL) {
                strcpy(mode, DISPLAY_MODE_LIST[i]);
                return true;
            }
        }
    }
    strcpy(mode, "");
    return false;
}

bool DisplayMode::isDolbyVisionEnable() {
    return pSysWrite->getPropertyBoolean(PROP_DOLBY_VISION_ENABLE, false);
}

void DisplayMode::setDolbyVisionEnable(int state) {
    if (DOLBY_VISION_SET_ENABLE == state) {
        //if TV
        if (DISPLAY_TYPE_TV == mDisplayType) {
            setHdrMode(HDR_MODE_OFF);
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SOURCE);
        }

        //if OTT
        if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
            pSysWrite->writeSysfs(DOLBY_VISION_POLICY, DV_POLICY_FOLLOW_SINK);
            pSysWrite->writeSysfs(DOLBY_VISION_HDR10_POLICY, DV_HDR10_POLICY);
        }

        usleep(100000);//100ms
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE, DV_ENABLE);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE, DV_MODE_IPT_TUNNEL);
        pSysWrite->setProperty(PROP_DOLBY_VISION_ENABLE, "true");
        SYS_LOGI("setDolbyVisionEnable Enable [%d]", isDolbyVisionEnable());
    } else {
        pSysWrite->writeSysfs(DOLBY_VISION_POLICY, DV_POLICY_FORCE_MODE);
        pSysWrite->writeSysfs(DOLBY_VISION_MODE, DV_MODE_BYPASS);
        usleep(100000);//100ms
        pSysWrite->writeSysfs(DOLBY_VISION_ENABLE, DV_DISABLE);
        pSysWrite->setProperty(PROP_DOLBY_VISION_ENABLE, "false");
        if (DISPLAY_TYPE_TV == mDisplayType) {
            setHdrMode(HDR_MODE_AUTO);
        }
        SYS_LOGI("setDolbyVisionEnable Enable [%d]", isDolbyVisionEnable());
    }
}

/* *
 * @Description: Detect Whether Dolby vision enable and TV support Dolby Vision
 *               if currentMode is not resolution TV Dolby Vision supported. and replace it by mode TV supported.
 * @params: state: current outputmode state INIT/POWER/SWITCH/ADATER etc
 *          outputmode: resolution needed to modify.
 * @result: the outputmode maybe changed into TV Dolby Vision supported.
 */
void DisplayMode::DetectDolbyVisionOutputMode(output_mode_state state, char* outputmode) {
    bool cvbsMode = false;
    if (!strcmp(outputmode, MODE_480CVBS) || !strcmp(outputmode, MODE_576CVBS)) {
        cvbsMode = true;
    }
    if (!cvbsMode && OUPUT_MODE_STATE_INIT == state
            && isDolbyVisionEnable()) {
        char mode[MODE_LEN] = {0};
        if (isTvSupportDolbyVision(mode)) {
            if (resolveResolutionValue(outputmode) > resolveResolutionValue(mode)) {
                memset(outputmode, 0, sizeof(outputmode));
                strcpy(outputmode, mode);
            }
        }
    }
}

/* *
 * @Description: set hdr mode
 * @params: mode "0":off "1":on "2":auto
 * */
void DisplayMode::setHdrMode(const char* mode) {
    if ((atoi(mode) >= 0) && (atoi(mode) <= 2)) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_HDR_MODE, mode);
        pSysWrite->setProperty(PROP_HDR_MODE_STATE, mode);
    }
}

/* *
 * @Description: set sdr mode
 * @params: mode "0":off "2":auto
 * */
void DisplayMode::setSdrMode(const char* mode) {
    if ((atoi(mode) == 0) || atoi(mode) == 2) {
        pSysWrite->writeSysfs(DISPLAY_HDMI_SDR_MODE, mode);
        pSysWrite->setProperty(PROP_SDR_MODE_STATE, mode);
    }
}

void DisplayMode::initHdrSdrMode() {
    char mode[MODE_LEN] = {0};
    pSysWrite->getPropertyString(PROP_HDR_MODE_STATE, mode, HDR_MODE_AUTO);
    setHdrMode(mode);
    memset(mode, 0, sizeof(mode));
    pSysWrite->getPropertyString(PROP_SDR_MODE_STATE, mode, SDR_MODE_OFF);
    setSdrMode(mode);
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

void DisplayMode::isHDCPTxAuthSuccess(int *status) {
    pTxAuth->isAuthSuccess(status);
}

void DisplayMode::onTxEvent (char* switchName, char* hpdstate, int outputState) {
    SYS_LOGI("onTxEvent switchName:%s hpdstate:%s state: %d\n", switchName, hpdstate, outputState);
#ifndef RECOVERY_MODE
    if (!strcmp(switchName, HDMI_UEVENT_HDMI_AUDIO)) {
        notifyEvent(hpdstate[0] == '1' ? EVENT_HDMI_AUDIO_IN : EVENT_HDMI_AUDIO_OUT);
        return;
    }
    if (hpdstate) {
        notifyEvent((hpdstate[0] == '1') ? EVENT_HDMI_PLUG_IN : EVENT_HDMI_PLUG_OUT);
        if (hpdstate[0] == '1')
            dumpCaps();
    }
#endif
    setSourceDisplay((output_mode_state)outputState);
}

void DisplayMode::onDispModeSyncEvent (const char* outputmode, int state) {
    SYS_LOGI("onDispModeSyncEvent outputmode:%s state: %d\n", outputmode, state);
    setSourceOutputMode(outputmode, (output_mode_state)state);
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

void DisplayMode::setListener(const sp<SystemControlNotify>& listener) {
    mNotifyListener = listener;
}
#endif

void DisplayMode::dumpCap(const char * path, const char * hint, char *result) {
    char logBuf[MAX_STR_LEN];
    pSysWrite->readSysfsOriginal(path, logBuf);

    if (mLogLevel > LOG_LEVEL_0)
        SYS_LOGI("%s%s", hint, logBuf);

    if (NULL != result) {
        strcat(result, hint);
        strcat(result, logBuf);
        strcat(result, "\n");
    }
}

void DisplayMode::dumpCaps(char *result) {
    dumpCap(DISPLAY_EDID_STATUS, "\nEDID parsing status: ", result);
    dumpCap(DISPLAY_EDID_VALUE, "General caps\n", result);
    dumpCap(DISPLAY_HDMI_DEEP_COLOR, "Deep color\n", result);
    dumpCap(DISPLAY_HDMI_HDR, "HDR\n", result);
    dumpCap(DISPLAY_HDMI_MODE_PREF, "Preferred mode: ", result);
    dumpCap(DISPLAY_HDMI_SINK_TYPE, "Sink type: ", result);
    dumpCap(DISPLAY_HDMI_AUDIO, "Audio caps\n", result);
    dumpCap(DISPLAY_EDID_RAW, "Raw EDID\n", result);
}

int DisplayMode::dump(char *result) {
    if (NULL == result)
        return -1;

    char buf[2048] = {0};
    sprintf(buf, "\ndisplay type: %d [0:none 1:tablet 2:mbox 3:tv], soc type:%s\n", mDisplayType, mSocType);
    strcat(result, buf);

    if ((DISPLAY_TYPE_MBOX == mDisplayType) || (DISPLAY_TYPE_REPEATER == mDisplayType)) {
        sprintf(buf, "default ui:%s\n", mDefaultUI);
        strcat(result, buf);
        dumpCaps(result);
    }
    return 0;
}

