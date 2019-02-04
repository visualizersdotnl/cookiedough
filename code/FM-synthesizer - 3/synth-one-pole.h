
/*
	Syntherklaas FM -- One pole filters, useful for basic oscillator or signal filtering.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	const float kLowpassDCBlockerCutoff = 10.f/kSampleRate;

	/* Lowpass (1-pole) */

	class LowpassFilter
	{
	public:
		LowpassFilter() :
			m_gain(1.f)
,			m_cutoff(0.f)
,			m_feedback(0.f)
	{}

	void Reset()
	{
		m_feedback = 0.f;
	}

	void SetCutoff(float cutoff)
	{
		SFM_ASSERT(cutoff >= 0.f && cutoff <= 1.f);
		m_cutoff = expf(-2.f*kPI*cutoff);
		m_gain = 1.f-m_cutoff;
	}

	SFM_INLINE float Apply(float input)
	{
		m_feedback = input*m_gain + m_feedback*m_cutoff;
		m_feedback *= kLeakyFactor;
		return m_feedback;
	}

	SFM_INLINE float GetValue() const 
	{
		return m_feedback;
	}

	private:
		float m_gain;
		float m_cutoff;

		float m_feedback;
	};

	/* Highpass (1-pole) */

	class HighpassFilter
	{
	public:
		HighpassFilter() :
			m_gain(1.f)
,			m_cutoff(0.f)
,			m_feedback(0.f)
	{}

	void Reset()
	{
		m_feedback = 0.f;
	}

	void SetCutoff(float cutoff)
	{
		SFM_ASSERT(cutoff >= 0.f && cutoff <= 1.f);
		m_cutoff = -expf(-2.f*kPI*(1.f-cutoff));
		m_gain = 1.f+m_cutoff;
	}

	SFM_INLINE float Apply(float input)
	{
		m_feedback = input*m_gain + m_feedback*m_cutoff;
		m_feedback *= 0.995f;
		return m_feedback;
	}

	SFM_INLINE float GetValue() const 
	{
		return m_feedback;
	}

	private:
		float m_gain;
		float m_cutoff;

		float m_feedback;
	};
}
                                           