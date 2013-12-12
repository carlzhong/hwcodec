//#include "StdAfx.h"
#include "CThread.h"

extern "C" {
#include "ffmpeg.h"
}
#include <android/log.h>



CThread* CThread::m_handler = NULL;

CThread::CThread(void)
{
  m_blsPtAttrOk_ = false;
  m_state_inner_ = READY;
  //__android_log_print(ANDROID_LOG_INFO, "tt", "CThread");
}


CThread::~CThread(void)
{
	if(m_blsPtAttrOk_ == true)
		 pthread_attr_destroy(&m_ptAttr_);
	m_state_inner_ = THREADEXIT;
}

bool CThread::join(void **ppValue)
{
	bool err;	
	err =  (pthread_join(m_tid_, ppValue) <= 0)? false : true;
	m_state_inner_ = READY;
	return err;
}
 
bool CThread::StartThread(s_FFMPEGSTRUCT *param)
{
	void* pThdRslt;
	
    if(pthread_attr_init(&m_ptAttr_) != 0){
 	   printf("initialize pthread attribute failed!");
       return false;
 	}
	
 	m_blsPtAttrOk_ = true;
	pthread_attr_setdetachstate(&m_ptAttr_,PTHREAD_CREATE_JOINABLE);
 	if(pthread_create(&m_tid_, &m_ptAttr_, CThread::__ThreadWorkFunc, param) != 0) {
	   printf("StartThread failed!");
       return false;
	}
	m_state_inner_ = RUNNING;
	
	return join(&pThdRslt);
}

bool CThread::StopThread()
{   
   void* pThdRslt;
 
   //pthread_detach(m_tid_); 
   ffmpeg_set_terminate(3);//3             
   //pthread_cancel(m_tid_);
   m_state_inner_ = THREADEXIT;
   return true;//join(&pThdRslt);
}

/*bool CThread::restartThread()
{
   bool thread_stop = StopThread();
   if(thread_stop == false)
  	  return false;
   
   resetParams();
   bool thread_start = StartThread();
   if(thread_start == false)
  	  return false;
  	  
   return true;
}*/


void* CThread::__ThreadConstructer()
{
   try {
   	 // __android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer is %d",m_handler);
     if(m_handler == NULL)
   	    m_handler = new CThread();
      //__android_log_print(ANDROID_LOG_INFO, "tt", "__ThreadConstructer1");
     return m_handler;
   } catch(...) {
     return NULL;
   }
}

void CThread::__SetCallback(int type,CallBack func)
{
   if(func == NULL) return;
   Set_CallBack_ff(type,func);
}


void* CThread::__ThreadWorkFunc(void *param)
{
	s_FFMPEGSTRUCT *pParam = reinterpret_cast<s_FFMPEGSTRUCT *> (param);//Ç¿ÖÆ×ª»»³És_FFMPEGSTRUCTÀ
	ffmpeg_main(pParam->argc, pParam->argv);

}

 
/*HANDLE CThread::GetThreadHandle()
{
    return m_hThread;
}*/
