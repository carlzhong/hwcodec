#ifndef REALTIME_ENC_H
#define REALTIME_ENC_H
#include <pthread.h>


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
}s_encparam;


//实时编码相关
int realtimeEnc_start(s_encparam encP,int level, const char *outputfile);
int realtimeEnc_videoproc(char *data, int type, __uint64_t pts);
int realtimeEnc_audioproc(short *data, int len, int64_t pts);

int realtimeEnc_stop();
int realtimeEnc_pause();




#endif
