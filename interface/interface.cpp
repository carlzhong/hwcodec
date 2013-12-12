#include <jni.h>
#include <stdlib.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> 
#include <sys/wait.h>
extern "C" {
#include "ffmpeg.h"
}
#include "RealTimeEnc.h"
#include "AudioDec.h"
#include "CThread.h"

//const char* const gClassName = "com/demo/classes/MainActivity";
//ppq
//const char* const gClassName = "com/iqiyi/share/controller/opengles/H264MediaRecoder";

//huawei  xuke
//const char* const gClassName = "com/iqiyi/demo/huawei/edit/opengles/H264MediaRecoder";

const char* const gClassName = "com/android/share/hw/edit/opengles/H264MediaRecoder";



//const char* const gClassName = "com/iqiyi/videoeffect/VideoJNILib";

//xingdaming
//const char* const gClassName = "com/iqiyi/demo/huawei/edit/videoeditor/VideoJNILib";

#undef exit
#undef sprintf
#undef printf
#undef fprintf
#undef strcat
jmethodID on_progress_callback = NULL;
jmethodID on_jpegnumber_callback = NULL;
jmethodID on_error_callback = NULL;
JNIEnv* jniEnv = NULL;
jclass gClass = NULL;
JavaVM* gVm = NULL;
void* handler_callback = NULL;
int calltype = -1;


int setProgress(int progress)
{
    //JNIEnv *env = AndroidRuntime::getJNIEnv();
    JNIEnv *env = NULL;
    int status = gVm->GetEnv((void **)&env, JNI_VERSION_1_4);
    if(status < 0) {
        status = gVm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return 0;
        }
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress is %d,%d,%d,%d,%d",on_progress_callback,jniEnv,gClass,progress,handler_callback); 
	if (env != NULL && gClass !=NULL && calltype >= 0)
	    env->CallStaticVoidMethod(gClass, on_progress_callback, progress,calltype,handler_callback);

	//__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress1 is %d,%d,%d,%d,%d",on_progress_callback,jniEnv,gClass,progress,handler_callback);
	gVm->DetachCurrentThread();
    //__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress is %d,%d,%d",on_progress_callback,jniEnv,gClass); 
}


int setJpegNumber(int jpegnum)
{
    //JNIEnv *env = AndroidRuntime::getJNIEnv();
    JNIEnv *env = NULL;
    int status = gVm->GetEnv((void **)&env, JNI_VERSION_1_4);
    if(status < 0) {
        status = gVm->AttachCurrentThread(&env, NULL);
        if(status < 0) {
            return 0;
        }
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress is %d,%d,%d,%d,%d",on_progress_callback,jniEnv,gClass,progress,handler_callback); 
	if (env != NULL && gClass !=NULL && calltype >= 0)
	    env->CallStaticVoidMethod(gClass, on_jpegnumber_callback, jpegnum,calltype);

	//__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress1 is %d,%d,%d,%d,%d",on_progress_callback,jniEnv,gClass,progress,handler_callback);
	gVm->DetachCurrentThread();
    //__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress is %d,%d,%d",on_progress_callback,jniEnv,gClass); 
}


int SetErrorFlag(int errorflag)
{
    //JNIEnv *env = AndroidRuntime::getJNIEnv();
    JNIEnv *env = NULL;
    int status = gVm->GetEnv((void **)&env, JNI_VERSION_1_4);
    if(status < 0)
    {
        status = gVm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            return 0;
        }
    }
	
    if (env != NULL && gClass != NULL && calltype >= 0)
	    env->CallStaticVoidMethod(gClass, on_error_callback, errorflag, calltype, handler_callback);
	
	gVm->DetachCurrentThread();
    //__android_log_print(ANDROID_LOG_INFO, "tt", "setProgress is %d,%d,%d",on_progress_callback,jniEnv,gClass); 
}


static  
void nativeVideoCut(JNIEnv *env, jobject thiz, jstring InputFile, jstring StartTime, jstring VideoLength, jstring OutputFile,jobject handler)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[1000];
	char mOutput[1000];
	char mStart[100];
	char mLength[100];
	char mTimeGop[100];
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);	
	const char *Start = env->GetStringUTFChars(StartTime, NULL);	
	const char *Length = env->GetStringUTFChars(VideoLength, NULL);
	strcpy(mInput,Input);
	strcpy(mOutput,Output);	
	strcpy(mStart,Start);
	strcpy(mLength,Length);
	double starttimeInt = (double)atof(mStart);
	double timeDurInt = (double)atof(mLength);
	starttimeInt/=1000.0;
	timeDurInt/=1000.0;
	sprintf(mStart,"%llf",(starttimeInt));
	sprintf(mLength,"%llf",(timeDurInt));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativeVideoCut mInput=%s,mStart=%s,mLength=%s,mOutput=%s, %d",mInput,mStart,mLength,mOutput,starttimeInt);
    handler_callback = handler;
	calltype = 0;
	__android_log_print(ANDROID_LOG_INFO, "tt", "handler_callback cut is %d,%s",handler_callback,mInput);
	int codecId = ffmpeg_getVideoCodecid(mInput);
    //strstr(mInput,".mp4");

	//int ret = ffmpeg_getVideoRotate(mInput);
	
    /*if (codecId == 28 && ((strstr(mInput,".mp4")!=NULL) || (strstr(mInput,".3gp")!=NULL))) {
	   sprintf(mTimeGop,"%lld %lld", (int64_t)(starttimeInt*1000), (int64_t)(1000*(starttimeInt+timeDurInt)));

	   s_FFMPEGSTRUCT a ={11,"ffmpeg","-i",mInput,"-frameidx", mTimeGop,"-vcodec","stagefrighticsenc","-acodec","copy","-y",mOutput};
	   //s_FFMPEGSTRUCT a ={13,"ffmpeg","-i",mInput,"-frameidx", mTimeGop,"-vcodec","libx264","-refs","6","-acodec","copy","-y",mOutput};
	   CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	   handle->StartThread(&a);
      } else {*/
		//s_FFMPEGSTRUCT a ={13,"ffmpeg","-ss",mStart,"-i",mInput,"-vcodec","copy","-acodec","copy","-t",mLength,"-y",mOutput};
		//CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
		//handle->StartThread(&a);
	//}


	  if (strstr(mInput,".ts")!=NULL) {
		  s_FFMPEGSTRUCT a ={15,"ffmpeg","-ss",mStart,"-i",mInput,"-absf","aac_adtstoasc","-vcodec","copy","-acodec","copy","-t",mLength,"-y",mOutput};
		  CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
		  handle->StartThread(&a);

      } else {
		   s_FFMPEGSTRUCT a ={13,"ffmpeg","-ss",mStart,"-i",mInput,"-vcodec","copy","-acodec","copy","-t",mLength,"-y",mOutput};
		   CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
		   handle->StartThread(&a);
	  }
	//good
	//s_FFMPEGSTRUCT a ={13,"ffmpeg","-ss",mStart,"-i",mInput,"-vcodec","copy","-acodec","copy","-t",mLength,"-y",mOutput};

	//s_FFMPEGSTRUCT a ={19,"ffmpeg","-ss",mStart,"-i",mInput,"-vcodec","copy","-acodec","aac","-ab","64k","-ar","44100","-t",mLength,"-strict","-2","-y",mOutput};

	

	//s_FFMPEGSTRUCT a = {17,"ffmpeg","-i",mInput,"-vf","movie=/mnt/sdcard/snow%d.png [watermark];[in][watermark] overlay=0:0 [out]","-acodec",\
	                //   "copy","-c:v","libx264","-threads", "6","-preset","ultrafast","-x264opts","bitrate=1000","-y",mOutput};

	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	env->ReleaseStringUTFChars(StartTime, Start);
	env->ReleaseStringUTFChars(VideoLength, Length);
	return;
}


static void nativeMux(JNIEnv *env, jobject thiz, jstring InputVideo, jstring InputAudio, jstring OutputAudioFile, jstring OutputFile, double starttime, double duration)
{
    pthread_t       ffmpegThread;
    void            *tret;
    int             err;
    char mInput1[100];
    char mInput2[100];
    char mOutput[100];
    char mOutputAudio[100];
    char mStart[10];
    char mLength[10];
    const char *Input1 = env->GetStringUTFChars(InputVideo, NULL);
    const char *Input2 = env->GetStringUTFChars(InputAudio, NULL);
    const char *Output = env->GetStringUTFChars(OutputFile, NULL);
    const char *OutputAudio = env->GetStringUTFChars(OutputAudioFile, NULL);
    strcpy(mInput1,Input1);
    strcpy(mInput2,Input2);
    strcpy(mOutput,Output);
    strcpy(mOutputAudio,OutputAudio);
    sprintf(mStart,"%llf",((double)starttime)/1000);
    sprintf(mLength,"%llf",((double)duration)/1000);
	calltype = 2;
    //__android_log_print(ANDROID_LOG_INFO, "tt", "nativeMux  is %s,%s",mInput2,mOutputAudio);
    s_FFMPEGSTRUCT a0 ={14,"ffmpeg","-i",mInput2,"-setError","d","-vn", "-ss", mStart, "-t", mLength, "-strict","-2", "-y", mOutputAudio};
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
    while (handle->GetStateFlag() == RUNNING)
        usleep(1200);
    handle->StartThread(&a0);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativeMux	is %s,%s",mInput1,mOutput);

    s_FFMPEGSTRUCT a1 ={9,"ffmpeg","-i",mInput1,"-i", mOutputAudio, "-c", "copy", "-y", mOutput};    
    while (handle->GetStateFlag() == RUNNING)
        usleep(1200);
    handle->StartThread(&a1);

/*
    s_FFMPEGSTRUCT a2 ={9,"ffmpeg","-i","concat:/storage/sdcard0/intermediate1.ts|/storage/sdcard0/intermediate2.ts","-c", "copy", "-bsf:a", "aac_adtstoasc", "-y", mOutput};    
    while (handle->GetStateFlag() == RUNNING)
        usleep(1200);
    handle->StartThread(&a2);
*/
/*
    int argc = 16 ;
    char* argv[] = {"ffmpeg","-i",mInput,"-vf","thumbnail","-vframes","1","-y",mOutput};
    argc = 9;
    startTranscode(argc,argv);
*/
    env->ReleaseStringUTFChars(InputVideo, Input1);
    env->ReleaseStringUTFChars(InputAudio, Input2);
    env->ReleaseStringUTFChars(OutputFile, Output);
    env->ReleaseStringUTFChars(OutputAudioFile, OutputAudio);
    return;
}


//GetJpegType 0为粗线条，1为细颗粒。
static void  nativeProduceJpeg(JNIEnv *env, jobject thiz,  jstring InputFile, jstring OutputFileDir, int GetJpegType, \
             int width, int height, jintArray array)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[1000];
	char mOutput[1000];	
	char mTimePitch[500] = {0};
	char tmp[50] = {0};
	char scale[50] = {0};
	int mParameter[4] = {0};
	//int volume = (int)(InputVolume * 256 + 0.5);	
	//sprintf(mInputVolume,"%d",(volume));
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFileDir, NULL);	
	strcpy(mInput,Input);
	strcpy(mOutput,Output);
	sprintf(scale,"%dx%d",width,height);

	//int ret = ffmpeg_getVideoParameter(mInput,mParameter);
	//if(ret == 1) {
      // char outScale[100] = {0};
	   //sprintf(outScale,"%dx%d",mParameter[0],mParameter[1]);
	  // strcat(mOutput,"/");
	  // strcat(mOutput,outScale);
	 //  strcat(mOutput,"_");
	  // strcat(mOutput,"%d.jpg");
	//}else {
		strcat(mOutput,"/");
		strcat(mOutput,scale);
		strcat(mOutput,"_");
		strcat(mOutput,"%d.jpg");

	//}
	//strcat(mOutput,"/");
	//strcat(mOutput,scale);
	//strcat(mOutput,"_");
	//strcat(mOutput,"%d.jpg");
	//sprintf(scale,"%dx%d",width,height);
	CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));

	jint* arr;
	int sum=0;

	arr=env->GetIntArrayElements(array,NULL);//得到一个指向原始数据类型内容的指针
	jint length=env->GetArrayLength(array);//得到数组的长度
	
	int i=0;
	for(i=0;i<length;i++){
		sprintf(tmp,"%d ",(arr[i]));
	    strcat(mTimePitch,tmp);
	}

	int ret = ffmpeg_getVideoRotate(mInput);

	//s_FFMPEGSTRUCT a2 ={11,"ffmpeg","-i",mInput,"-c:v","copy","-vol",mInputVolume,"-strict","-2","-y",mOutput};  
	
	//add by jjf
	//handler_callback = handler;
	calltype = 1;
	//s_FFMPEGSTRUCT a2 ={17,"ffmpeg","-i",mInput,"-c:v","copy","-c:a","aac","-ab","64k","-ar","44100","-vol",mInputVolume,"-strict","-2","-y",mOutput};   
	__android_log_print(ANDROID_LOG_INFO, "tt", "mTimePitch is %s,%s",mTimePitch,mInput);

	if (GetJpegType == 0) {
		if (ret==2) {
            s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "transpose=2", "-getjpgtype", "d","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else if(ret == 1) {
	        s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "transpose=1","-getjpgtype", "d","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else if(ret == 3) {
	        s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "hflip,vflip","-getjpgtype", "d","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else {
	        s_FFMPEGSTRUCT a2 ={16,"ffmpeg","-i",mInput,"-getjpgtype", "d","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		}
		
	} else { 
		if (ret==2) {
            s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "transpose=2", "-getjpgtype", "e","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else if(ret == 1) {
	        s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "transpose=1","-getjpgtype", "e","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else if(ret == 3) {
	        s_FFMPEGSTRUCT a2 ={18,"ffmpeg","-i",mInput,"-vf", "hflip,vflip","-getjpgtype", "e","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		} else {
	        s_FFMPEGSTRUCT a2 ={16,"ffmpeg","-i",mInput,"-getjpgtype", "e","-getjpeg", mTimePitch,"-qmin","10","-qmax","10","-f","image2","-s",scale,mOutput};
			handle->StartThread(&a2);
		}

	}
	//while (handle->GetStateFlag() == RUNNING)
		//usleep(1200);



	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFileDir, Output);
	
    return;	
}



static void nativeChangeVolume(JNIEnv *env, jobject thiz, jstring InputFile, jstring OutputFile, double InputVolume, jobject handler)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[200];
	char mOutput[200];	
	char mInputVolume[100];
	int volume = (int)(InputVolume * 256 + 0.5);	
	sprintf(mInputVolume,"%d",(volume));
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);	
	strcpy(mInput,Input);
	strcpy(mOutput,Output);
	
	CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));

	//s_FFMPEGSTRUCT a2 ={11,"ffmpeg","-i",mInput,"-c:v","copy","-vol",mInputVolume,"-strict","-2","-y",mOutput};  
	
	//add by jjf
	handler_callback = handler;
	calltype = 2;
	//s_FFMPEGSTRUCT a2 ={17,"ffmpeg","-i",mInput,"-c:v","copy","-c:a","aac","-ab","64k","-ar","44100","-vol",mInputVolume,"-strict","-2","-y",mOutput};   
    
	s_FFMPEGSTRUCT a2 ={14,"ffmpeg","-i",mInput,"-getjpgtype", "d","-getjpeg", "1000 5000 10000 20000 25000 33500 43800 53300","-qmin","10","-qmax","10","-f","image2","/sdcard/DCIM/out_%d.jpg"}; 

	//while (handle->GetStateFlag() == RUNNING)
		//usleep(1200);
	handle->StartThread(&a2);


	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "ChangeVolume finished");
    return;	
}


static void nativeBackgroundAudio(JNIEnv *env, jobject thiz, jstring InputFile, jstring BackgroundAudio, \
	                                    jstring OutputFile, double InputVolume,jobject handler)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[200];
	char mOutput[200];
	char mAudio[200];
	char mFixAudio[400]="amovie=";
	char mInputVolume[100];
	char mBackgroundVolume[100];	
	//jniEnv = env;
	double BackgroundVolume=1-InputVolume;
	InputVolume = InputVolume*2;
	sprintf(mInputVolume,"%llf",(InputVolume));
	sprintf(mBackgroundVolume,"%llf",(BackgroundVolume));

	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);	
	const char *Audio = env->GetStringUTFChars(BackgroundAudio, NULL);
	
	handler_callback = handler;
	calltype = 1;
	strcpy(mInput,Input);
	strcpy(mOutput,Output);
	strcpy(mAudio,Audio);
	strcat(mFixAudio,mInput);
	strcat(mFixAudio,",aresample=44100,volume=");
	strcat(mFixAudio,mInputVolume);
	strcat(mFixAudio,",aconvert=s16:stereo [a0];amovie=");
 
	strcat(mFixAudio,mAudio);
	strcat(mFixAudio,",aresample=44100,volume=");
	strcat(mFixAudio,mBackgroundVolume);
	strcat(mFixAudio,",aconvert=s16:stereo [a1];[a0][a1] amerge=inputs=2");

	CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	
	s_FFMPEGSTRUCT a2 ={14,"ffmpeg","-i",mInput,"-getjpgtype", "d","-getjpeg", "1000 50000 100000 200000 250000 333500 398800 500300","-qmin","10","-qmax","10","-f","image2","/sdcard/DCIM/out_%d.jpg"};

	//s_FFMPEGSTRUCT a2 ={17,"ffmpeg","-i",mInput,"-f","lavfi","-i",mFixAudio,"-c:a","aac","-ac","2","-c:v","copy","-strict","-2","-y",mOutput};    
	handle->StartThread(&a2);

	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	env->ReleaseStringUTFChars(BackgroundAudio, Audio); 
	//env->ReleaseStringUTFChars(SdcardPath, Sdcard);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "AddAudio finished");
    return;	
}

static void nativeThumbnail(JNIEnv *env, jobject thiz, jstring InputFile, jstring OutputFile)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[100];
	char mOutput[100];
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);
	strcpy(mInput,Input);
	strcpy(mOutput,Output);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "transcode start");
	s_FFMPEGSTRUCT a ={9,"ffmpeg","-i",mInput,"-vf","thumbnail","-vframes","1","-y",mOutput};
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	handle->StartThread(&a);

	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	return;
}



static void nativeMultiConcat(JNIEnv *env, jobject thiz, jobjectArray videos, jstring OutputFile)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	FILE			*fp;
	char 			mInput[100];
	int size=env->GetArrayLength(videos);
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	while (handle->GetStateFlag() == RUNNING)
		usleep(1200);
	
	if((fp=fopen("/storage/sdcard0/concatfile.txt","w+"))==NULL) {
		printf("Cannot open file strike any key exit!");		
		return;
	}

	for (int i=0; i<size; i++) {
		jstring video = (jstring)env->GetObjectArrayElement(videos,i);
		if (video != NULL) {
			const char* mVideo = env->GetStringUTFChars(video,NULL);//??jstringààDí×a??3écharààDíê?3?
			strcpy(mInput,mVideo);
			fprintf(fp,"file '%s'\n",mInput);
			env->ReleaseStringUTFChars(video, mVideo);
		}
	}
	fclose(fp);
	fp = NULL;//test code review
	char mOutput[100];
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);
	strcpy(mOutput,Output);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "transcode start");
	s_FFMPEGSTRUCT a0 ={9,"ffmpeg","-f","concat","-i","/storage/sdcard0/concatfile.txt","-c", "copy","-y", mOutput};
	handle->StartThread(&a0);
	env->ReleaseStringUTFChars(OutputFile, Output);
	return;
}



static void nativeModule(JNIEnv *env, jobject thiz, jstring InputFile, jstring ModuleName, jstring OutputFile)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[100];
	char mOutput[100];
	char mModule[200]="movie=";
	char *lastopt=" [FlashAuto];[in][FlashAuto] overlay=0:0 [out]";
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);
	const char *Module = env->GetStringUTFChars(ModuleName, NULL);
	strcpy(mInput,Input);
	strcpy(mOutput,Output);	
	strcat(mModule,Module);
	strcat(mModule,lastopt);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "transcode start");
	s_FFMPEGSTRUCT a ={19,"ffmpeg","-i",mInput,"-vf",mModule,"-c:v","libx264","-threads", "6","-preset","ultrafast","-x264opts","bitrate=1000","-c:a","copy","-strict","-2","-y",mOutput};
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	handle->StartThread(&a);

	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	env->ReleaseStringUTFChars(ModuleName, Module); 
    
    return;	
}

static void nativeStop()
{
   ((CThread *)(CThread::__ThreadConstructer()))->StopThread();
   return;
}

static int nativegetDuration(JNIEnv *env, jobject thiz, jstring InputFile)
{
	char mInput[100];
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	strcpy(mInput,Input); 
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Duration0");

	int mSec = ffmpeg_getDuration(mInput)/1000;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "Duration=%d ms",mSec);
	env->ReleaseStringUTFChars(InputFile, Input);
	return mSec;
}


static jintArray nativegetVideoParameter(JNIEnv *env, jobject thiz, jstring InputFile)
{

	char mInput[1000];
	int  mParameter[4] = {-1,-1,-1,-1};
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	strcpy(mInput,Input); 
	//__android_log_print(ANDROID_LOG_INFO, "tt", "mInput=%s",mInput);
	//int64_t duration = ffmpeg_getDuration(mInput);	
	int ret = ffmpeg_getVideoParameter(mInput,mParameter);
	//if (ret)	
		//__android_log_print(ANDROID_LOG_INFO, "tt", "Width=%d;Height=%d;Duration=%d ms;Rotation=%d degree",mParameter[0],mParameter[1],mParameter[2],mParameter[3]);
	//else
		//__android_log_print(ANDROID_LOG_INFO, "tt", "mInput=%s do not have videostream!",mInput);
	jintArray array = env-> NewIntArray(4);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativegetVideoParameter1 结束");
	env->SetIntArrayRegion(array, 0, 4, mParameter);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativegetVideoParameter2 结束");
	env->ReleaseStringUTFChars(InputFile, Input);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativegetVideoParameter3 结束");
	return array;
}

static int nativegetBitrate(JNIEnv *env, jobject thiz, jstring InputFile)
{
	char mInput[100];
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	strcpy(mInput,Input); 

	int bitrate = ffmpeg_getBitrate(mInput);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "bitrate=%d ",bitrate);
	env->ReleaseStringUTFChars(InputFile, Input);
	return bitrate;
}


static void nativeTranscodeBitrate(JNIEnv *env, jobject thiz, jstring InputFile, int OutBitrate, jstring OutputFile)
{
	pthread_t       ffmpegThread;
	void            *tret;
	int 			err;
	char mInput[100];
	char mOutput[100];
	char bitrate[200]="bitrate=";
	char bitrate_data[100] = {0};
	char *lastopt=" [FlashAuto];[in][FlashAuto] overlay=0:0 [out]";
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);
	//const char *Module = env->GetStringUTFChars(ModuleName, NULL);
	strcpy(mInput,Input);
	strcpy(mOutput,Output);	
	sprintf(bitrate_data,"%d",OutBitrate);
	strcat(bitrate,bitrate_data);
	calltype = -1;
	//strcat(mModule,lastopt);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "transcode start");
	s_FFMPEGSTRUCT a ={17,"ffmpeg","-i",mInput,"-c:v","libx264","-threads", "4","-preset","ultrafast","-x264opts",bitrate,"-c:a","copy","-strict","-2","-y",mOutput};
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	handle->StartThread(&a);

	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	//env->ReleaseStringUTFChars(ModuleName, Module); 
    return;	
}

//add by TS
static jintArray nativegetImageData(JNIEnv *env, jobject thiz, jstring InputFile, jstring OutputFile, int Width, int Height, double timestamp)
{
    
    void*	pixels;
    int     ret; 	
	char mInput[100];
	char mStart[10];
	char mOutput[100];
	char mSeek[10];
	char mWidth[10];
	char mHeight[10];	
	char mScale[20]="scale=";
	const char *Input = env->GetStringUTFChars(InputFile, NULL);
	const char *Output = env->GetStringUTFChars(OutputFile, NULL);

	strcpy(mInput,Input);
	strcpy(mOutput,Output);
	sprintf(mWidth,"%d",(Width));
	sprintf(mHeight,"%d",(Height));
	strcat(mScale,mWidth);
	strcat(mScale,":");
	strcat(mScale,mHeight);
	int64_t mTimeStamp = (int64_t)(timestamp*1000);
	double mSeekPoint = ffmpeg_getSeekPoint(mInput,mTimeStamp);
	//__android_log_print(ANDROID_LOG_INFO, "tt", "mTimeStamp=%lld us, mSeekPoint=%lf s",mTimeStamp,mSeekPoint);
	double StartTime = timestamp/1000.0;
	StartTime = StartTime - mSeekPoint;
	sprintf(mStart,"%llf",(StartTime));
	sprintf(mSeek,"%llf",(mSeekPoint));

	int len = Width * Height;
	jintArray array = env-> NewIntArray(len);
	pixels=(void *)env->GetIntArrayElements(array,NULL);


	ffmpeg_setTag(1,&pixels);
	s_FFMPEGSTRUCT a ={13,"ffmpeg","-ss",mSeek,"-i",mInput,"-ss",mStart,"-vf",mScale,"-vframes","1","-y",mOutput};
    CThread *handle = ((CThread *)(CThread::__ThreadConstructer()));
	while (handle->GetStateFlag() == RUNNING)
		usleep(1200);
	handle->StartThread(&a);
	
	pixels = NULL ;
	ffmpeg_setTag(0,&pixels);
    
	env->ReleaseStringUTFChars(InputFile, Input);
	env->ReleaseStringUTFChars(OutputFile, Output);
	return array;
}

static int nativerealtimeInit(JNIEnv *env, jobject thiz,double fps, int width, int height, int encodeType, int deviceType, \
	                           int cpuNums, int cpuFreq, int memSize, int degree, int level, jstring OutputFile)
{
   const char *Output = env->GetStringUTFChars(OutputFile, NULL);	
   CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
   //__android_log_print(ANDROID_LOG_INFO, "tt", "nativerealtimeInit %llf %d %d %s",fps,width,degree,Output);
   s_encparam enc_param;
   enc_param.fps = fps;
   enc_param.width = width;
   enc_param.height = height;
   enc_param.encodeType = encodeType;
   enc_param.deviceType = deviceType;
   enc_param.cpuNums = cpuNums;
   enc_param.cpuFreq = cpuFreq;
   enc_param.memSize = memSize;
   enc_param.degree = degree;
   enc_param.bitrate = 500000;//unit = bps  by jjf
   //realtimeEnc_start(enc_param,level,Output);
   realtime_encoder_handle->realtime_enc_initial(enc_param, level, (char *)Output);  
   return realtime_encoder_handle->realtime_enc_start();
}

static int nativerealtimeEnc(JNIEnv *env, jobject thiz, char *data, int64_t pts)
{
   return 0;
}

static int nativerealtimeExit(JNIEnv *env, jobject thiz)
{
	CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
	return realtime_encoder_handle->realtime_enc_stop();

}

/*
{"getBitrate", 	    "(Ljava/lang/String;)I",																(void *)nativegetBitrate},
{"transcodeBitrate",		"(Ljava/lang/String;ILjava/lang/String;)V",														(void *)nativeTranscodeBitrate},

*/

static int nativeAudioDecStart(JNIEnv *env, jobject thiz, jstring InputFile)
{
	const char *Input = env->GetStringUTFChars(InputFile, NULL);

	
    CRealAudioDec *audio_decode_handle = ((CRealAudioDec *)(CRealAudioDec::__RealTimeDecConstructer()));
	int ret = audio_decode_handle->audio_dec_start(Input);

	env->ReleaseStringUTFChars(InputFile, Input);
	return ret;
}

static int nativeAudioDecStop(JNIEnv *env, jobject thiz)
{

    CRealAudioDec *audio_decode_handle = ((CRealAudioDec *)(CRealAudioDec::__RealTimeDecConstructer()));
	return audio_decode_handle->audio_dec_stop();
	
}

static int nativeAudioDecProcess(JNIEnv *env, jobject thiz, jbyteArray audioData, int len, jlong pts)
{
    CRealAudioDec *audio_decode_handle = ((CRealAudioDec *)(CRealAudioDec::__RealTimeDecConstructer()));

	jbyte* data = env->GetByteArrayElements(audioData, 0);

    long audio_Pts = pts;
    
	int ret = audio_decode_handle->audio_dec_sample((char *)data, len , audio_Pts);

	env->ReleaseByteArrayElements(audioData, data, 0);

	return ret;
}


static const JNINativeMethod gMethods[] = {
{"nativeVideoCut", 		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Landroid/os/Handler;)V",			(void *)nativeVideoCut},
{"nativeGetDuration",     "(Ljava/lang/String;)I",									                            (void *)nativegetDuration},
{"nativeGetVideoParameter",   "(Ljava/lang/String;)[I",									                            (void *)nativegetVideoParameter},
{"nativeGetImageData",	"(Ljava/lang/String;Ljava/lang/String;IID)[I",											(void *)nativegetImageData},
{"nativeProduceJpeg",	    "(Ljava/lang/String;Ljava/lang/String;III[I)V",											(void *)nativeProduceJpeg},
{"nativeMux", 				   "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DD)V", 	   (void *)nativeMux},
//{"transcodeBitrate",		"(Ljava/lang/String;ILjava/lang/String;)V",														(void *)nativeTranscodeBitrate},
{"nativeBackgroundAudio", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLandroid/os/Handler;)V", 							(void *)nativeBackgroundAudio},
{"nativeChangeVolume", 	"(Ljava/lang/String;Ljava/lang/String;DLandroid/os/Handler;)V", 											(void *)nativeChangeVolume},
{"nativeThumbnail", 		"(Ljava/lang/String;Ljava/lang/String;)V", 												(void *)nativeThumbnail},
{"nativeMultiConcat",		"([Ljava/lang/String;Ljava/lang/String;)V",												(void *)nativeMultiConcat},
{"nativeModule", 			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", 							(void *)nativeModule},
{"nativeStop", 			"()V",														 							(void *)nativeStop},
{"nativeRealtimeInit",	"(DIIIIIIIIILjava/lang/String;)I",															    (void *)nativerealtimeInit},
{"nativeRealtimeEnc",	    "([BJ)I", 															                    (void *)nativerealtimeEnc},
{"nativeRealtimeEncExit", "()I",																                    (void *)nativerealtimeExit},
{"nativeAudiodecStart",	"(Ljava/lang/String;)I",																(void *)nativeAudioDecStart},
{"nativeAudiodecProcess", 	"([BIJ)I",																				(void *)nativeAudioDecProcess},
{"nativeAudiodecStop",    "()I",																					(void *)nativeAudioDecStop},
};

int registerMethods(JNIEnv* env)
{
    jclass clazz;
	jint result = JNI_ERR;
    clazz = env->FindClass(gClassName);
    if (clazz == NULL)
	{
        goto end;
    }
	//jniEnv = env;
	gClass = clazz;

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/sizeof(gMethods[0])) != JNI_OK) 
	{
        goto end;
    }

    if (on_progress_callback == NULL) 
        on_progress_callback = env->GetStaticMethodID(clazz, "onProgress_s","(IILandroid/os/Handler;)V");

    if (on_error_callback == NULL) 
        on_error_callback = env->GetStaticMethodID(clazz, "onError_s","(IILandroid/os/Handler;)V");

	if(on_jpegnumber_callback == NULL) 
		on_jpegnumber_callback = env->GetStaticMethodID(clazz, "onJpegCallBack_s","(II)V");

    if(on_progress_callback != NULL)
       CThread::__SetCallback(0,setProgress);  

	if(on_error_callback != NULL)
       CThread::__SetCallback(1,SetErrorFlag); 

	if(on_jpegnumber_callback != NULL)
       CThread::__SetCallback(2,setJpegNumber); 
	//__android_log_print(ANDROID_LOG_INFO, "tt", "on_progress_callback %d,%d",on_progress_callback,\
		                //                         on_error_callback);

	result = JNI_OK;
end:
	return result;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) 
{
    JNIEnv* env = NULL;
    jint result = JNI_ERR;
    gVm = vm;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) 
	{
        goto end;
    }

    if(registerMethods(env) != JNI_OK) 
	{	
    	goto end;
    }
	//__android_log_print(ANDROID_LOG_INFO, "tt", "JNI_OnLoad %d",gVm);

    result = JNI_VERSION_1_4;
end:
    return result;
}


int RealtimeEnc_videoproc(char *data, int type, __uint64_t pts) {
	CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
	realtime_encoder_handle->realtime_enc_set_videodata(data, type, pts);

	return 0;
}

int RealtimeEnc_audioproc(short *data, int len, __uint64_t pts) {
	CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
	realtime_encoder_handle->realtime_enc_set_audiodata((char *) data, len, pts);
	return 0;
	
}

int RealtimeStart(double fps, int width, int height, int encodeType, int deviceType, \
	               int cpuNums, int cpuFreq, int memSize, int degree, int level, \
	               int audiofreq, int audioChannel, int audiobitDepth, char *OutputFile)
{
	CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativerealtimeInit %llf %d %d %s",fps,width,degree,OutputFile);
	s_encparam enc_param;
	enc_param.fps = fps;
	enc_param.width = width;
	enc_param.height = height;
	enc_param.encodeType = encodeType;
	enc_param.deviceType = deviceType;
	enc_param.cpuNums = cpuNums;
	enc_param.cpuFreq = cpuFreq;
	enc_param.memSize = memSize;
	enc_param.degree = degree;
	enc_param.bitrate = 1000000;
	enc_param.audioFreq = audiofreq;
	enc_param.audioChannels = audioChannel;
	enc_param.audiobitDepth = audiobitDepth;
	//realtimeEnc_start(enc_param,level,Output);
	if (width>=720 && height>=720) 
		enc_param.bitrate = 2000000;
	else if(width>=1080 && height>=1080)
		enc_param.bitrate = 2800000;
	//__android_log_print(ANDROID_LOG_INFO, "tt", "nativerealtimeInit %llf %d %d %s",fps,width,degree,OutputFile);
	realtime_encoder_handle->realtime_enc_initial(enc_param, level, (char *)OutputFile);  
	return realtime_encoder_handle->realtime_enc_start();

}

int RealtimeStop()
{
	CRealTimeEnc *realtime_encoder_handle = ((CRealTimeEnc *)(CRealTimeEnc::__RealTimeEncConstructer()));
    int tmp = realtime_encoder_handle->realtime_enc_stop();
	//__android_log_print(ANDROID_LOG_INFO, "tt", "RealtimeStop %d",tmp);
	return tmp;
}

