
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_LIBRARIES := droidlogic droidlogic.external.pppoe
#LOCAL_SDK_VERSION := current

LOCAL_PACKAGE_NAME := droidlogic-res
LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := optional

# Install thie to system/priv-app
LOCAL_PROGUARD_ENABLED := disabled

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
else
LOCAL_PRIVILEGED_MODULE := true
endif

include $(BUILD_PACKAGE)


