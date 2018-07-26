
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
// - enabling /arch:SSE2 et cetera isn't very beneficial
// - use multi-byte character set (i.e. no _UNICODE)

// important to know:
// - always include main.h on top
// - there's kResX and soforth telling you about the size of the output buffer
// - for the render target(s) there's kTargetX et cetera
// - the delta time is in MS so it can be sensibly applied to for example gamepad axis values

// Undef. for (Windows only?) CRT leak check
// #define WIN32_CRT_LEAK_CHECK
#define WIN32_CRT_BREAK_ALLOC -1

#include "main.h" // always include first!
#include <windows.h>
#include "../3rdparty/SDL2-2.0.8/include/SDL.h"

#include "display.h"
#include "timer.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "gamepad.h"

#include "polar.h"

// effects
#include "torus-twister.h"
#include "landscape.h"
#include "ball.h"
#include "heartquake.h"
#include "tunnelscape.h"

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

// int SDL_main(int argc, char *argv[])
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
{
#if defined(_DEBUG) && defined(CRT_LEAK_CHECK)
	// Dump leak report at any possible exit.
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | 
	_CRTDBG_LEAK_CHECK_DF);
	
	// Report all to debug pane.
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

	if (-1 != WIN32_CRT_BREAK_ALLOC)
		_CrtSetBreakAlloc(WIN32_CRT_BREAK_ALLOC);
#endif

	if (0 != SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER))
	{
		MessageBox(NULL, SDL_GetError(), "Can't initialize SDL!", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	// change path to target root
	SetCurrentDirectoryA("../");

	// check for SSE4
	if (false == SDL_HasSSE41())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, "Processor does not support SSE 4.1 instructions.", nullptr);
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
	demoInit &= Tunnelscape_Create();

	Gamepad_Create();

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
					uint32_t* pDest = static_cast<uint32_t*>(mallocAligned(kOutputBytes, kCacheLine));
					memset32(pDest, 0, kOutputSize);

					float oldTime = 0.f, newTime = 0.f;
					while (true == HandleEvents())
					{
						oldTime = newTime;
						newTime = timer.Get();
						Demo_Draw(pDest, newTime, (newTime-oldTime)*100.f);
						display.Update(pDest);

					}

					freeAligned(pDest);
				}
			}
		}
	}

	Gamepad_Destroy();

	Audio_Destroy();
	Demo_Destroy();

	Twister_Destroy();
	Landscape_Destroy();
	Ball_Destroy();
	Heartquake_Destroy();
	Tunnelscape_Destroy();

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
