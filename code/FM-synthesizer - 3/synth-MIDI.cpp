
/*
	Syntherklaas FM -- General MIDI utilities.
*/

#include "synth-global.h"
#include "synth-midi.h"

namespace SFM
{
	float g_MIDIToFreqLUT[127];
	float g_MIDIFreqRange;

	void CalculateMIDIToFrequencyLUT()
	{
		const float base = kBaseHz;
		for (unsigned iKey = 0; iKey < 127; ++iKey)
		{
			const float frequency = base * powf(2.f, (iKey-69.f)/12.f);
			g_MIDIToFreqLUT[iKey] = fabsf(frequency);
		}

		g_MIDIFreqRange = fabsf(g_MIDIToFreqLUT[126]-g_MIDIToFreqLUT[0]);
	}
}
