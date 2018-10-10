
/*
	Syntherklaas FM -- Vorticity.
*/

#ifndef _SFM_SYNTH_VORTICITY_H_
#define _SFM_SYNTH_VORTICITY_H_

namespace SFM
{
	SFM_INLINE float Vorticize(float sample, float phase)
	{
		// FIXME: implement (will have to tinker a bit, has never been done!)

		// https://intelligentsoundengineering.wordpress.com/2016/05/19/real-time-synthesis-of-an-aeolian-tone/
		// https://sourceforge.net/p/zynaddsubfx/code/ci/master/tree/src/Effects/

		// Idea:
		// - Get some kind of 'wahwah' effect running ramping up quick then decaying in some relation
		//   to the Strouhal number and combine these parameters into 1 function/

		return sample;
	}
};

#endif // _SFM_SYNTH_VORTICITY_H_
