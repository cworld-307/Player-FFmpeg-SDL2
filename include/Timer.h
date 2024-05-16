#pragma once

#ifndef TIMER_H
#define TIMER_H

extern "C" {
#include "SDL.h"
}


/*
* 
* ��ʱˢ�²�����ͼ��
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
