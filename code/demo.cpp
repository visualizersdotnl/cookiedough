
// cookiedough -- TPB-07

#include "main.h"
// #include <windows.h> // for audio.h
#include "demo.h"
// #include "audio.h"
#include "rocket.h"
#include "image.h"

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

// --- Sync. tracks ---

SyncTrack trackEffect;
SyncTrack trackFadeToBlack, trackFadeToWhite;
SyncTrack trackCreditLogo, trackCreditLogoAlpha, trackCreditLogoBlurH, trackCreditLogoBlurV;
SyncTrack trackDiscoGuys;

// --------------------

// credits logos (1280x568)
static uint32_t *s_pCredits[4] = { nullptr };
constexpr auto kCredX = 1280;
constexpr auto kCredY = 568;

// vignette re-used (TPB-06)
static uint32_t *s_pVignette06 = nullptr;

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

	// init. sync.
	trackEffect = Rocket::AddTrack("demo:Effect");
	trackFadeToBlack = Rocket::AddTrack("demo:FadeToBlack");
	trackFadeToWhite = Rocket::AddTrack("demo:FadeToWhite");
	trackCreditLogo = Rocket::AddTrack("demo:CreditLogo");
	trackCreditLogoAlpha = Rocket::AddTrack("demo:CreditLogoAlpha");
	trackCreditLogoBlurH = Rocket::AddTrack("demo:CreditLogoBlurH");
	trackCreditLogoBlurV = Rocket::AddTrack("demo:CreditLogoBlurV");
	trackDiscoGuys = Rocket::AddTrack("demo:DiscoGuys");

	// load credits logos (1280x640)
	s_pCredits[0] = Image_Load32("assets/credits/Credits_Tag_Superplek_outlined.png");
	s_pCredits[1] = Image_Load32("assets/credits/Credits_Tag_Comatron_outlined.png");
	s_pCredits[2] = Image_Load32("assets/credits/Credits_Tag_Jade_outlined.png");
	s_pCredits[3] = Image_Load32("assets/credits/Credits_Tag_ErnstHot_outlined_new.png");
	for (auto *pImg : s_pCredits)
		if (nullptr == pImg)
			return false;

	// load generic TPB-06 dirty vignette
	s_pVignette06 = Image_Load32("assets/tpb-06-dirty-vignette-1280x720.png");
	if (nullptr == s_pVignette06)
		return false;

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
	// for this production:
	VIZ_ASSERT(kResX == 1280 && kResY == 720);

	Rocket::Boost();

	// render effect
	const int effect = Rocket::geti(trackEffect);
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

				const float discoGuys = clampf(0.f, 1.f, Rocket::getf(trackDiscoGuys));
				if (discoGuys > 0.f)
				{
					// now simply draw the 8 gentlemen	
					for (int iGuy = 0; iGuy < 8; ++iGuy)
					{
//						BlitSrc32A(pDest + xOffs + yOffs*kResX, g_pToyPusherTiles[iGuy], kResX, 128, 128, 0.4f + iGuy*(0.6f/7.f));
						BlitSrc32A(pDest + xOffs + yOffs*kResX, g_pToyPusherTiles[iGuy], kResX, 128, 128, discoGuys);
						xOffs += 128;
					}
				}
			}
			break;

		case 7:			
			Spikey_Draw(pDest, timer, delta, true);
 			MulSrc32A(pDest, s_pVignette06, kOutputSize); // FIXME?
			break;

		case 8:
			Spikey_Draw(pDest, timer, delta, false);
 			MulSrc32A(pDest, s_pVignette06, kOutputSize); // FIXME?
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

	// credit logo blit
	const int iLogo = clampi(0, 4, Rocket::geti(trackCreditLogo));
	if (0 != iLogo)
	{
		uint32_t *pCur = s_pCredits[iLogo-1];

		const float blurH = Rocket::getf(trackCreditLogoBlurH);
		if (0.f != blurH)
		{
			HorizontalBoxBlur32(g_renderTarget[0], pCur, kCredX, kCredY, BoxBlurScale(blurH));
			pCur = g_renderTarget[0];
		}

		const float blurV = Rocket::getf(trackCreditLogoBlurV);
		if (0 != blurV)
		{
			VerticalBoxBlur32(g_renderTarget[0], pCur, kCredX, kCredY, BoxBlurScale(blurV));
			pCur = g_renderTarget[0];
		}

		BlitSrc32A(pDest + ((kResY-kCredY)>>1)*kResX, pCur, kResX, kCredX, kCredY, clampf(0.f, 1.f, Rocket::getf(trackCreditLogoAlpha)));

//		BlitSrc32A(pDest + ((kResY-568)>>1)*kResX, s_pCredits[iLogo-1], kResX, 1280, 568, clampf(0.f, 1.f, Rocket::getf(trackCreditLogoAlpha)));
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
