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



#include "QcomOmxPublicInterface.h" 

#if BUILD_FOR_HONEYCOMB_AND_UP
//#include <ColorConverter.h>
#endif

static sem_t semaphoreBufferIsAvailable;


void* pdlhandle_d = NULL; 
void *(*p_decoder_create_d)(int *errorCode, const char *codecString) = NULL;
bool (*p_omx_component_is_available_d)(const char *hardwareCodecString) = NULL;
int  (*p_omx_interface_register_output_callback_d)(void *obj, _QcomOmxOutputCallback function, void *userData) = NULL;
int  (*p_omx_interface_register_input_callback_d)(void *obj, _QcomOmxInputCallback function, void *userData) = NULL;
int   (*p_omx_interface_init_d)(void *obj) = NULL;
int   (*p_omx_interface_send_input_data_d)(void *obj, void *buffer, size_t size, int timeStampLfile) = NULL;
int   (*p_omx_interface_send_end_of_input_flag_d)(void *obj, int timeStampLfile) = NULL;
int   (*p_omx_interface_deinit_d)(void *obj) = NULL;
int   (*p_omx_interface_destroy_d)(void *obj) = NULL;
int   (*p_getHardwareBaseVersion_d)() = NULL;

//similar to qcom enc
pthread_mutex_t frame_buf_mutex_d; 

#define FRAMEBUFSIZE 20
#define MIN(a,b)  ((a)>(b))?(b):(a)

typedef struct {
    int bufsize;
	unsigned char *buf;
}s_framebuf_d;

typedef struct {
	int readpos;
	int writepos;
	int size;
	s_framebuf_d framebuf[FRAMEBUFSIZE];
}s_framebuf_list_d;

s_framebuf_list_d g_framebuflist_d;

//

#define AV_RB16(x)                           \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])

#define TILE_ALIGN( num, to ) (((num) + (to-1)) & (~(to-1)))

static const size_t BLOCK_WIDTH = 64;
static const size_t BLOCK_HEIGHT = 32;
static const size_t BLOCK_SIZE = BLOCK_WIDTH*BLOCK_HEIGHT;
static const size_t BLOCK_GROUP_SIZE = BLOCK_SIZE*4;

void *omxDecoder = NULL;
long timeStampLfile = 0;
int firsttime = 1;
int count = 0;
int g_hwVersion = -1;
int width = 640;
int height = 480;



size_t TiledFrameSize(int frameWidth, int frameHeight) {
	size_t abx = (frameWidth - 1)/BLOCK_WIDTH + 1;
	size_t nbx = (abx + 1) & ~1;
	size_t nbyLuma = (frameHeight - 1)/BLOCK_HEIGHT + 1;
	size_t nbyChroma = (frameHeight/2 - 1)/BLOCK_HEIGHT + 1;
	size_t lumaSize = nbx*nbyLuma*BLOCK_SIZE;

	if((lumaSize % BLOCK_GROUP_SIZE) != 0) {
		lumaSize = (((lumaSize-1)/BLOCK_GROUP_SIZE)+1)*BLOCK_GROUP_SIZE;
	}

	size_t chromaSize = nbx*nbyChroma*BLOCK_SIZE;

	if((chromaSize % BLOCK_GROUP_SIZE) != 0) {
		chromaSize = (((chromaSize -1)/BLOCK_GROUP_SIZE)+1)*BLOCK_GROUP_SIZE;
	}
	return lumaSize + chromaSize;
}


extern "C" av_cold int QCOM_H264_decodeIsAvailable()
{
   bool isAvailable = 0;
   //deinit();
   if(NULL == pdlhandle_d)
      pdlhandle_d = dlopen("/data/data/com.yb.demotest/lib/libqcomomxsample_ics.so", RTLD_NOW);
   //__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_encodeIsAvailable is %d", pdlhandle);
   if(pdlhandle_d != NULL) {
	  p_omx_component_is_available_d =p_omx_component_is_available_d?p_omx_component_is_available_d:(bool (*)(const char *hardwareCodecString))dlsym(pdlhandle_d, "omx_component_is_available");
      p_omx_interface_send_end_of_input_flag_d = p_omx_interface_send_end_of_input_flag_d?p_omx_interface_send_end_of_input_flag_d:(int (*)(void *obj, int timeStampLfile))dlsym(pdlhandle_d, "omx_interface_send_end_of_input_flag");
      p_omx_interface_register_input_callback_d = p_omx_interface_register_input_callback_d?p_omx_interface_register_input_callback_d:(int (*)(void *obj, _QcomOmxInputCallback function, void *userData))dlsym(pdlhandle_d, "omx_interface_register_input_callback");
	  p_omx_interface_destroy_d = p_omx_interface_destroy_d?p_omx_interface_destroy_d:(int (*)(void *obj))dlsym(pdlhandle_d, "omx_interface_destroy");
	  p_decoder_create_d = p_decoder_create_d?p_decoder_create_d:(void* (*)(int *errorCode, const char *codecString))dlsym(pdlhandle_d, "decoder_create");
      p_omx_interface_send_input_data_d = p_omx_interface_send_input_data_d?p_omx_interface_send_input_data_d:(int (*)(void *obj, void *buffer, size_t size, int timeStampLfile))dlsym(pdlhandle_d, "omx_interface_send_input_data");
	  p_omx_interface_register_output_callback_d = p_omx_interface_register_output_callback_d?p_omx_interface_register_output_callback_d:(int  (*)(void *obj, _QcomOmxOutputCallback function, void *userData))dlsym(pdlhandle_d, "omx_interface_register_output_callback");
      p_omx_interface_deinit_d = p_omx_interface_deinit_d?p_omx_interface_deinit_d:(int (*)(void *obj))dlsym(pdlhandle_d, "omx_interface_deinit");
      p_omx_interface_init_d = p_omx_interface_init_d?p_omx_interface_init_d:(int (*)(void *obj))dlsym(pdlhandle_d, "omx_interface_init");
	  p_getHardwareBaseVersion_d = p_getHardwareBaseVersion_d?p_getHardwareBaseVersion_d:(int (*)())dlsym(pdlhandle_d, "getHardwareBaseVersion");
	  
	  if(!(p_omx_component_is_available_d && p_omx_interface_send_end_of_input_flag_d && p_omx_interface_register_input_callback_d \
		  && p_omx_interface_destroy_d && p_decoder_create_d && p_omx_interface_send_input_data_d \
		    && p_omx_interface_register_output_callback_d && p_omx_interface_deinit_d && p_omx_interface_init_d && p_getHardwareBaseVersion_d))
		     return 0;
   } else 
      return 0;
   
   if(p_omx_component_is_available_d)
      isAvailable = p_omx_component_is_available_d("OMX.qcom.video.decoder.avc");//carlzhong ("OMX.qcom.video.encoder.avc");
   //__android_log_print(ANDROID_LOG_INFO, "tt", "isAvailable is %d", isAvailable);
   return isAvailable ? 1 : 0;
}


int semaphoreEmptyBufferPost(void *obj, void *userData) 
{
	sem_post(&semaphoreBufferIsAvailable);
	count--;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "semaphoreEmptyBufferPost is %d", count);
	return 0;
}


/* get frame tile coordinate. XXX: nothing to be understood here, don't try. */
static 
size_t tile_pos(size_t x, size_t y, size_t w, size_t h)
{
    size_t flim = x + (y & ~1) * w;

    if (y & 1) {
        flim += (x & ~3) + 2;
    } else if ((h & 1) == 0 || y != (h - 1)) {
        flim += (x + 2) & ~3;
    }

    return flim;
}

//QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka to nv12??
int  qcom_convert(uint8_t *src, uint8_t *dst_y, uint8_t *dst_uv, int width, int height, int pitch)
{
    //size_t width = pic->format.i_width;
    //size_t pitch = pic->p[0].i_pitch;
    //size_t height = pic->format.i_height;
    int ret = width*height*1.5;
    const size_t tile_w = (width - 1) / BLOCK_WIDTH + 1;
    const size_t tile_w_align = (tile_w + 1) & ~1;

    const size_t tile_h_luma = (height - 1) / BLOCK_HEIGHT + 1;
    const size_t tile_h_chroma = (height / 2 - 1) / BLOCK_HEIGHT + 1;
	size_t luma_size = tile_w_align * tile_h_luma * BLOCK_SIZE;

	if((luma_size % BLOCK_GROUP_SIZE) != 0)
		luma_size = (((luma_size - 1) / BLOCK_GROUP_SIZE) + 1) * BLOCK_GROUP_SIZE;

	for(size_t y = 0; y < tile_h_luma; y++) {
	 	size_t row_width = width;
        for(size_t x = 0; x < tile_w; x++) {
			const uint8_t *src_luma  = src + tile_pos(x, y,tile_w_align, tile_h_luma) * BLOCK_SIZE;
			const uint8_t *src_chroma = src + luma_size + tile_pos(x, y/2, tile_w_align, tile_h_chroma) * BLOCK_SIZE;

			if(y & 1)
               src_chroma += BLOCK_SIZE/2;

			size_t tile_width = row_width;
            if(tile_width > BLOCK_WIDTH)
               tile_width = BLOCK_WIDTH;

			size_t tile_height = height;
            if(tile_height > BLOCK_HEIGHT)
               tile_height = BLOCK_HEIGHT;

			size_t luma_idx = y * BLOCK_HEIGHT * pitch + x * BLOCK_WIDTH;
			size_t chroma_idx = (luma_idx / pitch) * pitch/2 + (luma_idx % pitch);

			tile_height /= 2; //
			while(tile_height--) {
                  memcpy(&dst_y[luma_idx], src_luma, tile_width);
                  src_luma += BLOCK_WIDTH;
                  luma_idx += pitch;

                  memcpy(&dst_y[luma_idx], src_luma, tile_width);
                  src_luma += BLOCK_WIDTH;
                  luma_idx += pitch;

                  memcpy(&dst_uv[chroma_idx], src_chroma, tile_width);
                  src_chroma += BLOCK_WIDTH;
                  chroma_idx += pitch;
            }
            row_width -= BLOCK_WIDTH;
		}
		height -= BLOCK_HEIGHT;

	}

	return ret;
}

int handleOutputYuv(void *obj, void *buffer, size_t bufferSize, void *iomxBuffer, void *userData)
{
	

	if (bufferSize) {
	    int expectedSize = bufferSize;
		int size = 0;

#if BUILD_FOR_HONEYCOMB_AND_UP
		if (g_hwVersion == -1 && p_getHardwareBaseVersion_d) {
			g_hwVersion = p_getHardwareBaseVersion_d();
		}
		

		//QcomOmxInterface *omx = (QcomOmxInterface *)obj;
		//int width = omx->getPortDimensions(1, 0);
		//int height = omx->getPortDimensions(1, 1);
		if (g_hwVersion == kHardwareBase8x60) {
			int bufferSize1 = TiledFrameSize(width, height);

			int inWidth = TILE_ALIGN(width, BLOCK_WIDTH /*128*/);
			int inHeight = TILE_ALIGN(height, BLOCK_HEIGHT);
			unsigned char *tmp = NULL;
            //__android_log_print(ANDROID_LOG_INFO, "tt", "handleOutputYuv is %d,%d,%d,%d,%d,%d", width,height,inWidth,inHeight,bufferSize,bufferSize1);

			//refer to http://code.metager.de/source/xref/vlc/modules/codec/omxil/qcom.c
			//unsigned char *tmp = (unsigned char*)av_mallocz(inWidth*inHeight*1.5); //(char *)malloc(inWidth*inHeight*1.5);
			//qcom_convert((uint8_t *)buffer, tmp, tmp+inWidth*inHeight, width, height, inWidth);

			pthread_mutex_lock (&frame_buf_mutex_d);
			if (g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf != NULL) {
				//memcpy(g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf, buffer, copysize);
				tmp = g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf;
				size = qcom_convert((uint8_t *)buffer, tmp, tmp+inWidth*inHeight, width, height, inWidth);
				g_framebuflist_d.framebuf[g_framebuflist_d.writepos].bufsize = inWidth*inHeight*1.5;
			}
			g_framebuflist_d.size++;
			g_framebuflist_d.writepos++;
			g_framebuflist_d.writepos = g_framebuflist_d.writepos%FRAMEBUFSIZE;

			pthread_mutex_unlock (&frame_buf_mutex_d);
#if 0
			FILE *f = fopen("/sdcard/1.yuv", "ab+");
			if(f!=NULL) 
				fwrite(tmp, 1, inWidth*inHeight*1.5, f);
			fclose(f);
			f = NULL;
#endif
            //av_freep(&tmp);
			//android::ColorConverter converter(
				//	(OMX_COLOR_FORMATTYPE)QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka, 
					//OMX_COLOR_Format16bitRGB565);
		}

		if(size == 0) {
			pthread_mutex_lock (&frame_buf_mutex_d);
			if (g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf != NULL) {
				int copysize = MIN(bufferSize,width*height*2);
				memcpy(g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf, buffer, copysize);
				//unsigned char *tmp = g_framebuflist_d.framebuf[g_framebuflist_d.writepos].buf;
				//size = qcom_convert((uint8_t *)buffer, tmp, tmp+inWidth*inHeight, width, height, inWidth);
				g_framebuflist_d.framebuf[g_framebuflist_d.writepos].bufsize = copysize;
			}
			g_framebuflist_d.size++;
			g_framebuflist_d.writepos++;
			g_framebuflist_d.writepos = g_framebuflist_d.writepos%FRAMEBUFSIZE;

			pthread_mutex_unlock (&frame_buf_mutex_d);
		}
#endif

	} else {
	    //__android_log_print(ANDROID_LOG_INFO, "tt", "handleOutputYuv1 is %d", bufferSize);

	}

    return 0;
}

av_cold int Qcom_h264_dec_init(AVCodecContext *avctx)
{
	int errorCode = kQcomOmxInterfaceErrorSuccess;

    timeStampLfile = 0;
    sem_init(&semaphoreBufferIsAvailable, 0, 0);

	if(p_decoder_create_d)
	    omxDecoder = p_decoder_create_d(&errorCode, NULL);
	
	if (omxDecoder == NULL) {
		return errorCode;
	}

    if(p_omx_interface_register_input_callback_d)
	   p_omx_interface_register_input_callback_d(omxDecoder, semaphoreEmptyBufferPost, NULL);
	
	if(p_omx_interface_register_output_callback_d)
	   p_omx_interface_register_output_callback_d(omxDecoder, handleOutputYuv, NULL);

    if(p_omx_interface_init_d)
	   errorCode = p_omx_interface_init_d(omxDecoder);
	
	if (errorCode != kQcomOmxInterfaceErrorSuccess)
	   return errorCode; 

    firsttime = 1;
	g_hwVersion = -1;

	width = avctx->width;
	height = avctx->height;
	if(avctx->extradata_size > 0 && avctx->extradata) {
#if 0
					FILE *f = fopen("/sdcard/1.264", "ab+");
					if(f!=NULL)
					   fwrite(avctx->extradata, 1, avctx->extradata_size, f);
					fclose(f);
					f = NULL;
#endif	

	}

	int inWidth = TILE_ALIGN(width, BLOCK_WIDTH);
	int inHeight = TILE_ALIGN(height, BLOCK_HEIGHT);

	g_framebuflist_d.readpos = 0;
	g_framebuflist_d.size = 0;
	g_framebuflist_d.writepos = 0;
	for(int i = 0; i < FRAMEBUFSIZE; i++) {
	  if(g_framebuflist_d.framebuf[i].buf == NULL)
         g_framebuflist_d.framebuf[i].buf = (unsigned char*)av_mallocz(inWidth*inHeight*2);
	  g_framebuflist_d.framebuf[i].bufsize = 0;
	  //if(g_framebuflist.buf[i] == NULL)
	}
	pthread_mutex_init (&frame_buf_mutex_d, NULL);

	//avctx->pix_fmt = PIX_FMT_NV12;
    return 0;
}

av_cold int Qcom_h264_dec_close(AVCodecContext *avctx)
{
    int errorCode = kQcomOmxInterfaceErrorSuccess;

    if(p_omx_interface_send_end_of_input_flag_d)
	   p_omx_interface_send_end_of_input_flag_d(omxDecoder, timeStampLfile);

	if(p_omx_interface_deinit_d)
	   errorCode = p_omx_interface_deinit_d(omxDecoder);
	//fclose(file);

	if(p_omx_interface_destroy_d)
	   p_omx_interface_destroy_d(omxDecoder);
	
	sem_destroy(&semaphoreBufferIsAvailable);

	pthread_mutex_lock (&frame_buf_mutex_d);
	
	g_framebuflist_d.readpos = 0;
	g_framebuflist_d.size = 0;
	g_framebuflist_d.writepos = 0;
	for(int i = 0; i < FRAMEBUFSIZE; i++) {
		//__android_log_print(ANDROID_LOG_INFO, "tt", "QCOM_H264_close is %d", g_framebuflist.framebuf[i].buf);
		if(g_framebuflist_d.framebuf[i].buf)
			av_freep(&(g_framebuflist_d.framebuf[i].buf));
		g_framebuflist_d.framebuf[i].bufsize = 0;
	}
	
	pthread_mutex_unlock (&frame_buf_mutex_d);
	pthread_mutex_destroy(&frame_buf_mutex_d);

    return 0;
}


static 
int Qcom_h264_dec_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{

    uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
	AVFrame *pict = (AVFrame *)data;
    uint8_t header[3000] = {0};
     
	int result = 0;
	if(!omxDecoder || !p_omx_interface_send_input_data_d || !p_omx_interface_send_end_of_input_flag_d)
		return 0;

    if(firsttime && avctx->extradata && avctx->extradata_size > 0 && avctx->extradata_size < 3000) {
		int i, cnt, nalsize;
        unsigned char *p = avctx->extradata;
		unsigned char *p1 = header;
		//*((int *)p) = 0;
        // Decode sps from avcC
        cnt = *(p+5) & 0x1f; // Number of sps
        p += 6;

        for (i = 0; i < cnt; i++) {
            nalsize = AV_RB16(p) + 2;
			
			*((int *)p1) = 0x01000000;
		    p1+=4;
			memcpy(p1,p+2,nalsize-2);
			p1+=nalsize-2;
			
			if(nalsize > avctx->extradata_size - (p-buf))
                return -1;
			p += nalsize;
        }

		cnt = *(p++); // Number of pps
		for (i = 0; i < cnt; i++) {
			nalsize = AV_RB16(p) + 2;
			
			*((int *)p1) = 0x01000000;
		    p1+=4;
			memcpy(p1,p+2,nalsize-2);
			p1+=nalsize-2;
			
			if(nalsize > avctx->extradata_size - (p-buf))
				return -1;

			p += nalsize;
		}
		
		if (p1 > header) 
			p_omx_interface_send_input_data_d(omxDecoder, header, p1-header, 0);


#if 0	
		FILE *f = fopen("/sdcard/1.264", "ab+");
		if(f!=NULL)
		   fwrite(header, 1,  p1-header, f);
		fclose(f);
		f = NULL;
#endif	

		firsttime = 0;
    }



    *((int *)buf) = 0x01000000;
#if 0
	FILE *f = fopen("/sdcard/1.264", "ab+");
	if(f!=NULL)
	   fwrite(buf, 1, buf_size, f);
	fclose(f);
	f = NULL;
#endif	

    //return 0;
	do {
		if(buf_size > 0) {
			result = p_omx_interface_send_input_data_d(omxDecoder, buf, buf_size, timeStampLfile);
		} else {
			result = p_omx_interface_send_end_of_input_flag_d(omxDecoder, timeStampLfile);
		}

		//__android_log_print(ANDROID_LOG_INFO, "tt", "Qcom_h264_dec_frame is %d,%d,%d,%d", pict,pict->data[0], \
																					timeStampLfile, result);

		if (result == -1) {
			sem_wait(&semaphoreBufferIsAvailable);
		}
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Qcom_h264_dec_frame1 is %d,%d,%d,%d", buf,buf_size, \
			//																		timeStampLfile, result);

	} while (result != 0);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Qcom_h264_dec_frame is end is %d", g_framebuflist_d.size);

	pthread_mutex_lock (&frame_buf_mutex_d);
	if(pict && g_framebuflist_d.size > 0 && g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf != NULL) {
		int inWidth = TILE_ALIGN(width, BLOCK_WIDTH);
		int inHeight = TILE_ALIGN(height, BLOCK_HEIGHT);

		//memcpy(buf, g_framebuflist.framebuf[g_framebuflist.readpos].buf, copysize);
		unsigned char* u_ptr;
		unsigned char* v_ptr;
		unsigned char* nv_ptr;
        pict->data[0] =  g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf;
		pict->linesize[0] = inWidth;
		u_ptr = pict->data[1] = g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf+inWidth*inHeight*3/2;
		pict->linesize[1] = inWidth/2;
		v_ptr = pict->data[2] =  g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf+inWidth*inHeight*7/4;
		pict->linesize[2] = inWidth/2;
		nv_ptr = g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf + inWidth*inHeight;
#if 1
		for(int i=0; i<inHeight/2; i++)
			for(int j=0; j<inWidth/2; j++){
                *(u_ptr+j+i*inWidth/2)=*(nv_ptr+j*2+i*inWidth);
                *(v_ptr+j+i*inWidth/2)=*(nv_ptr+j*2+1+i*inWidth);
			}
#else
        pict->data[1] = g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf+inWidth*inHeight;
        pict->linesize[1] = inWidth;
		pict->data[2] =  g_framebuflist_d.framebuf[g_framebuflist_d.readpos].buf+inWidth*inHeight*5/4;
		pict->linesize[2] = inWidth;

#endif
		g_framebuflist_d.size--;
		g_framebuflist_d.readpos++;
		g_framebuflist_d.readpos = g_framebuflist_d.readpos%FRAMEBUFSIZE;
		*data_size = sizeof(AVFrame);
#if 0
			FILE *f = fopen("/sdcard/1.yuv", "ab+");
			if(f!=NULL){
				fwrite(pict->data[0], 1, inWidth*inHeight, f);
				fwrite(pict->data[1], 1, inWidth*inHeight/2, f);
				//fwrite(pict->data[2], 1, inWidth*inHeight, f);
			}
			fclose(f);
			f = NULL;
#endif
	}
	pthread_mutex_unlock (&frame_buf_mutex_d);
	
    count++;
    timeStampLfile += 1000000/10;
	//avctx->pix_fmt = PIX_FMT_NV12;
    return 0;
}


AVCodec ff_qcom264_decoder = {"qcom264dec", AVMEDIA_TYPE_VIDEO, CODEC_ID_H264, 0, Qcom_h264_dec_init, NULL, Qcom_h264_dec_close, Qcom_h264_dec_frame
};


#if 0
AVCodec ff_h264_decoder = {
    .name           = "qcom264dec",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H264,
    .priv_data_size = sizeof(H264Context),
    .init           = ff_h264_decode_init,
    .close          = ff_h264_decode_end,
    .decode         = decode_frame,
    .capabilities   = /*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 | CODEC_CAP_DELAY |
                      CODEC_CAP_SLICE_THREADS | CODEC_CAP_FRAME_THREADS,
    .flush= flush_dpb,
    .long_name = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
    .init_thread_copy      = ONLY_IF_THREADS_ENABLED(decode_init_thread_copy),
    .update_thread_context = ONLY_IF_THREADS_ENABLED(decode_update_thread_context),
    .profiles = NULL_IF_CONFIG_SMALL(profiles),
    .priv_class     = &h264_class,
};
#endif 

