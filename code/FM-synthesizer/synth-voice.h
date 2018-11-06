
/*
	Syntherklaas FM - Voice.
*/

#pragma once

#include "synth-global.h"
	#include "synth-carrier.h"
#include "synth-modulator.h"
#include "synth-ADSR.h."
#include "synth-filter.h"

namespace SFM
{
	// Initialized manually
	class Voice
	{
	public:
		enum Algo
		{
			kSingle,
			kDoubleCarriers,
			kMiniMOOG,
			kNumAlgorithms
		};

		Voice() :
			m_enabled(false)
		{}

		bool m_enabled;

		Algo m_algorithm;
		Carrier m_carriers[3];
		Modulator m_modulator;
		Modulator m_ampMod;

		// For wavetable samples
		bool m_oneShot;

		// For pulse-based waveforms
		float m_pulseWidth;
		
		// Filter instance for this particular voice
		LadderFilter *m_pFilter;

		// Pointer to 3 floats expected if algorithm is kMiniMOOG, just 1 in case of kDoubleCarriers
		float Sample(unsigned sampleCount, float brightness, float noise, const float carrierVolumes[]);

		// Can be used to determine if a one-shot is done
		bool HasCycled(unsigned sampleCount) /* const */
		{
			switch (m_algorithm)
			{
			default:

			case kMiniMOOG:
				// In MiniMOOG-mode, only the first carrier is allowed non-procedural

			case kSingle:
				return m_carriers[0].HasCycled(sampleCount);

			case kDoubleCarriers:
				return m_carriers[1].HasCycled(sampleCount) || m_carriers[1].HasCycled(sampleCount);
			}
		}
	};
}
