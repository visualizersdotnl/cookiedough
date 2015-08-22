
// cookiedough -- shared resources (FX)

#ifndef _SHARED_H_
#define _SHARED_H_

extern uint32_t *g_pDesireLogo3; // 640x136 (alpha)

// linear grayscale gradient (unpacked)
extern __m128i g_gradientUnp[256];

// 640x480 render target
extern uint32_t *g_renderTarget;

bool Shared_Create();
void Shared_Destroy();

#endif // _SHARED_H_
