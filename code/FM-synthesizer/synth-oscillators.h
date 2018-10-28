
/*
	Syntherklaas FM -- Oscillators.

	The idea is to, in due time, precalculate all these and use samplers during actual synthesis as is practically the case already
	for everything that depends on the sinus LUT.
*/

#pragma once

#include "synth-global.h"
#include "synth-random.h"
#include "synth-math.h"

namespace SFM
{
	/*
		Available waveforms.
	*/

	enum Waveform
	{
		kSine,
		/* BLIT forms */
		kSoftSaw,
		kSoftSquare,
		/* Straight forms */
		kDigiSaw,
		kDigiSquare,
		kTriangle,
		kWhiteNoise,
		kPinkNoise,
		/* Wavetable */
		kKick808,
		kSnare808,
		kGuitar
	};

	SFM_INLINE bool oscIsWavetable(Waveform form)
	{
		return form >= kKick808 && form <= kGuitar;
	}

	/*
		Sinus oscillator.
	*/

	SFM_INLINE float oscSine(float phase) { return lutsinf(phase); }

	/*
		Straight up sawtooth, square & triangle (will alias in most circumstances).
	*/

	SFM_INLINE float oscDigiSaw(float phase)
	{
		return fmodf(phase/kOscPeriod, 1.f)*2.f - 1.f;
	}

	SFM_INLINE float oscDigiSquare(float phase)
	{
		return lutsinf(phase) > 0.f ? 1.f : -1.f;
	}

	SFM_INLINE float oscTriangle(float phase)
	{
		return -1.f + fabsf(roundf(phase/kOscPeriod)-(phase/kOscPeriod))*4.f;
	}

	/*
		Band-limited saw and square (BLIT).
	*/

	const float kHarmonicsPrecHz = kAudibleLowHz;

	SFM_INLINE unsigned GetCarrierHarmonics(float frequency)
	{
		SFM_ASSERT(frequency >= 20.f);
		return 40; // FIXME
	}

	SFM_INLINE float oscSoftSaw(float phase, unsigned numHarmonics) 
	{
		float signal = 0.f, accPhase = phase*-1.f;
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
		Noise oscillator(s).
	*/

	SFM_INLINE float oscWhiteNoise(float phase)
	{
		return lutnoisef(phase + rand() /* Without this we'll definitely hear a pattern */);
	}

	// Paul Kellet's approximation to pink noise; basically just a filter
	// Taken from: http://www.firstpr.com.au/dsp/pink-noise/
	SFM_INLINE float oscPinkNoise(float phase)
	{
		const float white = oscWhiteNoise(phase);

		static float b0 = 0.f, b1 = 0.f, b2 = 0.f, b3 = 0.f, b4 = 0.f, b5 = 0.f, b6 = 0.f;
		static float pink = 0.5f;

		b0 = 0.99886f*b0 + white*0.0555179f;
		b1 = 0.99332f*b1 + white*0.0750759f; 
		b2 = 0.96900f*b2 + white*0.1538520f; 
		b3 = 0.86650f*b3 + white*0.3104856f; 
		b4 = 0.55000f*b4 + white*0.5329522f; 
		b5 = -0.7616f*b5 - white*0.0168980f; 
		
		// This is a bit of a judgement call but I prefer clearly hearing different noise over keeping
		// the not-too-exact spectral properties
		pink = lowpassf(b0+b1+b2+b3+b4+b5+b6 + white*0.5362f, pink, 1.314f); 

		b6 = white*0.115926f;
		
		return pink;
   }

   /*
		Wavetable oscillator(s)

		FIXME: 
			- Multiple tables to better accomodate different pitches
	*/

	class WavetableOscillator
	{
	public:
		WavetableOscillator(const uint8_t *pTable, unsigned length, unsigned octaveOffs, float baseHz = 220.f /* A3 */) :
			m_pTable(reinterpret_cast<const float*>(pTable))
,			m_numSamples(length/sizeof(float))
,			m_periodLen(float(m_numSamples)/kOscPeriod)
	{
		m_basePitch = CalculateOscPitch(baseHz * powf(2.f, float(octaveOffs)));
		m_rate = 1.f/(m_basePitch/m_periodLen);
	}

	SFM_INLINE float Sample(float phase) const
	{
		const float index = phase*m_rate;
		const unsigned from = unsigned(index);
		const unsigned to = from+1;
		const float delta = fracf(index);
		const float A = m_pTable[from%m_numSamples];
		const float B = m_pTable[from%m_numSamples];
		return lerpf<float>(A, B, delta);
		
//		const unsigned sample = unsigned(phase*m_rate);
//		return m_pTable[sample%m_numSamples];
	}

	float GetLength() const
	{
		return floorf(m_periodLen/m_rate * (kOscPeriod)) - 1.f;
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
}
