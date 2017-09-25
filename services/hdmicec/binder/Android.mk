LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    IHdmiCecService.cpp \
    IHdmiCecCallback.cpp \
    HdmiCecClient.cpp \
    HdmiCecHidlClient.cpp \
    HdmiCecBase.cpp

LOCAL_C_INCLUDES += \
   external/libcxx/include

LOCAL_CPPFLAGS += -std=c++14

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.hdmicec@1.0_vendor \
    libbase \
    libhidlbase \
    libhidltransport \
    libcutils \
    libutils \
    libbinder \
    liblog


LOCAL_MODULE:= libhdmicec

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
