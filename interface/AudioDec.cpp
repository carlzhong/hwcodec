#include"AudioDec.h"
#include "RealTimeInt.h"


CRealAudioDec* CRealAudioDec::m_decode_handler_ = NULL;

CRealAudioDec::CRealAudioDec() {
		m_codec = NULL;
        m_context = NULL;
}
CRealAudioDec::~CRealAudioDec() {

}

int CRealAudioDec::audio_dec_start(const char *InputFile)
{
	avcodec_init();
	av_register_all();
	int err;
    int mCodecid = -1;
	m_codec = NULL;
    m_context = NULL;
	uint64_t m_channel_layout=0;
	int m_sample_rate=0;
	int m_channels=0;
	int m_frame_size=0;
	int audio_stream_id = -1;
	enum AVSampleFormat m_sample_fmt = AV_SAMPLE_FMT_S16;
    //≤È’““Ù∆µµƒID
    AVFormatContext *mMovieFile = avformat_alloc_context();
	//Log(ANDROID_LOG_INFO, "tt", filename );
	if(!mMovieFile)
		return -1;	    
	if ((err = avformat_open_input(&mMovieFile, InputFile, NULL, NULL)) < 0) {
        //Log(ANDROID_LOG_INFO, "tt", "could not open input file" );
   	    return -1;
   	}
	if ((err =av_find_stream_info(mMovieFile)) < 0) {
		//Log(ANDROID_LOG_INFO, "tt", "could not find stream info" );
		return -1;
	}

	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			mCodecid = mMovieFile->streams[i]->codec->codec_id;//mMovieFile->video_codec_id;
			audio_stream_id = i;
			m_channel_layout = mMovieFile->streams[i]->codec->channel_layout;
			m_sample_rate = mMovieFile->streams[i]->codec->sample_rate;
			m_channels = mMovieFile->streams[i]->codec->channels;
			m_frame_size = mMovieFile->streams[i]->codec->frame_size;
            m_sample_fmt = mMovieFile->streams[i]->codec->sample_fmt;
		    //__android_log_print(ANDROID_LOG_INFO, "tt", "mCodecid=%lld,%d,%d,%d,%d",m_channel_layout,\
		      //          m_sample_rate,m_channels,m_frame_size,mMovieFile->streams[i]->codec->extradata_size);
		}
	}

	
	if(mCodecid < 0) {
		av_close_input_file(mMovieFile);
		return -1;
	}
    /* find the mpeg audio decoder */
	//__android_log_print(ANDROID_LOG_INFO, "tt", "mCodecid=%d ",mCodecid);
    m_codec = avcodec_find_decoder((enum CodecID)mCodecid);


	//m_codec = avcodec_find_decoder_by_name("aac_latm");
    if (!m_codec) {
		av_close_input_file(mMovieFile);
        return -1;
    }

    m_context= avcodec_alloc_context3(m_codec);
	
    m_context->channel_layout = m_channel_layout;
	m_context->sample_rate = m_sample_rate;
	m_context->channels = m_channels;
	m_context->frame_size = m_frame_size;
	m_context->sample_fmt = m_sample_fmt;

    if(audio_stream_id >= 0 && mMovieFile->streams[audio_stream_id]->codec->extradata_size > 0) {
		m_context->extradata = (uint8_t *)av_mallocz(mMovieFile->streams[audio_stream_id]->codec->extradata_size+
									  FF_INPUT_BUFFER_PADDING_SIZE);
	    memcpy(m_context->extradata, mMovieFile->streams[audio_stream_id]->codec->extradata,\
			             mMovieFile->streams[audio_stream_id]->codec->extradata_size);

		m_context->extradata_size = mMovieFile->streams[audio_stream_id]->codec->extradata_size;
    }
    //memcpy(m_context, mMovieFile->streams[audio_stream_id]->codec, );
	av_close_input_file(mMovieFile);


    if (avcodec_open(m_context, m_codec) < 0) {
        return -1;
    }

	__android_log_print(ANDROID_LOG_INFO, "tt", "audio_enc_start ok is %d,%s,%d", mCodecid,m_codec->name,\
		                       m_context->extradata_size);


    return 0;
}

int CRealAudioDec::audio_dec_sample(char *data, int len, long pts)
{
    if(data == NULL || len <= 0) return -1;
	
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;

    av_init_packet(&avpkt);

	avpkt.data = (uint8_t *)data;
    avpkt.size = len;
	//avpkt.pts = avpkt.dts = pts;

	if(avpkt.size > 0) {
		int got_frame = 0;

	    if (!(decoded_frame = avcodec_alloc_frame())) {
               return -1;
	    }
		//__android_log_print(ANDROID_LOG_INFO, "tt", "before is %d,%lld",len,pts);

	    int outlen = avcodec_decode_audio4(m_context, decoded_frame, &got_frame, &avpkt);

		//__android_log_print(ANDROID_LOG_INFO, "tt", "audio_dec_sample is %d,%d,%d",outlen,got_frame,m_context->extradata_size);
		if (outlen < 0) {
            return -1;
		}

	   if (got_frame) {
		   /* if a frame has been decoded, output it */
		   int data_size = av_samples_get_buffer_size(NULL, m_context->channels,
												   decoded_frame->nb_samples,
												   m_context->sample_fmt, 1);

           if (data_size > 0)
		       RealtimeEnc_audioproc((short *)(decoded_frame->data[0]), data_size/2, pts);

		   //data_size = av_samples_get_buffer_size(NULL, m_context->channels,
												   //decoded_frame->nb_samples,
												   //m_context->sample_fmt, 1);
		   //__android_log_print(ANDROID_LOG_INFO, "tt", "audio_dec_sample111 is %lld,%d",(__uint64_t)pts,data_size);
		   //fwrite(decoded_frame->data[0], 1, data_size, outfile);
	   }

	}
	if(decoded_frame)
	   av_free(decoded_frame);
	
    return 0;
}

int CRealAudioDec::audio_dec_stop()
{
    if (m_context) {
	    avcodec_close(m_context);
	    av_free(m_context);
    }
    return 0;
}

void* CRealAudioDec::__RealTimeDecConstructer()
{
   try {
   	 // __android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer is %d",m_handler);
     if(m_decode_handler_ == NULL)
   	    m_decode_handler_ = new CRealAudioDec();
      //__android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer1");
     return m_decode_handler_;
   } catch(...) {
     return NULL;
   }
}


