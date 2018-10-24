
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
#define SFM_INLINE VIZ_INLINE
#define SFM_ASSERT VIZ_ASSERT

#include "synth-log.h"
#include "synth-error.h"
#include "synth-math.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME))
	const unsigned kSampleRate = 44100;
	const unsigned kMaxSamplesPerUpdate = 1024;

	// Buffer size
	const unsigned kRingBufferSize = kMaxSamplesPerUpdate;

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
	const unsigned kMaxVoices = 64; 

	// Max. voice amplitude 
	const float kMaxVoicedB = -3.609121289; // 66% in dB (I've read Impulse Tracker did/does that)

	// Define oscillator period as a discrete amount of steps (i.e. in samples)
	// IMPORTANT: other LUTs and such also use this number
	const unsigned kOscPeriod = 4096;
	const unsigned kOscTabAnd = 4096-1;
	const float kInvOscPeriod = 1.f/kOscPeriod;

	// Use to multiply modulation value (radian) to osc. LUT period
	const float kRadToOscLUT = (1.f/k2PI)*kOscPeriod;

	// Maximum master (over)drive
	const float kMaxOverdrive = kGoldenRatio;
}

#include "synth-LUT.h"
#include "synth-util.h"
