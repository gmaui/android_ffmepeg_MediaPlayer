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
#include <android/surface.h>  
#include <gui/Surface.h>
#include <utils/Log.h>  
#include <SkBitmap.h>  
#include <SkCanvas.h>  
#include <android/native_window.h>
#include <ui/GraphicBufferMapper.h>
  
#define TAG "AndroidSurfaceWrapper"  

#define ALIGN(x, mask) ( ((x) + (mask) - 1) & ~((mask) - 1) )

static struct {
    jclass clazz;
    jfieldID mNativeObject;
    jfieldID mLock;
    jmethodID ctor;
} gSurfaceClassInfo;

//static struct {
//    jfieldID left;
//    jfieldID top;
//    jfieldID right;
//    jfieldID bottom;
//} gRectClassInfo;

//static struct {
//    jfieldID mSurfaceFormat;
//    jmethodID setNativeBitmap;
//} gCanvasClassInfo;
  
using namespace android;  
  
static sp<Surface> sur = NULL;  
  
static sp<Surface> getNativeSurface(JNIEnv* env, jobject jsurface) {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface IN");  
    if(sur.get() != NULL){ 
        __android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface sur has been generated return directly.");  
		return sur; 
	}
	jclass clazz = env->FindClass("android/view/Surface");
	gSurfaceClassInfo.clazz = jclass(env->NewGlobalRef(clazz));
	gSurfaceClassInfo.mNativeObject = env->GetFieldID(gSurfaceClassInfo.clazz, "mNativeObject", "J");
	gSurfaceClassInfo.mLock = env->GetFieldID(gSurfaceClassInfo.clazz, "mLock", "Ljava/lang/Object;");
	gSurfaceClassInfo.ctor = env->GetMethodID(gSurfaceClassInfo.clazz, "<init>", "(J)V");
    __android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface get the gSurfaceClassInfo");  

    //clazz = env->FindClass("android/graphics/Canvas");
    //gCanvasClassInfo.mSurfaceFormat = env->GetFieldID(clazz, "mSurfaceFormat", "I");
    //gCanvasClassInfo.setNativeBitmap = env->GetMethodID(clazz, "setNativeBitmap", "(J)V");
    //__android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface get the gCanvasClassInfo");  

    //clazz = env->FindClass("android/graphics/Rect");
    //gRectClassInfo.left = env->GetFieldID(clazz, "left", "I");
    //gRectClassInfo.top = env->GetFieldID(clazz, "top", "I");
    //gRectClassInfo.right = env->GetFieldID(clazz, "right", "I");
    //gRectClassInfo.bottom = env->GetFieldID(clazz, "bottom", "I");
    //__android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface get the gRectClassInfo");  
    //jfieldID field_surface = env->GetFieldID(clazz, "mNativeObject", "L");  
    //if(field_surface == NULL) {  
    //    return NULL;  
    //}  
    //return (Surface *) env->GetIntField(jsurface, field_surface);  
	jobject lock = env->GetObjectField(jsurface,gSurfaceClassInfo.mLock);
    __android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface get the lock:%d",lock);  
	if (env->MonitorEnter(lock) == JNI_OK) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface MonitoreEnter ok.");  
		sur = reinterpret_cast<Surface *>(env->GetLongField(jsurface, gSurfaceClassInfo.mNativeObject));
		__android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface MonitoreEnter sur=%p.",sur.get());  
		env->MonitorExit(lock);
	}
	env->DeleteLocalRef(lock);
    __android_log_print(ANDROID_LOG_INFO, TAG, "getNativeSurface OUT");  
	return sur;
}  
  
int AndroidSurface_register(JNIEnv* env, jobject jsurface) {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "registering video surface");  
      
    sur = getNativeSurface(env, jsurface);  
    if(sur.get() == NULL) {  
         __android_log_print(ANDROID_LOG_ERROR, TAG, "registering getNativeSurface failed");  
         return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
    }  

	sp<ANativeWindow> anw(sur.get());  

    if(native_window_api_disconnect(anw.get(),NATIVE_WINDOW_API_MEDIA) != OK)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_api_disconnect failed");
	}

	if(native_window_api_connect(anw.get(),NATIVE_WINDOW_API_MEDIA) != OK)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_api_connect failed");
	}

    if(native_window_set_usage( anw.get(), GRALLOC_USAGE_SW_READ_NEVER 
				| GRALLOC_USAGE_SW_WRITE_OFTEN 
				| GRALLOC_USAGE_HW_TEXTURE 
				| GRALLOC_USAGE_EXTERNAL_DISP) != OK)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_usage failed");
	}

	if(native_window_set_scaling_mode(anw.get(),NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW) != OK)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_scaling_mode failed");
	}

    //sp<ANativeWindow> anw(sur);  
    //int FromSurfaceFlinger = 1;
	//anw->query(anw.get(),NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER,&FromSurfaceFlinger);
	//if (1 == FromSurfaceFlinger) {
		//__android_log_print(ANDROID_LOG_INFO, TAG, "registering FromSurfaceFlinger");  
		//native_window_api_disconnect(anw.get(), NATIVE_WINDOW_API_MEDIA);
		//native_window_api_connect(anw.get(), NATIVE_WINDOW_API_CPU);
	//}
    __android_log_print(ANDROID_LOG_INFO, TAG, "registered");  
      
    return ANDROID_SURFACE_RESULT_SUCCESS;  
}  

int AndroidSurface_writePixels(/*in*/int width, int height, void *py, void *pu, void *pv, int64_t timestamp) {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "pixels(%d %d)(%p %p %p) (%lld)", width, height, py, pu, pv, timestamp);  
    if(sur.get() == NULL) {  
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
    }  
    sp<ANativeWindow> anw(sur.get());  
	int ret = 0;
    
	//Rect dirtyRect;
    //Rect* dirtyRectPtr = NULL;

	int m_width = (width + 1) & ~1;
	int m_height = (height + 1) & ~1;

	if(native_window_set_buffers_dimensions(anw.get(), m_width, m_height) != 0)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_buffers_dimensions failed");
	}

	if(native_window_set_buffers_format(anw.get(), HAL_PIXEL_FORMAT_YV12) != 0)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_buffers_format failed");
	}

	int minBufCount = 0;
	if(anw->query(anw.get(), NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minBufCount) != NO_ERROR)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "query failed");
	}

    __android_log_print(ANDROID_LOG_INFO, TAG, "nativeWindow set bufcount=%d",minBufCount);  
    if(native_window_set_buffer_count(anw.get(), minBufCount+1) != 0)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_buffer_count failed");
	}

    //ANativeWindow_Buffer outBuffer;
	//status_t err = sur->lock(&outBuffer, dirtyRectPtr);
    //if (err < 0) {
    //    __android_log_print(ANDROID_LOG_INFO, TAG, "surface lock failed:%d", err);
    //    return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
	//}
    //__android_log_print(ANDROID_LOG_INFO, TAG, "window(%d-%d-%d-%d-%p)",
		//outBuffer.width, outBuffer.height, outBuffer.stride, outBuffer.format, outBuffer.bits);
    //sur->unlockAndPost();  

	ANativeWindowBuffer *nativeWindowBuffer = NULL;
	if (native_window_dequeue_buffer_and_wait(anw.get(),&nativeWindowBuffer) != 0) {
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
	}

    __android_log_print(ANDROID_LOG_INFO, TAG, "nativeWindwo get GraphicBuffMapper");  
	GraphicBufferMapper &mapper = GraphicBufferMapper::get();

	Rect bounds(m_width, m_height);
	void *dstYUV = NULL;
	if(mapper.lock(nativeWindowBuffer->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dstYUV) != 0)
    {
        __android_log_print(ANDROID_LOG_INFO, TAG, "mapper lock failed");  
	}

	ANativeWindowBuffer *buf = nativeWindowBuffer;
    __android_log_print(ANDROID_LOG_INFO, TAG, "mapper buf(%d %d):(%d)", buf->width, buf->height, buf->stride);  
    //frameworks/av/media/libstagefright/colorconversion/SoftwareRenderer.cpp  ::render
    {
		const uint8_t *src_y = (const uint8_t *)py;
		const uint8_t *src_u = (const uint8_t *)pu;
		const uint8_t *src_v = (const uint8_t *)pv;
		uint8_t *dst_y = (uint8_t *)dstYUV;
		size_t dst_y_size = buf->stride * buf->height;
		size_t dst_c_stride = ALIGN(buf->stride / 2, 16);
		size_t dst_c_size = dst_c_stride * buf->height / 2;
		uint8_t *dst_v = dst_y + dst_y_size;
		uint8_t *dst_u = dst_v + dst_c_size;

		for (int y = 0; y < m_height; ++y)
		{
			memcpy(dst_y, src_y, m_width);
			src_y += width;
			dst_y += buf->stride;
		}

		for (int y = 0; y < (m_height + 1) / 2; ++y)
		{
			memcpy(dst_u, src_v, (m_width + 1) / 2);
			memcpy(dst_v, src_u, (m_width + 1) / 2);
			src_u += width/2; 
			src_v += width/2;
			dst_u += dst_c_stride;
			dst_v += dst_c_stride;
		}
    }

	if(mapper.unlock(nativeWindowBuffer->handle) != 0)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "mapper unlock failed");  
	}

    ret = native_window_set_buffers_timestamp(anw.get(), timestamp);
	if(ret != 0)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_set_buffers_timestamp:%lld", timestamp);  
	}

	ret = anw->queueBuffer(anw.get(), buf, -1);
	if(ret != 0)
	{
		 __android_log_print(ANDROID_LOG_INFO, TAG, "queueBuffer failed with error %s (%d)", strerror(-ret), -ret);
	}

	//ret = anw->cancelBuffer(anw.get(), nativeWindowBuffer, -1);
	//if(ret != 0)
	//{
	//	 __android_log_print(ANDROID_LOG_INFO, TAG, "cancelBuffer failed with error %s (%d)", strerror(-ret), -ret);
	//}
	
	//
	//ANativeWindow* window = ANativeWindow_fromSurface(env, sur);
	//if (!window) {
	//	 __android_log_print(ANDROID_LOG_INFO, TAG, "ANativeWindow_fromSurface failed");
	//	 return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;
	//}

	//ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGB_565);
	//

	//err = sur->lock(&outBuffer, dirtyRectPtr);
    //if (err < 0) {
    //    __android_log_print(ANDROID_LOG_INFO, TAG, "surface lock failed:%d", err);
    //    return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
	//}
    //__android_log_print(ANDROID_LOG_INFO, TAG, "window(%d-%d-%d-%d-%p)",
	//	outBuffer.width, outBuffer.height, outBuffer.stride, outBuffer.format, outBuffer.bits);
    //sur->unlockAndPost();

    __android_log_print(ANDROID_LOG_INFO, TAG, "written down");  
    return ANDROID_SURFACE_RESULT_SUCCESS;  
}  
  
int AndroidSurface_updateSurface() {  
    if(sur.get() == NULL) {  
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
    }  
    if (!Surface::isValid (sur.get())){  
        return ANDROID_SURFACE_RESULT_NOT_VALID;  
    }  
    if (sur->lock(NULL,NULL) < 0) {  
        return ANDROID_SURFACE_RESULT_COULDNT_LOCK;  
    }  
      
    if (sur->unlockAndPost() < 0) {  
        return ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST;  
    }  
    return ANDROID_SURFACE_RESULT_SUCCESS;  
}  
  
int AndroidSurface_unregister() {  
    __android_log_print(ANDROID_LOG_INFO, TAG, "unregistering video surface");  
    if(sur.get() == NULL) {  
        return ANDROID_SURFACE_RESULT_JNI_EXCEPTION;  
    }  
    sp<ANativeWindow> anw(sur.get());  

	if(native_window_api_disconnect(anw.get(),NATIVE_WINDOW_API_MEDIA) != OK)
	{
        __android_log_print(ANDROID_LOG_INFO, TAG, "native_window_api_disconnect failed");
	}
	sur = NULL;
    __android_log_print(ANDROID_LOG_INFO, TAG, "unregistered");  
    return ANDROID_SURFACE_RESULT_SUCCESS;  
} 
