/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   junchao.yuan
 *  @version  1.0
 *  @date     2018/12/12
 *  @par function description:
 *  - 1 dtvkit rpc service
 */

#define LOG_TAG "miracastserver"

#include <inttypes.h>
#include <string>
#include <cutils/properties.h>
#include <binder/IPCThreadState.h>
#include "MiracastServer.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace miracastserver {
namespace V1_0 {
namespace implementation {


MiracastServer::MiracastServer() : mDeathRecipient(new DeathRecipient(this)) {
    mMiracast = new MiracastService();
    mMiracast->setNotifyCallback(this);
    ALOGI("create MiracastServer\n");
    mDebug = true;
}

MiracastServer::~MiracastServer() {
    delete mMiracast;
}

Return<int32_t> MiracastServer::startWatching(const hidl_string& path, int32_t mask) {
    return mMiracast->startWatching(path, mask);
}

Return<void> MiracastServer::stopWatching() {
    mMiracast->stopWatching();
    return Void();
}


void MiracastServer::onEvent(const std::vector<std::string> &list) {
    AutoMutex _l(mLock);
    int clientSize = mClients.size();
    ALOGI("%s,clientSize = %d", __FUNCTION__, clientSize);
    hidl_vec<hidl_string> hidllist;
    hidllist.resize(list.size());
    for (size_t i = 0; i < list.size(); i++) {
        hidllist[i] = list[i];
    }
    for (int i = 0; i < clientSize; i++) {
        if (mClients[i] != nullptr) {
            ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
            mClients[i]->notifyCallback(hidllist);
        }
    }

}

Return<void> MiracastServer::setCallback(const sp<IMiracastServerCallback>& callback, ConnectType type) {
    AutoMutex _l(mLock);
    if ((int)type > (int)ConnectType::TYPE_TOTAL - 1) {
        ALOGE("%s don't support type:%d", __FUNCTION__, (int)type);
        return Void();
    }

    if (callback != nullptr) {
        int cookie = -1;
        int clientSize = mClients.size();
        for (int i = 0; i < clientSize; i++) {
            if (mClients[i] == nullptr) {
                ALOGI("%s, client index:%d had died, this id give the new client", __FUNCTION__, i);
                cookie = i;
                mClients[i] = callback;
                break;
            }
        }

        if (cookie < 0) {
            cookie = clientSize;
            mClients[clientSize] = callback;
        }

        Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, (int)type);
        bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
        if (!linkSuccess) {
            ALOGW("Couldn't link death recipient for type: %s", getConnectTypeStr(type));
        }

        if (mDebug)
            ALOGI("%s this type:%s, client size:%d", __FUNCTION__, getConnectTypeStr(type), (int)mClients.size());
    }
    return Void();
}

const char* MiracastServer::getConnectTypeStr(ConnectType type) {
    switch (type) {
        case ConnectType::TYPE_HAL:
            return "HAL";
        case ConnectType::TYPE_EXTEND:
            return "EXTEND";
        default:
            return "unknown type";
    }
}

void MiracastServer::handleServiceDeath(uint32_t type) {
    AutoMutex _l(mLock);
    ALOGI("MiracastServer daemon client:%s died", getConnectTypeStr((ConnectType)type));
    mClients[type]->unlinkToDeath(mDeathRecipient);
    mClients[type].clear();
}

MiracastServer::DeathRecipient::DeathRecipient(sp<MiracastServer> Mcs)
        : mMiracastServer(Mcs) {}

void MiracastServer::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("droid MiracastServer daemon a client died cookie:%d", (int)cookie);

    uint32_t type = static_cast<uint32_t>(cookie);
    mMiracastServer->handleServiceDeath(type);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace dtvkitserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
