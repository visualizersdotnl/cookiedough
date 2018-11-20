
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	Dependencies on Bevacqua (as far as I know):
		- Std3DMath
		- Macros
*/

#pragma once

// FIXME: only necessary when depending on the Kurt Bevacqua engine as testbed (FIXME)
#include "../main.h"

// Alias with Bevacqua's existing mechanisms (FIXME: adapt target platform's)
#define SFM_INLINE __inline
#define SFM_ASSERT VIZ_ASSERT

// Set to 1 to kill all SFM log output
#define SFM_NO_LOGGING 0

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
	const float kAudibleLowHz = 20.f;
	const float kAudibleHighHz = 22000.f;

	// LFO ranges
	const float kMaxSubLFO = kAudibleLowHz;
	const float kMaxSonicLFO = kAudibleHighHz;	

	// Nyquist frequency
	const float kNyquist = kSampleRate/2.f;

	// Max. number of voices
	const unsigned kMaxVoices = 24; 

	// Max. voice amplitude 
	const float kMaxVoiceAmp = 0.66f;

	// Master drive range
	const float kDriveHidB = 3.f; // >= 0 means overdrive, but it's filtered so it won't go Iron Maiden-crazy
	
	// Size of global (oscillator) LUTs
	const unsigned kOscLUTSize = 4096;
	const unsigned kOscLUTAnd = kOscLUTSize-1;

	// Pitch bend rane (in semitones)
	const float kPitchBendRange = 12.f;

	// Base note Hz
	const float kBaseHz = 440.f;

	// Number of FM synthesis operators
	const unsigned kNumOperators = 6;

	// Max. note jitter (in cents)
	const unsigned kMaxNoteJitter = 10;
}

#include "synth-LUT.h"
#include "synth-util.h"
