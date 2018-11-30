
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
		/* const */ Waveform m_form;
		/* const */ float m_frequency;
		/* const */ float m_amplitude;
		
		float m_syncFrequency;
		float m_syncPeriod;

		float m_pitch;
		float m_phase;

		unsigned m_cycles;

	public:
		// Default oscillator will yield 1.0
		Oscillator()
		{
			Initialize(kCosine, 0.f, 1.f);
		}

		void Initialize(Waveform form, float frequency, float amplitude, float phaseShift = 0.f)
		{
			m_form = form;
			m_amplitude = amplitude;

			m_frequency = frequency;
			m_pitch = CalculatePitch(frequency);

			// No sync.
			SyncTo(frequency);

			m_cycles = 0;
			m_phase = phaseShift;
		}

		// Synchronize to freq. (hard sync.)
		void SyncTo(float frequency)
		{
			m_syncFrequency = frequency;

			float ratio = 1.f;
			if (0.f != m_syncFrequency)
				ratio = m_frequency/m_syncFrequency;

			m_syncPeriod = ratio;
			FloatAssert(m_syncPeriod);
		}

		// Pitch bend
		void PitchBend(float semitones)
		{
			const float bend = powf(2.f, semitones/12.f);
			m_pitch = CalculatePitch(m_frequency*bend);
		}

		Waveform GetWaveform() const  { return m_form;      }
		float GetFrequency() const    { return m_frequency; }
		float GetPitch() const        { return m_pitch;     }
		float GetPhase() const        { return m_phase;     }
		float GetPeriodLength() const { return 1.f;         }
		unsigned GetCycle() const     { return m_cycles;    }

		float Sample(float modulation, float duty = 0.5f);
	};
}
