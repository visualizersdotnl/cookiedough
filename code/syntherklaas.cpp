
/*
	Syntherklaas -- FM synthesizer prototype.

	See: http://ccrma.stanford.edu/software/snd/snd/fm.html

	Important: Rocket not available (for now) in this mode, see main.cpp!

	This is intended to be a powerful yet relatively simple FM synthesizer core.
	Much of it's design is based on talks with Ronny Pries, Alex Bartholomeus, Tammo Hinrichs & Stijn Haring-Kuipers.

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	To do:
	- Check envelopes.
	- Voice logic seems kind of fucked.
	- Create saw and square using 'BLEP'.

	Current goals:
	- 1 instrument or 'patch' at a time.
	- Polyphony: 6 voices at once. Carrier frequency defined by MIDI input.
	- Patch configuration: carrier waveform, master envelope, master envelope operator, modulation frequency, modulation index LFO (or 'timbre modulation' if you will).

	Ideas for later:
	- Stacking and branching of modulators.
	- Vibrato (modulation of carrier) and tremolo (keep in mind to work with amplitude in dB scale).
	- Global resonance and cutoff.
	- Maybe: effects.
	- Maybe: multiple instruments (depends on goal, really; Eurorack won't need it, but that's unlikely).
	 
	Keep in mind:
	- The Slew-limiter Ronny offered might take care of that analogue falloff you have to deal with (looks great!).
	- Read IQ's little article on pitch modulation!
	- Read Ronny's notes.
	- Read the FM8 manual for design notes.
	- Don't forget: too clean is not cool.
	- Problem down the line: need a circular or feed buffer probably.
*/

#include "main.h"
#include "syntherklaas.h"

// Sinus LUT
#include "synth-sin-lut.h"

/*
	Constants.
*/

// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it).
const unsigned kSampleRate = 44100;
const unsigned kMaxSamplesPerUpdate = kSampleRate/4;

// Reasonable audible spectrum.
const float kAudibleLowHZ = 20.f;
const float kAudibleHighHZ = 22000.f;

// Nyquist frequencies.
const float kNyquist = kSampleRate/2.f;
const float kAudibleNyquist = std::min<float>(kAudibleHighHZ, kNyquist);

// Max. number of voices (FIXME: more!)
const unsigned kMaxVoices = 6;

// Number of discrete values that make up a period in the sinus LUT.
const unsigned kPeriodLength = kSinTabSize;

// Useful when using actual radians with sinus LUT;
const float kTabToRad = (1.f/k2PI)*kPeriodLength;

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	FM_CalculateSinLUT();

	return true;
}

void Syntherklaas_Destroy()
{
}

/*
	A few helper functions to translate between frequency and LUT.
*/

VIZ_INLINE float CalcSinPitch(float frequency)
{
	return (frequency*kPeriodLength)/kSampleRate;
}

// Not necessary for FM_lutsinf()!
VIZ_INLINE unsigned PhaseToPeriodIndex(float phase)
{
	const unsigned iPhase = (unsigned) phase;
	return iPhase & (kPeriodLength-1);
}

/* 
	Prototype envelopes (partially jacked from GR-1 codebase)
*/

#include "synth-envelopes.h"

/*
	Master envelope generator.

/* 

// ...

/*
	Index/timbre envelope generator.
	Envelope is 1 period long for now.
*/

float s_indexEnv[kPeriodLength];

/*
	Basic oscillators.

	FIXME: saw and square.
*/

enum Waveform
{
	kSine
//	kSaw,
//	kSquare
};

VIZ_INLINE float oscSine(float phase) { return FM_lutsinf(phase); }

/*
	FM modulator.
*/

struct FM_Modulator
{
	float index;
	float pitch;
	float phase;

	void Prepare(float constIndex, float frequency)
	{
		index = constIndex;
		pitch = CalcSinPitch(frequency);
		phase = 0.f;
	}

	float Sample()
	{
		const unsigned envIdx = PhaseToPeriodIndex(phase);
		const float envelope = s_indexEnv[envIdx];
		const float modulation = oscSine(phase)*kTabToRad;

		phase += pitch;

		return envelope*index*modulation;
	}
};

/*
	FM operator.
*/

struct FM_Operator
{
	Waveform form;

	float amplitude;
	float pitch;
	float phase;

	// FIXME: take amplitude in dB?
	void Prepare(Waveform form, float amplitude, float frequency)
	{
		VIZ_ASSERT(amplitude >= 0.f && amplitude <= 1.f);
		this->form = form;
		this->amplitude = amplitude;
		pitch = CalcSinPitch(frequency);
		phase = 0.f;
	}

	// Classic Chowning FM formula.
	float Sample(FM_Modulator *pModulator)
	{
		const float modulation = (nullptr != pModulator) ? pModulator->Sample() : 0.f;

		const float signal = oscSine(phase+modulation);
		phase += pitch;

		return amplitude*signal;
	}
};

/*
	Voice.

	FIXME: tie to / turn into note?
*/

struct FM_Voice
{
	bool active;
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
	FM_Voice voice[kMaxVoices];

	void Reset()
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			voice[iVoice].active = false;
	}

} static s_FM;

/*
	Render functions.
*/

VIZ_INLINE void RenderVoices(FM &voices, float *pWrite, unsigned numSamples)
{
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		float mix = 0.f;
		float divider = 0.f;

		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			FM_Voice &voice = voices.voice[iVoice];
			if (true == voice.active)
			{
				const float sample = voice.Sample();
				mix += sample;
				divider += 1.f;
			}
		}

		*pWrite++ = mix/divider;
	}
}

// FIXME: hack for now to initialize an indefinite voice and start the stream.
static bool s_isReady = false;

/*
	Test voice setup (+ envelope(s)).
*/

static void TestVoice1(FM_Voice &voice)
{
	// Define single voice (indefinite)
	voice.active = true;
	voice.carrier.Prepare(kSine, 1.f, 100.f);
	voice.modulator.Prepare(kPI, 1000.f);

	for (int i = 0; i < kPeriodLength; ++i)
		s_indexEnv[i] = 1.f;
}

/*
	Simple RAW waveform writer.
*/

static void WriteToFile(unsigned seconds)
{
	// Render a fixed amount of seconds to a buffer
	const unsigned numSamples = seconds*kSampleRate;
	float *buffer = new float[numSamples];
	RenderVoices(s_FM, buffer, numSamples);

	// And flush (no error checking)
	FILE* file;
	file = fopen("fm_test.raw", "wb");
	fwrite(buffer, sizeof(float), numSamples, file);
	fclose(file);
}

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	if (false == s_isReady)
	{
		// Test voice(s)
		s_FM.Reset();
		TestVoice1(s_FM.voice[0]);

		// Calculate mod. index envelope
//		CalcEnv_AD(s_indexEnv, 0.25f, 0.75f, 0.f);

		// Write test file with basic tone (FIXME: remove, replace)
		WriteToFile(32);

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
	RenderVoices(s_FM, static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}
