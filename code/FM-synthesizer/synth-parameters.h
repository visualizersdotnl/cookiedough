
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-voice.h"
#include "synth-ADSR.h"
#include "synth-filter.h"

namespace SFM
{
	struct Parameters
	{
		// Algorithms
		Voice::Algo m_algorithm;

		// Algorithm #2
		float m_doubleDetune;
		float m_doubleVolume;

		// Algorithm #3 (oscillator 2 & 3 forms are pulled in place)
		bool  m_hardSync;
		float m_slavesDetune;
		float m_slaveFM;
		float m_carrierVol[3];
		
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

		// Pulse oscillator width
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

		void SetDefaults()
		{
			// Single carrier algorithm, zero out other parameters
			m_algorithm = Voice::Algo::kSingle;

			// Algo #2
			m_doubleDetune = 0.f;
			m_doubleVolume = 0.f;

			// Algo #3
			m_hardSync      = false;
			m_slavesDetune  = 0.f;
			m_slaveFM       = 0.f;
			m_carrierVol[0] = 1.f;
			m_carrierVol[1] = 0.f;
			m_carrierVol[2] = 0.f;

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

			// Default pulse width
			m_pulseWidth = kPI*0.1f;

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