
/*
	Syntherklaas FM -- Stateless oscillator functions.

	These functions all accept any value where [0..1] is the length of one period.
	This costs a few cycles extra here and there which can be avoided if need be; this is however very flexible.

	However the real optimization opportunities here are making a wavetable for all oscillators or going full SIMD.
*/

#pragma once

#include "synth-global.h"
#include "synth-random.h"

namespace SFM
{
	/*
		Available waveforms.
	*/

	enum Waveform
	{
		kSine,
		kCosine,

		/* Straight forms */
		kDigiSaw,
		kDigiSquare,
		kDigiTriangle,
		kDigiPulse,

		/* BLIT forms */
		kSoftSaw,
		kSoftSquare,

		/* PolyBLEP forms */
		kPolyPulse,
		kPolySaw,
		kPolySquare,
		kPolyTriangle,	

		/* Noise */
		kWhiteNoise,
		kPinkNoise
	};

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }
	SFM_INLINE float oscCos(float phase)  { return lutcosf(phase); }

	/*
		Digital sawtooth, square & triangle (alias at audible frequencies).
	*/

	SFM_INLINE float oscDigiSaw(float phase)
	{
		return fmodf(phase, 1.f)*2.f - 1.f;
	}

	SFM_INLINE float oscDigiSquare(float phase)
	{
		return lutsinf(phase) > 0.f ? 1.f : -1.f;
	}

	SFM_INLINE float oscDigiTriangle(float phase)
	{
		return 2.f*(asinf(lutsinf(phase))*(1.f/kPI));
	}

	SFM_INLINE float oscDigiPulse(float phase, float duty)
	{
		const float cycle = fmodf(phase, 1.f);
		return (cycle < duty) ? 1.f : -1.f;
	}

	/*
		Band-limited saw and square (BLIT).

		These variants are very nice but computationally expensive.
	*/

	SFM_INLINE unsigned BLIT_GetNumHarmonics(float frequency)
	{
		SFM_ASSERT(frequency >= kAudibleLowHz);
		const unsigned numHarmonics = unsigned(kNyquist/(frequency-kAudibleLowHz));
		return numHarmonics;
	}

	SFM_INLINE float oscSoftSaw(float phase, float frequency) 
	{
		const unsigned numHarmonics = BLIT_GetNumHarmonics(frequency);

		phase *= -1.f;
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += phase;
		}

		return signal*(2.f/kPI);
	}

	SFM_INLINE float oscSoftSquare(float phase, float frequency) 
	{ 
		const unsigned numHarmonics = BLIT_GetNumHarmonics(frequency);
		
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += 2.f*phase;
		}

		return signal*(4.f/kPI);
	}

	/*
		PolyBLEP pulse, saw, square & triangle.

		Exponentially curves along the discontinuities.
		Not perfect but a whole lot faster than BLIT; they are here until I've implemented MinBLEP.

		They would be a bit faster in a non-stateless form.

		Source: http://www.martin-finke.de/blog/articles/audio-plugins-018-polyblep-oscillator/
	*/

	const float kPolyWidth = 2.f; // This is rather "soft", it could make a fine parameter (FIXME)
	const float kPolyMul = 1.f/(kSampleRate/kPolyWidth);

	SFM_INLINE float BiPolyBLEP(float t, float w)
	{
		// Not near point?
		if (fabsf(t) >= w)
			return 0.0f;

		// Near point: smoothen
		t /= w;
		float tt1 = t*t + 1.f;
		if (t >= 0.f)
			tt1 = -tt1;
		
		return tt1 + t+t;
	}

	SFM_INLINE float oscPolyPulse(float phase, float frequency, float duty)
	{
		// Wrapped positive value
		phase = fabsf(fmodf(phase, 1.f)); 

		const float width = frequency*kPolyMul;
		const float closestUp = float(phase-0.5f >= 0.f);
		const float closestDown = float(phase-0.5f >= duty) - float(phase+0.5f < duty) + duty;
		
		float pulse = oscDigiPulse(phase, duty);
		pulse += BiPolyBLEP(phase - closestUp, width);
		pulse -= BiPolyBLEP(phase - closestDown, width);
		return pulse;
	}

	SFM_INLINE float oscPolySaw(float phase, float frequency)
	{
		// Wrapped positive value
		phase = fabsf(fmodf(phase, 1.f)); 

		const float closestUp = float(phase-0.5f >= 0.f);
	
		float saw = oscDigiSaw(phase);
		saw -= BiPolyBLEP(phase - closestUp, frequency*kPolyMul);
		return saw;
	}

	SFM_INLINE float oscPolySquare(float phase, float frequency)
	{
		return oscPolyPulse(phase, frequency, 0.5f);
	}

	SFM_INLINE float oscPolyTriangle(float phase, float frequency)
	{
		phase = fabsf(fmodf(phase, 1.f));

		const float square = oscPolyPulse(phase, frequency, 0.5f);

		float triangle;
		if (phase < 0.25f)
			triangle = square*phase;
		else if (phase < 0.5f)
			triangle = square*(0.5f-phase);
		else if (phase < 0.75f)
			triangle = square*(phase-0.5f);
		else
			triangle = square*(1.f-phase);

		return triangle*4.f;
	}

	// FIXME: test
	SFM_INLINE float oscPolyTriangleNew(float phase, float frequency)
	{
		phase = fabsf(fmodf(phase, 1.f));

		phase *= 2.f;

		float slope;
		if (phase < 1.f)
			slope = oscPolySaw(phase, frequency);
		else
			slope = oscPolySaw(1.f-(phase-1.f), frequency);

		return slope;
	}

	/*
		Noise oscillator(s).
	*/	

	SFM_INLINE float oscWhiteNoise()
	{
		return -1.f + mt_randf()*2.f;
	}

	// Paul Kellet's approximation to pink noise; basically just a filter resulting in a "softer" spectral distribution
	// Taken from: http://www.firstpr.com.au/dsp/pink-noise/
	SFM_INLINE float oscPinkNoise(float phase)
	{
		const float white = oscWhiteNoise();

		static float b0 = 0.f, b1 = 0.f, b2 = 0.f, b3 = 0.f, b4 = 0.f, b5 = 0.f, b6 = 0.f;
		static float pink = kGoldenRatio/2.f;

		b0 = 0.99886f*b0 + white*0.0555179f;
		b1 = 0.99332f*b1 + white*0.0750759f; 
		b2 = 0.96900f*b2 + white*0.1538520f; 
		b3 = 0.86650f*b3 + white*0.3104856f; 
		b4 = 0.55000f*b4 + white*0.5329522f; 
		b5 = -0.7616f*b5 - white*0.0168980f; 

		pink = b0+b1+b2+b3+b4+b5+b6 + white*0.5362f;

		b6 = white*0.115926f;
		
		return Clamp(pink);
   }
}

