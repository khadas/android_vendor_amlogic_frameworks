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
 *  - 1 dtvkit rpc hwbinder service
 */

#ifndef MIRACASTSERVER_V1_0_H
#define MIRACASTSERVER_V1_0_H

#include <binder/IBinder.h>
#include <utils/Mutex.h>
#include <vector>
#include <map>
#include <vendor/amlogic/hardware/miracastserver/1.0/IMiracastServer.h>
//#include <vendor/amlogic/hardware/miracastserver/1.0/types.h>
#include "MiracastService.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace miracastserver {
namespace V1_0 {
namespace implementation {

using ::vendor::amlogic::hardware::miracastserver::V1_0::IMiracastServer;
using ::vendor::amlogic::hardware::miracastserver::V1_0::IMiracastServerCallback;
using ::vendor::amlogic::hardware::miracastserver::V1_0::ConnectType;
using ::vendor::amlogic::hardware::miracastserver::V1_0::Result;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using namespace android;

class MiracastServer : public IMiracastServer, public ServiceNotify {
public:
    MiracastServer();
    virtual ~MiracastServer();

    Return<int32_t> startWatching(const hidl_string& path, int32_t mask) override;
    Return<void> stopWatching() override;
    Return<void> setCallback(const sp<IMiracastServerCallback>& callback, ConnectType type) override;
    virtual void onEvent(const std::vector<std::string> &list);

private:
    const char* getEventTypeStr(int eventType);
    const char* getConnectTypeStr(ConnectType type);

    // Handle the case where the callback registered for the given type dies
    void handleServiceDeath(uint32_t type);

    bool mDebug = false;

    MiracastService *mMiracast = NULL;

    std::map<uint32_t, sp<IMiracastServerCallback>> mClients;

    mutable Mutex mLock;

    class DeathRecipient : public android::hardware::hidl_death_recipient  {
    public:
        DeathRecipient(sp<MiracastServer> Mcs);

        // hidl_death_recipient interface
        virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) override;

    private:
        sp<MiracastServer> mMiracastServer;
    };

    sp<DeathRecipient> mDeathRecipient;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace miracastserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
#endif /* MIRACASTSERVER_V1_0_H */
