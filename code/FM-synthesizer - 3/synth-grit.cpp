
/*
	Syntherklaas FM: Grit distortion.

	This approach was devised to add dirt to the top end, taking velocity into account.
	It's use case is the Wurlitzer approximation.

	WIP!
*/

#include "synth-grit.h"

namespace SFM
{
	float Grit::Apply(float sample, float factor)
	{
		SFM_ASSERT(factor >= 0.f && factor <= 1.f);

		// Filter dry sample
		const float filtered = (float) m_filter.tick(sample);

		// Crush
		int iCrushed = int(filtered*65535.f);
		iCrushed &= ~31;
		float fCrushed = iCrushed/65535.f;
//		float fCrushed = filtered;

		// Blend result
		const float result = lerpf<float>(sample, Clamp(sample+fCrushed), factor);
		return result;
	}
}
