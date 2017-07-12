LOCAL_PATH:= $(call my-dir)

#
# libimageplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  IImagePlayerService.cpp

LOCAL_SHARED_LIBRARIES := \
  libutils \
  libcutils \
  libmedia \
  libbinder

LOCAL_MODULE:= libimageplayerservice

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

# build for image server
# =========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
  main_imageserver.cpp \
  ImagePlayerService.cpp  \
  RGBPicture.c  \
  TIFF2RGBA.cpp

LOCAL_SHARED_LIBRARIES := \
  libimageplayerservice \
  libbinder                   \
  libskia                     \
  libcutils                   \
  libutils                    \
  liblog                      \
  libdl                       \
  libstagefright              \
  libmedia                    \
  libsystemcontrolservice     \
  libtiff

LOCAL_C_INCLUDES += \
  external/skia/include/core \
  external/skia/include/effects \
  external/skia/include/images \
  external/skia/src/ports \
  external/skia/include/utils \
  frameworks/av/include \
  frameworks/av \
  vendor/amlogic/frameworks/services/systemcontrol \
  vendor/amlogic/external/libtiff

LOCAL_MODULE:= imageserver
LOCAL_REQUIRED_MODULES := libtiff
LOCAL_32_BIT_ONLY := true

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_EXECUTABLE)
