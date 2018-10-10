
/*
	Syntherklaas FM -- Vorticity LFO.

	This isn't a physically correct vortex shedding implement by a long shot; the goal
	is clearly to have a cool feature that sounds a bit like it.

	FIXME:
		- Feed SetStrouhal
		- Use acceleration to blend in noise
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
		unsigned m_sampleOffs;
		float m_depth;
		float m_acceleration;

		float m_pitch;
		float m_pitchShift;
		float m_wetness;

		void Initialize(unsigned sampleOffs, float depth, float acceleration);
		void SetStrouhal(float sheddingFreq, float diameter, float velocity);
		float Sample(float input);
	};
}

#endif // _SFM_SYNTH_VORTICITY_H_
