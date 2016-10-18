LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES:= \
    IHdmiCecService.cpp \
    IHdmiCecCallback.cpp \
    HdmiCecClient.cpp \
    HdmiCecBase.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder


LOCAL_MODULE:= libhdmicec


include $(BUILD_SHARED_LIBRARY)