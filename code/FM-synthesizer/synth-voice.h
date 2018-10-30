
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
	// To be initialized manually
	class Voice
	{
	public:
		enum Algorithm
		{
			kSingle,
			kDetunedCarriers,
			kNumAlgorithms
		};

		Voice() :
			m_enabled(false)
		{}

		bool m_enabled;

		Algorithm m_algorithm;
		Carrier m_carrierA, m_carrierB;
		Modulator m_modulator;
		Modulator m_ampMod;
		bool m_oneShot; // For wavetable samples
		LadderFilter *m_pFilter;

		// Call before use to initialize pitched carriers and modulators (used for pitch bend)
		// Not the most elegant solution, but it doesn't depend on any delay line(s)
		Carrier m_carrierHi, m_carrierLo;
		Modulator m_modulatorHi, m_modulatorLo;
		void InitializeFeedback();

		float Sample(unsigned sampleCount, float pitchBend, float modBrightness, ADSR &envelope);

		// Can be used to determine if a one-shot is done
		bool HasCycled(unsigned sampleCount) /* const */
		{
			switch (m_algorithm)
			{
			default:
			case kSingle:
				return m_carrierA.HasCycled(sampleCount);

			case kDetunedCarriers:
				return m_carrierA.HasCycled(sampleCount) || m_carrierB.HasCycled(sampleCount);
			}
		}
	};
}
