
// cookiedough -- shared resources (FX)

#pragma once

// linear grayscale gradients
constexpr unsigned kNumGradients = 256;
extern __m128i g_gradientUnp16[kNumGradients]; // unpacked to lower 16-bit

// render targets
constexpr unsigned kNumRenderTargets = 4;
extern uint32_t *g_renderTarget[kNumRenderTargets];

// FIXME: move thsese images to demo implementation!
extern uint32_t *g_pNytrikMexico; // Nytrik's Mexico logo
extern uint32_t *g_pXboxLogoTPB;  // Alien's things for TPB-02 Xbox

// render target resolution (let us agree to keep it's aspect ratio identical to the output resolution)
constexpr size_t kTargetResX = kResX;
constexpr size_t kTargetResY = kResY;
constexpr size_t kTargetSize = kTargetResX*kTargetResY;
constexpr size_t kTargetBytes = kTargetSize*sizeof(uint32_t);

bool Shared_Create();
void Shared_Destroy();
