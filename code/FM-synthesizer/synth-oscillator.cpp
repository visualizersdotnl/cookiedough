
/*
	Syntherklaas FM -- Oscillator (VCO).
*/

#include "synth-global.h" 
#include "synth-oscillator.h" 

namespace SFM
{
	float Oscillator::Sample(unsigned sampleCount, float drift, float modulation, float duty /* = 0.5f */)
	{	
		const unsigned sample = sampleCount-m_sampleOffs;
		float phase = sample*m_pitch;
		const float masterPhase = sample*m_masterPitch;
			
		// Wrap?
		if (masterPhase >= m_periodLen)
		{
			phase = 0.f;
			m_sampleOffs = sampleCount;
			m_hasCycled = true;
		}

		const float modulated = phase+modulation;

		float signal;
		switch (m_form)
		{
			/* Straight forms (not supported, see LFO) */
			case kDigiSaw:
			case kDigiSquare:
			case kDigiTriangle:
			case kDigiPulse:

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
				signal = oscPinkNoise();
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
		}

		signal *= m_amplitude;
		SampleAssert(signal);

		return signal;
	}
}