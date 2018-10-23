
/*
	Syntherklaas FM -- Master filter(s)

	Each end filter should support the following functions:

	- Reset()
	- Set()
	- SetCutoff() (normalized [0..1])
	- SetResonance() (normalized [0..1])
	- Apply(pSamples, numSamples, wetness)
*/

#pragma once

#include "synth-global.h"
#include "synth-math.h"
#include "synth-ADSR.h"

namespace SFM
{
	/*
		Filter parameters.
	*/

	struct FilterParameters
	{
		// Normalized [0..1]
		float cutoff;
		float resonance;
	};

	/*
		Microtracker MOOG filter
		
		Credit:
		- Based on an implementation by Magnus Jonsson
		- https://github.com/magnusjonsson/microtracker (unlicense)
	*/

	struct MicrotrackerMoogFilter
	{
		float m_cutoff;
		float m_resonance;

		float m_P0, m_P1, m_P2, m_P3;
		float m_resoCoeffs[3];

		void Reset()
		{
			m_P0 = m_P1 = m_P2 = m_P3 = 0.f;
			m_resoCoeffs[0] = m_resoCoeffs[1] = m_resoCoeffs[2] = 0.f;
		}

		void Set(const FilterParameters &parameters)
		{
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
		}

		void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			value = value*2.f*kPI/kSampleRate;
			m_cutoff = std::min<float>(1.f, value); // Also known as omega
		}

		void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = std::max<float>(kEpsilon, value*4.f);
		}

		void Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
		{
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const float dry = pSamples[iSample];
				const float ADSR = envelope.Sample(sampleCount+iSample);

				const float feedback = m_P3;
				SFM_ASSERT(true == FloatCheck(feedback));

				// Coefficients optimized using differential evolution
				// to make feedback gain 4.0 correspond closely to the
				// border of instability, for all values of omega.
				float out = feedback*0.360891f + m_resoCoeffs[0]*0.417290f + m_resoCoeffs[1]*0.177896f + m_resoCoeffs[2]*0.0439725f;
				out = ultra_tanhf(out);

				// Move window
				m_resoCoeffs[2] = m_resoCoeffs[1];
				m_resoCoeffs[1] = m_resoCoeffs[0];
				m_resoCoeffs[0] = feedback;

				m_P0 += (fast_tanhf(dry - m_resonance*out) - fast_tanhf(m_P0)) * m_cutoff;
				m_P1 += (fast_tanhf(m_P0) - fast_tanhf(m_P1)) * m_cutoff;
				m_P2 += (fast_tanhf(m_P1) - fast_tanhf(m_P2)) * m_cutoff;
				m_P3 += (fast_tanhf(m_P2) - fast_tanhf(m_P3)) * m_cutoff;

				// Linear *might* work since we're blending between 2 separate circuits (from my POV)
				float sample = lerpf<float>(dry, out, globalWetness*(1.f-ADSR*ADSR)); 

				pSamples[iSample] = sample;
			}
		}
	};
}
