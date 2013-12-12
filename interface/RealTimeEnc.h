#ifndef REAL_TIME_ENC_H
#define REAL_TIME_ENC_H

#ifndef   UINT64_C
#define   UINT64_C(value)__CONCAT(value,ULL)
#endif
#ifndef   INT64_C
#define   INT64_C(value)__CONCAT(value,LL)
#endif

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
}
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <android/log.h>


using namespace std;

#define VIDEOFRAMEBUFSIZE 100
#define AUDIOFRAMEBUFSIZE 50

enum  DeviceType {
	UNKNOWNDEVICE = 0,
	XIAOMI_1S = 1,
	SAMSUNG_S3 = 2,
	SAMSUNG_NOTE2,
	HUAWEI_U9200,
	NEXUS_4,
	SAMSUNG_I9100,
	HUAWEI_C8813
};

typedef struct {
	int bufsize;
	__uint64_t pts;
	unsigned char *buf;
}s_framebuf;

typedef struct {
	int readpos;
	int writepos;
	int size;
	pthread_mutex_t frame_buf_mutex;
	s_framebuf framebuf[VIDEOFRAMEBUFSIZE];
}s_framebuf_video_list;

typedef struct {
	int readpos;
	int writepos;
	int size;
	pthread_mutex_t frame_buf_mutex;
	s_framebuf framebuf[AUDIOFRAMEBUFSIZE];
}s_framebuf_audio_list;

typedef struct {
	float fps;
	int width;
	int height;
	int encodeType;
	int deviceType;
	int cpuNums;
	int cpuFreq;
	int memSize;
	int degree;
	int bitrate;
	int audioFreq;
	int audioChannels;
	int audiobitDepth;
}s_encparam;





class CRealTimeEnc {
	public:
		CRealTimeEnc();
		~CRealTimeEnc();
		enum ProcessState {
			READY = 0,
			RUNNING = 1,
			THREADEXIT = 2,
			PROCESSEXIT
		};
		int realtime_enc_initial( s_encparam encP,int level, char *outputfile);
		int realtime_enc_start();
		int realtime_enc_stop();
		int realtime_enc_set_videodata(char *data, int type, __uint64_t pts);
		int realtime_enc_set_audiodata(char *data, int len, __uint64_t pts);
		static void* __RealTimeEncConstructer();
		
	protected:
		static CRealTimeEnc *m_encoder_handler_;
		s_encparam m_encoder_param_;
		//AVFormatContext* m_pinContext;
		AVFormatContext* m_pOutContext_;
		AVStream *m_video_stream_;
		AVStream *m_audio_stream_;
		uint8_t *m_video_outbuf_;
		int m_video_outbuf_size_;
		AVFrame m_picture_;//=NULL;// *tmp_picture;
		AVFrame m_picture_soft_;//=NULL;// *tmp_picture;
		pthread_t m_tid_;
		pthread_attr_t m_ptAttr_;
		pthread_t m_tid_audio_;
		pthread_attr_t m_ptAttr_audio_;
		//int m_framesize;
		int m_nExit_;
		int m_nPause_;
		//int m_sws_flags;
		enum ProcessState m_state_video_;
		enum ProcessState m_state_audio_;
		int m_pause_flag_;
		__uint64_t m_last_pts_;
		__uint64_t m_pause_gap_;
		s_framebuf_video_list m_video_frame_buffer_list_;
		s_framebuf_audio_list m_audio_frame_buffer_list_;
		pthread_mutex_t m_av_write_mutex_;
		//int m_vidframeListSize;
		//int m_audframeListSize;
		int m_encoder_level_;
		int m_encoder_width_;
		int m_encoder_height_;
		//float m_fT;
		//float m_fTincr;
		//float m_fTincr2;
		int m_frame_size_;	
		char *m_szFileName_;//输出文件名

		//计算帧率用
        clock_t starttime;
		clock_t endtime;
		int videocout;
		int fps_test;
		int64_t firstpts_video;
		int64_t firstpts_audio;
		long  audioSampleNum;
		__uint64_t lastpts;
		__uint64_t lastpts_audio;
		//AVFrame* alloc_picture(enum PixelFormat pix_fmt, int width, int height);
		//int setup_output_context(const char *filename);
		AVCodec* find_codec_or_die(const char *name, enum AVMediaType type, \
										int encoder);
		AVStream* add_video_stream(AVFormatContext *oc, enum CodecID codec_id, \
										s_encparam encP);
		AVStream* add_audio_stream(AVFormatContext *oc, enum CodecID codec_id);
		int open_video(AVFormatContext *oc, AVStream *st,s_encparam encP);
		int open_audio(AVFormatContext *oc, AVStream *st);
		//void fill_yuv_image(AVFrame *pict, char *data, int width, int height);
		//void pThreadParapgm_save(AVFrame *pict, int xsize, int ysize,char *filename);
		int join(void **ppValue);
		void fill_rgba_image(s_framebuf_video_list *list, char *data, int width, \
							int height, __uint64_t pts);

		int realtime_computeFps();
		static void*  video_encoder_thread_function(void *);
		static void* audio_encoder_thread_function(void *);

	

};



#endif // REAL_TIME_ENC_H