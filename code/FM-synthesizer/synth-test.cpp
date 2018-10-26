
/*
	Syntherklaas FM -- Misc. test functions.
*/

#include "synth-global.h"
// #include "synth-test.h"
#include "synth-oscillators.h"
// #include "synth-util.h"
#include "FM_BISON.h"
#include "synth-ringbuffer.h"

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
	void CrackleTest(float time, float speed)
	{
		static unsigned iA = -1, iB = -1;
		static bool triggerA = true, triggerB = false;

		float second = fmodf(time, speed);

		if (triggerA && second < speed*0.5f)
		{
			if (-1 != iB) ReleaseVoice(iB);

			iA = TriggerVoice(kSine, 440.f, 0.75f);

			triggerA = false;
			triggerB = true;
		}
		else if (triggerB && second >= speed*0.5f)
		{
			if (-1 != iA) ReleaseVoice(iA);

			iB = TriggerVoice(kSine, 220.f, 0.75f);

			triggerB = false;
			triggerA = true;
		}
	}
}
