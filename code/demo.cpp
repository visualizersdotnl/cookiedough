
// cookiedough -- demo flow for: <?> by Bypass (Shader Superfights Grand Prix 2019 commercial)

#include "main.h"
// #include <windows.h> // for audio.h
#include "demo.h"
// #include "audio.h"
#include "rocket.h"

// filters & blitters
#include "boxblur.h"
#include "polar.h"
#include "fx-blitter.h"
#include "satori-lumablur.h"

// effects
#include "ball.h"
#include "landscape.h"
#include "torus-twister.h"
#include "tunnelscape.h"
#include "shadertoy.h"

SyncTrack trackEffectTest;
SyncTrack trackFadeToBlack, trackFadeToWhite;

bool Demo_Create()
{
	if (false == Rocket::Launch())
		return false;

	bool fxInit = true;
	fxInit &= Twister_Create();
	fxInit &= Landscape_Create();
	fxInit &= Ball_Create();
	fxInit &= Tunnelscape_Create();
	fxInit &= Shadertoy_Create();

	trackEffectTest = Rocket::AddTrack("effectTest");
	trackFadeToBlack = Rocket::AddTrack("fadeToBlack");
	trackFadeToWhite = Rocket::AddTrack("fadeToWhite");

	return fxInit;
}

void Demo_Destroy()
{
	Rocket::Land();

	Twister_Destroy();
	Landscape_Destroy();
	Ball_Destroy();
	Tunnelscape_Destroy();
	Shadertoy_Destroy();
}

void Demo_Draw(uint32_t *pDest, float timer, float delta)
{
	// for this production (FIXME)
	// VIZ_ASSERT(kResX == 800 && kResY == 600);

	Rocket::Boost();

	// FIXME: not always necessary
	memset32(pDest, 0, kOutputSize);

	// render effect
	int effect = Rocket::geti(trackEffectTest);
	switch (effect)
	{
		case 1:
			Twister_Draw(pDest, timer, delta);
			break;
	
		case 2:
			Landscape_Draw(pDest, timer, delta);
			break;

		case 3:
			Ball_Draw(pDest, timer, delta);
			break;

		case 4:
			Tunnelscape_Draw(pDest, timer, delta);
			break;
		
		case 5:
			Plasma_Draw(pDest, timer, delta);
			break;

		case 6:
			{
				Nautilus_Draw(pDest, timer, delta);

				// offsets
				static_assert(kResX == 1280 && kResY == 720);
				const auto yOffs = ((kResY-128)/2) + 290; // lower..  
				auto xOffs = ((kResX-(128*8))/2) + 120;   // .. right corner

				// now simply draw the 8 gentlemen	
				for (int iGuy = 0; iGuy < 8; ++iGuy)
				{
					BlitSrc32A(pDest + xOffs + yOffs*kResX, g_pToyPusherTiles[iGuy], kResX, 128, 128, 0.4f + iGuy*(0.6f/7.f));
					xOffs += 128;
				}
			}
			break;

		case 7:			
			Spikey_Draw(pDest, timer, delta, true);
			break;

		case 8:
			Spikey_Draw(pDest, timer, delta, false);
			break;

		case 9:
			Tunnel_Draw(pDest, timer, delta);
			break;

		case 10:
			Sinuses_Draw(pDest, timer, delta);
			break;

		case 11:
			{
				Laura_Draw(pDest, timer, delta);

				static_assert(kResX == 1280 && kResY == 720);
				const auto yOffs = ((kResY-243)/2) + 240;
				const auto xOffs = 44; // ((kResX-263)/2) - 300;
				BlitSrc32(pDest + xOffs + yOffs*kResX, g_pXboxLogoTPB, kResX, 263, 243);
			}
			break;

		case 12:
			memset32(pDest, 0xffffff, kResX*kResY);
			BlitSrc32(pDest + ((kResX-800)/2) + ((kResY-600)/2)*kResX, g_pNytrikMexico, kResX, 800, 600);
			break;

		default:
			FxBlitter_DrawTestPattern(pDest);
	}

	// post processing
	const float fadeToBlack = Rocket::getf(trackFadeToBlack);
	const float fadeToWhite = Rocket::getf(trackFadeToWhite);

	if (fadeToWhite > 0.f)
		Fade32(pDest, kOutputSize, 0xffffff, uint8_t(fadeToWhite*255.f));

	if (fadeToBlack > 0.f)
		Fade32(pDest, kOutputSize, 0, uint8_t(fadeToBlack*255.f));

	return;
}
