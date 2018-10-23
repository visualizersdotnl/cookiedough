

/*
	Syntherklaas FM -- Oscillators.
*/

#include "synth-global.h"
#include "synth-oscillators.h"

// Wavetable(s)
#include "wavetable/oscWav808.h"

namespace SFM
{
	/*
		Wavetable oscillator(s).
	*/

	const size_t kTabSize = sizeof(s_oscWav808)/sizeof(float);
	const float kTabDiv = kTabSize/kOscPeriod;

	float oscKick808(float phase)
	{
		SFM_ASSERT(kTabSize == kSampleRate);
		const float *pTab = reinterpret_cast<const float*>(s_oscWav808);
		const unsigned sample = unsigned(phase/kTabDiv);
		return pTab[sample%kTabSize];
	}
}
