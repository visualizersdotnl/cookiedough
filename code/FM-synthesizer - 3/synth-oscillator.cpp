
/*
	Syntherklaas FM -- Oscillator (VCO).
*/

#include "synth-global.h" 
#include "synth-oscillator.h" 
// #include "synth-LUT.h"

namespace SFM
{
	float Oscillator::Sample(float modulation, float duty /* = 0.5f */)
	{	
		if (m_phase > m_syncPeriod)
			m_phase -= m_syncPeriod;

		const float phase = m_phase;
		const float modulated = phase+modulation;

		float signal;
		switch (m_form)
		{
			case kDigiTriangle:
				signal = oscDigiTriangle(modulated);
				break;

			case kSine:
				signal = oscSine(modulated);
				break;
					
			case kCosine:
				signal = oscCos(modulated);
				break;
			
			// Lots of unused oscillators :)
			default:
				signal = oscWhiteNoise();
				Log("Oscillator: unsupported waveform");
				SFM_ASSERT(false);
				break;
		}

		signal *= m_amplitude;

		// Can not check for range here
		FloatAssert(signal);

		// Advance
		m_phase += m_pitch;

		return signal;
	}
}