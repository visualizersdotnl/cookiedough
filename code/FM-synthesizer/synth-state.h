
/*
	Syntherklaas FM -- Global (state) PODs.

	Everything is copied per render cycle to a 'live' state; because of this it is important
	*not* to have any state altered during rendering as it will be lost.
*/

#pragma once

#include "synth-global.h"
#include "synth-voice.h"
#include "synth-ADSR.h"
#include "synth-filter.h"

namespace SFM
{
	struct FM
	{
		Voice m_voices[kMaxVoices];
		unsigned m_active;

		// Master drive [0..N]
		float m_drive;

		// Modulator parameters [0..1]
		float m_modIndex;
		float m_modRatioC, m_modRatioM;
		float m_modBrightness;
		float m_indexLFOFreq;

		// ADSR parameters
		ADSR::Parameters m_ADSR;

		// Filter wetness [0..1]
		float m_wetness;

		// Filter parameters
		FilterParameters m_filterParams;

		// Feedback (delay) effect parameters
		float m_feedback;
		float m_feedbackWetness;
		float m_feedbackPitch;

		void Reset(unsigned sampleCount)
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
			m_modRatioC = 0.f;
 			m_modRatioM = 0.f;
			m_modBrightness = 0.f;
			m_indexLFOFreq = 0.f;

			// Default ADSR envelope
			m_ADSR.attack  = kSampleRate/8;
			m_ADSR.decay   = kSampleRate/4;
			m_ADSR.release = kSampleRate/4;
			m_ADSR.sustain = kRootHalf;

			// Default filter (none)
			m_wetness = 0.f;
			m_filterParams.cutoff = 1.f;
			m_filterParams.resonance = 0.1f;
			m_filterParams.envInfl = 0.f;

			// Default feedback
			m_feedback = 0.f;
			m_feedbackWetness = 0.f;
			m_feedbackPitch = 0.5f;
		}
	};
}
