#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>
#include <unistd.h>
//#include <iostream.h>
#include <stdio.h>
//#include "LockMutex.h"

using namespace std;

enum ProcessState {
	  READY = 0,
	  RUNNING = 1,
	  THREADEXIT = 2,
	  PROCESSEXIT
};
typedef struct {
		int argc;
		char* argv[30]; 
}s_FFMPEGSTRUCT;

typedef int (*CallBack)(int data);

class CThread
{
public:
	 CThread();
	 ~CThread();
		 
	 bool StartThread(s_FFMPEGSTRUCT *param);
     bool StopThread();
     bool restartThread();
	 bool join(void **ppValue);
    // virtual bool mainProcess() = 0;
     //virtual void resetParams() = 0;
     // HANDLE GetThreadHandle();
	 inline ProcessState GetStateFlag(){return m_state_inner_;}
	 inline void SetStateFlag(ProcessState bFlag){m_state_inner_ = bFlag;}
	 static void* __ThreadConstructer();

	 static void __SetCallback(int type,CallBack func);
protected:
    
	 static void* __ThreadWorkFunc(void *);
	 static CThread* m_handler;

private:
     pthread_t m_tid_;
     pthread_attr_t m_ptAttr_;
     ProcessState m_state_inner_;
     bool m_blsPtAttrOk_;
    // Mutex m_mtx_;
};


#endif
