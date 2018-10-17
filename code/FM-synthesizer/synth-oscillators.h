
/*
	Syntherklaas FM -- Oscillators.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	/*
		Available waveforms.
	*/

	enum Waveform
	{
		kSine,
		/* Neutered forms */
		kSaw,
		kSquare,
		/* Aliasing forms */
		kDirtySaw,
		kDirtyTriangle,
		/* Noise */
		kPinkNoise
	};

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }

	/*
		Straight up sawtooth & triangle (aliases and thus noisy, but sometimes that's great).
		FIXME: these are dirty slow.
	*/

	SFM_INLINE float oscDirtySaw(float phase)
	{
		return -1.f + fmodf(phase*kInvOscPeriod, 2.f);
	}


	SFM_INLINE float oscDirtyTriangle(float phase)
	{
		return -1.f + 4.f*fabsf(fmodf(phase*kInvOscPeriod, 1.f) - 0.5f);
	}

	/*
		Band-limited saw and square.
	*/

	const float kHarmonicsPrecHz = kAudibleLowHz*2.f;

	SFM_INLINE unsigned GetCarrierHarmonics(float frequency)
	{
		SFM_ASSERT(frequency >= 20.f);
		const float lower = (kAudibleNyquist/frequency)/kHarmonicsPrecHz;
		return unsigned(lower);
	}

	SFM_INLINE float oscSaw(float phase, unsigned numHarmonics) 
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

	SFM_INLINE float oscSquare(float phase, unsigned numHarmonics) 
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

	SFM_INLINE float oscPinkNoise(float phase)
	{
		return 0.f;
	}
}
