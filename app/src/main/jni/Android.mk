LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := FFMediaPlayer-jni
LOCAL_SRC_FILES := cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayerJNI.c
LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid
LOCAL_LDLIBS += -L$(LOCAL_PATH)/ffmpeg-3.0.1/android/arm/lib/ \
                -lavformat-57 -lavcodec-57 -lswscale-4 -lavutil-55 -lavfilter-6 -lswresample-2

include $(BUILD_SHARED_LIBRARY)
#$(call import-module,ffmpeg-3.0.1/android/arm)