
// cookiedough -- fast (co)sine

#pragma once

void InitializeFastCosine();

// Period [0..1]
float fastcosf(double x);
VIZ_INLINE float fastsinf(double x) { return fastcosf(x-0.25); }
