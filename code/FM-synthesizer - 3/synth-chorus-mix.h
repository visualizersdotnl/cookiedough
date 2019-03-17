
/*
	Syntherklaas FM: chorus effect that mixes monaural signal to stereo output.

	This is a ham-fisted chorus effect that mixes 2 sweeping taps with the input signal.
	However it does sound pretty good.
*/

#pragma once

#include "synth-delay-line.h"
#include "synth-oscillator.h"
#include "synth-one-pole-filters.h"
#include "synth-ring-buffer.h"

namespace SFM
{
	class Chorus
	{
	public:
		Chorus(float rate);

		void Apply(float sample, float delay, float spread, float wet, RingBuffer &destBuf);

		void SetRate(float rate)
		{
			m_sweepPitch = CalculatePitch(rate);
			m_sweepMod.SetPitch(0.1f*rate);
		}

		// FIXME: use set functions for other parameters too?

	private:
		DelayLine m_delayLine;
		float m_sweepPhase, m_sweepPitch;
		Oscillator m_sweepMod;
		LowpassFilter m_sweepLPF1, m_sweepLPF2;
	};
}
