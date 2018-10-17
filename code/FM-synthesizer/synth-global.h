
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	Dependencies on Bevacqua (as far as I know):
		- Audio output (through BASS)
		- Std3DMath
		- Macros
*/

#pragma once

// FIXME: only necessary when depending on the Kurt Bevacqua engine as our base
#include "../main.h"

// Alias with Bevacqua's existing mechanisms (FIXME: adapt target platform's)
#define SFM_INLINE VIZ_INLINE
#define SFM_ASSERT VIZ_ASSERT

#include "synth-log.h"
#include "synth-error.h"
#include "synth-fast-math.h"
#include "synth-LUT.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME)).
	const unsigned kSampleRate = 44100;
	const unsigned kMaxSamplesPerUpdate = kSampleRate/8;

	// Buffer size.
	const unsigned kRingBufferSize = kMaxSamplesPerUpdate;

	// Reasonable audible spectrum.
	const float kAudibleLowHz = 20.f;
	const float kAudibleHighHz = 22000.f;

	// LFO ranges.
	const float kMaxStdLFO = kAudibleLowHz;
	const float kMaxSonicLFO = kAudibleHighHz;	

	// Nyquist frequencies.
	const float kNyquist = kSampleRate/2.f;
	const float kAudibleNyquist = std::min<float>(kAudibleHighHz, kNyquist);

	// Max. number of voices
	const unsigned kMaxVoices = 8;

	// Max. (or initial) voice amplitude
	const float kMaxVoiceAmplitude = 1.f/kMaxVoices;

	// Define oscillator period as a discrete amount of steps (derived from the sinus LUT)
	const unsigned kOscPeriod = kSinTabSize;
	const float kOscQuarterPeriod = kOscPeriod/4;
	const float kInvOscPeriod = 1.f/kOscPeriod;

	// Use to multiply modulation value (radian) to osc. LUT period
	const float kRadToOscLUT = (1.f/k2PI)*kOscPeriod;
}

#include "synth-util.h"
