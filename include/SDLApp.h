#pragma once
#ifndef SDLAPP_H
#define SDLAPP_H

extern "C" {
#include "SDL.h"
}
#include <map>
#include <functional>

#define sdlApp (SDLApp::instance())

class SDLApp
{
public:
	SDLApp();

public:
	int exec();
	void quit();
	void registerEvent(int type, const std::function<void(SDL_Event*)>& cb);
	static SDLApp* instance();

private:
	//m_userEventMaps��ŵľ��Ǽ�Ϊ�¼����ͣ�ֵΪ��������͵�һ������(�ص�����)
	std::map<int, std::function<void(SDL_Event*)> > m_userEventMaps;
};

#endif //SDLAPP_H