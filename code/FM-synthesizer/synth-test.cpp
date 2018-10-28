
/*
	Syntherklaas FM -- Misc. test functions.
*/

#include "synth-global.h"
// #include "synth-test.h"
#include "synth-oscillators.h"
#include "FM_BISON.h"

namespace SFM 
{
	void OscTest()
	{
		const unsigned seconds = 4;
		const float frequency = 1.f;
		float pitch = CalculateOscPitch(frequency);
	
		float buffer[kSampleRate*seconds];

		float phase = 0.f;
		for (unsigned iSample = 0; iSample < kSampleRate*seconds; ++iSample)
		{
			float sample = oscSoftSquare(phase, 32);
			buffer[iSample] = sample;
			phase += pitch;
		}

		FILE *file = fopen("oscTest.raw", "wb");
		fwrite(buffer, sizeof(float), kSampleRate*seconds, file);
		fclose(file);
	}

	void ClickTest(float time, float speed)
	{
		static unsigned indexA = -1, indexB = -1;
		static bool triggerA = true, triggerB = false;

		time = fmodf(time, speed);

		if (triggerA && time < speed*0.5f)
		{
			if (-1 != indexB) 
			{
				ReleaseVoice(indexB);
				indexB = -1;
			}

			TriggerVoice(&indexA, kSine, 440.f, 0.75f);

			triggerA = false;
			triggerB = true;
		}
		else if (triggerB && time >= speed*0.5f)
		{
			if (-1 != indexA)
			{
				ReleaseVoice(indexA);
				indexA = -1;
			}

			TriggerVoice(&indexB, kSine, 220.f, 0.75f);

			triggerB = false;
			triggerA = true;
		}
	}
}
