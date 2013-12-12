/**
 * @file
 * H264 decode for samsung mali
 * @author carlzhong
 */

#include <dlfcn.h>
#include <android/log.h>

#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"



#include "SsbSipMfcApi.h"
#include "OMX_Types.h"

#if 1
typedef struct _SEC_MFC_H264DEC_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;
    OMX_PTR pMFCStreamBuffer;
    OMX_PTR pMFCStreamPhyBuffer;
    OMX_U32    indexTimestamp;
    OMX_BOOL bConfiguredMFC;
    OMX_BOOL bThumbnailMode;
} SEC_MFC_H264DEC_HANDLE;
#endif
#if 0
typedef struct _SEC_MFC_H264DEC_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;
    OMX_PTR pMFCStreamBuffer;
    OMX_PTR pMFCStreamPhyBuffer;
    OMX_U32    indexTimestamp;
    OMX_BOOL bConfiguredMFC;
    OMX_BOOL bThumbnailMode;
    OMX_S32  returnCodec;
} SEC_MFC_H264DEC_HANDLE;
#endif 

#define DEFAULT_MFC_INPUT_BUFFER_SIZE    1024 * 1024 /*DEFAULT_VIDEO_INPUT_BUFFER_SIZE*/

#define AV_RB16(x)      \
((((const uint8_t*)(x))[0] << 8) |  \
((const uint8_t*)(x))[1])

#define MAX_TIMESTAMP        16


void *(*p_SsbSipMfcDecOpen)(void) = NULL;
void  *(*p_SsbSipMfcDecGetInBuf)(void *openHandle, void **phyInBuf, int inputBufferSize) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcDecSetConfig)(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcDecInit)(void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcDecExe)(void *openHandle, int lengthBufFill) = NULL;
SSBSIP_MFC_DEC_OUTBUF_STATUS (*p_SsbSipMfcDecGetOutBuf)(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcDecClose)(void *openHandle);
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcDecSetInBuf)(void *openHandle, void *phyInBuf, void *virInBuf, int size) = NULL;


static void* pdlhandle = NULL;
static int firsttime = 1;
SEC_MFC_H264DEC_HANDLE hMFCH264Handle;

static int indexTimestamp = 0;
static unsigned char *picBuf = NULL;

extern av_cold int Samsung_H264_decodeIsAvailable()
{
   int isAvailable = 0;
   //deinit();
   if(NULL == pdlhandle)
      pdlhandle = dlopen("/data/data/com.yb.demotest/lib/libsecmfcdecapi.so", RTLD_NOW);
   __android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_H264_decodeIsAvailable is %d", pdlhandle);
   
#if 1
   if (pdlhandle != NULL) {
	   p_SsbSipMfcDecOpen =p_SsbSipMfcDecOpen?p_SsbSipMfcDecOpen:(void* (*)(void))dlsym(pdlhandle, "SsbSipMfcDecOpen");
	   p_SsbSipMfcDecGetInBuf = p_SsbSipMfcDecGetInBuf?p_SsbSipMfcDecGetInBuf:(void *(*)(void *openHandle, void **phyInBuf, int inputBufferSize))dlsym(pdlhandle, "SsbSipMfcDecGetInBuf");
	   p_SsbSipMfcDecSetConfig = p_SsbSipMfcDecSetConfig?p_SsbSipMfcDecSetConfig:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_DEC_CONF conf_type, void *value))dlsym(pdlhandle, "SsbSipMfcDecSetConfig");
	   p_SsbSipMfcDecExe = p_SsbSipMfcDecExe?p_SsbSipMfcDecExe:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, int lengthBufFill))dlsym(pdlhandle, "SsbSipMfcDecExe");
	   p_SsbSipMfcDecGetOutBuf = p_SsbSipMfcDecGetOutBuf?p_SsbSipMfcDecGetOutBuf:(SSBSIP_MFC_DEC_OUTBUF_STATUS (*)(void *openHandle, SSBSIP_MFC_DEC_OUTPUT_INFO *output_info))dlsym(pdlhandle, "SsbSipMfcDecGetOutBuf");
	   p_SsbSipMfcDecClose = p_SsbSipMfcDecClose?p_SsbSipMfcDecClose:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle))dlsym(pdlhandle, "SsbSipMfcDecClose");
	   p_SsbSipMfcDecInit = p_SsbSipMfcDecInit?p_SsbSipMfcDecInit:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_CODEC_TYPE codec_type, int Frameleng))dlsym(pdlhandle, "SsbSipMfcDecInit");
	   p_SsbSipMfcDecSetInBuf = p_SsbSipMfcDecSetInBuf?p_SsbSipMfcDecSetInBuf:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, void *phyInBuf, void *virInBuf, int size))dlsym(pdlhandle, "SsbSipMfcDecSetInBuf");
	  //p_omx_interface_init = p_omx_interface_init?p_omx_interface_init:(int (*)(void *obj))dlsym(pdlhandle1, "omx_interface_init");
	   __android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_H264_decodeIsAvailable end is %d,%d,%d,%d,%d,%d", p_SsbSipMfcDecOpen,p_SsbSipMfcDecGetInBuf,\
		                      p_SsbSipMfcDecSetConfig,p_SsbSipMfcDecExe,p_SsbSipMfcDecGetOutBuf,p_SsbSipMfcDecClose);
	   if (!(p_SsbSipMfcDecOpen  && p_SsbSipMfcDecGetInBuf && p_SsbSipMfcDecSetConfig \
			  && p_SsbSipMfcDecExe && p_SsbSipMfcDecGetOutBuf && p_SsbSipMfcDecClose && \
			  p_SsbSipMfcDecSetInBuf && p_SsbSipMfcDecInit))
			     return 0;
   } else 
       return 0;
#endif

   __android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_H264_decodeIsAvailable end is %d", pdlhandle);
   return 1;// 需要进一步的判断
   //if(p_omx_component_is_available)
   // isAvailable = p_omx_component_is_available("OMX.SEC.AVC.Encoder");//carlzhong ("OMX.qcom.video.encoder.avc");
   //__android_log_print(ANDROID_LOG_INFO, "tt", "isAvailable is %d", isAvailable);
   //return isAvailable ? 1 : 0;
}


OMX_BOOL Check_H264_StartCode(OMX_U8 *pInputStream, OMX_U32 streamSize)
{
    if (streamSize < 4) {
        return OMX_FALSE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] == 0x00) &&
              (pInputStream[3] != 0x00) &&
              ((pInputStream[3] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else if ((pInputStream[0] == 0x00) &&
              (pInputStream[1] == 0x00) &&
              (pInputStream[2] != 0x00) &&
              ((pInputStream[2] >> 3) == 0x00)) {
        return OMX_TRUE;
    } else {
        return OMX_FALSE;
    }
}


int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
    int pixel_x_m1, pixel_y_m1;
    int roundup_x, roundup_y;
    int linear_addr0, linear_addr1, bank_addr ;
    int x_addr;
    int trans_addr;

    pixel_x_m1 = x_size -1;
    pixel_y_m1 = y_size -1;

    roundup_x = ((pixel_x_m1 >> 7) + 1);
    roundup_y = ((pixel_x_m1 >> 6) + 1);

    x_addr = x_pos >> 2;

    if ((y_size <= y_pos+32) && ( y_pos < y_size) &&
        (((pixel_y_m1 >> 5) & 0x1) == 0) && (((y_pos >> 5) & 0x1) == 0)) {
        linear_addr0 = (((y_pos & 0x1f) <<4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    } else {
        linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
        linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

        if (((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
            bank_addr = ((x_addr >> 4) & 0x1);
        else
            bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
    }

    linear_addr0 = linear_addr0 << 2;
    trans_addr = (linear_addr1 <<13) | (bank_addr << 11) | linear_addr0;

    return trans_addr;
}



void Y_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
    unsigned int max_index = x_size * y_size;

    for (i = 0; i < y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, y_size, j, i);
			__android_log_print(ANDROID_LOG_INFO, "tt", "Y_tile_to_linear_4x2 is %d", trans_addr,x_size,y_size);
            for (k = 0; k < 16; k++) {
                index = (i * x_size) + (x_size * k) + j;
        
                if (index + 16 > max_index) {
                    continue;
                }

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

                p_linear_addr[index] = data8[0];
                p_linear_addr[index + 1] = data8[1];
                p_linear_addr[index + 2] = data8[2];
                p_linear_addr[index + 3] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

                p_linear_addr[index + 4] = data8[0];
                p_linear_addr[index + 5] = data8[1];
                p_linear_addr[index + 6] = data8[2];
                p_linear_addr[index + 7] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

                p_linear_addr[index + 8] = data8[0];
                p_linear_addr[index + 9] = data8[1];
                p_linear_addr[index + 10] = data8[2];
                p_linear_addr[index + 11] = data8[3];

                data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
                data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
                data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
                data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

                p_linear_addr[index + 12] = data8[0];
                p_linear_addr[index + 13] = data8[1];
                p_linear_addr[index + 14] = data8[2];
                p_linear_addr[index + 15] = data8[3];
            }
        }
    }
}


void CbCr_tile_to_linear_4x2(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size)
{
    int trans_addr;
    unsigned int i, j, k, index;
    unsigned char data8[4];
	unsigned int half_y_size = y_size / 2;
    unsigned int max_index = x_size * half_y_size;
    unsigned char *pUVAddr[2];
    
    pUVAddr[0] = p_linear_addr;
    pUVAddr[1] = p_linear_addr + ((x_size * half_y_size) / 2);
    
    for (i = 0; i < half_y_size; i = i + 16) {
        for (j = 0; j < x_size; j = j + 16) {
            trans_addr = tile_4x2_read(x_size, half_y_size, j, i);
            for (k = 0; k < 16; k++) {
                index = (i * x_size) + (x_size * k) + j;
               
                if (index + 16 > max_index) {
                    continue;
                }

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 0];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 1];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 2];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 3];

				pUVAddr[index%2][index/2] = data8[0];
				pUVAddr[(index+1)%2][(index+1)/2] = data8[1];
				pUVAddr[(index+2)%2][(index+2)/2] = data8[2];
				pUVAddr[(index+3)%2][(index+3)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 4];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 5];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 6];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 7];

				pUVAddr[(index+4)%2][(index+4)/2] = data8[0];
				pUVAddr[(index+5)%2][(index+5)/2] = data8[1];
				pUVAddr[(index+6)%2][(index+6)/2] = data8[2];
				pUVAddr[(index+7)%2][(index+7)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 8];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 9];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 10];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 11];

				pUVAddr[(index+8)%2][(index+8)/2] = data8[0];
				pUVAddr[(index+9)%2][(index+9)/2] = data8[1];
				pUVAddr[(index+10)%2][(index+10)/2] = data8[2];
				pUVAddr[(index+11)%2][(index+11)/2] = data8[3];

				data8[0] = p_tiled_addr[trans_addr + 64 * k + 12];
				data8[1] = p_tiled_addr[trans_addr + 64 * k + 13];
				data8[2] = p_tiled_addr[trans_addr + 64 * k + 14];
				data8[3] = p_tiled_addr[trans_addr + 64 * k + 15];

				pUVAddr[(index+12)%2][(index+12)/2] = data8[0];
				pUVAddr[(index+13)%2][(index+13)/2] = data8[1];
				pUVAddr[(index+14)%2][(index+14)/2] = data8[2];
				pUVAddr[(index+15)%2][(index+15)/2] = data8[3];
            }
        }
    }
}


av_cold int Samsung_h264_dec_init(AVCodecContext *avctx)
{
    OMX_PTR pStreamBuffer    = NULL;
    OMX_PTR pStreamPhyBuffer = NULL;
    firsttime = 1;
	indexTimestamp = 0;
	memset( &hMFCH264Handle, 0, sizeof(SEC_MFC_H264DEC_HANDLE) );
	hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_init is %d", hMFCH264Handle.hMFCHandle);

	//SSBIP_MFC_BUFFER_TYPE buf_type = NO_CACHE;

    hMFCH264Handle.hMFCHandle = p_SsbSipMfcDecOpen();
	if(hMFCH264Handle.hMFCHandle == NULL)
		return -1;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_init1 is %d", hMFCH264Handle.hMFCHandle);

	pStreamBuffer = p_SsbSipMfcDecGetInBuf(hMFCH264Handle.hMFCHandle, &pStreamPhyBuffer, DEFAULT_MFC_INPUT_BUFFER_SIZE);
	if (pStreamBuffer == NULL)
		return -1;
	__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_init2 is %d,%d",pStreamBuffer,pStreamPhyBuffer);

	hMFCH264Handle.pMFCStreamBuffer    = pStreamBuffer;
    hMFCH264Handle.pMFCStreamPhyBuffer = pStreamPhyBuffer;

	if (picBuf == NULL)
		picBuf = (unsigned char*)av_mallocz(avctx->width*avctx->height*10);

	return 0;
}


av_cold int Samsung_h264_dec_close(AVCodecContext *avctx)
{
    if (hMFCH264Handle.hMFCHandle != NULL) {
        p_SsbSipMfcDecClose(hMFCH264Handle.hMFCHandle);
        hMFCH264Handle.hMFCHandle = NULL;
	}

    if (picBuf != NULL) {
		av_freep(&picBuf);
		picBuf = NULL;
	}
    return 0;
}


static 
int Samsung_h264_dec_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             AVPacket *avpkt)
{
    static int cout = 0;
	OMX_S32 setConfVal = 0;
	OMX_S32 returnCodec = 0;
    uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
	AVFrame *pict = (AVFrame *)data;
	OMX_PTR pStreamBuffer	 = NULL;
    int oneFrameSize =  1920*1080*2;
	SSBSIP_MFC_DEC_OUTPUT_INFO outputInfo;
    //int headeSize = 0
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame1 is %d", buf_size);
	//hMFCH264Handle.pMFCStreamBuffer = p_SsbSipMfcDecGetInBuf(hMFCH264Handle.hMFCHandle, &(hMFCH264Handle.pMFCStreamPhyBuffer), DEFAULT_MFC_INPUT_BUFFER_SIZE);

    if (firsttime && avctx->extradata && avctx->extradata_size > 0 && avctx->extradata_size < 3000) {
		int i, cnt, nalsize;
	    unsigned char *p = avctx->extradata;
		uint8_t header[3000] = {0};
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
		__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame122 is %d,%d,%d", hMFCH264Handle.pMFCStreamPhyBuffer ,p1-header,buf_size);
		if (p1 > header) 
			memcpy((unsigned char*)(hMFCH264Handle.pMFCStreamBuffer), header, p1-header);
		__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame122 is %d,%d,%d", hMFCH264Handle.pMFCStreamPhyBuffer ,p1-header,buf_size);

		unsigned char* tmp = (unsigned char*)(hMFCH264Handle.pMFCStreamBuffer);
		tmp += p1-header;
		*((int *)buf) = 0x01000000;
        if (buf_size > 0)
		    memcpy(tmp,buf,buf_size);
		buf_size += p1-header;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame122 is %d,%d,%d", hMFCH264Handle.pMFCStreamBuffer ,p1-header,buf_size);
		
#if 0	
		FILE *f = fopen("/sdcard/1.264", "ab+");
		if(f!=NULL)
		   fwrite(header, 1,  p1-header, f);
		fclose(f);
		f = NULL;
#endif	
		firsttime = 0;
    }else {
		*((int *)buf) = 0x01000000;
		if (buf_size > 0)
		    memcpy((unsigned char*)(hMFCH264Handle.pMFCStreamBuffer),buf,buf_size);
	}
	
#if 0
		FILE *f = fopen("/sdcard/1.264", "ab+");
		if(f!=NULL)
		   fwrite((unsigned char*)(hMFCH264Handle.pMFCStreamBuffer), 1, buf_size, f);
		fclose(f);
		f = NULL;
#endif	

	//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame2 is %d", buf_size);

	if (hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
		SSBSIP_MFC_CODEC_TYPE eCodecType = H264_DEC;

		//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame22 is %d", hMFCH264Handle.hMFCHandle);
        setConfVal = 5;
        p_SsbSipMfcDecSetConfig(hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &setConfVal);
        //__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame221 is %d", hMFCH264Handle.hMFCHandle);
        //设置GPU里的缓冲帧个数
	    setConfVal = 0;
	    p_SsbSipMfcDecSetConfig(hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_DISPLAY_DELAY, &setConfVal);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame221 is %d,%d", buf_size,p_SsbSipMfcDecInit);

	    returnCodec = p_SsbSipMfcDecInit(hMFCH264Handle.hMFCHandle, eCodecType, buf_size);

		if (returnCodec != MFC_RET_OK)
			return -1;
		hMFCH264Handle.bConfiguredMFC = OMX_TRUE;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame3 is %d", buf_size);
    //__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame3 is %d,%d,%d,%d,%d", buf[0],buf[1],buf[2],buf[3],buf_size);

	p_SsbSipMfcDecSetConfig(hMFCH264Handle.hMFCHandle, MFC_DEC_SETCONF_FRAME_TAG, &indexTimestamp);

	indexTimestamp++;
	indexTimestamp %= MAX_TIMESTAMP;

	if (Check_H264_StartCode(hMFCH264Handle.pMFCStreamBuffer, buf_size) == OMX_TRUE) {
		p_SsbSipMfcDecSetInBuf(hMFCH264Handle.hMFCHandle, hMFCH264Handle.pMFCStreamPhyBuffer, hMFCH264Handle.pMFCStreamBuffer, buf_size);
        returnCodec = p_SsbSipMfcDecExe(hMFCH264Handle.hMFCHandle, buf_size);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame3 is %d", returnCodec);
		if (returnCodec == MFC_RET_OK) {
			SSBSIP_MFC_DEC_OUTBUF_STATUS status;
			OMX_S32 indexTimestamp = 0;
			int bufWidth, bufHeight;
			
			status = p_SsbSipMfcDecGetOutBuf(hMFCH264Handle.hMFCHandle, &outputInfo);
			bufWidth =    (outputInfo.img_width + 15) & (~15);
            bufHeight =  (outputInfo.img_height + 15) & (~15);
			int frameSize = bufWidth * bufHeight;
			__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_h264_dec_frame is %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",((outputInfo.YPhyAddr)),outputInfo.CPhyAddr, \
				                                          pict->linesize[0],pict->linesize[1],outputInfo.consumedByte, \
				                                          outputInfo.crop_top_offset,outputInfo.res_change,outputInfo.crop_left_offset,outputInfo.timestamp_top, \
				                                          status);
            cout++;
            if ((status == MFC_GETOUTBUF_DISPLAY_DECODING) ||
                 (status == MFC_GETOUTBUF_DISPLAY_ONLY)) {
#if 0
				  if (picBuf == NULL)
		              picBuf = (unsigned char*)av_mallocz(avctx->width*avctx->height*10);

				  Y_tile_to_linear_4x2(
                    (unsigned char *)picBuf,
                    (unsigned char *)outputInfo.YVirAddr,
                    bufWidth, bufHeight);
				  
                  CbCr_tile_to_linear_4x2(
                    ((unsigned char *)picBuf) + frameSize,
                    (unsigned char *)outputInfo.CVirAddr,
                    bufWidth, bufHeight);
#endif

	              pict->sample_aspect_ratio.num =  outputInfo.YPhyAddr;
		          //pict->linesize[0] = 1920;
				  pict->sample_aspect_ratio.den = outputInfo.CPhyAddr;
				  //pict->linesize[1] = 1080;
				  pict->interlaced_frame = -1;
				  //pict->data[2] = outputInfo.CVirAddr + frameSize/4;
				  //pict->linesize[2] = bufWidth/2;
				  *data_size = sizeof(AVFrame);


#if 0
				FILE *f = fopen("/sdcard/2.yuv", "ab+");
				if(f!=NULL && cout <30) {
					 //for(int i =0; i<480; i++)
					   fwrite(outputInfo.YVirAddr, 1, frameSize, f);
					   fwrite(outputInfo.CVirAddr, 1, frameSize/2, f);
				}
				//fflush(f);
				fclose(f);
				f = NULL;
#endif
			}

		}

	}
	

    return 0;
}



AVCodec ff_samsung264_decoder = {"samsung264dec", AVMEDIA_TYPE_VIDEO, CODEC_ID_H264, 0, Samsung_h264_dec_init, NULL, Samsung_h264_dec_close, Samsung_h264_dec_frame
};

