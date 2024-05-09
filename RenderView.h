#pragma once
#ifndef RENDERVIEW_H
#define RENDERVIEW_H

extern "C" {
#include <SDL.h>
}

#include <list>
#include <mutex>

struct RenderItem
{
	SDL_Texture* texture;
	SDL_Rect srcRect;
	SDL_Rect dstRect;
};

class RenderView {
public:
	explicit RenderView();
	void setNativeHandle(void* handle);
	int initSDL();
	RenderItem* createRGB24Texture(int w, int h);
	void updateTexture(RenderItem* item, unsigned char* pixelData, int rows);
	void onRefresh();

private:
	SDL_Window* m_sdlWindow = nullptr;
	SDL_Renderer* m_sdlRender = nullptr;
	void* m_nativeHandle = nullptr;
	std::list<RenderItem*> m_items;
	std::mutex m_updateMutex;
};

#endif //RENDERVIEW_H