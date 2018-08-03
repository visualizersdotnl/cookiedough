
// cookiedough -- demo flow for: <?> by Bypass (Shader Superfights Grand Prix 2019 commercial)

// def. for sync. replay (instead of edit) mode
// #define SYNC_PLAYER

#include "main.h"
#include <windows.h> // for audio.h
#include "../3rdparty/rocket-stripped/lib/sync.h"
// #include "demo.h"
#include "audio.h"

// filters & blitters
#include "boxblur.h"
#include "polar.h"

// effects
#include "ball.h"
#include "landscape.h"
#include "torus-twister.h"
#include "heartquake.h"
#include "tunnelscape.h"
#include "shadertoy.h"

static sync_device *s_hRocket;

#if !defined(SYNC_PLAYER)

static sync_cb s_rocketCallbacks = {
	Audio_Rocket_Pause,
	Audio_Rocket_SetRow,
	Audio_Rocket_IsPlaying
};

#endif // !SYNC_PLAYER

static const sync_track *s_partTrack;

bool Demo_Create()
{
	s_hRocket = sync_create_device("sync");

#ifndef SYNC_PLAYER
//	sync_set_callbacks(s_hRocket, &s_rocketCallbacks, NULL);
	if (sync_tcp_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT) != 0)
	{
		SetLastError("Can not connect to GNU Rocket client.");
		return false;
	}
#endif // !SYNC_PLAYER

	bool fxInit = true;
	fxInit &= Twister_Create();
	fxInit &= Landscape_Create();
	fxInit &= Ball_Create();
	fxInit &= Heartquake_Create();
	fxInit &= Tunnelscape_Create();
	fxInit &= Shadertoy_Create();

	s_partTrack = sync_get_track(s_hRocket, "part");

	return fxInit;
}

void Demo_Destroy()
{
	if (s_hRocket != NULL)
		sync_destroy_device(s_hRocket);

	Twister_Destroy();
	Landscape_Destroy();
	Ball_Destroy();
	Heartquake_Destroy();
	Tunnelscape_Destroy();
	Shadertoy_Destroy();
}

void Demo_Draw(uint32_t *pDest, float timer, float delta)
{
	// for this production (FIXME)
	// VIZ_ASSERT(kResX == 640 && kResY == 480);

	unsigned int modOrder, modRow;
	float modRowAlpha;
	int rocketRow = Audio_Rocket_Sync(modOrder, modRow, modRowAlpha);

#ifndef SYNC_PLAYER
	if (sync_update(s_hRocket, rocketRow, &s_rocketCallbacks, nullptr))
		sync_tcp_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT);
#endif

	double part = sync_get_val(s_partTrack, (double) rocketRow);

	if (part == 1.0)
		Twister_Draw(pDest, timer, delta);
	
	if (part == 2.0)
		Landscape_Draw(pDest, timer, delta);
	
	if (part == 3.0)
		Ball_Draw(pDest, timer, delta);
	
	if (part == 4.0)
		Tunnelscape_Draw(pDest, timer, delta);
	
	if (part == 5.0)
		Plasma_Draw(pDest, timer, delta);
	
	if (part == 6.0)
		Nautilus_Draw(pDest, timer, delta);
	
	if (part == 7.0)
		Laura_Draw(pDest, timer, delta);
	
	if (part == 8.0)
		Spikey_Draw(pDest, timer, delta, true);
	
	if (part == 9.0)
		Spikey_Draw(pDest, timer, delta, false);

	// blit logo 
	uint32_t *pWrite = pDest + kResX*(kResY-145);
	for (int iY = 0; iY < 136; ++iY)
	{
		pWrite += (kResX-640)/2;
		MixSrc32(pWrite, g_pDesireLogo3 + iY*640, 640);
		pWrite += kResX - (kResX-640)/2;
	}

	return;
}
