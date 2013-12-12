#ifndef REALTIME_INT_H
#define REALTIME_INT_H


int RealtimeStart(double fps, int width, int height, int encodeType, int deviceType, \
	              int cpuNums, int cpuFreq, int memSize, int degree, int level, \
	              int audiofreq, int audioChannel, int audiobitDepth, char *OutputFile);

int RealtimeStop();
	                           
int RealtimeEnc_videoproc(char *data, int type, __uint64_t pts);
int RealtimeEnc_audioproc(short *data, int len, __uint64_t pts);

#endif