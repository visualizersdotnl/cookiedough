
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

		// For delay and spread 1.f is one 10th of a sample
		// Good default parameters are 0.3f and 0.025f
		void Apply(float sample, float delay, float spread, RingBuffer &destBuf);

		void SetRate(float rate)
		{
			m_sweepL.SetPitch(rate);
			m_sweepR.SetPitch(rate);
			m_sweepMod.SetPitch(0.1f*rate);
		}

		// FIXME: use set functions for delay and spread too?

	private:
		DelayLine m_delayLine;
		Oscillator m_sweepL, m_sweepR;
		Oscillator m_sweepMod;
		LowpassFilter m_sweepLPF1, m_sweepLPF2;
	};
}
