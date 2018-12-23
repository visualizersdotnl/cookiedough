
/*
	Syntherklaas FM - Yamaha DX-style voice.

	Important:
		- An operator can only be modulated by an operator above it (index)
		- Feedback can be taken from any level

	This does not fully emulate a DX7 voice but rather tries to adapt what works
	and what does not overcomplicate matters
*/

#pragma once

#include "synth-oscillator.h"
#include "synth-parameters.h"
#include "synth-ADSR.h"

namespace SFM
{
	// Initialized manually
	class DX_Voice
	{
	public:
		bool m_enabled;

		// Copy
		float m_velocity;

		struct Operator
		{
			bool enabled;
			Oscillator oscillator;

			// Indices: -1 means none, in case of modulator always modulater high up in the chain
			unsigned modulators[3], feedback;

			// Amount of tremolo & vibrato
			float tremolo;
			float vibrato;

			// Op env.
			ADSR opEnv;		

			// Pitch env. influence
			float pitchEnvAmt;	

			// Feedback amount
			float feedbackAmt;

			// Carrier yes/no
			bool isCarrier;

			SFM_INLINE void Reset()
			{
				enabled = false;
				oscillator = Oscillator();
				modulators[0] = modulators[1] = modulators[2] = -1;
				feedback = -1;
				tremolo = 0.f;
				opEnv.Reset();
				pitchEnvAmt = 1.f;
				feedbackAmt = 0.f;
				isCarrier = false;
			}
		} m_operators[kNumOperators];

		// Feedback buffer
		float m_feedback[kNumOperators];

		// LFO
		Oscillator m_tremolo;
		Oscillator m_vibrato;

		// Global pitch bend
		float m_pitchBend;

		// Amplitude env.
		ADSR m_ADSR;

		// Pitch env.
		ADSR m_pitchEnv;

		// Master filter
		LadderFilter *m_pFilter;
	
		DX_Voice() { Reset(); }

		void ResetOperators()
		{
			// Neutral operators
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				m_operators[iOp].Reset();
				m_feedback[iOp] = 0.f;
			}
		}

		// Full (or "hard") reset
		void Reset()
		{
			ResetOperators();

			// No LFO
			m_tremolo.Initialize(kSine, 0.f, 1.f);
			m_vibrato.Initialize(kSine, 0.f, 1.f);

			// No pitch bend
			m_pitchBend = 0.f;

			// Reset envelopes
			m_ADSR.Reset();
			m_pitchEnv.Reset();

			// No filter (must be set!)
			m_pFilter = nullptr;

			// Disable
			m_enabled = false;
		}

		// ** Does not apply ADSR nor filter **
		float Sample(const Parameters &parameters);

		// In octaves
		SFM_INLINE void SetPitchBend(float bend)
		{
			m_pitchBend = bend;
		}
	};
}
