
/*
	See: http://ccrma.stanford.edu/software/snd/snd/fm.html
	Important: Rocket not available (for now) in this mode, see main.cpp!

	This is intended to be a powerful yet relatively simple FM synthesizer core.
	Much of it's design is based on a discussion with Ronny Pries.

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	Priority:
	- Performance problem with Saw/Square.
	- Rip off Google's tone LUT and write a simple tone ladder test function.
	- Ring buffer!
	- Envelope for carrier, and operations to use it for different voices.
	- Envelope or Bessel-related multiplier as modulation index (look at some code, basic LFO seems to do, or something derived from the sample level).

	- Tremolo is cool!
	- Read Ronny's notes.
	- Read the FM8 manual for design notes.
	- Don't forget: too clean is not cool.
	- MIDI support (includes actual notes with their own duration), only then can I start making useful decisions
	- Lot of this shit is too slow now for embedded, but theory first!
	- Problem down the line: need a circular or feed buffer probably.

	Decision made:
	- Aliasing is solved for now because the sinus osc. doesn't have that problem and the others are fixed.
	- ...
*/

#include "main.h"
#include "syntherklaas.h"

/*
	Constants.
*/

// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it).
const unsigned kSampleRate = 44100;
const unsigned kMaxSamplesPerUpdate = kSampleRate/4;

// Nyquist freq.
const float kNyquist = kSampleRate/2.f;

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	return true;
}

void Syntherklaas_Destroy()
{
}

/* 
	Number of harmonics calc.

	FIXME: decrease amount to speed up oscillators.
*/

VIZ_INLINE unsigned CalcHarmonics(float frequency)
{
	VIZ_ASSERT(frequency > 0.f);
	return unsigned(kNyquist/frequency);
}

/*
	Basic oscillators.
	Nyquist frequency is asserted for sawtooth and square, for sine it doesn't matter.
*/

enum Waveform
{
	kSine,
	kSaw,
	kSquare
};

VIZ_INLINE float oscSine(float beta) { return sinf(beta); }

// Band-limited, adapted from 2 sources:
// - https://www.sfu.ca/sonic-studio/handbook/index.html
// - https://github.com/rcliftonharvey/rchoscillators
//
// Assumes frequency is below below Nyquist!
// FIXME: optimize both, too costly!

VIZ_INLINE float oscSaw(float beta, float period, unsigned numHarmonics) 
{ 
	// Simple implementation:
	// return -1.f + fmodf(beta, period)/(kPI*0.5f);

	const float phase = -1.f*beta/k2PI;

	float state = 0.f;
	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
	{
		const float harmonicStep = (float) iHarmonic + 1.f;
		const float harmonicPhase = phase*harmonicStep;
		state += sinf(harmonicPhase * period)/harmonicStep;
	}

	const float ampMul = 2.f/kPI;
	state *= ampMul;

	return state;
}

// FM can turn this into a PWM-style signal.
VIZ_INLINE float oscSquare(float beta, unsigned numHarmonics) 
{ 
	// Simple implementation:
	// const float sine = oscSine(beta);
	// return (sine >= 0.f) ? 1.f : -1.f;

	const float phase = beta/k2PI;

	float state = 0.f;
	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
	{
		const float harmonicStep = (float) iHarmonic + 1.f;
		const float harmonicPhase = phase*harmonicStep;
		state += sinf(harmonicPhase * k2PI)/harmonicStep;
	}

	const float ampMul = 4.f/kPI;
	state *= ampMul;

	return state;
}

/*
	Oscillator angle calculator.
*/

VIZ_INLINE float oscAngle(unsigned sampleIdx, float frequency)
{
	const float time = (float) sampleIdx / kSampleRate;
	return time*frequency*k2PI;
}

/*
	FM modulator.

	- Ronny says no envelope is necessary.
*/

struct FM_Modulator
{
	float index; // In practice this is timbre.
	float frequency;

	// FIXME: wrap
	unsigned sampleIdx;

	void Prepare()
	{
		sampleIdx = 0;
	}

	float Sample()
	{
		float modulation = sinf(oscAngle(sampleIdx++, frequency));
		return index*modulation;
	}
};

/*
	FM operator.

	FIXME: Ronny says that just a sinus waveform should be sufficient, but let's pretend I'm hard of hearing.
	FIXME: envelope!
*/

struct FM_Operator
{
	Waveform form;

	float amplitude; // Amplitude must be 0-1.
	float frequency; // What's generally known as the carrier frequency.

	// FIXME: wrap
	unsigned sampleIdx;

	void Prepare()
	{
		VIZ_ASSERT(amplitude >= 0.f && amplitude <= 1.f);

		sampleIdx = 0;
	}

	// Classic Chowning FM formula.
	float Sample(FM_Modulator *pModulator)
	{
		const float modulation = (nullptr != pModulator) ? pModulator->Sample() : 0.f;

		const float beta = oscAngle(sampleIdx++, frequency);

		float signal = 0.f;
		switch (form)
		{
		case kSine:
			signal = oscSine(beta+modulation);
			break;

		case kSaw:
			VIZ_ASSERT(frequency < kNyquist);
			signal = oscSaw(beta+modulation, k2PI, CalcHarmonics(frequency));
			break;

		case kSquare:
			VIZ_ASSERT(frequency < kNyquist);
			signal = oscSquare(beta+modulation, CalcHarmonics(frequency));
			break;

		default:
			VIZ_ASSERT(false);
		}

		return amplitude*signal;
	}
};

/*
	Voice.
*/

struct FM_Voice
{
	FM_Operator carrier;
	FM_Modulator modulator;

	float Sample()
	{
		return carrier.Sample(&modulator);
	}
};

/*
	Global state.
*/

struct FM
{
	// FIXME: expand when you've got gear.
	FM_Voice voice;

} static s_FM;

/*
	Render functions.
*/

VIZ_INLINE void RenderVoice(FM_Voice &voice, float *pWrite, unsigned numSamples)
{
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		float sample = voice.Sample();
		*pWrite++ = sample;
	}
}

// FIXME: hack for now to initialize an indefinite voice and start the stream.
static bool s_isReady = false;

/*
	Simple RAW waveform writer.
*/

static void WriteToFile(unsigned seconds)
{
	FM test;
	FM_Voice &voice = test.voice;

	// Define single voice (indefinite)
	voice.carrier.form = kSquare;
	voice.carrier.amplitude = 1.f;
	voice.carrier.frequency = 440.f; // kNyquist*1.25f;
	voice.carrier.Prepare();

	voice.modulator.index = 1.f;
	voice.modulator.frequency = 20.f;
	voice.modulator.Prepare();

	// Render a fixed amount of seconds to a buffer
	const unsigned numSamples = seconds*kSampleRate;
	float *buffer = new float[numSamples];
	RenderVoice(voice, buffer, numSamples);

	// And flush (no error checking)
	FILE* file;
	file = fopen("fm_test.raw", "wb");
	fwrite(buffer, sizeof(float), numSamples, file);
	fclose(file);
}

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	FM_Voice &voice = s_FM.voice;

	if (false == s_isReady)
	{
		// Write test file with basic tone (FIXME: remove, replace)
		WriteToFile(32);

		// Define single voice (indefinite)
		voice.carrier.form = kSquare;
		voice.carrier.amplitude = 1.f;
		voice.carrier.frequency = 1.f;
		voice.carrier.Prepare();

		// FIXME: relate these by definition through a value?
		voice.modulator.index = kPI;
		voice.modulator.frequency = 440.f;
		voice.modulator.Prepare();

		// Start blasting!
		Audio_Start_Stream();
		s_isReady = true;
	}
	
	// Check for stream starvation
	VIZ_ASSERT(true == Audio_Check_Stream());
}

/*
	Stream callback.
*/

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	const unsigned numSamplesReq = length/sizeof(float);
	VIZ_ASSERT(length == numSamplesReq*sizeof(float));

	const unsigned numSamples = numSamplesReq; // std::min<unsigned>(numSamplesReq, kMaxSamplesPerUpdate);
	RenderVoice(s_FM.voice, static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}
