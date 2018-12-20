
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

	static int s_handle = -1;

	bool SDL2_CreateAudio(SDL_AudioCallback callback)
	{
		SDL_AudioSpec audioSpec;
		audioSpec.freq = kSampleRate;
		audioSpec.format = AUDIO_F32;
		audioSpec.channels = 2;
		audioSpec.samples = kMinSamplesPerUpdate; 
		audioSpec.callback = (nullptr != callback) ? callback : SDL2_Callback_Sine;
//		audioSpec.callback = NULL;
		audioSpec.userdata = NULL;

		const int result = SDL_OpenAudio(&audioSpec, NULL);
		if (result < 0)
		{
			SFM_ASSERT(false);
			return false;
		}

		s_handle = result;

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

	int SDL2_GetAudioDevice()
	{
		return s_handle;
	}
}
