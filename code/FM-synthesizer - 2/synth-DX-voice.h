
/*
	Syntherklaas FM - Yamaha DX style voice.

	Important:
		- An operator can only be modulated by an operator above it (index)
		- Feedback can be taken from any level
*/

#pragma once

#include "synth-oscillator.h"
#include "synth-parameters.h"

namespace SFM
{
	// FIXME: move to synth-global.h?
	const unsigned kNumOperators = 6;

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
			unsigned modulator, feedback;
			bool isCarrier;

			SFM_INLINE void Reset()
			{
				enabled = false;
				oscillator = Oscillator();
				modulator = -1;
				feedback = -1;
				isCarrier = false;
			}

		} m_operators[kNumOperators];

		// Feedback buffer
		float m_feedback[kNumOperators];
	
		DX_Voice() : m_enabled(false) {}

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				m_operators[iOp].Reset();
				m_feedback[iOp] = 0.f;
			}
		}

		float Sample(const Parameters &parameters);

		SFM_INLINE void SetPitchBend(float bend)
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
				if (true == m_operators[iOp].enabled) 
					m_operators[iOp].oscillator.PitchBend(bend);
		}
	};
}
