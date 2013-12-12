
/**
 * @file
 * H264 encoder for samsung mali
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

#define ALIGN_TO_16B(x)   ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_64B(x)   ((((x) + (1 <<  6) - 1) >>  6) <<  6)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)   ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_4KB(x)   ((((x) + (1 << 12) - 1) >> 12) << 12)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)  ((((x) + (1 << 16) - 1) >> 16) << 16)
#define ALIGN_TO_128KB(x) ((((x) + (1 << 17) - 1) >> 17) << 17)


typedef struct
{
    void *pAddrY;
    void *pAddrC;
} MFC_ENC_ADDR_INFO;

typedef struct _SEC_BUFFER_HEADER{
    void *YPhyAddr; // [IN/OUT] physical address of Y
    void *CPhyAddr; // [IN/OUT] physical address of CbCr
    void *YVirAddr; // [IN/OUT] virtual address of Y
    void *CVirAddr; // [IN/OUT] virtual address of CbCr
    int YSize;      // [IN/OUT] input size of Y data
    int CSize;      // [IN/OUT] input size of CbCr data
} SEC_BUFFER_HEADER;

typedef struct _SEC_OMX_DATA
{
    OMX_BYTE  dataBuffer;
    OMX_U32   allocSize;
    OMX_U32   dataLen;
    OMX_U32   usedDataLen;
    OMX_U32   remainDataLen;
    OMX_U32   previousDataLen;
    OMX_U32   nFlags;
    OMX_TICKS timeStamp;
    SEC_BUFFER_HEADER specificBufferHeader;
} SEC_OMX_DATA;

typedef struct _EXTRA_DATA
{
    OMX_PTR pHeaderSPS;
    OMX_U32 SPSLen;
    OMX_PTR pHeaderPPS;
    OMX_U32 PPSLen;
} EXTRA_DATA;

typedef struct _SEC_MFC_H264ENC_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;
    SSBSIP_MFC_ENC_H264_PARAM mfcVideoAvc;
    SSBSIP_MFC_ENC_INPUT_INFO inputInfo;
/*    SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo; */
    OMX_U32    indexTimestamp;
    OMX_BOOL bConfiguredMFC;
    EXTRA_DATA headerData;
} SEC_MFC_H264ENC_HANDLE;


SEC_MFC_H264ENC_HANDLE hMFCH264Handle;
SSBSIP_MFC_ENC_INPUT_INFO inputInfo;
SSBSIP_MFC_ENC_OUTPUT_INFO outputInfo;
OMX_PTR hMFCHandle = NULL;

//SEC_OMX_DATA inputData;
//SEC_OMX_DATA outputData;
//MFC_ENC_ADDR_INFO AddrInfo;

unsigned char *picBuf1 = NULL;

void* pdlhandle1 = NULL;  
void* (*p_SsbSipMfcEncOpen)(void) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncInit)(void *openHandle, void *param) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncGetInBuf)(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncGetOutBuf)(void *openHandle, SSBSIP_MFC_ENC_OUTPUT_INFO *output_info) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncSetInBuf)(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncExe)(void *openHandle) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncClose)(void *openHandle) = NULL;
SSBSIP_MFC_ERROR_CODE (*p_SsbSipMfcEncSetConfig)(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value) = NULL;

#define MAX_TIMESTAMP        16
int indexTimestamp = 0;



int SetH264EncParam( SSBSIP_MFC_ENC_H264_PARAM* pH264Arg, AVCodecContext *ctx)
{
	pH264Arg->codecType = H264_ENC;
	pH264Arg->SourceWidth = ctx->width; //640;	//= pSECOutputPort->portDefinition.format.video.nFrameWidth;  
    pH264Arg->SourceHeight = ctx->height; //480;	//= pSECOutputPort->portDefinition.format.video.nFrameHeight;
    pH264Arg->IDRPeriod = 50;	//pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH264Arg->SliceMode = 1;
    pH264Arg->RandomIntraMBRefresh = 0;
    pH264Arg->Bitrate = 32000;	//pSECOutputPort->portDefinition.format.video.nBitrate;
    pH264Arg->QSCodeMax = 30;
    pH264Arg->QSCodeMin = 10;
    pH264Arg->PadControlOn = 0;             // 0: disable, 1: enable
    pH264Arg->LumaPadVal = 0;
    pH264Arg->CbPadVal = 0;
    pH264Arg->CrPadVal = 0;
	pH264Arg->FrameMap = 0;

    pH264Arg->ProfileIDC = 1;//2: base ;	//OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile); //0;  //(OMX_VIDEO_AVCProfileMain)
    pH264Arg->LevelIDC = 10;	//OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);       //40; //(OMX_VIDEO_AVCLevel4)
    pH264Arg->FrameRate = 29.97;	//(pSECInputPort->portDefinition.format.video.xFramerate) >> 16;
    pH264Arg->SliceArgument = 0;          // Slice mb/byte size number
    pH264Arg->NumberBFrames = 0;            // 0 ~ 2
    pH264Arg->NumberReferenceFrames = 0;
    pH264Arg->NumberRefForPframes = 0;
    pH264Arg->LoopFilterDisable = 1;    // 1: Loop Filter Disable, 0: Filter Enable
    pH264Arg->LoopFilterAlphaC0Offset = 1;
    pH264Arg->LoopFilterBetaOffset = 1;
    pH264Arg->SymbolMode = 1;         // 0: CAVLC, 1: CABAC
    pH264Arg->PictureInterlace = 0;   //0
    pH264Arg->Transform8x8Mode = 1;         // 0: 4x4, 1: allow 8x8
    pH264Arg->DarkDisable = 0;
    pH264Arg->SmoothDisable = 0;
    pH264Arg->StaticDisable = 0;
    pH264Arg->ActivityDisable = 0;

    pH264Arg->FrameQp = 20;	//pVideoEnc->quantization.nQpI;
    pH264Arg->FrameQp_P = 20;	//pVideoEnc->quantization.nQpP;
    pH264Arg->FrameQp_B = 20;	//pVideoEnc->quantization.nQpB;

	//	Android default
	pH264Arg->EnableFRMRateControl = 1;	// 0: Disable, 1: Frame level RC
    pH264Arg->EnableMBRateControl  = 1;	// 0: Disable, 1:MB level RC
    pH264Arg->CBRPeriodRf  = 100;

	//__android_log_print(ANDROID_LOG_INFO, "tt", "SetH264EncParam is %d", pdlhandle1);

	return 0;
}


OMX_U8* FindDelimiter(OMX_U8* pBuffer, OMX_U32 size)
{
    for( int i = 0; i < size-3; i++) 
	{
        if( (pBuffer[i] == 0x00) && (pBuffer[i+1] == 0x00) \ 
			  && (pBuffer[i+2] == 0x00) && (pBuffer[i+3] == 0x01) )
            return (pBuffer + i);
    }

    return NULL;
}


extern av_cold int Samsung_H264_encodeIsAvailable()
{
   int isAvailable = 0;
   //deinit();
   if (NULL == pdlhandle1)
        pdlhandle1 = dlopen("/data/data/com.iqiyi.share/lib/libsecmfcencapi.so", RTLD_NOW);
   
   //__android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_H264_encodeIsAvailable is %d", pdlhandle1);
   if(pdlhandle1 != NULL) {
		p_SsbSipMfcEncOpen =p_SsbSipMfcEncOpen?p_SsbSipMfcEncOpen:(void* (*)(void))dlsym(pdlhandle1, "SsbSipMfcEncOpen");
        p_SsbSipMfcEncInit = p_SsbSipMfcEncInit?p_SsbSipMfcEncInit:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, void *param))dlsym(pdlhandle1, "SsbSipMfcEncInit");
		//p_omx_component_is_available = (bool (*)(const char *hardwareCodecString))dlsym(pdlhandle1, "omx_component_is_available");
        p_SsbSipMfcEncGetInBuf = p_SsbSipMfcEncGetInBuf?p_SsbSipMfcEncGetInBuf:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info))dlsym(pdlhandle1, "SsbSipMfcEncGetInBuf");
		p_SsbSipMfcEncGetOutBuf = p_SsbSipMfcEncGetOutBuf?p_SsbSipMfcEncGetOutBuf:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_ENC_OUTPUT_INFO *output_info))dlsym(pdlhandle1, "SsbSipMfcEncGetOutBuf");
		p_SsbSipMfcEncSetInBuf = p_SsbSipMfcEncSetInBuf?p_SsbSipMfcEncSetInBuf:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_ENC_INPUT_INFO *input_info))dlsym(pdlhandle1, "SsbSipMfcEncSetInBuf");
        p_SsbSipMfcEncExe = p_SsbSipMfcEncExe?p_SsbSipMfcEncExe:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle))dlsym(pdlhandle1, "SsbSipMfcEncExe");
		p_SsbSipMfcEncClose = p_SsbSipMfcEncClose?p_SsbSipMfcEncClose:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle))dlsym(pdlhandle1, "SsbSipMfcEncClose");
        p_SsbSipMfcEncSetConfig = p_SsbSipMfcEncSetConfig?p_SsbSipMfcEncSetConfig:(SSBSIP_MFC_ERROR_CODE (*)(void *openHandle, SSBSIP_MFC_ENC_CONF conf_type, void *value))dlsym(pdlhandle1, "SsbSipMfcEncSetConfig");
        //p_omx_interface_init = p_omx_interface_init?p_omx_interface_init:(int (*)(void *obj))dlsym(pdlhandle1, "omx_interface_init");
		
		if(!(p_SsbSipMfcEncOpen && p_SsbSipMfcEncInit && p_SsbSipMfcEncGetInBuf \
			  && p_SsbSipMfcEncGetOutBuf && p_SsbSipMfcEncSetInBuf && p_SsbSipMfcEncExe && p_SsbSipMfcEncSetConfig))
			     return 0;
   } else 
        return 0;
   __android_log_print(ANDROID_LOG_INFO, "tt", "Samsung_H264_encodeIsAvailable end is %d", pdlhandle1);
   return 1;// 需要进一步的判断
   //if(p_omx_component_is_available)
   // isAvailable = p_omx_component_is_available("OMX.SEC.AVC.Encoder");//carlzhong ("OMX.qcom.video.encoder.avc");
   //__android_log_print(ANDROID_LOG_INFO, "tt", "isAvailable is %d", isAvailable);
   //return isAvailable ? 1 : 0;
}

static 
av_cold int samsung_enc_init(AVCodecContext *avctx)
{
    int nRet = 0;
    indexTimestamp = 0;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init is %d", pdlhandle1);
    memset( &hMFCH264Handle, 0, sizeof(SEC_MFC_H264ENC_HANDLE) );
	OMX_PTR hMFCHandle = NULL;
	memset( &inputInfo, 0, sizeof(SSBSIP_MFC_ENC_INPUT_INFO) );
	memset( &outputInfo, 0, sizeof(SSBSIP_MFC_ENC_OUTPUT_INFO) );
#if 0

	//memset( &inputData, 0, sizeof(SEC_OMX_DATA) );
	//memset( &outputData, 0, sizeof(SEC_OMX_DATA) );
	//memset( &AddrInfo, 0, sizeof(MFC_ENC_ADDR_INFO) );
    if(p_SsbSipMfcEncOpen)
	   hMFCHandle = p_SsbSipMfcEncOpen();
	if( hMFCHandle == NULL )
	   return -1;
    __android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init1 is %d", pdlhandle1);
	hMFCH264Handle.hMFCHandle = hMFCHandle;
	hMFCH264Handle.bConfiguredMFC = OMX_FALSE;

	SetH264EncParam( &(hMFCH264Handle.mfcVideoAvc) );

    if(p_SsbSipMfcEncInit){
		nRet = p_SsbSipMfcEncInit( hMFCH264Handle.hMFCHandle, &(hMFCH264Handle.mfcVideoAvc) );
		if( nRet != MFC_RET_OK )  return -1;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init2 is %d", pdlhandle1);

    if(p_SsbSipMfcEncGetInBuf) {
		nRet = p_SsbSipMfcEncGetInBuf( hMFCH264Handle.hMFCHandle, &(hMFCH264Handle.inputInfo) );
		if( nRet != MFC_RET_OK ) return -1;
    }

 	inputInfo = hMFCH264Handle.inputInfo;
	hMFCH264Handle.indexTimestamp = 0;
#endif 
	__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init3 is %d", pdlhandle1);

	if(picBuf1 == NULL)
		picBuf1 = (unsigned char*)av_mallocz(avctx->width*avctx->height*4/2);

	return 0;

}


static 
int samsung_enc_frame(AVCodecContext *ctx, uint8_t *buf,
                      int orig_bufsize, void *data)
{
    int nRet = 0;
	AVFrame *frame = (AVFrame *)data;
    int lenHeader = 0;
	if((!(frame && frame->data[0])))  return 0;
    //__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init1 is %d", pdlhandle1);
	if( hMFCH264Handle.bConfiguredMFC == OMX_FALSE ) {
#if 1
		if(p_SsbSipMfcEncOpen)
		   hMFCHandle = p_SsbSipMfcEncOpen();
		if( hMFCHandle == NULL )
		   return -1;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init1 is %d", pdlhandle1);
		hMFCH264Handle.hMFCHandle = hMFCHandle;
		hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
		
		SetH264EncParam( &(hMFCH264Handle.mfcVideoAvc), ctx);

		if(p_SsbSipMfcEncInit){
			nRet = p_SsbSipMfcEncInit( hMFCH264Handle.hMFCHandle, &(hMFCH264Handle.mfcVideoAvc) );
			if( nRet != MFC_RET_OK )  return -1;
		}
		//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_init2 is %d", pdlhandle1);
		
		if(p_SsbSipMfcEncGetInBuf) {
			nRet = p_SsbSipMfcEncGetInBuf( hMFCH264Handle.hMFCHandle, &(hMFCH264Handle.inputInfo) );
			if( nRet != MFC_RET_OK ) return -1;
		}
	    inputInfo = hMFCH264Handle.inputInfo;
	    hMFCH264Handle.indexTimestamp = 0;
#endif 
		if(p_SsbSipMfcEncGetOutBuf) {
			nRet = p_SsbSipMfcEncGetOutBuf( hMFCH264Handle.hMFCHandle, &outputInfo );
			if( nRet != MFC_RET_OK )  return 0;
		}

		char* p = NULL;
		int nSpsSize = 0;
		int nPpsSize = 0;

		p = (char*)FindDelimiter( (OMX_U8*)outputInfo.StrmVirAddr+4, outputInfo.headerSize-4 );

		nSpsSize = (unsigned int)p - (unsigned int)outputInfo.StrmVirAddr;
		hMFCH264Handle.headerData.pHeaderSPS = (OMX_PTR)outputInfo.StrmVirAddr;
		hMFCH264Handle.headerData.SPSLen = nSpsSize;

		nPpsSize = outputInfo.headerSize - nSpsSize;
		hMFCH264Handle.headerData.pHeaderPPS = (OMX_PTR)outputInfo.StrmVirAddr + nSpsSize;
		hMFCH264Handle.headerData.PPSLen = nPpsSize;

        memcpy(buf, (OMX_BYTE)outputInfo.StrmVirAddr, outputInfo.headerSize);
#if 0
				FILE *f = fopen("/sdcard/2.264", "ab+");
				if(f!=NULL)
				   fwrite(buf, 1, outputInfo.headerSize, f);
				fclose(f);
				f = NULL;
#endif	
        if(outputInfo.headerSize > 0) {
			av_freep(&ctx->extradata);
			ctx->extradata		= ( uint8_t *)av_malloc(outputInfo.headerSize);
			ctx->extradata_size = outputInfo.headerSize;//encode_nals(avctx, avctx->extradata, s, nal, nnal, 1);  
			memcpy(ctx->extradata,buf,outputInfo.headerSize);
        }

		//__android_log_print(ANDROID_LOG_INFO, "tt", "outputData.dataLen is %d,%d,%d,%d,%d",*(buf+1), *(buf+2),*(buf+3),*(buf+4),outputInfo.headerSize);
		buf += outputInfo.headerSize;
		lenHeader = outputInfo.headerSize;
		//outputData.dataBuffer = (OMX_BYTE)outputInfo.StrmVirAddr;
		//outputData.allocSize = outputInfo.headerSize;
		//outputData.dataLen = outputInfo.headerSize;
		//outputData.timeStamp = inputData.timeStamp;
		//outputData.nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
		//outputData.nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

		hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

        return 0;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "outputData.dataLen is %d", lenHeader);

		//return outputData.dataLen;
	}

    if ((frame && frame->data[0])) {
        if (0 && frame->interlaced_frame == -1) { //硬件地址
			//__android_log_print(ANDROID_LOG_INFO, "tt", "data is %d,%d", frame->data[0],frame->data[1]);

            inputInfo.YPhyAddr = frame->sample_aspect_ratio.num;//ALIGN_TO_64KB((int)(frame->data[0]));
            inputInfo.CPhyAddr = frame->sample_aspect_ratio.den;//ALIGN_TO_64KB((int)(frame->data[1]));
			//__android_log_print(ANDROID_LOG_INFO, "tt", "data is %d,%d", frame->sample_aspect_ratio.num,frame->data[1]);
            nRet = p_SsbSipMfcEncSetInBuf(hMFCH264Handle.hMFCHandle, &inputInfo);
			if (nRet != MFC_RET_OK) {
				//__android_log_print(ANDROID_LOG_INFO, "tt", "error is %d", nRet);
                return 0;
            }
		} else {  //虚拟地址
		    //__android_log_print(ANDROID_LOG_INFO, "tt", "dd is %d,%d",frame->linesize[0],frame->linesize[1]);
			if(picBuf1 == NULL)
				picBuf1 = (unsigned char*)av_mallocz(ctx->width*ctx->height*4/2);
			//临时拷
			unsigned char* picY = ALIGN_TO_64KB((int)picBuf1);
			unsigned char* picU = ALIGN_TO_64KB((int)picBuf1) + ALIGN_TO_64KB(ALIGN_TO_128B(ctx->width) * ALIGN_TO_32B(ctx->height));
			unsigned char* picV = ALIGN_TO_64KB((int)picBuf1) + ALIGN_TO_64KB(ALIGN_TO_128B(ctx->width) * ALIGN_TO_32B(ctx->height))+ctx->width*ctx->height/4;
			int vuWidth = ctx->width/2;
			int vuheight = ctx->height/2;
			int vuWidthDoub = vuWidth*2;
			int vuheightDoub = vuheight*2;

			for(int i = 0; i<ctx->height; i++)
			   	memcpy(inputInfo.YVirAddr + ctx->width*i, (unsigned char*)frame->data[0]+frame->linesize[0]*i,ctx->width);
#if 1
			for(int i = 0; i < vuheight; i++) {
	            for(int j = 0; j < vuWidth; j++) {
	                *(((unsigned char *)inputInfo.CVirAddr) + vuWidthDoub*i + j*2) = *((unsigned char*)(frame->data[1]+frame->linesize[1]*i + j));
					*(((unsigned char *)inputInfo.CVirAddr) + vuWidthDoub*i + j*2 + 1) = *((unsigned char*)(frame->data[2]+frame->linesize[2]*i + j));
				}
			}
#endif
		}
		//memset(inputInfo.CVirAddr,128,vuWidth*vuheight*2);
#if 0
		for(int i = 0; i<ctx->height; i++)
		   	memcpy(picY + ctx->width*i, (unsigned char*)frame->data[0]+frame->linesize[0]*i,ctx->width);
		for(int i = 0; i < vuheight; i++) {
            memcpy(picU + vuWidth*i, (unsigned char*)frame->data[1]+frame->linesize[1]*i,vuWidth);
			memcpy(picV + vuWidth*i, (unsigned char*)frame->data[2]+frame->linesize[2]*i,vuWidth);
		}
#endif

#if 0
				FILE *f = fopen("/sdcard/1.yuv", "ab+");
				if(f!=NULL) {
				   for(int i = 0; i<ctx->height; i++)
					   fwrite((unsigned char*)frame->data[0]+frame->linesize[0]*i, 1, ctx->width, f);
				   for(int i = 0; i<ctx->height/2; i++)
					   fwrite((unsigned char*)frame->data[1]+frame->linesize[1]*i, 1, ctx->width/2, f);
				   for(int i = 0; i<ctx->height/2; i++)
					   fwrite((unsigned char*)frame->data[2]+frame->linesize[2]*i, 1, ctx->width/2, f);
				}
				fclose(f);
				f = NULL;
#endif

#if 0

	    inputInfo.YPhyAddr = picY;
		inputInfo.CPhyAddr = picU;
		if(p_SsbSipMfcEncSetInBuf) {
			nRet = p_SsbSipMfcEncSetInBuf( hMFCH264Handle.hMFCHandle, &inputInfo );
			if( nRet != MFC_RET_OK )  return 0;
		}
#else
        //memcpy(inputInfo.YVirAddr, picY, ALIGN_TO_64KB(ALIGN_TO_128B(ctx->width) * ALIGN_TO_32B(ctx->height)));
        //memcpy(inputInfo.CVirAddr, picU, ctx->width*ctx->height/2);

#endif
        //if(p_SsbSipMfcEncSetConfig)
		  // p_SsbSipMfcEncSetConfig( hMFCH264Handle.hMFCHandle, MFC_ENC_SETCONF_FRAME_TAG, &(indexTimestamp));

		indexTimestamp++;
        indexTimestamp %= MAX_TIMESTAMP;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "p_SsbSipMfcEncExe is %d", nRet);
		if(p_SsbSipMfcEncExe) {        
			nRet = p_SsbSipMfcEncExe( hMFCH264Handle.hMFCHandle );
			if( nRet != MFC_RET_OK )  return 0;
		}
		//__android_log_print(ANDROID_LOG_INFO, "tt", "p_SsbSipMfcEncExe1 is %d", nRet);

		if(p_SsbSipMfcEncGetOutBuf) {
			nRet = p_SsbSipMfcEncGetOutBuf( hMFCH264Handle.hMFCHandle, &outputInfo );
			if( nRet != MFC_RET_OK )  return 0;
		}
        //__android_log_print(ANDROID_LOG_INFO, "tt", "p_SsbSipMfcEncExe2 is %d", outputInfo.dataSize);
		memcpy(buf, (OMX_BYTE)outputInfo.StrmVirAddr, outputInfo.dataSize);

#if 0
				FILE *f = fopen("/sdcard/2.264", "ab+");
				if(f!=NULL)
				   fwrite(buf, 1, outputInfo.dataSize, f);
				fclose(f);
				f = NULL;
#endif	
		if (buf[0]==0&&buf[1]==0&&buf[2]==0&&buf[3]==1&&((buf[4]&0x0f)==0x05)) {
			frame->key_frame = 1;
			frame->pict_type = AV_PICTURE_TYPE_I;
		}
		else {
			frame->key_frame = 0;
			frame->pict_type = AV_PICTURE_TYPE_P;
		}

		ctx->coded_frame = frame;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_frame is %d,%d,%d,%d,%d,%d,%d,%d",outputInfo.dataSize+lenHeader,ctx->width,ctx->height,*(buf+2), *(buf+3), \
		//	                                                                               *(buf+4),*(buf+5), outputInfo.frameType);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "p_SsbSipMfcEncExe2 is %d", nRet);

		return outputInfo.dataSize+lenHeader;
		//outputData.dataBuffer = (OMX_BYTE)outputInfo.StrmVirAddr;
		//outputData.allocSize = outputInfo.dataSize;
		//outputData.dataLen = outputInfo.dataSize;

    }


	return 0;
}


static 
av_cold int samsung_enc_close(AVCodecContext *avctx)
{
    if(hMFCH264Handle.hMFCHandle != NULL) {
	   if(p_SsbSipMfcEncClose) {
		   p_SsbSipMfcEncClose(hMFCH264Handle.hMFCHandle);
		   hMFCH264Handle.hMFCHandle = NULL;
	   }
	}

    if(picBuf1 != NULL){
	   av_freep(&picBuf1);
	   picBuf1 = NULL;
	}

	//__android_log_print(ANDROID_LOG_INFO, "tt", "samsung_enc_close %d", pdlhandle1);
    return 0;
}

AVCodec ff_samsung264_encoder = {"libsamsungenc", AVMEDIA_TYPE_VIDEO, CODEC_ID_H264, 0, samsung_enc_init, samsung_enc_frame, samsung_enc_close
};


#if 0
AVCodec ff_samsung264_encoder = {
    .name           = "libsamsungenc",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H264,
    .priv_data_size = 0,
    .init           = samsung_enc_init,
    .encode         = samsung_enc_frame,
    .close          = samsung_enc_close,
    //.capabilities   = CODEC_CAP_DELAY | CODEC_CAP_AUTO_THREADS,
    .long_name      = NULL,//NULL_IF_CONFIG_SMALL("libsamsung H.264"),
    .priv_class     = NULL,
    .defaults       = NULL,
    .init_static_data = NULL,
};
#endif
