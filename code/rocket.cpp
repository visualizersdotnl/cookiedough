
// cookiedough -- Rocket wrapper

// def. for sync. replay (instead of edit) mode
// FIXME: untested
// #define SYNC_PLAYER

#include "main.h"
#include "rocket.h"
#include <Windows.h> // for audio.h
#include "audio.h"

const char *kHost = "localhost";
// const char *kHost = "192.168.87.138";

static sync_device *s_hRocket = nullptr;

#if !defined(SYNC_PLAYER)

static sync_cb s_rocketCallbacks = {
	Audio_Rocket_Pause,
	Audio_Rocket_SetRow,
	Audio_Rocket_IsPlaying
};

#endif // !SYNC_PLAYER

namespace Rocket
{
	bool Launch()
	{
		s_hRocket = sync_create_device("sync"); // FIXME

	#ifndef SYNC_PLAYER
		if (sync_tcp_connect(s_hRocket, kHost, SYNC_DEFAULT_PORT) != 0)
		{
			SetLastError("Can not connect to GNU Rocket client.");
			return false;
		}
	#endif // !SYNC_PLAYER

		return true;
	}

	void Land()
	{
		if (nullptr != s_hRocket)
			sync_destroy_device(s_hRocket);
	}

	double s_rocketRow = 0.0;

	void Boost()
	{
		VIZ_ASSERT(s_hRocket != nullptr);

		unsigned int modOrder, modRow; 
		float modRowAlpha;
		s_rocketRow = Audio_Rocket_Sync(modOrder, modRow, modRowAlpha);

	#ifndef SYNC_PLAYER
		if (sync_update(s_hRocket, int(floor(s_rocketRow)), &s_rocketCallbacks, nullptr))
			sync_tcp_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT);
	#endif
	}

	const sync_track *AddTrack(const char *name)
	{
		return sync_get_track(s_hRocket, name);
	}

	double get(const sync_track *track)
	{
		return sync_get_val(track, s_rocketRow);
	}
}
