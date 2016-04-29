LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    main.cpp

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    liblog \
	libgui \
	libui \
	libstagefright_foundation

LOCAL_C_INCLUDES += \
	frameworks/base/native/include \
	frameworks/base/include \
	external/skia/src/core \
	external/skia/include/core \

LOCAL_MODULE:= xx

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)

