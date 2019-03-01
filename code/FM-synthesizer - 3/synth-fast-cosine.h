
/*
	Syntherklaas FM: fast (co)sine.

	Taken from Logicoma's WaveSabre, provided by Erik 'Kusma' Faye-Lund.
*/

#pragma once

namespace SFM
{
	void InitializeFastCos();

	// Period [0..1]
	float FastCos(double x);
	SFM_INLINE float FastSin(double x) { return FastCos(x-0.25); }
};
