
/*
	-÷---- - -÷- -  -   -                              - --÷-- - ----÷-
	:                                                             ·:¦
	.               ______      ___  ____  ____  ____               :
				_/_  \_ \/\__/   \_\_  \/  __\/  __\
				/  / _/  /  /  /  / __  /\__ \/\__ \
				/  _  /  /  /  ___/  /  /  /  /  /  /
				¯\___/¯\_  /¯\/   ¯\___/¯\___/¯\___/zS!
	:                  /__/                                         .
	¦:.                                                             :
	-÷--- -             -   -  - -- --÷-- - ---÷-- -  -   -      - --÷-
	      'cocktails with Kurt Bevacqua' retrosexual demosystem          
*/

// cookiedough (2009-2018) -- lo-fi demoscene testbed
// property of visualizers.nl (http://www.visualizers.nl)

// third party:
// - GNU Rocket by Erik Faye-Lund & contributors (last updated 27/07/2018)
// - Developer's Image Library (DevIL)
// - BASS audio library by Ian Luck
// - SDL 2.0.8

// compiler settings for Visual C++:
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision)
// - use multi-byte character set (i.e. no _UNICODE)

// important to know:
// - executables are built to target/x86/ or target/x64/ -- run from that directory!
// - keep DLLs (see above) up to date for each build
// - (almost) always include main.h on top
// - there's kResX/kResY and soforth telling you about the size of the output buffer
// - for the render target(s) there's kTargetX et cetera
// - the delta time is in MS so it can be sensibly applied to for example gamepad axis values

// Undef. for (Windows only?) CRT leak check
// #define WIN32_CRT_LEAK_CHECK
#define WIN32_CRT_BREAK_ALLOC -1

#include "main.h" // always include first!
#include <windows.h>
#include <float.h>
#include "../3rdparty/SDL2-2.0.8/include/SDL.h"

#include "display.h"
#include "timer.h"
#include "image.h"
#include "audio.h"
#include "demo.h"
#include "gamepad.h"

// filters & blitters
#include "polar.h"
#include "map-blitter.h"

// display config.
const char *kTitle = "powered by cocktails with Kurt Bevacqua";
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

	// set OpenMP threads to cores plus one (which is best for calculation-heavy threads, could yield more when threads are missing cache)
	omp_set_num_threads(std::thread::hardware_concurrency()+1);

	// set simplest rounding mode, since we do a fair bit of ftol()
	_controlfp(_MCW_RC, _RC_CHOP);

	// calculate cosine LUT
	CalculateCosLUT();

	bool utilInit = true;

	utilInit &= Image_Create();
	utilInit &= Shared_Create();
	utilInit &= Polar_Create();
	utilInit &= MapBlitter_Create();
	Gamepad_Create();

	if (utilInit)
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

	Audio_Destroy();
	Demo_Destroy();

	Gamepad_Destroy();
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
