
/*
	Syntherklaas -- FM synthesizer prototype.

	See: http://ccrma.stanford.edu/software/snd/snd/fm.html

	Important: Rocket not available (for now) in this mode, see main.cpp!

	This is intended to be a powerful yet relatively simple FM synthesizer core.
	Much of it's design is based on talks with Ronny Pries, Alex Bartholomeus, Tammo Hinrichs & Stijn Haring-Kuipers.

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	To do:
	- Figure out MOOG ladder parameters exactly.
	- Ring buffer.
	- Implement notes (generator) & ADSR.
	- Modulation LFO.
	- Modulator feedback.

	Current goals:
	- 1 instrument or 'patch' at a time: so a voice gets used by max. polyphony notes.
	- Polyphony: 6-12 voices at once. Carrier frequency defined by MIDI input.
	- Patch configuration: master envelope, master envelope operator, modulation frequencies, modulation index LFO (or 'timbre modulation' if you will).

	Ideas for later:
	- Stacking and branching of modulators, or: algorithms.
	- Vibrato (modulation of carrier) and tremolo (keep in mind to work with amplitude in dB scale).

	Maybe:
	- Envelope sampler (interpolation).
	- Effects like delay and chorus.
	 
	Keep in mind:
	- The Slew-limiter Ronny offered.
	- Read IQ's little article on pitch modulation!
	- Read Ronny's notes again?
	- Read the FM8 manual for design notes.

	Don't forget: too clean is not cool.
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
	Final filter (MOOG ladder including cutoff and resonance).
*/

#include "synth-moog-ladder.h"

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
	Note frequency calculator (according to Alex).
*/



/*
	A few helper functions to translate between frequency and LUT.
*/

VIZ_INLINE float CalcSinPitch(float frequency)
{
	return (frequency*kPeriodLength)/kSampleRate;
}

/*
	Index/timbre envelope generator.
	Envelope is 1 period long for now.
*/

// Prototype envelope(s) (partially jacked from GR-1 codebase)
#include "synth-envelopes.h"

static float s_indexEnv[kPeriodLength];

static void ClearIndexEnvelope()
{
	for (int iEnv = 0; iEnv < kPeriodLength; ++iEnv)
		s_indexEnv[iEnv] = 1.f;
}

/*
	Sinus oscillator.
*/

VIZ_INLINE float oscSine(float phase) { return FM_lutsinf(phase); }

/*
	Band-limited saw and square (additive sinuses).
	If you want dirt, use modulation and/or envelopes.

	** Only to be used for carrier signals within audible range or Nyquist **
*/

const float kHarmonicsPrecHZ = kAudibleLowHZ;

VIZ_INLINE unsigned GetCarrierHarmonics(float frequency)
{
	VIZ_ASSERT(frequency >= 20.f);
	const float lower = (kAudibleNyquist/frequency)/kHarmonicsPrecHZ;
	return unsigned(lower);
}

VIZ_INLINE float oscSaw(float phase, unsigned numHarmonics) 
{ 
	phase *= -1.f;
	float harmonicPhase = phase;

	float signal = 0.f;

	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
	{
		signal += FM_lutsinf(harmonicPhase)/(1.f+iHarmonic);
		harmonicPhase += phase;
	}

 	const float ampMul = 2.f/kPI;
	signal *= ampMul;

	return signal;
}

VIZ_INLINE float oscSquare(float phase, unsigned numHarmonics) 
{ 
	float harmonicPhase = phase;

	float signal = 0.f;

	for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
	{
		signal += FM_lutsinf(harmonicPhase)/(1.f+iHarmonic);
		harmonicPhase += phase*2.f;
	}

 	const float ampMul = 4.f/kPI;
	signal *= ampMul;

	return signal;
}

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
		float phase = m_sample*m_pitch;

		float envelope = 1.f;
		if (pEnv != nullptr)
		{
			const unsigned index = m_sample & (kPeriodLength-1);
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

enum CarrierForm
{
	kSineCarrier,
	kSawCarrier,
	kSquareCarrier
};

struct FM_Carrier
{
	CarrierForm m_form;
	float m_amplitude;
	float m_pitch;
	unsigned m_sample;
	unsigned m_numHarmonics;

	void Initialize(CarrierForm form, float amplitude, float frequency)
	{
		m_form = form;
		m_amplitude = amplitude;
		m_pitch = CalcSinPitch(frequency);
		m_sample = 0;
		m_numHarmonics = GetCarrierHarmonics(frequency);
	}

	// Classic Chowning FM formula.
	float Sample(float modulation)
	{
		const float phase = m_sample*m_pitch;
		++m_sample;

		float signal = 0.f;
		switch (m_form)
		{
		case kSineCarrier:
			signal = oscSine(phase+modulation);
			break;

		case kSawCarrier:
			signal = oscSaw(phase+modulation, m_numHarmonics);
			break;

		case kSquareCarrier:
			signal = oscSquare(phase+modulation, m_numHarmonics);
			break;
		}

		return m_amplitude*signal;
	}
};

/*
	Voice.

	FIXME: testbed for now.
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

VIZ_INLINE void RenderVoices(FM &core, float *pDest, unsigned numSamples)
{
	float *pWrite = pDest;
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		*pWrite++ = core.voice.Sample();
	}

	FM_Apply_Moog_Ladder(pDest, numSamples);
}

// FIXME: hack for now to initialize an indefinite voice and start the stream.
static bool s_isReady = false;

/*
	Test voice setup (+ envelope(s)).
*/

static void TestVoice1(FM_Voice &voice)
{
	// Define single voice (indefinite)
	voice.carrier.Initialize(kSawCarrier, 1.f, 440.f);
	voice.modulator.Initialize(10.f, 1000.f);
}

/*
	Simple RAW waveform writer.
*/

static void WriteToFile(unsigned seconds)
{
	FM_Moog_Ladder_Reset();

	// Render a fixed amount of seconds to a buffer
	const unsigned numSamples = seconds*kSampleRate;
	float *buffer = new float[numSamples];

	const float timeStep = 1.f/kSampleRate;
	float time = 0.f;
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		FM_Moog_Ladder_Set_Cutoff(1500.f + 500.f*cosf(time));
		FM_Moog_Ladder_Set_Resonance(1.f+sinf(time));
		RenderVoices(s_FM, buffer+iSample, 1);
		time += timeStep;
	}

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
		// Test voice
		TestVoice1(s_FM.voice);

		// Calculate mod. index envelope
		CalcEnv_Cosine(s_indexEnv, kPeriodLength, 0.8f, 2.f, 1000.f);
//		ClearIndexEnvelope();		

		// Reset final filter.
		FM_Moog_Ladder_Reset();

		// Write test file with basic tone (FIXME: remove, replace)
		WriteToFile(32);

		// Start blasting!
		Audio_Start_Stream();
		s_isReady = true;
	}

	// Test MOOG ladder
	FM_Moog_Ladder_Set_Cutoff(1500.f + 500.f*cosf(time*kPI));
	FM_Moog_Ladder_Set_Resonance(1.f+sinf(time*k2PI));
	
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
