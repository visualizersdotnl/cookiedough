
// cookiedough -- voxel balls (2-pass approach)

#ifndef _BALL_H_
#define _BALL_H_

bool Ball_Create();
void Ball_Destroy();
void Ball_Draw(uint32_t *pDest, float time, float delta);

// helper for sync. + resource share
uint32_t *Ball_GetBackground();
bool Ball_HasBeams();

#endif // _BALL_H_
