
/*
	Syntherklaas -- General MIDI utilities.
*/

#include "synth-global.h"
#include "synth-midi.h"

namespace SFM
{
	float g_midiToFreqLUT[127];

	void CalculateMidiToFrequencyLUT(float baseFreq)
	{
		const float base = baseFreq;
		for (unsigned iKey = 0; iKey < 127; ++iKey)
		{
			const float frequency = base * powf(2.f, (iKey-69.f)/12.f);
			g_midiToFreqLUT[iKey] = fabsf(frequency);
		}
	}
}
