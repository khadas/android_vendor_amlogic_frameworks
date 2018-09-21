/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CDynamicBackLight"

#include "CDynamicBackLight.h"

CDynamicBackLight *CDynamicBackLight::mInstance;
CDynamicBackLight* CDynamicBackLight::getInstance()
{
    if (NULL == mInstance) {
        mInstance = new CDynamicBackLight();
    }

    return mInstance;
}

CDynamicBackLight::CDynamicBackLight()
{
    mPreBacklightValue = 255;
    mGD_mvreflsh = 9;
    mRunStatus = THREAD_STOPED;
    mArithmeticPauseTime = -1;
}

CDynamicBackLight::~CDynamicBackLight()
{
    mRunStatus = THREAD_STOPED;
}

int CDynamicBackLight::startDected(void)
{
    if ((mRunStatus == THREAD_RUNING) || (mRunStatus == THREAD_PAUSED)) {
        SYS_LOGD ("Already start...");
        return 0;
    }

    if (run("CDynamicBackLight") < 0) {
        mRunStatus = THREAD_STOPED;
        SYS_LOGE ("StartDected failure!!!!!!");
        return -1;
    } else {
        mRunStatus = THREAD_RUNING;
        SYS_LOGD ("StartDected success.");
    }

    return 0;
}

void CDynamicBackLight::stopDected(void)
{
    mRunStatus = THREAD_STOPED;
    requestExit();
}

void CDynamicBackLight::pauseDected(int pauseTime)
{
    mArithmeticPauseTime = pauseTime;
    mRunStatus = THREAD_PAUSED;
}


void CDynamicBackLight::setOsdStatus(int osd_status)
{
    SYS_LOGD ("%s: status = %d", __FUNCTION__, osd_status);
    //tvWriteSysfs(SYSFS_HIST_SEL, osd_status);
}

void CDynamicBackLight::gd_fw_alg_frm(int value, int *tf_bl_value, int *LUT, int GD_ThTF, int ColorRangeMode)
{
    int nT0 = 0, nT1 = 0;
    int nL0 = 0, nR0 = 0;
    int nDt = 0;
    int bl_value = 0;
    int bld_lvl = 0, bl_diff = 0;//luma_dif = 0;
    int RBASE = 0;
    int apl_lut[10] = {0, 16, 35, 58, 69, 80, 91, 102, 235, 255};
    int step = 0;
    int k = 0;
    int average = 0;
    int GD_STEP_Th = 5;
    int GD_IIR_MODE = 0;//1-old iir;0-new iir,set constant step
    RBASE = (1 << mGD_mvreflsh);
    if (ColorRangeMode == 1) {//color renge limit
        if (value < 16) {
            value = 16;
        } else if (value > 236) {
            value = 236;
        }

        average = (value - 16)*256/(236-16);
    } else {
        if (value < 0) {//color renge full
            value = 0;
        } else if (value > 255) {
            value = 255;
        }

        average = value;
    }
    if (!GD_LUT_MODE) {//old or xiaomi project
        nT0 = average/16;
        nT1 = average%16;

        nL0 = LUT[nT0];
        nR0 = LUT[nT0+1];

        nDt = nL0*(16-nT1)+nR0*nT1+8;
        bl_value = nDt/16;
    } else {//new mode, only first ten elements used
        for (k = 0; k < 9; k++ ) {
            if (average <= apl_lut[k+1] && average >= apl_lut[k]) {
                nT0 = k;
                step= apl_lut[k+1] - apl_lut[k];
                break;
            }
        }
        nT1 = average - apl_lut[nT0];
        nL0 = LUT[nT0];
        nR0 = LUT[nT0+1];
        nDt = nL0*(step-nT1)+nR0*nT1+step/2;
        bl_value = nDt/step;//make sure that step != 0
    }

    if (GD_IIR_MODE) {
        bl_diff = (mPreBacklightValue > bl_value) ? (mPreBacklightValue - bl_value) : (bl_value - mPreBacklightValue);
        bld_lvl = (RBASE > (GD_ThTF + bl_diff)) ? (GD_ThTF + bl_diff) : RBASE;
        *tf_bl_value = ((RBASE - bld_lvl) * mPreBacklightValue + bld_lvl * bl_value + (RBASE >> 1)) >> mGD_mvreflsh;     //slowchange
    } else {
        step = bl_value - mPreBacklightValue;
        if (step > GD_STEP_Th  )// dark --> bright, limit increase step
            step = GD_STEP_Th;
        else if (step < (-GD_STEP_Th)) // bright --> dark, limit decrease step
            step = -GD_STEP_Th;

        *tf_bl_value = mPreBacklightValue + step;
    }
    mPreBacklightValue = *tf_bl_value;
}

int CDynamicBackLight::backLightScale(int backLight, int UIval)
{
    //SYS_LOGD ("backLightScale:backLight =  %d, UIvalue = %d\n", backLight, UIval);
    int ret = 255;
    if (backLight <= 0)
        backLight = 1;
    if (backLight >= 255)
        backLight = 255;

    ret = backLight * UIval / 100;
    if (ret <= 0) {
        ret = 1;
    } else if (ret >= 255) {
        ret = 255;
    }

    return ret;
}

bool CDynamicBackLight::threadLoop()
{
    dynamic_backlight_Param_t DynamicBacklightParam;
    memset(&DynamicBacklightParam, 0, sizeof(dynamic_backlight_Param_t));
    int backLight = 0, NewBacklightValue = 0;
    int LUT_high[17] = {25,170,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};
    int LUT_low[17] = {102,217,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255};

    while ( !exitPending() ) {
        if (mArithmeticPauseTime != -1) {
            usleep(mArithmeticPauseTime);
            SYS_LOGD ("Pasuse %d usecs", mArithmeticPauseTime);
            mArithmeticPauseTime = -1;
        }

        if (mpObserver != NULL) {
            mpObserver->GetDynamicBacklighParam(&DynamicBacklightParam);
            //SYS_LOGD("VideoStatus=%d, ave=%d, mode = %d\n", DynamicBacklightParam.VideoStatus, DynamicBacklightParam.hist.ave, DynamicBacklightParam.CurDynamicBacklightMode);
            /*if ((DynamicBacklightParam.hist.ave == 0)
                || (DynamicBacklightParam.VideoStatus == 0)) {
               DynamicBacklightParam.CurDynamicBacklightMode = DYNAMIC_BACKLIGHT_OFF;
            }*/

            if (DYNAMIC_BACKLIGHT_HIGH == DynamicBacklightParam.CurDynamicBacklightMode) {
                gd_fw_alg_frm(DynamicBacklightParam.hist.ave, &backLight, LUT_high, 8, 0);
            } else if (DYNAMIC_BACKLIGHT_LOW == DynamicBacklightParam.CurDynamicBacklightMode) {
                gd_fw_alg_frm(DynamicBacklightParam.hist.ave, &backLight, LUT_low, 8, 0);
            } else {
                backLight = 255;
            }

            NewBacklightValue = backLightScale(backLight, DynamicBacklightParam.UiBackLightValue);
            if ((DynamicBacklightParam.CurBacklightValue != NewBacklightValue) &&
                (DynamicBacklightParam.hist.ave != -1)) {
                mpObserver->Set_Backlight(NewBacklightValue);
            }
        }

        usleep( 30 * 1000);
    }
    return false;
}


