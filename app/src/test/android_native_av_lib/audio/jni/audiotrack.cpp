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
  
#include <android/audiotrack.h>  
#include <utils/Log.h>  
#include <media/AudioTrack.h>  
#include <media/AudioSystem.h>  
#include <utils/Errors.h>  
  
#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#endif

#define TAG "AndroidAudioTrack"  
  
using namespace android;  
  
static AudioTrack*                        nAudioTrack = NULL;  
  
int AndroidAudioTrack_register() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: registering audio track");  
    nAudioTrack = new AudioTrack();  
    if(nAudioTrack == NULL) {  
         __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: registering audio track(null)");  
         return ANDROID_AUDIOTRACK_RESULT_JNI_EXCEPTION;  
    }  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track registered");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_start() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: starting audio track");  
    if(nAudioTrack == NULL) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: staring audio track(null)");  
        return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;  
    }  
    nAudioTrack->start();  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track started");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_set(int streamType,  
                          uint32_t sampleRate,  
                          int format,  
                          int channels) {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: setting audio track");  
    if(nAudioTrack == NULL) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: setting audio track(null)");  
        return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;  
    }  
      
    status_t ret = nAudioTrack->set(static_cast<audio_stream_type_t> (streamType),   
                              sampleRate,   
                              static_cast<audio_format_t> (format),   
                              static_cast<audio_channel_mask_t> (channels));  
      
    if (ret != NO_ERROR) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: audio track set failed");  
        return ANDROID_AUDIOTRACK_RESULT_ERRNO;  
    }  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track set");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_flush() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: flushing audio track");  
    if(nAudioTrack == NULL) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: flushing audio track(null)");  
        return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;  
    }  
    nAudioTrack->flush();  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track flushed");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_stop() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: stopping audio track");  
    if(nAudioTrack == NULL) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: stopping audio track(null)");  
        return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;  
    }  
    nAudioTrack->stop();  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track stopped");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_reload() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: reloading audio track");  
    if(nAudioTrack == NULL) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: reloading audio track(null)");  
        return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;  
    }  
    if(nAudioTrack->reload() != NO_ERROR) {  
        __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: audio track reload failed");  
        return ANDROID_AUDIOTRACK_RESULT_ERRNO;  
    }  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track reloaded");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_unregister() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: unregistering audio track");  
	if(nAudioTrack){
        if(!nAudioTrack->stopped()) {  
            nAudioTrack->stop();  
        }  
		free(nAudioTrack);
        //delete nAudioTrack;
		nAudioTrack = NULL;
	}
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track unregistered");  
    return ANDROID_AUDIOTRACK_RESULT_SUCCESS;  
}  
  
int AndroidAudioTrack_write(void *buffer, int buffer_size) {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: writing audio track");  
    // give the data to the native AudioTrack object (the data starts at the offset)  
    ssize_t written = 0;  
    // regular write() or copy the data to the AudioTrack's shared memory?  
    if (nAudioTrack->sharedBuffer() == 0) {  
        written = nAudioTrack->write(buffer, buffer_size);  
    } else {  
        // writing to shared memory, check for capacity  
        if ((size_t)buffer_size > nAudioTrack->sharedBuffer()->size()) {  
            __android_log_print(ANDROID_LOG_INFO, TAG, "ERR: writing audio track(buffer size was too small)");  
            buffer_size = nAudioTrack->sharedBuffer()->size();  
        }  
        memcpy(nAudioTrack->sharedBuffer()->pointer(), buffer, buffer_size);  
        written = buffer_size;  
    }  
    __android_log_print(ANDROID_LOG_INFO, TAG, "INFO: audio track written");  
    return written;  
}  

