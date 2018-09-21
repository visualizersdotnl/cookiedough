
/*
	See: http://ccrma.stanford.edu/software/snd/snd/fm.html
	Important: Rocket not available (for now) in this mode, see main.cpp!

	This is intended to be a powerful yet relatively simple FM synthesizer core.
	Much of it's design is based on a discussion with Ronny Pries.

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	Current goals:
	- 1 instrument or 'patch' at a time.
	- Polyphony: 6 voices at once. Carrier frequency defined by MIDI input.
	- Patch configuration: carrier waveform, master envelope, master envelope operator, modulation frequency, modulation index LFO (or 'timbre modulation' if you will).

	Ideas for later:
	- Vibrato and tremolo (keep in mind to work with amplitude in dB scale).
	- Global resonance and cutoff.
	- Maybe: effects.
	- Maybe: multiple instruments (depends on goal, really; Eurorack won't need it, but that's unlikely).
	 
	Keep in mind:
	- The Slew-limiter Ronny offered might take care of that analogue falloff you have to deal with.
	- Read IQ's little article on pitch modulation!
	- Read Ronny's notes.
	- Read the FM8 manual for design notes.
	- Don't forget: too clean is not cool.
	- Problem down the line: need a circular or feed buffer probably.
*/

#include "main.h"
#include "syntherklaas.h"

/*
	Constants.
*/

// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it).
const unsigned kSampleRate = 44100;
const unsigned kMaxSamplesPerUpdate = kSampleRate/4;

// Reasonable audible spectrum.
const float kAudibleLowHZ = 20.f;
const float kAudiobleHighHZ = 22000.f;

// Nyquist frequencies.
const float kNyquist = kSampleRate/2.f;
const float kAudibleNyquist = std::min<float>(kAudiobleHighHZ, kNyquist);

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
	Carrier harmonics calculation.
*/

// This is quite a steep divider and it gives a pretty mellowed out sound, but perhaps that can be regulated later (FIXME).
const float kHarmonicsPrecHZ = kAudibleLowHZ;

VIZ_INLINE unsigned GetNumCarrierHarmonics(float frequency)
{
	VIZ_ASSERT(frequency >= 20.f);
	const float lower = (kAudibleNyquist/frequency)/kHarmonicsPrecHZ;
	return unsigned(lower);
}

/*
	Basic oscillators.

	Saw and square are a bit more complicated due to harmonics than the sine which is pretty much gauranteed to
	have no sidebands.

	The saw and square are momentarily not intended to be used for anything else than carrier (tone) generation.
	They assume a frequency within the (reasonably) audible spectrum.
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

VIZ_INLINE float oscSaw(float beta, unsigned numHarmonics) 
{ 
	// Simple implementation:
//	return -1.f + fmodf(beta, k2PI)/(kPI*0.5f);

	const float phase = -1.f*beta;
	float harmonicPhase = phase;

	float signal = 0.f;

	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
	{
		signal += sinf(harmonicPhase)/(1.f+iHarmonic);
		harmonicPhase += phase;
	}

	const float ampMul = 2.f/kPI;
	signal *= ampMul;

	return signal;
}

// FM can turn this into a PWM-style signal.
VIZ_INLINE float oscSquare(float beta, unsigned numHarmonics) 
{ 
	// Simple implementation:
	// const float sine = oscSine(beta);
	// return (sine >= 0.f) ? 1.f : -1.f;

	float harmonicPhase = beta;
	const float hPhaseStep = beta*2.f;

	float signal = 0.f;

	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
	{
		signal += sinf(harmonicPhase)/(1.f+iHarmonic);
		harmonicPhase += hPhaseStep;
	}

	const float ampMul = 4.f/kPI;
	signal *= ampMul;

	return signal;
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
	FIXME: Ronny says no envelope is necessary, except of course for the index/timbre.
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

	FIXME: Ronny says that just a sinus waveform should be sufficient, but let's pretend I'm hard of hearing, so I've got 3.
	FIXME: master envelope and envelope operators!
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

		VIZ_ASSERT(frequency >= 1.f);
		const float beta = oscAngle(sampleIdx++, frequency);

		float signal = 0.f;
		switch (form)
		{
		case kSine:
			signal = oscSine(beta+modulation);
			break;

		case kSaw:
			VIZ_ASSERT(frequency < kNyquist);
			signal = oscSaw(beta+modulation, GetNumCarrierHarmonics(frequency));
			break;

		case kSquare:
			VIZ_ASSERT(frequency < kNyquist);
			signal = oscSquare(beta+modulation, GetNumCarrierHarmonics(frequency));
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
	voice.carrier.form = kSaw;
	voice.carrier.amplitude = 1.f;
	voice.carrier.frequency = 100.f;
	voice.carrier.Prepare();

	voice.modulator.index = 1.f;
	voice.modulator.frequency = 1000.f;
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
		voice.carrier.frequency = 440.f;
		voice.carrier.Prepare();

		voice.modulator.index = 1.f;
		voice.modulator.frequency = 400.f*10.f;
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

	const unsigned numSamples = std::min<unsigned>(numSamplesReq, kMaxSamplesPerUpdate);
	RenderVoice(s_FM.voice, static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}
