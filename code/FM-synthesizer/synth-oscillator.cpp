
/*
	Syntherklaas FM -- Oscillator (VCO).
*/

#include "synth-global.h" 
#include "synth-oscillator.h" 

namespace SFM
{
	float Oscillator::Sample(float modulation, float duty /* = 0.5f */)
	{	
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
				return oscDigiSaw(modulated);
				break;

			case kDigiSquare:
				return oscDigiSquare(modulated);
				break;

			case kDigiTriangle:
				return oscDigiTriangle(modulated);
				break;

			case kDigiPulse:
				return oscDigiPulse(modulated, duty);
				break;

			/* BLIT forms (not supported, performance issue) */
			case kSoftSaw:
			case kSoftSquare:
			default:
				signal = oscWhiteNoise(phase);
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
				signal = oscWhiteNoise(modulated);
				break;

			case kPinkNoise:
				signal = oscPinkNoise(modulated);
				break;

			/* Wavetable (not modulated!) */
				
			case kKick808:
				signal = getOscKick808().Sample(phase);
				break;

			case kSnare808:
				signal = getOscSnare808().Sample(phase);
				break;

			case kGuitar:
				signal = getOscGuitar().Sample(phase);
				break;

			case kElectricPiano:				
				signal = getOscElecPiano().Sample(phase);
				break;

			case kFemale:
				signal = getOscFemale().Sample(phase);
				break;
		}

		signal *= m_amplitude;
		SampleAssert(signal);

		// Advance
		m_phase += m_pitch;

		return signal;
	}
}