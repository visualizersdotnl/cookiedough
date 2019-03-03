
/*
	Syntherklaas FM - Voice (largely inspired by Yamaha DX7).

	Important:
		- An operator can only be modulated by an operator above it (as in index value)
		- Feedback can be taken from any level
*/

#pragma once

#include "3rdparty/SvfLinearTrapOptimised2.hpp"

#include "synth-global.h"
#include "synth-oscillator.h"
#include "synth-parameters.h"
#include "synth-ADSR.h"
#include "synth-pluck-shaper.h"

namespace SFM
{
	// Initialize externally/manually
	class Voice
	{
	public:
		enum State
		{
			kIdle,
			kEnabled,
			kSustaining,
			kReleasing
		} m_state;

		struct Operator
		{
			bool enabled;
			Oscillator oscillator;

			// Envelope
			ADSR envelope;

			// Indices: -1 means none; modulators must be higher up
			unsigned modulators[3], feedback;

			// Feedback
			// See: https://www.reddit.com/r/FMsynthesis/comments/85jfrb/dx7_feedback_implementation/
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
		float m_feedbackBuf[kNumOperators];

		// LFO
		Oscillator m_LFO;

		// Pitch envelope
		ADSR m_pitchEnv;
		bool m_pitchEnvInvert;
		float m_pitchEnvBias;

		// Main filter
		SvfLinearTrapOptimised2 m_LPF;

	private:
		void ResetOperators();

	public:
		bool IsActive() /* const */
		{
			return kIdle != m_state;		
		}

		void Reset();
		void Release(float velocity); // On voice release (stops operator envelopes)
		bool IsDone(); /* const */
		float SummedOutput(); /* const */

		float Sample(const Parameters &parameters);
	};
}
