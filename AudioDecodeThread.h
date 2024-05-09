#pragma once

#ifndef AUDIODECODETHREAD_H
#define AUDIODECODETHREAD_H

#include "ThreadBase.h"
#include "FFmpegPlayerCtx.h"

double get_audio_clock(FFmpegPlayerCtx* is);

//struct FFmpegPlayerCtx;

class AudioDecodeThread : public ThreadBase
{
public:
	AudioDecodeThread();
	void setPlayerCtx(FFmpegPlayerCtx* ctx);
	void run();
	void stop();
	void getAudioData(unsigned char* stream, int len);

private:
	// void getAudioData(unsigned char* stream, int len);
	int audio_decode_frame(FFmpegPlayerCtx* is, double* pts_ptr);

private:
	FFmpegPlayerCtx* playCtx;
};

#endif // AUDIODECODETHREAD_H
