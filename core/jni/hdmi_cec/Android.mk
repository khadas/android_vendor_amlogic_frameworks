LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

HDMI_CEC_NATIVE_PATH := $(TOP)/vendor/amlogic/frameworks/services/hdmicec

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    HdmiCecExtend.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
    $(HDMI_CEC_NATIVE_PATH)/binder

LOCAL_SHARED_LIBRARIES := \
    libandroid_runtime \
    libcutils \
    libutils \
    libnativehelper \
    libhdmicec

LOCAL_MODULE:= libhdmicec_jni


include $(BUILD_SHARED_LIBRARY)