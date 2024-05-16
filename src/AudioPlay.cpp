#include "AudioPlay.h"
#include <iostream>

AudioPlay::AudioPlay()
{
}

int AudioPlay::openDevice(SDL_AudioSpec* spec)
{
	int ret = SDL_OpenAudio(spec, NULL);
	return ret;
}

void AudioPlay::start()
{
	SDL_PauseAudio(0);
}

void AudioPlay::stop()
{
	SDL_PauseAudio(1);
}
