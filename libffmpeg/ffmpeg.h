#ifndef FFMPEG_MAIN_H
#define FFMPEG_MAIN_H
//ffmpeg主程序相关
typedef int (*CallBack)(int data);

int ffmpeg_main(int argc, char **argv);
void  ffmpeg_set_terminate(int sig);


int64_t ffmpeg_getDuration(const char *filename);

int ffmpeg_getVideoCodecid(const char *filename);

int ffmpeg_getVideoRotate(const char *filename);

int64_t ffmpeg_getVideoParameter(const char *filename,int *parameter);
int ffmpeg_getBitrate(const char *filename);
double ffmpeg_getSeekPoint(const char *filename, int64_t timestamp); //add by TS
void ffmpeg_setTag(int mTag,void** mPixels);

void Set_CallBack_ff(int type, CallBack func);

#endif


