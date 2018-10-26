
/*
	Syntherklaas FM -- Master filter(s)

	Each end filter should implement the following functions:
	- Reset()
	- SetParameters()
	- SetCutoff() (normalized [0..1])
	- SetResonance() (same)
	- SetEnvelopeScale() (same)
	- Apply()

	FIXME: move code out of header file
*/

#pragma once

#include "synth-global.h"
#include "synth-math.h"
#include "synth-ADSR.h"

namespace SFM
{
	/*
		Filter parameters.
	*/

	struct FilterParameters
	{
		// [0..N]
		float drive;
	
		// Normalized [0..1]
		float cutoff;
		float resonance;
		float envInfl;
	};

	/*
		Microtracker MOOG filter
		
		Credit:
		- Based on an implementation by Magnus Jonsson
		- https://github.com/magnusjonsson/microtracker (unlicense)
	*/

	struct MicrotrackerMoogFilter
	{
		float m_cutoff;
		float m_resonance;
		float m_envInfl;

		float m_P0, m_P1, m_P2, m_P3;
		float m_resoCoeffs[3];

		void Reset()
		{
			m_P0 = m_P1 = m_P2 = m_P3 = 0.f;
			m_resoCoeffs[0] = m_resoCoeffs[1] = m_resoCoeffs[2] = 0.f;
		}

		void SetParameters(const FilterParameters &parameters)
		{
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
			SetEnvelopeInfluence(parameters.envInfl);
		}

		void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			value = value*2.f*kPI/kSampleRate;
			m_cutoff = std::min<float>(1.f, value); // Also known as omega
		}

		void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = std::max<float>(kEpsilon, value*4.f);
		}

		void SetEnvelopeInfluence(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_envInfl = value;
		}

		void Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope);
	};

	/*
		Improved MOOG ladder filter.

		This model is based on a reference implementation of an algorithm developed by
		Stefano D'Angelo and Vesa Valimaki, presented in a paper published at ICASSP in 2013.
		This improved model is based on a circuit analysis and compared against a reference
		Ngspice simulation. In the paper, it is noted that this particular model is
		more accurate in preserving the self-oscillating nature of the real filter.
		References: "An Improved Virtual Analog Model of the Moog Ladder Filter"

		Original Implementation: D'Angelo, Valimaki

		FIXME: use fast_tanhf()
	*/

	// Thermal voltage (26 milliwats at room temperature)
	const float kVT = 0.312f;

	struct ImprovedMoogFilter
	{
		float m_cutoff;
		float m_resonance;
		float m_envInfl;

		float m_V[4];
		float m_dV[4];
		float m_tV[4];

		void Reset()
		{
			for (unsigned iPole = 0; iPole < 4; ++iPole)
				m_V[iPole] = m_dV[iPole] = m_tV[iPole] = 0.f;
		}

		void SetParameters(const FilterParameters &parameters)
		{
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
			SetEnvelopeInfluence(parameters.envInfl);
		}

		void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			const float omega = (kPI*value)/kSampleRate;
			m_cutoff = 4.f*kPI * kVT * value * (1.f-omega) / (1.f+omega);
		}

		void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = std::max<float>(kEpsilon, value*4.f);
		}

		void SetEnvelopeInfluence(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_envInfl = value;
		}

		void Apply(float *pSamples, unsigned numSamples, float globalWetness, unsigned sampleCount, ADSR &envelope);
	};
}
