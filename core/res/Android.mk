
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_JAVA_LIBRARIES := droidlogic \

LOCAL_JNI_SHARED_LIBRARIES := libremotecontrol_jni libjniuevent

#LOCAL_SDK_VERSION := current
ifndef PRODUCT_SHIPPING_API_LEVEL
LOCAL_PRIVATE_PLATFORM_APIS := true
endif

LOCAL_PACKAGE_NAME := droidlogic-res
LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := optional

# Install thie to system/priv-app
LOCAL_PROGUARD_ENABLED := disabled

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_PRIVILEGED_MODULE := true

include $(BUILD_PACKAGE)


include $(call all-makefiles-under,$(LOCAL_PATH))
