LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  ISystemControlService.cpp \
  ISystemControlNotify.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libbinder

LOCAL_MODULE:= libsystemcontrolservice

LOCAL_MODULE_TAGS := optional

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

LOCAL_SRC_FILES:= \
  main_systemcontrol.cpp \
  ubootenv.c \
  VdcLoop.c \
  SysWrite.cpp \
  SystemControl.cpp \
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

LOCAL_C_INCLUDES := \
  external/zlib \
  external/libcxx/include

LOCAL_MODULE:= systemcontrol

LOCAL_STATIC_LIBRARIES := \
  libz

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
LOCAL_SRC_FILES:= \
  main_recovery.cpp \
  ubootenv.c \
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

LOCAL_SRC_FILES:= \
  main_recovery.cpp \
  ubootenv.c \
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

include $(BUILD_STATIC_LIBRARY)
include $(LOCAL_PATH)/cmd/Android.mk
