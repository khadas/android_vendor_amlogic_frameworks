LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  RemoteControl.cpp \
  RemoteControlImpl.cpp \
  RemoteControlServer.cpp

LOCAL_C_INCLUDES += \
    external/libcxx/include \
    system/libhidl/transport/include/hidl

LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.remotecontrol@1.0 \
  libhidlbase \
  libhidltransport \
  libutils \
  libcutils \
  liblog

LOCAL_MODULE:= libremotecontrolserver

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

#build bin test service
ifeq ($(BUILD_RCSERVICE_TEST),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := server_main.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libremotecontrolserver

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := rc-server

LOCAL_CPPFLAGS += -std=c++14
LOCAL_INIT_RC := rc_server.rc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)

endif


