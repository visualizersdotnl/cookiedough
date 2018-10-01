
/*
	Syntherklaas FM
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	This is intended to be a powerful yet relatively simple FM synthesizer core.

	I have to thank the following people for helping me with their knowledge, ideas and experience:
		- Ronny Pries
		- Tammo Hinrichs
		- Alex Bartholomeus
		- Pieter v/d Meer
		- Stijn Haring-Kuipers
		- Thorsten Ørts

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	To do:
	- See notebook.
*/

#include "core.h"
#include "sinus-LUT.h"
#include "final-MOOG-ladder.h"

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	SFM::CalculateSinLUT();

	SFM::MOOG_Reset_Parameters();
	SFM::MOOG_Reset_Feedback();

	return true;
}

void Syntherklaas_Destroy()
{
}

namespace SFM
{

	/*
		Frequency to pitch calculators.
	*/

	// Frequency to (PI*2)/tabSize
	SFM_INLINE float CalcSinPitch(float frequency)
	{
		return (frequency*kPeriodLength)/kSampleRate;
	}

	// Frequency to PI*2
	SFM_INLINE float CalRadPitch(float frequency)
	{
		return (frequency*k2PI)/kSampleRate;
	}

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }

	/*
		Band-limited saw and square (additive sinuses).
		If you want dirt, use modulation and/or envelopes.

		** Only to be used for carrier signals within audible range or Nyquist **
	*/

	const float kHarmonicsPrecHZ = kAudibleLowHZ;
	// const float kHarmonicsPrecHZ = kAudibleLowHZ*0.1f;

	SFM_INLINE unsigned GetCarrierHarmonics(float frequency)
	{
		VIZ_ASSERT(frequency >= 20.f);
		const float lower = (kAudibleNyquist/frequency)/kHarmonicsPrecHZ;
		return unsigned(lower);
	}

	SFM_INLINE float oscSaw(float phase, unsigned numHarmonics) 
	{ 
		phase *= -1.f;
		float harmonicPhase = phase;

		float signal = 0.f;

		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
		{
			signal += lutsinf(harmonicPhase)/(1.f+iHarmonic);
			harmonicPhase += phase;
		}

 		const float ampMul = 2.f/kPI;
		signal *= ampMul;

		return signal;
	}

	SFM_INLINE float oscSquare(float phase, unsigned numHarmonics) 
	{ 
		float harmonicPhase = phase;

		float signal = 0.f;

		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
		{
			signal += lutsinf(harmonicPhase)/(1.f+iHarmonic);
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
			float modulation = modulator.Sample(nullptr);
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
		Test voice setup.
		FIXME: make note trigger!
	*/

	static void SetTestVoice(FM_Voice &voice)
	{
		// Define single voice (indefinite)
		voice.carrier.Initialize(kSquareCarrier, 1.f, 220.f);
		const float CM = 220.f/4.f;
		voice.modulator.Initialize(CM, 10.f);
	}

	static void SetTestMOOG(float time)
	{
		float test = 1.f+cosf(time);
		MOOG_Cutoff(700.f + 300.f*test);
		// MOOG_Resonance(0.f + kPI*test);
	}

	/*
		Render functions.
	*/

	SFM_INLINE void RenderVoices(float *pDest, unsigned numSamples)
	{
		float *pWrite = pDest;
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			float sample = s_FM.voice.Sample();
			MOOG_Ladder(&sample, 1);
			*pWrite++ = sample;
		}
	}

	// FIXME: hack for now to initialize an indefinite voice and start the stream.
	static bool s_isReady = false;

	/*
		Simple RAW waveform writer.
	*/

	static float WriteToFile(unsigned seconds)
	{
		// Render a fixed amount of seconds to a buffer
		const unsigned numSamples = seconds*kSampleRate;
		float *buffer = new float[numSamples];

		const float timeStep = 1.f/kSampleRate;
		float time = 0.f;
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			SetTestMOOG(time);
			RenderVoices(buffer+iSample, 1);
			time += timeStep;
		}

		// And flush (no error checking)
		FILE* file;
		file = fopen("fm_test.raw", "wb");
		fwrite(buffer, sizeof(float), numSamples, file);
		fclose(file);

		return time;
	}
}; // namespace SFM


using namespace SFM;

/*
	Render function for Kurt Bevacqua codebase.
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	if (false == s_isReady)
	{
		// Test voice
		SetTestVoice(s_FM.voice);

		// Write test file with basic tone (FIXME: remove, replace)
		WriteToFile(32);

		// Start blasting!
		Audio_Start_Stream();
		s_isReady = true;
	}

	// Resest MOOG filter
	SetTestMOOG(time);
	
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

	const unsigned numSamples = std::min<unsigned>(numSamplesReq, SFM::kMaxSamplesPerUpdate);
	SFM::RenderVoices(static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}

