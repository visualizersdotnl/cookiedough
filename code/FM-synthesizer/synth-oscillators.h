
/*
	Syntherklaas FM -- Oscillators.
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
		kTriangle,
		kWhiteNoise
	};

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }

	/*
		Straight up sawtooth & triangle (will alias in most circumstances).
	*/

	SFM_INLINE float oscDigiSaw(float phase)
	{
		phase *= kInvOscPeriod;
		return -1.f + fmodf(phase, 2.f);
	}

	SFM_INLINE float oscTriangle(float phase)
	{
		return asinf(lutsinf(phase))*kHalfPI;
	}

	/*
		Band-limited saw and square (BLIT).
	*/

	const float kHarmonicsPrecHz = kAudibleLowHz*2.f;

	SFM_INLINE unsigned GetCarrierHarmonics(float frequency)
	{
		SFM_ASSERT(frequency >= 20.f);
		const float lower = (kAudibleNyquist/frequency)/kHarmonicsPrecHz;
		return unsigned(lower);
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

	SFM_INLINE float oscWhiteNoise()
	{
		return -1.f + 2.f*randf();
	}
}
