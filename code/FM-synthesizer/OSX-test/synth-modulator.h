
/*
	Syntherklaas FM -- Modulator (basically a type of LFO).
*/

#ifndef _SFM_SYNTH_MODULATOR_H_
#define _SFM_SYNTH_MODULATOR_H_

#include "synth-global.h"

namespace SFM
{
	/*
		FM modulator.
	*/

	struct FM_Modulator
	{
		float m_index;
		float m_pitch;
		unsigned m_sampleOffs;
		float m_phaseShift;

		void Initialize(float index, float frequency, float phaseShift /* In radians */);
		float Sample(const float *pEnv);
	};
}

#endif // _SFM_SYNTH_MODULATOR_H_

