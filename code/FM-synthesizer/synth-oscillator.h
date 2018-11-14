
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
		
		Waveform m_form;

		float m_amplitude;

		float m_frequency;
		float m_syncFrequency;
		float m_pitch;

		float m_period;
		float m_syncPeriod;

		unsigned m_cycles;
		float m_phase;

	public:
		// Default oscillator will yield 1.0
		Oscillator()
		{
			Initialize(kCosine, 0.f, 1.f);
		}

		void Initialize(Waveform form, float frequency, float amplitude)
		{
			m_form = form;
			m_amplitude = amplitude;

			m_frequency = frequency;
			m_pitch = CalculatePitch(frequency);

			if (false == oscIsWavetable(form))
			{	
				m_period = 1.f;
			}
			else
			{
				// Wavetables have their own period length (FIXME)
				switch (form)
				{
				case kKick808:
					m_period = getOscKick808().GetLength();
					break;

				case kSnare808:
					m_period = getOscSnare808().GetLength();
					break;

				case kGuitar:
					m_period = getOscGuitar().GetLength();
					break;

				case kElectricPiano:
					m_period = getOscElecPiano().GetLength();
					break;

				case kFemale:
					m_period = getOscFemale().GetLength();
					break;
				
				default:
					m_period = 1.f;
					SFM_ASSERT(false);
					break;
				}				
			}

			// No sync.
			SyncTo(frequency);

			m_phase = 0.f;
			m_cycles = 0;
		}

		// Synchronize to freq. (hard sync.)
		void SyncTo(float frequency)
		{
			m_syncFrequency = frequency;

			float ratio = 1.f;
			if (0.f != m_syncFrequency)
				ratio = m_frequency/m_syncFrequency;

			m_syncPeriod = m_period*ratio;
			FloatAssert(m_syncPeriod);
		}

		// Pitch bend
		void PitchBend(float semitones)
		{
			const float bend = powf(2.f, semitones/12.f);
			m_pitch = CalculatePitch(m_frequency*bend);
		}

		Waveform GetWaveform() const  { return m_form;        }
		float GetFrequency() const    { return m_frequency;   }
		float GetPitch() const        { return m_pitch;       }
		float GetPeriodLength() const { return m_period;      }
		bool HasCycled() const        { return 0 != m_cycles; }

		float Sample(float drift, float modulation, float duty = 0.5f);
	};
}
