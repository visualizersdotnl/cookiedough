
/*
	Syntherklaas FM -- Master filter(s)
*/

#include "synth-global.h"
#include "synth-filter.h"

namespace SFM
{
		void MicrotrackerMoogFilter::Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
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

		void ImprovedMoogFilter::Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
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
}