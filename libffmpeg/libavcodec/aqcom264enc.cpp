/**
 * @file
 * H264 encoder for qcom on android ics and up
 * @author carlzhong
 */

#ifndef   UINT64_C
#define   UINT64_C(value)__CONCAT(value,ULL)
#endif


extern "C" {
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"
}
#include <android/log.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <dlfcn.h>

//#include <../../../../../sources/cxx-stl/gnu-libstdc++/4.4.3/include/list>


//using namespace std ;


#include "QcomOmxPublicInterface.h" //QCOM硬件编码

static void 		   *g_encoder = NULL;
static encoderParams   g_params;
static int64_t 		    g_timeStamp;
static sem_t semaphoreFrameGet;
static int frameinDsp = 0;
//int be_close = false;
pthread_mutex_t frame_buf_mutex; 

#define FRAMEBUFSIZE 20
#define MIN(a,b)  ((a)>(b))?(b):(a)

typedef struct {
    int bufsize;
	unsigned char *buf;
}s_framebuf;

typedef struct {
	int readpos;
	int writepos;
	int size;
	s_framebuf framebuf[FRAMEBUFSIZE];
}s_framebuf_list;

static s_framebuf_list g_framebuflist;
unsigned char *picBuf = NULL;

extern "C"  int qcomErr = 0;


#define TEST_FILE
//typedef queue<s_framebuf*> FRAMEBUFLIST; 
#ifdef TEST_FILE
FILE 		 *f = NULL;
#endif

void* pdlhandle = NULL;  
char* pszerror = NULL;
int (*p_send_end_of_input_flag)(void *obj, int timeStampLfile) = NULL;
bool (*p_omx_component_is_available)(const char *hardwareCodecString) = NULL;
int (*p_omx_teardown_input_semaphore)() = NULL;
int   (*p_omx_interface_destroy)(void *obj) = NULL;
void* (*p_encoder_create)(int *errorCode, encoderParams *params) = NULL;
int (*p_omx_setup_input_semaphore)(void *omx) = NULL;
int  (*p_omx_interface_register_output_callback)(void *obj, _QcomOmxOutputCallback function, void *userData) = NULL;
int (*p_omx_send_data_frame_to_encoder)(void *omx, void *frameBytes, int width, int height, long timeStamp) = NULL;
int   (*p_omx_interface_init)(void *obj) = NULL;



int handleOutputEncoded(void *obj, void *buffer, size_t bufferSize, void *iomxBuffer, void *userData) {
    int ret;
	int copysize = 0;
	int status;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "handleOutputEncoded start is %d, %d", bufferSize, frameinDsp);
	if (bufferSize) {
		ret  = pthread_mutex_lock (&frame_buf_mutex);
		
		copysize = MIN(bufferSize,g_params.bitRate);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "handleOutputEncoded is %d ,%d %d", \
		//	                    copysize, g_framebuflist.framebuf[g_framebuflist.writepos].buf, \
		//	                    g_framebuflist.writepos);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "handleOutputEncoded is %d, %d, %d, %d", bufferSize,*((int *)(buffer+4)),g_framebuflist.writepos, \
		//                         g_framebuflist.framebuf[g_framebuflist.writepos].buf);
		if (g_framebuflist.framebuf[g_framebuflist.writepos].buf != NULL) {
		    memcpy(g_framebuflist.framebuf[g_framebuflist.writepos].buf, buffer, copysize);
            g_framebuflist.framebuf[g_framebuflist.writepos].bufsize = copysize;
		}
		g_framebuflist.size++;
		g_framebuflist.writepos++;
		g_framebuflist.writepos = g_framebuflist.writepos%FRAMEBUFSIZE;
		frameinDsp --; 
#if 0
        f = fopen("/sdcard/1.264", "ab+");
        if(f!=NULL)
		   fwrite(buffer, 1, bufferSize, f);
		fclose(f);
		f = NULL;
#endif		
		ret = pthread_mutex_unlock (&frame_buf_mutex);
	}
	return 0;
}

int deinit()
{
   if(pdlhandle) {
   	  dlclose(pdlhandle);
      pdlhandle = NULL;
   }
   p_send_end_of_input_flag = NULL;
   p_omx_component_is_available = NULL;
   p_omx_teardown_input_semaphore = NULL;
   p_omx_interface_destroy = NULL;
   p_encoder_create = NULL;
   p_omx_setup_input_semaphore = NULL;
   p_omx_interface_register_output_callback = NULL;
   p_omx_send_data_frame_to_encoder = NULL;
   p_omx_interface_init = NULL;
}

extern "C" av_cold int QCOM_H264_encodeIsAvailable()
{
   return 0;
   static bool isAvailable = 0;
   char err[100] = {0};
   //deinit();
   if(NULL != pdlhandle) 
   	  return isAvailable;
   else
      pdlhandle = dlopen("/data/data/com.meitu/lib/libqcomomxsample_ics.so", RTLD_NOW);
   //if(NULL == pdlhandle)
   	 // pdlhandle = dlopen("/data/data/com.meitu/lib/libqcomomxsample_gb.so", RTLD_NOW);
   #undef sprintf
   sprintf(err,"%s",dlerror());
   //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_encodeIsAvailable is %d, %s", pdlhandle,err);
   if(pdlhandle != NULL) {
		p_omx_component_is_available =p_omx_component_is_available?p_omx_component_is_available:(bool (*)(const char *hardwareCodecString))dlsym(pdlhandle, "omx_component_is_available");
        p_send_end_of_input_flag = p_send_end_of_input_flag?p_send_end_of_input_flag:(int (*)(void *obj, int timeStampLfile))dlsym(pdlhandle, "omx_interface_send_end_of_input_flag");
		//p_omx_component_is_available = (bool (*)(const char *hardwareCodecString))dlsym(pdlhandle, "omx_component_is_available");
        p_omx_teardown_input_semaphore = p_omx_teardown_input_semaphore?p_omx_teardown_input_semaphore:(int (*)())dlsym(pdlhandle, "omx_teardown_input_semaphore");
		p_omx_interface_destroy = p_omx_interface_destroy?p_omx_interface_destroy:(int (*)(void *obj))dlsym(pdlhandle, "omx_interface_destroy");
		p_encoder_create = p_encoder_create?p_encoder_create:(void* (*)(int *errorCode, encoderParams *params))dlsym(pdlhandle, "encoder_create");
        p_omx_setup_input_semaphore = p_omx_setup_input_semaphore?p_omx_setup_input_semaphore:(int (*)(void *omx))dlsym(pdlhandle, "omx_setup_input_semaphore");
		p_omx_interface_register_output_callback = p_omx_interface_register_output_callback?p_omx_interface_register_output_callback:(int  (*)(void *obj, _QcomOmxOutputCallback function, void *userData))dlsym(pdlhandle, "omx_interface_register_output_callback");
        p_omx_send_data_frame_to_encoder = p_omx_send_data_frame_to_encoder?p_omx_send_data_frame_to_encoder:(int (*)(void *omx, void *frameBytes, int width, int height, long timeStamp))dlsym(pdlhandle, "omx_send_data_frame_to_encoder");
        p_omx_interface_init = p_omx_interface_init?p_omx_interface_init:(int (*)(void *obj))dlsym(pdlhandle, "omx_interface_init");
		
		if(!(p_omx_component_is_available && p_send_end_of_input_flag && p_omx_teardown_input_semaphore \
			  && p_omx_interface_destroy && p_encoder_create && p_omx_setup_input_semaphore \
			    && p_omx_interface_register_output_callback && p_omx_send_data_frame_to_encoder && p_omx_interface_init))
			     return 0;
   } else 
        return 0;
   
   if(p_omx_component_is_available)
      isAvailable = p_omx_component_is_available("OMX.qcom.video.encoder.avc");//carlzhong ("OMX.qcom.video.encoder.avc");
   //__android_log_print(ANDROID_LOG_INFO, "tt", "isAvailable is %d", isAvailable);
   return isAvailable ? 1 : 0;
}

#if 1
extern "C" av_cold int QCOM_H264_closeEncode()
{
	pthread_mutex_lock (&frame_buf_mutex);
	
	g_framebuflist.readpos = 0;
	g_framebuflist.size = 0;
	g_framebuflist.writepos = 0;
	for(int i = 0; i < FRAMEBUFSIZE; i++) {
		//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close is %d", g_framebuflist.framebuf[i].buf);
		if(g_framebuflist.framebuf[i].buf) {
			av_freep(&(g_framebuflist.framebuf[i].buf));
     		g_framebuflist.framebuf[i].bufsize = 0;
		}
	}
	
	if(picBuf != NULL){
		av_freep(&picBuf);
		picBuf = NULL;
	}
	
	pthread_mutex_unlock (&frame_buf_mutex);
	
	pthread_mutex_destroy(&frame_buf_mutex);
	
	
	//sem_destroy(&semaphoreFrameGet);
	return 0;

}
#endif


static 
av_cold int QCOM_H264_init(AVCodecContext *avctx) 
{
	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_init");

    if(avctx == NULL) return -1;

	//like x264
	//avctx->has_b_frames = avctx->max_b_frames = 0;
	//avctx->bit_rate = 600*1000;
	//avctx->crf = 25.0;
	//deinit();
    //int  (*bar)(int  x) = NULL;
	//pdlhandle = dlopen("/data/data/com.yb.demotest/lib/libqcomomxsample_ics.so", RTLD_NOW);
	//pdlhandle = dlopen("/sdcard/libbar.so", RTLD_NOW);

	//pszerror = (char *)dlerror();
	//if(pdlhandle != NULL) {

	//}
	//int tpp = bar(10);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "SS %d, %d, %s",pdlhandle,p_send_end_of_input_flag,g_encoder);
	if(g_encoder != NULL){
		int status = 0;
		do {
		  if (p_send_end_of_input_flag)
		      status = p_send_end_of_input_flag(g_encoder, g_timeStamp);
		  else 
		      break;
		  //omx_interface_send_final_buffer(g_encoder, NULL, g_timeStamp);
		} while (status != 0);
		
		
		//be_close = true;
		if (p_omx_teardown_input_semaphore)
		    p_omx_teardown_input_semaphore();
		//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_init close");
		
		//status = omx_interface_deinit(g_encoder);
		if (status == 0) {
			//__android_log_print(ANDROID_LOG_ERROR,"tt", "QCOM_H264_init DESTROY");
			if(p_omx_interface_destroy)
			   status = p_omx_interface_destroy(g_encoder);
		}

	}
		
	g_encoder = NULL;
	g_timeStamp = 0;

	g_params.frameWidth = avctx->width;
	g_params.frameHeight = avctx->height;
	g_params.frameRate = 60;
	g_params.rateControl = 3;
	g_params.bitRate = 640*480*5;//avctx->width * avctx->height * 6;
	g_params.codecString = NULL;
	frameinDsp = 0;

	g_framebuflist.readpos = 0;
	g_framebuflist.size = 0;
	g_framebuflist.writepos = 0;
	for(int i = 0; i < FRAMEBUFSIZE; i++) {
	  if(g_framebuflist.framebuf[i].buf == NULL)
         g_framebuflist.framebuf[i].buf = (unsigned char*)av_mallocz(g_params.bitRate);
	  g_framebuflist.framebuf[i].bufsize = 0;
	  //if(g_framebuflist.buf[i] == NULL)
	}
	if(picBuf == NULL)
		picBuf = (unsigned char*)av_mallocz(g_params.frameWidth*g_params.frameHeight*3/2);

	//sem_init(&semaphoreFrameGet, 0, 0);
	pthread_mutex_init (&frame_buf_mutex, NULL);
	__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_init g_encoder1 is %d,%d",g_params.codecString,g_encoder);

	int status = 0;
	if(p_encoder_create)
	   g_encoder = p_encoder_create(&status, &g_params);
	__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_init g_encoder is %d,%d",g_params.codecString,g_encoder);
    if (g_encoder == NULL) {
		QCOM_H264_closeEncode();
		qcomErr = 1;
		return -1;
    }
	if(p_omx_setup_input_semaphore)
       p_omx_setup_input_semaphore(g_encoder);
    if(p_omx_interface_register_output_callback)
       p_omx_interface_register_output_callback(g_encoder, handleOutputEncoded, NULL);
    if(p_omx_interface_init)
       p_omx_interface_init(g_encoder);
	
	__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_init1 is %d", avctx->flags & CODEC_FLAG_GLOBAL_HEADER);


#ifdef TEST_FILE
	//f = fopen("/sdcard/1.264", "wb");
#endif

    return 0;
}

/*****************************************
 单帧的编码函数
ctx: ffmpeg全局信息结构体
buf: 输出码流内存
orig_bufsize: buf的大小
data: 存储图像原始数据的内存块
****************************************/
static 
int QCOM_H264_frame(AVCodecContext *ctx, uint8_t *buf,
                      int orig_bufsize, void *data)
{
	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame start");

    //return 0;	
	AVFrame *frame = (AVFrame *)data;
	int ret;
	int copysize = 0;
	static unsigned char spsandpps[100]={0};
	static int ppsandppsize = 0;
	//if(!p_omx_send_data_frame_to_encoder)
	   //return 0;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame start is %d,%d", frame, frame->data[0]);
    if (frame && frame->data[0]) {
		if(picBuf == NULL)
			picBuf = (unsigned char*)av_mallocz(g_params.frameWidth*g_params.frameHeight*3/2);

		//临时拷
		unsigned char* picY = picBuf;
		unsigned char* picU = picBuf + g_params.frameWidth*g_params.frameHeight;
		unsigned char* picV = picBuf + g_params.frameWidth*g_params.frameHeight*5/4;
		int vuWidth = g_params.frameWidth/2;
		int vuheight = g_params.frameHeight/2;
		int vuWidthDoub = vuWidth*2;
		int vuheightDoub = vuheight*2;
		for(int i = 0; i<g_params.frameHeight; i++)
		   	memcpy(picY + g_params.frameWidth*i, (unsigned char*)frame->data[0]+frame->linesize[0]*i,g_params.frameWidth);
 #if 1       //yv12 -> nv12
 //         if(1 || frame->linesize[0] != frame->linesize[2]) {
			for(int i = 0; i < vuheight; i++) {
	            for(int j = 0; j < vuWidth; j++) {
	                *(picU + vuWidthDoub*i + j*2) = *((unsigned char*)(frame->data[1]+frame->linesize[1]*i + j));
					*(picU + vuWidthDoub*i + j*2 + 1) = *((unsigned char*)(frame->data[2]+frame->linesize[2]*i + j));
				}
			}
#else
//        }else{ //direct nv12 copy
			for(int i = 0; i < vuheight; i++) {
	            memcpy(picU + i*g_params.frameWidth,frame->data[1]+frame->linesize[1]*i,g_params.frameWidth);
			}
//        }
#endif


		//memcpy(picU + (g_params.frameWidth/2)*i, (unsigned char*)frame->data[1]+frame->linesize[1]*i,g_params.frameWidth/2);
		//for(int i = 0; i<g_params.frameHeight/2; i++)
		  //  memcpy(picV + (g_params.frameWidth/2)*i, (unsigned char*)frame->data[2]+frame->linesize[2]*i,g_params.frameWidth/2);
        frameinDsp ++;
		int tmp = p_omx_send_data_frame_to_encoder(g_encoder, (unsigned char*)picBuf,
                               g_params.frameWidth, g_params.frameHeight, g_timeStamp);
		
		g_timeStamp += 1000000/30;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame is %d,%d, %d, %d, %d", tmp,g_params.frameWidth, \
			//                          g_params.frameHeight,  frame->linesize[0], frame->linesize[1]);
#if 0
        f = fopen("/sdcard/1.yuv", "ab+");
        if(f!=NULL) {
		   for(int i = 0; i<g_params.frameHeight; i++)
		   	   fwrite((unsigned char*)frame->data[0]+frame->linesize[0]*i, 1, g_params.frameWidth, f);
		   for(int i = 0; i<g_params.frameHeight/2; i++)
		       fwrite((unsigned char*)frame->data[1]+frame->linesize[1]*i, 1, g_params.frameWidth/2, f);
		   for(int i = 0; i<g_params.frameHeight/2; i++)
		       fwrite((unsigned char*)frame->data[2]+frame->linesize[2]*i, 1, g_params.frameWidth/2, f);
		}
		fclose(f);
		f = NULL;
#endif

	}

	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame start");

	ret  = pthread_mutex_lock (&frame_buf_mutex);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame is %d", ctx->pix_fmt);
	if(g_framebuflist.size > 0) {
		if (g_framebuflist.framebuf[g_framebuflist.readpos].buf != NULL) {
			
			copysize = MIN(g_params.bitRate,g_framebuflist.framebuf[g_framebuflist.readpos].bufsize);
			
            unsigned char* tmp = g_framebuflist.framebuf[g_framebuflist.readpos].buf;
			memcpy(buf, g_framebuflist.framebuf[g_framebuflist.readpos].buf, copysize);
			__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame is %d", g_framebuflist.size);
			if (ctx->flags & CODEC_FLAG_GLOBAL_HEADER && (tmp[0]==0&&tmp[1]==0&&tmp[2]==0&&tmp[3]==1&&tmp[4]==0x67)) {
                av_freep(&ctx->extradata);
				ctx->extradata      = ( uint8_t *)av_malloc(copysize);
                ctx->extradata_size = copysize;//encode_nals(avctx, avctx->extradata, s, nal, nnal, 1);  
                memcpy(ctx->extradata,g_framebuflist.framebuf[g_framebuflist.readpos].buf,copysize);
			}
			g_framebuflist.size--;
			g_framebuflist.readpos++;
			g_framebuflist.readpos = g_framebuflist.readpos%FRAMEBUFSIZE;

#if 0
	        f = fopen("/sdcard/2.264", "ab+");
	        if(f!=NULL)
			   fwrite(buf, 1, copysize, f);
			fclose(f);
			f = NULL;
#endif	
		}

	}
	ctx->coded_frame = frame;
	//frame->pts = g_timeStamp;
	ret = pthread_mutex_unlock (&frame_buf_mutex);

	//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_frame is %d, %d ,%d", copysize, frameinDsp, g_framebuflist.size);
    
    return copysize;//copysize;
}

static 
av_cold int QCOM_H264_close(AVCodecContext *avctx)
{
	  // __android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close1");
      av_freep(&avctx->extradata);
	  int status = 0;
	  do {
	  	if(p_send_end_of_input_flag)
		   status = p_send_end_of_input_flag(g_encoder, g_timeStamp);
		else
		    break;
	  } while (status != 0);

	  if(p_omx_teardown_input_semaphore)
         p_omx_teardown_input_semaphore();
      //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close");
  
      //status = omx_interface_deinit(g_encoder);
      if (status == 0) {
	      //__android_log_print(ANDROID_LOG_ERROR,"tt", "ENCODER DESTROY");
		  if(p_omx_interface_destroy)
	         status = p_omx_interface_destroy(g_encoder);
      }
      g_encoder = NULL;
	  
	  //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close33");
       
	  pthread_mutex_lock (&frame_buf_mutex);

	  g_framebuflist.readpos = 0;
	  g_framebuflist.size = 0;
	  g_framebuflist.writepos = 0;
	  for(int i = 0; i < FRAMEBUFSIZE; i++) {
	  	  //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close is %d", g_framebuflist.framebuf[i].buf);
	  	  if(g_framebuflist.framebuf[i].buf)
	  	      av_freep(&(g_framebuflist.framebuf[i].buf));
		  g_framebuflist.framebuf[i].bufsize = 0;
	  }

	  if(picBuf != NULL){
		  av_freep(&picBuf);
		  picBuf = NULL;
	  }

	  pthread_mutex_unlock (&frame_buf_mutex);

	  //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close22");
	  pthread_mutex_destroy(&frame_buf_mutex);

	  //if(pdlhandle) {
		// dlclose(pdlhandle);
		 //pdlhandle = NULL;
	  //}

	  //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close");

	  //sem_destroy(&semaphoreFrameGet);
	  return 0;
}



static const enum PixelFormat pix_fmts_8bit_nv[] = {
	PIX_FMT_YUV420P,
    PIX_FMT_NV12
};

AVCodec ff_qcom264_encoder = {"qcom264enc", AVMEDIA_TYPE_VIDEO, CODEC_ID_H264, 0, QCOM_H264_init, QCOM_H264_frame, QCOM_H264_close
};


