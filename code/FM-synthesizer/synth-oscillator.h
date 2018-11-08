
/*
	Syntherklaas FM -- Carrier oscillator (FIXME: work in progress)
*/

#pragma once

#include "synth-stateless-oscillators.h"

namespace SFM
{
	class Oscillator
	{
	private:
		unsigned m_sampleOffs;
		/* const */ Waveform m_form;
		/* const */ float m_frequency;
		/* const */ float m_amplitude;
		/* const */ float m_pitch;
		float m_masterPitch;
		/* const */ float m_periodLen;

	public:
		// Unspecified oscillator will yield 1.0
		Oscillator() :
			m_sampleOffs(-1), m_form(kCosine), m_frequency(1.f), m_amplitude(1.f), m_pitch(0.f), m_masterPitch(0.f), m_periodLen(1.f) {}

		Oscillator(unsigned sampleCount, Waveform form, float frequency, float amplitude) : 
			m_sampleOffs(sampleCount)
,			m_form(form)
,			m_frequency(frequency)
,			m_pitch(CalculatePitch(frequency))
,			m_amplitude(amplitude)
		{
			m_masterPitch = m_pitch;

			if (false == oscIsWavetable(form))
			{
				m_periodLen = 1.f;
			}
			else
			{
				// Wavetables have their own period length
				switch (form)
				{
				case kKick808:
					m_periodLen = getOscKick808().GetLength();
					break;

				case kSnare808:
					m_periodLen = getOscSnare808().GetLength();
					break;

				case kGuitar:
					m_periodLen = getOscGuitar().GetLength();
					break;

				case kElectricPiano:
					m_periodLen = getOscElecPiano().GetLength();
					break;
				
				default:
					m_periodLen = 1.f;
					SFM_ASSERT(false);
					break;
				}				
			}
		}

		// We'll wrap around this pitch; so by default it's identical
		// It enables to hard sync. this oscillator (slave mode)
		void SetMasterFrequency(float frequency)
		{
			m_masterPitch = CalculatePitch(frequency);
		}

		Waveform GetWaveform() const  { return m_form;      }
		float GetPitch() const        { return m_pitch;     }
		float GetPeriodLength() const { return m_periodLen; }

		// Parameter 'crush' must be a power of 2, performs exponential sample & hold
		float Sample(unsigned sampleCount, float modulation, float duty = 0.5f)
		{
			const unsigned sample = sampleCount-m_sampleOffs;
			float phase = sample*m_pitch;
			const float masterPhase = sample*m_masterPitch;
			
			// Wrap?
			if (masterPhase >= m_periodLen)
			{
				phase = 0.f;
				m_sampleOffs = sampleCount;
			}

			const float modulated = phase+modulation;

			float signal;
			switch (m_form)
			{
				/* Straight forms (not supported) */
				case kDigiSaw:
				case kDigiSquare:
				case kDigiTriangle:
				case kDigiPulse:

				/* BLIT forms (not supported) */
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

		SFM_INLINE bool HasCycled(unsigned sampleCount) /* const */
		{
			return (sampleCount-m_sampleOffs)*m_pitch >= m_periodLen;
		}
	};
}
