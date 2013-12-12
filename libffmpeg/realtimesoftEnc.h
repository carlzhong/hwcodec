#ifndef REALTIME_SOFTENC_H
#define REALTIME_SOFTENC_H


#define SOFTVIDEOFRAMEBUFSIZE 20
#define SOFTAUDIOFRAMEBUFSIZE 50

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
	s_framebuf framebuf[SOFTVIDEOFRAMEBUFSIZE];
}s_framebuf_video_list;

typedef struct {
	int readpos;
	int writepos;
	int size;
	pthread_mutex_t frame_buf_mutex;
	s_framebuf framebuf[SOFTAUDIOFRAMEBUFSIZE];
}s_framebuf_audio_list;


int realtime_SoftEnc_start(int width, int height, const char *outputfile);
int realtime_SoftEnc_videoproc(char *data, __uint64_t pts);
int realtime_SoftEnc_audioproc(short *data, int len, int64_t pts);

int realtime_SoftEnc_stop();
int realtime_SoftEnc_pause();


#endif
