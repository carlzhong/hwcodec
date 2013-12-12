#include"RealTimeEnc.h"
extern "C" {
extern int Samsung_H264_encodeIsAvailable();
}
//extern int QCOM_H264_encodeIsAvailable();

//encoder performance test                add by jjf
#define PERFORMANCE_TEST 0   
#if PERFORMANCE_TEST
#include <sys/time.h>   //add by jjf
#endif

#define LISTINIT(list,m_frame_size_,BUFSIZE) \
list.readpos = 0;  \
list.size = 0; \
list.writepos = 0; \
for(int i = 0; i < BUFSIZE; i++) { \
	if(list.framebuf[i].buf == NULL) { \
	   list.framebuf[i].buf = (unsigned char*)malloc(m_frame_size_); \
	   list.framebuf[i].bufsize = m_frame_size_; \
	   list.framebuf[i].pts = 0;  \
	} \
} \
pthread_mutex_init (&(list.frame_buf_mutex), NULL); 
#undef malloc
#undef free


CRealTimeEnc* CRealTimeEnc::m_encoder_handler_ = NULL;

/*
AVFrame* CRealTimeEnc::alloc_picture(enum PixelFormat pix_fmt, int width, int height) {
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = avcodec_alloc_frame();
	if (!picture)
		return NULL;
	size = avpicture_get_size(pix_fmt, width, height);
	picture_buf = (uint8_t *)av_malloc(size);
	if (!picture_buf) {
		av_free(picture);
	return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
	return picture;
}
*/
/*
int CRealTimeEnc::setup_output_context(const char *filename) {
	AVOutputFormat *ofmt = NULL;
	ofmt = av_guess_format("mp4", NULL, NULL);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setup_output_context %s",filename);

	m_pOutContext_ = avformat_alloc_context();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setup_output_context1 %s",filename);

	if(!m_pOutContext_) {
	return -1;
	}

	if (m_pOutContext_->oformat->priv_data_size > 0) {
		m_pOutContext_->priv_data = av_mallocz(m_pOutContext_->oformat->priv_data_size);
		if (!m_pOutContext_->priv_data) {
			avformat_free_context(m_pOutContext_);
			return -1;
		}

		if (m_pOutContext_->oformat->priv_class) {
			*(const AVClass**)m_pOutContext_->priv_data= m_pOutContext_->oformat->priv_class;
			av_opt_set_defaults(m_pOutContext_->priv_data);
	       }
 	} else
		m_pOutContext_->priv_data = NULL;
    
	if(filename)
		av_strlcpy(m_pOutContext_->filename, filename, sizeof(m_pOutContext_->filename));
	
	return 0;
}*/


AVCodec* CRealTimeEnc::find_codec_or_die(const char *name, 
										   enum AVMediaType type, 
										   int encoder) {
	AVCodec *codec;
	codec = encoder ?
	avcodec_find_encoder_by_name(name) :
	avcodec_find_decoder_by_name(name);
	return codec;
}


AVStream* CRealTimeEnc::add_video_stream(AVFormatContext *oc, 
											enum CodecID codec_id, 
											s_encparam encP) {
	AVCodecContext *c;
	AVStream *st;
	AVCodec *codec;	
	st = avformat_new_stream(oc, NULL);
	if (!st) {
		return NULL;
	}
	c = st->codec;
	codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
	/*
	//硬件编码
	if(m_encoder_level_ == 0) {
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else if (encP.encodeType == -1 && Samsung_H264_encodeIsAvailable()) {
		codec = find_codec_or_die("libsamsungenc", AVMEDIA_TYPE_VIDEO, 1);
	} else if(encP.encodeType >= 1) {
		codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
        //codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else { //软件编码
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
        }*/		
	//__android_log_print(ANDROID_LOG_INFO, "tt", "codec %d",codec);
	if (!codec) {
		return NULL;
	}
	avcodec_get_context_defaults3(c, codec);

	c->codec_id = codec_id;

	/* put sample parameters */
	c->bit_rate = 6000000;
	/* resolution must be a multiple of two */
	c->width = encP.width;
	c->height = encP.height;
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "add_video_stream  width = %d, height=%d ", encP.width,  encP.height);
	
	c->time_base.den = m_encoder_param_.audioFreq>0?m_encoder_param_.audioFreq:25;//25;//encP.fps>1?(int)(encP.fps):25;//180000;//
	c->time_base.num = 1;
	__android_log_print(ANDROID_LOG_INFO, "tt", "add_video_stream1 is %d ,%d",c->width,c->height);

	//测试旋转
	#undef sprintf
	char degree[10] = {0};
	sprintf(degree,"%d",encP.degree);
	av_dict_set(&st->metadata, "rotate", degree, 0);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "add_video_stream1 ");
	/*
	    PIX_FMT_NONE= -1,  
	    PIX_FMT_YUV420P,   normal
	    PIX_FMT_YUYV422,   huawei u9200
	    PIX_FMT_RGB24,     
	    PIX_FMT_BGR24,   
	*/
	//c->gop_size = 12; /* emit one intra frame every twelve frames at most */
	c->pix_fmt = PIX_FMT_YUV420P;
	switch(encP.encodeType) {
		case 0:
		case 1:
		case 2:
			{
				c->pix_fmt = PIX_FMT_YUV420P;
				break;
			}
					 
		case 3:
			{
				c->pix_fmt = PIX_FMT_YUYV422;
				break;
			}
					 
		case 4:  //需要降分辨率
			{
				c->pix_fmt = PIX_FMT_RGB24;
				break;
			}

		case 5: //vivo mtk
			{
				c->pix_fmt = PIX_FMT_BGR24;
				break;
			}
		case 6:
			{
				c->pix_fmt = PIX_FMT_YUV422P;
				break;
			}
		case 7:
			{
				c->pix_fmt = PIX_FMT_YUV444P;
				break;

		    }
		default:
			{
				c->pix_fmt = PIX_FMT_YUV420P;
				break;
			}
	}

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}


AVStream* CRealTimeEnc::add_audio_stream(AVFormatContext *oc, 
											enum CodecID codec_id) {
	AVCodecContext *c;
	AVStream *st;

	st = avformat_new_stream(oc, NULL);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "add_audio_stream %d",st);
	if (!st) {
		return NULL;
	}
	//st->id = 1;

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	/* put sample parameters */
	c->sample_fmt = AV_SAMPLE_FMT_S16;  //由m_encoder_param_.audiobitDepth决定
	c->bit_rate = 128000;//12200;
	c->sample_rate = m_encoder_param_.audioFreq;//  8000;//44100;
	c->channels = m_encoder_param_.audioChannels;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "m_encoder_param_.audioFreq %d,%d",m_encoder_param_.audioFreq,m_encoder_param_.audioChannels);
	//c->profile = FF_PROFILE_AAC_LOW;
	c->time_base.den = m_encoder_param_.audioFreq;//8000;
	c->time_base.num = 1;

	// some formats want stream headers to be separate
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}


int CRealTimeEnc::open_video(AVFormatContext *oc, AVStream *st,s_encparam encP) {
	AVCodec *codec;
	AVCodecContext *c;

	c = st->codec;
	
	//add bitrate setting   by jiangjinfeng
	c->bit_rate = encP.bitrate;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc bitrate = %d", encP.encodeType);


	codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
	/*
	//硬件编码
	if(m_encoder_level_ == 0) {
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else if (encP.encodeType == -1 && Samsung_H264_encodeIsAvailable()) {
		codec = find_codec_or_die("libsamsungenc", AVMEDIA_TYPE_VIDEO, 1);
	} else if(encP.encodeType >= 1) {
		codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
		__android_log_print(ANDROID_LOG_INFO, "tt", "find_codec_or_die= %d", codec);
		//codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else  //软件编码
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);

      */
	/* find the video encoder */
	//codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		return -1;
	}

	/* open the codec */
	if (avcodec_open(c, codec) < 0) {
		return -1;
	}
   
	if(m_video_outbuf_ == NULL) {
		m_video_outbuf_size_ = 1000000;
		m_video_outbuf_ = (uint8_t *)av_malloc(m_video_outbuf_size_);
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "find_codec_or_die1= %d", codec);
	return 0;
}



int CRealTimeEnc::open_audio(AVFormatContext *oc, AVStream *st) {
	AVCodecContext *c;
	AVCodec *codec;
	int err;
	c = st->codec;
	c->strict_std_compliance = -2;

    //获取FDKAAC的编码器
    //__android_log_print(ANDROID_LOG_INFO, "tt", "open_audio %d",codec);
	codec = find_codec_or_die("libfdk_aac", AVMEDIA_TYPE_AUDIO, 1);
	//c->codec_id = codec->id;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "open_audio1 %d",codec);

	/* find the audio encoder */
	//codec = avcodec_find_encoder(CODEC_ID_AMR_NB);
	if (!codec) return -1;
	
   
	/* open it */
	if ((err = avcodec_open(c, codec)) < 0) {
		return -1;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "open_audio2 %d",c);

	//audio_input_frame_size = c->frame_size;
	return 0;
}


int CRealTimeEnc::join(void **ppValue) {
	int err;
	err =  (pthread_join(m_tid_audio_, ppValue) != 0)? 1 : 0;
	
	err =  (pthread_join(m_tid_, ppValue) != 0)? 1 : 0;
	//m_state_video_ = READY;
	return err;
}


void CRealTimeEnc::fill_rgba_image(s_framebuf_video_list *list, 
									char *data, 
									int width, 
									int height, 
									__uint64_t pts) {
	int x, y, i;
	char *Y = (char *)(list->framebuf[list->writepos].buf);
	char *U = (char *)(list->framebuf[list->writepos].buf+width*height);
	char *V = (char *)(list->framebuf[list->writepos].buf+width*height*5/4);
	char *r = data+width*height;
	//char *g = data + width*height;
	//char *b = data + 2*width*height;
	//memcpy(Y,r,width*height*1.5);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "Y = %x, U = %x, V = %x, r =  %x", Y,U,V,r);
	
	//yuv  soft enc
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_proc is %d,%d",width,height);
	if (m_encoder_level_ == 0) { 
	    //__android_log_print(ANDROID_LOG_INFO, "tt", "fill_rgba_image is %d,%d,%d,%d,%d",Y,U,V,data,list->writepos);
	    memcpy(Y, data, width*height);
		
		for(int i = 0; i<height/2; i++) {
			for(int j = 0; j<width/2; j++) {
				*(U+i*width+j*2+1) = *(r+j*2+i*width);
				*(U+i*width+j*2) = *(r+j*2+i*width+1);
				//*(V+i*width/2+j) = *(r+j*2+i*width+1);
			}
		}

	     //memcpy(Y, data, width*height*1.5);
		 //__android_log_print(ANDROID_LOG_INFO, "tt", "fill_rgba_image1 is %d,%d,%d",Y,U,V);
#if 0
		int startgop = (m_encoder_width_-m_encoder_height_)/2;
		for(int i = 0; i < m_encoder_height_; i++)
			memcpy(Y+i*m_encoder_height_,r+i*m_encoder_width_+startgop,m_encoder_height_);
			//memcpy(Y,r+(Encwidth-Encheight)*Encwidth/2,Encheight*Encheight);
		U = (char *)(list->framebuf[list->writepos].buf+m_encoder_height_*m_encoder_height_);
		V = (char *)(list->framebuf[list->writepos].buf+m_encoder_height_*m_encoder_height_*5/4);
		//memset(U,128,Encheight*Encheight/4);
		//memset(V,128,Encheight*Encheight/4);
		r = data + m_encoder_width_*m_encoder_height_;
		int startgop1 = (m_encoder_width_/2-m_encoder_height_/2);
		for(int i = 0; i<m_encoder_height_/2; i++) {
			for(int j = 0; j<m_encoder_height_/2; j++) {
				*(U+i*m_encoder_height_/2+j) = *(r+j*2+i*m_encoder_width_+startgop1+1);
				*(V+i*m_encoder_height_/2+j) = *(r+j*2+i*m_encoder_width_+startgop1);
			}
		}
#endif
	} else {
		switch(m_encoder_param_.encodeType){
			case 0: //YV12
			case 5: //mtk
			case 6:
				//__android_log_print(ANDROID_LOG_INFO, "jjf", "encodeType 0 ");
				//__android_log_print(ANDROID_LOG_INFO, "jjf", "width = %d, height=%d ", width,height );
				{
					for(int i = 0; i<height; i++)
						for(int j = 0; j<width; j++) {
							*(Y+i*width+j) = *(r+i*width*4+j*4);
						}
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
							*(U+i*width/2+j) = *(r+i*width*8+j*8+1);
						}		
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
							*(V+i*width/2+j) = *(r+i*width*8+j*8+2);
						}
					break;
				}
			
			case 2:
			case 4:
				//__android_log_print(ANDROID_LOG_INFO, "jjf", "encodeType 1 ");
				{
					//samsung  nv21?
					for(int i = 0; i<height; i++)
						for(int j = 0; j<width; j++) {
							*(Y+i*width+j) = *(r+i*width*4+j*4);
						}
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
							*((U+i*width+j*2)) = *((r+i*width*8+j*8+2));
							*((U+i*width+j*2+1)) = *((r+i*width*8+j*8+1));
						}	
					break;
				}


			case 1:
			case 3:
			case 7://i9100g
				//__android_log_print(ANDROID_LOG_INFO, "jjf", "encodeType 2 ");
				{
					//nv12   hard enc
					for(int i = 0; i<height; i++)
						for(int j = 0; j<width; j++) {
							*(Y+i*width+j) = *(r+i*width*4+j*4);
						}
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
							*((short *)(U+i*width+j*2)) = *((short*)(r+i*width*8+j*8+1));
						}	
					break;
				}

			default:
				//__android_log_print(ANDROID_LOG_INFO, "jjf", "encodeType 3 ");
				{
					for(int i = 0; i<height; i++)
						for(int j = 0; j<width; j++) {
							*(Y+i*width+j) = *(r+i*width*4+j*4);
						}
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
							*(U+i*width/2+j) = *(r+i*width*8+j*8+1);
							*(V+i*width/2+j) = *(r+i*width*8+j*8+2);
						}	

					break;
				}
		}
	}

	list->framebuf[list->writepos].bufsize = width*height*1.5;
	list->framebuf[list->writepos].pts = pts;
	list->size++;
	list->writepos++;
	list->writepos = list->writepos%VIDEOFRAMEBUFSIZE;
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "fill_rgba_image22222 ");	
}


int CRealTimeEnc::realtime_enc_set_videodata(char *data, int type, __uint64_t pts) {
#if 0
	FILE *f = fopen("/sdcard/1.yuv", "ab+");
	fwrite(data, 1, 480*320*1.5, f);
	fclose(f);
#endif 
/*
	if(m_nExit_ == 1) return 0;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc is %d",pause);
	if (m_nPause_==1) {
		if(m_pause_flag_==0) {
			m_pause_flag_=!m_pause_flag_;
			m_last_pts_ = pts;
		}	
		return 0;
	}
	if (m_pause_flag_==1){
		m_pause_flag_=!m_pause_flag_;
		m_pause_gap_ += pts-m_last_pts_;
	}

	pts -= m_pause_gap_;
*/

	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc1 is %lld,%lld",lastpts,c_pts);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc is %lld,%lld",pts,c_pts);
	if(m_state_video_ != RUNNING) return 0;
	int ret  = pthread_mutex_lock (&(m_video_frame_buffer_list_.frame_buf_mutex));
	if (m_pOutContext_ == NULL || m_video_stream_ == NULL) {
		ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}
	if (!m_video_outbuf_) {
		ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc pts is %lld",c_pts);
	if(!data)  {
		ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}

    /*
	static clock_t starttime = clock();
	static int videocout = 0;
	videocout++;
	clock_t endtime = clock();
	if (endtime > starttime+1000000)
	    __android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc0 is %d,%d",videocout/((int)((endtime-starttime)*0.000001)),m_video_frame_buffer_list_.size);
       */
    if (firstpts_video < 0) firstpts_video = pts;

	pts -= firstpts_video;
	
	__uint64_t c_pts = (m_encoder_param_.audioFreq>0?m_encoder_param_.audioFreq:25)\
	                      *(((double)pts)/1000000);//25*(((double)pts)/1000000);

	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc pts is %lld,%lld",c_pts,pts);
    if(c_pts > lastpts) {
		lastpts = c_pts;        
	} else {
		c_pts = lastpts+1;
	    lastpts = c_pts;
	}
	
    //videocout++;
	//if (c_pts-firstpts>=25*4) {
        //fps_test = videocout/(((float)(c_pts-firstpts))/25);
	//}

	//__android_log_print(ANDROID_LOG_INFO, "tt", "99999999999 is %d",fps_test);
	
	AVCodecContext *c;
	c = m_video_stream_->codec;

	if(m_video_frame_buffer_list_.size >= VIDEOFRAMEBUFSIZE) {
		ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
		//__android_log_print(ANDROID_LOG_INFO, "jjf", "99999999999 is %d",data);
		return 0;
	}

	if(m_video_frame_buffer_list_.size > 50) {
       usleep(500);
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_proc  is %d,%lld,%lld", \
						//m_video_frame_buffer_list_.size,c_pts,pts);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_videoproc is %d,%d,%d",data,c->width,c->height);
	if (m_video_frame_buffer_list_.framebuf[m_video_frame_buffer_list_.writepos].buf != NULL) {
		
		fill_rgba_image(&m_video_frame_buffer_list_, (char*)data, c->width, c->height,c_pts);
	}

	ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));


	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_set_videodata  is %lld", pts);
	return 0;
}


int CRealTimeEnc::realtime_enc_set_audiodata(char *data, int len, __uint64_t pts) {
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_audioproc is %lld", pts);
  
	if(m_nExit_ == 1) return -1;
	//if (m_nPause_ == 1) return -1;
	if (m_state_audio_ != RUNNING) return -1;

	if(firstpts_audio < 0) firstpts_audio = pts;

	audioSampleNum++;

	pts -= firstpts_audio;

	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_audioproc is %lld,%lld,%d",\
		//                      pts, lastpts_audio, audioSampleNum);

	if(pts > lastpts_audio) {
		lastpts_audio = pts;        
	} else {
		pts = lastpts_audio+1;//(audioSampleNum>1)?(lastpts_audio+(pts/(audioSampleNum-1))):(lastpts_audio+1);
	    lastpts_audio = pts;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_audioproc11 is %lld,%lld,%d",\
		                      //pts, lastpts_audio, audioSampleNum);
	int ret  = pthread_mutex_lock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
	if (m_pOutContext_ == NULL || m_audio_stream_ == NULL) {
		ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}

	if (!m_audio_stream_) {
		ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}
  
	if (!data)  {
		ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}

	AVCodecContext *c;
	c = m_audio_stream_->codec;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_audioproc is %d,%d,%d,%d", m_audio_frame_buffer_list_.readpos,\
		//                  m_audio_frame_buffer_list_.writepos, len,data);

	if (m_audio_frame_buffer_list_.size >= AUDIOFRAMEBUFSIZE) {
		ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
		return -1;
	}


	if (m_audio_frame_buffer_list_.framebuf[m_audio_frame_buffer_list_.writepos].buf != NULL) {
		memcpy(m_audio_frame_buffer_list_.framebuf[m_audio_frame_buffer_list_.writepos].buf, \
				(char *)data,len*2);
		m_audio_frame_buffer_list_.framebuf[m_audio_frame_buffer_list_.writepos].bufsize = len*2;
		m_audio_frame_buffer_list_.size++;
		m_audio_frame_buffer_list_.framebuf[m_audio_frame_buffer_list_.writepos].pts = pts;
		m_audio_frame_buffer_list_.writepos++;
		m_audio_frame_buffer_list_.writepos = m_audio_frame_buffer_list_.writepos%AUDIOFRAMEBUFSIZE;
	}

	ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeEnc_audioproc end is %lld", pts);
	return 0;
}


/*
int CRealTimeEnc::Pause()
{
	m_nPause_ = !m_nPause_;
	return 0;
}*/

int CRealTimeEnc::realtime_enc_stop() {
	void* pThdRslt;
	int err;
	int ret;
	m_nExit_=1;

	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_stop ");	 
	if(m_pOutContext_==NULL) return -1;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_stop ");	 
	err = join(&pThdRslt);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "err=%d ",err);
	if(err)
		return -1;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_exit ");
	ret = pthread_mutex_lock (&(m_video_frame_buffer_list_.frame_buf_mutex));
	if(m_pOutContext_)
		av_write_trailer(m_pOutContext_);
	ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init22is %d",m_video_frame_buffer_list_.size); 

	ret = pthread_mutex_lock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
	if(m_audio_stream_)
		avcodec_close(m_audio_stream_->codec);
	ret = pthread_mutex_unlock (&(m_audio_frame_buffer_list_.frame_buf_mutex));
	 
	// __android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_exit ");
	ret = pthread_mutex_lock (&(m_video_frame_buffer_list_.frame_buf_mutex));
	if(m_video_stream_)
		avcodec_close(m_video_stream_->codec);
	 
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d",m_video_frame_buffer_list_.size);
	if(m_video_outbuf_) {
		av_free(m_video_outbuf_);
		m_video_outbuf_ = NULL;
		m_video_outbuf_size_ = 0;
	}
	
	if(m_pOutContext_) {
		for(int i = 0; i < m_pOutContext_->nb_streams; i++) {
			av_freep(&m_pOutContext_->streams[i]->codec);
			av_freep(&m_pOutContext_->streams[i]);
		}
		 
		if (m_pOutContext_->pb) {
			 /* close the output file */
			avio_close(m_pOutContext_->pb);
		}
		 
		/* free the stream */
		av_free(m_pOutContext_);
		//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d,%d",g_video_framebuflist.size,oc_);
		m_pOutContext_ = NULL;
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d",g_video_framebuflist.size);
	
	for(int i = 0; i < VIDEOFRAMEBUFSIZE; i++) {
		if(m_video_frame_buffer_list_.framebuf[i].buf) {
			free(m_video_frame_buffer_list_.framebuf[i].buf);
			m_video_frame_buffer_list_.framebuf[i].bufsize = 0;
			m_video_frame_buffer_list_.framebuf[i].buf = NULL;
		}		 
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init122 is %d",g_video_framebuflist.size);

	for(int i = 0; i < AUDIOFRAMEBUFSIZE; i++) {
		 if(m_audio_frame_buffer_list_.framebuf[i].buf) {
			free(m_audio_frame_buffer_list_.framebuf[i].buf);
			m_audio_frame_buffer_list_.framebuf[i].bufsize = 0;
			m_audio_frame_buffer_list_.framebuf[i].buf = NULL;
		}		 
	}

	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_stop is ");

	ret = pthread_mutex_unlock (&(m_video_frame_buffer_list_.frame_buf_mutex));
	//pthread_mutex_destroy(&(g_video_framebuflist.frame_buf_mutex));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init2 is %d",g_video_framebuflist.size); 

	return fps_test;  //如果帧率无效，就返回-1
}

CRealTimeEnc::CRealTimeEnc() {
	//m_pinContext = NULL;
	m_pOutContext_ = NULL;
	m_video_stream_ = NULL;
	m_audio_stream_ = NULL;
	m_video_outbuf_ = NULL;
	m_video_outbuf_size_ = 0;


	//m_fT = 0;
	//m_fTincr = 0;
	//m_fTincr2 = 0;
 	m_frame_size_ = 0;
	m_nExit_ = 0;
	m_nPause_ = 0;
	//m_sws_flags = SWS_BICUBIC;
	m_state_video_ = READY;
	m_state_audio_ = READY;
	m_pause_flag_ = 0;
	m_last_pts_ = 0;
	m_pause_gap_ = 0;
	//m_vidframeListSize = 20;
	//m_audframeListSize = 50;
	m_encoder_level_ = 0;
	m_encoder_width_ = 480;
	m_encoder_height_ = 480;
	m_szFileName_ = 0;

	memset( &m_encoder_param_, 0, sizeof(s_encparam));
	for(int i=0 ; i < VIDEOFRAMEBUFSIZE; i++)
		m_video_frame_buffer_list_.framebuf[i].buf = NULL;
	for(int i=0 ; i < AUDIOFRAMEBUFSIZE; i++)
		m_audio_frame_buffer_list_.framebuf[i].buf = NULL;	
}
CRealTimeEnc::~CRealTimeEnc() {

}

int CRealTimeEnc::realtime_enc_initial( s_encparam encP,int level,   char *outputfile)
{
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "realtime_enc_initial");
	try {
		m_encoder_level_ = level;

		//if (m_encoder_level_ == 0) {//适配机型dd
			//encP.encodeType = 0;
			//m_encoder_width_ = encP.width;
			//m_encoder_height_ = encP.height;
			//encP.width = encP.height;
		//}

		//if(encP.width == encP.height) {//竖拍为软编
			//encP.encodeType = 0;
		//}
		encP.encodeType = 3;//直接硬件编码
		memcpy( &m_encoder_param_, &encP, sizeof(s_encparam));
		if(m_state_video_ == RUNNING) return -1;

		m_pause_gap_ = m_last_pts_ = m_pause_flag_ = m_nExit_ = m_nPause_ = 0;
		m_szFileName_ = outputfile;

        //计算帧率相关
		starttime = -1;
		endtime = 0;
		videocout = 0;
		fps_test = -1;
		firstpts_video = -1;
		firstpts_audio = -1;
		//pauseonce = 0;
		return 0;
	}	
	catch (...)
	{
		return 2;
	}
	return 1;

}


int CRealTimeEnc::realtime_computeFps()
{
    if (starttime == -1)
        starttime = clock();

	videocout++;
	clock_t endtime = clock();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc is %lld,%lld",starttime, endtime);
	if (endtime > starttime+5000000) {
	    //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc0 is %d,%d",videocout/((int)((endtime-starttime)*0.000001)),m_video_frame_buffer_list_.size);
		fps_test = videocout/((int)((endtime-starttime)*0.000001));
	}
    return 0;
}

void* CRealTimeEnc::video_encoder_thread_function(void *param) {
	int ret;
	AVCodecContext *c;
	__uint64_t pic_pts = 0;
	__uint64_t firstpts = -1;
	int out_size;
	int got_pict = 0;
	//int VideoDataWriteIdex = 0;//add this code to deal with the bug of i9100G
	
#if PERFORMANCE_TEST	
	//define some variables of encoder performance test   by jjf
	long total_time_s=0;
	long total_time_us=0;	
	long total_outsize = 0;
	struct timeval start;
    struct timeval end;	
#endif   	 
	//强制转换成CRealTimeEnc
	CRealTimeEnc *pParam = reinterpret_cast<CRealTimeEnc *> (param);
	if (pParam->m_pOutContext_ == NULL || pParam->m_video_stream_ == NULL)
		return   NULL;

	if (!pParam->m_video_outbuf_) 
		return NULL;

	c = pParam->m_video_stream_->codec;
	pParam->m_state_video_ = RUNNING;
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_state_video_ = %d",pParam->m_state_video_ );	


	while(1) {
		if(pParam->m_nExit_ == 1 && pParam->m_video_frame_buffer_list_.size <= 0)  break;
		got_pict = 0;
		ret  = pthread_mutex_lock (&(pParam->m_video_frame_buffer_list_.frame_buf_mutex));
		//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_video_frame_buffer_list_.size = %d",pParam->m_video_frame_buffer_list_.size );
		if (pParam->m_video_frame_buffer_list_.size > 0) {
            //__android_log_print(ANDROID_LOG_INFO, "tt", "m_video_frame_buffer_list_.size = %d",pParam->m_video_frame_buffer_list_.size );			
		    if (pParam->m_video_frame_buffer_list_.framebuf[pParam->m_video_frame_buffer_list_.readpos].buf != NULL) {
				if (pParam->m_encoder_param_.encodeType == 0) {//soft
					avpicture_fill((AVPicture *)&pParam->m_picture_soft_, \
					(uint8_t *)pParam->m_video_frame_buffer_list_.framebuf[pParam->m_video_frame_buffer_list_.readpos].buf, \
					PIX_FMT_YUV420P,c->width,c->height);
				}
				else {
					avpicture_fill((AVPicture *)&pParam->m_picture_, \
					(uint8_t *)pParam->m_video_frame_buffer_list_.framebuf[pParam->m_video_frame_buffer_list_.readpos].buf, \
					PIX_FMT_YUV420P,c->width,c->height);
				}

				//fill_yuv_image(&picture, g_video_framebuflist.framebuf[g_video_framebuflist.readpos].buf, c->width, c->height);
				if(firstpts == -1)  
					firstpts = pParam->m_video_frame_buffer_list_.framebuf[pParam->m_video_frame_buffer_list_.readpos].pts;
				pic_pts = pParam->m_video_frame_buffer_list_.framebuf[pParam->m_video_frame_buffer_list_.readpos].pts - firstpts;
				pParam->m_video_frame_buffer_list_.size--;
				pParam->m_video_frame_buffer_list_.readpos++;
				pParam->m_video_frame_buffer_list_.readpos = pParam->m_video_frame_buffer_list_.readpos%VIDEOFRAMEBUFSIZE;
				got_pict = 1;

				//__android_log_print(ANDROID_LOG_INFO, "tt", "list read pos %d,%d",pParam->m_video_frame_buffer_list_.readpos,\
					      //         pParam->m_video_frame_buffer_list_.size);
			}
		}
		ret = pthread_mutex_unlock (&(pParam->m_video_frame_buffer_list_.frame_buf_mutex));
		if (got_pict == 0) {
			usleep(50);
			continue;
		}
	    //__android_log_print(ANDROID_LOG_INFO, "jjf", "GOT PICT " );		
		if (pParam->m_encoder_param_.encodeType == 0) { //soft
			pParam->m_picture_soft_.pts = pic_pts;
			pParam->m_picture_soft_.width = c->width;
			pParam->m_picture_soft_.height = c->height;

			/*__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out1 is %d,%d,%d,%d", \
			picture.data[0],picture.data[1],picture.linesize[0],picture.linesize[1]);*/
#if PERFORMANCE_TEST
			//index the start time of encoder   by jjf			
			gettimeofday(&start,NULL);
#endif
		

	

			out_size = avcodec_encode_video(c, pParam->m_video_outbuf_, \
										pParam->m_video_outbuf_size_, \
										&pParam->m_picture_soft_);
			//__android_log_print(ANDROID_LOG_INFO, "tt", "out_size = %d ", out_size);				
			//__android_log_print(ANDROID_LOG_INFO, "tt", "avcodec_encode_video start is %d,%d,%d,%d,%lld",c->width,c->height,\
			                        // pParam->m_video_frame_buffer_list_.size, out_size,pic_pts);	

#if PERFORMANCE_TEST
			//compute the test data  of encoding one frame  by jjf			
			total_outsize+=out_size;//jjf
			gettimeofday(&end,NULL);//jjf
			total_time_s=1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;//jjf
			total_time_us += total_time_s;//jjf
			//__android_log_print(ANDROID_LOG_INFO, "jjf", "frame_num=%d,time%d",c->frame_number,total_time_s);//jjf
#endif			
		} else { 
			pParam->m_picture_.pts = pic_pts;
			pParam->m_picture_.width = c->width;
			pParam->m_picture_.height = c->height;
			/*__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out1 is %d,%d,%d,%d", \
								picture.data[0],picture.data[1],picture.linesize[0],picture.linesize[1]);*/
			out_size = avcodec_encode_video(c, pParam->m_video_outbuf_, \
										pParam->m_video_outbuf_size_, \
										&pParam->m_picture_);
		}	
        //__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out is %d,%d,%d,%d", \
        	//					g_video_framebuflist.size, \
        	//					encParam_s.encodeType, \
        	//					encParam_s.width,\
			//                                encParam_s.height);
	    //if (out_size > 0 && c->coded_frame->key_frame && 0==VideoDataWriteIdex)//add this code to deal with the bug of i9100G 
			//VideoDataWriteIdex = 1;

		if (out_size > 0 /*&& VideoDataWriteIdex*/) {
			AVPacket pkt;
			av_init_packet(&pkt);
	        
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, pParam->m_video_stream_->time_base);
			
			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			//__android_log_print(ANDROID_LOG_INFO, "tt", "videoproc pts is %lld",pkt.pts);
			pkt.stream_index = pParam->m_video_stream_->index;
			pkt.data = pParam->m_video_outbuf_;
			pkt.size = out_size;

            //pParam->realtime_computeFps();
			
			//FILE *tmpf = fopen("/sdcard/DCIM/1.264", "ab+");
			//fwrite(pkt.data, 1, out_size, tmpf);
            //fflush(tmpf);
			//fclose(tmpf);
		    //__android_log_print(ANDROID_LOG_INFO, "tt", "video pts = %lld",pkt.pts);
			pthread_mutex_lock (&(pParam->m_av_write_mutex_));
	        ret = av_interleaved_write_frame(pParam->m_pOutContext_, &pkt);
			pthread_mutex_unlock (&(pParam->m_av_write_mutex_));
			//__android_log_print(ANDROID_LOG_INFO, "tt", "list read pos %d",pParam->m_video_frame_buffer_list_.readpos);
			av_free_packet(&pkt);
    	}
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc_thread1 is %d",g_video_framebuflist.size);
	pParam->m_state_video_ = THREADEXIT;


#if PERFORMANCE_TEST
	//printf the test results   by jjf	
	total_time_s = total_time_us/c->frame_number;
	__android_log_print(ANDROID_LOG_INFO, "tt", "enc_time per frame= %d",total_time_s);
	__android_log_print(ANDROID_LOG_INFO, "tt", "total_outsize= %d",total_outsize);
#endif



	
	//flush delayed frames
	for(int i = 0; out_size > 0; i++) {
		AVFrame tmp;		
		if(pParam->m_encoder_param_.encodeType == 0 || pParam->m_encoder_param_.encodeType == -1) //soft or note1 
			out_size = avcodec_encode_video(c, pParam->m_video_outbuf_, pParam->m_video_outbuf_size_, NULL);
		else  //hard
			out_size = avcodec_encode_video(c, pParam->m_video_outbuf_, pParam->m_video_outbuf_size_, &tmp);
		
		//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc_thread1 is %d",out_size);
		if (out_size > 0) {
			AVPacket pkt;
			av_init_packet(&pkt);
	        
			if (c->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts= av_rescale_q(c->coded_frame->pts, \
									c->time_base, \
									pParam->m_video_stream_->time_base);

			if(c->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index = pParam->m_video_stream_->index;
			pkt.data = pParam->m_video_outbuf_;
			pkt.size = out_size;
			pthread_mutex_lock (&(pParam->m_av_write_mutex_));
			ret = av_interleaved_write_frame(pParam->m_pOutContext_, &pkt);
			pthread_mutex_unlock (&(pParam->m_av_write_mutex_));
			av_free_packet(&pkt);
		}
	}
		
	
}


void *CRealTimeEnc::audio_encoder_thread_function(void *param) {
	//强制转换成CRealTimeEnc
	CRealTimeEnc *pParam = reinterpret_cast<CRealTimeEnc *> (param);
	if (pParam->m_pOutContext_ == NULL || pParam->m_audio_stream_ == NULL)
		return NULL;

	#define AUDIO_BUFSIZE 100000
	uint8_t samples[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3)/2] = {0};
	uint8_t tmpbuf[400];//max size 320
	int tmpbuf_size = 0;
	uint8_t audio_outbuf[AUDIO_BUFSIZE];
	int audio_outbuf_size = AUDIO_BUFSIZE;
	int ret;
	AVCodecContext *c;
	__uint64_t ptss = 0;
	__uint64_t last_audio_pts = 0;
	int out_size;
	int got_audio = 0;
	int i=0;
	c = pParam->m_audio_stream_->codec;
	int samplesize = 0;//audio_input_frame_size * 2 * 2 / (samplesize);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "AUDIO ID = %d",pParam->m_tid_audio_ );
	pParam->m_state_audio_ = RUNNING;
	while(1) {
		if(pParam->m_nExit_ == 1)  
			break;	
		got_audio = 0;
		ret  = pthread_mutex_lock (&(pParam->m_audio_frame_buffer_list_.frame_buf_mutex));
		if (pParam->m_audio_frame_buffer_list_.size > 0) {
			if (pParam->m_audio_frame_buffer_list_.framebuf[pParam->m_audio_frame_buffer_list_.readpos].buf != NULL) {
				if(tmpbuf_size>0) 
					memcpy(samples,tmpbuf,tmpbuf_size*2);
				memcpy(samples+tmpbuf_size*2, \
					pParam->m_audio_frame_buffer_list_.framebuf[pParam->m_audio_frame_buffer_list_.readpos].buf,\
					pParam->m_audio_frame_buffer_list_.framebuf[pParam->m_audio_frame_buffer_list_.readpos].bufsize);
				samplesize = tmpbuf_size + \
						     (pParam->m_audio_frame_buffer_list_.framebuf[pParam->m_audio_frame_buffer_list_.readpos].bufsize/2);
				tmpbuf_size = 0;
				ptss = pParam->m_audio_frame_buffer_list_.framebuf[pParam->m_audio_frame_buffer_list_.readpos].pts;
				pParam->m_audio_frame_buffer_list_.size--;
				pParam->m_audio_frame_buffer_list_.readpos++;
				pParam->m_audio_frame_buffer_list_.readpos = pParam->m_audio_frame_buffer_list_.readpos%AUDIOFRAMEBUFSIZE;
				got_audio = 1;
			}
		}
		ret = pthread_mutex_unlock (&(pParam->m_audio_frame_buffer_list_.frame_buf_mutex));
		if(got_audio == 0) {
			usleep(50);
			continue;
		}

		i = 0;
		while (samplesize >= c->channels*c->frame_size){
			//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread");
			out_size = avcodec_encode_audio(c, audio_outbuf, AUDIO_BUFSIZE, (short *)(samples+i*c->channels*c->frame_size*2));
			i++;
			samplesize -= c->channels*c->frame_size;

			//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread  is %d,%d,%d",pParam->m_audio_frame_buffer_list_.size,c->frame_size,out_size);

			if (out_size > 0) {
				AVPacket pkt;
				av_init_packet(&pkt);
			        
				//if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE) 
					//pkt.pts= av_rescale_q(c->coded_frame->pts, \
										//c->time_base, \
										//pParam->m_audio_stream_->time_base);
										
				pkt.pts= av_rescale_q(ptss, \
										AV_TIME_BASE_Q, \
										c->time_base);
				//pkt.pts *= 90;

				//__android_log_print(ANDROID_LOG_INFO, "tt", "audioproc pts is %lld,%lld,%d,%d,%d",pkt.pts,ptss,\
					                //c->time_base.den,c->time_base.num,samplesize);
			    if(pkt.pts > last_audio_pts) {
                    last_audio_pts = pkt.pts;
				} else {
					pkt.pts = last_audio_pts+100;
                    last_audio_pts = pkt.pts;
				}
				//__android_log_print(ANDROID_LOG_INFO, "tt", "audioproc pts is %lld,%lld,%d,%d,%d",pkt.pts,last_audio_pts,\
					//                c->time_base.den,c->time_base.num,samplesize);

				//__android_log_print(ANDROID_LOG_INFO, "tt", "audio pts = %lld,%lld,%d,%d,%d,%d", pkt.pts,ptss,\
					//                 c->time_base.den,c->time_base.num,pParam->m_audio_stream_->time_base.den,\
					  //               pParam->m_audio_stream_->time_base.num);
					
				pkt.flags |= AV_PKT_FLAG_KEY;
				pkt.stream_index = pParam->m_audio_stream_->index;
				pkt.data = audio_outbuf;
				pkt.size = out_size;

				/* write the compressed frame in the media file */
				pthread_mutex_lock (&(pParam->m_av_write_mutex_));
				ret = av_interleaved_write_frame(pParam->m_pOutContext_, &pkt);
				pthread_mutex_unlock (&(pParam->m_av_write_mutex_));
					
				av_free_packet(&pkt);
				//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread end");
		    	}
				//delete []readbuff;
        	}
		if(samplesize>0 && samplesize<c->channels*c->frame_size){
			tmpbuf_size = samplesize;
			memcpy(tmpbuf,samples+i*c->channels*c->frame_size*2,tmpbuf_size*2);
		}
	}

	//av_fifo_free(fifo); 
	pParam->m_state_audio_ = THREADEXIT;
}


int CRealTimeEnc::realtime_enc_start()
{
	int nErr = 0;
	lastpts = 0;
	lastpts_audio = 0;
	audioSampleNum = 0;
	av_register_all();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_start %d, %d, %d %s", \
					//m_encoder_param_.encodeType, \
					//m_encoder_param_.width,m_encoder_param_.height,m_szFileName_);
	avformat_alloc_output_context2(&m_pOutContext_, NULL, NULL, m_szFileName_);
	/*__android_log_print(ANDROID_LOG_INFO, "jjf", "Realtime_enc_start1 %llf %d %d %s", \
					m_encoder_param_.fps, m_encoder_param_.width, \
					m_encoder_param_.height,m_szFileName_);*/
	m_video_stream_ = add_video_stream(m_pOutContext_, CODEC_ID_H264, m_encoder_param_);
	if(m_video_stream_ == NULL) return -1;
	/*__android_log_print(ANDROID_LOG_INFO, "jjf", "Realtime_enc_start1 %llf %d %d %s", \
					m_encoder_param_.fps, \
					m_encoder_param_.width, \
					m_encoder_param_.height, \
					m_szFileName_);*/	
	m_audio_stream_ = add_audio_stream(m_pOutContext_, CODEC_ID_AAC);
	if(m_audio_stream_ == NULL) return -1;

	//__android_log_print(ANDROID_LOG_INFO, "tt", "Realtime_enc_start2 %llf %d %d %s", \
					//m_encoder_param_.fps, \
					//m_encoder_param_.width, \
					//m_encoder_param_.height, \
					//m_szFileName_);
	av_dump_format(m_pOutContext_, 0, m_szFileName_, 1);
	if (m_video_stream_)
		nErr = open_video(m_pOutContext_, m_video_stream_, m_encoder_param_);	
	if(nErr < 0) 
		return -1;
	
	if (m_audio_stream_)
		nErr = open_audio(m_pOutContext_, m_audio_stream_);
	
	if(nErr < 0) return -1;
	if(avio_open(&m_pOutContext_->pb, m_szFileName_, AVIO_FLAG_WRITE) < 0) {
		return -1;
	}

    if(avformat_write_header(m_pOutContext_, NULL)) {
		return -1;
	}
	
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init3  %d %d %s",m_encoder_param_.width,\
		//                                         m_encoder_param_.height,m_szFileName_);
	//video buf  init
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_video_frame_buffer_list_0: %x",m_video_frame_buffer_list_.framebuf[0].buf);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_audio_frame_buffer_list_0: %x",m_audio_frame_buffer_list_.framebuf[0].buf);	
	if(m_encoder_level_ == 0) {
		LISTINIT(m_video_frame_buffer_list_, m_encoder_param_.width * m_encoder_param_.height * 1.5,VIDEOFRAMEBUFSIZE)
	} else {
		LISTINIT(m_video_frame_buffer_list_, m_encoder_param_.width * m_encoder_param_.height * 1.5,VIDEOFRAMEBUFSIZE)
	}
	LISTINIT(m_audio_frame_buffer_list_,20000,AUDIOFRAMEBUFSIZE)
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_video_frame_buffer_list_1: %x",m_video_frame_buffer_list_.framebuf[0].buf);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "m_audio_frame_buffer_list_1: %x",m_audio_frame_buffer_list_.framebuf[0].buf);

	pthread_mutex_init (&(m_av_write_mutex_), NULL);
	
	m_state_video_ = READY;
	m_state_audio_ = READY;	
	if(pthread_attr_init(&m_ptAttr_) != 0){
		return -1;
 	}


	pthread_attr_setdetachstate(&m_ptAttr_,PTHREAD_CREATE_JOINABLE);
	//__android_log_print(ANDROID_LOG_INFO, "jjf", "video_encoder_thread_function");
	if(pthread_create(&m_tid_, &m_ptAttr_, CRealTimeEnc::video_encoder_thread_function, this) != 0) {
		//__android_log_print(ANDROID_LOG_INFO, "jjf", "pthread_create video_encoder_thread_function error");
		return -1;
	}	
	if(pthread_attr_init(&m_ptAttr_audio_) != 0){
		return -1;
 	}

	pthread_attr_setdetachstate(&m_ptAttr_audio_,PTHREAD_CREATE_JOINABLE);
	__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_start0");
 	if(pthread_create(&m_tid_audio_, &m_ptAttr_audio_, CRealTimeEnc::audio_encoder_thread_function, this) != 0) {
		//__android_log_print(ANDROID_LOG_INFO, "jjf", "pthread_create audio_encoder_thread_function error");
		return -1;
	}
	__android_log_print(ANDROID_LOG_INFO, "tt", "realtime_enc_start");
	
	return 0;
}


void* CRealTimeEnc::__RealTimeEncConstructer()
{
   try {
   	 // __android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer is %d",m_handler);
     if(m_encoder_handler_ == NULL)
   	    m_encoder_handler_ = new CRealTimeEnc();
      //__android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer1");
     return m_encoder_handler_;
   } catch(...) {
     return NULL;
   }
}

