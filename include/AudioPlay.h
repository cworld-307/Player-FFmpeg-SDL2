#pragma once
#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H

extern "C" {
#include <SDL.h>
}


class AudioPlay
{
public:
	AudioPlay();
	int openDevice(SDL_AudioSpec* spec);
	void start();
	void stop();

};

#endif // AUDIOPLAY_H
