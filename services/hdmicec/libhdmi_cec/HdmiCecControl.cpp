/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#define LOG_NDEBUG 0
#define LOG_CEE_TAG "HdmiCecControl"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "HdmiCecControl.h"


namespace android {

HdmiCecControl::HdmiCecControl() {
    LOGD("HdmiCecControl");
    init();
}

HdmiCecControl::~HdmiCecControl(){}

/**
 * initialize some cec flags before opening cec deivce.
 */
void HdmiCecControl::init()
{
    if (mCecDevice != NULL)
        return;

    mCecDevice = new hdmi_device();

    char value[PROPERTY_VALUE_MAX];
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("ro.hdmi.device_type", value, "4");
    int type = atoi(value);
    mCecDevice->mDeviceType =
            (type >= DEV_TYPE_TV && type <= DEV_TYPE_VIDEO_PROCESSOR) ? type : DEV_TYPE_PLAYBACK;

    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("persist.sys.hdmi.keep_awake", value, "true");
    mCecDevice->mExtendControl = (!strcmp(value, "false")) ? 1 : 0;
    LOGD("device_type: %d, ext_control: %d", mCecDevice->mDeviceType, mCecDevice->mExtendControl);
}

/**
 * close cec device, reset some cec flags, and recycle some resources.
 */
int HdmiCecControl::closeCecDevice()
{
    if (mCecDevice == NULL)
        return -EINVAL;

    mCecDevice->mRun = false;
    while (!mCecDevice->mExited) {
        usleep(100 * 1000);
    }

    close(mCecDevice->mFd);
    delete mCecDevice->mpPortData;
    delete mCecDevice;
    mCecDevice = NULL;

    LOGD("%s, cec has closed.", __FUNCTION__);
    return 0;
}

/**
 * initialize all cec flags when open cec devices, get the {@code fd} of cec devices,
 * and create a thread for cec working.
 * {@Return}  fd of cec device.
 */
int HdmiCecControl::openCecDevice()
{
    if (mCecDevice == NULL) {
        LOGE("mCecDevice is NULL, initialize it.");
        init();
    }

    mCecDevice->mRun = true;
    mCecDevice->mExited = false;
    mCecDevice->mThreadId = 0;
    mCecDevice->mTotalPort = 0;
    mCecDevice->mConnectStatus = 0;
    mCecDevice->mpPortData = NULL;
    mCecDevice->mAddrBitmap = (1 << CEC_ADDR_BROADCAST);
    mCecDevice->isCecEnabled = true;
    mCecDevice->isCecControlled = false;
    mCecDevice->mFd = open(CEC_FILE, O_RDWR);
    if (mCecDevice->mFd < 0) {
        ALOGE("can't open device. fd < 0");
        return -EINVAL;
    }
    int ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_DEV_TYPE, mCecDevice->mDeviceType);
    getBootConnectStatus();
    pthread_create(&mCecDevice->mThreadId, NULL, __threadLoop, this);
    pthread_setname_np(mCecDevice->mThreadId, "hdmi_cec_loop");
    return mCecDevice->mFd;
}

/**
 * get connect status when boot
 */
void HdmiCecControl::getBootConnectStatus()
{
    unsigned int total, i;
    int port, ret;

    ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_PORT_NUM, &total);
    LOGD("total port:%d, ret:%d", total, ret);
    if (ret < 0)
        return ;

    if (total > MAX_PORT)
        total = MAX_PORT;
    mCecDevice->mConnectStatus = 0;
    for (i = 0; i < total; i++) {
        port = i + 1;
        ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_CONNECT_STATUS, &port);
        if (ret) {
            LOGD("get port %d connected status failed, ret:%d", i, ret);
            continue;
        }
        mCecDevice->mConnectStatus |= ((port ? 1 : 0) << i);
    }
    LOGD("mConnectStatus: %d", mCecDevice->mConnectStatus);
}

void* HdmiCecControl::__threadLoop(void *user)
{
    HdmiCecControl *const self = static_cast<HdmiCecControl *>(user);
    self->threadLoop();
    return 0;
}

void HdmiCecControl::threadLoop()
{
    unsigned char msg_buf[CEC_MESSAGE_BODY_MAX_LENGTH];
    hdmi_cec_event_t event;
    int r = -1;

    LOGD("threadLoop start.");
    while (mCecDevice->mFd < 0) {
        usleep(1000 * 1000);
        mCecDevice->mFd = open(CEC_FILE, O_RDWR);
    }
    LOGD("file open ok, fd = %d.", mCecDevice->mFd);

    while (mCecDevice != NULL && mCecDevice->mRun) {
        if (!mCecDevice->isCecEnabled)
            continue;
        checkConnectStatus();

        memset(msg_buf, 0, sizeof(msg_buf));
        //try to get a message from dev.
        r = readMessage(msg_buf, CEC_MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1)//ignore received ping messages
            continue;

        printCecMsgBuf((const char*)msg_buf, r);

        event.eventType = 0;
        memcpy(event.cec.body, msg_buf + 1, r - 1);
        event.cec.initiator = cec_logical_address_t((msg_buf[0] >> 4) & 0xf);
        event.cec.destination = cec_logical_address_t((msg_buf[0] >> 0) & 0xf);
        event.cec.length = r - 1;

        if (mCecDevice->mDeviceType == DEV_TYPE_PLAYBACK
                && msg_buf[1] == CEC_MESSAGE_SET_MENU_LANGUAGE && mCecDevice->mExtendControl) {
            LOGD("ignore menu language change for hdmi-tx.");
        } else {
            if (mCecDevice->isCecControlled) {
                event.eventType |= HDMI_EVENT_CEC_MESSAGE;
            }
        }

        LOGD("mExtendControl = %d, mDeviceType = %d, isCecControlled = %d",
                mCecDevice->mExtendControl, mCecDevice->mDeviceType, mCecDevice->isCecControlled);
        /* call java method to process cec message for ext control */
        if ((mCecDevice->mExtendControl == 0x03)
                && (mCecDevice->mDeviceType == DEV_TYPE_PLAYBACK)
                && mCecDevice->isCecControlled) {
            event.eventType |= HDMI_EVENT_RECEIVE_MESSAGE;
        }
        if (mEventListener != NULL && event.eventType != 0) {
            mEventListener->onEventUpdate(&event);
        }
    }
    LOGE("thread end.");
    mCecDevice->mExited = true;
}

void HdmiCecControl::checkConnectStatus()
{
    unsigned int prev_status, bit;
    int i, port, ret;
    hdmi_cec_event_t event;

    prev_status = mCecDevice->mConnectStatus;
    for (i = 0; i < mCecDevice->mTotalPort && mCecDevice->mpPortData != NULL; i++) {
        port = mCecDevice->mpPortData[i].port_id;
        ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_CONNECT_STATUS, &port);
        if (ret) {
            LOGE("get port %d connected status failed, ret:%d\n", mCecDevice->mpPortData[i].port_id, ret);
            continue;
        }
        bit = prev_status & (1 << i);
        if (bit ^ ((port ? 1 : 0) << i)) {//connect status has changed
            LOGD("port:%d, connect status changed, now:%d, prev_status:%x\n",
                    mCecDevice->mpPortData[i].port_id, port, prev_status);
            if (mEventListener != NULL && mCecDevice->isCecEnabled && mCecDevice->isCecControlled) {
                event.eventType = HDMI_EVENT_HOT_PLUG;
                event.hotplug.connected = port;
                event.hotplug.port_id = mCecDevice->mpPortData[i].port_id;
                mEventListener->onEventUpdate(&event);
            }
            prev_status &= ~(bit);
            prev_status |= ((port ? 1 : 0) << i);
            LOGD("now mask:%x\n", prev_status);
        }
    }
    mCecDevice->mConnectStatus = prev_status;
}

int HdmiCecControl::readMessage(unsigned char *buf, int msg_cnt)
{
    if (msg_cnt <= 0 || !buf) {
        return 0;
    }

    int ret = -1;
    /* maybe blocked at driver */
    ret = read(mCecDevice->mFd, buf, msg_cnt);
    if (ret < 0) {
        LOGE("read :%s failed, ret:%d\n", CEC_FILE, ret);
        return -1;
    }

    return ret;
}

void HdmiCecControl::getPortInfos(hdmi_port_info_t* list[], int* total)
{
    if (assertHdmiCecDevice())
        return;

    ioctl(mCecDevice->mFd, CEC_IOC_GET_PORT_NUM, total);

    LOGD("total port:%d", *total);
    if (*total > MAX_PORT)
        *total = MAX_PORT;
    mCecDevice->mpPortData = new hdmi_port_info[*total];
    if (!mCecDevice->mpPortData) {
        LOGE("alloc port_data failed");
        *total = 0;
        return;
    }

    ioctl(mCecDevice->mFd, CEC_IOC_GET_PORT_INFO, mCecDevice->mpPortData);

    for (int i = 0; i < *total; i++) {
        LOGD("port %d, type:%s, id:%d, cec support:%d, arc support:%d, physical address:%x",
                i, mCecDevice->mpPortData[i].type ? "output" : "input",
                mCecDevice->mpPortData[i].port_id,
                mCecDevice->mpPortData[i].cec_supported,
                mCecDevice->mpPortData[i].arc_supported,
                mCecDevice->mpPortData[i].physical_address);
    }

    *list = mCecDevice->mpPortData;
    mCecDevice->mTotalPort = *total;
}

int HdmiCecControl::addLogicalAddress(cec_logical_address_t address)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    if (address < CEC_ADDR_BROADCAST)
        mCecDevice->mAddrBitmap |= (1 << address);

    if (mCecDevice->mDeviceType == DEV_TYPE_PLAYBACK && mCecDevice->mExtendControl) {
        mCecDevice->mExtendControl |= (0x02);
        if (mEventListener != NULL) {
            hdmi_cec_event_t event;
            event.eventType = HDMI_EVENT_ADD_LOGICAL_ADDRESS;
            event.logicalAddress = address;
            mEventListener->onEventUpdate(&event);
        }
    }
    LOGD("addr:%x, bitmap:%x\n", address, mCecDevice->mAddrBitmap);
    return ioctl(mCecDevice->mFd, CEC_IOC_ADD_LOGICAL_ADDR, address);
}

void HdmiCecControl::clearLogicaladdress()
{
    if (assertHdmiCecDevice())
        return;

    mCecDevice->mAddrBitmap = (1 << CEC_ADDR_BROADCAST);
    LOGD("bitmap:%x", mCecDevice->mAddrBitmap);
    if (mCecDevice->mExtendControl) {
        mCecDevice->mExtendControl &= ~(0x02);
    }

    ioctl(mCecDevice->mFd, CEC_IOC_CLR_LOGICAL_ADDR, 0);
}

void HdmiCecControl::setOption(int flag, int value)
{
    if (assertHdmiCecDevice())
        return;

    int ret = -1;
    switch (flag) {
        case HDMI_OPTION_ENABLE_CEC:
            ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_OPTION_ENALBE_CEC, value);
            mCecDevice->isCecEnabled = (value == 1) ? true : false;
            break;

        case HDMI_OPTION_WAKEUP:
            ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_OPTION_WAKEUP, value);
            break;

        case HDMI_OPTION_SYSTEM_CEC_CONTROL:
            ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_OPTION_SYS_CTRL, value);
            mCecDevice->isCecControlled = (value == 1) ? true : false;
            break;

        /* set device auto-power off by uboot */
        case HDMI_OPTION_CEC_AUTO_DEVICE_OFF:
            ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_AUTO_DEVICE_OFF, value);
            break;

        case HDMI_OPTION_SET_LANG:
            ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_OPTION_SET_LANG, value);
            break;

        default:
            break;
    }
    LOGD("%s, flag:0x%x, value:0x%x, ret:%d, isCecControlled:%x", __FUNCTION__, flag, value, ret, mCecDevice->isCecControlled);
}

void HdmiCecControl::setAudioReturnChannel(int port, bool flag)
{
    if (assertHdmiCecDevice())
        return;

    int ret = ioctl(mCecDevice->mFd, CEC_IOC_SET_ARC_ENABLE, flag);
    LOGD("%s, port id:%d, flag:%x, ret:%d\n", __FUNCTION__, port, flag, ret);
}

bool HdmiCecControl::isConnected(int port)
{
    if (assertHdmiCecDevice())
        return false;

    int status = -1, ret;
    /* use status pass port id */
    status = port;
    ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_CONNECT_STATUS, &status);
    if (ret)
        return false;
    LOGD("%s, port:%d, connected:%s", __FUNCTION__, port, status ? "yes" : "no");
    return status;
}

int HdmiCecControl::getVersion(int* version)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_VERSION, version);
    LOGD("%s, version:%x, ret = %d", __FUNCTION__, *version, ret);
    return ret;
}

int HdmiCecControl::getVendorId(uint32_t* vendorId)
{
    if (assertHdmiCecDevice())
        return -1;

    int ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_VENDOR_ID, vendorId);
    LOGD("%s, vendorId: %x, ret = %d", __FUNCTION__, *vendorId, ret);
    return ret;
}

int HdmiCecControl::getPhysicalAddress(uint16_t* addr)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    int ret = ioctl(mCecDevice->mFd, CEC_IOC_GET_PHYSICAL_ADDR, addr);
    LOGD("%s, physical addr: %x, ret = %d", __FUNCTION__, *addr, ret);
    return ret;
}

int HdmiCecControl::sendMessage(const cec_message_t* message, bool isExtend)
{
    if (assertHdmiCecDevice())
        return -EINVAL;

    if (isExtend) {
        LOGD("isExtend = %d, mExtendControl = %d", isExtend, mCecDevice->mExtendControl);
        return sendExtMessage(message);
    }
    /* don't send message if controlled by extend */
    if (mCecDevice->mExtendControl == 0x03 && hasHandledByExtend(message)) {
        return HDMI_RESULT_SUCCESS;
    }

    return send(message);
}

bool HdmiCecControl::hasHandledByExtend(const cec_message_t* message)
{
    bool ret = false;
    int opcode = message->body[0] & 0xff;
    switch (opcode) {
        case CEC_MESSAGE_SET_MENU_LANGUAGE:
            ret = true;
            break;
        default:
            break;
    }
    return ret;
}

int HdmiCecControl::sendExtMessage(const cec_message_t* message)
{
    int i, addr;
    cec_message_t msg;
    if (mCecDevice->mDeviceType == DEV_TYPE_PLAYBACK) {
        addr = mCecDevice->mAddrBitmap;
        addr &= 0x7fff;
        for (i = 0; i < 15; i++) {
            if (addr & 1) {
                break;
            }
            addr >>= 1;
        }
        msg.initiator = (cec_logical_address_t) i;
    } else {
        msg.initiator = CEC_ADDR_TV; /* root for TV */
    }
    if (msg.initiator == CEC_ADDR_UNREGISTERED)//source can not be 0xf
        return -1;
    msg.destination = message->destination;
    msg.length = message->length;
    memcpy(msg.body, message->body, msg.length);
    return send(&msg);
}

int HdmiCecControl::send(const cec_message_t* message)
{
    unsigned char msg_buf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int ret = -1;

    memset(msg_buf, 0, sizeof(msg_buf));
    msg_buf[0] = ((message->initiator & 0xf) << 4) | (message->destination & 0xf);
    memcpy(msg_buf + 1, message->body, message->length);
    ret = write(mCecDevice->mFd, msg_buf, message->length + 1);
    printCecMessage(message, ret);
    return ret;
}

bool HdmiCecControl::assertHdmiCecDevice()
{
    return !mCecDevice || mCecDevice->mFd < 0;
}

void HdmiCecControl::setEventObserver(const sp<HdmiCecEventListener> &eventListener)
{
    mEventListener = eventListener;
}



};//namespace android
