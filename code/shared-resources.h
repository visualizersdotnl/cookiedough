
// cookiedough -- shared resources (FX)

#pragma once

// linear grayscale gradients
constexpr unsigned kNumGradients = 256;
extern __m128i g_gradientUnp16[kNumGradients]; // unpacked to lower 16-bit
extern __m128i g_gradientUnp32[kNumGradients]; // unpacked 32-bit, multiplied by 65536

// render targets
constexpr unsigned kNumRenderTargets = 4;
extern uint32_t *g_renderTarget[kNumRenderTargets];

// Nytrik's Mexico logo
extern uint32_t *g_pNytrikMexico;

// Alien's things for TPB-02 Xbox
extern uint32_t *g_pXboxLogoTPB;

// Thorsten's world famous Toypusher disco character (128x128)
extern uint32_t *g_pToyPusherTiles[8];

// render target resolution (let us agree to keep it's aspect ratio identical to the output resolution)
constexpr size_t kTargetResX = kResX;
constexpr size_t kTargetResY = kResY;
constexpr size_t kTargetSize = kTargetResX*kTargetResY;
constexpr size_t kTargetBytes = kTargetSize*sizeof(uint32_t);

bool Shared_Create();
void Shared_Destroy();
