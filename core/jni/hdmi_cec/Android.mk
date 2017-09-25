LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_LDLIBS := -lm -llog

HDMI_CEC_NATIVE_PATH := $(TOP)/vendor/amlogic/frameworks/services/hdmicec

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    HdmiCecExtend.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
    system/core/base/include \
    $(HDMI_CEC_NATIVE_PATH)/binder

LOCAL_SHARED_LIBRARIES := \
    vendor.amlogic.hardware.hdmicec@1.0_vendor \
    libandroid_runtime \
    libcutils \
    libutils \
    libnativehelper \
    libhdmicec

LOCAL_MODULE:= libhdmicec_jni

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
