
/*
	Syntherklaas FM: chorus effect that mixes monaural signal to stereo output.
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
		m_sweepMod.Initialize(kDigiTriangle, 0.1f*rate, 1.f, 0.4321f);

		// Simple low pass avoids sampling artifacts
		m_sweepLPF1.SetCutoff(0.001f);
		m_sweepLPF2.SetCutoff(0.001f); 
	}

	void Chorus::Apply(float sample, float delay, float spread, RingBuffer &destBuf)
	{
		m_delayLine.Write(sample);

		// Modulate sweep LFOs
		const float sweepMod = m_sweepMod.Sample(0.f);
		
		// Sample sweep LFOs
		const float sweepL = m_sweepL.Sample( sweepMod);
		const float sweepR = m_sweepR.Sample(-sweepMod);

		// Sweep around one centre point
		SFM_ASSERT(delay > 0.f && spread < delay);
		const size_t lineSize = m_delayLine.size();
		const float delayCtr = lineSize*delay; 
		const float range = lineSize*spread;
	
		// Take sweeped L/R taps (lowpassed to circumvent artifacts)
		const float tapL = m_delayLine.Read(delayCtr + range*m_sweepLPF1.Apply(sweepL));
		const float tapR = m_delayLine.Read(delayCtr + range*m_sweepLPF2.Apply(sweepR));

		// And the current sample
		const float tapM = sample;
		
		// Write mix
		destBuf.Write(tapM + (tapM-tapL));
		destBuf.Write(tapM + (tapM-tapR));
	}
}
