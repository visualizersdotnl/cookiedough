
/*
	Syntherklaas FM -- LFOs.
*/

#include "synth-global.h"
// #include "synth-cosine-tilt.h"

namespace SFM
{
	void CalculateCosineTiltEnvelope(float *pDest, unsigned numSamples, float tilt, float curve, float frequency)
	{
		/* Let's assert that what we're getting is in accordance to what I was promised. */
		SFM_ASSERT(tilt >= -1.f && tilt <= 1.f);
  
		/* Note to self: raising by zero results in 1, ergo: flat. */

		const size_t envelopeSize = numSamples;

		/* Calculate "middle" (tilt) & remainder. */
		tilt = 0.5f + tilt*0.5f;
		const size_t middle = (size_t) floorf(tilt*envelopeSize);
		const size_t remainder = envelopeSize-middle;

		/* Calculate period deltas. */
		const float period = frequency*k2PI;
		const float dMiddle = period/middle;
		const float dRemainder = period/remainder;

		float *pEnv = pDest;

		/* Build curve up to middle (first half of full period). */
		float theta = 0.f;
		for (unsigned iGrain = 0; iGrain < middle; ++iGrain)
		{
			const float cosine = 1.f - cosf(theta);
			*pEnv++ = powf(cosine, curve);
			theta += dMiddle;
		}

		/* Build remainder of the curve (second half of full period). */
		theta = 0.f;
		for (unsigned iGrain = 0; iGrain < remainder; ++iGrain)
		{
			const float cosine = cosf(theta);
			*pEnv++ = powf(cosine, curve);
			theta += dRemainder;
		}
	}
}
