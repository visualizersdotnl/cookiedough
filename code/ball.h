
// cookiedough -- voxel ball (2-pass approach)

#ifndef _BALL_H_
#define _BALL_H_

bool Ball_Create();
void Ball_Destroy();
void Ball_Draw(uint32_t *pDest, float time, float delta);

uint32_t *Ball_GetBackground();

#endif // _BALL_H_
