
/*
	Syntherklaas FM: SDL2 audio output.
*/

#include "synth-global.h"
#include "SDL2-audio-out.h"

namespace SFM
{
	// Test function
	static void SDL2_Callback_Sine(void *pData, uint8_t *pStream, int length)
	{
		static float phase = 0.f;
		unsigned numSamplesReq = length/sizeof(float);
		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamplesReq; ++iSample)
		{
			*pWrite++ = sinf(phase);
			phase += CalculateAngularPitch(440.f);
		}
	}

	bool SDL2_CreateAudio(SDL_AudioCallback callback)
	{
		SDL_AudioSpec necSpec;
		necSpec.freq = 44100;
		necSpec.format = AUDIO_F32;
		necSpec.channels = 1;
		necSpec.samples = kRingBufferSize;
		necSpec.callback = (nullptr != callback) ? callback : SDL2_Callback_Sine;
		necSpec.userdata = NULL;

		const int result = SDL_OpenAudio(&necSpec, NULL);
		if (result < 0)
		{
			SFM_ASSERT(false);
			return false;
		}

		return true;
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
