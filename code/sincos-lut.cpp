
// cookiedough -- LUT sinus/cosinus (interpolated)

#include "main.h"
// #include "sincos-lut.h"

alignas(kAlignTo) float g_cosLUT[kCosTabSize+1];

void CalculateCosLUT()
{
	for (unsigned iCos = 0; iCos < kCosTabSize; ++iCos)
		g_cosLUT[iCos] = cosf(float(iCos)*(k2PI/kCosTabSize));

	// close loop (guard)
	g_cosLUT[kCosTabSize] = g_cosLUT[0];
}
