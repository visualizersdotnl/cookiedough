
/*
	Syntherklaas FM -- Master filter(s)
*/

#include "synth-global.h"
#include "synth-filter.h"

namespace SFM
{
	SFM_INLINE float Blend(float dry, float wet, float contour, float ADSR)
	{
		const float sample = lerpf<float>(dry, lerpf<float>(dry, wet, contour) /* Try non-linear? */, ADSR);
		return fast_tanhf(sample);
	}

	SFM_INLINE float SampleEnv(ADSR &envelope, bool invert)
	{
		const float ADSR = envelope.Sample();
		SFM_ASSERT(ADSR >= 0.f && ADSR <= 1.f);
		return (true == invert) ? 1.f-ADSR : ADSR;
	}

	void CleanFilter::Apply(float *pSamples, unsigned numSamples, float contour, bool invert, unsigned sampleCount)
	{
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = SampleEnv(m_ADSR, invert);

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

			m_P0 += (dry*m_drive - m_resonance*out - fast_tanhf(m_P0)) * m_cutoff;
			m_P1 += (m_P0-m_P1)*m_cutoff;
			m_P2 += (m_P1-m_P2)*m_cutoff;
			m_P3 += (m_P2-m_P3)*m_cutoff;
			m_P3 = ultra_tanhf(m_P3);

			const float sample = Blend(dry, m_P3, contour, ADSR);
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}

	void ImprovedMOOGFilter::Apply(float *pSamples, unsigned numSamples, float contour, bool invert, unsigned sampleCount)
	{
		double dV0, dV1, dV2, dV3;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = SampleEnv(m_ADSR, invert);

			const double drive = 1.3;
			dV0 = -m_cutoff * (fast_tanh((dry*m_drive + m_resonance*m_V[3]) / (2.0*kVT)) + m_tV[0]); // kVT = thermal voltage in milliwats
			m_V[0] += (dV0 + m_dV[0]) / (2.0*kSampleRate);
			m_dV[0] = dV0;
			m_tV[0] = fast_tanh(m_V[0] / (2.0*kVT));
			
			dV1 = m_cutoff * (m_tV[0] - m_tV[1]);
			m_V[1] += (dV1 + m_dV[1]) / (2.0*kSampleRate);
			m_dV[1] = dV1;
			m_tV[1] = fast_tanh(m_V[1] / (2.0*kVT));
			
			dV2 = m_cutoff * (m_tV[1] - m_tV[2]);
			m_V[2] += (dV2 + m_dV[2]) / (2.0*kSampleRate);
			m_dV[2] = dV2;
			m_tV[2] = fast_tanh(m_V[2] / (2.0*kVT));
			
			dV3 = m_cutoff * (m_tV[2] - m_tV[3]);
			m_V[3] += (dV3 + m_dV[3]) / (2.0*kSampleRate);
			m_dV[3] = dV3;
			m_tV[3] = fast_tanh(m_V[3] / (2.0*kVT));

			const float sample = Blend(dry, float(m_V[3]), contour, ADSR);
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}

	/*
		Transistor ladder filter by Teemu Voipio
		Source: https://www.kvraudio.com/forum/viewtopic.php?t=349859

		I've left most of Teemu's code more or less intact, including variable names and comments.
	*/

	// A tanh(x)/x approximation, flatline at very high inputs; might not be safe for very large feedback gains
	// Limit is 1/15 so very large means ~15 or +23dB
	SFM_INLINE double tanhXdX(double x)
	{
		const double a = x*x;
		return ((a + 105.0)*a + 945.0) / ((15.0*a + 420.0)*a + 945.0);
	}

	void TeemuFilter::Apply(float *pSamples, unsigned numSamples, float contour, bool invert, unsigned sampleCount)
	{
		const double f = m_cutoff;
		const double r = (40.0/9.0) * m_resonance;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			/* const */ float dry = pSamples[iSample];
			const float ADSR = SampleEnv(m_ADSR, invert);

			// Dry*Drive
			const float driven = dry*m_drive;

			// Input with half delay, for non-linearities
			double ih = 0.5 * (driven + m_inputDelay); 
			m_inputDelay = driven;

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
			double xx = t0*(driven - r*y3);
			double y0 = t1*g0*(m_state[0] + f*xx);
			double y1 = t2*g1*(m_state[1] + f*y0);
			double y2 = t3*g2*(m_state[2] + f*y1);

			// Update state
			m_state[0] += 2*f * (xx - y0);
			m_state[1] += 2*f * (y0 - y1);
			m_state[2] += 2*f * (y1 - y2);
			m_state[3] += 2*f * (y2 - t4*y3);

			const float sample = Blend(dry, float(y3), contour, ADSR);
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}
}
