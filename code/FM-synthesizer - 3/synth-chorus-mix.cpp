
/*
	Syntherklaas FM: chorus effect that mixes monaural signal to stereo output.

	// FIXME:
	// - Use sine and cosine directly, bypass oscillator.
	// - Evaluate algorithm.
*/

#pragma once

#include "synth-global.h"
#include "synth-chorus-mix.h"

namespace SFM
{
	Chorus::Chorus(float rate) :
		m_delayLine(kSampleRate/10)
,		m_sweepPhase(0.f)
	{
		SFM_ASSERT(rate > 0.f);

		// Set up sweep
		SetRate(rate);
		m_sweepMod.Initialize(kDigiTriangle, 0.1f*rate, 1.f); // A bit of linearity sounds better

		// Simple low pass avoids sampling artifacts
		m_sweepLPF1.SetCutoff(0.001f);
		m_sweepLPF2.SetCutoff(0.001f);
	}

	void Chorus::Apply(float sample, float delay, float spread, float wet, RingBuffer &destBuf)
	{
		SFM_ASSERT(delay > 0.f && spread < delay);

		m_delayLine.Write(sample);

		// Sample sweep modulate LFO
		const float sweepMod = m_sweepMod.Sample(0.f);
		
		// Sample sweep LFOs
		if (m_sweepPhase >= 1.f)
			m_sweepPhase -= 1.f;

		const float sweepL = fastsinf(m_sweepPhase+sweepMod);
		const float sweepR = fastsinf(m_sweepPhase+0.33f-sweepMod);

		m_sweepPhase += m_sweepPitch;

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
