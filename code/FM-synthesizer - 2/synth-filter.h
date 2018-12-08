
/*
	Syntherklaas FM -- Master (voice) filters.

	I'm no expert on filters such as these yet, so I took a few from the public domain
	and adapted them for use; all have their own distinct sound but if you're looking for
	a clean 24dB filter, the Krajeski MoogVCF implementation works like a charm
*/

#pragma once

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
	};

	/*
		Interface (base class).

		Notes:
			- Reset() needs to be called per voice
			- Start() must be called per voice
			- SetLiveParameters() can be called whilst rendering
			- Stop() when a voice ending (optional)
	*/

	class LadderFilter
	{
	public:
		LadderFilter() :
			m_drive(1.f)
		{}
		
		virtual ~LadderFilter() {}

		void Start(const ADSR::Parameters &parameters, float velocity)
		{
			m_ADSR.Start(parameters, velocity);
		}

		void Stop(float velocity)
		{
			m_ADSR.Stop(velocity);
		}

		virtual void SetLiveParameters(const FilterParameters &parameters)
		{
			SetDrive(parameters.drive);	
			SetCutoff(parameters.cutoff);
			SetResonance(parameters.resonance);
		}

		virtual void Reset() = 0;
		virtual void Apply(float *pSamples, unsigned numSamples, float contour, bool invert) = 0;

	protected:
		float m_drive;
		ADSR m_ADSR;

		void SetDrive(float value)
		{
			SFM_ASSERT(value >= 0.f);
			m_drive = value;
		}

		virtual void SetCutoff(float value) = 0;
		virtual void SetResonance(float value) = 0;
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
	*/

	// Thermal voltage (26 milliwats at room temperature)
	const double kVT = 0.312;

	class ImprovedMOOGFilter : public LadderFilter
	{
	private:
		double m_cutoff;
		double m_resonance;

		double m_V[4];
		double m_dV[4];
		double m_tV[4];

		virtual void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			const double omega = (kPI*value)/kSampleRate;
			m_cutoff = 4.0*kPI * kVT * value * (1.0-omega) / (1.0+omega);
		}

		virtual void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = std::max<double>(kEpsilon, value*4.0);
		}

	public:
		virtual void Reset()
		{
			for (unsigned iPole = 0; iPole < 4; ++iPole)
				m_V[iPole] = m_dV[iPole] = m_tV[iPole] = 0.0;
		}

		virtual void Apply(float *pSamples, unsigned numSamples, float contour, bool invert);
	};

	/*
		Transistor ladder filter by Teemu Voipio
		Source: https://www.kvraudio.com/forum/viewtopic.php?t=349859
	*/

	class TeemuFilter : public LadderFilter
	{
	private:
		double m_cutoff;
		double m_resonance;

		double m_inputDelay;
		double m_state[4];

		virtual void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			value *= 1000.f;
			value = value*2.f*kPI/kSampleRate;
			m_cutoff = value; // Also known as omega
		}

		virtual void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = value;
		}

	public:
		virtual void Reset()
		{
			m_inputDelay = 0.0;
			m_state[0] = m_state[1] = m_state[2] = m_state[3] = 0.0;
		}

		virtual void Apply(float *pSamples, unsigned numSamples, float contour, bool invert);
	};

	/*
		Krajeski's MoogVCF implementation 

		Sources: 
		+ http://song-swap.com/MUMT618/aaron/Presentation/demo.html
		+ https://github.com/ddiakopoulos/MoogLadders/blob/master/src/KrajeskiModel.h
	*/

	class KrajeskiFilter : public LadderFilter
	{
	private:
		double m_resonance;
		double m_cutoff;

		double m_state[5];
		double m_delay[5];

		// Kept as-is:
		double wc;     // The angular frequency of the cutoff
		double g;      // A derived parameter for the cutoff frequency
		double gRes;   // A similar derived parameter for resonance.
		double gComp;  // Compensation factor

		virtual void SetCutoff(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_cutoff = value*1000.f;
			wc = 2 * kPI * m_cutoff / kSampleRate;
			g = 0.9892 * wc - 0.4342 * pow(wc, 2) + 0.1381 * pow(wc, 3) - 0.0202 * pow(wc, 4);
		}

		virtual void SetResonance(float value)
		{
			SFM_ASSERT(value >= 0.f && value <= 1.f);
			m_resonance = value;
			gRes = m_resonance * (1.0029 + 0.0526 * wc - 0.926 * pow(wc, 2) + 0.0218 * pow(wc, 3));	
		}

	public:
		virtual void Reset()
		{
			// FIXME: interesting as parameter?
			gComp = 1.0;

			memset(m_state, 0, sizeof(m_state));
			memset(m_delay, 0, sizeof(m_delay));
		}

		virtual void Apply(float *pSamples, unsigned numSamples, float contour, bool invert);
	};
}

