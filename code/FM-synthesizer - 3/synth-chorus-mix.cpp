
/*
	Syntherklaas FM: chorus effect that mixes monaural signal to stereo output.

	// FIXME:
	// - Use sine and cosine directly, bypass oscillator.
	// - Evaluate mix.
	// - Evaluate algorithm.
*/

#pragma once

#include "synth-global.h"
#include "synth-chorus-mix.h"

namespace SFM
{
	Chorus::Chorus(float rate) :
		m_delayLine(kSampleRate/10)
	{
		SFM_ASSERT(rate > 0.f);

		// Set up sweep according to rate and some well tested values
		m_sweepL.Initialize(kSine, rate, 0.5f, 0.f);
		m_sweepR.Initialize(kSine, rate, 0.5f, 0.33f);
		m_sweepMod.Initialize(kSine, 0.1f*rate, 1.f);

		// Simple low pass avoids sampling artifacts
		m_sweepLPF1.SetCutoff(0.001f);
		m_sweepLPF2.SetCutoff(0.001f); 
	}

	void Chorus::Apply(float sample, float delay, float spread, float wet, RingBuffer &destBuf)
	{
		SFM_ASSERT(delay > 0.f && spread < delay);

		m_delayLine.Write(sample);

		// Modulate sweep LFOs
		const float sweepMod = m_sweepMod.Sample(0.f);
		
		// Sample sweep LFOs
		const float sweepL = m_sweepL.Sample( sweepMod);
		const float sweepR = m_sweepR.Sample(-sweepMod);

		// 2 sweeping taps
		auto size = m_delayLine.size();
		delay = size*delay;
		spread = size*spread;

	
		// Take sweeped L/R taps (lowpassed to circumvent artifacts)
		const float tapL = m_delayLine.Read(delay + spread*m_sweepLPF1.Apply(sweepL));
		const float tapR = m_delayLine.Read(delay + spread*m_sweepLPF2.Apply(sweepR));

		// And the current sample
		const float tapM = sample;
		
		// Write mix
		destBuf.Write(lerpf<float>(tapM, tapL, wet));
		destBuf.Write(lerpf<float>(tapM, tapR, wet));
	}
}
