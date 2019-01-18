
/*
	Syntherklaas FM - Voice (largely inspired by Yamaha DX7).

	Important:
		- An operator can only be modulated by an operator above it (as in index value)
		- Feedback can be taken from any level
*/

#pragma once

#include "synth-oscillator.h"
#include "synth-parameters.h"
#include "synth-ADSR.h"
#include "synth-pluck.h"

namespace SFM
{
	// Initialized manually
	class Voice
	{
	public:
		enum State
		{
			kIdle,
			kEnabled,
			kReleasing
		} m_state;

		enum Mode
		{
			// Fully flexible pure FM
			kFM,

			// First operator is treated as the *only* carrier and acts as an accumulator at 0Hz, and is
			// then distorted to imitate a magnetic pickup
			kPickup
		} m_mode;

		struct Operator
		{
			bool enabled;
			Oscillator oscillator;

			// Envelope
			ADSR envelope;

			// Indices: -1 means none; modulators must be higher up
			unsigned modulators[3], feedback;

			// Feedback amount
			float feedbackAmt;

			// LFO influence
			float ampMod;
			float pitchMod;

			// Distortion
			float distortion;

			// Is carrier (output)
			bool isCarrier;

			void Reset()
			{
				enabled = false;
				oscillator = Oscillator();
				envelope.Reset();
				modulators[0] = modulators[1] = modulators[2] = -1;
				feedback = -1;
				feedbackAmt = 0.f;
				ampMod = 0.f;
				pitchMod = 0.f;
				distortion = 0.f;
				isCarrier = false;
			}
		} m_operators[kNumOperators];

		// Feedback buffer
		float m_feedback[kNumOperators];

		// LFO
		Oscillator m_LFO;

		Voice()
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

		// Full reset (almost never necessary)
		void Reset()
		{
			ResetOperators();

			// Disable
			m_state = kIdle;

			// Default mode
			m_mode = kFM;
		}

		// On voice release (stops operator envelopes)
		void Release(float velocity)
		{
			for (auto &voiceOp : m_operators)
			{
				if (true == voiceOp.enabled)
					voiceOp.envelope.Stop(velocity);
			}

			m_state = kReleasing;
		}
		
		// Apply pitch bend (to carriers)
		void SetPitchBend(float bend)
		{
			for (auto &voiceOp : m_operators)
			{
				if (true == voiceOp.enabled)
					voiceOp.oscillator.PitchBend(bend);
			}
		}

		bool IsActive() /* const */
		{
			return kIdle != m_state;		
		}

		bool IsDone() /* const */
		{
			// Only true if all carrier envelopes are idle
			for (auto &voiceOp : m_operators)
			{
				if (true == voiceOp.enabled && true == voiceOp.isCarrier)
				{
					if (false == voiceOp.envelope.IsIdle())
						return false;
				}
			}

			return true;
		}

		float SummedOutput() /* const */
		{
			float summed = 0.f;
			for (auto &voiceOp : m_operators)
			{
				if (true == voiceOp.enabled && true == voiceOp.isCarrier)
				{
					summed += voiceOp.envelope.Get();
				}
			}

			return summed;
		}

		float Sample(const Parameters &parameters);
	};
}
