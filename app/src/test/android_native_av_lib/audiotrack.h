/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_AUDIOTRACK_WRAPPER_H
#define ANDROID_AUDIOTRACK_WRAPPER_H

#include <stdint.h>
#include <jni.h>


//sync from system/core/include/system/audio.h
/* PCM sub formats */
enum {
    /* All of these are in native byte order */
            ANDROID_AUDIO_FORMAT_PCM_SUB_16_BIT = 0x1, /* DO NOT CHANGE - PCM signed 16 bits */
            ANDROID_AUDIO_FORMAT_PCM_SUB_8_BIT = 0x2, /* DO NOT CHANGE - PCM unsigned 8 bits */
            ANDROID_AUDIO_FORMAT_PCM_SUB_32_BIT = 0x3, /* PCM signed .31 fixed point */
            ANDROID_AUDIO_FORMAT_PCM_SUB_8_24_BIT = 0x4, /* PCM signed 7.24 fixed point */
            ANDROID_AUDIO_FORMAT_PCM_SUB_FLOAT = 0x5, /* PCM single-precision floating point */
            ANDROID_AUDIO_FORMAT_PCM_SUB_24_BIT_PACKED = 0x6, /* PCM signed .23 fixed point packed in 3 bytes */
};

/* Audio format consists of a main format field (upper 8 bits) and a sub format
 * field (lower 24 bits).
 *
 * The main format indicates the main codec type. The sub format field
 * indicates options and parameters for each format. The sub format is mainly
 * used for record to indicate for instance the requested bitrate or profile.
 * It can also be used for certain formats to give informations not present in
 * the encoded audio stream (e.g. octet alignement for AMR).
 */
enum {
    ANDROID_AUDIO_FORMAT_INVALID = 0xFFFFFFFFUL,
    ANDROID_AUDIO_FORMAT_PCM = 0x00000000UL, /* DO NOT CHANGE */
            ANDROID_AUDIO_FORMAT_MP3 = 0x01000000UL,

    /* Aliases */
    /* note != AudioFormat.ENCODING_PCM_16BIT */
            ANDROID_AUDIO_FORMAT_PCM_16_BIT = (ANDROID_AUDIO_FORMAT_PCM |
                                       ANDROID_AUDIO_FORMAT_PCM_SUB_16_BIT),
    /* note != AudioFormat.ENCODING_PCM_8BIT */
            ANDROID_AUDIO_FORMAT_PCM_8_BIT = (ANDROID_AUDIO_FORMAT_PCM |
                                      ANDROID_AUDIO_FORMAT_PCM_SUB_8_BIT),
    ANDROID_AUDIO_FORMAT_PCM_32_BIT = (ANDROID_AUDIO_FORMAT_PCM |
                               ANDROID_AUDIO_FORMAT_PCM_SUB_32_BIT),
    ANDROID_AUDIO_FORMAT_PCM_8_24_BIT = (ANDROID_AUDIO_FORMAT_PCM |
                                 ANDROID_AUDIO_FORMAT_PCM_SUB_8_24_BIT),
    ANDROID_AUDIO_FORMAT_PCM_FLOAT = (ANDROID_AUDIO_FORMAT_PCM |
                              ANDROID_AUDIO_FORMAT_PCM_SUB_FLOAT),
    ANDROID_AUDIO_FORMAT_PCM_24_BIT_PACKED = (ANDROID_AUDIO_FORMAT_PCM |
                                      ANDROID_AUDIO_FORMAT_PCM_SUB_24_BIT_PACKED),
};

/* Audio stream types */
enum {
    /* These values must kept in sync with
     * frameworks/base/media/java/android/media/AudioSystem.java
     */
            ANDROID_AUDIO_STREAM_DEFAULT = -1,
    ANDROID_AUDIO_STREAM_MIN = 0,
    ANDROID_AUDIO_STREAM_VOICE_CALL = 0,
    ANDROID_AUDIO_STREAM_SYSTEM = 1,
    ANDROID_AUDIO_STREAM_RING = 2,
    ANDROID_AUDIO_STREAM_MUSIC = 3,
    ANDROID_AUDIO_STREAM_ALARM = 4,
    ANDROID_AUDIO_STREAM_NOTIFICATION = 5,
    ANDROID_AUDIO_STREAM_BLUETOOTH_SCO = 6,
    ANDROID_AUDIO_STREAM_ENFORCED_AUDIBLE = 7, /* Sounds that cannot be muted by user
                                        * and must be routed to speaker
                                        */
            ANDROID_AUDIO_STREAM_DTMF = 8,
    ANDROID_AUDIO_STREAM_TTS = 9, /* Transmitted Through Speaker.
                                         * Plays over speaker only, silent on other devices.
                                         */
            ANDROID_AUDIO_STREAM_ACCESSIBILITY = 10, /* For accessibility talk back prompts */
            ANDROID_AUDIO_STREAM_REROUTING = 11, /* For dynamic policy output mixes */
            ANDROID_AUDIO_STREAM_PATCH = 12, /* For internal audio flinger tracks. Fixed volume */
            ANDROID_AUDIO_STREAM_PUBLIC_CNT = ANDROID_AUDIO_STREAM_TTS + 1,
    ANDROID_AUDIO_STREAM_CNT = ANDROID_AUDIO_STREAM_PATCH + 1,
};

// Channel mask definitions must be kept in sync with JAVA values in /media/java/android/media/AudioFormat.java
enum {
    // output channels
            ANDROID_AUDIO_CHANNEL_OUT_FRONT_LEFT = 0x1,
    ANDROID_AUDIO_CHANNEL_OUT_FRONT_RIGHT = 0x2,
    ANDROID_AUDIO_CHANNEL_OUT_MONO = ANDROID_AUDIO_CHANNEL_OUT_FRONT_LEFT,
    ANDROID_AUDIO_CHANNEL_OUT_STEREO = (ANDROID_AUDIO_CHANNEL_OUT_FRONT_LEFT |
                                ANDROID_AUDIO_CHANNEL_OUT_FRONT_RIGHT)
};


#define ANDROID_AUDIOTRACK_RESULT_SUCCESS                 0
#define ANDROID_AUDIOTRACK_RESULT_BAD_PARAMETER            -1
#define ANDROID_AUDIOTRACK_RESULT_JNI_EXCEPTION            -2
#define ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED        -3
#define ANDROID_AUDIOTRACK_RESULT_ERRNO                    -4

#ifdef __cplusplus
extern "C" {
#endif

int AndroidAudioTrack_register();

int AndroidAudioTrack_set(int streamType,
                          uint32_t sampleRate,
                          int format,
                          int channels);

int AndroidAudioTrack_start();

int AndroidAudioTrack_flush();

int AndroidAudioTrack_stop();

int AndroidAudioTrack_reload();

int AndroidAudioTrack_unregister();

int AndroidAudioTrack_write(void *buffer, int buffer_size);

#ifdef __cplusplus
}
#endif

#endif
