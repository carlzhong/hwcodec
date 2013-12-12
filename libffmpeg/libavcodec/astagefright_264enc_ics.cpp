#include <binder/ProcessState.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <utils/List.h>
#include <new>
#include <android/log.h>


#ifndef   UINT64_C
#define   UINT64_C(value)__CONCAT(value,ULL)
#endif

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avcodec.h"
#include "internal.h"
}

using namespace android;


struct Frame {
    status_t status;
    size_t size;
    int64_t time;
    int key;
    uint8_t *buffer;
    MediaBuffer* mbuffer;
    int32_t w, h;
};

class CustomEncSource;


struct StagefrightEncContext {
    AVCodecContext *avctx;
    AVBitStreamFilterContext *bsfc;
    uint8_t* orig_extradata;
    int orig_extradata_size;
    sp<MediaSource> *source;
    List<Frame*> *in_queue, *out_queue;
    pthread_mutex_t in_mutex, out_mutex;
    pthread_cond_t condition;
    pthread_t encoder_thread_id;

    Frame *end_frame;
    bool source_done;
    volatile sig_atomic_t thread_started, thread_exited, stop_encode;

    AVFrame ret_frame;

    uint8_t *dummy_buf;
    int dummy_bufsize;

    OMXClient *client;
    sp<MediaSource> *encoder;
    const char *encoder_component;
	int pps_sps_packed;
};



class CustomEncSource : public MediaSource {
public:
    CustomEncSource(AVCodecContext *avctx, sp<MetaData> meta) {
        s = (StagefrightEncContext*)avctx->priv_data;
        source_meta = meta;
        frame_size  = (avctx->width * avctx->height * 3) / 2;
        buf_group.add_buffer(new MediaBuffer(frame_size));
    }

    virtual sp<MetaData> getFormat() {
        return source_meta;
    }

    virtual status_t start(MetaData *params) {
        return OK;
    }

    virtual status_t stop() {
        return OK;
    }

   
    virtual status_t read(MediaBuffer **buffer,
					  const MediaSource::ReadOptions *options) {
		Frame *frame;
		status_t ret = OK;

		if (s->thread_exited)
            return ERROR_END_OF_STREAM;
        pthread_mutex_lock(&s->in_mutex);

		while (s->in_queue->empty())
            pthread_cond_wait(&s->condition, &s->in_mutex);

        frame = *s->in_queue->begin();
        ret = frame->status;
		//__android_log_print(ANDROID_LOG_INFO, "tt", "read-1 is %d", ret);
        if (ret == OK) {
            ret = buf_group.acquire_buffer(buffer);
            if (ret == OK) {
                memcpy((*buffer)->data(), frame->buffer, frame->size);
                (*buffer)->set_range(0, frame->size);
                (*buffer)->meta_data()->clear();
                (*buffer)->meta_data()->setInt32(kKeyIsSyncFrame,frame->key);
                (*buffer)->meta_data()->setInt64(kKeyTime, frame->time);
				//__android_log_print(ANDROID_LOG_INFO, "tt", "read is %d", frame->size);
            } else {
                av_log(s->avctx, AV_LOG_ERROR, "Failed to acquire MediaBuffer\n");
            }
            av_freep(&frame->buffer);
        }


		s->in_queue->erase(s->in_queue->begin());
        pthread_mutex_unlock(&s->in_mutex);

        av_freep(&frame);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "read end");
		return ret;
    }
	
private:
	MediaBufferGroup buf_group;
	sp<MetaData> source_meta;
	StagefrightEncContext *s;
	int frame_size;

};


static 
av_cold int Stagefright_Ics_H264_init(AVCodecContext *avctx)
{
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init is %d",avctx->priv_data);

    StagefrightEncContext *s = (StagefrightEncContext*)avctx->priv_data;
	s->avctx = avctx;
	int ret = 0;
    if(!avctx)  return -1;
	
	sp<MetaData> enc_meta = new MetaData;
	

	if (enc_meta == NULL) {
        return -1;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init is %d,%d,%d",avctx->pix_fmt,\
		                                    avctx->width,avctx->height);
    if(avctx->pix_fmt == PIX_FMT_YUV422P) //type 6
		enc_meta->setInt32(kKeyBitRate, 8002000);  //android?μí3?a192000
	else
        enc_meta->setInt32(kKeyBitRate, 1002000);  //android?μí3?a192000
        
    enc_meta->setInt32(kKeyFrameRate, 25);
	//enc_meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RAW);
    enc_meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);//264±à??

	enc_meta->setInt32(kKeyWidth, avctx->width);
    enc_meta->setInt32(kKeyHeight, avctx->height);
    enc_meta->setInt32(kKeyIFramesInterval, 1);//gop′óD?
    enc_meta->setInt32(kKeyStride, avctx->width);
    enc_meta->setInt32(kKeySliceHeight, avctx->height);

	__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright is %d,%d",avctx->width,avctx->height);

	//enc_meta->setInt32(kKeyDisplayWidth, avctx->width);
	//enc_meta->setInt32(kKeyDisplayHeight, avctx->height);

	//enc_meta->setInt32(kKeyColorFormat, OMX_QCOM_COLOR_FormatYVU420SemiPlanar); //420p OMX_COLOR_FormatYUV420Planar  OMX_COLOR_FormatYUV420SemiPlanar
	//enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel13); 
	/*huawei u9200:OMX_TI_COLOR_FormatYUV420PackedSemiPlanar*/
#if 0
    if(avctx->pix_fmt == PIX_FMT_YUV422P) {
		enc_meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420Planar); //420p OMX_COLOR_FormatYUV420Planar  OMX_COLOR_FormatYUV420SemiPlanar
		enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel4); 
    } else if(avctx->pix_fmt == PIX_FMT_YUYV422) {//huawei
       enc_meta->setInt32(kKeyColorFormat, OMX_TI_COLOR_FormatYUV420PackedSemiPlanar); //420p OMX_COLOR_FormatYUV420Planar  OMX_COLOR_FormatYUV420SemiPlanar
	   enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel4); 
    } else if(PIX_FMT_RGB24 == avctx->pix_fmt) {
	   enc_meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420SemiPlanar); 
	   enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel13); 
    } else if(PIX_FMT_BGR24 == avctx->pix_fmt) {  //vivo
	   enc_meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420Planar); 
	   enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel13); 
    } else if(PIX_FMT_YUV444P == avctx->pix_fmt) {  //i9100g
	   enc_meta->setInt32(kKeyColorFormat, OMX_TI_COLOR_FormatYUV420PackedSemiPlanar); 
	   enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel4); 
    } else {
	   enc_meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420SemiPlanar); //420p OMX_COLOR_FormatYUV420Planar	OMX_COLOR_FormatYUV420SemiPlanar
	   enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel13); //这两个参数要测试下
	}
#endif

	enc_meta->setInt32(kKeyColorFormat, OMX_COLOR_FormatYUV420SemiPlanar); //420p OMX_COLOR_FormatYUV420Planar  OMX_COLOR_FormatYUV420SemiPlanar
	enc_meta->setInt32(kKeyVideoLevel, OMX_VIDEO_AVCLevel4); 

    //enc_meta->setInt32(kKeyTimeScale, mVideoTimeScale);
    if (PIX_FMT_YUV444P == avctx->pix_fmt)
		enc_meta->setInt32(kKeyVideoProfile, OMX_VIDEO_AVCProfileMain);
	else
       enc_meta->setInt32(kKeyVideoProfile, OMX_VIDEO_AVCProfileBaseline);
	//huawei  OMX_VIDEO_AVCLevel4
	

	//enc_meta->setInt32(kKeyMaxInputSize, avctx->width*avctx->height*1.5);

	android::ProcessState::self()->startThreadPool();

	uint32_t encoder_flags = 0;
	encoder_flags |= OMXCodec::kHardwareCodecsOnly;  //???üó2?t±à??
	//encoder_flags |= OMXCodec::kClientNeedsFramebuffer;

	
    //encoder_flags |= OMXCodec::kStoreMetaDataInVideoBuffers;
	//encoder_flags |= OMXCodec::kOnlySubmitOneInputBufferAtOneTime;//′y?é?¤
    s->pps_sps_packed = 0;
	s->source    = new sp<MediaSource>();
    *s->source   = new CustomEncSource(avctx, enc_meta);

	s->in_queue  = new List<Frame*>;
    s->out_queue = new List<Frame*>;
    s->client    = new OMXClient;
	s->end_frame = (Frame*)av_mallocz(sizeof(Frame));

	if (s->source == NULL || !s->in_queue || !s->out_queue || !s->client || !s->end_frame) {
		ret = -1;
        goto fail;
    }
	
    //__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init1 is %d,%d",avctx->width,
		//              avctx->height);

	if (s->client->connect() !=  OK) {
        ret = -1;
        goto fail;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init2");

	s->encoder  = new sp<MediaSource>();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init22");
    //enc_meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);//264±à??
	*s->encoder = OMXCodec::Create(
            s->client->interface(), enc_meta,
            true /* createEncoder */, *s->source,
            NULL, encoder_flags);
	
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init3 ");

	//(*s->encoder)->getFormat()->findCString(kKeyDecoderComponent, (const char **)(&(avctx->stats_out)));

	if ((*s->encoder)->start() !=  OK) {
        //av_log(avctx, AV_LOG_ERROR, "Cannot start decoder\n");
        ret = -1;
        s->client->disconnect();
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init45");
        goto fail;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init4");

    pthread_mutex_init(&s->in_mutex, NULL);
    pthread_mutex_init(&s->out_mutex, NULL);
    pthread_cond_init(&s->condition, NULL);

	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_init3 %s",avctx->stats_out);
    return 0;

fail:
    //av_bitstream_filter_close(s->bsfc);
    //av_freep(&s->orig_extradata);
    //av_freep(&s->end_frame);
    delete s->in_queue;
    delete s->out_queue;
    delete s->client;
    return ret;
}


void* encode_thread(void *arg)
{
	int encode_done = 0;
	AVCodecContext *avctx = (AVCodecContext*)arg;
	StagefrightEncContext *s = (StagefrightEncContext*)avctx->priv_data;
	Frame* frame;
    MediaBuffer *buffer;

    do {
		buffer = NULL;
        frame = (Frame*)av_mallocz(sizeof(Frame));
        if (!frame) {
            frame         = s->end_frame;
            frame->status = AVERROR(ENOMEM);
            encode_done   = 1;
            s->end_frame  = NULL;
        } else {
            //__android_log_print(ANDROID_LOG_INFO, "tt", "encode_thread0 is %d",s->stop_encode);
            frame->status = (*s->encoder)->read(&buffer);
			if (frame->status == OK) {
                //sp<MetaData> outFormat = (*s->decoder)->getFormat();
                //outFormat->findInt32(kKeyWidth , &frame->w);
                //outFormat->findInt32(kKeyHeight, &frame->h);
                frame->size    = buffer->range_length();
                frame->mbuffer = new MediaBuffer(frame->size);  //buffer;
                (buffer)->meta_data()->findInt64(kKeyTime, &(frame->time));
				(buffer)->meta_data()->findInt32(kKeyIsSyncFrame, &(frame->key));
                if(frame->size>0) {
	                uint8_t *databuf_src = (uint8_t*)buffer->data();
					uint8_t *databuf_dst = (uint8_t*)frame->mbuffer->data();
					memcpy(databuf_dst,databuf_src,frame->size);
                }
				buffer->release();
            }   else {
                encode_done = 1;
            }
		}

        while (true) {
            pthread_mutex_lock(&s->out_mutex);
            if (s->out_queue->size() >= 10) {
                pthread_mutex_unlock(&s->out_mutex);
                usleep(10000);
                continue;
            }
            break;
        }
		//__android_log_print(ANDROID_LOG_INFO, "tt", "encode_thread3 is %d",s->stop_encode);
        s->out_queue->push_back(frame);
        pthread_mutex_unlock(&s->out_mutex);
	} while (!encode_done && !s->stop_encode);

	s->thread_exited = true;
    return 0;
}

static 
int Stagefright_Ics_H264_frame(AVCodecContext *ctx, uint8_t *buf,
                      int orig_bufsize, void *data)
{
    StagefrightEncContext *s = (StagefrightEncContext*)ctx->priv_data;
    MediaBuffer *mbuffer;
	status_t status;
    size_t size;
	AVFrame *inframe = (AVFrame *)data;
	Frame *frame;
	
    if(!s->thread_started) {
        pthread_create(&s->encoder_thread_id, NULL, &encode_thread, ctx);
        s->thread_started = true;
	}
    //__android_log_print(ANDROID_LOG_INFO, "pp", "s->source_done0 is %d",s->source_done);
	if(!s->source_done) {
        //frame = (Frame*)av_mallocz(sizeof(Frame));
		if (inframe && inframe->data[0]) {
			frame = (Frame*)av_mallocz(sizeof(Frame));
            frame->status  = OK;
            frame->size    = inframe->width*inframe->height*1.5;
			//__android_log_print(ANDROID_LOG_INFO, "pp", "Stagefright_Ics_H264_frame1 is %lld",inframe->pts);
			if (inframe->pts >= 0)
                frame->time    = inframe->pts;
			
			frame->key     = inframe->key_frame;//& AV_PKT_FLAG_KEY ? 1 : 0;
			frame->buffer  = (uint8_t*)av_malloc(frame->size);
            if (!frame->buffer) {
                av_freep(&frame);
                return AVERROR(ENOMEM);
            }
#if 0
						FILE *f = fopen("/sdcard/1.yuv", "ab+");
						if(f!=NULL){
							fwrite(inframe->data[0], 1, inframe->width*inframe->height, f);
							fwrite(inframe->data[1], 1, inframe->width*inframe->height/4, f);
							fwrite(inframe->data[2], 1, inframe->width*inframe->height/4, f);
							//fwrite(pict->data[2], 1, inWidth*inHeight, f);
						}
						fclose(f);
						f = NULL;
#endif

			memcpy(frame->buffer, inframe->data[0], inframe->width*inframe->height);//DèòaDT??
			memcpy(frame->buffer+inframe->width*inframe->height, inframe->data[1], inframe->width*inframe->height/4);
			memcpy(frame->buffer+inframe->width*inframe->height*5/4, inframe->data[2], inframe->width*inframe->height/4);
		} else {
            //frame->status  = ERROR_END_OF_STREAM;
            //s->source_done = true;
			//__android_log_print(ANDROID_LOG_INFO, "pp", "s->source_done is %d",s->source_done);
			goto OUT;
		}
        
		while (true) {
            if (s->thread_exited) {
                s->source_done = true;
                break;
            }
            pthread_mutex_lock(&s->in_mutex);
            if (s->in_queue->size() >= 10) {
				//__android_log_print(ANDROID_LOG_INFO, "tt", "s->in_queue->size is %d",s->in_queue->size());
                pthread_mutex_unlock(&s->in_mutex);
                usleep(10000);
                continue;
            }
            s->in_queue->push_back(frame);
            pthread_cond_signal(&s->condition);
            pthread_mutex_unlock(&s->in_mutex);
            break;
        }
	}

OUT:
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_frame1 is %lld");

	while (true) {
        pthread_mutex_lock(&s->out_mutex);
        if (!s->out_queue->empty()) break;
        pthread_mutex_unlock(&s->out_mutex);
        if (s->source_done) {
            usleep(10000);
            continue;
        } else {
            //__android_log_print(ANDROID_LOG_INFO, "pp", "s->source_done1 is %d",s->source_done);
            return 0; //??óD±à??o?μ?êy?Y
        }
    }    
   
    //__android_log_print(ANDROID_LOG_INFO, "pp", "s->source_done12 is %d",s->source_done);

	frame = *s->out_queue->begin();
    s->out_queue->erase(s->out_queue->begin());
    pthread_mutex_unlock(&s->out_mutex);
	
    mbuffer = frame->mbuffer;
    status  = frame->status;
    size    = frame->size;
	inframe->pts = frame->time;
    av_freep(&frame);

	//(*buffer)->meta_data()->setInt64(kKeyTime, frame->time);
	//__android_log_print(ANDROID_LOG_INFO, "pp", "s->source_done13 is %d",s->source_done);
    if (status == ERROR_END_OF_STREAM)  {
        return 0;
	}
	
    if (mbuffer) {
		uint8_t *databuf = (uint8_t*)mbuffer->data();
		if(size>0) {
			/*
			int packed_size = 0;
			//在第一帧前插入PPS SPS		
			if ((databuf[0]==0&&databuf[1]==0&&databuf[2]==0&&databuf[3]==1&&((databuf[4]&0x0f)==0x05)) \
				  && (!s->pps_sps_packed) && (ctx->extradata_size>0)) {
                            s->pps_sps_packed = 1;
				memcpy(buf, ctx->extradata, ctx->extradata_size);
				buf += ctx->extradata_size;	
				packed_size = ctx->extradata_size;
				__android_log_print(ANDROID_LOG_INFO, "pp", "Stagefright_Ics_H264_frame3333 is %d",packed_size);
			}
			*/
			//__android_log_print(ANDROID_LOG_INFO, "pp", "Stagefright_Ics_H264_frame3 is %d,%x,%x,%x,%x,%x,%x,%x",size,databuf[0],\
			//                                           databuf[1],databuf[2],databuf[3],databuf[4],databuf[5],databuf[6]);
            if((*((int *)databuf)) == 0) { //防止开始的第一个I的前缀码为多个0
                 while(size>0) {
                    databuf++;
					if(databuf[0]==0&&databuf[1]==0&&databuf[2]==0&&databuf[3]==1) break;
					size--;
				 }
			}
			memcpy(buf, databuf, size);
			//size += packed_size;
		} 
			
		mbuffer->release();
    } else {
        return 0;
	}

	if (buf[0]==0&&buf[1]==0&&buf[2]==0&&buf[3]==1&&((buf[4]&0x0f)==0x05)) {
		inframe->key_frame = 1;
		inframe->pict_type = AV_PICTURE_TYPE_I;
	}
	else {
		inframe->key_frame = 0;
		inframe->pict_type = AV_PICTURE_TYPE_P;
	}

	
	ctx->coded_frame = inframe;
	if (ctx->flags & CODEC_FLAG_GLOBAL_HEADER && (buf[0]==0&&buf[1]==0&&buf[2]==0&&buf[3]==1&&((buf[4]&0x0f)==0x07))) {
		av_freep(&ctx->extradata);
		ctx->extradata		= ( uint8_t *)av_malloc(size);
		ctx->extradata_size = size;//encode_nals(avctx, avctx->extradata, s, nal, nnal, 1);  
		memcpy(ctx->extradata,buf,size);
		return 0;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Stagefright_Ics_H264_frame2 is %lld",inframe->pts);

	//__android_log_print(ANDROID_LOG_INFO, "pp", "Stagefright_Ics_H264_frame3 is %x,%x,%d,%d,%d",*((int *)buf),buf[4],\
	 	                 //size,mbuffer,size);
#if 0
	FILE *f = fopen("/sdcard/2.264", "ab+");
	if(f!=NULL)
	   fwrite(buf, 1, size, f);
	fclose(f);
	f = NULL;
#endif	

    return size;
}


static 
av_cold int Stagefright_Ics_H264_close(AVCodecContext *avctx)
{
    StagefrightEncContext *s = (StagefrightEncContext*)avctx->priv_data;
    Frame *frame;

	if (s->thread_started) {
        if (!s->thread_exited) {
            s->stop_encode = 1;

		    pthread_mutex_lock(&s->out_mutex);
	        while (!s->out_queue->empty()) {
	            frame = *s->out_queue->begin();
	            s->out_queue->erase(s->out_queue->begin());
	            if (frame->size)
	                frame->mbuffer->release();
	            av_freep(&frame);
	        }
	        pthread_mutex_unlock(&s->out_mutex);

		    pthread_mutex_lock(&s->in_mutex);
	        s->end_frame->status = ERROR_END_OF_STREAM;
	        s->in_queue->push_back(s->end_frame);
	        pthread_cond_signal(&s->condition);
	        pthread_mutex_unlock(&s->in_mutex);
	        s->end_frame = NULL;
		}

		pthread_join(s->encoder_thread_id, NULL);
		s->thread_started = false;
	}


	while (!s->in_queue->empty()) {
        frame = *s->in_queue->begin();
        s->in_queue->erase(s->in_queue->begin());
        if (frame->size)
            av_freep(&frame->buffer);
        av_freep(&frame);
    }

    while (!s->out_queue->empty()) {
        frame = *s->out_queue->begin();
        s->out_queue->erase(s->out_queue->begin());
        if (frame->size)
            frame->mbuffer->release();
        av_freep(&frame);
    }

	(*s->encoder)->stop();
    s->client->disconnect();

	if (avctx->extradata)
	    av_freep(&avctx->extradata);

    delete s->in_queue;
    delete s->out_queue;
    delete s->client;
    delete s->encoder;
    delete s->source;

    pthread_mutex_destroy(&s->in_mutex);
    pthread_mutex_destroy(&s->out_mutex);
    pthread_cond_destroy(&s->condition);

	return 0;
}


AVCodec ff_libstagefright_ics_h264_encoder = {
	    "stagefrighticsenc", 
		AVMEDIA_TYPE_VIDEO, 
		CODEC_ID_H264, 
		sizeof(StagefrightEncContext), 
		Stagefright_Ics_H264_init, 
		Stagefright_Ics_H264_frame, 
		Stagefright_Ics_H264_close,
		NULL,
		CODEC_CAP_DELAY
};

