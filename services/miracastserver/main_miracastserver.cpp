#include "MiracastServer.h"
#include <stdio.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <HidlTransportSupport.h>
#include <cutils/properties.h>

using namespace android;
using ::android::hardware::configureRpcThreadpool;
using ::vendor::amlogic::hardware::miracastserver::V1_0::implementation::MiracastServer;
using ::vendor::amlogic::hardware::miracastserver::V1_0::IMiracastServer;

int main(void)
{
    android::ProcessState::initWithDriver("/dev/vndbinder");

    configureRpcThreadpool(4, false);
    sp<ProcessState> proc(ProcessState::self());

    sp<IMiracastServer>hidlMiracastServer = new MiracastServer();
    if (hidlMiracastServer == nullptr) {
        ALOGE("Cannot create IMiracastServer service");
    } else if (hidlMiracastServer->registerAsService() != OK) {
        ALOGE("Cannot register IMiracastServer service.");
    } else {
        ALOGI("Treble IMiracastServer service created.");
    }

    IPCThreadState::self()->joinThreadPool();
    return 0;
}

