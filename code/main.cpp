
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

// codename: cookiedough (2009-2018)
// property of visualizers.nl (http://www.visualizers.nl)

// 32-bit build DISCONTINUED (as of August 2018), because:
// - OpenMP not working properly
// - _mm_cvtsi128_si64() not supported on x86 (map-blitter.cpp)
// - other potential SSE 4.1 / 64-bit instruction use
// - probably too slow for what I'm doing anyway

// third party:
// - GNU Rocket by Erik Faye-Lund & contributors (last updated 27/07/2018)
// - Developer's Image Library (DevIL)
// - BASS audio library by Ian Luck
// - SDL 2.0.8
// - Tiny Mersenne-Twister by Mutsuo Saito & Makoto Matsumoto
// - sse_mathfun.h by Julien Pommier

// compiler settings for Visual C++:
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision, also steer clear of expensive ftol())
// - use multi-byte character set (i.e. no _UNICODE)
// - adv. instruction set: SSE2 for x86, not set for 64-bit (SSE2 is implied)
// - uses C++11 (and possibly C++14)

// important:
// - executables are built to target/x86/ or target/x64/ -- run from that directory!
// - keep DLLs (see above) up to date for each build
// - (almost) always include main.h on top
// - there's kResX/kResY and soforth telling you about the size of the output buffer
// - for other targets there are likewise constants
// - the delta time is in MS so it can be sensibly applied to for example gamepad axis values

// where to configure what?
// - windowed mode and window title can be decided in main.cpp
// - CRT leak check can be toggled in main.cpp
// - module name specified in main.cpp
// - module rows-per-pattern in audio.cpp
// - module playback flags in audio.cpp
// - main resolution in main.h (adjust target and effect map sizes in shared-resources.h and fx-blitter.h)

// Undef. for (Windows - should work on other platforms too) CRT leak check
// #define WIN32_CRT_LEAK_CHECK
#define WIN32_CRT_BREAK_ALLOC -1

// ** this will kill Rocket and module replay and use the FM synth. to feed a stream **
const bool kTestBedForFM = true;

// Undef. for < 60FPS warning
#define FPS_WARNING

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
#include "tests.h"

// filters & blitters
#include "polar.h"
#include "fx-blitter.h"

// FM synthesizer
#include "FM-synthesizer/FM_BISON.h"
#include "cspan.h"

// -- display & audio config. --

const char *kTitle = "cocktails at Kurt Bevacqua's";

const bool kFullScreen = false;

// const char *kModule = "assets/moby_-_eliminator-tribute.mod";
// const char *kModule = "assets/theduel.mod";
const char *kModule = "assets/knulla-kuk.mod";

// when you're working on anything else than synchronization
const bool kSilent = false;

// -----------------------------

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

// Delete after beating Zden in Bratislava.
// #include "../shaderGP-detour/snatchtiler.h"

// int SDL_main(int argc, char *argv[])
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
{
#if defined(_DEBUG) && defined(WIN32_CRT_LEAK_CHECK)
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

//	const auto audioFlag = (true == kTestBedForFM) ? SDL_INIT_AUDIO : 0;
//	if (0 != SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | audioFlag))
	if (0 != SDL_Init(SDL_INIT_EVERYTHING))
	{
		MessageBox(NULL, SDL_GetError(), "Can't initialize SDL!", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	// change path to target root
	SetCurrentDirectoryA("../");

	// check for SSE 4.1
	if (false == SDL_HasSSE41())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, "Processor does not support SSE 4.1 instructions.", nullptr);
		return 1;
	}

	// calculate cosine LUT
	CalculateCosLUT();

	// set simplest rounding mode, since we do a fair bit of ftol()
	if (false == kTestBedForFM)
		_controlfp(_MCW_RC, _RC_CHOP);
		
	bool utilInit = true;

	utilInit &= Image_Create();
	utilInit &= Shared_Create();
	utilInit &= Polar_Create();
	utilInit &= FxBlitter_Create();

	Gamepad_Create();
	initialize_random_generator();

	// utilInit &= Snatchtiler();

	float avgFPS = 0.f;

	if (utilInit && RunTests() /* just always run the functional tests, never want to run if they fail */)
	{
		if (true == kTestBedForFM)
		{
			/* Test code for FM synth */

			auto *bumper = Image_Load32("../code/FM-synthesizer/artwork/window-filler.png");

			Display display;
			if (display.Open(kTitle, 1280, 303, kFullScreen))
			{
				Syntherklaas_Create();

				Timer timer;

				float oldTime = 0.f, newTime = 0.f;
				while (true == HandleEvents())
				{
					oldTime = newTime;
					newTime = timer.Get();
					const float delta = newTime-oldTime;
					
					// sloppy VU meter
					static float prevLoudest = 0.f;
					float loudest = Syntherklaas_Render(nullptr, newTime, delta*100.f);
					if (loudest == 0.f) loudest = prevLoudest;
					prevLoudest = lowpassf(prevLoudest, loudest, 4.f);

					unsigned length = 1+unsigned(loudest*1279.f);

					for (int iY = 3; iY < 15; ++iY)
						cspan(bumper + 1280*iY, 1, 1280, 1280, 0, 0x7f7f7f);

					for (int iY = 3; iY < 15; ++iY)
					{
						if (iY & 1)
							cspan(bumper + 1280*iY, 1, length, length, 0x007f00, 0xff0000);
						else
							cspan(bumper + 1280*iY, 1, length, length, 0xff7f00, 0xff0000);
					}

					display.Update(bumper);
				}

				// bypass message box
				avgFPS = 60.f;
			}
		}
		else
		{
			if (Demo_Create())
			{
				if (Audio_Create(-1, kModule, GetForegroundWindow(), kSilent)) // FIXME: or is this just fine?
				{
					Display display;
					if (display.Open(kTitle, kResX, kResY, kFullScreen))
					{
						// frame buffer
						uint32_t* pDest = static_cast<uint32_t*>(mallocAligned(kOutputBytes, kCacheLine));
						memset32(pDest, 0, kOutputSize);

						Timer timer;

						size_t numFrames = 0;
						float oldTime = 0.f, newTime = 0.f, totTime = 0.f;
						while (true == HandleEvents())
						{
							oldTime = newTime;
							newTime = timer.Get();
							const float delta = newTime-oldTime;
							Demo_Draw(pDest, newTime, delta*100.f);
							display.Update(pDest);

							totTime += delta;
							++numFrames;
						}

						avgFPS = numFrames/totTime;

						freeAligned(pDest);
					}

				}
			}
		}
	}

	if (true == kTestBedForFM)
		Syntherklaas_Destroy();

	Gamepad_Destroy();

	Audio_Destroy();
	Demo_Destroy();

	Image_Destroy();
	Shared_Destroy();
	Polar_Destroy();
	FxBlitter_Destroy();

	SDL_Quit();

	if (false == s_lastErr.empty())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, s_lastErr.c_str(), nullptr);
		return 1;
	}

#if !defined(_DEBUG) && defined(FPS_WARNING)
	if (avgFPS < 59.f)
	{
		char fpsString[256];
		sprintf(fpsString, "You're dropping below (roughly) sixty boy, avg. FPS: %f", avgFPS);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, kTitle, fpsString, nullptr);
	}

	char fpsString[256];
	sprintf(fpsString, "\n *** Rough avg. FPS: %f ***\n", avgFPS);
	OutputDebugString(fpsString);
#endif

	return 0;
}
