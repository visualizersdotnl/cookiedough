
/*
	Syntherklaas FM -- Vorticity LFO.

	This isn't a physically correct vortex shedding implementation by a long shot; the goal
	is clearly to have a cool feature that sounds a bit like it but isn't special at all.

	It's called branding.
*/

#ifndef _SFM_SYNTH_VORTICITY_H_
#define _SFM_SYNTH_VORTICITY_H_

#include "synth-global.h"
#include "synth-modulator.h"

namespace SFM
{
	const float kCommonStrouhal = 0.22f*10.f;

	struct Vorticity
	{
		unsigned m_sampleOffs;
		float m_acceleration;

		float m_pitch;
		float m_pitchMod;
		float m_wetness;

		void Initialize(unsigned sampleOffs, float acceleration, float wetness);
		void SetStrouhal(float sheddingFreq, float diameter, float velocity);
		float Sample(float input);
	};
}

#endif // _SFM_SYNTH_VORTICITY_H_
