
/*
	Syntherklaas FM -- LFOs.
*/

#ifndef _SFM_SYNTH_LFOS_H_
#define _SFM_SYNTH_LFOS_H_

namespace SFM
{

	/* 
		Based on the document in Koen's prototyping folder (from the GR-1 project).
	
		Values between [-1..1]

		'tilt' = Peak of the curve.
		'curve' = Curvature (power).
		'frequency' = In hZ.

		When used as part of a modulator be sure to limit the frequency; go too high and it'll just be odd noise.
	*/
	static void CalcLFO_CosTilt(float *pDest, unsigned numSamples, float tilt, float curve, float frequency);
}

#endif // _SFM_SYNTH_LFOS_H_