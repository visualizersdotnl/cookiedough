
/*
	Syntherklaas FM -- Vorticity LFO.

	This isn't a physically correct vortex shedding implement by a long shot; the goal
	is clearly to have a cool feature that sounds a bit like it.

	FIXME:
		- Implement parametrization
		- Do not let it control the full range, but let this happen over time!
		- Start on sustain?
*/

#ifndef _SFM_SYNTH_VORTICITY_H_
#define _SFM_SYNTH_VORTICITY_H_

#include "synth-global.h"
#include "synth-modulator.h"

namespace SFM
{
	const float kCommonStrouhal = 0.22f;

	struct Vorticity
	{
		FM_Modulator m_LFO;

		void Initialize()
		{
			// FIXME: derive from Strouhal constant, in turn derived from frequency, diameter and flow speed
			m_LFO.Initialize(1.f, 1.f+kCommonStrouhal, kPI*0.5f);
		}

		float Sample()
		{
			const float modulation = m_LFO.Sample(NULL);
			SFM_ASSERT(fabsf(modulation) <= 1.f);
			return modulation;
		}
	};
}

#endif // _SFM_SYNTH_VORTICITY_H_
