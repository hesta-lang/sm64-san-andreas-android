LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sm64sanandreas
LOCAL_SRC_FILES := main.cpp deps/base/mod/logger.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/deps/base
LOCAL_CPPFLAGS += -std=c++17 -O2 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -Wall -Wextra -Wno-unused-parameter
LOCAL_LDLIBS += -llog
include $(BUILD_SHARED_LIBRARY)
