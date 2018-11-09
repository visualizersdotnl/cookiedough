
/*
	Syntherklaas FM - Voice.
*/

#pragma once

#include "synth-global.h"
#include "synth-oscillator.h"
#include "synth-modulator.h"
// #include "synth-ADSR.h."
// #include "synth-filter.h"
// #include "synth-LFO.h."
#include "synth-parameters.h"
#include "synth-simple-filters.h"

namespace SFM
{
	// Initialized manually
	class Voice
	{
	public:
		Voice() :
			m_enabled(false)
		{}

		bool m_enabled;

		Algorithm m_algorithm;
		Oscillator m_carriers[3];
		Modulator m_modulator;
		LFO m_ampMod;

		// For Algorithm #3
		LowpassFilter m_LPF;

		// For wavetable samples
		bool m_oneShot;

		// For pulse-based waveforms
		float m_pulseWidth;
		
		// Filter instance for this particular voice
		LadderFilter *m_pFilter;

		float Sample(unsigned sampleCount, const Parameters &parameters);

		// Can be used to determine if a one-shot is done
		bool HasCycled(unsigned sampleCount) /* const */
		{
			switch (m_algorithm)
			{
			default:

			case kMiniMOOG: // In MiniMOOG-mode, only the first carrier is allowed non-procedural
			case kSingle:
				return m_carriers[0].HasCycled();

			case kDoubleCarriers:
				return m_carriers[1].HasCycled() || m_carriers[1].HasCycled();
			}
		}
	};
}
