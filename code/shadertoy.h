
// cookiedough -- simple Shadertoy ports & my own shaders

#pragma once

bool Shadertoy_Create();
void Shadertoy_Destroy();

// ready to tweak & use:
void Nautilus_Draw(uint32_t *pDest, float time, float delta);
void Laura_Draw(uint32_t *pDest, float time, float delta);
void Spikey_Draw(uint32_t *pDest, float time, float delta, bool close = true);

// need work but may be usable (you just need to convince Michiel):
void Sinuses_Draw(uint32_t *pDest, float time, float delta);

// not great and/or need work:
void Plasma_Draw(uint32_t *pDest, float time, float delta);
void Tunnel_Draw(uint32_t *pDest, float time, float delta);
