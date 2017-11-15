#ifndef _HDMI_CEC_CONTROL_CPP_
#define _HDMI_CEC_CONTROL_CPP_

#include <pthread.h>
#include <HdmiCecBase.h>
#include <utils/StrongPointer.h>

#define CEC_FILE        "/dev/cec"
#define MAX_PORT        32
#define MESSAGE_SET_MENU_LANGUAGE 0x32

#define CEC_IOC_MAGIC                   'C'
#define CEC_IOC_GET_PHYSICAL_ADDR       _IOR(CEC_IOC_MAGIC, 0x00, uint16_t)
#define CEC_IOC_GET_VERSION             _IOR(CEC_IOC_MAGIC, 0x01, int)
#define CEC_IOC_GET_VENDOR_ID           _IOR(CEC_IOC_MAGIC, 0x02, uint32_t)
#define CEC_IOC_GET_PORT_INFO           _IOR(CEC_IOC_MAGIC, 0x03, int)
#define CEC_IOC_GET_PORT_NUM            _IOR(CEC_IOC_MAGIC, 0x04, int)
#define CEC_IOC_GET_SEND_FAIL_REASON    _IOR(CEC_IOC_MAGIC, 0x05, uint32_t)
#define CEC_IOC_SET_OPTION_WAKEUP       _IOW(CEC_IOC_MAGIC, 0x06, uint32_t)
#define CEC_IOC_SET_OPTION_ENALBE_CEC   _IOW(CEC_IOC_MAGIC, 0x07, uint32_t)
#define CEC_IOC_SET_OPTION_SYS_CTRL     _IOW(CEC_IOC_MAGIC, 0x08, uint32_t)
#define CEC_IOC_SET_OPTION_SET_LANG     _IOW(CEC_IOC_MAGIC, 0x09, uint32_t)
#define CEC_IOC_GET_CONNECT_STATUS      _IOR(CEC_IOC_MAGIC, 0x0A, uint32_t)
#define CEC_IOC_ADD_LOGICAL_ADDR        _IOW(CEC_IOC_MAGIC, 0x0B, uint32_t)
#define CEC_IOC_CLR_LOGICAL_ADDR        _IOW(CEC_IOC_MAGIC, 0x0C, uint32_t)
#define CEC_IOC_SET_DEV_TYPE            _IOW(CEC_IOC_MAGIC, 0x0D, uint32_t)
#define CEC_IOC_SET_ARC_ENABLE          _IOW(CEC_IOC_MAGIC, 0x0E, uint32_t)
#define CEC_IOC_SET_AUTO_DEVICE_OFF     _IOW(CEC_IOC_MAGIC, 0x0F, uint32_t)

#define DEV_TYPE_TV                     0
#define DEV_TYPE_RECORDER               1
#define DEV_TYPE_RESERVED               2
#define DEV_TYPE_TUNER                  3
#define DEV_TYPE_PLAYBACK               4
#define DEV_TYPE_AUDIO_SYSTEM           5
#define DEV_TYPE_PURE_CEC_SWITCH        6
#define DEV_TYPE_VIDEO_PROCESSOR        7

namespace android {

//must sync with hardware\libhardware\include\hardware\Hdmi_cec.h
enum {
    /* Option 4 not used */
    HDMI_OPTION_CEC_AUTO_DEVICE_OFF = 4,
};

/*
 * HDMI CEC messages para value
 */
enum cec_message_para_value{
    CEC_KEYCODE_POWER = 0x40,
    CEC_KEYCODE_ROOT_MENU = 0x09,
    CEC_KEYCODE_POWER_ON_FUNCTION = 0x6D
};

/**
 * struct for information of cec device.
 * @mDeviceType      Indentify type of cec device, such as TV or BOX.
 * @mAddrBitmap      Bit maps for each valid logical address
 * @mFd              File descriptor for global read/write
 * @isCecEnabled     Flag of HDMI_OPTION_ENABLE_CEC,
 * can't do anything when value is false.
 * @isCecControlled  Flag of HDMI_OPTION_SYSTEM_CEC_CONTROL,
 * android system will stop handling CEC service when vaule is false.
 * @mTotalPort       Total ports of HDMI
 * @mpPortData       Array of HDMI ports
 * @mThreadId        pthread for polling cec rx message
 * @Run              Flag for rx polling thread
 * @mExited          Whether the thread is exited
 * @mExtendControl   Flag for extend cec device
 */
typedef struct hdmi_device {
    int                         mDeviceType;
    int                         mAddrBitmap;
    int                         mFd;
    bool                        isCecEnabled;
    bool                        isCecControlled;
    unsigned int                mConnectStatus;
    int                         mTotalPort;
    hdmi_port_info_t            *mpPortData;
    pthread_t                   mThreadId;
    bool                        mRun;
    bool                        mExited;
    int                         mExtendControl;
} hdmi_device_t;

class HdmiCecControl : public HdmiCecBase {
public:
    HdmiCecControl();
    ~HdmiCecControl();

    virtual int openCecDevice();
    virtual int closeCecDevice();

    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend);

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total);
    virtual int addLogicalAddress(cec_logical_address_t address);
    virtual void clearLogicaladdress();
    virtual void setOption(int flag, int value);
    virtual void setAudioReturnChannel(int port, bool flag);
    virtual bool isConnected(int port);

    void setEventObserver(const sp<HdmiCecEventListener> &eventListener);
private:
    void init();

    void getBootConnectStatus();
    static void *__threadLoop(void *data);
    void threadLoop();

    int sendExtMessage(const cec_message_t* message);
    int send(const cec_message_t* message);
    int readMessage(unsigned char *buf, int msgCount);
    void checkConnectStatus();

    bool assertHdmiCecDevice();
    bool hasHandledByExtend(const cec_message_t* message);
    bool isWakeUpMsg(char *msgBuf, int len, int deviceType);
    hdmi_device_t mCecDevice;
    sp<HdmiCecEventListener> mEventListener;
};


}; //namespace android

#endif /* _HDMI_CEC_CONTROL_CPP_ */
