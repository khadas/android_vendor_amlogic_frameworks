LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

CSV_RET=$(shell ($(LOCAL_PATH)/csvAnalyze.sh > /dev/zero;echo $$?))
ifeq ($(CSV_RET), 1)
  $(error "Csv file or common.h file is not exist!!!!")
else ifeq ($(CSV_RET), 2)
  $(error "Csv file's Id must be integer")
else ifeq ($(CSV_RET), 3)
  $(error "Csv file's Size must be integer or defined in common.h")
endif

PQ_INCLUDE_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)frameworks/services/systemcontrol/PQ/include)
LIB_SQLITE_PATH := $(wildcard external/sqlite/dist)
LIB_TV_BINDER_PATH := $(wildcard $(BOARD_AML_VENDOR_PATH)tv/frameworks/libtvbinder)

LOCAL_SRC_FILES:= \
  CPQdb.cpp \
  COverScandb.cpp \
  CPQControl.cpp  \
  CSqlite.cpp  \
  SSMAction.cpp  \
  SSMHandler.cpp  \
  SSMHeader.cpp  \
  CDevicePollCheckThread.cpp  \
  CPQColorData.cpp  \
  CPQLog.cpp  \
  CFile.cpp  \
  CEpoll.cpp \
  CDynamicBackLight.cpp \
  CConfigFile.cpp

LOCAL_SHARED_LIBRARIES := \
  vendor.amlogic.hardware.tvserver@1.0 \
  libsqlite  \
  libutils  \
  liblog \
  libcutils \
  libtvbinder \
  libbinder

LOCAL_C_INCLUDES := \
  $(LIB_TV_BINDER_PATH)/include \
  $(PQ_INCLUDE_PATH) \
  $(LIB_SQLITE_PATH)

LOCAL_MODULE:= libpqcontrol

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)