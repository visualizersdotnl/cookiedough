
// cookiedough (2011) -- lo-fi demoscene testbed
// property of visualizers.nl (http://www.visualizers.nl)

// third party:
// - GNU Rocket by Erik Faye-Lund & Egbert Teeselink (modified)
// - Developer's Image Library (DevIL)
// - BASSMOD audio library by Ian Luck
// - SDL 2.0

// compiler settings for Visual C++:
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision)
// - enabling /arch:SSE2 isn't very beneficial
// - use multi-byte character set (i.e. no _UNICODE)

// ignore:
#pragma warning(disable:4018) // signed <-> unsigned mismatch (PixelToaster)

#include "main.h" // always include first!
#include <windows.h>
#include "../3rdparty/SDL2-2.0.3/include/SDL.h"

#include "display.h"
#include "timer.h"
#include "image.h"
#include "shared.h"
#include "audio.h"
#include "polar.h"
#include "demo.h"

// effects
#include "twister.h"
#include "landscape.h"
#include "ball.h"
#include "heartquake.h"

// display config.
const char *kTitle = "cocktails at Kurt Bevacqua's";
const bool kFullScreen = false;

static std::string s_lastErr;

void SetLastError(const std::string &description)
{
	s_lastErr = description;
}

static bool HandleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			return false;

		case SDL_KEYDOWN:
		{
			const SDL_Keycode key = event.key.keysym.sym;
			if (key == SDLK_ESCAPE)
				return false; 
		}

		default:
			break;
		}
	}

	return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
{
	if (0 != SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER))
	{
		MessageBox(NULL, SDL_GetError(), "Can't initialize SDL!", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	// change path to target root
	SetCurrentDirectoryA("../");

	// check for SSE2
	if (false == SDL_HasSSE2())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, "Processor does not support SSE2 instructions.", nullptr);
		return 1;
	}

	bool demoInit = true;

	demoInit &= Image_Create();
	demoInit &= Shared_Create();
	demoInit &= Polar_Create();

	demoInit &= Twister_Create();
	demoInit &= Landscape_Create();
	demoInit &= Ball_Create();
	demoInit &= Heartquake_Create();

	if (demoInit)
	{
		if (Demo_Create())
		{
			if (Audio_Create(-1, "assets/moby_-_eliminator-tribute.mod", GetForegroundWindow())) // FIXME? (GetForegroundWindow())
			{
				Display display;
				if (display.Open(kTitle, kResX, kResY, kFullScreen))
				{
					Timer timer;

					// frame buffer
					std::unique_ptr<uint32_t> pixels(new uint32_t[kResX*kResY]);
					memset(pixels.get(), 0, kResX*kResY*4);

					while (true == HandleEvents())
					{
						Demo_Draw(pixels.get(), timer.Get());
						display.Update(pixels.get());
					}
				}
			}
		}
	}

	Audio_Destroy();
	Demo_Destroy();

	Twister_Destroy();
	Landscape_Destroy();
	Ball_Destroy();
	Heartquake_Destroy();

	Image_Destroy();
	Shared_Destroy();
	Polar_Destroy();

	SDL_Quit();

	if (false == s_lastErr.empty())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, s_lastErr.c_str(), nullptr);
		return 1;
	}
	
	return 0;
}
