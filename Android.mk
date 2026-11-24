LOCAL_PATH := $(call my-dir)
LIBSM64_PATH := $(LOCAL_PATH)/deps/libsm64

# Build upstream libsm64 as a static library for the game's 32-bit ARM ABI.
LIBSM64_SRC_DIRS := \
    src \
    src/decomp \
    src/decomp/engine \
    src/decomp/include/PR \
    src/decomp/game \
    src/decomp/pc \
    src/decomp/pc/audio \
    src/decomp/mario \
    src/decomp/tools \
    src/decomp/audio

include $(CLEAR_VARS)
LOCAL_MODULE := sm64
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(foreach dir,$(LIBSM64_SRC_DIRS),$(wildcard $(LIBSM64_PATH)/$(dir)/*.c)))
LOCAL_SRC_FILES += deps/libsm64/src/decomp/mario/geo.inc.c
LOCAL_SRC_FILES += deps/libsm64/src/decomp/mario/model.inc.c
LOCAL_C_INCLUDES := $(LIBSM64_PATH)/src $(LIBSM64_PATH)/src/decomp/include
LOCAL_CFLAGS += -std=gnu99 -fno-strict-aliasing -Wall -Wno-unused-function -fPIC -fvisibility=hidden
LOCAL_CFLAGS += -DSM64_LIB_EXPORT -DGBI_FLOATS -DVERSION_US -DNO_SEGMENTED_MEMORY
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sm64sanandreas
LOCAL_SRC_FILES := main.cpp sm64_bridge.cpp deps/base/mod/logger.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/deps/base $(LIBSM64_PATH)/src
LOCAL_CPPFLAGS += -std=c++17 -O2 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -Wall -Wextra -Wno-unused-parameter
LOCAL_STATIC_LIBRARIES := sm64
LOCAL_LDLIBS += -llog -lm
include $(BUILD_SHARED_LIBRARY)
