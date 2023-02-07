
// cookiedough -- simple Shadertoy ports & my own shaders

#pragma once

bool Shadertoy_Create();
void Shadertoy_Destroy();

// ready to tweak & use:
void Nautilus_Draw(uint32_t *pDest, float time, float delta);
void Spikey_Draw(uint32_t *pDest, float time, float delta, bool close = true);
void Sinuses_Draw(uint32_t *pDest, float time, float delta);
void Laura_Draw(uint32_t *pDest, float time, float delta);
void Plasma_Draw(uint32_t *pDest, float time, float delta);

// need work:
void Tunnel_Draw(uint32_t *pDest, float time, float delta);
