
/*
	Syntherklaas FM - DX-style matrix voice.

	Step by step implementation:
		- Copy voice spawn function and modify it to create on of these.
		- Sample this function.
		- Complete all 3 algorithms in FM_BISON.
		- Try Yamaha DX7's algorithm 5.
*/

#pragma once

#include "synth-global.h"
#include "synth-oscillator.h"
#include "synth-ADSR.h"

namespace SFM
{
	// FIXME: move to synth-global.h?
	const unsigned kNumOperators = 6;

	// Initialized manually
	class DX_Voice
	{
	public:
		bool m_enabled;
		Oscillator m_operators[kNumOperators];
		unsigned m_routing[kNumOperators];
		bool m_isCarrier[kNumOperators];

//	public:
		DX_Voice() : m_enabled(false) {}

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{
				m_operators[iOp].Initialize(kCosine, 0.f, 1.f);
				m_routing[iOp] = -1;
				m_isCarrier[iOp] = false;
			}
		}

		float Sample()
		{
			SFM_ASSERT(true == m_enabled);

			/*
				- 1. Sample all operators, top down.
				- 2. Make sure all carriers are mixed into the output signal.
				- 3. Do as always and factor in ADSR, tremolo, noise et cetera.
			*/

			return 0.f;
		}
	};
}
