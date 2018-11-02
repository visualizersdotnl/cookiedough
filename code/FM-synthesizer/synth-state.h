
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

		// Algorithm
		Voice::Algorithm m_algorithm;
		float m_algoTweak;

		// Master drive [0..N]
		float m_drive;

		// Modulator parameters [0..1]
		float m_modIndex;
		float m_modRatioC, m_modRatioM;
		float m_modBrightness;
		float m_indexLFOFreq;

		// Tremolo
		float m_tremolo;

		// Loop wavetable oscillators
		bool m_loopWaves;

		// Pulse oscillator width (no multiples, that tends to sound likewise)
		float m_pulseWidth;

		// ADSR parameters
		ADSR::Parameters m_voiceADSR;
		ADSR::Parameters m_filterADSR;

		// Filter contour (ADSR influence) [0..1]
		float m_filterContour;

		// Filter parameters
		VoiceFilter m_curFilter;
		FilterParameters m_filterParams;

		// Feedback (delay) effect parameters
		float m_feedback;
		float m_feedbackWetness;
		float m_feedbackPitch;

		void Reset(unsigned sampleCount)
		{
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
				m_voices[iVoice].m_enabled = false;

			m_active = 0;

			// Simple algorithm
			m_algorithm = Voice::kSingle;
			m_algoTweak = 0.f;

			// Neutral
			m_drive = 1.f;

			// No FM
			m_modIndex = 0.f;
			m_modRatioC = 0.f;
			m_modRatioM = 0.f;
			m_modBrightness = 0.f;
			m_indexLFOFreq = 0.f;

			// No tremolo
			m_tremolo = 0.0f;

			// Don't loop waves
			m_loopWaves = false;

			// Smallest pulse width
			m_pulseWidth = kPulseWidths[0];

			// Default ADSR envelopes
			m_voiceADSR.attack  = kSampleRate/8;
			m_voiceADSR.decay   = kSampleRate/4;
			m_voiceADSR.release = kSampleRate/4;
			m_voiceADSR.sustain = kRootHalf;

			// Neutered copys for filter
			m_filterADSR = m_voiceADSR;
			m_filterADSR.sustain = 0.f;
			m_filterADSR.release = 0.f;

			// Default filter
			m_curFilter = kTeemuFilter;
			m_filterContour = 0.f;
			m_filterParams.drive = 1.f;
			m_filterParams.cutoff = 1.f;
			m_filterParams.resonance = 0.1f;

			// Default feedback
			m_feedback = 0.f;
			m_feedbackWetness = 0.f;
			m_feedbackPitch = 0.5f;
		}
	};
}
