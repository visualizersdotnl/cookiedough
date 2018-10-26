
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

		// [0..2] where 1 is neutral, 0 silent, 2 obviously overdrive (no hard limit)
		float m_drive;

		// Master modulation index value, ratio & brightness (used on note trigger)
		// [0..1]
		float m_modIndex;
		float m_modRatioC, m_modRatioM;
		float m_modBrightness;

		// Global filter wetness
		// [0..1]
		float m_wetness;

		// Master ADSR parameters
		ADSR::Parameters m_ADSR;

		// Master filter parameters
		FilterParameters m_filterParams;

		// Feedback (delay) effect parameters
		float m_feedback;
		float m_feedbackWetness;
		float m_feedbackPhaser;

		// Master FM index LFO parameters
		IndexEnvelope::Parameters m_indexLFOParams;

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

			// No filter
			m_wetness = 0.f;

			// Default ADSR envelope
			m_ADSR.attack  = kSampleRate/8;
			m_ADSR.decay   = kSampleRate/4;
			m_ADSR.release = kSampleRate/4;
			m_ADSR.sustain = kRootHalf;

			// Default filter 
			m_filterParams.cutoff = 1.f;
			m_filterParams.resonance = 0.1f;
			m_filterParams.envInfl = 0.f;

			// Default feedback
			m_feedback = 0.f;
			m_feedbackWetness = 0.f;
			m_feedbackPhaser = 0.5f;

			// Default FM index modulation
			m_indexLFOParams.curve = 0.f;
			m_indexLFOParams.frequency = 1.f;
			m_indexLFOParams.shape = 0.f;
		}
	};
}
