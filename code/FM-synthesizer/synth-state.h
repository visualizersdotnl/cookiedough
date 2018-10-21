
/*
	Syntherklaas FM -- Global (state) PODs.

	Everything is copied per render cycle to a 'live' state; because of this it is important
	*not* to have any state altered during rendering as it will be lost.
*/

#pragma once

#include "synth-global.h"
#include "synth-voice.h"
#include "synth-ADSR.h"

namespace SFM
{
	struct FM
	{
		Voice m_voices[kMaxVoices];
		unsigned m_active;

		// [0..2] where 1 is neutral, 0 silent, 2 obviously overdrive
		float m_drive;

		// Currently used as global modulation index value (used on note trigger), probably temporary
		// [0..N]
		float m_modIndex;

		// Master ADSR parameters
		ADSR::Parameters m_ADSR;

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

			// Default ADSR envelope
			m_ADSR.attack  = kSampleRate/8;
			m_ADSR.decay   = kSampleRate/4;
			m_ADSR.release = kSampleRate/4;
			m_ADSR.sustain = kRootHalf;
		}
	};
}
