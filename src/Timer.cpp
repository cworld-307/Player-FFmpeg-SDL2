#include "Timer.h"

//��ʱ���ص�����
static Uint32 callbackfunc(Uint32 interval, void* param)
{
	SDL_Event event;
	SDL_UserEvent userevent;
	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = param;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);

	return interval;
}

Timer::Timer()
{
}

void Timer::start(void* cb, int interval)
{
	//��ʱ����ʼ
	if (m_timerId != 0)
	{
		return;
	}

	//����¼�ʱ��
	SDL_TimerID timerId = SDL_AddTimer(interval, callbackfunc, cb);
	if (timerId == 0)
	{
		return;
	}

	m_timerId = timerId;

}

void Timer::stop()
{
	if (m_timerId != 0)
	{
		//�����ʱ��
		SDL_RemoveTimer(m_timerId);
		m_timerId = 0;
	}
}
