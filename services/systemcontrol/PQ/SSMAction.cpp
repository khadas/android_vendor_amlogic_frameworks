/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "SSMAction"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cutils/properties.h>
#include "SSMAction.h"
#include "CPQLog.h"


pthread_mutex_t ssm_r_w_op_mutex = PTHREAD_MUTEX_INITIALIZER;

SSMAction *SSMAction::mInstance;
SSMHandler *SSMAction::mSSMHandler = NULL;
SSMAction *SSMAction::getInstance()
{
    if (NULL == mInstance)
        mInstance = new SSMAction();
    return mInstance;
}

SSMAction::SSMAction()
{
    bool FileExist;
    //check ssm file exist!
    FileExist = isFileExist(SSM_DATA_PATH);

    //open SSM handler!
    mSSMHandler = SSMHandler::GetSingletonInstance();

    //open ssm file!
    if (!FileExist) {
        SYS_LOGD("%s don't exist,create and open!\n",SSM_DATA_PATH);
        m_dev_fd = open(SSM_DATA_PATH, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
        SSMRestoreDefault(0, true);
    } else {
        SYS_LOGD("open %s file!\n",SSM_DATA_PATH);
        m_dev_fd = open(SSM_DATA_PATH, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);
    }

    if (m_dev_fd < 0) {
        SYS_LOGE("Open %s failed! error: %s.\n", SSM_DATA_PATH, strerror(errno));
    } else {
        SYS_LOGD("Open %s success!",SSM_DATA_PATH);
    }

    //ssm check
    if (mSSMHandler != NULL) {
        SSM_status_t SSM_status = (SSM_status_t)GetSSMStatus();
        SYS_LOGD ("Verify SSMHeader, status= %d\n", SSM_status);

        if (SSM_status == SSM_HEADER_INVALID) {
            mSSMHandler->SSMRecreateHeader();
        } else if (SSM_status == SSM_HEADER_STRUCT_CHANGE) {
            SSMRecovery();
        }
    }
}

SSMAction::~SSMAction()
{
    if (m_dev_fd > 0) {
        close(m_dev_fd);
        m_dev_fd = -1;
    }

    if (mSSMHandler != NULL) {
        delete mSSMHandler;
        mSSMHandler = NULL;
    }
}

bool SSMAction::isFileExist(const char *file_name)
{
    struct stat tmp_st;
    int ret = -1;

    ret = stat(file_name, &tmp_st);
    if (ret != 0 ) {
       SYS_LOGE("%s don't exist!\n",file_name);
       return false;
    } else {
       return true;
    }
}

int SSMAction::WriteBytes(int offset, int size, unsigned char *buf)
{
    lseek(m_dev_fd, offset, SEEK_SET);
    write(m_dev_fd, buf, size);

    return 0;
}
int SSMAction::ReadBytes(int offset, int size, unsigned char *buf)
{

    lseek(m_dev_fd, offset, SEEK_SET);
    read(m_dev_fd, buf, size);
    return 0;
}
int SSMAction::EraseAllData()
{
    ftruncate(m_dev_fd, 0);
    lseek (m_dev_fd, 0, SEEK_SET);

    return 0;
}

int SSMAction::GetSSMActualAddr(int id)
{
    return mSSMHandler->SSMGetActualAddr(id);
}

int SSMAction::GetSSMActualSize(int id)
{
    return mSSMHandler->SSMGetActualSize(id);
}

int SSMAction::GetSSMStatus(void)
{
    return (int)mSSMHandler->SSMVerify();
}

int SSMAction::SSMWriteNTypes(int id, int data_len, int *data_buf, int offset)
{
    pthread_mutex_lock(&ssm_r_w_op_mutex);
    if (data_buf == NULL) {
        SYS_LOGE("data_buf is NULL.\n");
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (0 == data_len) {
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    unsigned int actualAddr = mSSMHandler->SSMGetActualAddr(id) + offset;

    if (WriteBytes(actualAddr, data_len, (unsigned char *) data_buf) < 0) {
        SYS_LOGE("device WriteNBytes error.\n");
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }
    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}

int SSMAction::SSMReadNTypes(int id, int data_len, int *data_buf, int offset)
{
    pthread_mutex_lock(&ssm_r_w_op_mutex);

    if (data_buf == NULL) {
        SYS_LOGE("data_buf is NULL.\n");
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    if (0 == data_len) {
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }

    unsigned int actualAddr = mSSMHandler->SSMGetActualAddr(id) + offset;

    if (ReadBytes(actualAddr, data_len, (unsigned char *) data_buf) < 0) {
        SYS_LOGE("device ReadNBytes error.\n");
        pthread_mutex_unlock(&ssm_r_w_op_mutex);
        return -1;
    }
    pthread_mutex_unlock(&ssm_r_w_op_mutex);
    return 0;
}

bool SSMAction::SSMRecovery()
{
    bool ret = true;
    current_ssmheader_section2_t header_cur;

    ret = mSSMHandler->SSMSaveCurrentHeader(&header_cur);

    unsigned char* SSMBuff = (unsigned char*) malloc(header_cur.size);

    if (!SSMBuff)
        ret = false;

    if (ret) {
        ReadBytes(0, header_cur.size, SSMBuff);
        EraseAllData();

        for (unsigned int i = 0; i < gSSMHeader_section1.count; i++) {
            if (header_cur.header_section2_data[i].size == gSSMHeader_section2[i].size) {
                WriteBytes(gSSMHeader_section2[i].addr,
                           gSSMHeader_section2[i].size, SSMBuff +
                           header_cur.header_section2_data[i].addr);
            }
            else {
                SSMRestoreDefault(i, false);
                SYS_LOGD ("id : %d size from %d -> %d", i, header_cur.header_section2_data[i].size, gSSMHeader_section2[i].size);
            }
        }

        if (ret)
            free (SSMBuff);

        ret = mSSMHandler->SSMRecreateHeader();
    }

    return ret;
}

int SSMAction::SSMRestoreDefault(int id, bool resetAll)
{
    int i = 0, tmp_val = 0;
    int tmp_panorama_nor = 0, tmp_panorama_full = 0;
    int offset_r = 0, offset_g = 0, offset_b = 0, gain_r = 1024, gain_g = 1024, gain_b = 1024;
    int8_t std_buf[6] = { 0, 0, 0, 0, 0, 0 };
    int8_t warm_buf[6] = { 0, 0, -8, 0, 0, 0 };
    int8_t cold_buf[6] = { -8, 0, 0, 0, 0, 0 };
    unsigned char tmp[2] = {0, 0};

    if (resetAll || VPP_DATA_POS_COLOR_DEMO_MODE_START == id)
        SSMSaveColorDemoMode ( VPP_COLOR_DEMO_MODE_ALLON);

    if (resetAll || VPP_DATA_POS_COLOR_BASE_MODE_START == id)
        SSMSaveColorBaseMode ( VPP_COLOR_BASE_MODE_OPTIMIZE);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_R_START == id)
        SSMSaveRGBGainRStart(0, gain_r);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_G_START == id)
        SSMSaveRGBGainGStart(0, gain_g);

    if (resetAll || VPP_DATA_POS_RGB_GAIN_B_START == id)
        SSMSaveRGBGainBStart(0, gain_b);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_R_START == id)
        SSMSaveRGBPostOffsetRStart(0, offset_r);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_G_START== id)
        SSMSaveRGBPostOffsetGStart(0, offset_g);

    if (resetAll || VPP_DATA_POS_RGB_POST_OFFSET_B_START == id)
        SSMSaveRGBPostOffsetBStart(0, offset_b);

    if (resetAll || VPP_DATA_GAMMA_VALUE_START == id)
        SSMSaveGammaValue(0);

    if (resetAll || VPP_DATA_RGB_START == id) {
        for (i = 0; i < 6; i++) {
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_STANDARD * 6, std_buf[i]); //0~5
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_WARM * 6, warm_buf[i]); //6~11
            SSMSaveRGBValueStart(i + VPP_COLOR_TEMPERATURE_MODE_COLD * 6, cold_buf[i]); //12~17
        }
    }

    for (i = SOURCE_INVALID; i < SOURCE_MAX; i++) {//invalid souce setting also need clear when restore default
        if (resetAll || VPP_DATA_COLOR_SPACE_START == id) {
            if (i == SOURCE_TYPE_HDMI) {
                SSMSaveColorSpaceStart ( VPP_COLOR_SPACE_AUTO);
            }
        }

        tmp_val = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
        if (resetAll || VPP_DATA_POS_COLOR_TEMPERATURE_START == id)
            SSMSaveColorTemperature(i, tmp_val);

        tmp_val = 50;
        if (resetAll || VPP_DATA_POS_BRIGHTNESS_START == id)
            SSMSaveBrightness(i, tmp_val);

        if (resetAll || VPP_DATA_POS_CONTRAST_START == id)
            SSMSaveContrast(i, tmp_val);

        if (resetAll || VPP_DATA_POS_SATURATION_START == id)
            SSMSaveSaturation(i, tmp_val);

        if (resetAll || VPP_DATA_POS_HUE_START == id)
            SSMSaveHue(i, tmp_val);

        if (resetAll || VPP_DATA_POS_SHARPNESS_START == id)
            SSMSaveSharpness(i, tmp_val);

        tmp_val = VPP_PICTURE_MODE_STANDARD;
        if (resetAll || VPP_DATA_POS_PICTURE_MODE_START == id)
            SSMSavePictureMode(i, tmp_val);

        tmp_val = VPP_DISPLAY_MODE_169;
        if (resetAll || VPP_DATA_POS_DISPLAY_MODE_START == id)
            SSMSaveDisplayMode(i, tmp_val);

        tmp_val = VPP_NOISE_REDUCTION_MODE_AUTO;
        if (resetAll || VPP_DATA_POS_NOISE_REDUCTION_START == id)
            SSMSaveNoiseReduction(i, tmp_val);

        tmp_val = DEFAULT_BACKLIGHT_BRIGHTNESS;
        if (resetAll || VPP_DATA_POS_BACKLIGHT_START == id)
            SSMSaveBackLightVal(i, tmp_val);
    }

    if (resetAll || VPP_DATA_POS_DDR_SSC_START == id)
        SSMSaveDDRSSC(0);

    if (resetAll || VPP_DATA_POS_LVDS_SSC_START == id)
        SSMSaveLVDSSSC(tmp);

    if (resetAll || VPP_DATA_EYE_PROTECTION_MODE_START == id)
       SSMSaveEyeProtectionMode(0);
    return 0;
}

//PQ mode
int SSMAction::SSMSavePictureMode(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_PICTURE_MODE_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadPictureMode(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_PICTURE_MODE_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//Color Temperature
int SSMAction::SSMSaveColorTemperature(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_COLOR_TEMPERATURE_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadColorTemperature(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_COLOR_TEMPERATURE_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveColorDemoMode(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_COLOR_DEMO_MODE_START, 1, &tmp_val);
}

int SSMAction::SSMReadColorDemoMode(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_COLOR_DEMO_MODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}
int SSMAction::SSMSaveColorBaseMode(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_COLOR_BASE_MODE_START, 1, &tmp_val);
}

int SSMAction::SSMReadColorBaseMode(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = SSMReadNTypes(VPP_DATA_POS_COLOR_BASE_MODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBGainRStart(int offset, unsigned int rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_R_START, 4, &tmp_val, offset);
}

int SSMAction::SSMReadRGBGainRStart(int offset, unsigned int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;

    ret = SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_R_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBGainGStart(int offset, unsigned int rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_G_START, 4, &tmp_val, offset);
}

int SSMAction::SSMReadRGBGainGStart(int offset, unsigned int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_G_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBGainBStart(int offset, unsigned int rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_RGB_GAIN_B_START, 4, &tmp_val, offset);
}

int SSMAction::SSMReadRGBGainBStart(int offset, unsigned int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_RGB_GAIN_B_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBPostOffsetRStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_R_START, 4, &rw_val, offset);
}

int SSMAction::SSMReadRGBPostOffsetRStart(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_R_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBPostOffsetGStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_G_START, 4, &rw_val, offset);
}

int SSMAction::SSMReadRGBPostOffsetGStart(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_G_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveRGBPostOffsetBStart(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_RGB_POST_OFFSET_B_START, 4, &rw_val, offset);
}

int SSMAction::SSMReadRGBPostOffsetBStart(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_RGB_POST_OFFSET_B_START, 4, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMReadRGBOGOValue(int offset, int size, unsigned char data_buf[])
{
    int tmp_off = 0, val = 0, max = 0, ret = -1;

    val = offset + size;
    max = SSM_CR_RGBOGO_LEN + SSM_CR_RGBOGO_CHKSUM_LEN;
    if (val > max) {
        SYS_LOGE("Out of max_size!!!\n");
        return ret;
    }

    tmp_off = SSM_RGBOGO_FILE_OFFSET + offset;

    ret = ReadDataFromFile(SSM_RGBOGO_FILE_PATH, tmp_off, size, data_buf);

    return ret;
}

int SSMAction::SSMSaveRGBOGOValue(int offset, int size, unsigned char data_buf[])
{
    int tmp_off = 0, val = 0, max = 0, ret = -1;

    val = offset + size;
    max = SSM_CR_RGBOGO_LEN + SSM_CR_RGBOGO_CHKSUM_LEN;
    if (val > max) {
        SYS_LOGE("Out of max_size!!!\n");
        return ret;
    }

    tmp_off = SSM_RGBOGO_FILE_OFFSET + offset;

    ret = SaveDataToFile(SSM_RGBOGO_FILE_PATH, tmp_off, size, data_buf);

    return ret;
}

int SSMAction::SSMSaveRGBValueStart(int offset, int8_t rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_RGB_START, 1, &tmp_val, offset);
}

int SSMAction::SSMReadRGBValueStart(int offset, int8_t *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_RGB_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveColorSpaceStart(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_COLOR_SPACE_START, 1, &tmp_val);
}

int SSMAction::SSMReadColorSpaceStart(unsigned char *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_COLOR_SPACE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::ReadDataFromFile(const char *file_name, int offset, int nsize, unsigned char data_buf[])
{
    int device_fd = -1;

    if (data_buf == NULL) {
        SYS_LOGE("data_buf is NULL!!!\n");
        return -1;
    }

    if (file_name == NULL) {
        SYS_LOGE("%s is NULL!\n",file_name);
        return -1;
    }

    device_fd = open(file_name, O_RDONLY);
    if (device_fd < 0) {
        SYS_LOGE("open file \"%s\" error(%s).\n", file_name, strerror(errno));
        return -1;
    }

    lseek(device_fd, offset, SEEK_SET);
    read(device_fd, data_buf, nsize);

    close(device_fd);
    device_fd = -1;

    return 0;
}

int SSMAction::SaveDataToFile(const char *file_name, int offset, int nsize, unsigned char data_buf[])
{
    int device_fd = -1;

    if (data_buf == NULL) {
        SYS_LOGE("%s, data_buf is NULL!!!\n", __FUNCTION__);
        return -1;
    }

    if (file_name == NULL) {
        SYS_LOGE("%s IS NULL!\n",file_name);
        return -1;
    }

    device_fd = open(file_name, O_RDWR | O_SYNC);
    if (device_fd < 0) {
        SYS_LOGE("open file \"%s\" error(%s).\n", file_name, strerror(errno));
        return -1;
    }

    lseek(device_fd, offset, SEEK_SET);
    write(device_fd, data_buf, nsize);
    fsync(device_fd);

    close(device_fd);
    device_fd = -1;

    return 0;
}


//Brightness
int SSMAction::SSMSaveBrightness(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_BRIGHTNESS_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadBrightness(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_BRIGHTNESS_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//constract
int SSMAction::SSMSaveContrast(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_CONTRAST_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadContrast(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_CONTRAST_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//saturation
int SSMAction::SSMSaveSaturation(int offset, int rw_val)
{

    return SSMWriteNTypes(VPP_DATA_POS_SATURATION_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadSaturation(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_SATURATION_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//hue
int SSMAction::SSMSaveHue(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_HUE_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadHue(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_HUE_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//Sharpness
int SSMAction::SSMSaveSharpness(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_SHARPNESS_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadSharpness(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_SHARPNESS_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

//NoiseReduction
int SSMAction::SSMSaveNoiseReduction(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_NOISE_REDUCTION_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadNoiseReduction(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_NOISE_REDUCTION_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveGammaValue(int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_GAMMA_VALUE_START, 1, &rw_val);
}

int SSMAction::SSMReadGammaValue(int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_GAMMA_VALUE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveEyeProtectionMode(int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_EYE_PROTECTION_MODE_START, 1, &rw_val);
}

int SSMAction::SSMReadEyeProtectionMode(int *rw_val)
{
    int ret = 0;
    int tmp_val = 0;
    ret = SSMReadNTypes(VPP_DATA_EYE_PROTECTION_MODE_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}
int SSMAction::SSMSaveTestPattern(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, &tmp_val);
}

int SSMAction::SSMReadTestPattern(unsigned char *rw_val)
{
    int tmp_val;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_TEST_PATTERN_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveDDRSSC(unsigned char rw_val)
{
    int tmp_val = rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_DDR_SSC_START, 1, &tmp_val);
}

int SSMAction::SSMReadDDRSSC(unsigned char *rw_val)
{
    int tmp_val;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_DDR_SSC_START, 1, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveLVDSSSC(unsigned char *rw_val)
{
    int tmp_val = *rw_val;
    return SSMWriteNTypes(VPP_DATA_POS_LVDS_SSC_START, 2, &tmp_val);
}

int SSMAction::SSMReadLVDSSSC(unsigned char *rw_val)
{
    int tmp_val;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_LVDS_SSC_START, 2, &tmp_val);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveDisplayMode(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_DISPLAY_MODE_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadDisplayMode(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_DISPLAY_MODE_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

int SSMAction::SSMSaveBackLightVal(int offset, int rw_val)
{
    return SSMWriteNTypes(VPP_DATA_POS_BACKLIGHT_START, 1, &rw_val, offset);
}

int SSMAction::SSMReadBackLightVal(int offset, int *rw_val)
{
    int tmp_val = 0;
    int ret = 0;
    ret = SSMReadNTypes(VPP_DATA_POS_BACKLIGHT_START, 1, &tmp_val, offset);
    *rw_val = tmp_val;

    return ret;
}

