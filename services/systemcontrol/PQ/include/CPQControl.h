/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */


#ifndef _C_CPQCONTROL_H
#define _C_CPQCONTROL_H

#include "SSMAction.h"
#include "CDevicePollCheckThread.h"
#include "CPQdb.h"
#include "PQType.h"
#include "CPQColorData.h"
#include "CPQLog.h"

#define PQ_DB_PATH                "/vendor/etc/tvconfig/pq.db"
#define LDIM_PATH                 "/dev/aml_ldim"
#define VPP_DEV_PATH              "/dev/amvecm"
#define DI_DEV_PATH               "/dev/di0"
#define AFE_DEV_PATH              "/dev/tvafe0"
#define DNLP_ENABLE               "/sys/module/am_vecm/parameters/dnlp_en"
#define SYS_VIDEO_FRAME_HEIGHT    "/sys/class/video/frame_height"
#define HDMIRX_DEV_PATH           "/dev/hdmirx0"

#define CROP_PATH                 "/sys/class/video/crop"
#define SCREEN_MODE_PATH          "/sys/class/video/screen_mode"
#define NOLINER_FACTORY           "/sys/class/video/nonlinear_factor"
#define BACKLIGHT_PATH            "/sys/class/backlight/aml-bl/brightness"
#define PQ_MOUDLE_ENABLE_PATH     "/sys/class/amvecm/pc_mode"

#define HDMI_IOC_MAGIC 'H'
#define HDMI_IOC_PC_MODE_ON         _IO(HDMI_IOC_MAGIC, 0x04)
#define HDMI_IOC_PC_MODE_OFF        _IO(HDMI_IOC_MAGIC, 0x05)

#define TVIN_IOC_MAGIC 'T'
#define TVIN_IOC_LOAD_REG           _IOW(TVIN_IOC_MAGIC, 0x20, struct am_regs_s)

// screem mode index value
#define  SCREEN_MODE_NORMAL           0
#define  SCREEN_MODE_FULL_STRETCH     1
#define  SCREEN_MODE_4_3              2
#define  SCREEN_MODE_16_9             3
#define  SCREEN_MODE_NONLINEAR        4
#define  SCREEN_MODE_NORMAL_NOSCALEUP 5
#define  SCREEN_MODE_CROP_FULL        6
#define  SCREEN_MODE_CROP             7

class CPQControl: public CDevicePollCheckThread::IDevicePollCheckObserver{
public:
    CPQControl();
    ~CPQControl();
    static CPQControl *GetInstance();

    virtual void onVframeSizeChange();
    virtual void onHDRStatusChange();
    int SetPQMoudleStatus(int status);
    int LoadPQSettings(source_input_param_t source_input_param);
    int LoadCpqLdimRegs(void);
    int Cpq_LoadRegs(am_regs_t regs);
    int Cpq_LoadDisplayModeRegs(ve_pq_load_t regs);
    int DI_LoadRegs(am_pq_param_t di_regs );
    int Cpq_LoadBasicRegs(source_input_param_t source_input_param);
    int Cpq_SetDIMode(di_mode_param_t di_param, source_input_param_t source_input_param);
    int ResetLastPQSettingsSourceType(void);
    int BacklightInit(void);
    //PQ mode
    int SetPQMode(int pq_mode, int is_save, int is_autoswitch);
    int GetPQMode(void);
    int SavePQMode(int pq_mode);
    int Cpq_SetPcModeStatus(int value);
    int Cpq_SetPQMode(vpp_picture_mode_t pq_mode, source_input_param_t source_input_param);
    int SetPQParams(vpp_pq_para_t pq_para, source_input_param_t source_input_param);
    int GetPQParams(source_input_param_t source_input_param, vpp_picture_mode_t pq_mode, vpp_pq_para_t *pq_para);
    //color Temperature
    int SetColorTemperature(int temp_mode, int is_save);
    int GetColorTemperature(void);
    int SaveColorTemperature(int temp_mode);
    int Cpq_SetColorTemperatureWithoutSave(vpp_color_temperature_mode_t Tempmode, tv_source_input_t tv_source_input __unused);
    int Cpq_CheckColorTemperatureParamAlldata(source_input_param_t source_input_param);
    unsigned short Cpq_CalColorTemperatureParamsChecksum(void);
    int Cpq_SetColorTemperatureParamsChecksum(void);
    unsigned short Cpq_GetColorTemperatureParamsChecksum(void);
    int Cpq_SetColorTemperatureUser(vpp_color_temperature_mode_t temp_mode __unused, tv_source_input_type_t source_type);
    int Cpq_RestoreColorTemperatureParamsFromDB(source_input_param_t source_input_param);
    int Cpq_CheckTemperatureDataLable(void);
    int Cpq_SetTemperatureDataLable(void);
    int SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
    int GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t *params);
    int SaveColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tcon_rgb_ogo_t params);
    int Cpq_CheckColorTemperatureParams(void);
    //Brightness
    int SetBrightness(int value, int is_save);
    int GetBrightness(void);
    int SaveBrightness(int value);
    int Cpq_SetBrightness(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoBrightness(int value);
    //Contrast
    int SetContrast(int value, int is_save);
    int GetContrast(void);
    int SaveContrast(int value);
    int Cpq_SetContrast(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoContrast(int value);
    //Saturation
    int SetSaturation(int value, int is_save);
    int GetSaturation(void);
    int SaveSaturation(int value);
    int Cpq_SetSaturation(int value, source_input_param_t source_input_param);
    //Hue
    int SetHue(int value, int is_save);
    int GetHue(void);
    int SaveHue(int value);
    int Cpq_SetHue(int value, source_input_param_t source_input_param);
    int Cpq_SetVideoSaturationHue(int satVal, int hueVal);
    void video_set_saturation_hue(signed char saturation, signed char hue, signed long *mab);
    void video_get_saturation_hue(signed char *sat, signed char *hue, signed long *mab);
    //Sharpness
    int SetSharpness(int value, int is_enable, int is_save);
    int GetSharpness(void);
    int SaveSharpness(int value);
    int Cpq_SetSharpness(int value, source_input_param_t source_input_param);
    //NoiseReductionMode
    int SetNoiseReductionMode(int nr_mode, int is_save);
    int GetNoiseReductionMode(void);
    int SaveNoiseReductionMode(int nr_mode);
    int Cpq_SetNoiseReductionMode(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param);
    //GammaValue
    int SetGammaValue(vpp_gamma_curve_t gamma_curve, int is_save);
    int GetGammaValue();
    //Displaymode
    int SetDisplayMode(tv_source_input_t source_input, vpp_display_mode_t display_mode, int is_save);
    int GetDisplayMode(tv_source_input_t source_input);
    int SaveDisplayMode(tv_source_input_t source_input, vpp_display_mode_t mode);
    int Cpq_SetDisplayMode(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetDisplayModeForDTV(tv_source_input_t source_input, vpp_display_mode_t display_mode);
    int Cpq_SetVideoScreenMode(int value);
    int Cpq_GetScreenModeValue(vpp_display_mode_t display_mode);
    int Cpq_SetVideoCrop(int Voffset0, int Hoffset0, int Voffset1, int Hoffset1);
    int Cpq_SetNonLinearFactor(int value);
    //Backlight
    int SetBacklight(tv_source_input_t source_input, int value, int is_save);
    int GetBacklight(tv_source_input_t source_input);
    int SaveBacklight(tv_source_input_t source_input, int value);
    int Cpq_SetBackLightLevel(int value);
    //Factory
    int FactoryResetPQMode(void);
    int FactoryResetColorTemp(void);

    int FactorySetPQParam(source_input_param_t source_input_param, int pq_mode, int id, int value);
    int FactoryGetPQParam(source_input_param_t source_input_param, int pq_mode, int id);
    int FactorySetColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value);
    int FactoryGetColorTemperatureParam(int colortemperature_mode, pq_color_param_t id);
    int FactorySaveColorTemperatureParam(int colortemperature_mode, pq_color_param_t id, int value);

    int FactorySetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode, int brightness );
    int FactoryGetPQMode_Brightness(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode, int contrast );
    int FactoryGetPQMode_Contrast(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode, int saturation );
    int FactoryGetPQMode_Saturation(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode, int sharpness );
    int FactoryGetPQMode_Sharpness(source_input_param_t source_input_param, int pq_mode );
    int FactorySetPQMode_Hue(source_input_param_t source_input_param, int pq_mode, int hue );
    int FactoryGetPQMode_Hue(source_input_param_t source_input_param, int pq_mode );

    int FactoryGetTestPattern(void );
    int FactoryResetNonlinear(void);
    int FactorySetParamsDefault(void);
    int FactorySetNolineParams(source_input_param_t source_input_param, int type, noline_params_t noline_params);
    int FactoryGetNolineParams(source_input_param_t source_input_param,          int type, noline_params_ID_t id);
    int FactorySetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t);
    int FactoryGetOverscanParam(source_input_param_t source_input_param, tvin_cutwin_param_t id);
    int FactorySetGamma(int gamma_r_value, int gamma_g_value, int gamma_b_value);
    int FcatorySSMRestore(void);
    int SetColorDemoMode(vpp_color_demomode_t demomode);
    int Cpq_SetCMRegisterMap(struct cm_regmap_s *pRegMap);
    int SetBaseColorMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param);
    vpp_color_basemode_t GetBaseColorMode(void);
    int SaveBaseColorMode(vpp_color_basemode_t basemode);
    int SetBaseColorModeWithoutSave(vpp_color_basemode_t basemode, source_input_param_t source_input_param);
    int Cpq_SetBaseColorMode(vpp_color_basemode_t basemode, source_input_param_t source_input_param);
    int Cpq_SetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo);
    int Cpq_GetRGBOGO(const struct tcon_rgb_ogo_s *rgbogo);
    int Cpq_LoadGamma(vpp_gamma_curve_t gamma_curve);
    int Cpq_SetGammaTbl_R(unsigned short red[256]);
    int Cpq_SetGammaTbl_G(unsigned short green[256]);
    int Cpq_SetGammaTbl_B(unsigned short blue[256]);
    int Cpq_SetGammaOnOff(unsigned char onoff);
    int SetDNLP(source_input_param_t source_input_param);
    int Cpq_SetVENewDNLP(const ve_dnlp_curve_param_t *pDNLP);
    int Cpq_SetDNLPStatus(const char *val);
    int SetEyeProtectionMode(tv_source_input_t source_input, int enable, int is_save);
    int GetEyeProtectionMode(tv_source_input_t source_input);
    int Cpq_SSMReadNTypes(int id, int data_len, int offset);
    int Cpq_SSMWriteNTypes(int id, int data_len, int data_buf, int offset);
    int Cpq_GetSSMActualAddr(int id);
    int Cpq_GetSSMActualSize(int id);
    int Cpq_SSMRecovery(void);
    int Cpq_GetSSMStatus();
    int SetFlagByCfg(Set_Flag_Cmd_t id, int value);
    int SetPLLValues(source_input_param_t source_input_param);
    int SetCVD2Values(source_input_param_t source_input_param);
    int SetCurrentSourceInputInfo(source_input_param_t source_input_param);
    source_input_param_t GetCurrentSourceInputInfo();
    int resetLastSourceTypeSettings(void);
    int autoSwitchMode();
private:
    int VPPOpenModule(void);
    int VPPCloseModule(void );
    int VPPDeviceIOCtl(int request, ...);
    int DIOpenModule(void);
    int DICloseModule(void);
    int DIDeviceIOCtl(int request, ...);

    tvin_sig_fmt_t getVideoResolutionToFmt();
    int Cpq_SetXVYCCMode(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param);
    int AFE_DeviceIOCtl ( int request, ... );
    void InitSourceInputInfo();
    int pqWriteSys(const char *path, const char *val);

    tv_source_input_type_t cpq_setting_last_source_type;
    tvin_sig_fmt_t cpq_setting_last_sig_fmt;
    tvin_trans_fmt_t cpq_setting_last_trans_fmt;
    bool mIsHdrLastTime;
    bool mInitialized;
    int mAutoSwitchPCModeFlag;
    //cfg
    bool mbVppCfg_backlight_reverse = false;
    bool mbVppCfg_backlight_init = false;
    bool mbVppCfg_pqmode_depend_bklight = false;
    bool mbCpqCfg_pqmode_without_hue = true;
    bool mbCpqCfg_hue_reverse = true;
    bool mbCpqCfg_gamma_onoff = false;
    bool mbCpqCfg_new_cm = true;
    bool mbCpqCfg_new_nr = true;
    bool mbCpqCfg_colortemp_by_source = true;
    bool mbCpqCfg_xvycc_switch_control = false;
    //osd config
    bool mbCpqCfg_contrast_withOSD = false;
    bool mbCpqCfg_hue_withOSD = false;
    bool mbCpqCfg_brightness_withOSD = false;

    CPQdb *mPQdb;
    SSMAction *mSSMAction;
    static CPQControl *mInstance;
    CDevicePollCheckThread mCDevicePollCheckThread;

    int mAmvideoFd;
    int mDiFd;
    tcon_rgb_ogo_t rgbfrompq[3];
    source_input_param_t mCurentSourceInputInfo;
};
#endif
