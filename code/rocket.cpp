
// cookiedough -- Rocket wrapper

#include "main.h"
#include "rocket.h"
#include "audio.h"

const char *kHost = "localhost";

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
	const sync_track *s_stopTrack;

	bool Launch()
	{
		s_hRocket = sync_create_device("sync/");

	#if !defined(SYNC_PLAYER)
		if (sync_tcp_connect(s_hRocket, kHost, SYNC_DEFAULT_PORT) != 0)
		{
			SetLastError("Can not connect to GNU Rocket client.");
			return false;
		}
	#endif

		// purely to indicate when it's time to exit
		// there are more elegant ways to do this but it worked in 'hot stuff' so it'll work now just as well
		s_stopTrack = AddTrack("demo:quit");

		return true;
	}

	void Land()
	{
	#if !defined(SYNC_PLAYER)
		// taken from TPB-06; this way the tracks saved to disk are always up to date
		sync_save_tracks(s_hRocket);
	#endif

		if (nullptr != s_hRocket)
			sync_destroy_device(s_hRocket);
	}

	double s_rocketRow = 0.0;

	bool Boost()
	{
		VIZ_ASSERT(s_hRocket != nullptr);

	#if defined(SYNC_PLAYER)
		if (!Audio_Rocket_IsPlaying(nullptr))
		{
			Audio_Start_Stream(0);
		}
	#endif

		unsigned int modOrder, modRow; 
		float modRowAlpha;
		s_rocketRow = Audio_Rocket_Sync(modOrder, modRow, modRowAlpha);

	#if !defined(SYNC_PLAYER)
		if (sync_update(s_hRocket, int(floor(s_rocketRow)), &s_rocketCallbacks, nullptr))
			sync_tcp_connect(s_hRocket, "localhost", SYNC_DEFAULT_PORT);
	#endif

		if (0.0 != get(s_stopTrack))
			return false;

		return true;
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
