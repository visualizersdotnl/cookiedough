
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	Possible dependencies on Bevacqua:
		- Host 
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
	// Pretty standard sample rate
	// For now host codebase relies on it (FIXME)
	const unsigned kSampleRate = 44100;

	// Buffer size
	const unsigned kRingBufferSize = 1024*2; // Stereo
	const unsigned kMinSamplesPerUpdate = kRingBufferSize/2;

	// Reasonable audible spectrum
	const float kAudibleLowHz = 20.f;
	const float kAudibleHighHz = 22000.f;

	// Nyquist frequency
	const float kNyquist = kSampleRate/2.f;

	// Max. number of voices
	const unsigned kMaxVoices = 24;
	
	// Size of global (oscillator) LUTs
	const unsigned kOscLUTSize = 4096;
	const unsigned kOscLUTAnd = kOscLUTSize-1;

	// Base note Hz
	const float kBaseHz = 440.f;

	// Number of FM synthesis operators (can't just change this!)
	const unsigned kNumOperators = 6;

	// Long release range enables bass drums, pads et cetera
	const float kReleaseMul = kGoldenRatio;

	// To divide lin. amplitude by 2 (-3dB = half as loud)
	const float kMinus3dB = 0.707945764f;

	// Pitch bend range (in semitones)
	const float kPitchBendRange = 6.f;

	// Max. note jitter (in cents); tied to 'liveliness'
	const unsigned kMaxNoteJitter = 8;

	// Max. voice amplitude (linear)
	const float kMaxVoiceAmp = 1.f;

	// Max. filter resonance range (must be <= 39.f)
	const float kFilterMaxResonance = 4.f;

	// Working cutoff for rotary MIDI controls (use with LPF)
	const float kControlCutoff = 0.01f;
	const float kControlCutoffS = 0.001f;

	// Chorus rate (1.f is proven to be good)
	const float kChorusRate = 2.f;

	// Leaky integration factor
	const float kLeakyFactor = 0.995f;

	// Default main filter settings
	const float kDefFilterCutoff = 1.f;
	const float kDefFilterResonance = 1.f;

	// Envelope rate multiplier range
	// Range (as in seconds) taken from Arturia DX7-V (http://downloads.arturia.com/products/dx7-v/manual/dx7-v_Manual_1_0_EN.pdf)
	// FIXME: temporarily scaled down to accomodate 7-bit MIDI rotary
	const float kEnvMulMin = 0.5f;  // 0.1f;
	const float kEnvMulRange = 4.f; // 10.f-kEnvMulMin;
}

#include "synth-LUT.h"
#include "synth-util.h"
