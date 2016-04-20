LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES := 

LOCAL_MODULE := FFMediaPlayer-jni
LOCAL_SRC_FILES := libffplayer/cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer.cpp \
                   libffplayer/mediaplayer.cpp \

LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid
LOCAL_LDFLAGS := -L$(LOCAL_PATH)/ffmpeg-3.0.1/android/arm/lib/ -L$(LOCAL_PATH)/prebuilt/
LOCAL_LDLIBS += -lavformat-57 -lavcodec-57 -lswscale-4 -lavutil-55 -lavfilter-6 -lswresample-2 -ljniaudio -ljnivideo

#LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil libavfilter libswresample

include $(BUILD_SHARED_LIBRARY)
#$(call import-module,ffmpeg-3.0.1/android/arm)