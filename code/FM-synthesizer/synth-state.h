
/*
	Syntherklaas FM -- Global (state) PODs.

	Everything is copied per render cycle to a 'live' state; because of this it is important
	*not* to have any state altered during rendering as it will be lost.
*/

#pragma once

#include "synth-global.h"
#include "synth-voice.h"
#include "synth-ADSR.h"
#include "synth-cosine-tilt.h"

namespace SFM
{
	struct FM
	{
		Voice m_voices[kMaxVoices];
		unsigned m_active;

		// [0..2] where 1 is neutral, 0 silent, 2 obviously overdrive (no hard limit)
		float m_drive;

		// Master modulation index value & ratio (used on note trigger)
		// [0..1]
		float m_modIndex;
		float m_modRatio;

		// Filter wetness
		// [0..1]
		float m_wetness;

		// Master ADSR parameters
		ADSR::Parameters m_ADSR;

		// LFO for modulation index (osc. period length)
		float m_modIndexLFO[kOscPeriod];

		void Reset()
		{
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				m_voices[iVoice].m_enabled = false;
			}

			m_active = 0;

			// Neutral
			m_drive = 1.f;

			// No FM
			m_modIndex = 0.f;
			m_modRatio = 0.f;

			// No filter
			m_wetness = 0.f;

			// Default ADSR envelope
			m_ADSR.attack  = kSampleRate/8;
			m_ADSR.decay   = kSampleRate/4;
			m_ADSR.release = kSampleRate/4;
			m_ADSR.sustain = kRootHalf;

			// Std. modulation index LFO
			CalculateCosineTiltEnvelope(m_modIndexLFO, kOscPeriod, 0.f, 0.f, 1.f);
		}
	};
}
