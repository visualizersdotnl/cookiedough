
// cookiedough -- shared resources (FX)

#pragma once

// linear grayscale gradient (unpacked)
extern __m128i g_gradientUnp[256];

// render target
extern uint32_t *g_renderTarget;

// render target resolution (let us agree to keep it's aspect ratio identical to the output resolution)
constexpr size_t kTargetResX = kResX;
constexpr size_t kTargetResY = kResY;
constexpr size_t kTargetSize = kTargetResX*kTargetResY;
constexpr size_t kTargetBytes = kTargetSize*sizeof(uint32_t);

bool Shared_Create();
void Shared_Destroy();
