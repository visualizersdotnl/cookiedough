
/*
	Syntherklaas FM -- Master filter(s)

	Each end filter should implement the following functions:
	- Reset()
	- SetParameters()
	- SetCutoff() (normalized [0..1])
	- SetResonance() (same)
	- SetEnvelopeScale() (same)
	- Apply()

	FIXME: move code out of header file
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
		// [0..N]
		float drive;
	
		// Normalized [0..1]
		float cutoff;
		float resonance;
		float envInfl;
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
		float m_envInfl;

		float m_P0, m_P1, m_P2, m_P3;
		float m_resoCoeffs[3];

		void Reset()
		{
			m_P0 = m_P1 = m_P2 = m_P3 = 0.f;
			m_resoCoeffs[0] = m_resoCoeffs[1] = m_resoCoeffs[2] = 0.f;
		}

		void SetParameters(const FilterParameters &parameters)
		{
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
			SetEnvelopeInfluence(parameters.envInfl);
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

		void SetEnvelopeInfluence(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_envInfl = value;
		}

		void Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
		{
			globalWetness = invsqrf(globalWetness);

			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const float dry = pSamples[iSample];
				const float ADSR = envelope.SampleForFilter(sampleCount);
				SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);

				const float feedback = m_P3;
				SFM_ASSERT(true == FloatCheck(feedback));

				// Coefficients optimized using differential evolution
				// to make feedback gain 4.0 correspond closely to the
				// border of instability, for all values of omega.
				const float out = feedback*0.360891f + m_resoCoeffs[0]*0.417290f + m_resoCoeffs[1]*0.177896f + m_resoCoeffs[2]*0.0439725f;

				// Move window
				m_resoCoeffs[2] = m_resoCoeffs[1];
				m_resoCoeffs[1] = m_resoCoeffs[0];
				m_resoCoeffs[0] = feedback;

				m_P0 += (fast_tanhf(dry - m_resonance*out) - fast_tanhf(m_P0)) * m_cutoff;
				m_P1 += (fast_tanhf(m_P0) - fast_tanhf(m_P1)) * m_cutoff;
				m_P2 += (fast_tanhf(m_P1) - fast_tanhf(m_P2)) * m_cutoff;
				m_P3 += (fast_tanhf(m_P2) - fast_tanhf(m_P3)) * m_cutoff;

				const float wetness = lerpf<float>(globalWetness, ADSR, m_envInfl);
				const float sample = lerpf<float>(dry, out, wetness); 
				SFM_ASSERT(sample >= -1.f && sample <= 1.f);

				pSamples[iSample] = sample;
			}
		}
	};

	/*
		Improved MOOG ladder filter.

		This model is based on a reference implementation of an algorithm developed by
		Stefano D'Angelo and Vesa Valimaki, presented in a paper published at ICASSP in 2013.
		This improved model is based on a circuit analysis and compared against a reference
		Ngspice simulation. In the paper, it is noted that this particular model is
		more accurate in preserving the self-oscillating nature of the real filter.
		References: "An Improved Virtual Analog Model of the Moog Ladder Filter"

		Original Implementation: D'Angelo, Valimaki

		FIXME: use fast_tanhf()
	*/

	// Thermal voltage (26 milliwats at room temperature)
	const float kVT = 0.312f;

	struct ImprovedMoogFilter
	{
		float m_cutoff;
		float m_resonance;
		float m_envInfl;

		float m_V[4];
		float m_dV[4];
		float m_tV[4];

		void Reset()
		{
			for (unsigned iPole = 0; iPole < 4; ++iPole)
				m_V[iPole] = m_dV[iPole] = m_tV[iPole] = 0.f;
		}

		void SetParameters(const FilterParameters &parameters)
		{
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
			SetEnvelopeInfluence(parameters.envInfl);
		}

		void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			const float omega = (kPI*value)/kSampleRate;
			m_cutoff = 4.f*kPI * kVT * value * (1.f-omega) / (1.f+omega);
		}

		void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = std::max<float>(kEpsilon, value*4.f);
		}

		void SetEnvelopeInfluence(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_envInfl = value;
		}

		void Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
		{
			globalWetness = invsqrf(globalWetness);

			float dV0, dV1, dV2, dV3;

			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const float dry = pSamples[iSample];
				const float ADSR = envelope.SampleForFilter(sampleCount);
				SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);

				dV0 = -m_cutoff * (fast_tanhf((1.f*dry + m_resonance*m_V[3]) / (2.f*kVT)) + m_tV[0]);
				m_V[0] += (dV0 + m_dV[0]) / (2.f*kSampleRate);
				m_dV[0] = dV0;
				m_tV[0] = fast_tanhf(m_V[0] / (2.f*kVT));
			
				dV1 = m_cutoff * (m_tV[0] - m_tV[1]);
				m_V[1] += (dV1 + m_dV[1]) / (2.f*kSampleRate);
				m_dV[1] = dV1;
				m_tV[1] = fast_tanhf(m_V[1] / (2.f*kVT));
			
				dV2 = m_cutoff * (m_tV[1] - m_tV[2]);
				m_V[2] += (dV2 + m_dV[2]) / (2.f*kSampleRate);
				m_dV[2] = dV2;
				m_tV[2] = fast_tanhf(m_V[2] / (2.f*kVT));
			
				dV3 = m_cutoff * (m_tV[2] - m_tV[3]);
				m_V[3] += (dV3 + m_dV[3]) / (2.f*kSampleRate);
				m_dV[3] = dV3;
				m_tV[3] = fast_tanhf(m_V[3] / (2.f*kVT));

				const float wetness = lerpf<float>(globalWetness, ADSR, m_envInfl);
				const float sample = lerpf<float>(dry, m_V[3], wetness); 
				SFM_ASSERT(sample >= -1.f && sample <= 1.f);

				pSamples[iSample] = sample;
			}
		}
	};
}
