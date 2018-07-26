
// cookiedough -- shared resources (FX)

#pragma once

extern uint32_t *g_pDesireLogo3; // 640x136 (alpha)

// linear grayscale gradient (unpacked)
extern __m128i g_gradientUnp[256];

// render target
extern uint32_t *g_renderTarget;

// render target resolution
const size_t kTargetResX = 800;
const size_t kTargetResY = 600;
constexpr size_t kTargetSize = kTargetResX*kTargetResY;
constexpr size_t kTargetBytes = kTargetSize*sizeof(uint32_t);

bool Shared_Create();
void Shared_Destroy();
