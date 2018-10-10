
/*
	Syntherklaas FM -- Vorticity LFO.

	Some resources:
		- https://intelligentsoundengineering.wordpress.com/2016/05/19/real-time-synthesis-of-an-aeolian-tone/
		- http://www.mate.tue.nl/mate/pdfs/8998.pdf

	I'll have to get a little creative here.
*/

#ifndef _SFM_SYNTH_VORTICITY_H_
#define _SFM_SYNTH_VORTICITY_H_

#include "synth-global.h"
#include "synth-modulator.h"

namespace SFM
{
	struct Vorticity
	{
		FM_Modulator m_LFO; // Modulator type can be used as LFO

		void Initialize()
		{
			m_LFO.Initialize(1.f, 4.f);
		}

		float Sample()
		{
			return m_LFO.Sample();
		}
	}
};

#endif // _SFM_SYNTH_VORTICITY_H_
