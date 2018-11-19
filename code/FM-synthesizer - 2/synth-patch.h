
/*
	Syntherklaas FM -- Instrument patch.
*/

#pragma once

namespace SFM
{
	struct Patch
	{
		struct Operator
		{
			// Related to g_modRatioLUT (synth-LUT.cpp)
			unsigned coarse;

			float amplitude;
		};
	
		Operator operators[kNumOperators];

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{	
				Operator &OP = operators[iOp];
				OP.coarse = 0;
				OP.amplitude = 1.f;
			}
		}
	};
}