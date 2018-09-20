/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef CPQDB_H_
#define CPQDB_H_

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CSqlite.h"
#include "PQType.h"
#include "CPQLog.h"

using namespace android;

#ifdef SYSTEMCONTROL_DEBUG_PQ_ENABLE
#define DEBUG_FLAG 1
#else
#define DEBUG_FLAG 0
#endif

#define PROP_DEBUG_PQ "systemcontrol.debug.pq.enable"

#define getSqlParams(func, buffer, args...) \
    do{\
        sprintf(buffer, ##args);\
        if (DEBUG_FLAG) {\
            char value[PROPERTY_VALUE_MAX];\
            memset(value, '\0', PROPERTY_VALUE_MAX);\
            property_get(PROP_DEBUG_PQ, value, "0");\
            if(!strcmp(value, "1") || !strcmp(value, "true")){\
                SYS_LOGD("getSqlParams for %s\n", func);\
                SYS_LOGD("%s = %s\n",#buffer, buffer);\
            }\
        }\
    }while(0)

class CPQdb: public CSqlite {
public:
    CPQdb();
    ~CPQdb();
    int PQ_GetBaseColorParams(vpp_color_basemode_t basemode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int PQ_GetCM2Params(vpp_color_management2_t basemode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int PQ_GetNR2Params(vpp_noise_reduction2_mode_t basemode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int PQ_GetXVYCCParams(vpp_xvycc_mode_t xvycc_mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs, am_regs_t *regs_1);
    int PQ_GetDIParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt __unused,  am_regs_t *regs);
    int PQ_GetDemoSquitoParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt __unused,  am_regs_t *regs);
    int PQ_GetMCDIParams(vpp_mcdi_mode_t mcdi_mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int PQ_GetDeblockParams(vpp_deblock_mode_t mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int getDIRegValuesByValue(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs);
    int PQ_GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt,
                                     tcon_rgb_ogo_t *params);
    int PQ_SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt,
                                     tcon_rgb_ogo_t params);
    int PQ_ResetAllColorTemperatureParams(void);
    int PQ_SetNoLineAllBrightnessParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllBrightnessParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetBrightnessParams(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int *params);
    int PQ_SetBrightnessParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int params);
    int PQ_SetNoLineAllContrastParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllContrastParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetContrastParams(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int *params);
    int PQ_SetContrastParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int params);
    int PQ_SetNoLineAllSaturationParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllSaturationParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetSaturationParams(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int *params);
    int PQ_SetSaturationParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int params);
    int PQ_SetNoLineAllHueParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllHueParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetHueParams(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int *params);
    int PQ_SetHueParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int params);
    int PQ_SetNoLineAllSharpnessParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllSharpnessParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetSharpnessParams(tv_source_input_type_t source_type, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, am_regs_t *regs, am_regs_t *regs_l);
    int PQ_SetSharpnessParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, am_regs_t regs);
    int PQ_SetNoLineAllVolumeParams(tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_GetNoLineAllVolumeParams(tv_source_input_type_t source_type, int *osd0, int *osd25, int *osd50, int *osd75, int *osd100);
    int PQ_GetVolumeParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int *length, int *params);
    int PQ_SetVolumeParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int level, int params);
    int PQ_ResetAllNoLineParams(void);
    int PQ_GetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int *params);
    int PQ_SetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int *params);
    int PQ_GetDNLPParams(tvin_port_t source_port, tvin_sig_fmt_t fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, Dynamic_contrst_status_t mode, ve_dnlp_curve_param_t *newParams);
    int PQ_GetOverscanParams(tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, vpp_display_mode_t dmode, tvin_cutwin_t *cutwin_t);
    int PQ_SetOverscanParams(tv_source_input_type_t source_type, tvin_sig_fmt_t fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, tvin_cutwin_t cutwin_t);
    int PQ_ResetAllOverscanParams(void);
    bool PQ_GetPqVersion(String8& ProjectVersion, String8& GenerateTime);
    int PQ_GetPQModeParams(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_SetPQModeParams(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_ResetAllPQModeParams(void);
    int PQ_GetGammaTableR(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt, tcon_gamma_table_t *gamma_r);
    int PQ_GetGammaTableG(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt, tcon_gamma_table_t *gamma_g);
    int PQ_GetGammaTableB(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt, tcon_gamma_table_t *gamma_b);
    int PQ_GetGammaSpecialTable(vpp_gamma_curve_t gamma_curve, const char *f_name, tcon_gamma_table_t *gamma_r);
    int PQ_GetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t *adjparam);
    int PQ_SetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t adjparam);
    int PQ_GetPhaseArray(am_phase_t *am_phase);
    int PQ_GetPLLParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt,  am_regs_t *regs);
    int PQ_GetCVD2Params(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt,  am_regs_t *regs);
    int openPqDB(const char *db_path);
    int reopenDB(const char *db_path);
    int getRegValues(const char *table_name, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, am_regs_t *regs);
    int getRegValuesByValue(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs);
    int getRegValuesByValue_long(const char *name, const char *f_name, const char *f2_name, const int val, const int val2, am_regs_t *regs, am_regs_t *regs_1);
    int LoadAllPQData(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int flag);
    void initialTable(int type);
    int getSharpnessFlag();
    bool PQ_GetLDIM_Regs(vpu_ldim_param_s *vpu_ldim_param);
    bool isHdrTableExist(const String8&);
    bool IsMatchHDRCondition(const tvin_port_t source_port);
    bool loadHdrStatus(const tvin_port_t, const String8&);
    int GetFileAttrIntValue(const char *fp, int flag);

private:
    int CaculateLevelParam(tvpq_data_t *pq_data, int nodes, int level);
    am_regs_t CaculateLevelRegsParam(tvpq_sharpness_regs_t *pq_regs, int level, int flag);
    int GetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_type_t source_type, int level, int *params);
    int GetNonlinearMappingByOSDFac(tvpq_data_type_t data_type, tv_source_input_type_t source_type, int *params);
    int SetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int LoadPQData(tvpq_data_type_t data_type, tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int flag);
    int loadSharpnessData(char *sqlmaster, char *table_name);
    int PQ_GetGammaTable(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt, const char *f_name, tcon_gamma_table_t *val);
    int SetNonlinearMappingByName(const char *name, tvpq_data_type_t data_type, tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100);
    int PQ_SetPQModeParamsByName(const char *name, tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);

    tvpq_data_t pq_bri_data[15];
    tvpq_data_t pq_con_data[15];
    tvpq_data_t pq_sat_data[15];
    tvpq_data_t pq_hue_data[15];
    tvpq_sharpness_regs_t pq_sharpness_reg_data[10];
    tvpq_sharpness_regs_t pq_sharpness_reg_data_1[10];
    int bri_nodes;
    int con_nodes;
    int hue_nodes;
    int sat_nodes;
    int sha_nodes;
    int sha_nodes_1;
    int sha_diff_flag;
};
#endif
