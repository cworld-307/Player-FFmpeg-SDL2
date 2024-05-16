#pragma once
#ifndef VIDEODECODETHREAD_H
#define VIDEODECODETHREAD_H

#include "FFmpegPlayerCtx.h"
#include "ThreadBase.h"


struct FFmpegPlayerCtx;

class VideoDecodeThread : public ThreadBase
{
public:
	VideoDecodeThread();
	void setPlayerCtx(FFmpegPlayerCtx* ctx);
	void run();
	void stop();

private:
	int video_entry();
	
	static double synchronize_video(FFmpegPlayerCtx* is, AVFrame* src_frame, double pts);
	
	int queue_picture(FFmpegPlayerCtx* is, AVFrame* pFrame, double pts);

private:
	FFmpegPlayerCtx* playCtx = nullptr;

};
#endif // !VIDEODECODETHREAD_H
