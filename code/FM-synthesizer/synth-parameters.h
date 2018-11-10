
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-ADSR.h"
#include "synth-filter.h"
#include "synth-formant.h"

namespace SFM
{
	// Available algorithms
	enum Algorithm
	{
		kSingle,
		kDoubleCarriers,
		kMiniMOOG,
		kNumAlgorithms
	};

	// Most global parameters; some are still "pulled" in place (FIXME)
	struct Parameters
	{
		Algorithm m_algorithm;

		// Algorithm #2
		float m_doubleDetune;
		float m_doubleVolume;

		// Algorithm #3 (oscillator 2 & 3 forms are pulled in place)
		bool  m_hardSync;
		float m_slavesDetune;
		float m_slavesLP;
		float m_slaveFM;
		float m_carrierVol[3];
		
		// Master drive [0..N]
		float m_drive;

		// Modulator parameters [0..1]
		float m_modIndex;
		float m_modRatioC, m_modRatioM;
		float m_modBrightness;
		float m_indexLFOFreq;

		// Noise, formant shaper parameters, Nintendize, tremolo
		float m_noisyness;
		FormantShaper::Vowel m_formantVowel;
		float m_formant;
		float m_formantStep;
		float m_Nintendize;
		float m_tremolo;

		// Loop wavetable oscillators
		bool m_loopWaves;

		// Pulse oscillator width
		float m_pulseWidth;

		// ADSR parameters
		ADSR::Parameters m_voiceADSR;
		ADSR::Parameters m_filterADSR;

		// Filter parameters
		VoiceFilter m_curFilter;
		float m_filterContour; // Filter contour (ADSR influence) [0..1]
		bool m_flipFilterEnv;  // Filter envelope inversion
		FilterParameters m_filterParams;

		// Feedback (delay) effect parameters
		float m_feedback;
		float m_feedbackWetness;
		float m_feedbackPitch;

		void SetDefaults()
		{
			// Single carrier algorithm, zero out other parameters
			m_algorithm = kSingle;

			// Algo #2
			m_doubleDetune = 0.f;
			m_doubleVolume = 0.f;

			// Algo #3
			m_hardSync      = false;
			m_slavesDetune  = 0.f;
			m_slavesLP      = 0.f;
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

			// No noise
			m_noisyness = 0.f;

			// No shaping
			m_formantVowel = FormantShaper::kNeutral;
			m_formant = 0.f;
			m_formantStep = 0.f;

			// No chiptune-ish crap
			m_Nintendize = 0.f;

			// No tremolo
			m_tremolo = 0.f;

			// Don't loop waves
			m_loopWaves = false;

			// Default pulse width
			m_pulseWidth = kPI*0.1f;

			// Default ADSR envelopes
			m_voiceADSR.attack  = kSampleRate/8;
			m_voiceADSR.decay   = kSampleRate/4;
			m_voiceADSR.release = kSampleRate/4;
			m_voiceADSR.sustain = kRootHalf;

			// Zero filter ADS(R)
			m_filterADSR.attack  = 0.f;
			m_filterADSR.release = 0.f;
			m_filterADSR.sustain = 0.f;
			m_filterADSR.release = 0.f;

			// Default filter
			m_curFilter = kTeemuFilter;
			m_filterContour = 0.f;
			m_flipFilterEnv = false;
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
