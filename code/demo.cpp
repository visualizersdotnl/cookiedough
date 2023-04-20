
// cookiedough -- Bypass featuring TPB. presents 'Arrested Development'

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
SyncTrack trackDiscoGuys, trackDiscoGuysAppearance[8];
SyncTrack trackCheapJoke;
SyncTrack trackShow1995, trackShow2006;
SyncTrack trackDirt;
SyncTrack trackScapeOverlay, trackScapeRevision, trackScapeFade;
SyncTrack trackDistortTPB, trackDistortStrengthTPB, trackBlurTPB, trackRibbonsTPB;
SyncTrack trackGreetSwitch;
SyncTrack trackCousteau;
SyncTrack trackCousteauHorzBlur;

SyncTrack trackShooting;
SyncTrack trackShootingX, trackShootingY, trackShootingAlpha;
SyncTrack trackShootingTrail;

SyncTrack trackWaterLove;

SyncTrack trackLoveBlurHorz;

SyncTrack trackCloseUpMoonraker, trackCloseUpMoonrakerText, trackCloseUpMoonrakerTextBlur;

SyncTrack trackSpikeDemoLogoIndex;

SyncTrack trackBloodEffectLogoBlend, trackCreditLogoBlend;

// boolean (here look, I'm starting to describe parameters!) to decide if the warp should be applied to the entire part or just the foreground
SyncTrack trackFullWarpTPB;

// --------------------

// credits logos (1280x568)
static uint32_t *s_pCredits[4] = { nullptr };
constexpr auto kCredX = 1280;
constexpr auto kCredY = 568;
static uint32_t *s_pComatron[5] = { nullptr };
static uint32_t *s_pSuperplek[5] = { nullptr };
static uint32_t *s_pJadeNytrik[5] = { nullptr };
static uint32_t *s_pErnstHot[5] = { nullptr }; 

// vignette re-used (TPB-06)
static uint32_t *s_pVignette06 = nullptr;

// Stars/NoooN + MFX text overlays, lens dirt & vignette (1280x720)
static uint32_t *s_pNoooN[4] = { nullptr };
static uint32_t *s_pMFX[4] = { nullptr };
static uint32_t *s_pTunnelFullDirt = nullptr;
static uint32_t *s_pTunnelVignette = nullptr;
static uint32_t *s_pTunnelVignette2 = nullptr;

// first spikey ball art
static uint32_t *s_pSpikeyFullDirt = nullptr;
static uint32_t *s_pSpikeyBypass = nullptr;
static uint32_t *s_pSpikeyArrested[4] = { nullptr };
static uint32_t *s_pSpikeyVignette = nullptr;
static uint32_t *s_pSpikeyVignette2 = nullptr;

// landscape art
static uint32_t *s_pGodLayer = nullptr;
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
static uint32_t *s_pCloseSpikeDirtRaker = nullptr;
static uint32_t *s_pCloseSpikeVignette = nullptr;
static uint32_t *s_pCloseSpikeVignetteForRaker = nullptr;
static uint32_t *s_pCloseSpike1961 = nullptr; // 442x152 px.

// under water tunnel art
static uint32_t *s_pWaterDirt = nullptr;
static uint32_t* s_pWaterPrismOverlay = nullptr;

// shooting star art
static uint32_t *s_pLenz = nullptr;

// ribbons (2160x720)
static uint32_t *s_pRibbons = nullptr;

// GPU joke (960x160)
static uint32_t *s_pGPUJoke = nullptr;

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
	trackDiscoGuysAppearance[0] = Rocket::AddTrack("demo:DiscoGuy1");
	trackDiscoGuysAppearance[1] = Rocket::AddTrack("demo:DiscoGuy2");
	trackDiscoGuysAppearance[2] = Rocket::AddTrack("demo:DiscoGuy3");
	trackDiscoGuysAppearance[3] = Rocket::AddTrack("demo:DiscoGuy4");
	trackDiscoGuysAppearance[4] = Rocket::AddTrack("demo:DiscoGuy5");
	trackDiscoGuysAppearance[5] = Rocket::AddTrack("demo:DiscoGuy6");
	trackDiscoGuysAppearance[6] = Rocket::AddTrack("demo:DiscoGuy7");
	trackDiscoGuysAppearance[7] = Rocket::AddTrack("demo:DiscoGuy8");

	trackShow1995 = Rocket::AddTrack("demo:Show1995");
	trackDirt = Rocket::AddTrack("demo:Dirt");
	trackShow2006 = Rocket::AddTrack("demo:Show2006");
	trackScapeOverlay = Rocket::AddTrack("demo:ScapeOverlay");
	trackScapeRevision = Rocket::AddTrack("demo:ScapeRev");
	trackScapeFade = Rocket::AddTrack("demo:ScapeFade");
	trackDistortTPB = Rocket::AddTrack("demo:DistortTPB");
	trackDistortStrengthTPB = Rocket::AddTrack("demo:DistortStrengthTPB");
	trackBlurTPB = Rocket::AddTrack("demo:BlurTPB");
	trackRibbonsTPB = Rocket::AddTrack("demo:RibbonsX");
	trackGreetSwitch = Rocket::AddTrack("demo:GreetSwitch");
	trackCousteau = Rocket::AddTrack("demo:Cousteau");
	trackCousteauHorzBlur = Rocket::AddTrack("demo:CousteauHorzBlur");
	trackWaterLove = Rocket::AddTrack("demo:WaterLove");
	trackLoveBlurHorz = Rocket::AddTrack("demo:LoveBlurHorZ");

	trackShooting = Rocket::AddTrack("shootingStar:Enabled");
	trackShootingX = Rocket::AddTrack("shootingStar:X");
	trackShootingY = Rocket::AddTrack("shootingStar:Y");
	trackShootingAlpha = Rocket::AddTrack("shootingStar:A");
	trackShootingTrail = Rocket::AddTrack("shootingStar:Trail");

	trackCheapJoke = Rocket::AddTrack("demo:CheapGPU");

	trackSpikeDemoLogoIndex = Rocket::AddTrack("demo:MainLogoIndex");
	trackCreditLogoBlend = Rocket::AddTrack("demo:CreditAnimBlend");

	trackFullWarpTPB = Rocket::AddTrack("demo:FullWarpTPB");

	// FIXME: this one might not belong in this file for but for compositing reasons it does
	trackCloseUpMoonraker = Rocket::AddTrack("closeSpike:Moonraker");
	trackCloseUpMoonrakerText = Rocket::AddTrack("closeSpike:MoonrakerText");
	trackCloseUpMoonrakerTextBlur = Rocket::AddTrack("closeSpike:MoonrakerBlur");

	// load credits logos (1280x568)
	s_pCredits[0] = Image_Load32("assets/credits/Credits_Tag_Superplek_outlined.png");
	s_pCredits[1] = Image_Load32("assets/credits/Credits_Tag_Comatron_Featuring_Celin_outlined.png");
	s_pCredits[2] = Image_Load32("assets/credits/Credits_Tag_Jade_outlined.png");
	s_pCredits[3] = Image_Load32("assets/credits/Credits_Tag_ErnstHot_outlined_new.png");
	for (auto *pImg : s_pCredits)
		if (nullptr == pImg)
			return false;
	
	s_pComatron[0] = Image_Load32("assets/credits/comatron_anim/comatron_1.png");
	s_pComatron[1] = Image_Load32("assets/credits/comatron_anim/comatron_2.png");
	s_pComatron[2] = Image_Load32("assets/credits/comatron_anim/comatron_3.png");
	s_pComatron[3] = Image_Load32("assets/credits/comatron_anim/comatron_4.png");
	s_pComatron[4] = Image_Load32("assets/credits/comatron_anim/comatron_5.png");
	for (auto *pImg : s_pComatron)
		if (nullptr == pImg)
			return false;

	s_pSuperplek[0] = Image_Load32("assets/credits/animplek/animplek0.png");
	s_pSuperplek[1] = Image_Load32("assets/credits/animplek/animplek1.png");
	s_pSuperplek[2] = Image_Load32("assets/credits/animplek/animplek2.png");
	s_pSuperplek[3] = Image_Load32("assets/credits/animplek/animplek3.png");
	s_pSuperplek[4] = Image_Load32("assets/credits/animplek/animplek4.png");
	for (auto *pImg : s_pSuperplek)
		if (nullptr == pImg)
			return false;

	s_pJadeNytrik[0] = Image_Load32("assets/credits/jade&nytrik/jade&nytrik0.png");
	s_pJadeNytrik[1] = Image_Load32("assets/credits/jade&nytrik/jade&nytrik1.png");
	s_pJadeNytrik[2] = Image_Load32("assets/credits/jade&nytrik/jade&nytrik2.png");
	s_pJadeNytrik[3] = Image_Load32("assets/credits/jade&nytrik/jade&nytrik3.png");
	s_pJadeNytrik[4] = Image_Load32("assets/credits/jade&nytrik/jade&nytrik4.png");
	for (auto *pImg : s_pJadeNytrik)
		if (nullptr == pImg)
			return false;

	s_pErnstHot[0] = Image_Load32("assets/credits/animhot0/animhot0.png");
	s_pErnstHot[1] = Image_Load32("assets/credits/animhot0/animhot1.png");
	s_pErnstHot[2] = Image_Load32("assets/credits/animhot0/animhot2.png");
	s_pErnstHot[3] = Image_Load32("assets/credits/animhot0/animhot3.png");
	s_pErnstHot[4] = Image_Load32("assets/credits/animhot0/animhot4.png");
	for (auto *pImg : s_pErnstHot)
		if (nullptr == pImg)
			return false;

	// load generic TPB-06 dirty vignette
	s_pVignette06 = Image_Load32("assets/demo/tpb-06-dirty-vignette-1280x720.png");
	if (nullptr == s_pVignette06)
		return false;
	
	// first appearance of the 'spikey ball' including the title and main group
	s_pSpikeyArrested[0] = Image_Load32("assets/spikeball/Layer 2023_1.png");
	s_pSpikeyArrested[1] = Image_Load32("assets/spikeball/Layer 2023_2.png");
	s_pSpikeyArrested[2] = Image_Load32("assets/spikeball/Layer 2023_3.png");
	s_pSpikeyArrested[3] = Image_Load32("assets/spikeball/Layer 2023_4.png");
	s_pSpikeyVignette = Image_Load32("assets/spikeball/Vignette_CoolFilmLook.png");
	s_pSpikeyVignette2 = Image_Load32("assets/spikeball/Vignette_Layer02_inverted.png");
	s_pSpikeyBypass = Image_Load32("assets/spikeball/SpikeyBall_byPass_BG_Overlay.png");
	s_pSpikeyFullDirt = Image_Load32("assets/spikeball/nytrik-TheYearWas_Overlay_LensDirt.jpg");
	const bool spikeyArresteds = nullptr == s_pSpikeyArrested[0] || nullptr == s_pSpikeyArrested[1] || nullptr == s_pSpikeyArrested[2] || nullptr == s_pSpikeyArrested[3];
	if (true == spikeyArresteds || nullptr == s_pSpikeyBypass || nullptr == s_pSpikeyFullDirt || nullptr == s_pSpikeyVignette || nullptr == s_pSpikeyVignette2)
		return false;

	// NoooN et cetera
	s_pNoooN[0] = Image_Load32("assets/tunnels/layer 1995_1.png");
	s_pNoooN[1] = Image_Load32("assets/tunnels/layer 1995_2.png");
	s_pNoooN[2] = Image_Load32("assets/tunnels/layer 1995_3.png");
	s_pNoooN[3] = Image_Load32("assets/tunnels/layer 1995_4.png");
	s_pMFX[0] = Image_Load32("assets/tunnels/layer 2006_1.png");
	s_pMFX[1] = Image_Load32("assets/tunnels/layer 2006_2.png");
	s_pMFX[2] = Image_Load32("assets/tunnels/layer 2006_3.png");
	s_pMFX[3] = Image_Load32("assets/tunnels/layer 2006_4.png");
	s_pTunnelFullDirt = Image_Load32("assets/tunnels/nytrik-TheYearWas_Overlay_LensDirt.png");
	s_pTunnelVignette = Image_Load32("assets/tunnels/Vignette_CoolFilmLook.png");;
	s_pTunnelVignette2 = Image_Load32("assets/tunnels/Vignette_Layer02_inverted.png");

	const bool NoooN = nullptr == s_pNoooN[0] || nullptr == s_pNoooN[1] || nullptr == s_pNoooN[2] || nullptr == s_pNoooN[3];
	const bool MFX = nullptr == s_pMFX[0] || nullptr == s_pMFX[1] || nullptr == s_pMFX[2] || nullptr == s_pMFX[3];

	if (true == NoooN || true == MFX || nullptr == s_pTunnelFullDirt || nullptr == s_pTunnelVignette || nullptr == s_pTunnelVignette2)
		return false;
	
	// landscape
	s_pGodLayer = Image_Load32("assets/demo/nytrik-god-layer-720p.png"); 
	s_pRevLogo = Image_Load32("assets/scape/revision-logo_white.png");
	if (nullptr == s_pGodLayer || nullptr == s_pRevLogo)
		return false;

	// voxel ball
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
//	s_pAreWeDone = Image_Load32("assets/demo/are-we-done-1000x52.png");
	s_pAreWeDone = Image_Load32("assets/demo/are-we-done-1100x57.png");
	if (nullptr == s_pAreWeDone)
		return false;

	// close-up 'spikey' 
	s_pCloseSpikeDirtRaker = Image_Load32("assets/closeup/raker-LensDirt5_invert.png");
	s_pCloseSpikeVignetteForRaker = Image_Load32("assets/closeup/VignetteForRaker.png");
	s_pCloseSpikeVignette = Image_Load32("assets/closeup/Vignette_CoolFilmLook.png");
	s_pCloseSpike1961 = Image_Load32("assets/closeup/raker_textSmall.png"); // 624x115
	if (nullptr == s_pCloseSpikeDirtRaker || nullptr == s_pCloseSpikeVignette || nullptr == s_pCloseSpikeVignetteForRaker || nullptr == s_pCloseSpike1961)
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

	// ribbons
	s_pRibbons = Image_Load32("assets/demo/ribbons.png");
	if (nullptr == s_pRibbons)
		return false;

	// making fun of competition machine
	s_pGPUJoke = Image_Load32("assets/demo/GPU-joke.png");
	if (nullptr == s_pGPUJoke)
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

// blend blood logos from zero to full ([0..3]) -- uses g_renderTarget[3]!
static uint32_t *BloodBlend(float blend, uint32_t *pLogos[4])
{
	VIZ_ASSERT(nullptr != pLogos);

	uint32_t *pTarget = g_renderTarget[3];

	const float factor = fmodf(blend, 1.f);
	const uint8_t iFactor = uint8_t(255.f*factor);

	// we're going to do this the stupid way
	if (blend >= 0.f && blend < 1.f)
	{
		memcpy(pTarget, pLogos[0], kOutputBytes);
		Mix32(pTarget, pLogos[1], kOutputSize, iFactor);	
	}
	else if (blend >= 1.f && blend < 2.f)
	{
		memcpy(pTarget, pLogos[1], kOutputBytes);
		Mix32(pTarget, pLogos[2], kOutputSize, iFactor);	
	}
	else if (blend >= 2.f && blend < 3.f)
	{
		memcpy(pTarget, pLogos[2], kOutputBytes);
		Mix32(pTarget, pLogos[3], kOutputSize, iFactor);	
	}
	else if (blend >= 3.f)
	{
		return pLogos[3];
	}

	return pTarget;
}

// blend credit anim. logos from zero to full ([0..4]) -- uses g_renderTarget[3]!
// FIXME: collapse with function above, it does exactly the same, except that the resolution is different
static uint32_t *CreditBlend(float blend, uint32_t *pLogos[5])
{
	VIZ_ASSERT(nullptr != pLogos);

	uint32_t *pTarget = g_renderTarget[3];

	const float factor = fmodf(blend, 1.f);
	const uint8_t iFactor = uint8_t(255.f*factor);

	constexpr size_t kCreditImgSize = kCredX*kCredY;
	constexpr size_t kCreditImgBytes = kCreditImgSize*sizeof(uint32_t);

	// we're going to do this the stupid way
	if (blend >= 0.f && blend < 1.f)
	{
		memcpy(pTarget, pLogos[0], kCreditImgBytes);
		Mix32(pTarget, pLogos[1], kCreditImgSize, iFactor);
	}
	else if (blend >= 1.f && blend < 2.f)
	{
		memcpy(pTarget, pLogos[1], kCreditImgBytes);
		Mix32(pTarget, pLogos[2], kCreditImgSize, iFactor);
	}
	else if (blend >= 2.f && blend < 3.f)
	{
		memcpy(pTarget, pLogos[2], kCreditImgBytes);
		Mix32(pTarget, pLogos[3], kCreditImgSize, iFactor);
	}
	else if (blend >= 3.f && blend < 4.f)
	{
		memcpy(pTarget, pLogos[3], kCreditImgBytes);
		Mix32(pTarget, pLogos[4], kCreditImgSize, iFactor);
	}
	else if (blend >= 4.f)
	{
		return pLogos[4];
	}

	return pTarget;
}

bool Demo_Draw(uint32_t *pDest, float timer, float delta)
{
	// update sync.
#if defined(SYNC_PLAYER)
	if (false == Rocket::Boost())
		return false; // demo is over!
#else
	Rocket::Boost();
#endif

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
//			SoftLight32(pDest, s_pBallVignette, kOutputSize);
			FadeFlash(pDest, fadeToBlack, fadeToWhite);

			// FIXME: placeholder
			SoftLight32A(pDest, s_pCloseSpikeVignette, kOutputSize);

			// curious but might be grunge enough for this part
			MulSrc32A(pDest, s_pVignette06, kOutputSize);
			break;
	
		case 2:
			{
				// Introduction: landscape
				Landscape_Draw(pDest, timer, delta);

				const float scapeFade = saturatef(Rocket::getf(trackScapeFade));
				FadeFlash(pDest, scapeFade, 0.f);

				// shooting star (or what has to pass for it)
				// this is the charm of a hack made possible by Rocket
				// FIXME: really shouldn't be using threaded blit function(s) here
				if (1 == Rocket::geti(trackShooting))
				{
					int xPos = Rocket::geti(trackShootingX);
					int yPos = Rocket::geti(trackShootingY);
					float alpha = Rocket::getf(trackShootingAlpha);

					const unsigned offset = yPos*kResX + xPos;
					BlitAdd32A(pDest + offset, s_pLenz, kResX, kLenzSize, kLenzSize, alpha);

					int trail = Rocket::geti(trackShootingTrail);
					if (trail > 0)
					{
						const int xStep = kLenzSize/16;
						const int yStep = 1;
						const float alphaStep = alpha/trail;

						for (int iTrail = 0; iTrail < trail; ++iTrail)
						{
							xPos += xStep; // simply assuming we're going from right to left...
							yPos -= yStep; // ... and from top to bottom
							alpha -= alphaStep;

							const unsigned offset = yPos*kResX + xPos;
							BlitAdd32A(pDest + offset, s_pLenz, kResX, kLenzSize, kLenzSize, alpha);
						}
					}
				}

				// add overlay
				const float overlayAlpha = saturatef(Rocket::getf(trackScapeOverlay));
				if (0.f != overlayAlpha)
					BlitAdd32A(pDest, s_pGodLayer, kResX, kResX, kResY, overlayAlpha);

				// add Revision logo
				const float alphaRev = saturatef(Rocket::getf(trackScapeRevision));
				if (0.f != alphaRev)
				{
					if (alphaRev < 0.314f)
					{
						const float easeA = easeOutElasticf(alphaRev)*kGoldenAngle;
						const float easeB = easeInBackf(alphaRev)*kGoldenRatio;
						TapeWarp32(g_renderTarget[0], s_pRevLogo, kResX, kResY, easeA, easeB);
						BlitSrc32A(pDest, g_renderTarget[0], kResX, kResX, kResY, alphaRev);
					}
					else
					{
						BoxBlur32(g_renderTarget[0], s_pRevLogo, kResX, kResY, BoxBlurScale(((alphaRev-0.314f)*k2PI)));
						BlitSrc32A(pDest, g_renderTarget[0], kResX, kResX, kResY, alphaRev);
					}
				}

				FadeFlash(pDest, fadeToBlack, fadeToWhite);

				// FIXME: placeholder
				SoftLight32A(pDest, s_pCloseSpikeVignette, kOutputSize);
			}
			break;

		case 3:
			// Voxel ball
			{
				Ball_Draw(pDest, timer, delta);

				if (false == Ball_HasBeams())
					MulSrc32(pDest, s_pGreetingsVignette, kOutputSize); // FIXME: borrowed/placeholder
				else
					SoftLight32(pDest, s_pBallVignette, kOutputSize);

				FadeFlash(pDest, fadeToBlack, fadeToWhite);

				// I'm unsure how this looks (might need tweaking), but let's just try it on the beams version to "grime" it up a little
				if (true == Ball_HasBeams())				
					MulSrc32A(pDest, s_pVignette06, kOutputSize);
			}
			break;

		case 4:
			// Tunnels (also see case 9)
			{
				Tunnelscape_Draw(pDest, timer, delta);

				Sub32(pDest, s_pTunnelVignette2, kOutputSize);

				MixSrc32(pDest, s_pTunnelFullDirt, kOutputSize);

				// FIXME: belongs to the old lens dirt overlay; remove, or?
//				const float dirt = Rocket::getf(trackDirt);
//				if (0.f == dirt)
//				{
//					Excl32(pDest, s_pTunnelFullDirt, kOutputSize);
//				}
//				else
//				{
//					BoxBlur32(g_renderTarget[0], s_pTunnelFullDirt, kResX, kResY, BoxBlurScale(dirt));
//					Excl32(pDest, g_renderTarget[0], kOutputSize);
//				}

				const float show1995 = clampf(0.f, 3.f, Rocket::getf(trackShow1995));
				if (show1995 > 0.f)
					MixOver32(pDest, BloodBlend(show1995, s_pNoooN), kOutputSize);

				Overlay32(pDest, s_pTunnelVignette, kOutputSize);
			}
			break;
		
		case 5:
			// Plasma and credits
			{
				Plasma_Draw(pDest, timer, delta);

				const int iLogo = clampi(0, 4, Rocket::geti(trackCreditLogo));
				if (0 != iLogo)
				{
					const float logoBlend = clampf(0.f, 4.f, Rocket::getf(trackCreditLogoBlend));
					if (true) // (1 == iLogo || 2 == iLogo) // Superplek & Comatron (ordered after s_pCredits)
					{
						uint32_t **pLogos;
						switch (iLogo-1) // again, ordered after s_pCredits
						{
						case 0:
							pLogos = s_pSuperplek;
							break;

						case 1:
							pLogos = s_pComatron;
							break;

						case 2:
							pLogos = s_pJadeNytrik;
							break;

						case 3:
							pLogos = s_pErnstHot;
							break;

						default:
							pLogos = nullptr;
							VIZ_ASSERT(false);
						}

						// credit logo blit (animated)
						uint32_t *pCur = CreditBlend(logoBlend, pLogos);

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
					else
					{
						// credit logo blit (rest)
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
			}
			break;

		case 6:
			// Nautilus (Michiel, RIP)
			{
				Nautilus_Draw(pDest, timer, delta);
				SoftLight32(pDest, s_pNautilusVignette, kOutputSize);
				SoftLight32(pDest, s_pNautilusDirt, kOutputSize);
				FadeFlash(pDest, fadeToBlack, 0.f);

				// just so we can add a little shakin'
				const uint32_t *pCousteau;
				const uint32_t *pCousteauRim;
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

				// add rim overlay
				Overlay32A(pDest, pCousteauRim, kOutputSize);

				float hBlur = Rocket::getf(trackCousteauHorzBlur);
				if (0.f != hBlur)
				{
					hBlur = BoxBlurScale(hBlur);
					HorizontalBoxBlur32(g_renderTarget[0], pCousteau, kResX, kResY, hBlur);
					pCousteau = g_renderTarget[0];
				}

				// and now Jacques himself!
				MixSrc32(pDest, pCousteau, kOutputSize);

				FadeFlash(pDest, 0.f, fadeToWhite);

				MixSrc32(pDest, s_pNautilusText, kOutputSize);
			}
			break;
      
		case 7:			
			{
				// Close-up spike ball
				Spikey_Draw(pDest, timer, delta, true);

				// what follows ain't pretty
				const auto dirt = Rocket::geti(trackDirt);

				if (1 != dirt)
					MulSrc32(pDest, s_pSpikeyVignette, kOutputSize);

				if (1 == dirt)
				{
					// Moonraker
					const float raker = Rocket::getf(trackCloseUpMoonraker);
					const float rakerText = clampf(0.f, 2.f, Rocket::getf(trackCloseUpMoonrakerText));
					
					if (raker > 0.f)
					{
						MulSrc32(pDest, s_pCloseSpikeVignetteForRaker, kOutputSize);
						SoftLight32AA(pDest, s_pCloseSpikeDirtRaker, kOutputSize, raker);
						
						if (rakerText > 0.f && rakerText < 1.f)
						{						
							// this is shit slow, but it'll only last a short whole (FIXME: optimize for major release)
							memset32(g_renderTarget[2], 0, kOutputSize);
							BlitSrc32(g_renderTarget[2] + ((kResY-115)*kResX), s_pCloseSpike1961, kResX, 624, 115);
							SoftLight32AA(pDest, g_renderTarget[2], kOutputSize, rakerText); // <- this would be the function to make work on arbitrarily sized bitmaps
						}
						else if (rakerText >= 1.f)
						{
							// just (possibly) blur and blit

							const uint32_t *pText = s_pCloseSpike1961;
							const float rakerBlur = clampf(0.f, 100.f, Rocket::getf(trackCloseUpMoonrakerTextBlur));
							if (rakerBlur >= 1.f)
							{
								HorizontalBoxBlur32(g_renderTarget[3] /* just assuming this one is free */, pText, 624, 115, BoxBlurScale(rakerBlur));
								pText = g_renderTarget[3];
							}

							BlitSrc32(pDest + ((kResY-115)*kResX), pText, kResX, 624, 115);
						}
						

						FadeFlash(pDest, 0.f, fadeToWhite);
						Overlay32(pDest, s_pCloseSpikeDirtRaker, kOutputSize);
						FadeFlash(pDest, fadeToBlack, 0.f);
					}
				}
				else if (2 == dirt)
					SoftLight32AA(pDest, s_pGreetingsDirt, kOutputSize, 0.09f*kGoldenAngle); // FIXME: borrowed asset
				else if (3 == dirt)
					SoftLight32AA(pDest, s_pGreetingsDirt, kOutputSize, 0.075f*kGoldenAngle); // FIXME: borrowed asset

				if (1 != dirt)
					FadeFlash(pDest, fadeToBlack, fadeToWhite);
			}
			break;

		case 8:
			{
				const int logoIdx = clampi(0, 4, Rocket::geti(trackSpikeDemoLogoIndex));

				// Spike ball with title and group name (Bypass)
				Spikey_Draw(pDest, timer, delta, false);
				FadeFlash(pDest, fadeToBlack, fadeToWhite);
				SoftLight32(pDest, s_pSpikeyBypass, kOutputSize);
				Sub32(pDest, s_pSpikeyVignette2, kOutputSize);
				Excl32(pDest, s_pSpikeyFullDirt, kOutputSize);
				MulSrc32A(pDest, s_pVignette06, kOutputSize);

				if (0 != logoIdx)
					MixOver32(pDest, s_pSpikeyArrested[logoIdx-1], kOutputSize);
				
				Overlay32(pDest, s_pSpikeyVignette, kOutputSize);
			}
			break;

		case 9:
			// Part of the 'tunnels' part
			{
				Tunnel_Draw(pDest, timer, delta);
				Sub32(pDest, s_pTunnelVignette2, kOutputSize);

				const float show2006 = clampf(0.f, 3.f, Rocket::getf(trackShow2006));
				if (show2006 > 0.f)
					MixOver32(pDest, BloodBlend(show2006, s_pMFX), kOutputSize);
			}
			break;

		case 10:
			// The 'under water' tunnel
			{
				const float overlayA = saturatef(Rocket::getf(trackWaterLove));
				Sinuses_Draw(pDest, timer, delta);

				const uint32_t *pWaterOverlay = s_pWaterPrismOverlay;
				const float waterOverlayBlurHorz = clampf(0.f, 100.f, Rocket::getf(trackLoveBlurHorz));
				if (0.f != waterOverlayBlurHorz)
				{
					HorizontalBoxBlur32(g_renderTarget[0], pWaterOverlay, kResX, kResY, BoxBlurScale(waterOverlayBlurHorz));
					pWaterOverlay = g_renderTarget[0];
				}

				BlitAdd32A(pDest, pWaterOverlay, kResX, kResX, kResY, overlayA);

				if (0 != Rocket::geti(trackDirt))
					MulSrc32(pDest, s_pWaterDirt, kOutputSize);

				FadeFlash(pDest, fadeToBlack, fadeToWhite);
			}
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
				
				// first let's check if we want the simple (warp everything) version or the more sophisticated warp logo only version
				const bool warpAll = 0 != Rocket::geti(trackFullWarpTPB);

				if (false == warpAll)
				{
					// clear 2 targets, so we can composite them and only warp one
					memset32(g_renderTarget[0], 0xffffff, kOutputSize);
					memset32(pDest, 0xffffff, kOutputSize);

					// ribbon to layer 
					const auto ribX = clampi(0, kResX, Rocket::geti(trackRibbonsTPB));
					MixSrc32S(pDest, s_pRibbons + ribX, kResX, kResY-1, 2160); // FIXME

	//				BlitSrc32(g_renderTarget[0] + ((kResX-800)/2) + ((kResY-600)/2)*kResX, g_pNytrikMexico, kResX, 800, 600);
	//				memcpy(g_renderTarget[0], g_pNytrikTPB, kOutputBytes);

					// logo to layer
					MixSrc32(g_renderTarget[0], g_pNytrikTPB, kOutputSize);

					// blur logo
					float blurTPB = Rocket::getf(trackBlurTPB);
					if (0.f != blurTPB)
					{
						blurTPB = BoxBlurScale(blurTPB);
						HorizontalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kResX, kResY, blurTPB);
					}

					// distort logo
					const float distortTPB = Rocket::getf(trackDistortTPB);
					const float distortStrengthTPB = Rocket::getf(trackDistortStrengthTPB);
					TapeWarp32(g_renderTarget[1], g_renderTarget[0], kResX, kResY, distortStrengthTPB, distortTPB);

					// add logo on top of layer
					MixOver32(pDest, g_renderTarget[1], kOutputSize);
				}
				else
				{
					// some subtle re-use of the plasma marching effect
					Plasma_Draw(pDest, timer, delta);

					// logo to layer (FIXME: blit instead?)
					memset32(g_renderTarget[0], 0xffffff, kOutputSize);
					MixSrc32(g_renderTarget[0], g_pNytrikTPB, kOutputSize);

					// blur logo (V)
					float blurTPB = Rocket::getf(trackBlurTPB);
					if (0.f != blurTPB)
					{
						blurTPB = BoxBlurScale(blurTPB);
						VerticalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kResX, kResY, blurTPB);
					}

					// distort logo
					const float distortTPB = Rocket::getf(trackDistortTPB);
					const float distortStrengthTPB = Rocket::getf(trackDistortStrengthTPB);
					TapeWarp32(g_renderTarget[1], g_renderTarget[0], kResX, kResY, distortStrengthTPB, distortTPB);

					// add logo on top of layer
					MixOver32(pDest, g_renderTarget[1], kOutputSize);
				}

				// vignette
				MulSrc32(pDest, s_pNautilusVignette, kOutputSize); // FIXME: placeholder

			}
			break;

		case 13:
			{
				memset32(pDest, 0, kResX*kResY);

				const float discoGuys = saturatef(Rocket::getf(trackDiscoGuys));
				const float joke = saturatef(Rocket::getf(trackCheapJoke));

				if (discoGuys > 0.f)
				{
					const unsigned xStart = (kResX-(8*128))>>1;
					const unsigned yOffs = ((kResY-128)>>1) + 16;
					for (int iGuy = 0; iGuy < 8; ++iGuy)
					{
						// this gives me the opportunity to for ex. fade them in in order
						const float appearance = saturatef(Rocket::getf(trackDiscoGuysAppearance[iGuy]));

						BlitSrc32A(pDest + xStart + iGuy*128 + yOffs*kResX, s_pDiscoGuys[iGuy], kResX, 128, 128, discoGuys*smootherstepf(0.f, 1.f, appearance));

						if (discoGuys < 1.f)
						{
							uint32_t *pStrip = pDest + yOffs*kResX;
							HorizontalBoxBlur32(pStrip, pStrip, kResX, 128, BoxBlurScale((1.f-discoGuys)*k2PI*kGoldenAngle));
						}
					}

					// (semi-)full credits
//					BlitAdd32A(pDest + (((kResX-1000)/2)-1) + (yOffs+130)*kResX, s_pAreWeDone, kResX, 1000, 52, discoGuys);
					BlitAdd32A(pDest + (((kResX-1100)/2)-1) + (yOffs+130)*kResX, s_pAreWeDone, kResX, 1100, 57, discoGuys);
				}
				else if (joke > 0.f)
				{
					// they can't 'ford no GPU
					memset(pDest, 0, kOutputBytes);
					BlitSrc32A(pDest + ((kResX-960)/2) + (((kResY-160 )/2)*kResX), s_pGPUJoke, kResX, 960, 160, joke);
				}
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
	case 7:
	case 8:
	case 10:
		// handled by effect/part
		break;

	default:
		FadeFlash(pDest, fadeToBlack, fadeToWhite);
	}

	return true;
}
