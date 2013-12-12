



#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"

#include <stdbool.h>
//extern{

#include "QcomOmxPublicInterface.h" //QCOMÓ²¼þ±àÂë½Ó

void 		 *g_encoder = NULL;
encoderParams g_params;
long 		  g_timeStamp;

//}

static av_cold int QCOM_H264_init(AVCodecContext *avctx) 
{

 return 0;
}

static int QCOM_H264_frame(AVCodecContext *ctx, uint8_t *buf,
                      int orig_bufsize, void *data)
{
 
 return 0;
}

static av_cold int QCOM_H264_close(AVCodecContext *avctx)
{

 return 0;
}



static const enum PixelFormat pix_fmts_8bit_nv[] = {
    PIX_FMT_NV12
};

AVCodec ff_qcom264_encoder = {
    .name           = "qcom264enc",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_H264,
    .priv_data_size = 0,//sizeof(X264Context),
    .init           = QCOM_H264_init,
    .encode         = QCOM_H264_frame,
    .close          = QCOM_H264_close,
    .capabilities   = CODEC_CAP_DELAY,//| CODEC_CAP_AUTO_THREADS,
    .long_name      = NULL_IF_CONFIG_SMALL("Qcom hardware enc H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"),
    //.priv_class     = &class,
    //.defaults       = NULL,
    //.init_static_data = NULL,
    .pix_fmts       = pix_fmts_8bit_nv,
};


