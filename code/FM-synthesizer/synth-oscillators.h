
/*
	Syntherklaas FM -- Oscillators.

	The idea is to, in due time, precalculate all these and use samplers during actual synthesis as is practically the case already
	for everything that depends on the sinus LUT.
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
		/* BLIT forms */
		kSoftSaw,
		kSoftSquare,
		/* Straight forms */
		kDigiSaw,
		kDigiSquare,
		kTriangle,
		kWhiteNoise
	};

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }

	/*
		Straight up sawtooth, square & triangle (will alias in most circumstances).
	*/

	SFM_INLINE float oscDigiSaw(float phase)
	{
		return fmodf(phase/kOscPeriod, 1.f)*2.f - 1.f;
	}

	SFM_INLINE float oscDigiSquare(float phase)
	{
		return lutsinf(phase) > 0.f ? 1.f : -1.f;
	}

	SFM_INLINE float oscTriangle(float phase)
	{
		return asinf(lutsinf(phase))*(2.f/kPI);
	}

	/*
		Band-limited saw and square (BLIT).
	*/

	const float kHarmonicsPrecHz = kAudibleLowHz;

	SFM_INLINE unsigned GetCarrierHarmonics(float frequency)
	{
		SFM_ASSERT(frequency >= 20.f);
		return 40; // FIXME (documented)
	}

	SFM_INLINE float oscSoftSaw(float phase, unsigned numHarmonics) 
	{ 
		phase *= -1.f;
		float harmonicPhase = phase;

		float signal = 0.f;

		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
		{
			signal += lutsinf(harmonicPhase)/(1.f+iHarmonic);
			harmonicPhase += phase;
		}

 		const float ampMul = 2.f/kPI;
 		signal *= ampMul;

		return signal;
	}

	SFM_INLINE float oscSoftSquare(float phase, unsigned numHarmonics) 
	{ 
		float harmonicPhase = phase;

		float signal = 0.f;

		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
		{
			signal += lutsinf(harmonicPhase)/(1.f+iHarmonic);
			harmonicPhase += phase*2.f;
		}

 		const float ampMul = 4.f/kPI;
		signal *= ampMul;


		return signal;
	}

	/*
		Noise oscillator(s).
	*/

	SFM_INLINE float oscWhiteNoise(float phase)
	{
		return lutnoisef(phase + mt_randu32() /* Without this we'll definitely hear a pattern */);
	}
}
