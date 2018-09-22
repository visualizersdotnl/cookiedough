
/*
	Syntherklaas -- FM synthesizer prototype.
	Two basic envelope calculators taken from GR-1 (by Tasty Chips) prototyping code (written by me, though).

	FIXME:
	- Fix CalcEnv_AD(): pure attack goes to sustain directly, et cetera (check Alex' notes), also add state (might be more appropriate somewhere else).
	- Calculate envelopes for a multiple of periods?
	- Check cosine envelope.
*/

#pragma once

/*
	Linear attack, curved decay, remain at sustain.

	'attack'  - Part of the curve [0..1] that is the linear attack.
	'decay'   - Part of the curve [0..1] that is the curved decay.
	'sustain' - Amplitude to sustain at.
*/

static void CalcEnv_AD(float *pDest, float attack, float decay, float sustain = 0.5f)
{
	saturatef(attack);
	saturatef(decay);

    const size_t envelopeSize = kSampleRate; // FIXME

    /* Pick what's needed for full attack (priority). */
    const size_t attackSize = (size_t) floorf(envelopeSize*attack);

    /* Take what's left for decay. */
    const size_t decaySize = std::min<size_t>(envelopeSize-attackSize, (size_t) floorf(decay * (envelopeSize)));

    const size_t adSize = attackSize+decaySize;
    VIZ_ASSERT(adSize == envelopeSize);
	// FIXME: deal with remainder if need be.

    /* Attack delta. */
    const float dA = (attackSize > 0) ? 1.f/attackSize : 1.f;

    /* Decay delta. */
    const float dD = (decaySize > 0) ? 1.f/decaySize : 1.f;

    float *pEnv = pDest;
    float curAmp = 0.f;

    /* Build attack up fully in it's number of frames. */
    for (unsigned iGrain = 0; iGrain < attackSize; ++iGrain)
    {
        *pEnv++ = curAmp;
        curAmp += dA;
    }

    /* Clamp up to full attack. */
    curAmp = 1.f;

	/* Decay (let's get a little creative here and use Ken Perlin's smoothstep function to curve it). */
	float delta = 0.f;
    for (unsigned iGrain = 0; iGrain < decaySize; ++iGrain)
    {
		*pEnv++ = smoothstepf(curAmp, sustain, delta);
		delta += dD;
    }
}

/* 
	Based on the document in Koen's prototyping folder. 

	'tilt' = Peak of the curve.
	'curve' = Curvature (power).
	'frequency' = In hZ;
*/
static void CalcEnv_Cosine(float *pDest, float tilt, float curve, float frequency)
{
    /* Let's assert that what we're getting is in accordance to what I was promised. */
	VIZ_ASSERT(tilt >= -1.f && tilt <= 1.f);
  
    /* Note to self: raising by zero results in 1, ergo: flat. */

    const size_t envelopeSize = kSampleRate; // FIXME

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
