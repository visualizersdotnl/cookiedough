
/*
	Syntherklaas -- FM synthesizer prototype.

	See: http://ccrma.stanford.edu/software/snd/snd/fm.html

	Important: Rocket not available (for now) in this mode, see main.cpp!

	This is intended to be a powerful yet relatively simple FM synthesizer core.
	Much of it's design is based on talks with Ronny Pries, Alex Bartholomeus, Tammo Hinrichs & Stijn Haring-Kuipers.

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	To do:
	- Use 'BLEP' to filter aliasing.
	- Implement notes & ADSR.

	Current goals:
	- 1 instrument or 'patch' at a time: so a voice gets used by max. polyphony notes.
	- Polyphony: 6-12 voices at once. Carrier frequency defined by MIDI input.
	- Patch configuration: master envelope, master envelope operator, modulation frequencies, modulation index LFO (or 'timbre modulation' if you will).

	Ideas for later:
	- Stacking and branching of modulators, or: algorithms.
	- Vibrato (modulation of carrier) and tremolo (keep in mind to work with amplitude in dB scale).
	- Global resonance and cutoff.

	Maybe:
	- Effects like delay and chorus.
	 
	Keep in mind:
	- The Slew-limiter Ronny offered.
	- Read IQ's little article on pitch modulation!
	- Read Ronny's notes again?
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

/* 
	Prototype envelope(s) (partially jacked from GR-1 codebase)
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

static float s_indexEnv[kSampleRate];

static void ClearIndexEnvelope()
{
	for (int iEnv = 0; iEnv < kSampleRate; ++iEnv)
		s_indexEnv[iEnv] = 1.f;
}

/*
	Sinus oscillator.
	We'll create all other waveforms using FM.
*/

VIZ_INLINE float oscSine(float phase) { return FM_lutsinf(phase); }

/*
	FM modulator.
*/

struct FM_Modulator
{
	float m_index;
	float m_pitch;
	unsigned m_sample;

	void Initialize(float index, float frequency)
	{
		m_index = index;
		m_pitch = CalcSinPitch(frequency);
		m_sample = 0;
	}

	float Sample(const float *pEnv)
	{
		const float phase = m_sample*m_pitch;

		float envelope = 1.f;
		if (pEnv != nullptr)
		{
			const unsigned index = m_sample%kSampleRate; // FIXME: slow, make power of 2.
			envelope = pEnv[index];
		}

		++m_sample;

		const float modulation = oscSine(phase)*kTabToRad;
		return envelope*m_index*modulation;
	}
};

/*
	FM carrier.
*/

struct FM_Carrier
{
	float m_amplitude;
	float m_pitch;
	unsigned m_sample;

	void Initialize(float amplitude, float frequency)
	{
		m_amplitude = amplitude;
		m_pitch = CalcSinPitch(frequency);
		m_sample = 0;
	}

	// Classic Chowning FM formula.
	float Sample(float modulation)
	{
		const float phase = m_sample*m_pitch;
		++m_sample;

		const float signal = oscSine(phase+modulation);
		return m_amplitude*signal;
	}
};

/*
	Voice.
	FIXME: load with a patch, then 1 instance be used for notes.
*/

struct FM_Voice
{
	FM_Carrier carrier;
	FM_Modulator modulator;

	float Sample()
	{
		float modulation = modulator.Sample(s_indexEnv);
		return carrier.Sample(modulation);
	}
};

/*
	Global state.
*/

struct FM
{
	// For now there's just 1 indefinite voice.
	FM_Voice voice;
} static s_FM;

/*
	Render functions.
*/

VIZ_INLINE void RenderVoices(FM &core, float *pWrite, unsigned numSamples)
{
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		*pWrite++ = core.voice.Sample();
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
	voice.carrier.Initialize(1.f, 440.f);
	voice.modulator.Initialize(2.f, 44.4f);
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
	// Calculate mod. index envelope
	CalcEnv_Cosine(s_indexEnv, 1.f, 2.f + cosf(time), 2.f);

	if (false == s_isReady)
	{
		// Test voice
		TestVoice1(s_FM.voice);

		// Calculate mod. index envelope
//		CalcEnv_Cosine(s_indexEnv, 1.f, 2.f, 2.f);
//		ClearIndexEnvelope();		

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
