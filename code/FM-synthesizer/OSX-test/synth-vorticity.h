
/*
	Syntherklaas FM -- Vorticity LFO.

	This isn't a physically correct vortex shedding implement by a long shot; the goal
	is clearly to have a cool feature that sounds a bit like it.

	FIXME:
		- Implement parametrization using Strouhal
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
		unsigned m_sampleOffs;
		float m_pitchShift;

		void Initialize(unsigned sampleOffs);
		float Sample();
	};
}

#endif // _SFM_SYNTH_VORTICITY_H_
