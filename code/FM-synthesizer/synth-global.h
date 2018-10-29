
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	Dependencies on Bevacqua (as far as I know):
		- Std3DMath
		- Macros
*/

#pragma once

// FIXME: only necessary when depending on the Kurt Bevacqua engine as our base
#include "../main.h"

// Alias with Bevacqua's existing mechanisms (FIXME: adapt target platform's)
#define SFM_INLINE __inline
#define SFM_ASSERT VIZ_ASSERT

// Set to 1 to kill all SFM log output
#define SFM_NO_LOGGING 1

#include "synth-log.h"
#include "synth-error.h"
#include "synth-math.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME))
	const unsigned kSampleRate = 44100;

	// Buffer size
	const unsigned kRingBufferSize = 1024;
	const unsigned kMinSamplesPerUpdate = kRingBufferSize/2;

	// Reasonable audible spectrum
	const float kAudibleLowHz = 12.f;
	const float kAudibleHighHz = 22000.f;

	// LFO ranges
	const float kMaxSubLFO = kAudibleLowHz;
	const float kMaxSonicLFO = kAudibleHighHz;	

	// Nyquist frequencies.
	const float kNyquist = kSampleRate/2.f;
	const float kAudibleNyquist = std::min<float>(kAudibleHighHz, kNyquist);

	// Max. number of voices
	const unsigned kMaxVoices = 32; 

	// Max. voice amplitude 
	const float kMaxVoicedB = -3.609121289; // 66% in dB (I've read Impulse Tracker did/does that)

	// Define oscillator period as a discrete amount of steps (i.e. in samples)
	// IMPORTANT: other LUTs and such also use this number
	const unsigned kOscPeriod = 4096;
	const unsigned kOscTabAnd = 4096-1;
	const float kInvOscPeriod = 1.f/kOscPeriod;

	// Use to multiply modulation value (radian) to osc. LUT period
	const float kRadToOscLUT = (1.f/k2PI)*kOscPeriod;

	// Drive range
	const float kDriveHidB = 6.f; // >= 0 means overdrive, but it's filtered so it won't go Iron Maiden-crazy
}

#include "synth-LUT.h"
#include "synth-util.h"
