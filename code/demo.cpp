
// cookiedough -- demo flow (640x480)

// def. for sync. replay (instead of edit) mode
// #define SYNC_PLAYER

#include "main.h"
#include <windows.h> // for audio.h
#include "../3rdparty/rocket/lib/sync.h"
// #include "demo.h"
#include "audio.h"
// #include "timer.h"

#include "shared.h"
#include "boxblur.h"

#include "ball.h"
#include "landscape.h"
#include "twister.h"
#include "heartquake.h"
// #include "polar.h"

static sync_device *s_hRocket;

#ifndef SYNC_PLAYER

static sync_cb s_rocketCallbacks = {
	Audio_Rocket_Pause,
	Audio_Rocket_SetRow,
	Audio_Rocket_IsPlaying
};

#endif // !SYNC_PLAYER

bool Demo_Create()
{
	s_hRocket = sync_create_device("sync");

#ifndef SYNC_PLAYER
//	sync_set_callbacks(s_hRocket, &s_rocketCallbacks, NULL);
	if (sync_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT) != 0)
	{
		SetLastError("Can not connect to GNU Rocket client.");
		return false;
	}
#endif // !SYNC_PLAYER

	return true;
}

void Demo_Destroy()
{
	if (s_hRocket != NULL)
		sync_destroy_device(s_hRocket);
}

void Demo_Draw(uint32_t *pDest, float sysTimer)
{
	VIZ_ASSERT(kResX == 640 && kResY == 480);

	unsigned int modOrder, modRow;
	float modRowAlpha;
	int rocketRow = Audio_Rocket_Sync(modOrder, modRow, modRowAlpha);

#ifndef SYNC_PLAYER
	if (sync_update(s_hRocket, rocketRow, &s_rocketCallbacks, nullptr))
		sync_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT);
#endif

//	Twister_Draw(pDest, sysTimer);
	Landscape_Draw(pDest, sysTimer);
//	Ball_Draw(pDest, sysTimer);

	MixSrc32(pDest, g_pDesireLogo3, 640*136);

	return;
}
