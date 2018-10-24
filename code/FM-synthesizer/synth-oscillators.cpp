

/*
	Syntherklaas FM -- Oscillators.
*/

#include "synth-global.h"
#include "synth-oscillators.h"

// Wavetable(s)
// Not included in the workspace since Visual Studio 2017 kept crashing when I did
#include "wavetable/wavKick808.h" // 1 sec.
#include "wavetable/wavSnare808.h" // 0.2 sec.

namespace SFM
{
	/*
		Wavetable oscillator(s).

		FIXME: 
			- add little wrapper class
			- resample these to a multiple of the LUT period size
	*/

	const size_t kKickLen = sizeof(s_wavKick808)/sizeof(float);
	const float kKickDiv = kKickLen/kOscPeriod;

	float oscKick808(float phase)
	{
		const float *pTab = reinterpret_cast<const float*>(s_wavKick808);
		const unsigned sample = unsigned(phase/kKickDiv);
		return pTab[sample%kKickLen];
	}

	const size_t kSnareLen = sizeof(s_wavSnare808)/sizeof(float);
	const float kSnareDiv = (kSnareLen/kOscPeriod)*12.f;

	float oscSnare808(float phase)
	{
		const float *pTab = reinterpret_cast<const float*>(s_wavSnare808);
		const unsigned sample = unsigned(phase/kSnareDiv);
		return pTab[sample%kSnareLen];
	}
}
