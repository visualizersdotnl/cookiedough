
/*
	:                                                               :
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
	:             BYPASS presents 'Arrested Development'            :
	:        a retrosexual promotion in co-operation with TPB       :
*/

// codename: cookiedough (2009-2023)
// property of njdewit technologies & visualizers.nl (http://www.visualizers.nl)

// this codebase was started as a simple experiment and doesn't have much to show for
// modern handling of C++, but I'm confident that can be retrofitted slowly as we progress

// OSX build:
// - relies on: LLVM supporting OpenMP, DevIL & SDL2 (use Homebrew to install)
// - uses CMake (only for OSX!)

// 32-bit build DISCONTINUED (as of August 2018), because:
// - it's 2023 by now!

// third party:
// - GNU Rocket by Erik Faye-Lund & contributors (last updated 27/07/2018)
// - Developer's Image Library (DevIL)
// - BASS audio library by Ian Luck
// - SDL 2.x
// - Tiny Mersenne-Twister by Mutsuo Saito & Makoto Matsumoto
// - sse_mathfun.h by Julien Pommier
// - sse-intrincs-test by Alfred Klomp (http://www.alfredklomp.com/programming/sse-intrinsics/)
// - sse2neon by a whole bunch of people (see sse2neon.h)
// - OpenMP

// compiler settings for Visual C++:
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision, also steer clear of expensive ftol())
// - use multi-byte character set (i.e. no _UNICODE)
// - adv. instruction set: SSE2 for x86, not set for 64-bit (SSE2 is implied)
// - uses C++20 (not really, but for OSX at least I'm using that standard)

// important:
// - Windows: executables are built to target/x86/ or target/x64/ -- run from that directory!
// - keep DLLs (see above) up to date for each build
// - (almost) always include main.h on top
// - there's kResX/kResY and soforth telling you about the size of the output buffer
// - for other targets there are likewise constants
// - the delta time is in MS so it can be sensibly applied to for example gamepad axis values
// - could probably be using more streamed (WC) writes, but pick those battles carefully
// - not all Rocket sync. tracks are clamped, so you can potentially f*ck things over and violate access 

// where to configure what?
// - windowed mode and window title can be decided in main.cpp
// - CRT leak check can be toggled in main.cpp
// - module name specified in main.cpp
// - module rows-per-pattern in audio.cpp
// - module playback flags in audio.cpp
// - stream playback details, also: audio.cpp
// - main resolution in main.h (adjust target and effect map sizes in shared-resources.h and fx-blitter.h)
// - when writing code that depends on a certain resolution it's wise to put a static_assert() along with it
// - to enable playback mode (Rocket): rocket.h

// Undef. for Windows CRT leak check
#define WIN32_CRT_LEAK_CHECK
#define WIN32_CRT_BREAK_ALLOC -1

// Undef. for < 60FPS warning (release builds only)
// define FPS_WARNING

#include "main.h" // always include first!

#include <filesystem>

#if defined(_WIN32)
	#include <windows.h>
	#include <crtdbg.h>
#endif

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
#include "satori-lumablur.h"
#include "boxblur.h"

// -- display & audio config. --

const char *kTitle = "Bypass ft. TPB present 'ARRESTED DEVELOPMENT'";

constexpr bool kFullScreen = false;

static const char *kStream = "assets/audio/comatron - to the moon - final.wav";
// static const char *kStream = "assets/audio/development-only/comatron - to the moon - sync final.wav"; // SYNC. TEST STREAM
constexpr bool kSilent = false; // when you're working on anything else than synchronization

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

#if !defined(_WIN32)
int main(int argc, char *argv[])
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int nCmdShow)
#endif
{
#if defined(_WIN32) && defined(_DEBUG) && defined(WIN32_CRT_LEAK_CHECK)
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

	if (0 != SDL_Init(SDL_INIT_EVERYTHING))
	{
#if defined(_WIN32)
		MessageBox(NULL, SDL_GetError(), "Can't initialize SDL!", MB_OK | MB_ICONEXCLAMATION);
#else
		printf("Can't initialize SDL: %s", SDL_GetError());
#endif
		return 1;
	}

	// change path to target root
#if !defined(CMAKE_BUILD)	
	// probably Windows
    std::filesystem::current_path("../");
#else
    std::filesystem::current_path("../target");
#endif

	// check for SSE 4.1 / NEON
	if (false == SDL_HasSSE41() && false == SDL_HasNEON())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, "Processor does not support SSE 4.1 instructions.", nullptr);
		return 1;
	}

	// calculate cosine LUT
	CalculateCosLUT();

	// initialize fast (co)sine
	InitializeFastCosine();

#if defined(FOR_INTEL) && defined(_WIN32)
	// set simplest rounding mode, since we do a fair bit of ftol()
	_controlfp(_MCW_RC, _RC_CHOP);
#endif

	bool utilInit = true;

	utilInit &= Image_Create();
	utilInit &= Shared_Create();
	utilInit &= Polar_Create();
	utilInit &= FxBlitter_Create();
	utilInit &= SatoriLumaBlur_Create();
	utilInit &= BoxBlur_Create();

	Gamepad_Create();
	initialize_random_generator();

	// utilInit &= Snatchtiler();

	float avgFPS = 0.f;

	if (utilInit && RunTests() /* just always run the functional tests, never want to run if they fail */)
	{
		if (Demo_Create())
		{
			HWND audioHWND = nullptr;

#if defined(_WIN32)
			audioHWND = GetForegroundWindow();
#endif

			if (Audio_Create(-1, kStream, audioHWND, kSilent)) // FIXME: or is this just fine?
			{
				Display display;
				if (display.Open(kTitle, kResX, kResY, kFullScreen))
				{
					if (true == kFullScreen)
						SDL_ShowCursor(SDL_DISABLE);

					// frame buffer
					uint32_t* pDest = static_cast<uint32_t*>(mallocAligned(kOutputBytes, kAlignTo));
					memset32(pDest, 0, kOutputSize);

					Timer timer;

					size_t numFrames = 0;
					float oldTime = 0.f, newTime = 0.f, totTime = 0.f;
					while (true == HandleEvents())
					{
						oldTime = newTime;
						newTime = timer.Get();
						const float delta = newTime-oldTime; // base delta on sys. time

						const float audioTime = Audio_Get_Pos_In_Sec();
						if (false == Demo_Draw(pDest, audioTime, delta*100.f))
							break; // Rocket track says we're done

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

	Gamepad_Destroy();

	Audio_Destroy();
	Demo_Destroy();

	Image_Destroy();
	Shared_Destroy();
	Polar_Destroy();
	FxBlitter_Destroy();
	SatoriLumaBlur_Destroy();
	BoxBlur_Destroy();

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
		snprintf(fpsString, 256, "You're dropping below (roughly) sixty boy, avg. FPS: %f", avgFPS);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, kTitle, fpsString, nullptr);
	}
#endif

	char fpsString[256];
	snprintf(fpsString, 256, "\n *** Rough avg. FPS: %f ***\n", avgFPS);

#if defined(_WIN32)
	OutputDebugString(fpsString);
#else
	printf("%s", fpsString);
#endif


	return 0;
}
