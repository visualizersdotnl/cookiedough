
/*
	Syntherklaas FM -- Oscillator (VCO).
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
		bool m_hasCycled;

	public:
		// Default oscillator will yield 1.0
		Oscillator() :
			m_sampleOffs(-1), 
			m_form(kCosine), 
			m_frequency(1.f), 
			m_amplitude(1.f), 
			m_pitch(0.f), 
			m_masterPitch(0.f), 
			m_periodLen(1.f), 
			m_hasCycled(false) {}

		void Initialize(unsigned sampleCount, Waveform form, float frequency, float amplitude)
		{
			m_sampleOffs = sampleCount;
			m_form = form;
			m_frequency = frequency;
			m_pitch = CalculatePitch(frequency);
			m_amplitude = amplitude;
			m_hasCycled = false;
			
			// No sync.
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
		bool HasCycled() const        { return m_hasCycled; }

		float Sample(unsigned sampleCount, float modulation, float duty = 0.5f);
	};
}
