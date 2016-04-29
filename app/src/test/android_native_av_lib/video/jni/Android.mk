BASE_PATH := $(call my-dir)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# our source files
#
LOCAL_SRC_FILES:= \
	surface.cpp

LOCAL_SHARED_LIBRARIES := \
    libskia \
    libutils \
    liblog \
	libgui \
	libui

#    libsurfaceflinger_client \

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
	frameworks/base/native/include \
	frameworks/base/include \
	external/skia/src/core \
	external/skia/include/core \

LOCAL_MODULE:= libjnivideo

include $(BUILD_SHARED_LIBRARY)

