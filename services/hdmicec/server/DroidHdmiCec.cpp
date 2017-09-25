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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2017/9/25
 *  @par function description:
 *  - 1 droidlogic hdmi cec hwbinder service
 */

#define LOG_TAG "hdmicecd"

#include <inttypes.h>
#include <string>
#include "../binder/HdmiCecBase.h"
#include "DroidHdmiCec.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace hdmicec {
namespace V1_0 {
namespace implementation {


void DroidHdmiCec::instantiate()
{
    DroidHdmiCec *pcec = new DroidHdmiCec();
}

DroidHdmiCec::DroidHdmiCec()
{
    mHdmiCecControl = new HdmiCecControl();
    mHdmiCecControl->setEventObserver(this);
}

DroidHdmiCec::~DroidHdmiCec()
{
    delete mHdmiCecControl;
}

Return<void> DroidHdmiCec::openCecDevice(openCecDevice_cb _hidl_cb)
{
    int32_t fd = -1;
    if (NULL != mHdmiCecControl) {
        fd = mHdmiCecControl->openCecDevice();
    }

    _hidl_cb(Result::SUCCESS, fd);
    return Void();
}

Return<void> DroidHdmiCec::closeCecDevice()
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->closeCecDevice();
    }
    return Void();
}

Return<int32_t> DroidHdmiCec::getCecVersion()
{
    int version = 0;
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->getVersion(&version);
    }
    return version;
}

Return<uint32_t> DroidHdmiCec::getVendorId()
{
    uint32_t vendorId = 0;
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->getVendorId(&vendorId);
    }
    return vendorId;
}

Return<void> DroidHdmiCec::getPhysicalAddress(getPhysicalAddress_cb _hidl_cb)
{
    uint16_t addr = 0;
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->getPhysicalAddress(&addr);
    }

    _hidl_cb(Result::SUCCESS, addr);
    return Void();
}

Return<SendMessageResult> DroidHdmiCec::sendMessage(const CecMessage& message, bool isExtend)
{
    if (message.body.size() > CEC_MESSAGE_BODY_MAX_LENGTH) {
        ALOGE("sendMessage body size > %d", CEC_MESSAGE_BODY_MAX_LENGTH);
        return static_cast<SendMessageResult>(HDMI_RESULT_FAIL);
    }

    if (NULL != mHdmiCecControl) {
        //change message from hwbinder data structure to needed data structure
        cec_message_t msg;
        msg.initiator = static_cast<cec_logical_address_t>(message.initiator);
        msg.destination = static_cast<cec_logical_address_t>(message.destination);

        for (size_t i = 0; i < message.body.size(); ++i) {
            msg.body[i] = message.body[i];
        }
        msg.length = message.body.size();

        return static_cast<SendMessageResult>(mHdmiCecControl->sendMessage(&msg, isExtend));
    }
    return static_cast<SendMessageResult>(HDMI_RESULT_FAIL);
}

Return<void> DroidHdmiCec::getPortInfo(getPortInfo_cb _hidl_cb)
{
    hdmi_port_info_t *legacyPorts;
    int numPorts;
    hidl_vec<HdmiPortInfo> portInfos;
    if (NULL != mHdmiCecControl) {

        mHdmiCecControl->getPortInfos(&legacyPorts, &numPorts);
        portInfos.resize(numPorts);
        for (int i = 0; i < numPorts; ++i) {
            portInfos[i] = {
                .type = static_cast<HdmiPortType>(legacyPorts[i].type),
                .portId = static_cast<uint32_t>(legacyPorts[i].port_id),
                .cecSupported = legacyPorts[i].cec_supported != 0,
                .arcSupported = legacyPorts[i].arc_supported != 0,
                .physicalAddress = legacyPorts[i].physical_address
            };
            /*
            ALOGD("droidhdmicec port %d, type:%s, id:%d, cec support:%d, arc support:%d, physical address:%x",
                i, (HDMI_OUTPUT==static_cast<hdmi_port_type_t>(portInfos[i].type)) ? "output" : "input",
                portInfos[i].portId,
                portInfos[i].cecSupported?1:0,
                portInfos[i].arcSupported?1:0,
                portInfos[i].physicalAddress);*/
        }

        _hidl_cb(portInfos);
    }
    return Void();

}

Return<Result> DroidHdmiCec::addLogicalAddress(CecLogicalAddress addr)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->addLogicalAddress(static_cast<cec_logical_address_t>(addr));
        return Result::SUCCESS;
    }
    return Result::FAILURE_UNKNOWN;
}

Return<void> DroidHdmiCec::clearLogicalAddress()
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->clearLogicaladdress();
    }
    return Void();
}

Return<void> DroidHdmiCec::setOption(OptionKey key, bool value)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setOption((int)key, value);
    }
    return Void();
}

Return<void> DroidHdmiCec::enableAudioReturnChannel(int32_t portId, bool enable)
{
    if (NULL != mHdmiCecControl) {
        mHdmiCecControl->setAudioReturnChannel(portId, enable);
    }
    return Void();
}

Return<bool> DroidHdmiCec::isConnected(int32_t portId)
{
    if (NULL != mHdmiCecControl) {
        return mHdmiCecControl->isConnected(portId);
    }
    return false;
}

Return<void> DroidHdmiCec::setCallback(const sp<IDroidHdmiCecCallback>& callback)
{
    if (callback != NULL) {
        mClients.push_back(callback);
    }
    return Void();
}

void DroidHdmiCec::onEventUpdate(const hdmi_cec_event_t* event)
{
    //change native data structure to hwbinder data structure
    CecEvent hidlEvent;
    hidlEvent.eventType = event->eventType;
    if (HDMI_EVENT_ADD_LOGICAL_ADDRESS == event->eventType) {
        hidlEvent.logicalAddress = event->logicalAddress;
    }
    else if (HDMI_EVENT_HOT_PLUG == event->eventType) {
        hidlEvent.hotplug.connected = (0 == event->hotplug.connected)?false:true;
        hidlEvent.hotplug.portId = event->hotplug.port_id;
    }
    //HDMI_EVENT_CEC_MESSAGE or HDMI_EVENT_RECEIVE_MESSAGE
    else if (0 != event->eventType) {
        hidlEvent.cec.initiator = static_cast<CecLogicalAddress>(event->cec.initiator);
        hidlEvent.cec.destination = static_cast<CecLogicalAddress>(event->cec.destination);
        for (size_t i = 0; i < event->cec.length; i++) {
            hidlEvent.cec.body[i] = event->cec.body[i];
        }
    }

    int clientSize = mClients.size();
    ALOGV("%s, %s, client size:%d", __FUNCTION__, getEventType(event->eventType), clientSize);

    for (int i = 0; i < clientSize; i++) {
        mClients[i]->notifyCallback(hidlEvent);
    }
}

const char* DroidHdmiCec::getEventType(int eventType)
{
    switch (eventType) {
        case HDMI_EVENT_CEC_MESSAGE:
            return "cec message";
        case HDMI_EVENT_HOT_PLUG:
            return "hotplug message";
        case HDMI_EVENT_ADD_LOGICAL_ADDRESS:
            return "add logical address for extend";
        case HDMI_EVENT_RECEIVE_MESSAGE:
            return "cec message for extend";
        case (HDMI_EVENT_CEC_MESSAGE | HDMI_EVENT_RECEIVE_MESSAGE):
            return "cec message for system and extend";
        default:
            return "unknown message";
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace hdmicec
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
