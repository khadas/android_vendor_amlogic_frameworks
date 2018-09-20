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
 *  @version  1.0
 *  @date     2016/09/06
 *  @par function description:
 *  - 1 process HDCP RX key combie and refresh
 */

#ifndef _HDCP_RX_KEY_H
#define _HDCP_RX_KEY_H

#include <assert.h>
#include "common.h"
#include "../SysWrite.h"
enum {
    HDCP_RX_14_KEY                  = 0,
    HDCP_RX_22_KEY                  = 1
};

class HDCPRxKey {
public:
    HDCPRxKey(int keyType);

    ~HDCPRxKey();

    void init();
    /**
     * refresh the 1.4 key or 2.2 key firmware
     */
    bool refresh();
    int getHdcpRX14key(char *value, int size);
    int setHdcpRX14key(const char *value, const int size);
    int getHdcpRX22key(char *value, int size);
    int setHdcpRX22key(const char *value, const int size);

private:
    int mKeyType;

    bool setHDCP14Key();
    bool setHDCP22Key();

    bool aicTool();
    bool esmSwap();
    bool genKeyImg();
    bool combineFirmware();
    SysWrite sysWrite;
};

#endif // _HDCP_RX_KEY_H
