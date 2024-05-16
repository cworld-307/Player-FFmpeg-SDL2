#pragma once

#ifndef TIMER_H
#define TIMER_H

extern "C" {
#include "SDL.h"
}


/*
* 
* 定时刷新播放器图像
* 
*/

typedef void(*TimerOutCb)();

class Timer
{
public:
	Timer();
	void start(void* cb, int interval);
	void stop();

private:
	SDL_TimerID m_timerId = 0;
};

#endif //TIMER_H
