
// cookiedough -- simple Shadertoy ports & my own shaders

#pragma once

bool Shadertoy_Create();
void Shadertoy_Destroy();

void Plasma_Draw(uint32_t *pDest, float time, float delta);
void Nautilus_Draw(uint32_t *pDest, float time, float delta);
void Laura_Draw(uint32_t *pDest, float time, float delta);
void Spikey_Draw(uint32_t *pDest, float time, float delta);
