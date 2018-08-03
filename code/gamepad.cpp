
// cookiedough -- *very* basic gamepad support (uses the first device detected)

#include "main.h"
#include "../3rdparty/SDL2-2.0.8/include/SDL.h"
#include "gamepad.h"

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

bool Gamepad_Update(PadState &state)
{
	// small courtesy so you don't really have to check
	memset(&state, 0, sizeof(PadState));

	if (nullptr != s_pPad)
	{
		SDL_GameControllerUpdate();

		if (SDL_FALSE == SDL_GameControllerGetAttached(s_pPad))
		{
			Gamepad_Destroy();
			return false;
		}

		int iLeftX = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX);
		int iLeftY = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY);
		int iRightX = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX);
		int iRightY = SDL_GameControllerGetAxis(s_pPad, SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY);

		const float delta = 1.f;
		state.leftX  = delta*ClampAxisDeadzone(iLeftX);
		state.leftY  = delta*ClampAxisDeadzone(iLeftY);
		state.rightX = delta*ClampAxisDeadzone(iRightX);
		state.rightY = delta*ClampAxisDeadzone(iRightY);

		state.lShoulder = SDL_GameControllerGetButton(s_pPad, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		state.rShoulder = SDL_GameControllerGetButton(s_pPad, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

		// with PS4 on Win10 it's binary, but let's also make it so for any other pad
		state.lShoulder = std::min(state.lShoulder, 1);
		state.rShoulder = std::min(state.rShoulder, 1);

		return true;
	}
	else
	{
		static int numCalls = 0;
		if (++numCalls > 60*4) // let us check every 4 frames (assuming we're in candyland and running 60FPS)
		{
			Gamepad_Create();
			numCalls = 0;
		}
	}

	return false;
}

