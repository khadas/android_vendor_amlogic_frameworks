LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	systemcontroltest.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils  \
	libbinder \
	libsystemcontrolservice

LOCAL_MODULE:= test-systemcontrol

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
