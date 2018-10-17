
/*
	Syntherklaas FM -- Cosine tilt envelope.
*/

#pragma once

namespace SFM
{

	/* 
		'tilt'      = Peak of the curve [-1..1]
		'curve'     = Steepness of curve (power).
		'frequency' = In hZ.

		When used as part of a modulator be sure to limit the frequency; going too high (above or close to Nyquist) will result in noise.
	*/
	void CalculateCosineTiltEnvelope(float *pDest, unsigned numSamples, float tilt, float curve, float frequency);
}
