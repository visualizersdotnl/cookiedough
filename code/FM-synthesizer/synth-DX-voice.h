
/*
	Syntherklaas FM - DX-style matrix voice.
*/

#pragma once

#include "synth-global.h"
#include "synth-oscillator.h"
#include "synth-ADSR.h"

namespace SFM
{
	// FIXME: move to synth-global.h?
	const unsigned kNumOperators = 4;

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
