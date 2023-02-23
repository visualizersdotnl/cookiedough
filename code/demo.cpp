
// cookiedough -- Bypass/TPB-07: arrested development

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

// for this production:
static_assert(kResX == 1280 && kResY == 720);

// --- Sync. tracks ---

SyncTrack trackEffect;
SyncTrack trackFadeToBlack, trackFadeToWhite;
SyncTrack trackCreditLogo, trackCreditLogoAlpha, trackCreditLogoBlurH, trackCreditLogoBlurV;
SyncTrack trackDiscoGuys;
SyncTrack trackShow1995, trackShow2006;
SyncTrack trackDirt;
SyncTrack trackScapeHUD, trackScapeRevision;
SyncTrack trackDistortTPB;
// SyncTrack trackFuckBlurV;
SyncTrack trackGreetSwitch;

// --------------------

// credits logos (1280x568)
static uint32_t *s_pCredits[4] = { nullptr };
constexpr auto kCredX = 1280;
constexpr auto kCredY = 568;

// vignette re-used (TPB-06)
static uint32_t *s_pVignette06 = nullptr;

// Nytrik's work once more (1280x602)
static uint32_t *s_pFighters = nullptr;

// Stars/NoooN text overlay, lens dirt & vignette (1280x720)
static uint32_t *s_pNoooN = nullptr;
static uint32_t *s_pMFX = nullptr;
static uint32_t *s_pTunnelFullDirt = nullptr;
static uint32_t *s_pTunnelVignette = nullptr;
static uint32_t *s_pTunnelVignette2 = nullptr;

// first spikey ball art
static uint32_t *s_pSpikeyFullDirt = nullptr;
static uint32_t *s_pSpikeyBypass = nullptr;
static uint32_t *s_pSpikeyArrested = nullptr;
static uint32_t *s_pSpikeyVignette = nullptr;
static uint32_t *s_pSpikeyVignette2 = nullptr;

// landscape art
static uint32_t *s_pHeliHUD = nullptr;
static uint32_t *s_pRevLogo = nullptr;

// ball art
static uint32_t *s_pBallVignette = nullptr; // and free color grading too!
// static uint32_t *s_pBallText = nullptr;

// greetings art
static uint32_t *s_pGreetingsDirt = nullptr;
static uint32_t *s_pGreetings1 = nullptr;
static uint32_t *s_pGreetings2 = nullptr;
static uint32_t *s_pGreetingsVignette = nullptr;

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
	trackShow1995 = Rocket::AddTrack("demo:Show1995");
	trackDirt = Rocket::AddTrack("demo:LensDirt");
	trackShow2006 = Rocket::AddTrack("demo:Show2006");
	trackScapeHUD = Rocket::AddTrack("demo:ScapeHUD");
	trackScapeRevision = Rocket::AddTrack("demo:ScapeRev");
	trackDistortTPB = Rocket::AddTrack("demo:DistortTPB");
//	trackFuckBlurV = Rocket::AddTrack("demo:FuckBlurV");
	trackGreetSwitch = Rocket::AddTrack("demo:GreetSwitch");

	// load credits logos (1280x640)
	s_pCredits[0] = Image_Load32("assets/credits/Credits_Tag_Superplek_outlined.png");
	s_pCredits[1] = Image_Load32("assets/credits/Credits_Tag_Comatron_Featuring_Celin_outlined.png");
	s_pCredits[2] = Image_Load32("assets/credits/Credits_Tag_Jade_outlined.png");
	s_pCredits[3] = Image_Load32("assets/credits/Credits_Tag_ErnstHot_outlined_new.png");
	for (auto *pImg : s_pCredits)
		if (nullptr == pImg)
			return false;

	// load generic TPB-06 dirty vignette
	s_pVignette06 = Image_Load32("assets/tpb-06-dirty-vignette-1280x720.png");
	if (nullptr == s_pVignette06)
		return false;

	// Nytrik's fighter logo
	s_pFighters = Image_Load32("assets/roundfighters-1280.png");
	if (nullptr == s_pFighters)
		return false;
	
	// first appearance of the 'spikey ball' including the title and main group
	s_pSpikeyArrested = Image_Load32("assets/spikeball/TheYearWas2023_Overlay_Typo.png");
	s_pSpikeyVignette = Image_Load32("assets/spikeball/Vignette_CoolFilmLook.png");
	s_pSpikeyVignette2 = Image_Load32("assets/spikeball/Vignette_Layer02_inverted.png");
	s_pSpikeyBypass = Image_Load32("assets/spikeball/SpikeyBall_byPass_BG_Overlay.png");
	s_pSpikeyFullDirt = Image_Load32("assets/spikeball/TheYearWas_Overlay_LensDirt.jpg");
	if (nullptr == s_pSpikeyArrested || nullptr == s_pSpikeyBypass || nullptr == s_pSpikeyFullDirt || nullptr == s_pSpikeyVignette || nullptr == s_pSpikeyVignette2)
		return false;

	// NoooN et cetera
	s_pNoooN = Image_Load32("assets/tunnels/TheYearWas_Overlay_Typo_style2.png");
	s_pMFX = Image_Load32("assets/tunnels/TheYearWas2006_Overlay_Overlay_Typo_style2.png");
	s_pTunnelFullDirt = Image_Load32("assets/tunnels/TheYearWas_Overlay_LensDirt.jpg");
	s_pTunnelVignette = Image_Load32("assets/tunnels/Vignette_CoolFilmLook.png");;
	s_pTunnelVignette2 = Image_Load32("assets/tunnels/Vignette_Layer02_inverted.png");
	if (nullptr == s_pNoooN || nullptr == s_pTunnelFullDirt || nullptr == s_pTunnelVignette || nullptr == s_pTunnelVignette2 || nullptr == s_pMFX)
		return false;
	
	// landscape
	s_pHeliHUD = Image_Load32("assets/scape/aircraft_hud_960x720.png"); 
	s_pRevLogo = Image_Load32("assets/scape/revision-logo_white.png");
	if ( nullptr == s_pHeliHUD || nullptr == s_pRevLogo)
		return false;

	// voxel ball
//	s_pBallText = Image_Load32("assets/ball/RGBFuckUp_Pixel_MaskedOut_smaller_desaturate.png");
	s_pBallVignette = Image_Load32("assets/ball/Vignette_Sparta300.png");
	if (nullptr == s_pBallVignette)
		return false;

	// greetings
	s_pGreetingsDirt = Image_Load32("assets/greetings/Bokeh_Lens_Dirt_51.png");
	s_pGreetings1 = Image_Load32("assets/greetings/Greetings_Part1_BG_Overlay.png");
	s_pGreetings2 = Image_Load32("assets/greetings/Greetings_Part2_BG_Overlay.png");
	s_pGreetingsVignette = Image_Load32("assets/greetings/Vignette_CoolFilmLook.png");
	if (nullptr == s_pGreetingsDirt || nullptr == s_pGreetings1 || nullptr == s_pGreetings2 || nullptr == s_pGreetingsVignette)
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

static void FadeFlash(uint32_t *pDest, float fadeToBlack, float fadeToWhite)
{
		if (fadeToWhite > 0.f)
			Fade32(pDest, kOutputSize, 0xffffff, uint8_t(fadeToWhite*255.f));

		if (fadeToBlack > 0.f)
			Fade32(pDest, kOutputSize, 0, uint8_t(fadeToBlack*255.f));
}

void Demo_Draw(uint32_t *pDest, float timer, float delta)
{
	// update sync.
	Rocket::Boost();

	// get fade/flash amounts
	const float fadeToBlack = Rocket::getf(trackFadeToBlack);
	const float fadeToWhite = Rocket::getf(trackFadeToWhite);

	// render effect/part
	const int effect = Rocket::geti(trackEffect);
	switch (effect)
	{
		case 1:
			// Quick intermezzo: voxel torus
			Twister_Draw(pDest, timer, delta);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);
			MulSrc32A(pDest, s_pVignette06, kOutputSize);
			break;
	
		case 2:
			{
				// Introduction: landscape
				Landscape_Draw(pDest, timer, delta);

				// overlay HUD
				const float alphaHUD = saturatef(Rocket::getf(trackScapeHUD));
				if (0.f != alphaHUD)
					BlitAdd32A(pDest + (kResX-960)/2, s_pHeliHUD, kResX, 960, 720, alphaHUD);

				// add Revision logo
				const float alphaRev = saturatef(Rocket::getf(trackScapeRevision));
				if (0.f != alphaRev)
				{
					if (0.6f > alphaRev)
						BlitSrc32A(pDest, s_pRevLogo, kResX, kResX, kResY, alphaRev);
					else
					{
						BoxBlur32(g_renderTarget[0], s_pRevLogo, kResX, kResY, BoxBlurScale(((alphaRev-0.6f)*8.f)));
						BlitSrc32A(pDest, g_renderTarget[0], kResX, kResX, kResY, alphaRev);
					}
				}
			}
			break;

		case 3:
			// Voxel ball
			{
				Ball_Draw(pDest, timer, delta);
				SoftLight32(pDest, s_pBallVignette, kOutputSize);
				FadeFlash(pDest, fadeToBlack, fadeToWhite);
				MulSrc32A(pDest, s_pVignette06, kOutputSize);
			}
			break;

		case 4:
			// Tunnels (also see case 9)
			{
				Tunnelscape_Draw(pDest, timer, delta);

				Sub32(pDest, s_pTunnelVignette2, kOutputSize);

				const float dirt = Rocket::getf(trackDirt);
				if (0.f == dirt)
					Excl32(pDest, s_pTunnelFullDirt, kOutputSize);
				else
				{
					BoxBlur32(g_renderTarget[0], s_pTunnelFullDirt, kResX, kResY, BoxBlurScale(dirt));
					Excl32(pDest, g_renderTarget[0], kOutputSize);
				}

				if (0 != Rocket::geti(trackShow1995))
					MixSrc32(pDest, s_pNoooN, kOutputSize);

				Overlay32(pDest, s_pTunnelVignette, kOutputSize);
			}
			break;
		
		case 5:
			// Plasma and credits
			{
				Plasma_Draw(pDest, timer, delta);

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
				}
			}
			break;

		case 6:
			// Nautilus (Michiel, RIP)
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
			// Close-up spike ball
			Spikey_Draw(pDest, timer, delta, true);
 			MulSrc32A(pDest, s_pVignette06, kOutputSize); // FIXME?
			break;

		case 8:
			// Spike ball with title and group name (Bypass)
			Spikey_Draw(pDest, timer, delta, false);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);
			SoftLight32(pDest, s_pSpikeyBypass, kOutputSize);
			Sub32(pDest, s_pSpikeyVignette2, kOutputSize);
			Excl32(pDest, s_pSpikeyFullDirt, kOutputSize);
			MulSrc32A(pDest, s_pVignette06, kOutputSize);
			MixSrc32(pDest, s_pSpikeyArrested, kOutputSize);
			SoftLight32(pDest, s_pSpikeyVignette, kOutputSize);
			break;

		case 9:
			// Part of the 'tunnels' part
			{
				Tunnel_Draw(pDest, timer, delta);
				Sub32(pDest, s_pTunnelVignette2, kOutputSize);

				if (0 != Rocket::geti(trackShow2006))
					MixSrc32(pDest, s_pMFX, kOutputSize);
			}
			break;

		case 10:
			// The 'golden tunnel'
			Sinuses_Draw(pDest, timer, delta);
			break;

		case 11:
			// What was supposed to be the 'Aura for Laura' grid, but became boxy and intended for greetings
			{
				Laura_Draw(pDest, timer, delta);

				static_assert(kResX == 1280 && kResY == 720);
				const auto yOffs = ((kResY-243)/2) + 240;
				const auto xOffs = 12; // ((kResX-263)/2) - 300;
				BlitSrc32(pDest + xOffs + yOffs*kResX, g_pXboxLogoTPB, kResX, 263, 243);

				const int greetSwitch = Rocket::geti(trackGreetSwitch);
				switch (greetSwitch)
				{
				case 0:
					Darken32_50(pDest, s_pGreetings1, kOutputSize);
					break;

				default:
				case 1:
					Darken32_50(pDest, s_pGreetings2, kOutputSize);
					break;
				}

				SoftLight32(pDest, s_pGreetingsDirt, kOutputSize);
				Overlay32(pDest, s_pGreetingsVignette, kOutputSize);
			}
			break;

		case 12:
			{
				// TPB represent
				memset32(g_renderTarget[0], 0xffffff, kResX*kResY);
				BlitSrc32(g_renderTarget[0] + ((kResX-800)/2) + ((kResY-600)/2)*kResX, g_pNytrikMexico, kResX, 800, 600);

				const float distortTPB = Rocket::getf(trackDistortTPB);
				TapeWarp32(pDest, g_renderTarget[0], kResX, kResY, kGoldenRatio, distortTPB);
			}
			break;

		case 13:
			// Nate Diaz represent
			memset32(pDest, 0, kResX*kResY);
			BlitSrc32(pDest + ((kResY-602)/2)*kResX, s_pFighters, kResX, 1280, 602);
			break;

		default:
			FxBlitter_DrawTestPattern(pDest);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 	}

	// post fade/flash
	switch (effect)
	{
	case 1:
	case 3:
	case 8:
		// handled by effect/part
		break;

	default:
		FadeFlash(pDest, fadeToBlack, fadeToWhite);
	}

	return;
}
