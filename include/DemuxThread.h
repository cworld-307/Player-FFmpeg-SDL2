#pragma once
#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include "ThreadBase.h"
#include <string>
#include "FFmpegPlayerCtx.h"

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

struct FFmpegPlayerCtx;

/**
 * @brief Class representing a demultiplexer thread.
 *
 * �����ԭʼ���������װΪ��Ƶ������Ƶ��������Ӧ������Ƶ���к���Ƶ���С�
 * 
 */
class DemuxThread : public ThreadBase
{
public:
	DemuxThread();
	void setPlayerCtx(FFmpegPlayerCtx* ctx);
	int initDemuxThread();
	void finiDemuxThread();
	void run();
	void stop();

private:
	int decode_loop();
	int audio_decode_frame(FFmpegPlayerCtx* is, double *pts_ptr);
	int stream_open(FFmpegPlayerCtx* is, int media_type);

private:
	FFmpegPlayerCtx* is = nullptr;
};
#endif //DEMUXTHREAD_H