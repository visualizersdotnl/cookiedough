
/*
	Syntherklaas FM -- 1-pole filter(s).

	Useful information: http://www.earlevel.com/main/2012/12/15/a-one-pole-filter/
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	const float kLowpassDCBlockerCutoff = 10.f/kSampleRate;

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
		m_cutoff = -expf(-2.f*kPI*(0.5f-cutoff));
		m_gain = 1.f-m_cutoff;
	}

	SFM_INLINE float Apply(float input)
	{
		m_feedback = input*m_gain + m_feedback*m_cutoff;
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
