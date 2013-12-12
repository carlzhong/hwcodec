#include <libavformat/avformat.h>

#include "ffmpeg.h"
#include <android/log.h>
#include <pthread.h>
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
#include "realtimeEnc.h"


enum ProcessState {
	  READY = 0,
	  RUNNING = 1,
	  THREADEXIT = 2,
	  PROCESSEXIT
};


AVFormatContext* ic_ = NULL;
AVFormatContext* oc_ = NULL;
AVStream *video_st = NULL;
AVStream *audio_st = NULL;
static uint8_t *video_outbuf = NULL;
static int video_outbuf_size = 0;
static float t, tincr, tincr2;
static AVFrame picture;//=NULL;// *tmp_picture;
static AVFrame picture_soft;//=NULL;// *tmp_picture;


pthread_t m_tid_;
pthread_attr_t m_ptAttr_;
pthread_t m_tid_audio_;
pthread_attr_t m_ptAttr_audio_;
static int framesize = 0;
static int exit = 0;
static int pause = 0;
static int sws_flags = SWS_BICUBIC;
enum ProcessState m_state_video_ = READY;
enum ProcessState m_state_audio_ = READY;
extern int qcomErr;
//extern int QCOM_H264_encodeIsAvailable();
extern int Samsung_H264_encodeIsAvailable();

static int pause_flag = 0;
static __uint64_t lastpts = 0;
static __uint64_t pause_gap = 0;

static s_framebuf_video_list g_video_framebuflist;
static s_framebuf_audio_list g_audio_framebuflist;
pthread_mutex_t av_write_mutex;

s_encparam encParam_s;
static int vidframeListSize = 20;
static int audframeListSize = 50;
static int Enclevel = 0;
static int Encwidth = 480;
static int Encheight = 480;
#define LISTINIT(list,framesize,BUFSIZE) \
list.readpos = 0;  \
list.size = 0; \
list.writepos = 0; \
for(int i = 0; i < BUFSIZE; i++) { \
	if(list.framebuf[i].buf == NULL) { \
	   list.framebuf[i].buf = (unsigned char*)malloc(framesize); \
	   list.framebuf[i].bufsize = framesize; \
	   list.framebuf[i].pts = 0;  \
	} \
} \
pthread_mutex_init (&(list.frame_buf_mutex), NULL); 
	

#undef malloc
#undef free

static 
AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);
    return picture;
}


static 
int setup_output_context(const char *filename)
{
	AVOutputFormat *ofmt = NULL;
	ofmt = av_guess_format("mp4", NULL, NULL);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setup_output_context %s",filename);

    oc_ = avformat_alloc_context();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setup_output_context1 %s",filename);

	if(!oc_) {
       return -1;
	}

	if (oc_->oformat->priv_data_size > 0) {
       oc_->priv_data = av_mallocz(oc_->oformat->priv_data_size);
	   if (!oc_->priv_data) {
           avformat_free_context(oc_);
		   return -1;
	   }

	   if (oc_->oformat->priv_class) {
            *(const AVClass**)oc_->priv_data= oc_->oformat->priv_class;
            av_opt_set_defaults(oc_->priv_data);
       }
 	} else
       oc_->priv_data = NULL;
    
	if(filename)
	   av_strlcpy(oc_->filename, filename, sizeof(oc_->filename));
	
	return 0;
}

static 
AVCodec *find_codec_or_die(const char *name, enum AVMediaType type, int encoder)
{
    const char *codec_string = encoder ? "encoder" : "decoder";
    AVCodec *codec;

    codec = encoder ?
        avcodec_find_encoder_by_name(name) :
        avcodec_find_decoder_by_name(name);
    return codec;
}


static 
AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id, s_encparam encP)
{
    AVCodecContext *c;
    AVStream *st;
    AVCodec *codec;
	
    st = avformat_new_stream(oc, NULL);
    if (!st) {
        return NULL;
    }

	c = st->codec;

	//硬件编码
	if(Enclevel == 0) {
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else if (encP.encodeType == -1 && Samsung_H264_encodeIsAvailable()) {
		codec = find_codec_or_die("libsamsungenc", AVMEDIA_TYPE_VIDEO, 1);
	} else if(encP.encodeType >= 1) {
        codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
        //codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else  //软件编码
        codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
		
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
	//__android_log_print(ANDROID_LOG_INFO, "tt", "add_video_stream ");
	
    c->time_base.den = 25;//encP.fps>1?(int)(encP.fps):25;//180000;//
    c->time_base.num = 1;

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
			   	     c->pix_fmt = PIX_FMT_YUV420P;
					 break;
					 
               case 3:
			   	     c->pix_fmt = PIX_FMT_YUYV422;
			   	     break;
					 
			   case 4:  //需要降分辨率
				     c->pix_fmt = PIX_FMT_RGB24;
				     break;

			   case 5: //vivo mtk
			         c->pix_fmt = PIX_FMT_BGR24;
					 break;
			   case 6:
			   	     c->pix_fmt = PIX_FMT_YUV422P;
					 break;
			   default:
			   	     c->pix_fmt = PIX_FMT_YUV420P;
					 break;
	}

	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}


static 
AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id)
{
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
    c->sample_fmt = AV_SAMPLE_FMT_S16;
    c->bit_rate = 5900;//12200;
    c->sample_rate = 8000;
    c->channels = 1;
	//c->profile = FF_PROFILE_AAC_LOW;
	c->time_base.den = 8000;
    c->time_base.num = 1;

    // some formats want stream headers to be separate
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}


static 
int open_video(AVFormatContext *oc, AVStream *st,s_encparam encP)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

	//硬件编码
	if(Enclevel == 0) {
		codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else if (encP.encodeType == -1 && Samsung_H264_encodeIsAvailable()) {
		codec = find_codec_or_die("libsamsungenc", AVMEDIA_TYPE_VIDEO, 1);
	} else if(encP.encodeType >= 1) {
        codec = find_codec_or_die("stagefrighticsenc", AVMEDIA_TYPE_VIDEO, 1);
        //codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);
	} else  //软件编码
        codec = find_codec_or_die("libx264", AVMEDIA_TYPE_VIDEO, 1);


    /* find the video encoder */
    //codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        return -1;
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        return -1;
    }
   
    if(video_outbuf == NULL) {
       video_outbuf_size = 1000000;
	   video_outbuf = av_malloc(video_outbuf_size);
	}
	return 0;
}


static 
int open_audio(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVCodec *codec;
    int err;
    c = st->codec;
	c->strict_std_compliance = -2;

    /* find the audio encoder */
    codec = avcodec_find_encoder(CODEC_ID_AMR_NB);
    if (!codec) return -1;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "open_audio %d",c);
   
    /* open it */
    if ((err = avcodec_open(c, codec)) < 0) {
		return -1;
    }

    //audio_input_frame_size = c->frame_size;
	return 0;
}


static 
void fill_yuv_image(AVFrame *pict, char *data, int width, int height)
{
    int x, y, i;
	char *Y = data;
	char *U = data+width*height;
	char *V = data+width*height*5/4;
    pict->data[0] = data;
	pict->data[1] = data+width*height;
	pict->data[2] = data+width*height*5/4;
	pict->linesize[0] = width;
	pict->linesize[1] = width/2;
	pict->linesize[2] = width/2;
	return;
}

static 
void pgm_save(AVFrame *pict, int xsize, int ysize,
                     char *filename)
{
#undef printf
#undef fprintf

    FILE *f;
    int i;

    f=fopen("/mnt/sdcard/out.yuv","w");
    //fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
    for(i=0;i<ysize;i++)
        fwrite(pict->data[0]+ i * pict->linesize[0],1,xsize,f);
	for(i=0;i<ysize/2;i++)
        fwrite(pict->data[1]+ i * pict->linesize[1],1,xsize/2,f);
	for(i=0;i<ysize/2;i++)
        fwrite(pict->data[2]+ i * pict->linesize[2],1,xsize/2,f);
    fclose(f);
}

static 
void videoEnc_thread()
{
	int ret;
	AVCodecContext *c;
	__uint64_t pic_pts = 0;
	__uint64_t firstpts = -1;
	int out_size;
	int got_pict = 0;

	if (oc_ == NULL || video_st == NULL)
		return -1;

	if (!video_outbuf) 
		return -1;
	
	c = video_st->codec;

    m_state_video_ = RUNNING;
    while(1) {
		if(exit == 1 && g_video_framebuflist.size <= 0)  break;
		
		got_pict = 0;
		ret  = pthread_mutex_lock (&(g_video_framebuflist.frame_buf_mutex));
		if (g_video_framebuflist.size > 0) {
		    if (g_video_framebuflist.framebuf[g_video_framebuflist.readpos].buf != NULL) {
				if (encParam_s.encodeType == 0) //soft
				    avpicture_fill(&picture_soft,(uint8_t *)g_video_framebuflist.framebuf[g_video_framebuflist.readpos].buf,PIX_FMT_YUV420P,c->width,c->height);
			    else
					avpicture_fill(&picture,(uint8_t *)g_video_framebuflist.framebuf[g_video_framebuflist.readpos].buf,PIX_FMT_YUV420P,c->width,c->height);


				//fill_yuv_image(&picture, g_video_framebuflist.framebuf[g_video_framebuflist.readpos].buf, c->width, c->height);
				if(firstpts == -1)  firstpts = g_video_framebuflist.framebuf[g_video_framebuflist.readpos].pts;
				pic_pts = g_video_framebuflist.framebuf[g_video_framebuflist.readpos].pts - firstpts;
				g_video_framebuflist.size--;
				g_video_framebuflist.readpos++;
				g_video_framebuflist.readpos = g_video_framebuflist.readpos%VIDEOFRAMEBUFSIZE;
				got_pict = 1;
			}
		}
        ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
		if(got_pict == 0) {
           usleep(50);
		   continue;
		}

		if(encParam_s.encodeType == 0) { //soft
			picture_soft.pts = pic_pts;
			picture_soft.width = c->width;
			picture_soft.height = c->height;
			//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out1 is %d,%d,%d,%d",picture.data[0],picture.data[1],picture.linesize[0],picture.linesize[1]);
			out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, &picture_soft);
		} else { 
			picture.pts = pic_pts;
			picture.width = c->width;
			picture.height = c->height;
			//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out1 is %d,%d,%d,%d",picture.data[0],picture.data[1],picture.linesize[0],picture.linesize[1]);
			out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, &picture);
		}
        //__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out is %d,%d,%d,%d",g_video_framebuflist.size,encParam_s.encodeType,encParam_s.width,\
			                                                                      encParam_s.height);
		if (out_size > 0) {
	        AVPacket pkt;
	        av_init_packet(&pkt);
	        
	        if (c->coded_frame->pts != AV_NOPTS_VALUE)
	            pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
			
			if(c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
			
	        pkt.stream_index = video_st->index;
	        pkt.data = video_outbuf;
	        pkt.size = out_size;
		    //__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc out is %x,%x,%x,%x,%x",pkt.data[0],pkt.data[1],pkt.data[2],pkt.data[3],\
		   	  //                                     pkt.data[4]);
            pthread_mutex_lock (&(av_write_mutex));
	        ret = av_interleaved_write_frame(oc_, &pkt);
			pthread_mutex_unlock (&(av_write_mutex));
			av_free_packet(&pkt);
    	}
	}
	//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc_thread1 is %d",g_video_framebuflist.size);
	m_state_video_ = THREADEXIT;
	//flush delayed frames
    for (int i = 0; out_size > 0; i++) {
		AVFrame tmp;
		
		if(encParam_s.encodeType == 0 || encParam_s.encodeType == -1) //soft or note1 
            out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, NULL);
		else  //hard
			out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, &tmp);
		
		//__android_log_print(ANDROID_LOG_INFO, "tt", "videoEnc_thread1 is %d,%d",encParam_s.encodeType,out_size);
	    if (out_size > 0) {
	        AVPacket pkt;
	        av_init_packet(&pkt);
	        
	        if (c->coded_frame->pts != AV_NOPTS_VALUE)
	            pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);

			if(c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

	        pkt.stream_index = video_st->index;
	        pkt.data = video_outbuf;
	        pkt.size = out_size;
            pthread_mutex_lock (&(av_write_mutex));
	        ret = av_interleaved_write_frame(oc_, &pkt);
			pthread_mutex_unlock (&(av_write_mutex));
			av_free_packet(&pkt);
    	}
		
    }
}

static 
int  audioEnc_thread()
{
    if (oc_ == NULL || audio_st == NULL)
		return -1;

    #define AUDIO_BUFSIZE 100000
	uint8_t samples[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3)/2] = {0};
	uint8_t tmpbuf[400];//max size 320
	int tmpbuf_size = 0;
	uint8_t audio_outbuf[AUDIO_BUFSIZE];
	int audio_outbuf_size = AUDIO_BUFSIZE;
	int ret;
	AVCodecContext *c;
	int64_t ptss = 0;
	int out_size;
	int got_audio = 0;
	int i=0;
	c = audio_st->codec;
	int samplesize = 0;//audio_input_frame_size * 2 * 2 / (samplesize);
	
    m_state_audio_ = RUNNING;
	while(1) {
		if(exit == 1)  break;
		
        got_audio = 0;
		ret  = pthread_mutex_lock (&(g_audio_framebuflist.frame_buf_mutex));
		if (g_audio_framebuflist.size > 0) {
		    if (g_audio_framebuflist.framebuf[g_audio_framebuflist.readpos].buf != NULL) {
				if(tmpbuf_size>0) memcpy(samples,tmpbuf,tmpbuf_size*2);
                memcpy(samples+tmpbuf_size*2, g_audio_framebuflist.framebuf[g_audio_framebuflist.readpos].buf, g_audio_framebuflist.framebuf[g_audio_framebuflist.readpos].bufsize);
                samplesize = tmpbuf_size+(g_audio_framebuflist.framebuf[g_audio_framebuflist.readpos].bufsize/2);
				tmpbuf_size = 0;
				g_audio_framebuflist.size--;
				g_audio_framebuflist.readpos++;
				g_audio_framebuflist.readpos = g_audio_framebuflist.readpos%AUDIOFRAMEBUFSIZE;
				got_audio = 1;
			}
		}
		ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));
		if(got_audio == 0) {
           usleep(50);
		   continue;
		}

        i = 0;
        while (samplesize >= 160){
			    out_size = avcodec_encode_audio(c, audio_outbuf, AUDIO_BUFSIZE, samples+i*320);
			    i++;
				samplesize -= 160;
				//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread  is %d,%d",g_audio_framebuflist.size,out_size);

			    if (out_size > 0) {
			        AVPacket pkt;
			        av_init_packet(&pkt);
			        
					if (c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE) 
						pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, audio_st->time_base);
					
					pkt.flags |= AV_PKT_FLAG_KEY;
					pkt.stream_index = audio_st->index;
					pkt.data = audio_outbuf;
					pkt.size = out_size;

					/* write the compressed frame in the media file */
					pthread_mutex_lock (&(av_write_mutex));
					//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread  is %d,%d,%d,%lld",samplesize,g_audio_framebuflist.size,out_size,pkt.pts);
					ret = av_interleaved_write_frame(oc_, &pkt);
					//__android_log_print(ANDROID_LOG_INFO, "tt", "audioEnc_thread out is %d,%d,%d,%lld",samplesize,g_audio_framebuflist.size,out_size,pkt.pts);
                    pthread_mutex_unlock (&(av_write_mutex));
					
					av_free_packet(&pkt);
		    	}
				//delete []readbuff;
        }
		if(samplesize>0 && samplesize<160){
           tmpbuf_size = samplesize;
		   memcpy(tmpbuf,samples+i*320,tmpbuf_size*2);
		}
	}

	//av_fifo_free(fifo); 
	m_state_audio_ = THREADEXIT;
}


int join(void **ppValue)
{
	int err;
	err =  (pthread_join(m_tid_audio_, ppValue) <= 0)? 1 : 0;
	
	err =  (pthread_join(m_tid_, ppValue) <= 0)? 1 : 0;
	//m_state_video_ = READY;
	return err;
}

int realtimeEnc_start(s_encparam encP, int level, const char *outputfile)
{
    const char *filename;
	int err = 0;
	float fps = encP.fps;
	Enclevel = level;
	if (level == 0) {//适配机型dd
		encP.encodeType = 0;
		Encwidth = encP.width;
		Encheight = encP.height;
		encP.width = encP.height;
	}
	int width = encP.width;
	int height = encP.height;

	if(encP.width == encP.height)   encP.encodeType = 0;//竖拍为软编
	
	memcpy(&encParam_s,&encP,sizeof(s_encparam));
	
	if(m_state_video_ == RUNNING) return -1;
	
	pause_gap = lastpts = pause_flag = exit = pause = 0;
	
	av_register_all();
    filename = outputfile;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init %d, %d, %d %s",encP.encodeType,encP.width,encP.height,filename);

	avformat_alloc_output_context2(&oc_, NULL, NULL, filename);
    //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 %llf %d %d %s",fps,width,height,filename);
	video_st = add_video_stream(oc_, CODEC_ID_H264, encP);
    if(video_st == NULL) return -1;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 %llf %d %d %s",fps,width,height,filename);

	audio_st = add_audio_stream(oc_, CODEC_ID_AMR_NB);
	if(audio_st == NULL) return -1;
    //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init2 %llf %d %d %s",fps,width,height,filename);
	av_dump_format(oc_, 0, filename, 1);

    if (video_st)     
	    err = open_video(oc_, video_st, encP);

    if(err < 0) return -1;
	
	if (audio_st)
        err = open_audio(oc_, audio_st);

	if(err < 0) return -1;
 
	if(avio_open(&oc_->pb, filename, AVIO_FLAG_WRITE) < 0) {
       return -1;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init4 %llf %d %d %s",fps,width,height,filename);

    if(avformat_write_header(oc_, NULL)) {
       return -1;
	}

	//video buf  init
	if(level == 0) {
       LISTINIT(g_video_framebuflist,height*height*1.5,VIDEOFRAMEBUFSIZE)
	} else {
	   LISTINIT(g_video_framebuflist,width*height*1.5,VIDEOFRAMEBUFSIZE)
    }
    LISTINIT(g_audio_framebuflist,20000,AUDIOFRAMEBUFSIZE)
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init3 %llf %d %d %s",fps,width,height,filename);

	pthread_mutex_init (&(av_write_mutex), NULL);

    m_state_video_ = READY;
	m_state_audio_ = READY;
    if(pthread_attr_init(&m_ptAttr_) != 0){
       return -1;
 	}

	pthread_attr_setdetachstate(&m_ptAttr_,PTHREAD_CREATE_JOINABLE);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init2 %llf %d %d %s",fps,width,height,filename);
 	if(pthread_create(&m_tid_, &m_ptAttr_, videoEnc_thread, NULL) != 0) {
		//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init %llf %d %d %s",fps,width,height,filename);
       return -1;
	}
	
	if(pthread_attr_init(&m_ptAttr_audio_) != 0){
       return -1;
 	}

	pthread_attr_setdetachstate(&m_ptAttr_audio_,PTHREAD_CREATE_JOINABLE);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init2 %llf %d %d %s",fps,width,height,filename);
 	if(pthread_create(&m_tid_audio_, &m_ptAttr_audio_, audioEnc_thread, NULL) != 0) {
		//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init %llf %d %d %s",fps,width,height,filename);
       return -1;
	}
    return 0;
}

static 
void fill_rgba_image(s_framebuf_video_list *list, char *data, int width, int height, __uint64_t pts)
{
    int x, y, i;
	char *Y = list->framebuf[list->writepos].buf;
	char *U = list->framebuf[list->writepos].buf+width*height;
	char *V = list->framebuf[list->writepos].buf+width*height*5/4;
    char *r = data;
	//char *g = data + width*height;
	//char *b = data + 2*width*height;
	//memcpy(Y,r,width*height*1.5);
	
    //yuv  soft enc
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_proc is %d,%d",Encwidth,Encheight);
    if (Enclevel == 0) { //直接从摄像头过来
        int startgop = (Encwidth-Encheight)/2;
        for(int i = 0; i < Encheight; i++)
			memcpy(Y+i*Encheight,r+i*Encwidth+startgop,Encheight);
        //memcpy(Y,r+(Encwidth-Encheight)*Encwidth/2,Encheight*Encheight);
		U = list->framebuf[list->writepos].buf+Encheight*Encheight;
		V = list->framebuf[list->writepos].buf+Encheight*Encheight*5/4;
		//memset(U,128,Encheight*Encheight/4);
		//memset(V,128,Encheight*Encheight/4);
		r = data + Encwidth*Encheight;
		int startgop1 = (Encwidth/2-Encheight/2);
		for(int i = 0; i<Encheight/2; i++)
			for(int j = 0; j<Encheight/2; j++) {
		        *(U+i*Encheight/2+j) = *(r+j*2+i*Encwidth+startgop1+1);
				*(V+i*Encheight/2+j) = *(r+j*2+i*Encwidth+startgop1);
			}
    /*
        switch(encParam_s.encodeType){
            case 0: //YV12
			case 5: //mtk
			{
			     memcpy(Y,r,height*width);
				 r += height*width;
				 for(int i = 0; i<height/2; i++)
					for(int j = 0; j<width/2; j++) {
				        *(U+i*width/2+j) = *(r+j*2+i*width);
						*(V+i*width/2+j) = *(r+j*2+i*width+1);
					}
				break;
			}
		    case 2:
			case 4:
			{
				//samsung  nv21?
				memcpy(Y,r,height*width);
				r += height*width;

				for(int i = 0; i<height/2; i++)
					for(int j = 0; j<width/2; j++) {
				        *(U+i*width/2+j) = *(r+j*2+i*width+1);
						*(V+i*width/2+j) = *(r+j*2+i*width);

					}	
				break;
			}
			case 1:
			case 3:
			{
				//nv12   hard enc
				memcpy(Y,r,height*width*1.5);
	            break;
			}
			
			default:
			{
			     memcpy(Y,r,height*width);
				 r += height*width;
				 for(int i = 0; i<height/2; i++)
					for(int j = 0; j<width/2; j++) {
				        *(U+i*width/2+j) = *(r+j*2+i*width);
						*(V+i*width/2+j) = *(r+j*2+i*width+1);
					}
				 break;
			}

		}*/
	} else {
	    switch(encParam_s.encodeType){
			case 0: //YV12
			case 5: //mtk
			case 6:
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
/*
					for(int i = 0; i<height/2; i++)
						for(int j = 0; j<width/2; j++) {
				            *(V+i*width/2+j) = *(r+i*width*8+j*8+2);
						}
*/
					break;
				}
		}
	}
    list->framebuf[list->writepos].bufsize = width*height*1.5;
	list->framebuf[list->writepos].pts = pts;
	list->size++;
	list->writepos++;
	list->writepos = list->writepos%VIDEOFRAMEBUFSIZE;
}


int realtimeEnc_videoproc(char *data, int type, __uint64_t pts)
{
    //return 0;
#if 0
       FILE *f = fopen("/sdcard/1.yuv", "ab+");
	   fwrite(data, 1, 480*320*1.5, f);
	   fclose(f);
#endif 
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc0 is %d,%d,%d",exit,pause,m_state_video_);
    if(exit == 1) return 0;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc is %d",pause);
    if (pause==1) {
		if(pause_flag==0) {
			pause_flag=!pause_flag;
            lastpts = pts;
		}	
		return 0;
    }
	if (pause_flag==1){
		pause_flag=!pause_flag;
	    pause_gap += pts-lastpts;
	}
	
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc0 is %lld,%lld,%d,%d",pts,pause_gap,m_state_video_,RUNNING);

	pts -= pause_gap;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc111");
	if(m_state_video_ != RUNNING) return 0;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc11 is %d",data);
	int ret  = pthread_mutex_lock (&(g_video_framebuflist.frame_buf_mutex));
    if (oc_ == NULL || video_st == NULL) {
		ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
		return -1;
	}

	if (!video_outbuf) {
		ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
		return -1;
	}
    //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_videoproc is %d",data);
    //if(m_state_video_ != RUNNING) {ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));return -1;}
	if(!data)  {ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));return -1;}
	

	AVCodecContext *c;
	c = video_st->codec;

	if(g_video_framebuflist.size >= VIDEOFRAMEBUFSIZE) {ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));return 0;}

	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_proc  is %d,%d",c->width,c->height);
	if (g_video_framebuflist.framebuf[g_video_framebuflist.writepos].buf != NULL) {
		fill_rgba_image(&g_video_framebuflist, data, c->width, c->height,pts);
	}

	ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_proc1  is,%d",g_video_framebuflist.size);
    return 0;
}


int realtimeEnc_audioproc(short *data, int len, int64_t pts)
{
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_audioproc");


    if(exit == 1) return -1;
    if (pause == 1) return -1;
	if (m_state_audio_ != RUNNING) return -1;
	
	int ret  = pthread_mutex_lock (&(g_audio_framebuflist.frame_buf_mutex));
    if (oc_ == NULL || audio_st == NULL) {
		ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));
		return -1;
	}

	if (!audio_st) {
		ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));
		return -1;
	}
  
    //if(m_state_video_ != RUNNING) {ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));return -1;}
	if (!data)  {ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));return -1;}

	AVCodecContext *c;
	c = audio_st->codec;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_audioproc  is %d,%d",g_audio_framebuflist.size,len);

	if (g_audio_framebuflist.size >= AUDIOFRAMEBUFSIZE) {ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));return -1;}


	if (g_audio_framebuflist.framebuf[g_audio_framebuflist.writepos].buf != NULL) {
		memcpy(g_audio_framebuflist.framebuf[g_audio_framebuflist.writepos].buf,(char *)data,len*2);
	    g_audio_framebuflist.framebuf[g_audio_framebuflist.writepos].bufsize = len*2;
	    g_audio_framebuflist.size++;
	    g_audio_framebuflist.writepos++;
	    g_audio_framebuflist.writepos = g_audio_framebuflist.writepos%AUDIOFRAMEBUFSIZE;
	}

	ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));
    return 0;
}

int realtimeEnc_pause()
{
    pause = !pause;
	return 0;
}


int realtimeEnc_stop()
{
	 void* pThdRslt;
	 int err;
	 int ret;
     exit=1;
	 if(oc_==NULL) return 0;
	 
	 err = join(&pThdRslt);
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_exit ");
	 ret = pthread_mutex_lock (&(g_video_framebuflist.frame_buf_mutex));
	 if(oc_)
		av_write_trailer(oc_);
	 ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
	// __android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init22is %d",g_video_framebuflist.size); 

	 ret = pthread_mutex_lock (&(g_audio_framebuflist.frame_buf_mutex));
	 if(audio_st)
		avcodec_close(audio_st->codec);
	 ret = pthread_mutex_unlock (&(g_audio_framebuflist.frame_buf_mutex));
	 
	// __android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_exit ");
	 ret = pthread_mutex_lock (&(g_video_framebuflist.frame_buf_mutex));
	 if(video_st)
		avcodec_close(video_st->codec);
	 
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d",g_video_framebuflist.size);
	 if(video_outbuf) {
		av_free(video_outbuf);
		video_outbuf = NULL;
		video_outbuf_size = 0;
	 }
	
	 if(oc_) {
		 for(int i = 0; i < oc_->nb_streams; i++) {
			 av_freep(&oc_->streams[i]->codec);
			 av_freep(&oc_->streams[i]);
		 }
		 
		 if (oc_->pb) {
			 /* close the output file */
			 avio_close(oc_->pb);
		 }
		 
		 /* free the stream */
		 av_free(oc_);
	     //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d,%d",g_video_framebuflist.size,oc_);
	  	 oc_ = NULL;
	 }
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init1 is %d",g_video_framebuflist.size);
	
	 for(int i = 0; i < VIDEOFRAMEBUFSIZE; i++) {
		 if(g_video_framebuflist.framebuf[i].buf) {
			 free(g_video_framebuflist.framebuf[i].buf);
			 g_video_framebuflist.framebuf[i].bufsize = 0;
			 g_video_framebuflist.framebuf[i].buf = NULL;
		 }		 
	 }
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init122 is %d",g_video_framebuflist.size);

	 for(int i = 0; i < AUDIOFRAMEBUFSIZE; i++) {
		 if(g_audio_framebuflist.framebuf[i].buf) {
			 free(g_audio_framebuflist.framebuf[i].buf);
			 g_audio_framebuflist.framebuf[i].bufsize = 0;
			 g_audio_framebuflist.framebuf[i].buf = NULL;
		 }		 
	 }
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init122 is %d",g_video_framebuflist.size);

	 ret = pthread_mutex_unlock (&(g_video_framebuflist.frame_buf_mutex));
	 //pthread_mutex_destroy(&(g_video_framebuflist.frame_buf_mutex));
	 //__android_log_print(ANDROID_LOG_INFO, "tt", "realtimeEnc_init2 is %d",g_video_framebuflist.size); 

	return 0;
}

