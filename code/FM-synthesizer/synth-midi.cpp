
/*
	Syntherklaas -- Generic MIDI-related.
*/

#include "synth-global.h"
#include "synth-midi.h"

namespace SFM
{
	float g_midiToFreqLUT[127];

	void CalcMidiToFreqLUT()
	{
		const float base = 440.f;
		for (unsigned iKey = 0; iKey < 127; ++iKey)
		{
			const float frequency = base * powf(2.f, (iKey-69.f)/12.f);
			g_midiToFreqLUT[iKey] = floorf(frequency);
		}
	}
}
