
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

		/* PolyBLEP forms */
		kPolySaw,
		kPolySquare,
		kPolyPulse,

		/* Straight forms */
		kDigiSaw,
		kDigiSquare,
		kDigiTriangle,
		kSofterTriangle,
		kDigiPulse,
		kWhiteNoise,
		kPinkNoise,
		kBrownishNoise,

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
		return -1.f + fabsf(roundf(phase)-phase)*4.f;
	}

	SFM_INLINE float oscDigiPulse(float phase, float duty)
	{
		const float time = fmodf(phase, 1.f);
		if (time < duty)
			return 1.f;
		else
			return -1.f;
	}

	/*
		Clamped triangle; little softer on the ears, but essentially a hack.
	*/

	SFM_INLINE float oscSofterTriangle(float phase)
	{
		return fast_tanhf(-1.f + fabsf(roundf(phase)-phase)*4.f);
	}

	/*
		Band-limited saw and square (BLIT).

		These variants are very nice but computationally expensive.
	*/

	SFM_INLINE float oscSoftSaw(float phase, unsigned numHarmonics) 
	{
		phase *= -1.f;
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; ++iHarmonic)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += phase;
		}

		return signal*(2.f/kPI);
	}

	SFM_INLINE float oscSoftSquare(float phase, unsigned numHarmonics) 
	{ 
		float signal = 0.f, accPhase = phase;
		for (unsigned iHarmonic = 0; iHarmonic < numHarmonics; iHarmonic += 2)
		{
			signal += lutsinf(accPhase)/(1.f+iHarmonic);
			accPhase += 2.f*phase;
		}

		return signal*(4.f/kPI);
	}

	/*
		PolyBLEP saw & square.

		Why no triangle? A triangle wave does not exhibit clear discontinuities and thus I'd like to think that any extra effort is wasted;
		instead if you want to soften that sound use a filter or other features of the synthesizer to get there.

		Information used:
		- http://www.martin-finke.de/blog/articles/audio-plugins-018-polyblep-oscillator/

		In essence this interpolates exponentially around discontinuities.
	*/

	const float kPolySoftness = 8.f; // Might be a fun parameter to test once the oscillators sound OK (FIXME)
	const float kPolyDiv = kSampleRate/kPolySoftness; // FIXME: recip.

	SFM_INLINE float SawBLEP(float phase, float delta)
	{
		phase = fmodf(phase, 1.f);

		float value = 0.f;

		if (phase < delta)
		{
			phase /= delta;
			value = phase+phase - phase*phase - 1.f;
		}
		else if (phase > 1.f-delta)
		{
			phase = (phase-1.f)/delta;
			value = phase*phase + phase+phase + 1.f;
		}

		return value;
	}

	SFM_INLINE float oscPolySaw(float phase, float frequency)
	{
		float value = oscDigiSaw(phase);
		value -= SawBLEP(phase+1.f, frequency/kPolyDiv);
		return value;
	}

	SFM_INLINE float oscPolySquare(float phase, float frequency)
	{
		const float saw = oscPolySaw(phase, frequency);
		const float shifted = oscPolySaw(phase + 0.5f, frequency);
		const float value = shifted-saw;
		return value;
	}

	SFM_INLINE float oscPolyPulse(float phase, float frequency, float duty)
	{
		float value = oscDigiPulse(phase, duty);
		return value;
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
		
		return pink;
   }

   // Approximation at best
   	SFM_INLINE float oscBrownishNoise()
	{
		static float value = 0.f;
		const float noise = oscWhiteNoise();
		value = lowpassf(value, noise, kGoldenRatio);
		return value;
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
,			m_periodLen(float(m_numSamples))
	{
		float pitchMul = powf(2.f, float(octaveOffs) + semitones/12.f);
		if (octaveOffs < 0) pitchMul = 1.f/pitchMul;
		m_basePitch = CalculatePitch(baseHz*pitchMul);
		m_rate = m_basePitch/m_periodLen;
	}

	SFM_INLINE float Sample(float phase) const
	{
		const float index = phase*m_rate;
		const unsigned from = unsigned(index);
		const unsigned to = from+1;
		const float delta = index-from;
		const float A = m_pTable[from%m_numSamples];
		const float B = m_pTable[to%m_numSamples];
		return lerpf<float>(A, B, delta);
	}

	float GetLength() const
	{
		return floorf(m_periodLen/m_rate);
	}

	private:
		const float *m_pTable;
		const unsigned m_numSamples;
		/* const */ float m_basePitch;
		const float m_periodLen;
		/* const */ float m_rate;
	};

	WavetableOscillator &getOscKick808();
	WavetableOscillator &getOscSnare808();
	WavetableOscillator &getOscGuitar();
	WavetableOscillator &getOscElecPiano();
}
