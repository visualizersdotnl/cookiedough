
// cookiedough -- Rocket wrapper

#pragma once

#include "../3rdparty/rocket-stripped/lib/sync.h"

// outside of namespace for convenience
typedef const sync_track* Track;

namespace Rocket
{
	bool Launch();
	void Land();
	void Boost();

	Track AddTrack(const char *name);
	double GetValue(Track track);
}
