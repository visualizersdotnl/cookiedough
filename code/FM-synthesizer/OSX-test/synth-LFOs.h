
/*
	Syntherklaas FM -- LFOs.
*/

#ifndef _SFM_SYNTH_LFOS_H_
#define _SFM_SYNTH_LFOS_H_

namespace SFM
{

	/* 
		'tilt' = Peak of the curve [-1..1]
		'curve' = Curvature (power).
		'frequency' = In hZ.

		When used as part of a modulator be sure to limit the frequency; go too high and it'll just be noise.
	*/
	void CalcLFO_CosTilt(float *pDest, unsigned numSamples, float tilt, float curve, float frequency);
}

#endif // _SFM_SYNTH_LFOS_H_
