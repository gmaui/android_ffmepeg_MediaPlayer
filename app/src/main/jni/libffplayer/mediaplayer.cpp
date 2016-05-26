//
// Created by hc on 2016/4/12.
//
/*
 * mediaplayer.cpp
 */

#define LOG_NDEBUG 0
#define TAG "mediaplayer"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libavutil/avutil.h"
#include "libavutil/frame.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

#include <android/log.h>
#include "mediaplayer.h"

#define FPS_DEBUGGING false

static MediaPlayer *sPlayer;
static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

MediaPlayer::MediaPlayer() {
    mListener = NULL;
    mCookie = NULL;
    mDuration = -1;
    mStreamType = ANDROID_AUDIO_STREAM_MUSIC;
    mCurrentPosition = -1;
    mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mPrepareSync = false;
    mPrepareStatus = NO_ERROR;
    mLoop = false;
    pthread_mutex_init(&mLock, NULL);
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
    mAudioOnly = 0;

    sPlayer = this;
}

MediaPlayer::~MediaPlayer() {
    if (mListener != NULL) {
        free(mListener);
    }
    if(mMovieFile != NULL)
    {
        avformat_free_context(mMovieFile);
        mMovieFile = NULL;
    }
}

status_t MediaPlayer::prepareAudio() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio");
    mAudioStreamIndex = -1;
    for (int i = 0; i < mMovieFile->nb_streams; i++) {
        if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            mAudioStreamIndex = i;
            __android_log_print(ANDROID_LOG_INFO, TAG,
                                "prepareAudio found the first audio stream index!");
            break;
        }
    }

    if (mAudioStreamIndex == -1) {
        __android_log_print(ANDROID_LOG_INFO, TAG,
                            "prepareAudio can't find the first audio stream index!");
        return INVALID_OPERATION;
    }

    AVStream *stream = mMovieFile->streams[mAudioStreamIndex];
    // Get a pointer to the codec context for the video stream
    AVCodecContext *codec_ctx = stream->codec;
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio avcodec_find_decoder failed!");
        return INVALID_OPERATION;
    }

    AVDictionary *param = 0;
    // Open codec
    if (avcodec_open2(codec_ctx, codec, &param) < 0) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio avcodec_open2 failed!");
        return INVALID_OPERATION;
    }

    if (Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_INFO, TAG,
                            "prepareAudio Output::AudioDriver_register failed!");
        return INVALID_OPERATION;
    }

    // prepare os output
    if (Output::AudioDriver_set(ANDROID_AUDIO_STREAM_MUSIC,
                                stream->codec->sample_rate,
                                ANDROID_AUDIO_FORMAT_PCM_16_BIT,
                                (stream->codec->channels == 2) ? ANDROID_AUDIO_CHANNEL_OUT_STEREO
                                                               : ANDROID_AUDIO_CHANNEL_OUT_MONO) !=
        ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio Output::AudioDriver_set failed!");
        return INVALID_OPERATION;
    }

    if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_INFO, TAG,
                            "prepareAudio Output::AudioDriver_start failed!");
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t MediaPlayer::prepareVideo() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo");
    // Find the first video stream
    mVideoStreamIndex = -1;
    for (int i = 0; i < mMovieFile->nb_streams; i++) {
        if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStreamIndex = i;
            __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo find the first videostream");
            break;
        }
    }

    if (mVideoStreamIndex == -1) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo can't find the first videostream");
        return INVALID_OPERATION;
    }

    AVStream *stream = mMovieFile->streams[mVideoStreamIndex];
    // Get a pointer to the codec context for the video stream
    AVCodecContext *codec_ctx = stream->codec;
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo can't find the codec");
        return INVALID_OPERATION;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo find the codec");

    AVDictionary *param = 0;
    // Open codec
    if (avcodec_open2(codec_ctx, codec, &param) < 0) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo avcodec_open2 failed");
        return INVALID_OPERATION;
    }

    mVideoWidth = codec_ctx->width;
    mVideoHeight = codec_ctx->height;
    mDuration = mMovieFile->duration;
    __android_log_print(ANDROID_LOG_INFO, TAG, "avcodec_open2 (w,h,dur)=(%d-%d-%d)",mVideoWidth, mVideoHeight, mDuration);
    mConvertCtx = sws_getContext(stream->codec->width,
                                 stream->codec->height,
                                 stream->codec->pix_fmt,
                                 stream->codec->width,
                                 stream->codec->height,
                                 AV_PIX_FMT_YUV420P,
                                 SWS_POINT,
                                 NULL,
                                 NULL,
                                 NULL);

    if (mConvertCtx == NULL) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo sws_getContext failed");
        return INVALID_OPERATION;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo sws_getContext done.");
//
//    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo VideoDriver_getPixels");
//    void *pixels;
//    if (Output::VideoDriver_getPixels(stream->codec->width,
//                                      stream->codec->height,
//                                      &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
//        __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo VideoDriver_getPixels failed");
//        return INVALID_OPERATION;
//    }
//
//
//    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo allocated");
//    mFrame = av_frame_alloc();
//    if (mFrame == NULL) {
//        return INVALID_OPERATION;
//    }
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
//    avpicture_fill((AVPicture *) mFrame,
//                   (uint8_t *) pixels,
//                   AV_PIX_FMT_YUV420P,
//                   stream->codec->width,
//                   stream->codec->height);
//    __android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo done");
    return NO_ERROR;
}

status_t MediaPlayer::prepare() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "prepare");
    status_t ret;
    mCurrentState = MEDIA_PLAYER_PREPARING;
    av_log_set_callback(ffmpegNotify);
    if ((ret = prepareVideo()) != NO_ERROR) {
        //mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        //return ret;
        mAudioOnly = true;
        __android_log_print(ANDROID_LOG_INFO, TAG, "prepare :only audio");
    }
    if ((ret = prepareAudio()) != NO_ERROR) {
        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
        return ret;
    }
    mCurrentState = MEDIA_PLAYER_PREPARED;
    return NO_ERROR;
}

status_t MediaPlayer::setListener(MediaPlayerListener *listener) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
    av_register_all();
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(register_all)");
    // Open video file
    char buf[1024] = {0};
    mMovieFile = avformat_alloc_context();
    int err_code = avformat_open_input(&mMovieFile, url, NULL, NULL);
    if (err_code != 0) {
        av_strerror(err_code, buf, 1024);
        __android_log_print(ANDROID_LOG_INFO, TAG, "Couldn't open file %s: %d(%s)", url, err_code,
                            buf);
        return INVALID_OPERATION;
    }
    // Retrieve stream information
    if (avformat_find_stream_info(mMovieFile, NULL) < 0) {
        return INVALID_OPERATION;
    }
    mCurrentState = MEDIA_PLAYER_INITIALIZED;
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "suspend");

    mCurrentState = MEDIA_PLAYER_STOPPED;
    if (mDecoderAudio != NULL) {
        mDecoderAudio->stop();
    }
    if (mDecoderVideo != NULL) {
        mDecoderVideo->stop();
    }

    if (pthread_join(mPlayerThread, NULL) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel player thread");
    }

    // Close the codec
    free(mDecoderAudio);
    free(mDecoderVideo);

    // Close the video file
    avformat_close_input(&mMovieFile);

    //close OS drivers
    Output::AudioDriver_unregister();
    Output::VideoDriver_unregister();

    __android_log_print(ANDROID_LOG_ERROR, TAG, "suspended");

    return NO_ERROR;
}

status_t MediaPlayer::resume() {
    //pthread_mutex_lock(&mLock);
    mCurrentState = MEDIA_PLAYER_STARTED;
    //pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::release() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "release IN");
    suspend();
    __android_log_print(ANDROID_LOG_INFO, TAG, "release OUT");
    return NO_ERROR;
}


status_t MediaPlayer::setVideoSurface(JNIEnv *env, jobject jsurface) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "setVideoSurface");
    if (Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "setVideoSurface failed");
        return INVALID_OPERATION;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "setVideoSurface done");
//    if (Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
//        return INVALID_OPERATION;
//    }
    return NO_ERROR;
}

bool MediaPlayer::shouldCancel(PacketQueue *queue) {
    return (mCurrentState == MEDIA_PLAYER_STATE_ERROR || mCurrentState == MEDIA_PLAYER_STOPPED ||
            ((mCurrentState == MEDIA_PLAYER_DECODED || mCurrentState == MEDIA_PLAYER_STARTED)
             && queue->size() == 0));
}

void MediaPlayer::decode(AVFrame *frame, double pts) {
    if (FPS_DEBUGGING) {
        timeval pTime;
        static int frames = 0;
        static double t1 = -1;
        static double t2 = -1;

        gettimeofday(&pTime, NULL);
        t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
        if (t1 == -1 || t2 > t1 + 1) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Video fps:%i", frames);
            //sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
            t1 = t2;
            frames = 0;
        }
        frames++;
    }
    __android_log_print(ANDROID_LOG_ERROR, TAG, "VideoDecoder writePixels");
    // Convert the image from its native format to RGB
//    sws_scale(sPlayer->mConvertCtx,
//              frame->data,
//              frame->linesize,
//              0,
//              sPlayer->mVideoHeight,
//              sPlayer->mFrame->data,
//              sPlayer->mFrame->linesize);
    Output::VideoDriver_writePixels(frame->width, frame->height,
                               (void*)frame->data[0], (void*)frame->data[2], (void*)frame->data[1],pts);
    //Output::VideoDriver_updateSurface();
}

void MediaPlayer::decode(int16_t *buffer, int buffer_size) {
    if (FPS_DEBUGGING) {
        timeval pTime;
        static int frames = 0;
        static double t1 = -1;
        static double t2 = -1;

        gettimeofday(&pTime, NULL);
        t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
        if (t1 == -1 || t2 > t1 + 1) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "Audio fps:%i", frames);
            //sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
            t1 = t2;
            frames = 0;
        }
        frames++;
    }

    if (Output::AudioDriver_write(buffer, buffer_size) <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't write samples to audio track");
    }
}

void MediaPlayer::decodeMovie(void *ptr) {
    AVPacket pPacket;

    AVStream *stream_audio = mMovieFile->streams[mAudioStreamIndex];
    mDecoderAudio = new DecoderAudio(stream_audio);
    mDecoderAudio->onDecode = decode;
    mDecoderAudio->startAsync();

    AVStream *stream_video = mMovieFile->streams[mVideoStreamIndex];
    mDecoderVideo = new DecoderVideo(stream_video);
    mDecoderVideo->onDecode = decode;
    mDecoderVideo->startAsync();

    mCurrentState = MEDIA_PLAYER_STARTED;
    __android_log_print(ANDROID_LOG_INFO, TAG, "playing %ix%i", mVideoWidth, mVideoHeight);
    while (mCurrentState != MEDIA_PLAYER_DECODED && mCurrentState != MEDIA_PLAYER_STOPPED &&
           mCurrentState != MEDIA_PLAYER_STATE_ERROR) {
        if (mDecoderVideo->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE &&
                mDecoderAudio->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) {
            usleep(200);
            continue;
        }

        if (av_read_frame(mMovieFile, &pPacket) < 0) {
            mCurrentState = MEDIA_PLAYER_DECODED;
            continue;
        }

        // Is this a packet from the video stream?
        if (pPacket.stream_index == mVideoStreamIndex) {
            mDecoderVideo->enqueue(&pPacket);
        }
        else if (pPacket.stream_index == mAudioStreamIndex) {
            mDecoderAudio->enqueue(&pPacket);
        }
        else {
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&pPacket);
        }
    }

    //waits on end of video thread
    __android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on video thread");
    int ret = -1;
    if ((ret = mDecoderVideo->wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel video thread: %i", ret);
    }

    __android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on audio thread");
    if ((ret = mDecoderAudio->wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio thread: %i", ret);
    }

    if (mCurrentState == MEDIA_PLAYER_STATE_ERROR) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "playing err");
    }
    mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
    __android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

void *MediaPlayer::startPlayer(void *ptr) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting main player thread");
    sPlayer->decodeMovie(ptr);
}

status_t MediaPlayer::start() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "start");
    if (mCurrentState != MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    pthread_create(&mPlayerThread, NULL, startPlayer, NULL);
    return NO_ERROR;
}

status_t MediaPlayer::stop() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "stop");
    //pthread_mutex_lock(&mLock);
    mCurrentState = MEDIA_PLAYER_STOPPED;
    //pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::pause() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "pause");
    //pthread_mutex_lock(&mLock);
    mCurrentState = MEDIA_PLAYER_PAUSED;
    //pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

bool MediaPlayer::isPlaying() {
    return mCurrentState == MEDIA_PLAYER_STARTED ||
           mCurrentState == MEDIA_PLAYER_DECODED;
}

status_t MediaPlayer::getVideoWidth(int *w) {
    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h) {
    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec) {
    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *msec = 0/*av_gettime()*/;
    //__android_log_print(ANDROID_LOG_INFO, TAG, "position %i", *msec);
    return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec) {
    if (mCurrentState < MEDIA_PLAYER_PREPARED) {
        return INVALID_OPERATION;
    }
    *msec = mDuration / 1000;
    return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec) {
    return INVALID_OPERATION;
}

status_t MediaPlayer::reset() {
    return INVALID_OPERATION;
}

status_t MediaPlayer::setAudioStreamType(int type) {
    return NO_ERROR;
}

void MediaPlayer::ffmpegNotify(void *ptr, int level, const char *fmt, va_list vl) {

    switch (level) {
        /**
         * Something went really wrong and we will crash now.
         */
        case AV_LOG_PANIC:
            __android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_PANIC: %s", fmt);
            //sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
            break;

            /**
             * Something went wrong and recovery is not possible.
             * For example, no header was found for a format which depends
             * on headers or an illegal combination of parameters is used.
             */
        case AV_LOG_FATAL:
            __android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_FATAL: %s", fmt);
            //sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
            break;

            /**
             * Something went wrong and cannot losslessly be recovered.
             * However, not all future data is affected.
             */
        case AV_LOG_ERROR:
            __android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_ERROR: %s", fmt);
            //sPlayer->mCurrentState = MEDIA_PLAYER_STATE_ERROR;
            break;

            /**
             * Something somehow does not look correct. This may or may not
             * lead to problems. An example would be the use of '-vstrict -2'.
             */
        case AV_LOG_WARNING:
            __android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_WARNING: %s", fmt);
            break;

        case AV_LOG_INFO:
            __android_log_print(ANDROID_LOG_INFO, TAG, "%s", fmt);
            break;

        case AV_LOG_DEBUG:
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", fmt);
            break;

    }
}

void MediaPlayer::notify(int msg, int ext1, int ext2) {
    //__android_log_print(ANDROID_LOG_INFO, TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    if ((mListener != 0) && send) {
        //__android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
        mListener->notify(msg, ext1, ext2);
        //__android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
    }
}

//-------------------- Audio driver --------------------

int Output::AudioDriver_register() {
    return AndroidAudioTrack_register();
}

int Output::AudioDriver_unregister() {
    return AndroidAudioTrack_unregister();
}

int Output::AudioDriver_start() {
    return AndroidAudioTrack_start();
}

int Output::AudioDriver_set(int streamType,
                            uint32_t sampleRate,
                            int format,
                            int channels) {
    return AndroidAudioTrack_set(streamType,
                                 sampleRate,
                                 format,
                                 channels);
}

int Output::AudioDriver_flush() {
    return AndroidAudioTrack_flush();
}

int Output::AudioDriver_stop() {
    return AndroidAudioTrack_stop();
}

int Output::AudioDriver_reload() {
    return AndroidAudioTrack_reload();
}

int Output::AudioDriver_write(void *buffer, int buffer_size) {
    return AndroidAudioTrack_write(buffer, buffer_size);
}

//-------------------- Video driver --------------------

int Output::VideoDriver_register(JNIEnv *env, jobject jsurface) {
    return AndroidSurface_register(env, jsurface);
}

int Output::VideoDriver_unregister() {
    return AndroidSurface_unregister();
}

int Output::VideoDriver_writePixels(int width, int height, void *py,void *pu,void *pv,long long int timestamp) {
    return AndroidSurface_writePixels(width, height, py, pu, pv, timestamp);
}

int Output::VideoDriver_updateSurface() {
//    return AndroidSurface_updateSurface();
    return 0;
}

Thread::Thread() {
    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCondition, NULL);
}

Thread::~Thread() {
}

void Thread::start() {
    handleRun(NULL);
}

void Thread::startAsync() {
    pthread_create(&mThread, NULL, startThread, this);
}

int Thread::wait() {
    if (!mRunning) {
        return 0;
    }
    return pthread_join(mThread, NULL);
}

void Thread::stop() {
}

void *Thread::startThread(void *ptr) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting thread");
    Thread *thread = (Thread *) ptr;
    thread->mRunning = true;
    thread->handleRun(ptr);
    thread->mRunning = false;
    __android_log_print(ANDROID_LOG_INFO, TAG, "thread ended");
}

void Thread::waitOnNotify() {
    pthread_mutex_lock(&mLock);
    pthread_cond_wait(&mCondition, &mLock);
    pthread_mutex_unlock(&mLock);
}

void Thread::notify() {
    pthread_mutex_lock(&mLock);
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
}

void Thread::handleRun(void *ptr) {
}

IDecoder::IDecoder(AVStream * stream) {
    mQueue = new PacketQueue();
    mStream = stream;
}

IDecoder::~IDecoder() {
    if (mRunning) {
        stop();
    }
    free(mQueue);
    avcodec_close(mStream->codec);
}

void IDecoder::enqueue(AVPacket * packet) {
    if(NULL == mQueue)
        return;
    mQueue->put(packet);
}

int IDecoder::packets() {
    if(NULL == mQueue)
        return 0;
    return mQueue->size();
}

void IDecoder::stop() {
    if(NULL == mQueue)
        return;
    mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of decoder thread");
    int ret = -1;
    if ((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel IDecoder: %i", ret);
        return;
    }
}

void IDecoder::handleRun(void *ptr) {
    if (!prepare()) {
        __android_log_print(ANDROID_LOG_INFO, TAG, "Couldn't prepare decoder");
        return;
    }
    decode(ptr);
}

bool IDecoder::prepare() {
    return false;
}

bool IDecoder::process(AVPacket * packet) {
    return false;
}

bool IDecoder::decode(void *ptr) {
    return false;
}


DecoderAudio::DecoderAudio(AVStream * stream) : IDecoder(stream) {
}

DecoderAudio::~DecoderAudio() {
}

bool DecoderAudio::prepare() {
    __android_log_print(ANDROID_LOG_INFO, TAG, "DecoderAudio::prepare");
    mFrame = av_frame_alloc();
    mSwrFrame = av_frame_alloc();
    mSwrCtx = swr_alloc();

    if (mFrame == NULL || mSwrFrame == NULL || mSwrCtx == NULL) {
        return false;
    }
    //initSwr
    if(mStream->codec->sample_fmt != AV_SAMPLE_FMT_S16)
    {
        av_opt_set_int(mSwrCtx, "in_channel_layout",    mStream->codec->channel_layout, 0);
        av_opt_set_int(mSwrCtx, "in_sample_rate",       mStream->codec->sample_rate, 0);
        av_opt_set_sample_fmt(mSwrCtx, "in_sample_fmt", mStream->codec->sample_fmt, 0);
        av_opt_set_int(mSwrCtx, "out_channel_layout",    mStream->codec->channel_layout, 0);
        av_opt_set_int(mSwrCtx, "out_sample_rate",       mStream->codec->sample_rate, 0);
        av_opt_set_sample_fmt(mSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

        swr_init(mSwrCtx);
        mResample = 1;
    }else{
        mResample = 0;
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "DecoderAudio::prepare mResample=%d", mResample);
    return true;
}

bool DecoderAudio::process(AVPacket * packet) {
    int success = 0, len = 0;
    int pkt_size = packet->size;
    for (; pkt_size > 0; pkt_size -= len) {
        len = avcodec_decode_audio4(mStream->codec, mFrame, &success, packet);
        __android_log_print(ANDROID_LOG_INFO, TAG, "decoding success=%d len=%d pkt_size=%d", success, len, pkt_size);
        if (len > 0 && success) {
            if(mResample == 1)
            {
                mFrame->pts = av_frame_get_best_effort_timestamp(mFrame);
                mSwrFrame->pts = mFrame->pts;

                if (mSwrCtx != NULL)
                {
                    mSwrFrame->nb_samples = av_rescale_rnd(swr_get_delay(mSwrCtx, mStream->codec->sample_rate) + mFrame->nb_samples,
                                                           mStream->codec->sample_rate, mStream->codec->sample_rate, AV_ROUND_UP);

                    av_freep(&mSwrFrame->data[0]);
                    int ret = av_samples_alloc(mSwrFrame->data, &mSwrFrame->linesize[0],
                                           mStream->codec->channels, mSwrFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
                    if (ret < 0)
                    {
                        return false;
                    }

                    ret = swr_convert(mSwrCtx, mSwrFrame->data, mSwrFrame->nb_samples,
                                      (const uint8_t**)mFrame, mFrame->nb_samples);
                    if (ret < 0)
                    {
                        return false;
                    }
                }
            }
            else{
                mSwrFrame = mFrame;
            }
        int data_size = av_samples_get_buffer_size(mSwrFrame->linesize,
                                                   mStream->codec->channels, mSwrFrame->nb_samples,
                                                   AV_SAMPLE_FMT_S16, 1);
//            int data_size = mFrame->nb_samples * av_frame_get_channels(mFrame) * sizeof(int16_t);
            __android_log_print(ANDROID_LOG_INFO, TAG, "decoding data_size=%d decode_size=%d channes=%d samples=%d sample_fmt=%d",
                                data_size, mSwrFrame->linesize[0],mStream->codec->channels, mSwrFrame->nb_samples,mStream->codec->sample_fmt);
            //call handler for posting buffer to os audio driver
            onDecode((int16_t *) mSwrFrame->data[0],data_size/* mSwrFrame->linesize[0]*/);
        } else {
            pkt_size = 0;
        }
    }
    return true;
}

bool DecoderAudio::decode(void *ptr) {
    AVPacket pPacket;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

    while (mRunning) {
        if (mQueue->get(&pPacket, true) < 0) {
            mRunning = false;
            return false;
        }
        if (!process(&pPacket)) {
            mRunning = false;
            return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio ended");

    // Free audio samples buffer
    av_frame_free(&mFrame);
    return true;
}


DecoderVideo::DecoderVideo(AVStream * stream) : IDecoder(stream) {
//    mStream->codec->get_buffer2 = getBuffer;
//    mStream->codec->release_buffer2 = releaseBuffer;
}

DecoderVideo::~DecoderVideo() {
}

bool DecoderVideo::prepare() {
    mFrame = av_frame_alloc();
    if (mFrame == NULL) {
        return false;
    }
    mSwsCtx = sws_getContext(mStream->codec->width,
                             mStream->codec->height,
                             mStream->codec->pix_fmt,
                             mStream->codec->width,
                             mStream->codec->height,
                   AV_PIX_FMT_YUV420P,
                   SWS_POINT,
                   NULL,
                   NULL,
                   NULL);
    if (mSwsCtx == NULL) {
        return false;
    }
    return true;
}

double DecoderVideo::synchronize(AVFrame *src_frame, double pts) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "synchronize");
    double frame_delay;

    if (pts != 0) {
        /* if we have pts, set video clock to it */
        mVideoClock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = mVideoClock;
    }
    /* update the video clock */
    frame_delay = av_q2d(mStream->codec->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    mVideoClock += frame_delay;
    return pts;
}

bool DecoderVideo::process(AVPacket * packet) {
    static int iFrame = 0;
    int completed = 0;
    double pts = 0;
    __android_log_print(ANDROID_LOG_INFO, TAG, "DecoderVideo::process");
    // Decode video frame
    char buf[1024] = {0};
    int err_code = avcodec_decode_video2(mStream->codec,
                          mFrame,
                          &completed,
                          packet);
    if (err_code < 0) {
        av_strerror(err_code, buf, 1024);
        __android_log_print(ANDROID_LOG_INFO, TAG, "avcodec_decode_video2: %d(%s)", err_code,
                            buf);
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "avcodec_decode_video2 completed=%d", completed);
    pts = av_frame_get_best_effort_timestamp(mFrame);

    pts = pts*av_q2d(mStream->time_base);

    if (completed) {
        pts = synchronize(mFrame, pts);
        AVCodecContext* codec_ctx = mStream->codec;
        mSwsFrame = av_frame_alloc();
        if (mSwsFrame == NULL) {
            __android_log_print(ANDROID_LOG_INFO, TAG, "av_frame_alloc mSwsFrame failed");
        }
        int numBytesFrame = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
        mBufferFrame = (uint8_t *)av_malloc(numBytesFrame*sizeof(uint8_t));
        avpicture_fill((AVPicture *)mSwsFrame, mBufferFrame, AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height);
        mSwsFrame->width = mFrame->width;
        mSwsFrame->height = mFrame->height;
        sws_scale(mSwsCtx, mFrame->data, mFrame->linesize, 0, codec_ctx->height, mSwsFrame->data, mSwsFrame->linesize);
        __android_log_print(ANDROID_LOG_INFO, TAG, "DecoderVideo onDecode ");
        onDecode(mSwsFrame, pts);
        if(iFrame++ == 0){
            __android_log_print(ANDROID_LOG_INFO, TAG, "DataInfo: (w-h)=(%d-%d) (y-u-v)=(%d-%d-%d)",mSwsFrame->width, mSwsFrame->height,mSwsFrame->linesize[0], mSwsFrame->linesize[1],mSwsFrame->linesize[2]);
            FILE *pFile;
            char szFilename[32];
            int i;

            // Open file
            sprintf(szFilename, "/mnt/usb/sda1/video/frame%d_%d_%d.yuv", iFrame, mSwsFrame->width, mSwsFrame->height);
            pFile=fopen(szFilename, "wb");
            __android_log_print(ANDROID_LOG_INFO, TAG, "szFileName:%s %d",szFilename,pFile==NULL);
            if(pFile!=NULL) {

                // Write header
                //fprintf(pFile, "P6\n%d %d\n255\n", mSwsFrame->width, mSwsFrame->height);

                // Write pixel data
                for (i = 0; i < mSwsFrame->height; i++)
                    fwrite(mSwsFrame->data[0] + i * mSwsFrame->linesize[0], 1, mSwsFrame->width,
                           pFile);
                for (i = 0; i < mSwsFrame->height; i++)
                    fwrite(mSwsFrame->data[1] + i * mSwsFrame->linesize[1], 1, mSwsFrame->width/2,
                           pFile);
                for (i = 0; i < mSwsFrame->height; i++)
                    fwrite(mSwsFrame->data[2] + i * mSwsFrame->linesize[2], 1, mSwsFrame->width/2,
                           pFile);
//                fwrite(mBufferFrame, 1, numBytesFrame, pFile);
                // Close file
                fclose(pFile);
            }
        }

        av_free(mBufferFrame);
        av_frame_free(&mSwsFrame);
    }

    return true;
}

bool DecoderVideo::decode(void *ptr) {
    AVPacket pPacket;
    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding video");

    while (mRunning) {
        if (mQueue->get(&pPacket, true) < 0) {
            mRunning = false;
            return false;
        }
        if (!process(&pPacket)) {
            //mRunning = false;
            //return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding video ended");

    // Free the RGB image
    av_frame_free(&mFrame);

    return true;
}

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
//int DecoderVideo::getBuffer(struct AVCodecContext *c, AVFrame *pic) {
//    int ret = avcodec_default_get_buffer(c, pic);
//    uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));
//    *pts = global_video_pkt_pts;
//    pic->opaque = pts;
//    return ret;
//}
//void DecoderVideo::releaseBuffer(struct AVCodecContext *c, AVFrame *pic) {
//    if (pic)
//        av_freep(&pic->opaque);
//    avcodec_default_release_buffer(c, pic);
//}


PacketQueue::PacketQueue() {
    pthread_mutex_init(&mLock, NULL);
    pthread_cond_init(&mCondition, NULL);
    mFirst = NULL;
    mLast = NULL;
    mNbPackets = 0;;
    mSize = 0;
    mAbortRequest = false;
}

PacketQueue::~PacketQueue() {
    flush();
    pthread_mutex_destroy(&mLock);
    pthread_cond_destroy(&mCondition);
}

int PacketQueue::size() {
    pthread_mutex_lock(&mLock);
    int size = mNbPackets;
    pthread_mutex_unlock(&mLock);
    return size;
}

void PacketQueue::flush() {
    AVPacketList *pkt, *pkt1;

    pthread_mutex_lock(&mLock);

    for (pkt = mFirst; pkt != NULL; pkt = pkt1) {
        av_free_packet(&pkt->pkt);
        av_freep(&pkt);
        pkt1 = pkt->next;
    }
    mLast = NULL;
    mFirst = NULL;
    mNbPackets = 0;
    mSize = 0;

    pthread_mutex_unlock(&mLock);
}

int PacketQueue::put(AVPacket * pkt) {
    AVPacketList *pkt1;

    /* duplicate the packet */
    if (av_dup_packet(pkt) < 0)
        return -1;

    pkt1 = (AVPacketList *) av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    pthread_mutex_lock(&mLock);

    if (!mLast) {
        mFirst = pkt1;
    }
    else {
        mLast->next = pkt1;
    }

    mLast = pkt1;
    mNbPackets++;
    mSize += pkt1->pkt.size + sizeof(*pkt1);

    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);

    return 0;

}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int PacketQueue::get(AVPacket *pkt, bool block) {
    AVPacketList *pkt1;
    int ret;

    pthread_mutex_lock(&mLock);

    for (; ;) {
        if (mAbortRequest) {
            ret = -1;
            break;
        }

        pkt1 = mFirst;
        if (pkt1) {
            mFirst = pkt1->next;
            if (!mFirst)
                mLast = NULL;
            mNbPackets--;
            mSize -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&mCondition, &mLock);
        }

    }
    pthread_mutex_unlock(&mLock);
    return ret;

}

void PacketQueue::abort() {
    pthread_mutex_lock(&mLock);
    mAbortRequest = true;
    pthread_cond_signal(&mCondition);
    pthread_mutex_unlock(&mLock);
}