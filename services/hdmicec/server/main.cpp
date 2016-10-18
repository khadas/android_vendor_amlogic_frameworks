#define LOG_NDEBUG 0
#define LOG_TAG "HdmiCecService"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include "HdmiCecService.h"

using namespace android;

int main(int argc __unused, char** argv __unused)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
    HdmiCecService::instantiate();
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}

