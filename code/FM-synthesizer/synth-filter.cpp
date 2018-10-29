
/*
	Syntherklaas FM -- Master filter(s)
*/

#include "synth-global.h"
#include "synth-filter.h"

namespace SFM
{
	void MicrotrackerMoogFilter::Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
	{
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = envelope.SampleForFilter(sampleCount);
			SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);

			const float feedback = m_P3;
			SampleAssert(feedback);

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
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}

	void ImprovedMoogFilter::Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
	{
		double dV0, dV1, dV2, dV3;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = envelope.SampleForFilter(sampleCount);
			SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);

			dV0 = -m_cutoff * (tanh((1.0*dry + m_resonance*m_V[3]) / (2.0*kVT)) + m_tV[0]);
			m_V[0] += (dV0 + m_dV[0]) / (2.0*kSampleRate);
			m_dV[0] = dV0;
			m_tV[0] = tanh(m_V[0] / (2.0*kVT));
			
			dV1 = m_cutoff * (m_tV[0] - m_tV[1]);
			m_V[1] += (dV1 + m_dV[1]) / (2.0*kSampleRate);
			m_dV[1] = dV1;
			m_tV[1] = tanh(m_V[1] / (2.0*kVT));
			
			dV2 = m_cutoff * (m_tV[1] - m_tV[2]);
			m_V[2] += (dV2 + m_dV[2]) / (2.0*kSampleRate);
			m_dV[2] = dV2;
			m_tV[2] = tanh(m_V[2] / (2.0*kVT));
			
			dV3 = m_cutoff * (m_tV[2] - m_tV[3]);
			m_V[3] += (dV3 + m_dV[3]) / (2.0*kSampleRate);
			m_dV[3] = dV3;
			m_tV[3] = tanh(m_V[3] / (2.0*kVT));

			const float wetness = lerpf<float>(globalWetness, ADSR, m_envInfl);
			const float sample = lerpf<float>(dry, float(m_V[3]), wetness); 
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}

	/*
		Teemu's filter.

		I've left the core of Teemu's code more or less intact, including variable names.
	*/

	// A tanh(x)/x approximation, flatline at very high inputs; might not be safe for very large feedback gains
	// Limit is 1/15 so very large means ~15 or +23dB
	SFM_INLINE double tanhXdX(double x)
	{
		const double a = x*x;
		return ((a + 105.0)*a + 945.0) / ((15.0*a + 420.0)*a + 945.0);
	}

	void TeemuFilter::Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope)
	{
		const double f = m_cutoff;
		const double r = (40.0/9.0) * m_resonance;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = envelope.SampleForFilter(sampleCount);
			SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);

			// Input with half delay, for non-linearities
			double ih = 0.5 * (dry + m_inputDelay); 
			m_inputDelay = dry;

			// Evaluate the non-linear gains
			double t0 = tanhXdX(ih - r * m_state[3]);
			double t1 = tanhXdX(m_state[0]);
			double t2 = tanhXdX(m_state[1]);
			double t3 = tanhXdX(m_state[2]);
			double t4 = tanhXdX(m_state[3]);

			// G# the denominators for solutions of individual stages
			double g0 = 1 / (1 + m_cutoff*t1), g1 = 1 / (1 + f*t2);
			double g2 = 1 / (1 + m_cutoff*t3), g3 = 1 / (1 + f*t4);
        
			// F# are just factored out of the feedback solution
			double f3 = f*t3*g3, f2 = f*t2*g2*f3, f1 = f*t1*g1*f2, f0 = f*t0*g0*f1;

			// Solve feedback 
			double y3 = (g3*m_state[3] + f3*g2*m_state[2] + f2*g1*m_state[1] + f1*g0*m_state[0] + f0*dry) / (1 + r*f0);

			// Then solve the remaining outputs (with the non-linear gains here)
			double xx = t0*(dry - r*y3);
			double y0 = t1*g0*(m_state[0] + f*xx);
			double y1 = t2*g1*(m_state[1] + f*y0);
			double y2 = t3*g2*(m_state[2] + f*y1);

			// Update state
			m_state[0] += 2*f * (xx - y0);
			m_state[1] += 2*f * (y0 - y1);
			m_state[2] += 2*f * (y1 - y2);
			m_state[3] += 2*f * (y2 - t4*y3);

			const float wetness = lerpf<float>(globalWetness, ADSR, m_envInfl);
			const float sample = lerpf<float>(dry, float(y3), wetness); 
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}
}
