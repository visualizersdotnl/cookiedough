
// cookiedough -- basic gamepad support (uses the first one detected)

#include "main.h"
#include "../3rdparty/SDL2-2.0.8/include/SDL.h"

static SDL_GameController *s_pPad = nullptr;

void Gamepad_Create()
{
	int nJoysticks = SDL_NumJoysticks();
	for (int iPad = 0; iPad < nJoysticks; ++iPad) 
	{
		if (SDL_IsGameController(iPad)) 
		{
			s_pPad = SDL_GameControllerOpen(iPad);
			break;
		}
	}
}

void Gamepad_Destroy()
{
	if (nullptr != s_pPad)
	{
		SDL_GameControllerClose(s_pPad);
	}
}

inline float ClampAxisDeadzone(int input)
{
	// as described in SDL_gamecontroller.h (version 2.0.8)
	const int docDeadZone = 8000;

	return (input > docDeadZone || input < -docDeadZone) ? input / (float) SDL_JOYSTICK_AXIS_MAX : 0.f;
}

bool Gamepad_Update(float &leftX, float &leftY, float &rightX, float &rightY)
{
	if (nullptr != s_pPad)
	{
		SDL_GameControllerUpdate();

		if (SDL_FALSE == SDL_GameControllerGetAttached(s_pPad))
		{
			Gamepad_Destroy();
			return false;
		}

		// FIXME: you could check if the controller has these but come on, it's 2018
		int iLeftX = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX);
		int iLeftY = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY);
		int iRightX = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX);
		int iRightY = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY);

		leftX = ClampAxisDeadzone(iLeftX);
		leftY = ClampAxisDeadzone(iLeftY);
		rightX = ClampAxisDeadzone(iRightX);
		rightY = ClampAxisDeadzone(iRightY);

		return true;
	}

	// small courtesy so you don't really have to check
	leftX = leftY = 0.f;
	rightX = rightY = 0.f;

	return false;
}
