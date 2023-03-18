
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
#include "lens.h"

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
SyncTrack trackDistortTPB, trackDistortStrengthTPB, trackBlurTPB;
SyncTrack trackGreetSwitch;
SyncTrack trackCousteau;
SyncTrack trackCousteauHorzBlur;

SyncTrack trackShooting;
SyncTrack trackShootingX, trackShootingY, trackShootingAlpha;
SyncTrack trackShootingTrail;

SyncTrack trackWaterLove;

// --------------------

// credits logos (1280x568)
static uint32_t *s_pCredits[4] = { nullptr };
constexpr auto kCredX = 1280;
constexpr auto kCredY = 568;

// vignette re-used (TPB-06)
static uint32_t *s_pVignette06 = nullptr;

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

// greetings art
static uint32_t *s_pGreetingsDirt = nullptr;
static uint32_t *s_pGreetings[4] = { nullptr };
static uint32_t *s_pGreetingsVignette = nullptr;

// nautilus art
static uint32_t *s_pNautilusVignette = nullptr;
static uint32_t *s_pNautilusDirt = nullptr;
static uint32_t *s_pNautilusCousteau1 = nullptr;
static uint32_t *s_pNautilusCousteauRim1 = nullptr;
static uint32_t* s_pNautilusCousteauRim2 = nullptr;
static uint32_t *s_pNautilusCousteau2 = nullptr;
static uint32_t *s_pNautilusText = nullptr;

// disco guys (hello Thorsten, TPB-06) + melancholic numbers to accompany them
static uint32_t *s_pDiscoGuys[8] = { nullptr };
static uint32_t *s_pAreWeDone = nullptr;

// close-up 'spikey' art
static uint32_t *s_pCloseSpikeDirt = nullptr;
static uint32_t *s_pCloseSpikeVignette = nullptr;

// under water tunnel art
static uint32_t *s_pWaterDirt = nullptr;
static uint32_t* s_pWaterPrismOverlay = nullptr;

// shooting star art
static uint32_t *s_pLenz = nullptr;

// --- Shooting star related things ---

constexpr unsigned kLenzSize = 64;

// --------------------

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
	trackDistortStrengthTPB = Rocket::AddTrack("demo:DistortStrengthTPB");
	trackBlurTPB = Rocket::AddTrack("demo:BlurTPB");
	trackGreetSwitch = Rocket::AddTrack("demo:GreetSwitch");
	trackCousteau = Rocket::AddTrack("demo:Cousteau");
	trackCousteauHorzBlur = Rocket::AddTrack("demo:CousteauHorzBlur");
	trackWaterLove = Rocket::AddTrack("demo:WaterLove");

	trackShooting = Rocket::AddTrack("shootingStar:Enabled");
	trackShootingX = Rocket::AddTrack("shootingStar:X");
	trackShootingY = Rocket::AddTrack("shootingStar:Y");
	trackShootingAlpha = Rocket::AddTrack("shootingStar:A");
	trackShootingTrail = Rocket::AddTrack("shootingStar:Trail");

	// load credits logos (1280x640)
	s_pCredits[0] = Image_Load32("assets/credits/Credits_Tag_Superplek_outlined.png");
	s_pCredits[1] = Image_Load32("assets/credits/Credits_Tag_Comatron_Featuring_Celin_outlined.png");
	s_pCredits[2] = Image_Load32("assets/credits/Credits_Tag_Jade_outlined.png");
	s_pCredits[3] = Image_Load32("assets/credits/Credits_Tag_ErnstHot_outlined_new.png");
	for (auto *pImg : s_pCredits)
		if (nullptr == pImg)
			return false;

	// load generic TPB-06 dirty vignette
	s_pVignette06 = Image_Load32("assets/demo/tpb-06-dirty-vignette-1280x720.png");
	if (nullptr == s_pVignette06)
		return false;
	
	// first appearance of the 'spikey ball' including the title and main group
	s_pSpikeyArrested = Image_Load32("assets/spikeball/TheYearWas2023_Overlay_Typo.png");
	s_pSpikeyVignette = Image_Load32("assets/spikeball/Vignette_CoolFilmLook.png");
	s_pSpikeyVignette2 = Image_Load32("assets/spikeball/Vignette_Layer02_inverted.png");
	s_pSpikeyBypass = Image_Load32("assets/spikeball/SpikeyBall_byPass_BG_Overlay.png");
	s_pSpikeyFullDirt = Image_Load32("assets/spikeball/nytrik-TheYearWas_Overlay_LensDirt.jpg");
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
	s_pGreetings[0]= Image_Load32("assets/greetings/Greetings_Part1_BG_Overlay.png");
	s_pGreetings[1] = Image_Load32("assets/greetings/Greetings_Part2_BG_Overlay.png");
	s_pGreetings[2] = Image_Load32("assets/greetings/Greetings_Part3_BG_Overlay.png");
	s_pGreetings[3] = Image_Load32("assets/greetings/Greetings_Part4_BG_Overlay.png");
	s_pGreetingsVignette = Image_Load32("assets/greetings/Vignette_CoolFilmLook.png");
	if (
		nullptr == s_pGreetingsDirt || 
		nullptr == s_pGreetings[0] || nullptr == s_pGreetings[1] || nullptr == s_pGreetings[2] || nullptr == s_pGreetings[3] ||
		nullptr == s_pGreetingsVignette)
			return false;

	// nautilus
	s_pNautilusVignette = Image_Load32("assets/nautilus/Vignette.png");
	s_pNautilusDirt = Image_Load32("assets/nautilus/GlassDirt_Distorted2.png");
	s_pNautilusCousteau2 = Image_Load32("assets/nautilus/JacquesCousteau_Silhouette2.png");
	s_pNautilusCousteau1 = Image_Load32("assets/nautilus/JacquesCousteau1_Silhouette.png");
	s_pNautilusCousteauRim1 = Image_Load32("assets/nautilus/JacquesCousteau1_Silhouette_RimMask.png");
	s_pNautilusCousteauRim2 = Image_Load32("assets/nautilus/JacquesCousteau_Silhouette2_RimMask.png");
	s_pNautilusText = Image_Load32("assets/nautilus/JacquesCousteau_Text.png");
	if (nullptr == s_pNautilusVignette || nullptr == s_pNautilusDirt || nullptr == s_pNautilusText || nullptr == s_pNautilusCousteau1 || nullptr == s_pNautilusCousteau2 || nullptr == s_pNautilusCousteauRim1 || nullptr == s_pNautilusCousteauRim2)
		return false;

	// load 'disco guys'
	s_pDiscoGuys[0] = Image_Load32("assets/demo/tpb-06-disco-guy/1.png");
	s_pDiscoGuys[1] = Image_Load32("assets/demo/tpb-06-disco-guy/1b.png");
	s_pDiscoGuys[2] = Image_Load32("assets/demo/tpb-06-disco-guy/2.png");
	s_pDiscoGuys[3] = Image_Load32("assets/demo/tpb-06-disco-guy/2b.png");
	s_pDiscoGuys[4] = Image_Load32("assets/demo/tpb-06-disco-guy/3.png");
	s_pDiscoGuys[5] = Image_Load32("assets/demo/tpb-06-disco-guy/3b.png");
	s_pDiscoGuys[6] = Image_Load32("assets/demo/tpb-06-disco-guy/4.png");
	s_pDiscoGuys[7] = Image_Load32("assets/demo/tpb-06-disco-guy/4b.png");
	for (const auto *pointer : s_pDiscoGuys)
		if (nullptr == pointer)
			return false;

	// full credits (used to be a melancholic '2001-2023' to signify the end of TPB, hence the variable name)
	s_pAreWeDone = Image_Load32("assets/demo/are-we-done-1000x52.png");
	if (nullptr == s_pAreWeDone)
		return false;

	// close-up 'spikey' 
	s_pCloseSpikeDirt = Image_Load32("assets/closeup/LensDirt5_invert.png");
	s_pCloseSpikeVignette = Image_Load32("assets/closeup/Vignette_CoolFilmLook.png");
	if (nullptr == s_pCloseSpikeDirt || nullptr == s_pCloseSpikeVignette)
		return false;

	// under water tunnel
	s_pWaterDirt = Image_Load32("assets/underwater/LensDirt3_invert.png");
	if (nullptr == s_pWaterDirt)
		return false;

	s_pWaterPrismOverlay = Image_Load32("assets/underwater/love prism_alpha 1280_720.png");
	if (nullptr == s_pWaterPrismOverlay)
		return false;

	// shooting star
	s_pLenz = Image_Load32("assets/shooting/Lenz.png");

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
			SoftLight32(pDest, s_pBallVignette, kOutputSize);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);

			// commented why? see case #3
//			MulSrc32A(pDest, s_pVignette06, kOutputSize);
			break;
	
		case 2:
			{
				// Introduction: landscape
				Landscape_Draw(pDest, timer, delta);

				// shooting star (or what has to pass for it)
				// this is the charm of a hack made possible by Rocket
				// FIXME: really shouldn't be using threaded blit function(s) here
				if (1 == Rocket::geti(trackShooting))
				{
					int xPos = Rocket::geti(trackShootingX);
					int yPos = Rocket::geti(trackShootingY);
					float alpha = Rocket::getf(trackShootingAlpha);

					const unsigned offset = yPos*kResX + xPos;
					BlitSrc32A(pDest + offset, s_pLenz, kResX, kLenzSize, kLenzSize, alpha);

					int trail = Rocket::geti(trackShootingTrail);
					if (trail > 0)
					{
						const int xStep = kLenzSize/16;
						const int yStep = 1;
						const float alphaStep = alpha/trail;

						for (int iTrail = 0; iTrail < trail; ++iTrail)
						{
							xPos += xStep; // simply assuming we're going from right to left
							yPos -= yStep; // and from top to bottom
							alpha -= alphaStep;

							const unsigned offset = yPos*kResX + xPos;
							BlitSrc32A(pDest + offset, s_pLenz, kResX, kLenzSize, kLenzSize, alpha);
						}
					}
				}

				// FIXME: placeholder
				SoftLight32(pDest, s_pTunnelVignette, kOutputSize);

				// overlay HUD
				const float alphaHUD = saturatef(Rocket::getf(trackScapeHUD));
				if (0.f != alphaHUD)
					BlitAdd32A(pDest + (kResX-960)/2, s_pHeliHUD, kResX, 960, 720, alphaHUD);

				// add Revision logo
				const float alphaRev = saturatef(Rocket::getf(trackScapeRevision));
				if (0.f != alphaRev)
				{
					if (0.5f > alphaRev)
						BlitSrc32A(pDest, s_pRevLogo, kResX, kResX, kResY, alphaRev);
					else
					{
						BoxBlur32(g_renderTarget[0], s_pRevLogo, kResX, kResY, BoxBlurScale(((alphaRev-0.5f)*12.f)));
						BlitSrc32A(pDest, g_renderTarget[0], kResX, kResX, kResY, alphaRev);
					}
				}

				FadeFlash(pDest, fadeToBlack, fadeToWhite);
			}
			break;

		case 3:
			// Voxel ball
			{
				Ball_Draw(pDest, timer, delta);

				if (false == Ball_HasBeams())
					MulSrc32(pDest, s_pGreetingsVignette, kOutputSize); // FIXME: borrowed
				else
					SoftLight32(pDest, s_pBallVignette, kOutputSize);

				FadeFlash(pDest, fadeToBlack, fadeToWhite);
				
				// I think it looks slightly better without this vignette on top
				// MulSrc32A(pDest, s_pVignette06, kOutputSize);
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
					MixOver32(pDest, s_pNoooN, kOutputSize);

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
				SoftLight32(pDest, s_pNautilusVignette, kOutputSize);
				SoftLight32(pDest, s_pNautilusDirt, kOutputSize);
				FadeFlash(pDest, fadeToBlack, 0.f);

				Overlay32A(pDest, s_pNautilusCousteauRim1, kOutputSize);

				// just so we can add a little shakin'
				const uint32_t* pCousteau;
				const uint32_t* pCousteauRim;
				if (0 == Rocket::geti(trackCousteau))
				{
					pCousteau = s_pNautilusCousteau1;
					pCousteauRim = s_pNautilusCousteauRim1;
				}
				else
				{
					pCousteau = s_pNautilusCousteau2;
					pCousteauRim = s_pNautilusCousteauRim2;
				}

				Overlay32A(pDest, pCousteauRim, kOutputSize);

				float hBlur = Rocket::getf(trackCousteauHorzBlur);
				if (0.f != hBlur)
				{
					hBlur = BoxBlurScale(hBlur);
					HorizontalBoxBlur32(g_renderTarget[0], pCousteau, kResX, kResY, hBlur);
					pCousteau = g_renderTarget[0];
				}

				MixSrc32(pDest, pCousteau, kOutputSize);

				FadeFlash(pDest, 0.f, fadeToWhite);

				MixSrc32(pDest, s_pNautilusText, kOutputSize);
			}
			break;
      
		case 7:			
			// Close-up spike ball
			Spikey_Draw(pDest, timer, delta, true);
			MulSrc32(pDest, s_pCloseSpikeDirt, kOutputSize);
			Overlay32A(pDest, s_pSpikeyVignette, kOutputSize); 
			break;

		case 8:
			// Spike ball with title and group name (Bypass)
			Spikey_Draw(pDest, timer, delta, false);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);
			SoftLight32(pDest, s_pSpikeyBypass, kOutputSize);
			Sub32(pDest, s_pSpikeyVignette2, kOutputSize);
			Excl32(pDest, s_pSpikeyFullDirt, kOutputSize);
			MulSrc32A(pDest, s_pVignette06, kOutputSize);
			MixOver32(pDest, s_pSpikeyArrested, kOutputSize);
			Overlay32(pDest, s_pSpikeyVignette, kOutputSize);
			break;

		case 9:
			// Part of the 'tunnels' part
			{
				Tunnel_Draw(pDest, timer, delta);
				Sub32(pDest, s_pTunnelVignette2, kOutputSize);

				if (0 != Rocket::geti(trackShow2006))
					MixOver32(pDest, s_pMFX, kOutputSize);
			}
			break;

		case 10:
			// The 'under water' tunnel
			Sinuses_Draw(pDest, timer, delta);
			BlitAdd32A(pDest, s_pWaterPrismOverlay, kResX, kResX, kResY, saturatef(Rocket::getf(trackWaterLove)));
			MulSrc32(pDest, s_pWaterDirt, kOutputSize);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);
			break;

		case 11:
			// What was supposed to be the 'Aura for Laura' grid, but became boxy and intended for greetings
			{
				Laura_Draw(pDest, timer, delta);

				const int greetSwitch = Rocket::geti(trackGreetSwitch);

				Darken32_50(pDest, s_pGreetings[greetSwitch], kOutputSize);
				SoftLight32(pDest, s_pGreetingsDirt, kOutputSize);

				const auto yOffs = ((kResY-243)/2) + 227;
				const auto xOffs = 24; // ((kResX-263)/2) - 300;
				BlitSrc32(pDest + xOffs + yOffs*kResX, g_pXboxLogoTPB, kResX, 263, 243);

				Overlay32(pDest, s_pGreetingsVignette, kOutputSize);
			}
			break;

		case 12:
			{
				// TPB represent
				memset32(g_renderTarget[0], 0xffffff, kResX*kResY);

//				BlitSrc32(g_renderTarget[0] + ((kResX-800)/2) + ((kResY-600)/2)*kResX, g_pNytrikMexico, kResX, 800, 600);
//				memcpy(g_renderTarget[0], g_pNytrikTPB, kOutputBytes);

				MixSrc32(g_renderTarget[0], g_pNytrikTPB, kOutputSize);

				float blurTPB = Rocket::getf(trackBlurTPB);
				if (0.f != blurTPB)
				{
					blurTPB = BoxBlurScale(blurTPB);
					HorizontalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kResX, kResY, blurTPB);
				}

				MulSrc32(g_renderTarget[0], s_pNautilusVignette, kOutputSize); // FIXME: placeholder

				const float distortTPB = Rocket::getf(trackDistortTPB);
				const float distortStrengthTPB = Rocket::getf(trackDistortStrengthTPB);
				TapeWarp32(pDest, g_renderTarget[0], kResX, kResY, distortStrengthTPB, distortTPB);
			}
			break;

		case 13:
			{
				memset32(pDest, 0, kResX*kResY);

				const float discoGuys = saturatef(Rocket::getf(trackDiscoGuys));
				const unsigned xStart = (kResX-(8*128))>>1;
				const unsigned yOffs = ((kResY-128)>>1) + 16;
				for (int iGuy = 0; iGuy < 8; ++iGuy)
					BlitSrc32A(pDest + xStart + iGuy*128 + yOffs*kResX, s_pDiscoGuys[iGuy], kResX, 128, 128, discoGuys);

				// FIXME: this 1 pixel offset (X) matters, and that's entirely because it's a fucked up bitmap (waiting for Jade)
				BlitAdd32A(pDest + (((kResX-1000)/2)-1) + (yOffs+130)*kResX, s_pAreWeDone, kResX, 1000, 52, discoGuys);
			}
			break;

		default:
			FxBlitter_DrawTestPattern(pDest);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 	}

	// post fade/flash
	switch (effect)
	{
	case 1:
	case 2:
	case 3:
	case 6:
	case 8:
	case 10:
		// handled by effect/part
		break;

	default:
		FadeFlash(pDest, fadeToBlack, fadeToWhite);
	}

	return;
}
