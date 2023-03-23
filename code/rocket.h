
// cookiedough -- Rocket wrapper

#pragma once

#include "../3rdparty/rocket-stripped/lib/sync.h"

typedef const sync_track* SyncTrack;

namespace Rocket
{
	bool Launch();
	void Land();
	bool Boost();

	const sync_track *AddTrack(const char *name);
	
	double get(const sync_track *track);

	VIZ_INLINE float getf(const sync_track *track) 
	{ 
		return (float) get(track);     
	}

	VIZ_INLINE int geti(const sync_track *track) 
	{ 
		return int(roundf(getf(track))); 
	}
}
