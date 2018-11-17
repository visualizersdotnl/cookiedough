
/*
	Syntherklaas FM - DX-style voice.

	Missing:
		- Feedback
		- Specific envelope (probably need a simpler one) per operator
		- These loops are an impractical solution

	Rules:
		- An operator can only be modulated by an operator above it (index)
		- If flagged as carrier, they will be mixed at the end
		- ADSR et cetera have to be applied as well
*/

#pragma once

#include "synth-oscillator.h"
#include "synth-parameters.h"
// #include "synth-ADSR.h"
#include "synth-simple-filters.h"

namespace SFM
{
	// FIXME: move to synth-global.h?
	const unsigned kNumOperators = 6;

	// Initialized manually
	class DX_Voice
	{
	public:
		bool m_enabled;

		struct Operator
		{
			bool enabled;
			Oscillator oscillator;
			unsigned routing, feedback;
			bool isCarrier;
			float prevSample;
			
			// Slave operator means it is subject to variable modulation & lowpass
			bool isSlave;

			// So define these:
			float modAmount;
			LowpassFilter filter;

			SFM_INLINE void Reset()
			{
				enabled = false;
				oscillator = Oscillator();
				routing = -1;
				feedback = -1;
				isCarrier = false;
				prevSample = 0.f;
				isSlave = false;
			}

		} m_operators[kNumOperators];


		// Modulator ADSR
		ADSR m_modADSR;

		// Modulator LFO (vibrato)
		Oscillator m_modVibrato;

		// Global tremolo
		Oscillator m_AM;   
		
		// For wavetable samples
		bool m_oneShot;

		// For pulse-based waveforms
		float m_pulseWidth;

		// Filter instance for this particular voice
		LadderFilter *m_pFilter;
	
		DX_Voice() : m_enabled(false) {}

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
				m_operators[iOp].Reset();

			// Defensive (FIXME: trim down)
			m_modADSR.Reset();
			m_modVibrato = Oscillator();
			m_AM = Oscillator();
			m_oneShot = false;
			m_pulseWidth = 0.5f;
			m_pFilter = nullptr;
		}

		float Sample(const Parameters &parameters);

		SFM_INLINE void SetSlaveCutoff(float cutoff)
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
				if (true == m_operators[iOp].isSlave  && true == m_operators[iOp].enabled) 
					m_operators[iOp].filter.SetCutoff(cutoff);
		}

		SFM_INLINE void SetPitchBend(float bend)
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
				if (true == m_operators[iOp].enabled) 
					m_operators[iOp].oscillator.PitchBend(bend);
		}

		void SetSlaveLowpass(float amount)
		{
		}

		SFM_INLINE bool HasCycled() /* const */
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				Operator &opDX = m_operators[iOp];

				// Enabled and carrier?
				if (true == opDX.enabled && true == opDX.isCarrier)
				{
					// Not a slave carrier and has cycled?
					if (false == opDX.isSlave && true == opDX.oscillator.HasCycled())
					{
						return true;
					}
				}
			}

			return false;
		}
	};
}
