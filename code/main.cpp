
// codename: cookiedough (2009-2025)
// 2023 release: Arrested Development by Bypass ft. TPB @ Revision 2023
// 2025 release: Radix equals Patrik by Replay, the design kings (TheParty.DK software competition entry)
// property of njdewit technologies, Guillamne Werle, Patrik Neumann, Vincent Bijwaard and a few other motherf*ckers

// this codebase was started as a simple experiment and doesn't have much to show for
// modern handling of C++, but I'm confident that can be retrofitted slowly as we progress

// also after Revision 2023 recently, there are bits and pieces that are just plain sloppy, 
// but there's something Ryg once said about such code and I guess that's true to this day for most of us

// misc. facts:
// - 2023: competition version was 1080p, consumer grade version is 720p
// - 2025: upgrade competition version to 4K? discuss with Chaos and Reza (rather CPU-bound, but customer grade should go up to 1080p)
// - 32-bit build DISCONTINUED (as of August 2018)

// OSX build (Windows one should be obvious):
// - relies on (but not limited to): LLVM supporting OpenMP, DevIL & SDL2 (use Homebrew to install)
// - uses CMake and a script in '/target/osx'

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
// - To circle around making a grown up OSX application: https://github.com/SCG82/macdylibbundler

// compiler settings for Visual C++:
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision, also steer clear of expensive ftol())
// - use multi-byte character set (i.e. no _UNICODE)
// - adv. instruction set: SSE 4.2 / NEON
// - uses C++20 (not really, but for OSX at least I'm using that standard)

// important:
// - executables are built to target/<arch> -- run from that directory!
// - keep DLLs (for Windows, see above) up to date for each build
// - (almost) always include main.h on top
// - there's kResX/kResY and soforth telling you about the size of the output buffer
// - for other targets there are likewise constants
// - the delta time is in MS so it can be sensibly applied to for example gamepad axis values
// - could probably be using more streamed (WC) writes, but pick those battles carefully
// - not all Rocket sync. tracks are clamped, so you can potentially f*ck things over and violate access 

// where to configure what?
// - windowed mode and window title can be decided in main.cpp
// - CRT leak check can be toggled in main.cpp
// - module name specified in main.cpp (FIXME: you'll need to modify some code (commented, mostly) to get module playback to work again)    
//   + module rows-per-pattern in audio.cpp  
//   + module playback flags in audio.cpp
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

// FIXME: the f*ck is this header right here for then?
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

// -- debug, display & audio config. --

const char *kTitle = "REPLAY PC SOFTWARE COMPETITION DEMO 2025";

constexpr bool kFullScreen = true;

static const char *kStream = "assets/audio/comatron - to the moon - final.wav";
constexpr bool kSilent = false; // when you're working on anything else than synchronization

// enable this to receive derogatory comments
// #define DISPLAY_AVG_FPS

/*
	look for SYNC_PLAYER in the header to switch between editor and replay (release) mode
	when running in editor mode, on (regular) exit, all Rocket tracks will be exported to '/target/sync'
*/

// -----------------------------

/*
	so as luck would have it, not only is statically linking a nightmare in OSX if you do things the Linux way, but the work dir. also
	differs depending on if you launch from finder or for example the terminal; for this project I am unwilling to turn to XCode, so we'll
	be doing it the dirty way instead
*/

#if defined(__APPLE__)

#include <mach-o/dyld.h>
#include <limits.h>

static const std::string GetMacWorkDir()
{
	char pathBuf[PATH_MAX] = { 0 };
	uint32_t bufSize = PATH_MAX;
	_NSGetExecutablePath(pathBuf, &bufSize);
	std::string fullPath(pathBuf);
	return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

#endif

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

	// change path to target root (which is a dirty affair on Mac)
#if defined(__APPLE__)
	std::__fs::filesystem::current_path(GetMacWorkDir() + "/..");
#elif defined(__linux__)
    std::filesystem::current_path(".");
#else
    std::filesystem::current_path("..");
#endif

	printf("And today we'll be working from: %s\n", reinterpret_cast<const char *>(std::__fs::filesystem::current_path().c_str()));

	// check for SSE 4.2 / NEON 
#if defined(FOR_ARM)
	if (false == SDL_HasNEON())
#elif defined(FOR_INTEL)
	if (false == SDL_HasSSE42())
#endif
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kTitle, "SDL tells me your processor does not support SSE 4.2 (x64) nor NEON (ARM) instructions.", nullptr);
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

#if defined(DISPLAY_AVG_FPS)
#if defined(_WIN32)
	OutputDebugString(fpsString);
#else
	printf("%s", fpsString);
#endif
#endif

	return 0;
}
