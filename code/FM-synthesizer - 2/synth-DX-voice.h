
/*
	Syntherklaas FM - Yamaha DX style voice.

	Important:
		- An operator can only be modulated by an operator above it (index)
		- Feedback can be taken from any level
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
				feedbackAmt = 0.f;
				isCarrier = false;
			}
		} m_operators[kNumOperators];

		// Feedback buffer
		float m_feedback[kNumOperators];

		// Tremolo osc.
		Oscillator m_tremolo;

		// Vibrato osc.
		Oscillator m_vibrato;

		// Global pitch bend
		float m_pitchBend;

		// Amplitude env.
		ADSR m_ADSR;

		// Master filter
		LadderFilter *m_pFilter;
	
		DX_Voice() : m_enabled(false) {}

		void Reset()
		{
			// Neutral operators
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				m_operators[iOp].Reset();
				m_feedback[iOp] = 0.f;
			}

			// No tremolo
			m_tremolo.Initialize(kCosine, 0.f, 1.f);

			// No vibrato
			m_vibrato.Initialize(kCosine, 0.f, 1.f);

			// No pitch bend
			m_pitchBend = 0.f;

			// Reset envelopes
			m_ADSR.Reset();

			// No filter (must be set!)
			m_pFilter = nullptr;
		}

		// ** Does not apply ADSR nor filter **
		float Sample(const Parameters &parameters);

		SFM_INLINE void SetPitchBend(float bend)
		{
			m_pitchBend = bend;
		}
	};
}
