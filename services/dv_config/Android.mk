ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= dv_config.c
LOCAL_MODULE := dv_config
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libcutils libc liblog
include $(BUILD_EXECUTABLE)

endif  # TARGET_SIMULATOR != true
