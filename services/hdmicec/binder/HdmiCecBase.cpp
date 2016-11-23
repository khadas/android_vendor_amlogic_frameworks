#define LOG_NDEBUG 0
#define LOG_TAG "HdmiCecBase"

#include "HdmiCecBase.h"

namespace android {

void HdmiCecBase::printCecMsgBuf(const char *msg_buf)
{
    if (!DEBUG)
        return;

    char buf[64] = { };
    int i, r = strlen(msg_buf), size = 0;
    memset(buf, 0, sizeof(buf));
    for (i = 0; i < r; i++) {
        size += sprintf(buf + size, " %02x", msg_buf[i]);
    }
    LOGD("msg:%s", buf);
}

void HdmiCecBase::printCecEvent(const hdmi_cec_event_t *event)
{
    if (!DEBUG)
        return;

    if (((event->eventType & HDMI_EVENT_CEC_MESSAGE) != 0)
            || ((event->eventType & HDMI_EVENT_RECEIVE_MESSAGE) != 0)) {
        LOGD("eventType: %d", event->eventType);
        printCecMessage(&event->cec);
    } else if ((event->eventType & HDMI_EVENT_HOT_PLUG) != 0) {
        LOGD("hotplug, connected:%d, port_id:%d", event->hotplug.connected, event->hotplug.port_id);
    } else if ((event->eventType & HDMI_EVENT_ADD_PHYSICAL_ADDRESS) != 0) {
        LOGD("add physical address, physicalAdd:%x", event->physicalAdd);
    }
}

void HdmiCecBase::printCecMessage(const cec_message_t* message)
{
    if (!DEBUG)
        return;

    char buf[64];
    int i, size = 0;
    memset(buf, 0, sizeof(buf));
    for (i = 0; i < message->length; i++) {
        size += sprintf(buf + size, " %02x", message->body[i]);
    }
    LOGD("[%x -> %x] len: %d, body:%s", message->initiator, message->destination, message->length,
            buf);
}

void HdmiCecBase::printCecMessage(const cec_message_t* message, int result)
{
    if (!DEBUG)
        return;

    char buf[64];
    int i, size = 0;
    memset(buf, 0, sizeof(buf));
    for (i = 0; i < message->length; i++) {
        size += sprintf(buf + size, " %02x", message->body[i]);
    }
    LOGD("[%x -> %x] len: %d, body:%s, result: %s",
            message->initiator, message->destination, message->length, buf, getResult(result));
}

const char *HdmiCecBase::getResult(int result)
{
    switch (result) {
        case HDMI_RESULT_SUCCESS:
            return "success";
        case HDMI_RESULT_NACK:
            return "no ack";
        case HDMI_RESULT_BUSY:
            return "busy";
        case HDMI_RESULT_FAIL:
            return "fail other";
        default:
            return "unknown fail code";
    }
}

const char* HdmiCecBase::getEventType(int eventType)
{
    switch (eventType) {
        case HDMI_EVENT_CEC_MESSAGE:
            return "cec message";
        case HDMI_EVENT_HOT_PLUG:
            return "hotplug message";
        case HDMI_EVENT_ADD_PHYSICAL_ADDRESS:
            return "add physical address for extend";
        case HDMI_EVENT_RECEIVE_MESSAGE:
            return "cec message for extend";
        case (HDMI_EVENT_CEC_MESSAGE | HDMI_EVENT_RECEIVE_MESSAGE):
            return "cec message for system and extend";
        default:
            return "unknown message";
    }
}


};//namespace android
