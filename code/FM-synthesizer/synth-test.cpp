
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
		const unsigned seconds = 2;
		const float frequency = 220.f;
		float pitch = CalculatePitch(frequency);
	
		float buffer[kSampleRate*seconds];

		float phase = 0.f;
		for (unsigned iSample = 0; iSample < kSampleRate*seconds; ++iSample)
		{
			float phase = iSample*pitch;

			// Broken
			float sample = oscPolyPulse(phase, frequency, 0.33f);
//			float sample = oscSofterTriangle(phase);
			
			// Status: OK
//			float sample = oscPolySquare(phase, frequency);
//			float sample = oscPolySaw(phase, frequency);

			// Status: OK+
//			float sample = oscSoftSquare(phase, 8);
//			float sample = oscDigiSaw(phase);
//			float sample = oscDigiTriangle(phase);
//			float sample = oscDigiPulse(phase, 0.25f);
//			float sample = oscSoftSaw(phase, 16);

			buffer[iSample] = sample;
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
				ReleaseVoice(indexB, 1.f);
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
				ReleaseVoice(indexA, 1.f);
				indexA = -1;
			}

			TriggerVoice(&indexB, kSine, 220.f, 0.75f);

			triggerB = false;
			triggerA = true;
		}
	}
}
