LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  SystemControlClient.cpp \
  ISystemControlService.cpp \
  ISystemControlNotify.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  liblog \
  libbinder

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.systemcontrol@1.0_vendor \
  libbase \
  libhidlbase \
  libhidltransport

LOCAL_MODULE:= libsystemcontrolservice

LOCAL_MODULE_TAGS := optional

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxl)
LOCAL_CFLAGS += -DGXL_ENVSIZE
endif

LOCAL_CFLAGS += -DHDCP_AUTHENTICATION
LOCAL_CPPFLAGS += -std=c++14
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SRC_FILES:= \
  main_systemcontrol.cpp \
  ubootenv/Ubootenv.cpp \
  VdcLoop.c \
  SysWrite.cpp \
  SystemControl.cpp \
  SystemControlHal.cpp \
  SystemControlService.cpp \
  DisplayMode.cpp \
  Dimension.cpp \
  SysTokenizer.cpp \
  UEventObserver.cpp \
  HDCP/aes.cpp \
  HDCP/HdcpKeyDecrypt.cpp \
  HDCP/HDCPRxKey.cpp \
  HDCP/HDCPRxAuth.cpp \
  HDCP/HDCPTxAuth.cpp \
  HDCP/sha1.cpp \
  HDCP/HDCPRx22ImgKey.cpp \
  FrameRateAutoAdaption.cpp \
  FormatColorDepth.cpp

LOCAL_SHARED_LIBRARIES := \
  libsystemcontrolservice \
  libcutils \
  libutils \
  liblog \
  libbinder \
  libgui \
  libm

LOCAL_SHARED_LIBRARIES += \
  vendor.amlogic.hardware.systemcontrol@1.0_vendor \
  vendor.amlogic.hardware.droidvold@1.0_vendor \
  libbase \
  libhidlbase \
  libhidltransport

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include \
  system/libhidl/transport/include/hidl

LOCAL_MODULE:= systemcontrol

LOCAL_INIT_RC := systemcontrol.rc

LOCAL_STATIC_LIBRARIES := \
  libz

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)


# build for recovery mode
# =========================================================
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

LOCAL_CFLAGS += -DRECOVERY_MODE
LOCAL_CPPFLAGS += -std=c++14
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SRC_FILES:= \
  main_recovery.cpp \
  ubootenv/Ubootenv.cpp \
  SysWrite.cpp \
  DisplayMode.cpp \
  SysTokenizer.cpp \
  UEventObserver.cpp \
  HDCP/aes.cpp \
  HDCP/HdcpKeyDecrypt.cpp \
  HDCP/HDCPRxKey.cpp \
  HDCP/HDCPRxAuth.cpp \
  HDCP/HDCPTxAuth.cpp \
  FrameRateAutoAdaption.cpp \
  FormatColorDepth.cpp

LOCAL_STATIC_LIBRARIES := \
  libcutils \
  liblog \
  libz \
  libc \
  libm

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/utilities
LOCAL_MODULE:= systemcontrol_static

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), meson8)
LOCAL_CFLAGS += -DMESON8_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxbaby)
LOCAL_CFLAGS += -DGXBABY_ENVSIZE
endif

ifeq ($(TARGET_BOARD_PLATFORM), gxtvbb)
LOCAL_CFLAGS += -DGXTVBB_ENVSIZE
endif

LOCAL_CFLAGS += -DRECOVERY_MODE
LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SRC_FILES:= \
  main_recovery.cpp \
  ubootenv/Ubootenv.cpp \
  SysWrite.cpp \
  DisplayMode.cpp \
  SysTokenizer.cpp \
  UEventObserver.cpp \
  HDCP/aes.cpp \
  HDCP/HdcpKeyDecrypt.cpp \
  HDCP/HDCPRxKey.cpp \
  HDCP/HDCPRxAuth.cpp \
  HDCP/HDCPTxAuth.cpp \
  FrameRateAutoAdaption.cpp \
  FormatColorDepth.cpp

LOCAL_STATIC_LIBRARIES := \
  libcutils \
  liblog \
  libz \
  libc \
  libm

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include

LOCAL_MODULE:= libsystemcontrol_static

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_STATIC_LIBRARY)
