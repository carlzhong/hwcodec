#include <libavformat/avformat.h>

//#include "ffmpeg.h"
#include <android/log.h>
#include <pthread.h>
#include "libswscale/swscale.h"
#include "libavutil/fifo.h"
#include "realtimesoftEnc.h"

enum ProcessState {
	  READY = 0,
	  RUNNING = 1,
	  THREADEXIT = 2,
	  PROCESSEXIT
};


static AVFormatContext* ic_ = NULL;
static AVFormatContext* oc_ = NULL;
static AVStream *video_st = NULL;
static AVStream *audio_st = NULL;
static uint8_t *video_outbuf = NULL;
static int video_outbuf_size = 0;
static float t, tincr, tincr2;
static AVFrame picture;//=NULL;// *tmp_picture;
static AVFrame picture_soft;//=NULL;// *tmp_picture;


pthread_t m_tid_;
pthread_attr_t m_ptAttr_;
pthread_t m_tid_audio_;
pthread_attr_t m_ptAttr_audio_;
int framesize = 0;
static int exit = 0;
static int pause = 0;
//static int sws_flags = SWS_BICUBIC;
enum ProcessState m_state_video_;
enum ProcessState m_state_audio_;
//extern int qcomErr;
//extern int QCOM_H264_encodeIsAvailable();
//extern int Samsung_H264_encodeIsAvailable();

int pause_flag = 0;
__uint64_t lastpts = 0;
__uint64_t pause_gap = 0;

static s_framebuf_video_list g_video_framebuflist;
static s_framebuf_audio_list g_audio_framebuflist;
pthread_mutex_t av_write_mutex;

//s_encparam encParam_s;


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


