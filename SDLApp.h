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
	//m_userEventMaps存放的就是键为事件类型，值为处理该类型的一个方法(回调函数)
	std::map<int, std::function<void(SDL_Event*)> > m_userEventMaps;
};

#endif //SDLAPP_H