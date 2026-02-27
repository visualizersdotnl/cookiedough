
/*
	codename: cookiedough (2006-2023-?)
	- 2023 release: Arrested Development by Bypass ft. TPB @ Revision 2023

	this codebase was started as a simple experiment and doesn't have much to show for,
	in terms of modern C/C++, but I'm confident that can be retrofitted slowly as we march on

	also after Revision 2023 recently, there are bits and pieces that are just plain sloppy, 
	but there's something Fabian Giesen once said about demo source code being disposable after the party, right? ;)

	builds (all 64-bit, 32-bit discountinued in 2018):
	
	* Windows build (x64 only): Visual Studio 2019 or later

	* OSX build (originally intended for Silicon only, but has worked for Intel as well):
	  - relies on (but not limited to): LLVM supporting OpenMP, BASS 2.4, DevIL & SDL2 (use Homebrew to install)
	  - BASS is supplied in /3rdparty and /target/osx for arm64, needs some tinkering to get the x64 version working
	  - build using CMake / VSCode (install CMake, CMake Tools, MS' C++ extensions et cetera, VSCode will tell you)
	  - inspect CMakeLists.txt for details on dependencies, compiler flags et cetera

	* Linux build (should build both on x64 and ARM64):
	  - graciously provided by Erik Faye-Lund
	  - same dependencies as OSX build
	  - if you install BASS to /usr/local/include and /lib it'll pick it up automatically at time of writing (10/01/2026)

	to circle around making a grown up OSX application: https://github.com/SCG82/macdylibbundler
	what you'd typically do is bundle the .app with the required .dylibs and resources in /target/libs

	Kusma did likewise for the Linux build (x64), so depending on if it's x64 or ARM64, the right BASS .so needs to be in /target/libs
	however, the Linux build *does* expect you've got your DevIL and SDL2 shared libraries installed system-wide
*/

// I'd like: 
// - transparent x64/ARM64 support for OSX and Linux (FIXME)
// - to get rid of 'macdylibbundler' use (FIXME)

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
// - ImGui (FIXME: add details / perhaps update?)
// - Std3DMath is of my own making

//  how to use ImGui integration:
// - TAB to show/hide
// - currently only enabled in windowed mode
// - ImGuiIsVisible() will tell you if you should be drawing ImGui widgets
// - currently included in main.h, so should be available everywhere you might need it

// compiler settings for Visual C++ (last checked by me in 2023):
// - GNU Rocket depends on ws2_32.lib
// - use multi-threaded CRT (non-DLL)
// - disable C++ exceptions
// - fast floating point model (i.e. single precision, also steer clear of expensive ftol())
// - use multi-byte character set (i.e. no _UNICODE)
// - adv. instruction set: SSE 4.2 / NEON
// - C++20 (at least that's the standard fed to the compiler)

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

#include <filesystem> // FIXME: might only be necessary for OSX

#if defined(_WIN32)
	#include <windows.h>
	#include <crtdbg.h>
#endif

#include <float.h>
#include "../3rdparty/SDL2-2.28.5/include/SDL.h"

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
#include "boxblur.h"

// -- debug, display & audio config. --

const char *kTitle = "Arrested Development (BPS ft. TPB)";

static const char *kStream = "assets/audio/comatron - to the moon - final.wav";

// when you're working on anything else than synchronization/demonstration
constexpr bool kSilent = false; 

// enable this to receive derogatory comments
// #define DISPLAY_AVG_FPS

/*
	look for SYNC_PLAYER in the header to switch between editor and replay (release) mode
	when running in editor mode, on (regular) exit, all Rocket tracks will be exported to '/target/sync'
*/

// -----------------------------

/*
	so as luck would have it, not only is statically linking a nightmare in OSX if you do things the Unix way, 
	but the work dir. also *differs* depending on if you launch from finder or for example the terminal 
	
	for this project I am unwilling to turn to XCode, so we'll be doing it the 100% deterministic way instead
*/

#if defined(__APPLE__)

#include <mach-o/dyld.h>
#include <limits.h>

static const std::string OSX_GetExecutableDirectory()
{
	// grab *actual* executable path
	char pathBuf[PATH_MAX] = { 0 };
	uint32_t bufSize = PATH_MAX;
	_NSGetExecutablePath(pathBuf, &bufSize);
	
	// strip executable name
	const std::string fullPath(pathBuf);
	const std::string exeDir = fullPath.substr(0, fullPath.find_last_of("/"));

	return exeDir;
}

#endif

// -----------------------------

static bool s_showImGui = false;

bool ImGuiIsVisible()
{
	return s_showImGui;
}

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

#if !defined(SYNC_PLAYER)
		if (!kFullScreen)
			ImGui_ImplSDL2_ProcessEvent(&event);
#endif
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
	std::__fs::filesystem::current_path(OSX_GetExecutableDirectory() + "/..");
#else // Windows and Linux
    std::filesystem::current_path("..");
#endif

	printf("And today we'll be working from: %s\n", reinterpret_cast<const char *>(std::filesystem::current_path().c_str()));

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

#if !defined(SYNC_PLAYER)
						if (ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Tab)) && !kFullScreen)
							s_showImGui = !s_showImGui;

						if (!kFullScreen)
						{
							ImGui_ImplSDLRenderer2_NewFrame();
							ImGui_ImplSDL2_NewFrame();
							
							ImGui::NewFrame();
							
							if (ImGuiIsVisible())
								ImGui::Begin("I'm ImGui!"); // dear lord Thorsten, that is a particularly wimpy introduction :D
						}
#endif

						const float audioTime = Audio_Get_Pos_In_Sec();
						if (false == Demo_Draw(pDest, audioTime, delta * 100.f))
							break; // Rocket track says we're done

#if !defined(SYNC_PLAYER)
						if (!kFullScreen)
						{
							if (ImGuiIsVisible())
								ImGui::End();
							
							ImGui::Render();
						}
#endif

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
