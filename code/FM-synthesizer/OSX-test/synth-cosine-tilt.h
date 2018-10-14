
/*
	Syntherklaas FM -- Cosine tilt envelope.
*/

#ifndef _SFM_SYNTH_COS_TILT_H_
#define _SFM_SYNTH_COS_TILT_H_

namespace SFM
{

	/* 
		'tilt' = Peak of the curve [-1..1]
		'curve' = Curvature (power).
		'frequency' = In hZ.

		When used as part of a modulator be sure to limit the frequency; go too high (aboe or close to Nyquist)
		will result in noise.
	*/
	void CalculateCosineTiltEnvelope(float *pDest, unsigned numSamples, float tilt, float curve, float frequency);
}

#endif // _SFM_SYNTH_COS_TILT_H_
