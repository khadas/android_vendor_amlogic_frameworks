/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CPQControl"

#include <cutils/properties.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>

#include "CPQControl.h"


#define PI 3.14159265358979


CPQControl *CPQControl::mInstance = NULL;
CPQControl *CPQControl::GetInstance()
{
    if (NULL == mInstance)
        mInstance = new CPQControl();
    return mInstance;
}

CPQControl::CPQControl()
{
    mIsHdrLastTime = false;
    mInitialized = false;
    mAutoSwitchPCModeFlag = 1;
    mAmvideoFd = -1;
    mDiFd = -1;
    mPQdb = new CPQdb();
    SetFlagByCfg(RESET_ALL, 1);

    mSSMAction = SSMAction::getInstance();
    //open DB
    if (mPQdb->openPqDB(PQ_DB_PATH)) {
        SYS_LOGE("%s, open pq DB failed!", __FUNCTION__);
    } else {
        SYS_LOGD("%s, open pq DB success!", __FUNCTION__);
    }

    //open module
    mAmvideoFd = VPPOpenModule();
    if (mAmvideoFd < 0) {
        SYS_LOGE("Open PQ module failed!\n");
    } else {
        SYS_LOGD("Open PQ module success!\n");
    }

    //open DI module
    mDiFd = DIOpenModule();
    if (mDiFd < 0) {
        SYS_LOGE("Open DI module failed!\n");
    } else {
        SYS_LOGD("Open DI module success!\n");
    }

    //init source
    InitSourceInputInfo();

    //Load PQ
    if (LoadPQSettings(mCurentSourceInputInfo) < 0) {
        SYS_LOGE("Load PQ failed!\n");
    } else {
        SYS_LOGD("Load PQ success!\n");
    }

    //set backlight
    BacklightInit();

    //Vframe size
    mCDevicePollCheckThread.setObserver(this);
    mCDevicePollCheckThread.StartCheck();
    mInitialized = true;
}

CPQControl::~CPQControl()
{
    //reset sig_fmt
    resetLastSourceTypeSettings();
    //close moduel
    VPPCloseModule();
    //close DI module
    DICloseModule();

    if (mSSMAction!= NULL) {
        delete mSSMAction;
        mSSMAction = NULL;
    }

    if (mPQdb != NULL) {
        //closed DB
        mPQdb->closeDb();

        delete mPQdb;
        mPQdb = NULL;
    }

    mCDevicePollCheckThread.requestExit();
}

int CPQControl::VPPOpenModule(void)
{
    mAmvideoFd = mCDevicePollCheckThread.HDR_fd;
    if (mAmvideoFd < 0) {
        mAmvideoFd = open(VPP_DEV_PATH, O_RDWR);
        if (mAmvideoFd < 0) {
            SYS_LOGE("Open PQ module, error(%s)!\n", strerror(errno));
            return -1;
        }
    } else {
        SYS_LOGD("vpp OpenModule has been opened before!\n");
    }

    return mAmvideoFd;
}

int CPQControl::VPPCloseModule(void)
{
    if (mAmvideoFd >= 0) {
        close ( mAmvideoFd);
        mAmvideoFd = -1;
    }
    return 0;
}

int CPQControl::VPPDeviceIOCtl(int request, ...)
{
    int ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    ret = ioctl(mAmvideoFd, request, arg);
    return ret;
}

int CPQControl::DIOpenModule(void)
{
    if (mDiFd < 0) {
        mDiFd = open(DI_DEV_PATH, O_RDWR);

        SYS_LOGD("DI OpenModule path: %s", DI_DEV_PATH);

        if (mDiFd < 0) {
            SYS_LOGE("Open DI module, error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    return mDiFd;
}

int CPQControl::DICloseModule(void)
{
    if (mDiFd>= 0) {
        close ( mDiFd);
        mDiFd = -1;
    }
    return 0;
}

int CPQControl::DIDeviceIOCtl(int request, ...)
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;
    va_start(ap, request);
    arg = va_arg ( ap, void * );
    va_end(ap);
    tmp_ret = ioctl(mDiFd, request, arg);
    return tmp_ret;
}

void CPQControl::onVframeSizeChange()
{
    source_input_param_t SourceInputParam = GetCurrentSourceInputInfo();
    if (SourceInputParam.source_input == SOURCE_DTV) {
        //set DTV port signal info
        SourceInputParam.source_type = SOURCE_TYPE_DTV;
        SourceInputParam.source_port = TVIN_PORT_DTV;
        SourceInputParam.sig_fmt = getVideoResolutionToFmt();
        SourceInputParam.is3d = INDEX_2D;
        SourceInputParam.trans_fmt = TVIN_TFMT_2D;

        SetCurrentSourceInputInfo(SourceInputParam);
        LoadPQSettings(SourceInputParam);
    } else if((SourceInputParam.source_input == SOURCE_INVALID)
              ||(SourceInputParam.source_input == SOURCE_MPEG)) {
        //set MPEG port signal info
        SourceInputParam.source_input = SOURCE_MPEG;
        SourceInputParam.source_type = SOURCE_TYPE_MPEG;
        SourceInputParam.source_port = TVIN_PORT_MPEG0;
        SourceInputParam.sig_fmt = getVideoResolutionToFmt();
        SourceInputParam.is3d = INDEX_2D;
        SourceInputParam.trans_fmt = TVIN_TFMT_2D;

        SetCurrentSourceInputInfo(SourceInputParam);
        LoadPQSettings(SourceInputParam);
    }
}

tvin_sig_fmt_t CPQControl::getVideoResolutionToFmt()
{
    int fd = -1;
    char buf[32] = {0};
    tvin_sig_fmt_t sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;

    fd = open(SYS_VIDEO_FRAME_HEIGHT, O_RDONLY);
    if (fd < 0) {
        SYS_LOGE("[%s] open: %s error!\n", __FUNCTION__, SYS_VIDEO_FRAME_HEIGHT);
        return sig_fmt;
    }

    if (read(fd, buf, sizeof(buf)) >0) {
        int height = atoi(buf);
        if (height <= 576) {
            sig_fmt = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
        } else if (height > 576 && height <= 720) {
            sig_fmt = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
        } else if (height > 720 && height <= 1088) {
            sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        } else {
            sig_fmt = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;
        }
    } else {
        SYS_LOGE("[%s] read error!\n", __FUNCTION__);
    }
    close(fd);

    SYS_LOGD("sig_fmt = %d\n", sig_fmt);
    mCurentSourceInputInfo.sig_fmt = sig_fmt;
    return sig_fmt;
}

void CPQControl::onHDRStatusChange()
{
    source_input_param_t SourceInputParam = GetCurrentSourceInputInfo();

    if ((SourceInputParam.source_input >= SOURCE_HDMI1)
      &&(SourceInputParam.source_input <= SOURCE_HDMI4)) {

        LoadPQSettings(SourceInputParam);
    }
}
int CPQControl::SetPQMoudleStatus(int status)
{
    //status:0 OFF; 1 ON
    FILE *fp = NULL;
    fp = fopen(PQ_MOUDLE_ENABLE_PATH, "w");
    if (fp == NULL) {
        SYS_LOGE("Open %s error(%s)!\n", PQ_MOUDLE_ENABLE_PATH, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", status);
    fclose(fp);
    fp = NULL;

    return 0;
}

int CPQControl::LoadPQSettings(source_input_param_t source_input_param)
{
    int ret = 0;
    char buf[128] = {0};
    property_get("persist.sys.PQ.enable", buf, "false");
    if (strcmp(buf, "true") != 0) {
        SYS_LOGD("PQ moudle close!\n");
        SetPQMoudleStatus(0);//turn off PQ moudle
        return ret;
    } else {
        di_mode_param_t di_param;
        vpp_color_temperature_mode_t temp_mode = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
        vpp_picture_mode_t pqmode = VPP_PICTURE_MODE_STANDARD;
        vpp_display_mode_t dispmode = VPP_DISPLAY_MODE_169;
        vpp_noise_reduction_mode_t nr_mode = VPP_NOISE_REDUCTION_MODE_MID;
        bool hdrStatus = mPQdb->IsMatchHDRCondition(source_input_param.source_port);

        if ((cpq_setting_last_source_type == source_input_param.source_type)
            && (cpq_setting_last_sig_fmt == source_input_param.sig_fmt)
            && (cpq_setting_last_trans_fmt == source_input_param.trans_fmt)
            && (hdrStatus == mIsHdrLastTime)) {
            SYS_LOGD("Same signal,no need load!\n");
            return ret;
        }
        ret |= Cpq_SetXVYCCMode(VPP_XVYCC_MODE_STANDARD, source_input_param);

        nr_mode =(vpp_noise_reduction_mode_t)GetNoiseReductionMode();

        di_param.nr_mode = (vpp_noise_reduction2_mode_t)nr_mode;
        di_param.deblock_mode = VPP_DEBLOCK_MODE_MIDDLE;
        di_param.mcdi_mode = VPP_MCDI_MODE_STANDARD;

        ret |= Cpq_SetDIMode(di_param, source_input_param);

        Cpq_LoadBasicRegs(source_input_param);

        if (!mInitialized) {
            SetGammaValue(VPP_GAMMA_CURVE_MAX, 0);
        } else {
            SetGammaValue(VPP_GAMMA_CURVE_AUTO, 1);
        }

        ret |= Cpq_SetBaseColorMode(GetBaseColorMode(), source_input_param);

        temp_mode = (vpp_color_temperature_mode_t)GetColorTemperature();
        if (temp_mode != VPP_COLOR_TEMPERATURE_MODE_USER)
            Cpq_CheckColorTemperatureParamAlldata(source_input_param);
        ret |= SetColorTemperature((int)temp_mode, 0);

        pqmode = (vpp_picture_mode_t)GetPQMode();
        ret |= Cpq_SetPQMode(pqmode, source_input_param);

        ret |= SetDNLP(source_input_param);

        cpq_setting_last_source_type = source_input_param.source_type;
        cpq_setting_last_sig_fmt = source_input_param.sig_fmt;
        cpq_setting_last_trans_fmt = source_input_param.trans_fmt;

        if (hdrStatus) {
            mIsHdrLastTime = true;
            Cpq_SetDNLPStatus("0");
        } else {
            mIsHdrLastTime = false;
        }
        return ret;
    }

}
int CPQControl::Cpq_LoadRegs(am_regs_t regs)
{
    if (regs.length == 0) {
        SYS_LOGD("%s--Regs is NULL!\n", __FUNCTION__);
        return -1;
    }

    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_LOAD_REG, &regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::Cpq_LoadDisplayModeRegs(ve_pq_load_t regs)
{
    if (regs.length == 0) {
        SYS_LOGD("%s--Regs is NULL!\n", __FUNCTION__);
        return -1;
    }

    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_GET_OVERSCAN, &regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::DI_LoadRegs(am_pq_param_t di_regs)
{
    if (di_regs.table_len == 0) {
        SYS_LOGD("%s--DI_regs is NULL!\n", __FUNCTION__);
        return -1;
    }

    int count_retry = 20;
    int ret = 0;
    while (count_retry) {
        ret = DIDeviceIOCtl(AMDI_IOC_SET_PQ_PARM, &di_regs);
        if (ret < 0) {
            SYS_LOGE("%s, error(%s), errno(%d)\n", __FUNCTION__, strerror(errno), errno);
            if (errno == EBUSY) {
                SYS_LOGE("%s, %s, retry...\n", __FUNCTION__, strerror(errno));
                count_retry--;
                continue;
            }
        }
        break;
    }

    return ret;
}

int CPQControl::LoadCpqLdimRegs()
{
    bool ret = 0;
    int ldFd = -1;

    ldFd = open(LDIM_PATH, O_RDWR);

    if (ldFd < 0) {
        SYS_LOGE("Open ldim module, error(%s)!\n", strerror(errno));
        ret = -1;
    } else {
        vpu_ldim_param_s *ldim_param_temp = new vpu_ldim_param_s();

        if (ldim_param_temp) {
            if (!mPQdb->PQ_GetLDIM_Regs(ldim_param_temp) || ioctl(ldFd, LDIM_IOC_PARA, ldim_param_temp) < 0) {
               SYS_LOGE("LoadCpqLdimRegs, error(%s)!\n", strerror(errno));
               ret = -1;
            }

            delete ldim_param_temp;
        }
            close (ldFd);
    }

    return ret;
}

int CPQControl::Cpq_LoadBasicRegs(source_input_param_t source_input_param)
{
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    int ret = -1, enableFlag = -1;

    if (mPQdb->loadHdrStatus(source_input_param.source_port, String8("GeneralCommonTable")))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    if (mPQdb->getRegValues("GeneralCommonTable", source_input_param.source_port, source_input_param.sig_fmt,
                             source_input_param.is3d, source_input_param.trans_fmt, &regs) > 0) {
        if (Cpq_LoadRegs(regs) < 0) {
            SYS_LOGE("%s, Cpq_LoadRegs failed!\n", __FUNCTION__);
        } else {
            ret = 0;
        }
    } else {
        SYS_LOGE("%s, getRegValues failed!\n",__FUNCTION__);
    }

    if (mPQdb->LoadAllPQData(source_input_param.source_port, source_input_param.sig_fmt, source_input_param.is3d,
                             source_input_param.trans_fmt, enableFlag) == 0) {
        ret = 0;
    } else {
        SYS_LOGE("Cpq_LoadBasicRegs getPQData failed!\n");
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::BacklightInit(void)
{
    int ret = 0;
    int backlight = GetBacklight(SOURCE_MPEG);

    if (mbVppCfg_backlight_init) {
        backlight = (backlight + 100) * 255 / 200;

        if (backlight < 127 || backlight > 255) {
            backlight = 255;
        }
    }

    ret = SetBacklight(SOURCE_MPEG, backlight, 1);

    return ret;
}

int CPQControl::Cpq_SetDIMode(di_mode_param_t di_param, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0, sizeof(am_pq_param_t));

    if (mPQdb->PQ_GetDIParams(source_input_param.source_port, source_input_param.sig_fmt,
                              source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
        di_regs.table_name |= TABLE_NAME_DI;
    } else {
        SYS_LOGE("%s GetDIParams failed!\n",__FUNCTION__);
    }

    if (mPQdb->PQ_GetMCDIParams(di_param.mcdi_mode, source_input_param.source_port, source_input_param.sig_fmt,
                              source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
        di_regs.table_name |= TABLE_NAME_MCDI;
    } else {
        SYS_LOGE("%s GetMCDIParams failed!\n",__FUNCTION__);
    }

    if (mPQdb->PQ_GetDeblockParams(di_param.deblock_mode, source_input_param.source_port, source_input_param.sig_fmt,
                              source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
        di_regs.table_name |= TABLE_NAME_DEBLOCK;
    } else {
        SYS_LOGE("%s GetDeblockParams failed!\n",__FUNCTION__);
    }

    if (mPQdb->PQ_GetNR2Params(di_param.nr_mode, source_input_param.source_port, source_input_param.sig_fmt,
                              source_input_param.is3d, source_input_param.trans_fmt,  &regs) == 0) {
        di_regs.table_name |= TABLE_NAME_NR;
    } else {
        SYS_LOGE("%s GetNR2Params failed!\n",__FUNCTION__);
    }

    if (mPQdb->PQ_GetDemoSquitoParams(source_input_param.source_port, source_input_param.sig_fmt,
                                      source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
        di_regs.table_name |= TABLE_NAME_DEMOSQUITO;
    } else {
        SYS_LOGE("%s GetDemoSquitoParams failed!\n",__FUNCTION__);
    }

    di_regs.table_len = regs.length;

    am_reg_t tmp_buf[regs.length];
    for (unsigned int i=0;i<regs.length;i++) {
          tmp_buf[i].addr = regs.am_reg[i].addr;
          tmp_buf[i].mask = regs.am_reg[i].mask;
          tmp_buf[i].type = regs.am_reg[i].type;
          tmp_buf[i].val  = regs.am_reg[i].val;
    }

    di_regs.table_ptr = (long long)tmp_buf;

    ret = DI_LoadRegs(di_regs);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::resetLastSourceTypeSettings(void)
{
    cpq_setting_last_source_type = SOURCE_TYPE_MAX;
    cpq_setting_last_sig_fmt = TVIN_SIG_FMT_MAX;
    cpq_setting_last_trans_fmt = TVIN_TFMT_3D_MAX;
    return 0;
}

int CPQControl::SetPQMode(int pq_mode, int is_save , int is_autoswitch)
{
    SYS_LOGD("%s, pq_mode = %d\n", __FUNCTION__, pq_mode);
    int ret = -1;
    mAutoSwitchPCModeFlag = is_autoswitch;

    source_input_param_t source_input_param = GetCurrentSourceInputInfo();
    int cur_mode = GetPQMode();

    if (cur_mode == pq_mode) {
        SYS_LOGD("Same PQ mode,no need set again!\n");
        return 0;
    }

    if (0 == Cpq_SetPQMode((vpp_picture_mode_t)pq_mode, source_input_param)) {
        if (is_save == 1) {
            ret = SavePQMode(pq_mode);
        }

        if ((source_input_param.source_input >= SOURCE_HDMI1)
            || (source_input_param.source_input <= SOURCE_HDMI4))
        {
            if (cur_mode == VPP_PICTURE_MODE_MONITOR) {
                Cpq_SetPcModeStatus(0);
            } else if (pq_mode == VPP_PICTURE_MODE_MONITOR) {
                Cpq_SetPcModeStatus(1);
            }
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetPQMode(void)
{
    vpp_picture_mode_t data = VPP_PICTURE_MODE_STANDARD;
    int tmp_pic_mode = 0;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    mSSMAction->SSMReadPictureMode(source_input_param.source_input, &tmp_pic_mode);
    data = (vpp_picture_mode_t) tmp_pic_mode;

    if (data < VPP_PICTURE_MODE_STANDARD || data >= VPP_PICTURE_MODE_MAX) {
        data = VPP_PICTURE_MODE_STANDARD;
    }

    SYS_LOGD("GetPQMode, PQMode[%d].", (int)data);
    return (int)data;
}

int CPQControl::SavePQMode(int pq_mode)
{
    int ret = -1;
    SYS_LOGD("%s, pq_mode = %d\n", __FUNCTION__, pq_mode);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    ret = mSSMAction->SSMSavePictureMode(source_input_param.source_input, pq_mode);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return ret;
}

int CPQControl::Cpq_SetPcModeStatus(int value)
{
    int m_hdmi_fd = -1;
    int ret = -1;
    m_hdmi_fd = open(HDMIRX_DEV_PATH, O_RDWR);
    if (m_hdmi_fd < 0) {
        ret = -1;
        SYS_LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, HDMIRX_DEV_PATH, strerror(errno));
    }
    if (value == 1) {
        ret = ioctl(m_hdmi_fd, HDMI_IOC_PC_MODE_ON, NULL);
    }else if (value == 0) {
        ret = ioctl(m_hdmi_fd, HDMI_IOC_PC_MODE_OFF, NULL);
    }
    close(m_hdmi_fd);

    if (ret < 0) {
        SYS_LOGD("Cpq_SetPcModeStatus error!\n");
    } else {
        SYS_LOGD("Cpq_SetPcModeStatus success!\n");
    }

    return ret;
}

int CPQControl::Cpq_SetPQMode(vpp_picture_mode_t pq_mode, source_input_param_t source_input_param)
{
    vpp_pq_para_t pq_para;
    int ret = -1;

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        ret = mSSMAction->SSMReadBrightness(source_input_param.source_input, &pq_para.brightness);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: SSMReadBrightness failed!\n");
            return ret;
        }

        ret = mSSMAction->SSMReadContrast(source_input_param.source_input, &pq_para.contrast);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: SSMReadContrast failed!\n");
            return ret;
        }

        ret = mSSMAction->SSMReadSaturation(source_input_param.source_input, &pq_para.saturation);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: SSMReadSaturation failed!\n");
            return ret;
        }

        ret = mSSMAction->SSMReadHue(source_input_param.source_input, &pq_para.hue);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: SSMReadHue failed!\n");
            return ret;
        }

        ret = mSSMAction->SSMReadSharpness(source_input_param.source_input, &pq_para.sharpness);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: SSMReadSharpness failed!\n");
            return ret;
        }
    } else {
        ret = GetPQParams(source_input_param, pq_mode, &pq_para);
        if (ret < 0) {
            SYS_LOGE("Cpq_SetPQMode: GetPQParams failed!\n");
            return ret;
        }
    }

    ret =SetPQParams(pq_para, source_input_param);

    return ret;
}

int CPQControl::SetPQParams(vpp_pq_para_t pq_para, source_input_param_t source_input_param)
{
    int ret = 0, brightness = 50, contrast = 50, saturation = 50, hue = 50, sharnpess = 50;
    am_regs_t regs, regs_l;
    memset(&regs, 0, sizeof(am_regs_t));
    memset(&regs_l, 0, sizeof(am_regs_t));
    int level;

    tv_source_input_type_t source_type = source_input_param.source_type;

    if (pq_para.brightness >= 0 && pq_para.brightness <= 100) {
        if (mPQdb->PQ_GetBrightnessParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                          source_input_param.trans_fmt, pq_para.brightness, &brightness) == 0) {
        } else {
            SYS_LOGE("SetPQParams, PQ_GetBrightnessParams error!\n");
        }

        ret |= Cpq_SetVideoBrightness(brightness);
    }

    if (pq_para.saturation >= 0 && pq_para.saturation <= 100) {
        if (mPQdb->PQ_GetSaturationParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                          source_input_param.trans_fmt, pq_para.saturation, &saturation) == 0) {

            if (mbCpqCfg_hue_reverse) {
                pq_para.hue = 100 - pq_para.hue;
            } else {
                pq_para.hue = pq_para.hue;
            }

            if (mPQdb->PQ_GetHueParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                       source_input_param.trans_fmt, pq_para.hue, &hue) == 0) {
                if ((source_type == SOURCE_TYPE_TV || source_type == SOURCE_TYPE_AV) &&
                    (source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_M || source_input_param.sig_fmt == TVIN_SIG_FMT_CVBS_NTSC_443)) {

                } else {
                    hue = 0;
                }
            } else {
                SYS_LOGE("SetPQParams, PQ_GetHueParams error!\n");
            }
        } else {
            SYS_LOGE("SetPQParams, PQ_GetSaturationParams error!\n");
        }
        ret |= Cpq_SetVideoSaturationHue(saturation, hue);
    }

    if (pq_para.contrast >= 0 && pq_para.contrast <= 100) {
        if (mPQdb->PQ_GetContrastParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                        source_input_param.trans_fmt, pq_para.contrast, &contrast) == 0) {
        } else {
            SYS_LOGE("SetPQParams, PQ_GetContrastParams error!\n");
        }

        ret |= Cpq_SetVideoContrast(contrast);
    }



    if (pq_para.sharpness >= 0 && pq_para.sharpness <= 100) {
        level = pq_para.sharpness;

        if (mPQdb->PQ_GetSharpnessParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                         source_input_param.trans_fmt, level, &regs, &regs_l) == 0) {
            if (Cpq_LoadRegs(regs) < 0) {
                SYS_LOGE("SetPQParams, load reg for sharpness0 failed!\n");
            }
            if (mPQdb->getSharpnessFlag() == 6 && Cpq_LoadRegs(regs_l) < 0) {
                SYS_LOGE("SetPQParams, load reg for sharpness1 failed!\n");
            }
        } else {
            SYS_LOGE("SetPQParams, PQ_GetSharpnessParams failed!\n");
        }
    }
    return ret;
}

int CPQControl::GetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t *pq_para)
{
    if (pq_para == NULL) {
        return -1;
    }

    if (mPQdb->PQ_GetPQModeParams(source_input_param.source_type, pq_mode, pq_para) == 0) {
        if (mbCpqCfg_pqmode_without_hue) {
            mSSMAction->SSMReadHue(source_input_param.source_input, &(pq_para->hue));
        }
    } else {
        SYS_LOGE("GetPQParams, PQ_GetPQModeParams failed!\n");
        return -1;
    }

    return 0;
}

//color temperature
int CPQControl::SetColorTemperature(int temp_mode, int is_save)
{
    int ret = -1;
    SYS_LOGD("%s, ColorTemperature mode = %d\n", __FUNCTION__, temp_mode);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (temp_mode == VPP_COLOR_TEMPERATURE_MODE_USER) {
        ret = Cpq_SetColorTemperatureUser((vpp_color_temperature_mode_t)temp_mode, source_input_param.source_type);
        if (ret < 0) {
            SYS_LOGD("Cpq_SetColorTemperatureUser failed!\n");
        }
    } else {
        ret = Cpq_SetColorTemperatureWithoutSave((vpp_color_temperature_mode_t)temp_mode, source_input_param.source_input);
        if (ret <  0) {
            SYS_LOGD("Cpq_SetColorTemperatureWithoutSave failed!\n");
        }
    }

    if ((ret == 0) && (is_save == 1)) {
        ret = SaveColorTemperature((vpp_color_temperature_mode_t)temp_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetColorTemperature(void)
{
    vpp_color_temperature_mode_t data = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();
    int tmp_temp_mode = 0;

    if (mbCpqCfg_colortemp_by_source) {
        mSSMAction->SSMReadColorTemperature((int)source_input_param.source_input, &tmp_temp_mode);
    } else {
        mSSMAction->SSMReadColorTemperature(0, &tmp_temp_mode);
    }

    data = (vpp_color_temperature_mode_t) tmp_temp_mode;

    if (data < VPP_COLOR_TEMPERATURE_MODE_STANDARD || data > VPP_COLOR_TEMPERATURE_MODE_USER) {
        data = VPP_COLOR_TEMPERATURE_MODE_STANDARD;
    }

    SYS_LOGD("GetColorTemperature, ColorTemperature[%d].", (int)data);
    return (int)data;
}

int CPQControl::SaveColorTemperature(int temp_mode)
{
    int ret = -1;
    SYS_LOGD("%s, ColorTemperature mode = %d\n", __FUNCTION__, temp_mode);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (mbCpqCfg_colortemp_by_source) {
        ret = mSSMAction->SSMSaveColorTemperature((int)source_input_param.source_input, (vpp_color_temperature_mode_t)temp_mode);
    } else {
        ret = mSSMAction->SSMSaveColorTemperature(0, temp_mode);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetColorTemperatureWithoutSave(vpp_color_temperature_mode_t Tempmode, tv_source_input_t tv_source_input __unused)
{
    tcon_rgb_ogo_t rgbogo;

    if (mbCpqCfg_gamma_onoff) {
        Cpq_SetGammaOnOff(0);
    } else {
        Cpq_SetGammaOnOff(1);
    }

    GetColorTemperatureParams(Tempmode, &rgbogo);

    if (GetEyeProtectionMode(mCurentSourceInputInfo.source_input))//if eye protection mode is enable, b_gain / 2.
        rgbogo.b_gain /= 2;

    return Cpq_SetRGBOGO(&rgbogo);
}

int CPQControl::Cpq_CheckColorTemperatureParamAlldata(source_input_param_t source_input_param)
{
    int ret= -1;
    unsigned short ret1 = 0, ret2 = 0;
    source_input_param.source_port = TVIN_PORT_HDMI0;

    ret = Cpq_CheckTemperatureDataLable();
    ret1 = Cpq_CalColorTemperatureParamsChecksum();
    ret2 = Cpq_GetColorTemperatureParamsChecksum();

    if (ret && (ret1 == ret2)) {
        SYS_LOGD("%s, color temperature param lable & checksum ok.\n",__FUNCTION__);
        if (Cpq_CheckColorTemperatureParams() == 0) {
            SYS_LOGD("%s, color temperature params check failed.\n", __FUNCTION__);
            Cpq_RestoreColorTemperatureParamsFromDB(source_input_param);
         }
    } else {
        SYS_LOGD("%s, color temperature param data error.\n", __FUNCTION__);

        Cpq_SetTemperatureDataLable();
        Cpq_RestoreColorTemperatureParamsFromDB(source_input_param);
    }

    return 0;
}

unsigned short CPQControl::Cpq_CalColorTemperatureParamsChecksum(void)
{
    unsigned char data_buf[SSM_CR_RGBOGO_LEN];
    unsigned short sum = 0;
    int cnt;
    USUC usuc;

    mSSMAction->SSMReadRGBOGOValue(0, SSM_CR_RGBOGO_LEN, data_buf);

    for (cnt = 0; cnt < SSM_CR_RGBOGO_LEN; cnt++) {
        sum += data_buf[cnt];
    }

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, sum);

    return sum;
}

int CPQControl::Cpq_SetColorTemperatureParamsChecksum(void)
{
    int ret = 0;
    USUC usuc;

    usuc.s = Cpq_CalColorTemperatureParamsChecksum();

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    ret |= mSSMAction->SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    return ret;
}

unsigned short CPQControl::Cpq_GetColorTemperatureParamsChecksum(void)
{
    USUC usuc;

    mSSMAction->SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN, SSM_CR_RGBOGO_CHKSUM_LEN, usuc.c);

    SYS_LOGD("%s, sum = 0x%X.\n", __FUNCTION__, usuc.s);

    return usuc.s;
}

int CPQControl::Cpq_SetColorTemperatureUser(vpp_color_temperature_mode_t temp_mode __unused, tv_source_input_type_t source_type)
{
    tcon_rgb_ogo_t rgbogo;
    unsigned int gain_r, gain_g, gain_b;

    if (mSSMAction->SSMReadRGBGainRStart(0, &gain_r) != 0) {
        SYS_LOGE("SSMReadRGBGain-RStart error!\n");
        return -1;
    }

    rgbogo.r_gain = gain_r;

    if (mSSMAction->SSMReadRGBGainGStart(0, &gain_g) != 0) {
        SYS_LOGE("SSMReadRGBGain-GStart error!\n");
        return -1;
    }

    rgbogo.g_gain = gain_g;

    if (mSSMAction->SSMReadRGBGainBStart(0, &gain_b) != 0) {
        SYS_LOGE("SSMReadRGBGain-BStart error!\n");
        return -1;
    }

    rgbogo.b_gain = gain_b;
    rgbogo.r_post_offset = 0;
    rgbogo.r_pre_offset = 0;
    rgbogo.g_post_offset = 0;
    rgbogo.g_pre_offset = 0;
    rgbogo.b_post_offset = 0;
    rgbogo.b_pre_offset = 0;

    if (GetEyeProtectionMode(mCurentSourceInputInfo.source_input))//if eye protection mode is enable, b_gain / 2.
    {
        SYS_LOGD("eye protection mode is enable!\n");
        rgbogo.b_gain /= 2;
    }

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    SYS_LOGE("Cpq_SetColorTemperatureUser, source_type_user[%d] failed!\n",source_type);
    return -1;
}

int CPQControl::Cpq_RestoreColorTemperatureParamsFromDB(source_input_param_t source_input_param)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    for (i = 0; i < 3; i++) {
        mPQdb->PQ_GetColorTemperatureParams((vpp_color_temperature_mode_t) i, source_input_param.source_port, source_input_param.sig_fmt,
                                            source_input_param.trans_fmt, &rgbogo);
        SaveColorTemperatureParams((vpp_color_temperature_mode_t) i, rgbogo);
    }

    Cpq_SetColorTemperatureParamsChecksum();

    return 0;
}

int CPQControl::Cpq_CheckTemperatureDataLable(void)
{
    USUC usuc;
    USUC ret;

    mSSMAction->SSMReadRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, ret.c);

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    if ((usuc.c[0] == ret.c[0]) && (usuc.c[1] == ret.c[1])) {
        SYS_LOGD("%s, lable ok.\n", __FUNCTION__);
        return 1;
    } else {
        SYS_LOGE("%s, lable error.\n", __FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetTemperatureDataLable(void)
{
    USUC usuc;
    int ret = 0;

    usuc.c[0] = 0x55;
    usuc.c[1] = 0xAA;

    ret = mSSMAction->SSMSaveRGBOGOValue(SSM_CR_RGBOGO_LEN - 2, 2, usuc.c);

    return ret;
}

int CPQControl::SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    SaveColorTemperatureParams(Tempmode, params);
    Cpq_SetColorTemperatureParamsChecksum();

    return 0;
}

int CPQControl::GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;
    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        ret |= mSSMAction->SSMReadRGBOGOValue(0, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(2, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(4, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(6, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(8, 2, usuc.c);
        params->r_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(10, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(12, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(14, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(16, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(18, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        ret |= mSSMAction->SSMReadRGBOGOValue(20, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(22, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(24, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(26, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(28, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(30, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(32, 2, usuc.c);
        params->b_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(34, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(36, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(38, 2, suc.c);
        params->b_post_offset = suc.s;
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        ret |= mSSMAction->SSMReadRGBOGOValue(40, 2, usuc.c);
        params->en = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(42, 2, suc.c);
        params->r_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(44, 2, suc.c);
        params->g_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(46, 2, suc.c);
        params->b_pre_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(48, 2, usuc.c);
        params->r_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(50, 2, usuc.c);
        params->g_gain = usuc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(52, 2, usuc.c);
        params->b_gain = usuc.s;
        ret |= mSSMAction->SSMReadRGBOGOValue(54, 2, suc.c);
        params->r_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(56, 2, suc.c);
        params->g_post_offset = suc.s;

        ret |= mSSMAction->SSMReadRGBOGOValue(58, 2, suc.c);
        params->b_post_offset = suc.s;
    }

    SYS_LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
         params->r_gain, params->g_gain, params->b_gain, params->r_post_offset,
         params->g_post_offset, params->b_post_offset);

    return ret;
}

int CPQControl::SaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params)
{
    SUC suc;
    USUC usuc;
    int ret = 0;

    if (VPP_COLOR_TEMPERATURE_MODE_STANDARD == Tempmode) { //standard
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(0, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(2, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(4, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(6, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(8, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(10, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(12, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(14, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(16, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(18, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_WARM == Tempmode) { //warm
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(20, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(22, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(24, 2, suc.c);
        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(26, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(28, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(30, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(32, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(34, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(36, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(38, 2, suc.c);
    } else if (VPP_COLOR_TEMPERATURE_MODE_COLD == Tempmode) { //cool
        usuc.s = params.en;
        ret |= mSSMAction->SSMSaveRGBOGOValue(40, 2, usuc.c);

        suc.s = params.r_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(42, 2, suc.c);

        suc.s = params.g_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(44, 2, suc.c);

        suc.s = params.b_pre_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(46, 2, suc.c);

        usuc.s = params.r_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(48, 2, usuc.c);

        usuc.s = params.g_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(50, 2, usuc.c);

        usuc.s = params.b_gain;
        ret |= mSSMAction->SSMSaveRGBOGOValue(52, 2, usuc.c);

        suc.s = params.r_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(54, 2, suc.c);

        suc.s = params.g_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(56, 2, suc.c);

        suc.s = params.b_post_offset;
        ret |= mSSMAction->SSMSaveRGBOGOValue(58, 2, suc.c);
    }

    SYS_LOGD("%s, rgain[%d], ggain[%d],bgain[%d],roffset[%d],goffset[%d],boffset[%d]\n", __FUNCTION__,
         params.r_gain, params.g_gain, params.b_gain, params.r_post_offset,
         params.g_post_offset, params.b_post_offset);
    return ret;
}

int CPQControl::Cpq_CheckColorTemperatureParams(void)
{
    int i = 0;
    tcon_rgb_ogo_t rgbogo;

    for (i = 0; i < 3; i++) {
        GetColorTemperatureParams((vpp_color_temperature_mode_t) i, &rgbogo);

        if (rgbogo.r_gain > 2047 || rgbogo.b_gain > 2047 || rgbogo.g_gain > 2047) {
            if (rgbogo.r_post_offset > 1023 || rgbogo.g_post_offset > 1023 || rgbogo.b_post_offset > 1023 ||
                rgbogo.r_post_offset < -1024 || rgbogo.g_post_offset < -1024 || rgbogo.b_post_offset < -1024) {
                return 0;
            }
        }
    }

    return 1;
}

//Brightness
int CPQControl::SetBrightness(int value, int is_save)
{
    int ret =0;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();
    if (0 == Cpq_SetBrightness(value, source_input_param)) {
        if (is_save == 1) {
            ret = mSSMAction->SSMSaveBrightness(source_input_param.source_input, value);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }
    return 0;
}

int CPQControl::GetBrightness(void)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();
    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        mSSMAction->SSMReadBrightness(source_input_param.source_input, &data);
    } else {
        if (GetPQParams(source_input_param, pq_mode, &pq_para) == 0) {
            data = pq_para.brightness;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("GetBrightness, Brightness[%d].", data);
    return data;
}

int CPQControl::SaveBrightness(int value)
{
    int ret = -1;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    ret =  mSSMAction->SSMSaveBrightness(source_input_param.source_input, value);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetBrightness(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int params;
    int level;

    if (value >= 0 && value <= 100) {
        level = value;
        if (mPQdb->PQ_GetBrightnessParams(source_input_param.source_type, source_input_param.sig_fmt,
                                          source_input_param.is3d, source_input_param.trans_fmt, level, &params)
                == 0) {
            if (Cpq_SetVideoBrightness(params) == 0) {
                return 0;
            }
        } else {
            SYS_LOGE("Vpp_SetBrightness, PQ_GetBrightnessParams failed!\n");
        }
    }

    return ret;
}

int CPQControl::Cpq_SetVideoBrightness(int value)
{
    SYS_LOGD("Cpq_SetVideoBrightness brightness : %d", value);
    FILE *fp = NULL;
    if (mbCpqCfg_brightness_withOSD) {
        fp = fopen("/sys/class/amvecm/brightness2", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/brightness2 error(%s)!\n", strerror(errno));
            return -1;
        }
    } else {
        fp = fopen("/sys/class/amvecm/brightness", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/brightness error(%s)!\n", strerror(errno));
            return -1;
    }
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

//Contrast

int CPQControl::SetContrast(int value, int is_save)
{
    int ret = 0;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (0 == Cpq_SetContrast(value, source_input_param)) {
        if (is_save == 1) {
            ret = mSSMAction->SSMSaveContrast(source_input_param.source_input, value);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::GetContrast(void)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        mSSMAction->SSMReadContrast(source_input_param.source_input, &data);
    } else {
        if (GetPQParams(source_input_param, pq_mode, &pq_para) == 0) {
            data = pq_para.contrast;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("GetContrast:value = %d\n",data);
    return data;
}

int CPQControl::SaveContrast(int value)
{
    int ret = -1;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    ret = mSSMAction->SSMSaveContrast(source_input_param.source_input, value);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetContrast(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int params;
    int level;

    if (value >= 0 && value <= 100) {
        level = value;
        if (mPQdb->PQ_GetContrastParams(source_input_param.source_type, source_input_param.sig_fmt,
                                        source_input_param.is3d, source_input_param.trans_fmt, level, &params) == 0) {
            if (Cpq_SetVideoContrast(params) == 0) {
                return 0;
            }
        } else {
            SYS_LOGE("Cpq_SetContrast, PQ_GetContrastParams failed!\n");
        }
    }

    return ret;
}

int CPQControl::Cpq_SetVideoContrast(int value)
{
    SYS_LOGD("Cpq_SetVideoContrast: %d", value);
    FILE *fp = NULL;

    if (mbCpqCfg_contrast_withOSD) {
        fp = fopen("/sys/class/amvecm/contrast2", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/contrast2 error(%s)!\n", strerror(errno));
            return -1;
        }
    } else {
        fp = fopen("/sys/class/amvecm/contrast", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/contrast error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;

    return 0;
}

//Saturation
int CPQControl::SetSaturation(int value, int is_save)
{
    int ret =0;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (0 == Cpq_SetSaturation(value, source_input_param)) {
        if (is_save == 1) {
            ret = mSSMAction->SSMSaveSaturation(source_input_param.source_input, value);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::GetSaturation(void)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        mSSMAction->SSMReadSaturation(source_input_param.source_input, &data);
    } else {
        if (GetPQParams(source_input_param, pq_mode, &pq_para) == 0) {
            data = pq_para.saturation;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("GetSaturation:value = %d\n",data);
    return data;
}

int CPQControl::SaveSaturation(int value)
{
  int ret = -1;
  SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
  source_input_param_t source_input_param = GetCurrentSourceInputInfo();

  ret = mSSMAction->SSMSaveSaturation(source_input_param.source_input, value);

  if (ret < 0) {
      SYS_LOGE("%s failed!\n",__FUNCTION__);
      return -1;
  } else {
      SYS_LOGD("%s success!\n",__FUNCTION__);
      return 0;
  }
}

int CPQControl::Cpq_SetSaturation(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int saturation = 0, hue = 0;
    int satuation_level = 0, hue_level = 0;

    if (value >= 0 && value <= 100) {
        satuation_level = value;
        hue_level = GetHue();
        ret = mPQdb->PQ_GetHueParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                source_input_param.trans_fmt, hue_level, &hue);
        if (ret == 0) {
            ret = mPQdb->PQ_GetSaturationParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                               source_input_param.trans_fmt, satuation_level, &saturation);
            if (ret == 0) {
                ret = Cpq_SetVideoSaturationHue(saturation, hue);
            } else {
                SYS_LOGE("PQ_GetSaturationParams failed!\n");
            }
        } else {
            SYS_LOGE("PQ_GetHueParams failed!\n");
        }
    }

    return ret;
}

//Hue
int CPQControl::SetHue(int value, int is_save)
{
    int ret = 0;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (0 == Cpq_SetHue(value, source_input_param)) {
        if (is_save == 1) {
            ret = mSSMAction->SSMSaveHue(source_input_param.source_input, value);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::GetHue(void)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        mSSMAction->SSMReadHue(source_input_param.source_input, &data);
    } else {
        if (GetPQParams(source_input_param, pq_mode, &pq_para) == 0) {
            data = pq_para.hue;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("GetHue:value = %d\n",data);
    return data;
}

int CPQControl::SaveHue(int value)
{
    int ret = -1;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    ret = mSSMAction->SSMSaveHue(source_input_param.source_input, value);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetHue(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    int hue_params = 0, saturation_params = 0;
    int hue_level = 0, saturation_level = 0;

    if (value >= 0 && value <= 100) {
        if (mbCpqCfg_hue_reverse) {
            hue_level = 100 - value;
        } else {
            hue_level = value;
        }

        ret = mPQdb->PQ_GetHueParams(source_input_param.source_type, source_input_param.sig_fmt,
                                   source_input_param.is3d, source_input_param.trans_fmt, hue_level, &hue_params);
        if (ret == 0) {
            saturation_level = GetSaturation();
            ret = mPQdb->PQ_GetSaturationParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                              source_input_param.trans_fmt, saturation_level, &saturation_params);
            if (ret == 0) {
                ret = Cpq_SetVideoSaturationHue(saturation_params, hue_params);
            } else {
                SYS_LOGE("PQ_GetSaturationParams failed!\n");
            }
        } else {
            SYS_LOGE("PQ_GetHueParams failed!\n");
        }
    }

    return ret;
}

int CPQControl::Cpq_SetVideoSaturationHue(int satVal, int hueVal)
{
    signed long temp;
    FILE *fp = NULL;

    SYS_LOGD("Cpq_SetVideoSaturationHue: %d %d", satVal, hueVal);

    if (mbCpqCfg_hue_withOSD) {
        fp = fopen("/sys/class/amvecm/saturation_hue_post", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/saturation_hue_post error(%s)!\n", strerror(errno));
            return -1;
        }
    } else {
        fp = fopen("/sys/class/amvecm/saturation_hue", "w");
        if (fp == NULL) {
            SYS_LOGE("Open /sys/class/amvecm/saturation_hue error(%s)!\n", strerror(errno));
            return -1;
        }
    }

    video_set_saturation_hue(satVal, hueVal, &temp);
    fprintf(fp, "0x%lx", temp);
    fclose(fp);
    fp = NULL;
    return 0;
}

void CPQControl::video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab)
{
    signed short ma = (signed short) (cos((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);
    signed short mb = (signed short) (sin((float) hue * PI / 128.0) * ((float) saturation / 128.0
                                      + 1.0) * 256.0);

    if (ma > 511) {
        ma = 511;
    }

    if (ma < -512) {
        ma = -512;
    }

    if (mb > 511) {
        mb = 511;
    }

    if (mb < -512) {
        mb = -512;
    }

    *mab = ((ma & 0x3ff) << 16) | (mb & 0x3ff);
}

void CPQControl::video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab)
{
    signed long temp = *mab;
    signed int ma = (signed int) ((temp << 6) >> 22);
    signed int mb = (signed int) ((temp << 22) >> 22);
    signed int sat16 = (signed int) ((sqrt(
                                          ((float) ma * (float) ma + (float) mb * (float) mb) / 65536.0) - 1.0) * 128.0);
    signed int hue16 = (signed int) (atan((float) mb / (float) ma) * 128.0 / PI);

    if (sat16 > 127) {
        sat16 = 127;
    }

    if (sat16 < -128) {
        sat16 = -128;
    }

    if (hue16 > 127) {
        hue16 = 127;
    }

    if (hue16 < -128) {
        hue16 = -128;
    }

    *sat = (signed char) sat16;
    *hue = (signed char) hue16;
}

//sharpness
int CPQControl::SetSharpness(int value, int is_enable, int is_save)
{
    int ret = 0;
    SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (Cpq_SetSharpness(value, source_input_param) == 0) {
        if ((is_save == 1) && (is_enable == 1)) {
            ret = mSSMAction->SSMSaveSharpness(source_input_param.source_input, value);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

    return 0;
}

int CPQControl::GetSharpness(void)
{
    int data = 50;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();

    if (pq_mode == VPP_PICTURE_MODE_USER) {
        mSSMAction->SSMReadSharpness(source_input_param.source_input, &data);
    } else {
        if (GetPQParams(source_input_param, pq_mode, &pq_para) == 0) {
            data = pq_para.sharpness;
        }
    }

    if (data < 0 || data > 100) {
        data = 50;
    }

    SYS_LOGD("GetSharpness:value = %d\n",data);
    return data;
}

int CPQControl::SaveSharpness(int value)
{
   int ret = -1;
   SYS_LOGD("%s, value = %d\n", __FUNCTION__, value);
   source_input_param_t source_input_param = GetCurrentSourceInputInfo();

   ret = mSSMAction->SSMSaveSharpness(source_input_param.source_input, value);

   if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
   return 0;
}

int CPQControl::Cpq_SetSharpness(int value, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs, regs_l;
    memset(&regs, 0, sizeof(am_regs_t));
    memset(&regs_l, 0, sizeof(am_regs_t));
    int level;

    if (value >= 0 && value <= 100) {
        level = value;

        if (mPQdb->PQ_GetSharpnessParams(source_input_param.source_type,  source_input_param.sig_fmt,
                                         source_input_param.is3d, source_input_param.trans_fmt, level, &regs, &regs_l) == 0) {
            SYS_LOGD("%s, sharpness flag:%d\n", __FUNCTION__, mPQdb->getSharpnessFlag());
            if (mPQdb->getSharpnessFlag() == 6) {
                if (Cpq_LoadRegs(regs) >= 0 &&Cpq_LoadRegs(regs_l) >= 0)
                    ret = 0;
            } else {
                if (Cpq_LoadRegs(regs) >= 0)
                    ret = 0;
            }
        } else {
        }
    }

    return ret;
}

//NoiseReductionMode
int CPQControl::SetNoiseReductionMode(int nr_mode, int is_save)
{
    int ret = 0;
    SYS_LOGD("%s, NoiseReductionMode = %d\n", __FUNCTION__, nr_mode);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (0 == Cpq_SetNoiseReductionMode((vpp_noise_reduction_mode_t)nr_mode, source_input_param)) {
        if (is_save == 1) {
            ret = SaveNoiseReductionMode((vpp_noise_reduction_mode_t)nr_mode);
        }
    } else {
        ret = -1;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::GetNoiseReductionMode(void)
{
    vpp_noise_reduction_mode_t data = VPP_NOISE_REDUCTION_MODE_MID;
    int tmp_nr_mode = 0;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    mSSMAction->SSMReadNoiseReduction(source_input_param.source_input, &tmp_nr_mode);
    data = (vpp_noise_reduction_mode_t) tmp_nr_mode;

    if (data < VPP_NOISE_REDUCTION_MODE_OFF || data > VPP_NOISE_REDUCTION_MODE_AUTO) {
        data = VPP_NOISE_REDUCTION_MODE_MID;
    }

    SYS_LOGD("GetNoiseReductionMode:value = %d\n",(int)data);
    return (int)data;
}

int CPQControl::SaveNoiseReductionMode(int nr_mode)
{
    int ret = 0;
    SYS_LOGD("%s, NoiseReductionMode = %d\n", __FUNCTION__, nr_mode);
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    ret = mSSMAction->SSMSaveNoiseReduction(source_input_param.source_input, nr_mode);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }
}

int CPQControl::Cpq_SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    am_pq_param_t di_regs;
    memset(&regs, 0x0, sizeof(am_regs_t));
    memset(&di_regs, 0x0,sizeof(am_pq_param_t));

    if (mbCpqCfg_new_nr) {
        if (mPQdb->PQ_GetNR2Params((vpp_noise_reduction2_mode_t)nr_mode, source_input_param.source_port,
                                  source_input_param.sig_fmt, source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
            di_regs.table_name = TABLE_NAME_NR;
            di_regs.table_len = regs.length;
            am_reg_t tmp_buf[regs.length];
            for (unsigned int i=0;i<regs.length;i++) {
                  tmp_buf[i].addr = regs.am_reg[i].addr;
                  tmp_buf[i].mask = regs.am_reg[i].mask;
                  tmp_buf[i].type = regs.am_reg[i].type;
                  tmp_buf[i].val  = regs.am_reg[i].val;
            }
            di_regs.table_ptr = (long long)tmp_buf;

            ret = DI_LoadRegs(di_regs);
        } else {
            SYS_LOGE("PQ_GetNR2Params failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save)
{
    int rw_val = 0;
    int ret = -1;
    SYS_LOGD("%s, GammaValue = %d\n", __FUNCTION__, gamma_curve);
    if (gamma_curve < VPP_GAMMA_CURVE_AUTO || gamma_curve > VPP_GAMMA_CURVE_MAX) {
        SYS_LOGE("%s failed: gamma_curve is invalid!\n", __FUNCTION__);
        return ret;
    }

    if (gamma_curve == VPP_GAMMA_CURVE_AUTO) {
        if (mSSMAction->SSMReadGammaValue(&rw_val) < 0) {
            gamma_curve = VPP_GAMMA_CURVE_DEFAULT;
        } else {
            gamma_curve = (vpp_gamma_curve_t)rw_val;
        }
    }
    SYS_LOGD("%s, gamma curve is %d.", __FUNCTION__, gamma_curve);

    ret = Cpq_LoadGamma(gamma_curve);
    if (ret == 0) {
        if (is_save == 1) {
            ret = mSSMAction->SSMSaveGammaValue(gamma_curve);

            if (ret < 0) {
                SYS_LOGE("%s failed!\n",__FUNCTION__);
            } else {
                SYS_LOGD("%s success!\n",__FUNCTION__);
            }
        }
    } else {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetGammaValue()
{
    int gammaValue = 0;

    if (mSSMAction->SSMReadGammaValue(&gammaValue) < 0) {
        SYS_LOGE("%s, SSMReadGammaValue ERROR!!!\n", __FUNCTION__);
        return -1;
    }

    SYS_LOGD("GetGammaValue:value = %d\n",gammaValue);
    return gammaValue;
}

//Displaymode
int CPQControl::SetDisplayMode(tv_source_input_t source_input, vpp_display_mode_t display_mode, int is_save)
{
    int ret = -1;
    SYS_LOGD("%s, source_input = %d, display_mode = %d\n", __FUNCTION__, source_input, display_mode);
    if (source_input == SOURCE_DTV) {
        ret = Cpq_SetDisplayModeForDTV(source_input, display_mode);
    } else {
        ret = Cpq_SetDisplayModeForDTV(source_input, display_mode);
        ret = Cpq_SetDisplayMode(source_input, display_mode);
    }

    if ((ret == 0) && (is_save == 1))
        ret = SaveDisplayMode(source_input, display_mode);

    return ret;
}

int CPQControl::GetDisplayMode(tv_source_input_t source_input)
{
    vpp_display_mode_t data = VPP_DISPLAY_MODE_169;
    int tmp_dis_mode = 0;

    mSSMAction->SSMReadDisplayMode(source_input, &tmp_dis_mode);
    data = (vpp_display_mode_t) tmp_dis_mode;

    if (data < VPP_DISPLAY_MODE_169 || data >= VPP_DISPLAY_MODE_MAX) {
        data = VPP_DISPLAY_MODE_169;
    }

    SYS_LOGD("GetDisplayMode, DisplayMode[%d].", (int)data);

    return data;
}

int CPQControl::SaveDisplayMode(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int ret = -1;
    SYS_LOGD("%s, source_input = %d, display_mode = %d\n", __FUNCTION__, source_input, display_mode);
    ret = mSSMAction->SSMSaveDisplayMode(source_input, (int)display_mode);

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetDisplayMode(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int ret = -1;
    tvin_cutwin_t cutwin;
    int video_screen_mode = SCREEN_MODE_16_9;

    source_input_param_t source_input_param = GetCurrentSourceInputInfo();
    ret = mPQdb->PQ_GetOverscanParams(source_input_param.source_type, source_input_param.sig_fmt, source_input_param.is3d,
                                      source_input_param.trans_fmt, display_mode, &cutwin);

    SYS_LOGD("Cpq_SetDisplayMode: get crop %d %d %d %d \n", cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);

    switch ( display_mode ) {
    case VPP_DISPLAY_MODE_169:
        video_screen_mode = SCREEN_MODE_16_9;
        break;
    case VPP_DISPLAY_MODE_MODE43:
        video_screen_mode = SCREEN_MODE_4_3;
        break;
    case VPP_DISPLAY_MODE_NORMAL:
        video_screen_mode = SCREEN_MODE_NORMAL;
        break;
    case VPP_DISPLAY_MODE_FULL:
        video_screen_mode = SCREEN_MODE_NONLINEAR;
        Cpq_SetNonLinearFactor(20);
        break;
    case VPP_DISPLAY_MODE_CROP_FULL:
        video_screen_mode = SCREEN_MODE_CROP_FULL;
        cutwin.vs = 0;
        cutwin.hs = 0;
        cutwin.ve = 0;
        cutwin.he = 0;
        break;
    case VPP_DISPLAY_MODE_NOSCALEUP:
        video_screen_mode = SCREEN_MODE_NORMAL_NOSCALEUP;
        break;
    case VPP_DISPLAY_MODE_FULL_REAL:
        video_screen_mode = SCREEN_MODE_16_9;    //added for N360 by haifeng.liu
        break;
    case VPP_DISPLAY_MODE_PERSON:
        video_screen_mode = SCREEN_MODE_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 20;
        cutwin.ve = cutwin.ve + 20;
        break;
    case VPP_DISPLAY_MODE_MOVIE:
        video_screen_mode = SCREEN_MODE_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 40;
        cutwin.ve = cutwin.ve + 40;
        break;
    case VPP_DISPLAY_MODE_CAPTION:
        video_screen_mode = SCREEN_MODE_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 55;
        cutwin.ve = cutwin.ve + 55;
        break;
    case VPP_DISPLAY_MODE_ZOOM:
        video_screen_mode = SCREEN_MODE_FULL_STRETCH;
        cutwin.vs = cutwin.vs + 70;
        cutwin.ve = cutwin.ve + 70;
        break;
    default:
        break;
    }

    if (source_input_param.source_type == SOURCE_TYPE_HDMI) {
        if ((display_mode == VPP_DISPLAY_MODE_FULL_REAL)
            || (GetPQMode() == VPP_PICTURE_MODE_MONITOR)) {
            cutwin.vs = 0;
            cutwin.hs = 0;
            cutwin.ve = 0;
            cutwin.he = 0;
        }
    }

    Cpq_SetVideoScreenMode(video_screen_mode);

    if (ret == 0) {
        ret = Cpq_SetVideoCrop(cutwin.vs, cutwin.hs, cutwin.ve, cutwin.he);
    } else {
        SYS_LOGD("PQ_GetOverscanParams failed!\n");
    }

    return ret;
}
int CPQControl::Cpq_SetDisplayModeForDTV(tv_source_input_t source_input, vpp_display_mode_t display_mode)
{
    int i = 0, value = 0;
    int ret = -1;
    ve_pq_load_t ve_pq_load_reg;
    memset(&ve_pq_load_reg, 0, sizeof(ve_pq_load_t));

    ve_pq_load_reg.param_id = TABLE_NAME_OVERSCAN;
    if (source_input == SOURCE_DTV) {
        ve_pq_load_reg.length = 4;
    } else {
        ve_pq_load_reg.length = 1;
    }

    ve_pq_table_t ve_pq_table[ve_pq_load_reg.length];
    tvin_cutwin_t cutwin[ve_pq_load_reg.length];
    tvin_cutwin_t cutwin_tmp;
    tvin_sig_fmt_t sig_fmt[ve_pq_load_reg.length];
    int flag[ve_pq_load_reg.length];
    memset(ve_pq_table, 0, sizeof(ve_pq_table));
    memset(cutwin, 0, sizeof(cutwin));
    memset(&cutwin_tmp, 0, sizeof(cutwin_tmp));
    memset(sig_fmt, 0, sizeof(sig_fmt));
    memset(flag, 0, sizeof(flag));

    if (source_input == SOURCE_DTV) {
        sig_fmt[0] = TVIN_SIG_FMT_HDMI_720X480P_60HZ;
        sig_fmt[1] = TVIN_SIG_FMT_HDMI_1280X720P_60HZ;
        sig_fmt[2] = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
        sig_fmt[3] = TVIN_SIG_FMT_HDMI_3840_2160_00HZ;

        flag[0] = 0x0;
        flag[1] = 0x1;
        flag[2] = 0x2;
        flag[3] = 0x3;

        for (i=0;i<ve_pq_load_reg.length;i++) {
            value = Cpq_GetScreenModeValue(display_mode);
            ve_pq_table[i].src_timing = (0x1<<31) | ((value & 0x7f) << 24) | ((SOURCE_DTV & 0x7f) << 16 ) | (flag[i]);
            ret = mPQdb->PQ_GetOverscanParams(SOURCE_TYPE_DTV, sig_fmt[i], mCurentSourceInputInfo.is3d,
                                        mCurentSourceInputInfo.trans_fmt, display_mode, &cutwin_tmp);
            if (ret == 0) {
                if (display_mode == VPP_DISPLAY_MODE_CROP_FULL) {
                    cutwin[i].he = 0;
                    cutwin[i].hs = 0;
                    cutwin[i].ve = 0;
                    cutwin[i].vs = 0;
                } else if (display_mode == VPP_DISPLAY_MODE_PERSON) {
                    cutwin[i].he = cutwin_tmp.he;
                    cutwin[i].hs = cutwin_tmp.hs;
                    cutwin[i].ve = cutwin_tmp.ve + 20;
                    cutwin[i].vs = cutwin_tmp.vs + 20;
                } else if (display_mode == VPP_DISPLAY_MODE_MOVIE) {
                    cutwin[i].he = cutwin_tmp.he;
                    cutwin[i].hs = cutwin_tmp.hs;
                    cutwin[i].ve = cutwin_tmp.ve + 40;
                    cutwin[i].vs = cutwin_tmp.vs + 40;
                } else if (display_mode == VPP_DISPLAY_MODE_CAPTION) {
                    cutwin[i].he = cutwin_tmp.he;
                    cutwin[i].hs = cutwin_tmp.hs;
                    cutwin[i].ve = cutwin_tmp.ve + 55;
                    cutwin[i].vs = cutwin_tmp.vs + 55;
                } else if (display_mode == VPP_DISPLAY_MODE_ZOOM) {
                    cutwin[i].he = cutwin_tmp.he;
                    cutwin[i].hs = cutwin_tmp.hs;
                    cutwin[i].ve = cutwin_tmp.ve + 70;
                    cutwin[i].vs = cutwin_tmp.vs + 70;
                } else {
                    cutwin[i].he = cutwin_tmp.he;
                    cutwin[i].hs = cutwin_tmp.hs;
                    cutwin[i].ve = cutwin_tmp.ve;
                    cutwin[i].vs = cutwin_tmp.vs;
                }

                ve_pq_table[i].value1 = ((cutwin[i].he & 0xffff)<<16) | (cutwin[i].hs & 0xffff);
                ve_pq_table[i].value2 = ((cutwin[i].ve & 0xffff)<<16) | (cutwin[i].vs & 0xffff);
            } else {
                SYS_LOGD("PQ_GetOverscanParams failed!\n");
            }
        }
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;
    } else {
        ve_pq_table[0].src_timing = (0x0<<31) | ((value & 0x7f) << 24) | ((source_input & 0x7f) << 16 ) | (0x0);
        ve_pq_table[0].value1 = 0;
        ve_pq_table[0].value2 = 0;
        ve_pq_load_reg.param_ptr = (long long)ve_pq_table;

        ret = 0;
    }

    if (ret == 0) {
        ret = Cpq_LoadDisplayModeRegs(ve_pq_load_reg);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

}

int CPQControl::Cpq_GetScreenModeValue(vpp_display_mode_t display_mode)
{
    int video_screen_mode = SCREEN_MODE_16_9;

    switch ( display_mode ) {
    case VPP_DISPLAY_MODE_169:
    case VPP_DISPLAY_MODE_FULL_REAL:
        video_screen_mode = SCREEN_MODE_16_9;
        break;
    case VPP_DISPLAY_MODE_MODE43:
        video_screen_mode = SCREEN_MODE_4_3;
        break;
    case VPP_DISPLAY_MODE_CROP:
        video_screen_mode = SCREEN_MODE_CROP;
        break;
    case VPP_DISPLAY_MODE_CROP_FULL:
        video_screen_mode = SCREEN_MODE_CROP_FULL;
        break;
    case VPP_DISPLAY_MODE_NORMAL:
        video_screen_mode = SCREEN_MODE_NORMAL;
        break;
    case VPP_DISPLAY_MODE_FULL:
        video_screen_mode = SCREEN_MODE_NONLINEAR;
        break;
    case VPP_DISPLAY_MODE_NOSCALEUP:
        video_screen_mode = SCREEN_MODE_NORMAL_NOSCALEUP;
        break;
    case VPP_DISPLAY_MODE_PERSON:
    case VPP_DISPLAY_MODE_MOVIE:
    case VPP_DISPLAY_MODE_CAPTION:
    case VPP_DISPLAY_MODE_ZOOM:
        video_screen_mode = SCREEN_MODE_FULL_STRETCH;
        break;
    default:
        break;
    }

    return video_screen_mode;
}

int CPQControl::Cpq_SetVideoScreenMode(int value)
{
    SYS_LOGD("Cpq_SetVideoScreenMode, value = %d\n" , value);

    char val[64] = {0};
    sprintf(val, "%d", value);
    pqWriteSys(SCREEN_MODE_PATH, val);
    return 0;
}

int CPQControl::Cpq_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1)
{
    char set_str[32];

    SYS_LOGD("Cpq_SetVideoCrop value: %d %d %d %d\n", Voffset0, Hoffset0, Voffset1, Hoffset1);
    int fd = open(CROP_PATH, O_RDWR);
    if (fd < 0) {
        SYS_LOGE("Open %s error(%s)!\n", CROP_PATH, strerror(errno));
        return -1;
    }

    memset(set_str, 0, 32);
    sprintf(set_str, "%d %d %d %d", Voffset0, Hoffset0, Voffset1, Hoffset1);
    write(fd, set_str, strlen(set_str));
    close(fd);

    return 0;
}

int CPQControl::Cpq_SetNonLinearFactor(int value)
{
    SYS_LOGD("Cpq_SetNonLinearFactor : %d\n", value);
    FILE *fp = fopen(NOLINER_FACTORY, "w");
    if (fp == NULL) {
        SYS_LOGE("Open %s error(%s)!\n", NOLINER_FACTORY, strerror(errno));
        return -1;
    }

    fprintf(fp, "%d", value);
    fclose(fp);
    fp = NULL;
    return 0;
}

//Backlight
int CPQControl::SetBacklight(tv_source_input_t source_input, int value, int is_save)
{
    int ret = -1;
    int set_value;

    SYS_LOGD("%s: source_input = %d, value = %d\n", __FUNCTION__, source_input, value);
    if (value < 0 || value > 100) {
        value = 100;
    }

    if (mbVppCfg_backlight_reverse) {
        set_value = (100 - value) * 255 / 100;
    } else {
        set_value = value * 255 / 100;
    }

    ret = Cpq_SetBackLightLevel(set_value);
    if ((ret >= 0) && (is_save == 1)) {
        ret = SaveBacklight(source_input, value);
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return 0;
    }

}

int CPQControl::GetBacklight(tv_source_input_t source_input)
{
    int data = 0;
    vpp_pq_para_t pq_para;
    source_input_param_t source_input_param = GetCurrentSourceInputInfo();

    if (mbVppCfg_pqmode_depend_bklight) {
        vpp_picture_mode_t pq_mode = (vpp_picture_mode_t)GetPQMode();
        if (pq_mode == VPP_PICTURE_MODE_USER) {
            mSSMAction->SSMReadBackLightVal(source_input, &data);
        } else {
            GetPQParams(source_input_param, pq_mode, &pq_para);
            data = pq_para.backlight;
        }
    } else {
        source_input = SOURCE_TV;
        mSSMAction->SSMReadBackLightVal(source_input, &data);
    }

    if (data < 0 || data > 100) {
        data = DEFAULT_BACKLIGHT_BRIGHTNESS;
    }

    SYS_LOGD("GetBacklight, Backlight[%d].", (int)data);
    return data;
}

int CPQControl::SaveBacklight(tv_source_input_t source_input, int value)
{
    int ret = -1;
    SYS_LOGD("%s: source_input = %d, value = %d\n", __FUNCTION__, source_input, value);
    if (!mbVppCfg_pqmode_depend_bklight) {
        source_input = SOURCE_TV;
    }

    if (value < 0 || value > 100) {
        value = 100;
    }

    ret = mSSMAction->SSMSaveBackLightVal(source_input, value);

    return ret;
}

int CPQControl::Cpq_SetBackLightLevel(int value)
{
    SYS_LOGD("Cpq_SetBackLightLevel: value = %d\n", value);
    char val[64] = {0};
    sprintf(val, "%d", value);
    return pqWriteSys(BACKLIGHT_PATH, val);
}

//PQ Factory
int CPQControl::FactoryResetPQMode(void)
{
    mPQdb->PQ_ResetAllPQModeParams();
    return 0;
}

int CPQControl::FactoryResetColorTemp(void)
{
    mPQdb->PQ_ResetAllColorTemperatureParams();
    return 0;
}

int CPQControl::FactorySetPQParam(source_input_param_t source_input_param, int pq_mode, int id, int value)
{
    int ret = -1;
    vpp_pq_para_t pq_para;

    if (mPQdb->PQ_GetPQModeParams(source_input_param.source_type, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
        switch ((vpp_pq_param_t)id) {
            case BRIGHTNESS: {
                pq_para.brightness = value;
                break;
            }
            case CONTRAST: {
                pq_para.contrast = value;
                break;
            }
            case SATURATION: {
                pq_para.saturation = value;
                break;
            }
            case SHARPNESS: {
                pq_para.sharpness = value;
                break;
            }
            case HUE: {
                pq_para.hue = value;
                break;
            }
            case BACKLIGHT: {
                pq_para.backlight = value;
                break;
            }
            case NR: {
                pq_para.nr = value;
                break;
            }
            default: {
                SYS_LOGE("FactorySetPQParam: Param ID error!\n");
                break;
            }
        }

        if (mPQdb->PQ_SetPQModeParams(source_input_param.source_type, (vpp_picture_mode_t) pq_mode, &pq_para) == 0) {
            ret = 0;
        } else {
            SYS_LOGE("%s -PQ_SetPQModeParams",__FUNCTION__);
            ret = 1;
        }
    } else {
        ret = -1;
        SYS_LOGE("%s -PQ_GetPQModeParams error!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::FactoryGetPQParam(source_input_param_t source_input_param, int pq_mode, int id)
{
    vpp_pq_para_t pq_para;

    if (mPQdb->PQ_GetPQModeParams(source_input_param.source_type, (vpp_picture_mode_t) pq_mode, &pq_para) != 0) {
        SYS_LOGE("FactoryGetPQParam error!\n");
        return -1;
    }

    switch ((vpp_pq_param_t)id) {
        case BRIGHTNESS: {
            return pq_para.brightness;
        }
        case CONTRAST: {
            return pq_para.contrast;
        }
        case SATURATION: {
            return pq_para.saturation;
        }
        case SHARPNESS: {
            return pq_para.sharpness;
        }
        case HUE: {
            return pq_para.hue;
        }
        case BACKLIGHT: {
            return pq_para.backlight;
        }
        case NR: {
            return pq_para.nr;
        }
        default: {
            SYS_LOGE("FactoryGetPQParam: Param ID error!\n");
            return -1;
        }
    }
}

int CPQControl::FactorySetColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value)
{
    tcon_rgb_ogo_t rgbogo;

    GetColorTemperatureParams((vpp_color_temperature_mode_t) colortemperature_mode, &rgbogo);
    switch (id) {
        case EN: {
            rgbogo.en = value;
            break;
        }
        case PRE_OFFSET_R: {
            rgbogo.r_pre_offset = value;
            break;
        }
        case PRE_OFFSET_G: {
            rgbogo.g_pre_offset = value;
            break;
        }
        case PRE_OFFSET_B: {
            rgbogo.b_pre_offset = value;
            break;
        }
        case POST_OFFSET_R: {
            rgbogo.r_post_offset = value;
            break;
        }
        case POST_OFFSET_G: {
            rgbogo.g_post_offset = value;
            break;
        }
        case POST_OFFSET_B: {
            rgbogo.b_post_offset = value;
            break;
        }
        case GAIN_R: {
            rgbogo.r_gain = value;
            break;
        }
        case GAIN_G: {
            rgbogo.g_gain = value;
            break;
        }
        case GAIN_B: {
            rgbogo.b_gain = value;
            break;
        }
        default: {
            SYS_LOGE("FactoryGetPQParam: Param ID error!\n");
            break;
        }
    }

    rgbogo.en = 1;

    if (Cpq_SetRGBOGO(&rgbogo) == 0) {
        return 0;
    }

    SYS_LOGE("FactorySetColorTemperatureParam error!\n");
    return -1;
}

int CPQControl::FactoryGetColorTemperatureParam(int colortemp_mode, pq_color_param_t id)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t)colortemp_mode, &rgbogo)) {
        switch (id) {
            case EN: {
                return rgbogo.en;
        }
            case PRE_OFFSET_R: {
                return rgbogo.r_pre_offset;
            }
            case PRE_OFFSET_G: {
                return rgbogo.g_pre_offset;
            }
            case PRE_OFFSET_B: {
                return rgbogo.b_pre_offset;
            }
            case POST_OFFSET_R: {
                return rgbogo.r_post_offset;
            }
            case POST_OFFSET_G: {
                return rgbogo.g_post_offset;
            }
            case POST_OFFSET_B: {
                return rgbogo.b_post_offset;
            }
            case GAIN_R: {
                return rgbogo.r_gain;
            }
            case GAIN_G: {
                return rgbogo.g_gain;
            }
            case GAIN_B: {
                return rgbogo.b_gain;
            }
            default: {
                SYS_LOGE("FactoryGetColorTemperatureParam: Param ID error!\n");
                return -1;
            }
        }
    }

    SYS_LOGE("FactoryGetColorTemperatureParam error!\n");
    return -1;
}

int CPQControl::FactorySaveColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value)
{
    tcon_rgb_ogo_t rgbogo;

    if (0 == GetColorTemperatureParams((vpp_color_temperature_mode_t)colortemperature_mode, &rgbogo)) {
        switch (id) {
            case PRE_OFFSET_R: {
                rgbogo.r_pre_offset = value;
                break;
            }
            case PRE_OFFSET_G: {
                rgbogo.g_pre_offset = value;
                break;
            }
            case PRE_OFFSET_B: {
                rgbogo.b_pre_offset = value;
                break;
            }
            case POST_OFFSET_R: {
                rgbogo.r_post_offset = value;
                break;
            }
            case POST_OFFSET_G: {
                rgbogo.g_post_offset = value;
                break;
            }
            case POST_OFFSET_B: {
                rgbogo.b_post_offset = value;
                break;
            }
            case GAIN_R: {
                rgbogo.r_gain = value;
                break;
            }
            case GAIN_G: {
                rgbogo.g_gain = value;
                break;
            }
            case GAIN_B: {
                rgbogo.b_gain = value;
                break;
            }
            default: {
                SYS_LOGE("FactorySaveColorTemperatureParam: Param ID error!\n");
                break;
            }
        }

        return SetColorTemperatureParams((vpp_color_temperature_mode_t)colortemperature_mode, rgbogo);
    }

    SYS_LOGE("FactorySaveColorTemp_Boffset error!\n");
    return -1;
}
//End PQ Factory

//Other factory

int CPQControl::FactoryResetNonlinear(void)
{
    mPQdb->PQ_ResetAllNoLineParams();
    return 0;
}

int CPQControl::FactorySetParamsDefault(void)
{
    FactoryResetPQMode();
    FactoryResetNonlinear();
    FactoryResetColorTemp();
    mPQdb->PQ_ResetAllOverscanParams();
    return 0;
}

int CPQControl::FactorySetNolineParams(source_input_param_t source_input_param, int type __unused, noline_params_t noline_params)
{
    int ret = -1;

    ret = mPQdb->PQ_SetNoLineAllBrightnessParams(source_input_param.source_type,
                noline_params.osd0, noline_params.osd25, noline_params.osd50, noline_params.osd75,
                noline_params.osd100);

    return ret;
}

int CPQControl::FactoryGetNolineParams(source_input_param_t source_input_param, int type, noline_params_ID_t id)
{
    int ret = -1;
    noline_params_t noline_params;

    memset(&noline_params, 0, sizeof(noline_params_t));

    switch (type) {
    case NOLINE_PARAMS_TYPE_BRIGHTNESS:
        ret = mPQdb->PQ_GetNoLineAllBrightnessParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_CONTRAST:
        ret = mPQdb->PQ_GetNoLineAllContrastParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_SATURATION:
        ret = mPQdb->PQ_GetNoLineAllSaturationParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_HUE:
        ret = mPQdb->PQ_GetNoLineAllHueParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_SHARPNESS:
        ret = mPQdb->PQ_GetNoLineAllSharpnessParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    case NOLINE_PARAMS_TYPE_VOLUME:
        ret = mPQdb->PQ_GetNoLineAllVolumeParams(source_input_param.source_type,
                &noline_params.osd0, &noline_params.osd25, &noline_params.osd50,
                &noline_params.osd75, &noline_params.osd100);

    default:
        break;
    }

    switch (id) {
    case OSD_0: {
        return noline_params.osd0;
    }
    case OSD_25: {
        return noline_params.osd25;
    }
    case OSD_50: {
        return noline_params.osd50;
    }
    case OSD_75: {
        return noline_params.osd75;
    }
    case OSD_100: {
        return noline_params.osd100;
    }
    default: {
        return -1;
    }
    }
}

int CPQControl::FactorySetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t)
{
    int ret = -1;

    ret = mPQdb->PQ_SetOverscanParams(source_input_param.source_type,
                                      source_input_param.sig_fmt,
                                      source_input_param.is3d,
                                      source_input_param.trans_fmt,
                                      cutwin_t);
    if (ret != 0) {
        SYS_LOGE("%s, PQ_SetOverscanParams fail.\n", __FUNCTION__);
        return -1;
    }

    return ret;
}

int CPQControl::FactoryGetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_param_t id )
{
    int ret = -1;
    tvin_cutwin_t cutwin_t;
    memset(&cutwin_t, 0, sizeof(cutwin_t));

    if (source_input_param.trans_fmt < TVIN_TFMT_2D || source_input_param.trans_fmt > TVIN_TFMT_3D_LDGD) {
        return 0;
    }

    ret = mPQdb->PQ_GetOverscanParams(source_input_param.source_type,
                                      source_input_param.sig_fmt,
                                      source_input_param.is3d,
                                      source_input_param.trans_fmt,
                                      VPP_DISPLAY_MODE_169,
                                      &cutwin_t);

    if (ret != 0) {
        SYS_LOGE("%s, PQ_GetOverscanParams faild.\n", __FUNCTION__);
        return -1;
    }

    switch (id) {
        case CUTWIN_HE:
            return cutwin_t.he;
        case CUTWIN_HS:
            return cutwin_t.hs;
        case CUTWIN_VE:
            return cutwin_t.ve;
        case CUTWIN_VS:
            return cutwin_t.vs;
        default:
            return -1;
    }
}

int CPQControl::FactorySetGamma(int gamma_r_value, int gamma_g_value, int gamma_b_value)
{
    int ret = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    memset(gamma_r.data, (unsigned short)gamma_r_value, 256);
    memset(gamma_g.data, (unsigned short)gamma_g_value, 256);
    memset(gamma_b.data, (unsigned short)gamma_b_value, 256);

    ret |= Cpq_SetGammaTbl_R((unsigned short *) gamma_r.data);
    ret |= Cpq_SetGammaTbl_G((unsigned short *) gamma_g.data);
    ret |= Cpq_SetGammaTbl_B((unsigned short *) gamma_b.data);

    return ret;
}

int CPQControl::FcatorySSMRestore(void)
{
    return mSSMAction->SSMRestoreDefault(0, true);
}
//End Other factory

int CPQControl::Cpq_SetXVYCCMode(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs, regs_1;
    memset(&regs, 0, sizeof(am_regs_t));
    memset(&regs_1, 0, sizeof(am_regs_t));

    if (mbCpqCfg_xvycc_switch_control) {
        if (mPQdb->PQ_GetXVYCCParams((vpp_xvycc_mode_t) xvycc_mode, source_input_param.source_port, source_input_param.sig_fmt,
                                     source_input_param.is3d, source_input_param.trans_fmt, &regs, &regs_1) == 0) {
            ret = Cpq_LoadRegs(regs);
            ret |= Cpq_LoadRegs(regs_1);
        } else {
            SYS_LOGE("PQ_GetXVYCCParams failed!\n");
        }
    } else {
        SYS_LOGD("XVYCC disabled!\n");
        ret = 0;
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}
int CPQControl::SetColorDemoMode(vpp_color_demomode_t demomode)
{
    cm_regmap_t regmap;
    unsigned long *temp_regmap;
    int i = 0;
    int tmp_demo_mode = 0;
    vpp_display_mode_t displaymode = VPP_DISPLAY_MODE_MODE43;

    switch (demomode) {
    case VPP_COLOR_DEMO_MODE_YOFF:
        temp_regmap = DemoColorYOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_COFF:
        temp_regmap = DemoColorCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_GOFF:
        temp_regmap = DemoColorGOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_MOFF:
        temp_regmap = DemoColorMOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ROFF:
        temp_regmap = DemoColorROffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_BOFF:
        temp_regmap = DemoColorBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_RGBOFF:
        temp_regmap = DemoColorRGBOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_YMCOFF:
        temp_regmap = DemoColorYMCOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLOFF:
        temp_regmap = DemoColorALLOffRegMap;
        break;

    case VPP_COLOR_DEMO_MODE_ALLON:
    default:
        if (displaymode == VPP_DISPLAY_MODE_MODE43) {
            temp_regmap = DemoColorSplit4_3RegMap;
        } else {
            temp_regmap = DemoColorSplitRegMap;
        }

        break;
    }

    for (i = 0; i < CM_REG_NUM; i++) {
        regmap.reg[i] = temp_regmap[i];
    }

    if (Cpq_SetCMRegisterMap(&regmap) == 0) {
        tmp_demo_mode = demomode;
        SYS_LOGD("SetColorDemoMode, demomode[%d] success.", demomode);
        return 0;
    }

    SYS_LOGE("SetColorDemoMode, demomode[%d] failed.", demomode);
    return -1;
}

int CPQControl::Cpq_SetCMRegisterMap(struct cm_regmap_s *pRegMap)
{
    SYS_LOGD("Cpq_SetCMRegisterMap AMSTREAM_IOC_CM_REGMAP");
    int rt = VPPDeviceIOCtl(AMSTREAM_IOC_CM_REGMAP, pRegMap);
    if (rt < 0) {
        SYS_LOGE("Cpq_api_SetCMRegisterMap, error(%s)!\n", strerror(errno));
    }

    return rt;
}

int CPQControl::SetBaseColorMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param)
{
    int ret = -1;
    if (0 == SetBaseColorModeWithoutSave(basemode, source_input_param)) {
        ret = SaveBaseColorMode(basemode);
    } else {
        SYS_LOGE("SetBaseColorMode() Failed!!!");
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

vpp_color_basemode_t CPQControl::GetBaseColorMode(void)
{
    vpp_color_basemode_t data = VPP_COLOR_BASE_MODE_OFF;
    unsigned char tmp_base_mode = 0;
    mSSMAction->SSMReadColorBaseMode(&tmp_base_mode);
    data = (vpp_color_basemode_t) tmp_base_mode;
    if (data < VPP_COLOR_BASE_MODE_OFF || data >= VPP_COLOR_BASE_MODE_MAX) {
        data = VPP_COLOR_BASE_MODE_OPTIMIZE;
    }

    return data;
}

int CPQControl::SaveBaseColorMode(vpp_color_basemode_t basemode)
{
    int ret = -1;

    if (basemode == VPP_COLOR_BASE_MODE_DEMO) {
        ret = 0;
    } else {
        ret |= mSSMAction->SSMSaveColorBaseMode(basemode);
    }

    return ret;
}

int CPQControl::SetBaseColorModeWithoutSave(vpp_color_basemode_t basemode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));

    if (mbCpqCfg_new_cm) {
        if (mPQdb->PQ_GetCM2Params((vpp_color_management2_t) basemode, source_input_param.source_port, source_input_param.sig_fmt,
                                     source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
            ret = Cpq_LoadRegs(regs);
        } else {
            SYS_LOGE("PQ_GetCM2Params failed!\n");
        }
    }

    return ret;
}

int CPQControl::Cpq_SetBaseColorMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param)
{
    int ret = -1;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));

    if (mbCpqCfg_new_cm) {
        if (mPQdb->PQ_GetCM2Params((vpp_color_management2_t) basemode, source_input_param.source_port, source_input_param.sig_fmt,
                                    source_input_param.is3d, source_input_param.trans_fmt, &regs) == 0) {
            ret = Cpq_LoadRegs(regs);
        } else {
            SYS_LOGE("PQ_GetCM2Params failed!\n");
        }
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}


int CPQControl::Cpq_SetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_S_RGB_OGO, rgbogo);

    usleep(50 * 1000);

    if (ret < 0) {
        SYS_LOGE("Cpq_api_SetRGBOGO, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_GetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo)
{
    SYS_LOGD("Cpq_GetRGBOGO AMVECM_IOC_G_RGB_OGO");
    int ret = VPPDeviceIOCtl(AMVECM_IOC_G_RGB_OGO, rgbogo);
    if (ret < 0) {
        SYS_LOGE("Cpq_api_GetRGBOGO, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_LoadGamma(vpp_gamma_curve_t gamma_curve)
{
    int ret = -1;
    int panel_id = 0;
    tcon_gamma_table_t gamma_r, gamma_g, gamma_b;

    ret = mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Red", &gamma_r);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Green", &gamma_g);
    ret |= mPQdb->PQ_GetGammaSpecialTable(gamma_curve, "Blue", &gamma_b);

    if (ret == 0) {
        Cpq_SetGammaTbl_R((unsigned short *) gamma_r.data);
        Cpq_SetGammaTbl_G((unsigned short *) gamma_g.data);
        Cpq_SetGammaTbl_B((unsigned short *) gamma_b.data);
    } else {
        SYS_LOGE("%s, PQ_GetGammaSpecialTable failed!", __FUNCTION__);
    }

    return ret;
}



int CPQControl::Cpq_SetGammaTbl_R(unsigned short red[256])
{
    struct tcon_gamma_table_s Redtbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Redtbl.data[i] = red[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_R, &Redtbl);
    if (ret < 0) {
        SYS_LOGE("Cpq_SetGammaTbl_R, error(%s)!\n", strerror(errno));
    }
    return ret;
}

int CPQControl::Cpq_SetGammaTbl_G(unsigned short green[256])
{
    struct tcon_gamma_table_s Greentbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Greentbl.data[i] = green[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_G, &Greentbl);

    if (ret < 0) {
        SYS_LOGE("Cpq_SetGammaTbl_G, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetGammaTbl_B(unsigned short blue[256])
{
    struct tcon_gamma_table_s Bluetbl;
    int ret = -1, i = 0;

    for (i = 0; i < 256; i++) {
        Bluetbl.data[i] = blue[i];
    }

    ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_B, &Bluetbl);

    if (ret < 0) {
        SYS_LOGE("Cpq_SetGammaTbl_B, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetGammaOnOff(unsigned char onoff)
{
    int ret = -1;

    if (onoff == 1) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_EN);
        SYS_LOGD("Cpq_SetGammaOnOff AMVECM_IOC_GAMMA_TABLE_EN");
    }

    if (onoff == 0) {
        ret = VPPDeviceIOCtl(AMVECM_IOC_GAMMA_TABLE_DIS);
        SYS_LOGD("Cpq_SetGammaOnOff AMVECM_IOC_GAMMA_TABLE_DIS");
    }

    if (ret < 0) {
        SYS_LOGE("Cpq_api_SetGammaOnOff, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::SetDNLP(source_input_param_t source_input_param)
{
    int ret = -1;
    ve_dnlp_curve_param_t newdnlp;

    if (mPQdb->PQ_GetDNLPParams(source_input_param.source_port, source_input_param.sig_fmt, source_input_param.is3d,
                                source_input_param.trans_fmt, DYNAMIC_CONTRAST_MID, &newdnlp) == 0) {
        ret = Cpq_SetVENewDNLP(&newdnlp);
        Cpq_SetDNLPStatus("1");
    } else {
        SYS_LOGE("mPQdb->PQ_GetDNLPParams failed!\n");
    }

    if (ret < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::Cpq_SetVENewDNLP(const ve_dnlp_curve_param_t *pDNLP)
{
    int ret = VPPDeviceIOCtl(AMVECM_IOC_VE_NEW_DNLP, pDNLP);
    if (ret < 0) {
        SYS_LOGE("Cpq_SetVENewDNLP, error(%s)!\n", strerror(errno));
    }

    return ret;
}

int CPQControl::Cpq_SetDNLPStatus(const char *val)
{
    int ret = -1;
    int fd = open(DNLP_ENABLE, O_RDWR);

    if (fd < 0) {
        SYS_LOGE("open %s error!\n",DNLP_ENABLE);
    } else {
        ret = write(fd, val, strlen(val));
        if (ret <= 0) {
           SYS_LOGE("write %s error!\n",DNLP_ENABLE);
        }

        close(fd);
    }

    return ret;
}

int CPQControl::SetEyeProtectionMode(tv_source_input_t source_input, int enable, int is_save)
{
    int pre_mode = -1;
    int ret = -1;
    if (mSSMAction->SSMReadEyeProtectionMode(&pre_mode) < 0 || pre_mode == -1 || pre_mode == enable)
        return ret;

    mSSMAction->SSMSaveEyeProtectionMode(enable);

    int temp_mode = GetColorTemperature();
    ret = SetColorTemperature(temp_mode, 0);

    if (ret < 0 ) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
    }

    return ret;
}

int CPQControl::GetEyeProtectionMode(tv_source_input_t source_input)
{
    int mode = -1;

    if (mSSMAction->SSMReadEyeProtectionMode(&mode) < 0) {
        SYS_LOGE("%s failed!\n",__FUNCTION__);
        return -1;
    } else {
        SYS_LOGD("%s success!\n",__FUNCTION__);
        return mode;
    }
}

int CPQControl::SetFlagByCfg(Set_Flag_Cmd_t id, int value)
{
    bool bvalue = false;

    if (value != 0) {
        bvalue = true;
    }

    switch (id) {
    case PQ_DEPEND_BACKLIGHT: {
        mbVppCfg_pqmode_depend_bklight = bvalue;
        break;
    }
    case BACKLIGHT_REVERSE: {
        mbVppCfg_backlight_reverse = bvalue;
        break;
    }
    case BACKLIGHT_INIT: {
        mbVppCfg_backlight_init = bvalue;
        break;
    }
    case PQ_WITHOUT_HUE: {
        mbCpqCfg_pqmode_without_hue = bvalue;
        break;
    }
    case HUE_REVERSE: {
        mbCpqCfg_hue_reverse = bvalue;
        break;
    }
    case GAMMA_ONOFF: {
        mbCpqCfg_gamma_onoff = bvalue;
        break;
    }
    case NEW_CM: {
        mbCpqCfg_new_cm = bvalue;
        break;
    }
    case NEW_NR: {
        mbCpqCfg_new_nr = bvalue;
        break;
    }
    case COLORTEMP_BY_SOURCE: {
        mbCpqCfg_colortemp_by_source = bvalue;
        break;
    }
    case XVYCC_SWITCH_CONTROL: {
        mbCpqCfg_xvycc_switch_control = bvalue;
        break;
    }
    case CONTRAST_WITHOSD: {
        mbCpqCfg_contrast_withOSD = bvalue;
        break;
    }
    case HUE_WITHOSD: {
        mbCpqCfg_hue_withOSD = bvalue;
        break;
    }
    case BRIGHTNESS_WITHOSD: {
        mbCpqCfg_brightness_withOSD = bvalue;
        break;
    }
    case RESET_ALL: {
        mbVppCfg_pqmode_depend_bklight = false;
        mbVppCfg_backlight_reverse = false;
        mbVppCfg_backlight_init = false;
        mbCpqCfg_pqmode_without_hue = true;
        mbCpqCfg_hue_reverse = true;
        mbCpqCfg_gamma_onoff = false;
        mbCpqCfg_new_cm = true;
        mbCpqCfg_new_nr = true;
        mbCpqCfg_colortemp_by_source = true;
        mbCpqCfg_xvycc_switch_control = false;
        mbCpqCfg_contrast_withOSD = false;
        mbCpqCfg_hue_withOSD = false;
        mbCpqCfg_brightness_withOSD = false;
        break;
    }
    }

    return 0;
}

int CPQControl::SetPLLValues(source_input_param_t source_input_param)
{
    am_regs_t regs;
    int ret = 0;
    if (mPQdb->PQ_GetPLLParams (source_input_param.source_port, source_input_param.sig_fmt, &regs ) == 0 ) {
        ret = AFE_DeviceIOCtl(TVIN_IOC_LOAD_REG, &regs);
        if ( ret < 0 ) {
            SYS_LOGE ( "%s, SetPLLValues failed!\n", __FUNCTION__ );
            return -1;
        }
    } else {
        SYS_LOGE ( "%s, PQ_GetPLLParams failed!\n", __FUNCTION__ );
        return -1;
    }

    return 0;
}

int CPQControl::SetCVD2Values(source_input_param_t source_input_param)
{
    am_regs_t regs;
    int ret = 0;

    if (mPQdb->PQ_GetCVD2Params ( source_input_param.source_port, source_input_param.sig_fmt , &regs) == 0) {
        ret = AFE_DeviceIOCtl ( TVIN_IOC_LOAD_REG, &regs );
        if ( ret < 0 ) {
            SYS_LOGE ( "SetCVD2Values, error(%s)!\n", strerror ( errno ) );
            return -1;
        }
    } else {
        SYS_LOGE ( "%s, PQ_GetCVD2Params failed!\n", __FUNCTION__);
        return -1;
    }

    return 0;

}

int CPQControl::AFE_DeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    int afe_dev_fd = -1;
    va_list ap;
    void *arg;

    afe_dev_fd = open ( AFE_DEV_PATH, O_RDWR );

    if ( afe_dev_fd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( afe_dev_fd, request, arg );

        close(afe_dev_fd);
        return tmp_ret;
    } else {
        SYS_LOGE ( "Open tvafe module, error(%s).\n", strerror ( errno ) );
        return -1;
    }
}

int CPQControl::Cpq_SSMReadNTypes(int id, int data_len, int offset)
{
    int value = 0;
    int ret = 0;

    ret = mSSMAction->SSMReadNTypes(id, data_len, &value, offset);

    if (ret < 0) {
        SYS_LOGE("Cpq_SSMReadNTypes, error(%s).\n", strerror ( errno ) );
        return -1;
    } else {
        return value;
    }
}

int CPQControl::Cpq_SSMWriteNTypes(int id, int data_len, int data_buf, int offset)
{
    int ret = 0;
    ret = mSSMAction->SSMWriteNTypes(id, data_len, &data_buf, offset);

    if (ret < 0) {
        SYS_LOGE("Cpq_SSMWriteNTypes, error(%s).\n", strerror ( errno ) );
    }

    return ret;
}

int CPQControl::Cpq_GetSSMActualAddr(int id)
{
    return mSSMAction->GetSSMActualAddr(id);
}

int CPQControl::Cpq_GetSSMActualSize(int id)
{
    return mSSMAction->GetSSMActualSize(id);
}

int CPQControl::Cpq_SSMRecovery(void)
{
    return mSSMAction->SSMRecovery();
}

int CPQControl::Cpq_GetSSMStatus()
{
    return mSSMAction->GetSSMStatus();
}

int CPQControl::SetCurrentSourceInputInfo(source_input_param_t source_input_param)
{
    mCurentSourceInputInfo.source_input = source_input_param.source_input;
    mCurentSourceInputInfo.source_type = source_input_param.source_type;
    mCurentSourceInputInfo.source_port = source_input_param.source_port;
    mCurentSourceInputInfo.sig_fmt = source_input_param.sig_fmt;
    mCurentSourceInputInfo.is3d = source_input_param.is3d;
    mCurentSourceInputInfo.trans_fmt = source_input_param.trans_fmt;

    return 0;
}

source_input_param_t CPQControl::GetCurrentSourceInputInfo()
{
    return mCurentSourceInputInfo;
}

void CPQControl::InitSourceInputInfo()
{
    mCurentSourceInputInfo.source_input = SOURCE_MPEG;
    mCurentSourceInputInfo.source_type = SOURCE_TYPE_MPEG;
    mCurentSourceInputInfo.source_port = TVIN_PORT_MPEG0;
    mCurentSourceInputInfo.sig_fmt = TVIN_SIG_FMT_HDMI_1920X1080P_60HZ;
    mCurentSourceInputInfo.is3d = INDEX_2D;
    mCurentSourceInputInfo.trans_fmt = TVIN_TFMT_2D;
}

int CPQControl::autoSwitchMode() {
    return mAutoSwitchPCModeFlag;
}

int CPQControl::pqWriteSys(const char *path, const char *val)
{
    int fd;
    if ((fd = open(path, O_RDWR)) < 0) {
        SYS_LOGE("writeSys, open %s error(%s)", path, strerror (errno));
        return -1;
    }

    int len = write(fd, val, strlen(val));
    close(fd);
    return len;
}
