
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

	/*
		Filter (exact source unknown) (http://www.musicdsp.org)
	*/

	void UnknownFilter:: Apply(float *pSamples, unsigned numSamples, float contour, bool invert)
	{
		const float cutoff = 2.f * m_cutoff/kSampleRate;
		const float resonance = powf(10.f, 0.05f * -m_resonance);
		const float K = 0.5f*resonance*sinf(kPI*cutoff); // lutsinf(0.5f*cutoff)
		const float C1 = 0.5f * (1.f-K) / (1.f+K);
		const float C2 = (0.5f + C1) * cosf(kPI*cutoff); // lutcosf(0.5f*cutoff)
		const float C3 = (0.5f + C1-C2) * 0.25f;
		const float mA0 = 2.f*C3;
		const float mA1 = 2.f*2.f*C3;
		const float mA2 = 2.f*C3;
		const float mB1 = 2.f*-C2;
		const float mB2 = 2.f*C1;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = SampleEnv(m_ADSR, invert);

			float driven = dry*m_drive;

			float sample = mA0*driven + mA1*m_mX1 + mA2*m_mX2 - mB1*m_mY1 - mB2*m_mY2;

			m_mX2 = m_mX1;
			m_mX1 = driven;
			m_mY2 = m_mY1;
			m_mY1 = sample;
				
			sample = fast_tanhf(sample);

			sample = Blend(dry, sample, contour, ADSR);
			SampleAssert(sample);

			pSamples[iSample] = sample;
		}
	}

	/*
		Improved MOOG filter.
	*/

	void ImprovedMOOGFilter::Apply(float *pSamples, unsigned numSamples, float contour, bool invert)
	{
		double dV0, dV1, dV2, dV3;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			const float dry = pSamples[iSample];
			const float ADSR = SampleEnv(m_ADSR, invert);

			dV0 = -m_cutoff * (fast_tanh((dry*m_drive + m_resonance*m_V[3]) / (2.0*kVT)) + m_tV[0]); // kVT = thermal voltage in milliwatts
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

	void TeemuFilter::Apply(float *pSamples, unsigned numSamples, float contour, bool invert)
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
