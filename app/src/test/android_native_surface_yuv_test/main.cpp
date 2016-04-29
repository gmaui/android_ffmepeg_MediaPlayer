/*************************************************************************
    > File Name: main.cpp
    > Author: cao.bing
    > Mail: cao.bing@whaley.cn 
    > Created Time: Thu Apr 28 21:49:13 2016
 ************************************************************************/

#include <cutils/memory.h>  
  
#include <unistd.h>  
#include <utils/Log.h>  
  
#include <binder/IPCThreadState.h>  
#include <binder/ProcessState.h>  
#include <binder/IServiceManager.h>  
#include <media/stagefright/foundation/ADebug.h>  
#include <gui/Surface.h>  
#include <gui/SurfaceComposerClient.h>  
#include <gui/ISurfaceComposer.h>  
#include <ui/DisplayInfo.h>  
#include <android/native_window.h>  
#include <system/window.h>  
#include <ui/GraphicBufferMapper.h>  
//ANativeWindow 就是surface，对应surface.cpp里的code  
using namespace android;  
  
//将x规整为y的倍数,也就是将x按y对齐  
static int ALIGN(int x, int y) {  
    // y must be a power of 2.  
    return (x + y - 1) & ~(y - 1);  
}  
  
void render(  
        const void *data, size_t size, const sp<ANativeWindow> &nativeWindow,int width,int height) {  
    sp<ANativeWindow> mNativeWindow = nativeWindow;  
    int err;  
    int mCropWidth = width;  
    int mCropHeight = height;  
      
    int halFormat = HAL_PIXEL_FORMAT_YV12;//颜色空间  
    int bufWidth = (mCropWidth + 1) & ~1;//按2对齐  
    int bufHeight = (mCropHeight + 1) & ~1;  
      
    CHECK_EQ(0,  
            native_window_set_usage(  
            mNativeWindow.get(),  
            GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN  
            | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));  
  
    CHECK_EQ(0,  
            native_window_set_scaling_mode(  
            mNativeWindow.get(),  
            NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));  
  
    // Width must be multiple of 32???  
    //很重要,配置宽高和和指定颜色空间yuv420  
    //如果这里不配置好，下面deque_buffer只能去申请一个默认宽高的图形缓冲区  
    CHECK_EQ(0, native_window_set_buffers_geometry(  
                mNativeWindow.get(),  
                bufWidth,  
                bufHeight,  
                halFormat));  
      
      
    ANativeWindowBuffer *buf;//描述buffer  
    //申请一块空闲的图形缓冲区  
    if ((err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(),  
            &buf)) != 0) {  
        ALOGW("Surface::dequeueBuffer returned error %d", err);  
        return;  
    }  
  
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();  
  
    Rect bounds(mCropWidth, mCropHeight);  
  
    void *dst;  
    CHECK_EQ(0, mapper.lock(//用来锁定一个图形缓冲区并将缓冲区映射到用户进程  
                buf->handle, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst));//dst就指向图形缓冲区首地址  

    printf("[%s][%d] bound(%d-%d) buf(%d-%d-%d)\n",__FILE__,__LINE__, mCropWidth, mCropHeight, buf->width, buf->height, buf->stride);  
  
    if (true){  
        size_t dst_y_size = buf->stride * buf->height;  
        size_t dst_c_stride = ALIGN(buf->stride / 2, 16);//1行v/u的大小  
        size_t dst_c_size = dst_c_stride * buf->height / 2;//u/v的大小  
          
        //memcpy(dst, data, dst_y_size + dst_c_size*2);//将yuv数据copy到图形缓冲区  

        //FILE *fp = fopen("/mnt/usb/F6B9-6B49/video/out","wb+");  
		printf("GGmaui hit\r\n");

		//if(fp!=NULL)
		//{
		//	fwrite(data,dst_y_size + dst_c_size*2,1,fp);
		//	fclose(fp);
		//}

		uint8_t *dst_y = (uint8_t *)dst;
		uint8_t *dst_v = dst_y + dst_y_size;
		uint8_t *dst_u = dst_v + dst_c_size;
		uint8_t *src_y = (uint8_t*)data;
		uint8_t *src_u = (uint8_t*)data+width*height;
		uint8_t *src_v = src_u + width/2*height/2;

		for (int y = 0; y < mCropHeight; ++y)
		{
			memcpy(dst_y, src_y, mCropWidth);
			src_y += width;
			dst_y += buf->stride;
		}

		for (int y = 0; y < (mCropHeight + 1) / 2; ++y)
		{
			memcpy(dst_u, src_v, (mCropWidth + 1) / 2);
			memcpy(dst_v, src_u, (mCropWidth + 1) / 2);
			src_u += width/2;
			src_v += width/2;
			dst_u += dst_c_stride;
			dst_v += dst_c_stride;
		}
		//memset(dst, 0x00, dst_y_size/4);//将yuv数据copy到图形缓冲区  
		//memset(dst+dst_y_size/4, 0xFF, dst_y_size/4);//将yuv数据copy到图形缓冲区  
		//memset(dst+dst_y_size/2, 0x00, dst_y_size/4);//将yuv数据copy到图形缓冲区  
		//memset(dst+dst_y_size*3/4, 0xFF, dst_y_size/4);//将yuv数据copy到图形缓冲区  
		//memset(dst+dst_y_size, 0x00, dst_y_size/2);//将yuv数据copy到图形缓冲区  
	}  

	CHECK_EQ(0, mapper.unlock(buf->handle));  

	if ((err = mNativeWindow->queueBuffer(mNativeWindow.get(), buf,  
					-1)) != 0) {  
		ALOGW("Surface::queueBuffer returned error %d", err);  
	}  
    buf = NULL;  
}  
  
bool getYV12Data(const char *path,unsigned char * pYUVData,int size){  
    FILE *fp = fopen(path,"rb");  
    if(fp == NULL){  
        printf("read %s fail !!!!!!!!!!!!!!!!!!!\n",path);  
        return false;  
    }  
    fread(pYUVData,size,1,fp);  
    fclose(fp);  
    return true;  
}  
  
int main(int argc, char **argv){  
    // set up the thread-pool  
    sp<ProcessState> proc(ProcessState::self());  
    ProcessState::self()->startThreadPool();  
      
    // create a client to surfaceflinger  
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();  
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(  
            ISurfaceComposer::eDisplayIdMain));  
    DisplayInfo dinfo;  
    //获取屏幕的宽高等信息  
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);  
    printf("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n",   
        dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);  
    if (status)  
        return -1;  
    //创建surface  
    sp<SurfaceControl> surfaceControl = client->createSurface(String8("testsurface"),  
            dinfo.w, dinfo.h, PIXEL_FORMAT_RGBA_8888, 0);  
              
/*************************get yuv data from file;****************************************/            
	if(argc < 4)
	{
		printf("Usage: %s file width height\r\n", argv[0]);
		return 0;
	}

    printf("[%s][%d]\n",__FILE__,__LINE__);  
    int width,height;  
    width = atoi(argv[2]);  
    height = atoi(argv[3]);  
    int size = width * height * 3/2;  
    unsigned char *data = new unsigned char[size];  

    char path[128] ={0};  
	memcpy(path, argv[1], strlen(argv[1]));
    printf("[%s][%d]path=%s width=%d height=%d\r\n",__FILE__,__LINE__,path,width,height);  
    getYV12Data(path,data,size);//get yuv data from file;  
      
/*********************配置surface*******************************************************************/  
    SurfaceComposerClient::openGlobalTransaction();  
    surfaceControl->setLayer(100000);//设定Z坐标  
    surfaceControl->setPosition(100, 100);//以左上角为(0,0)设定显示位置  
    surfaceControl->setSize(width, height);//设定视频显示大小  
    SurfaceComposerClient::closeGlobalTransaction();  
    sp<Surface> surface = surfaceControl->getSurface();  
    printf("[%s][%d]\n",__FILE__,__LINE__);  
      
/**********************显示yuv数据******************************************************************/     
    render(data,size,surface,width,height);  
    printf("[%s][%d]\n",__FILE__,__LINE__);  
      
    IPCThreadState::self()->joinThreadPool();//可以保证画面一直显示，否则瞬间消失  
    IPCThreadState::self()->stopProcess();  
    return 0;  
}
