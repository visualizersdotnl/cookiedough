
/*
	Syntherklaas FM: SDL2 audio output.
*/

#pragma once

#include "../../3rdparty/SDL2-2.0.8/include/SDL.h"

namespace SFM
{
	bool SDL2_CreateAudio(SDL_AudioCallback callback);
	void SDL2_DestroyAudio();
	void SDL2_StartAudio();
	int  SDL2_GetAudioDevice();
}
