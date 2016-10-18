LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    main.cpp \
    HdmiCecService.cpp

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/../libhdmi_cec \
   $(LOCAL_PATH)/../binder

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libhdmicec

LOCAL_STATIC_LIBRARIES := \
    libhdmi_cec_static

LOCAL_MODULE:= hdmi_cec


include $(BUILD_EXECUTABLE)