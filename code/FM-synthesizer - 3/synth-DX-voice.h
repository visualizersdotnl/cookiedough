
/*
	Syntherklaas FM - Yamaha DX-style voice.

	Important:
		- An operator can only be modulated by an operator above it (as in index value)
		- Feedback can be taken from any level
*/

#pragma once

#include "synth-oscillator.h"
#include "synth-parameters.h"

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

			// Indices: -1 means none; modulators must be higher up
			unsigned modulators[3], feedback;

			// Feedback amount
			float feedbackAmt;

			// Is carrier (output)
			bool isCarrier;

			void Reset()
			{
				enabled = false;
				oscillator = Oscillator();
				modulators[0] = modulators[1] = modulators[2] = -1;
				feedback = -1;
				feedbackAmt = 0.f;
				isCarrier = false;
			}
		} m_operators[kNumOperators];

		// Feedback buffer
		float m_feedback[kNumOperators];
	
		DX_Voice() 
		{ 
			Reset();	
		}

		void ResetOperators()
		{
			// NULL operators
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				m_operators[iOp].Reset();
				m_feedback[iOp] = 0.f;
			}
		}

		// Full reset
		void Reset()
		{
			ResetOperators();

			// Disable
			m_enabled = false;
		}

		float Sample(const Parameters &parameters);
	};
}
