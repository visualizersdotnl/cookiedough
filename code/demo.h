
// cookiedough -- demo flow

#ifndef _DEMO_H_
#define _DEMO_H_

#include "../3rdparty/rocket-stripped/lib/sync.h"

bool Demo_Create();
void Demo_Destroy();
bool Demo_Draw(uint32_t *pDest, float time, float delta);

#endif // _DEMO_H_
