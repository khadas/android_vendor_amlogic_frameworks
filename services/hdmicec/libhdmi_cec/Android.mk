LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    HdmiCecControl.cpp

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/../binder

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils

LOCAL_MODULE:= libhdmi_cec_static


include $(BUILD_STATIC_LIBRARY)