LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
  main_miracastserver.cpp \
  MiracastServer.cpp \
  MiracastService.cpp \
  SysTokenizer.cpp

LOCAL_C_INCLUDES := \
   external/libcxx/include \
   system/core/base/include \
   system/libhidl/transport \
   frameworks/native/libs/binder/include \
   hardware/libhardware/include \
   system/libhidl/transport/include/hidl \
   system/core/libutils/include \
   system/core/liblog/include

LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.miracastserver@1.0 \
  libhidlbase \
  libhidltransport \
  libutils \
  libcutils \
  libbinder \
  liblog

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := miracastserver

LOCAL_CPPFLAGS += -std=c++14
LOCAL_INIT_RC := miracastserver.rc

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_VENDOR_MODULE := true
include $(BUILD_EXECUTABLE)



