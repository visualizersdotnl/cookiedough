
// cookiedough -- basic gamepad support (uses the first one detected)

#pragma once

void Gamepad_Create();
void Gamepad_Destroy();

// call this yourself on demand to poll the L+R analog sticks (values range from -1 to 1, deadzone is taken care of)
// returns false if there's no input available
bool Gamepad_Update(float &leftX, float &leftY, float &rightX, float &rightY);
