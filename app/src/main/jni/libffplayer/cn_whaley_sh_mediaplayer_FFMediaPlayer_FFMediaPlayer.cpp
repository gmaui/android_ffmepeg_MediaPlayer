#include "cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#ifdef __cplusplus
}
#endif

#include "mediaplayer.h"

#include "android/log.h"

static const char *const kClassPathName = "cn/whaley/sh/mediaplayer/FFMediaPlayer/FFMediaPlayer";
static const char *const TAG = "FFMediaPlayer-JNI";

struct fields_t {
    jfieldID context;
    jmethodID post_event;
};
static fields_t fields;

static JavaVM *sVm;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv *env, const char *className, const char *msg) {
    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,
                            TAG,
                            "Unable to find exception class %s",
                            className);
        return -1;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
                            TAG,
                            "Failed throwing '%s' '%s'",
                            className, msg);
    }
    return 0;
}

JNIEnv *getJNIEnv() {
    JNIEnv *env = NULL;
    if (sVm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
                            TAG,
                            "Failed to obtain JNIEnv");
        return NULL;
    }
    return env;
}

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayerListener : public MediaPlayerListener {
public:
    JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz);

    ~JNIMediaPlayerListener();

    void notify(int msg, int ext1, int ext2);

private:
    JNIMediaPlayerListener();

    jclass mClass;     // Reference to MediaPlayer class
    jobject mObject;    // Weak ref to MediaPlayer Java object to call on
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv *env, jobject thiz, jobject weak_thiz) {
    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/Exception", kClassPathName);
        return;
    }
    mClass = (jclass) env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener() {
    // remove global references
    JNIEnv *env = getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2) {
    JNIEnv *env = getJNIEnv();
    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, 0);
}

// ----------------------------------------------------------------------------

static MediaPlayer *getMediaPlayer(JNIEnv *env, jobject thiz) {
    return (MediaPlayer *) env->GetIntField(thiz, fields.context);
}

static MediaPlayer *setMediaPlayer(JNIEnv *env, jobject thiz, MediaPlayer *player) {
    MediaPlayer *old = (MediaPlayer *) env->GetIntField(thiz, fields.context);
    if (old != NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "freeing old mediaplayer object");
        delete old;
    }
    env->SetIntField(thiz, fields.context, (int) player);
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, status_t opStatus,
                                      const char *exception, const char *message) {
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        /*
        if (opStatus != (status_t) OK) {
            sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
        */
    } else {  // Throw exception!
        if (opStatus == (status_t) INVALID_OPERATION) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if (opStatus != (status_t) OK) {
            if (strlen(message) > 230) {
                // if the message is too long, don't bother displaying the status code
                jniThrowException(env, exception, message);
            } else {
                char msg[256];
                // append the status code to the message
                sprintf(msg, "%s: status=0x%X", message, opStatus);
                jniThrowException(env, exception, msg);
            }
        }
    }
}


/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1getVersion
        (JNIEnv *env, jobject thiz) {
    char wd[128] = {0};
    sprintf(wd, "VERSION: AVCODEC(%u) AVFORMAT(%u)\n", avcodec_version(), avformat_version());
    //return (*env)->NewStringUTF(env, wd);
    return env->NewStringUTF(wd);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_setDataSource
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1setDataSource
        (JNIEnv *env, jobject thiz, jstring path) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *pathStr = env->GetStringUTFChars(path, NULL);
    if (pathStr == NULL) {  // Out of memory
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource: path %s", pathStr);
    status_t opStatus = mp->setDataSource(pathStr);

    // Make sure that local ref is released before a potential exception
    env->ReleaseStringUTFChars(path, pathStr);

    process_media_player_call(
            env, thiz, opStatus, "java/io/IOException",
            "setDataSource failed.");

}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_prepare
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1prepare
        (JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepare");
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->prepare(), "java/io/IOException", "Prepare failed.");
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_prepareAsync
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1prepareAsync
        (JNIEnv *env, jobject thiz) { }

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_start
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1start
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->start(), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1stop
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->stop(), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_pause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1pause
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->pause(), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_seekTo
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1seekTo
        (JNIEnv *env, jobject thiz, jint msec) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->seekTo(msec), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1release
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->release(), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1reset
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call(env, thiz, mp->reset(), NULL, NULL);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_isPlaying
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1isPlaying
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    const jboolean is_playing = mp->isPlaying();
    return is_playing;
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_getVideoWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1getVideoWidth
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w;
    if (0 != mp->getVideoWidth(&w)) {
        w = 0;
    }
    return w;
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_getVideoHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1getVideoHeight
        (JNIEnv *env, jobject thiz) {
    MediaPlayer *mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h;
    if (0 != mp->getVideoHeight(&h)) {
        h = 0;
    }
    return h;
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_invoke
 * Signature: (Landroid/os/Parcel;Landroid/os/Parcel;)I
 */
JNIEXPORT jint JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1invoke
        (JNIEnv *env, jobject thiz, jobject jobject2, jobject jobject3) { return 0; }

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_setParameter
 * Signature: (ILandroid/os/Parcel;)Z
 */
JNIEXPORT jboolean JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1setParameter
        (JNIEnv *env, jobject thiz, jint jint1, jobject jobject2) { return 0; }

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_setVideoSurface
 * Signature: (Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1setVideoSurface
        (JNIEnv *env, jobject thiz, jobject jsurface) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_setVideoSurface");
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    if (jsurface == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setVideoSurface(env, jsurface),
                               "java/io/IOException", "Set video surface failed.");
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1init
        (JNIEnv *env, jclass jclass1) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_init");
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException",
                          "Can't find cn.whaley.sh.mediaplayer.FFMediaPlayer.FFMediaPlayer");
        return;
    }

    fields.context = env->GetFieldID(clazz , "mNativeContext", "I");
    if (fields.context == NULL) {
        jniThrowException(env, "java/lang/RuntimeException",
                          "Can't find MediaPlayer.mNativeContext");
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        jniThrowException(env, "java/lang/RuntimeException",
                          "Can't find FFMpegMediaPlayer.postEventFromNative");
        return;
    }
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_setup
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1setup
        (JNIEnv *env, jobject thiz, jobject weak_this) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_setup");
    MediaPlayer *mp = new MediaPlayer();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener *listener = new JNIMediaPlayerListener(env, thiz, weak_this);
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    setMediaPlayer(env, thiz, mp);
}

/*
 * Class:     cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer
 * Method:    native_finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_cn_whaley_sh_mediaplayer_FFMediaPlayer_FFMediaPlayer_native_1finalize
        (JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_finalize");
}

