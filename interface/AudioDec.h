#ifndef REAL_AUDIO_DEC_H
#define REAL_AUDIO_DEC_H


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

class CRealAudioDec {
	public:
		CRealAudioDec();
		~CRealAudioDec();


		int audio_dec_start(const char *InputFile);
		int audio_dec_stop();

		int audio_dec_sample(char *data, int len, long pts);
		static void* __RealTimeDecConstructer();

		AVCodec *m_codec;
        AVCodecContext *m_context;

    private:
		static CRealAudioDec *m_decode_handler_;
};



#endif

