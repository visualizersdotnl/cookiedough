
// cookiedough -- fast (co)sine (by or at least lifted from Kusma/Excess)

#include "main.h"

// constexpr unsigned kFastCosTabLog2Size = 10; // Equals size of 1024
// constexpr unsigned kFastCosTabSize = (1 << kFastCosTabLog2Size);

alignas(kAlignTo) double g_fastCosTab[kFastCosTabSize+1];

void InitializeFastCosine()
{
	for (unsigned iPhase = 0; iPhase < kFastCosTabSize+1; ++iPhase)
	{
		const double phase = double(iPhase) * ((k2PI/kFastCosTabSize));
		g_fastCosTab[iPhase] = cos(phase);
	}
}
