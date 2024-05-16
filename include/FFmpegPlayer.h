#pragma once

#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include "DemuxThread.h"
#include "AudioDecodeThread.h"
#include "VideoDecodeThread.h"
#include "AudioPlay.h"
#include "Timer.h"

class FFmpegPlayer
{
public:
	FFmpegPlayer();
	void setFilePath(const char* filePath);
	//void setImageCb(Image_Cb cb, void* userData);
	int initPlayer();
	void start();
	void stop();
	//void pause(PauseState state);

public:
	void onRefreshEvent(SDL_Event* e);
	void onKeyEvent(SDL_Event* e);

private:
	FFmpegPlayerCtx playerCtx;
	std::string m_filePath;
	SDL_AudioSpec audio_wanted_spec;
	std::atomic<bool> m_stop{false};

	// 静态函数，用于包装成员函数调用
	static void AudioDecodeThread_callback(void* userdata, unsigned char* stream, int len);

private:
	DemuxThread* m_demuxThread = nullptr;
	VideoDecodeThread* m_videoDecodeThread = nullptr;
	AudioDecodeThread* m_audioDecodeThread = nullptr;
	AudioPlay* m_audioPlay = nullptr;
	//Timer* m_timer = nullptr;
};
#endif