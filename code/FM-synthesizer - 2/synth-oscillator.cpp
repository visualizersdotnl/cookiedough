
/*
	Syntherklaas FM -- Oscillator (VCO).
*/

#include "synth-global.h" 
#include "synth-oscillator.h" 

namespace SFM
{
	float Oscillator::Sample(float modulation, float duty /* = 0.5f */)
	{	
		m_phase += m_pitch;

		if (m_phase >= m_syncPeriod)
		{
			m_phase -= m_syncPeriod;
			++m_cycles;
		}

		const float phase = m_phase;
		const float modulated = phase+modulation;

		float signal;
		switch (m_form)
		{
			/* Straight digital */
	
			case kDigiSaw:
				signal = oscDigiSaw(modulated);
				break;

			case kDigiSquare:
				signal = oscDigiSquare(modulated);
				break;

			case kDigiTriangle:
				signal = oscDigiTriangle(modulated);
				break;

			case kDigiPulse:
				signal = oscDigiPulse(modulated, duty);
				break;

			/* BLIT forms (not supported, performance issue) */
			case kSoftSaw:
			case kSoftSquare:
			default:
				signal = oscWhiteNoise();
				Log("Oscillator: unsupported waveform");
				SFM_ASSERT(false);
				break;

			case kSine:
				signal = oscSine(modulated);
				break;
					
			case kCosine:
				signal = oscCos(modulated);
				break;

			/* PolyBLEP forms */
				
			case kPolyPulse:
				signal = oscPolyPulse(modulated, m_frequency, duty);
				break;

			case kPolySaw:
				signal = oscPolySaw(modulated, m_frequency);
				break;

			case kPolySquare:
				signal = oscPolySquare(modulated, m_frequency);
				break;

			case kPolyTriangle:
				signal = oscPolyTriangle(modulated, m_frequency);
				break;

			/* Noise */

			case kWhiteNoise:
				signal = oscWhiteNoise();
				break;

			case kPinkNoise:
				signal = oscPinkNoise(modulated);
				break;
		}

		signal *= m_amplitude;

		// Can not check for range here
		FloatAssert(signal);

		return signal;
	}
}