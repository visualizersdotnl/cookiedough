
/*
	Syntherklaas FM: SDL2 audio output.
*/

#include "synth-global.h"
#include "SDL2-audio-out.h"

namespace SFM
{
	static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
	{
		memset(pStream, rand(), length);
	}

	void SDL2_CreateAudio()
	{
		SDL_AudioSpec necSpec;
		necSpec.freq = 44100;
		necSpec.format = AUDIO_F32;
		necSpec.channels = 1;
		necSpec.samples = kRingBufferSize;
		necSpec.callback = SDL2_Callback;
		necSpec.userdata = NULL;

		const int result = SDL_OpenAudio(&necSpec, NULL);
		if (result < 0)
		{
			// FIXME: error handling
			SFM_ASSERT(false);
		}
	}

	void SDL2_DestroyAudio()
	{
		SDL_CloseAudio();
	}

	void SDL2_StartAudio()
	{
		SDL_PauseAudio(0);
	}
}
