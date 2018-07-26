
// cookiedough -- *very* basic gamepad support (uses the first device detected)

#pragma once

void Gamepad_Create();
void Gamepad_Destroy();

// call this on demand to poll the left and right analog sticks and shoulder buttons
// - returns false (and zeroes) if there's nothing available, otherwise between -1 and 1 with dead zone taken care of
// - when used for movement, be sure to scale by system timer delta

struct PadState
{
	// axis sticks
	float leftX, leftY;
	float rightX, rightY;

	// buttons (binary)
	int lShoulder, rShoulder;
};

bool Gamepad_Update(PadState &state);
