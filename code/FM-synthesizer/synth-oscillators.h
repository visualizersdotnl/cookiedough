
/*
	Syntherklaas FM -- Oscillators.

	These oscillators are and shall remain stateless (unless there's a good reason not to).
*/

#pragma once

#include "synth-global.h"
#include "synth-random.h"

namespace SFM
{
	/*
		Available waveforms.
	*/

	enum Waveform
	{
		kSine,
		kCosine,

		/* BLIT forms */
		kSoftSaw,
		kSoftSquare,

		/* Straight forms */
		kDigiSaw,
		kDigiSquare,
		kDigiTriangle,
		kSofterTriangle,
		kDigiPulse,
		kWhiteNoise,
		kPinkNoise,

		/* Wavetable */
		kKick808,
		kSnare808,
		kGuitar,
		kElectricPiano
	};

	SFM_INLINE bool oscIsWavetable(Waveform form)
	{
		return form >= kKick808 && form <= kElectricPiano;
	}

	SFM_INLINE bool oscIsProcedural(Waveform form)
	{
		return false == oscIsWavetable(form);
	}

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }
	SFM_INLINE float oscCos(float phase)  { return lutcosf(phase); }

	/*
		Digital sawtooth, square & triangle (alias at audible frequencies).
	*/

	SFM_INLINE float oscDigiSaw(float phase)
	{
		return fmodf(phase, 1.f)*2.f - 1.f;
	}

	SFM_INLINE float oscDigiSquare(float phase)
	{
		return lutsinf(phase) > 0.f ? 1.f : -1.f;
	}

	SFM_INLINE float oscDigiTriangle(float phase)
	{
		return 2.f*(asinf(lutsinf(phase))*(1.f/kPI));
	}

	SFM_INLINE float oscSofterTriangle(float phase) { return fast_tanhf(oscDigiTriangle(phase)); }

	SFM_INLINE float oscDigiPulse(float phase, float duty)
	{
		const float time = fmodf(phase, 1.f);
		return (time < duty) ? 1.f : -1.f;
	}

	/*
		Band-limited saw and square (BLIT).

		These variants are very nice but computationally expensive.
	*/

	SFM_INLINE unsigned BLIT_GetNumHarmonics(float frequency)
	{
		const unsigned numHarmonics = (unsigned) floorf(kNyquist/(kAudibleLowHz+frequency));
		return numHarmonics;
	}

	SFM_INLINE float oscSoftSaw(float phase, float frequency) 
	{
		const unsigned numHarmonics = BLIT_GetNumHarmonics(frequency);

		phase *= -1.f;
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += phase;
		}

		return signal*(2.f/kPI);
	}

	SFM_INLINE float oscSoftSquare(float phase, float frequency) 
	{ 
		const unsigned numHarmonics = BLIT_GetNumHarmonics(frequency);
		
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += 2.f*phase;
		}

		return signal*(4.f/kPI);
	}

	/*
		Noise oscillator(s).
	*/

	SFM_INLINE float oscWhiteNoise()
	{
		// No need for the noise LUT (FIXME: yet)
		return -1.f + 2.f*mt_randf();
	}

	// Paul Kellet's approximation to pink noise; basically just a filter resulting in a "softer" spectral distribution
	// Taken from: http://www.firstpr.com.au/dsp/pink-noise/
	SFM_INLINE float oscPinkNoise()
	{
		const float white = oscWhiteNoise();

		static float b0 = 0.f, b1 = 0.f, b2 = 0.f, b3 = 0.f, b4 = 0.f, b5 = 0.f, b6 = 0.f;
		static float pink = 0.f;

		b0 = 0.99886f*b0 + white*0.0555179f;
		b1 = 0.99332f*b1 + white*0.0750759f; 
		b2 = 0.96900f*b2 + white*0.1538520f; 
		b3 = 0.86650f*b3 + white*0.3104856f; 
		b4 = 0.55000f*b4 + white*0.5329522f; 
		b5 = -0.7616f*b5 - white*0.0168980f; 

		pink = b0+b1+b2+b3+b4+b5+b6 + white*0.5362f;

		b6 = white*0.115926f;
		
		return Clamp(pink);
   }

   /*
		Wavetable oscillator(s)

		FIXME: 
			- Better filtering
	*/

	class WavetableOscillator
	{
	public:
		WavetableOscillator(const uint8_t *pTable, unsigned length, int octaveOffs, int semitones, float baseHz = 220.f /* A3 */) :
			m_pTable(reinterpret_cast<const float*>(pTable))
,			m_numSamples(length/sizeof(float))
	{
		m_periodLen = m_numSamples/(float)kSampleRate;
		
		const float tonalOffs = float(octaveOffs) + (semitones/12.f);
		const float pitchMul = powf(2.f, tonalOffs);
		const float basePitch = CalculatePitch(baseHz*pitchMul);

		m_rate = basePitch*m_periodLen;
	}

	SFM_INLINE float Sample(float phase) const
	{
		const float index = phase/m_rate;
		const unsigned from = unsigned(index);
		const unsigned to = from+1;
		const float delta = index-from;
		const float A = m_pTable[from%m_numSamples];
		const float B = m_pTable[to%m_numSamples];
		return lerpf<float>(A, B, delta);
	}

	float GetLength() const
	{
		return m_numSamples*m_rate;
	}

	private:
		const float *m_pTable;
		const unsigned m_numSamples;
		float m_periodLen;
		float m_rate;
	};

	WavetableOscillator &getOscKick808();
	WavetableOscillator &getOscSnare808();
	WavetableOscillator &getOscGuitar();
	WavetableOscillator &getOscElecPiano();
}
