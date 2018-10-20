
/*
	Syntherklaas FM -- Test functions.
*/

#include "synth-global.h"
// #include "synth-test.h"
#include "synth-oscillators.h"
// #include "synth-util.h"

namespace SFM 
{
	static void OscTest()
	{
		const unsigned seconds = 4;
		const float frequency = 1.f;
		float pitch = CalculateOscPitch(frequency);
	
		float buffer[kSampleRate*seconds];

		float phase = 0.f;
		for (unsigned iSample = 0; iSample < kSampleRate*seconds; ++iSample)
		{
			float sample = oscDigiSaw(phase); // , 40);
			buffer[iSample] = sample;
			phase += pitch;
		}

		FILE *file = fopen("oscTest.raw", "wb");
		fwrite(buffer, sizeof(float), kSampleRate*seconds, file);
		fclose(file);
	}
}