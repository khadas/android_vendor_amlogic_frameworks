LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    main_hdmicec.cpp \
    HdmiCecService.cpp \
    DroidHdmiCec.cpp

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/../libhdmi_cec \
   $(LOCAL_PATH)/../binder \
   external/libcxx/include \
   system/libhidl/transport/include/hidl

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.hdmicec@1.0_vendor \
    libbase \
    libhidlbase \
    libhidltransport \
    libcutils \
    libutils \
    libbinder \
    libhdmicec \
    liblog

LOCAL_STATIC_LIBRARIES := \
    libhdmi_cec_static

LOCAL_CPPFLAGS += -std=c++14
LOCAL_INIT_RC := hdmicecd.rc

LOCAL_MODULE:= hdmicecd


ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
