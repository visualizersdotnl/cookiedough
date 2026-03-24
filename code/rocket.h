
// cookiedough -- Rocket wrapper (sync. tool for demos, read up on it in /3rdparty)

#pragma once

#include "../3rdparty/rocket-stripped/lib/sync.h"

typedef const sync_track* SyncTrack;

namespace Rocket
{
	// these are hilarious aliases for Start(), Stop() and Update()
	bool Launch();
	void Land();
	bool Boost();

	// define a SyncTrack anywhere you like, register it here and it will show up in GNU Rocket
	const sync_track *AddTrack(const char *name);

	// from there on out you use these to grab the values
	double get(const sync_track *track);

	CKD_INLINE float getf(const sync_track *track) { 
		return (float) get(track); 
	}

	CKD_INLINE int geti(const sync_track *track) { 
		return int(roundf(getf(track)));
	}
}
