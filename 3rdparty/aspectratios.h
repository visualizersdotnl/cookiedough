
/*
 *  This is a collection of motion picture & monitor/TV aspect ratios.
 *
 *  Notes:
 *  - A distinction is made between "film" and "theatre" aspect ratios.
 *    Film always refers to the ratio on the original negative.
 *    Theatre refers to either print (possibly matted) or (supported) theatre output ratio.
 *    If no distinction is made, it's either 1:1 or the latter.
 *
 *  Comments, corrections and additions are surprisingly welcome: ndewit@gmail.com
 *  And if you've got time to kill, read the referred articles, it's fun.
 *
 *
 *  Revision:
 *  - 25/01/2012 - Initial.
 *  - 25/01/2012 - Minor corrections (comments).
 *  - 25/01/2012 - Fixed Techniscope::kTheatre.
 *  - 27/01/2012 - Grouped Panavision, documented Todd-AO and fixed several (anamorphic) formats.
 *                 Added IMAX.
 *  - 22/11/2013 - Updated Technirama.
 *  - 12/12/2014 - Added Polyvision Ultra.
 *
 * (C) 2014 visualizers.nl
 */

#ifndef _ASPECTRATIOS_H_
#define _ASPECTRATIOS_H_

namespace AspectRatios
{
	// -- motion picture --
	
	// Academy (of Motion Picture Arts and Sciences)
	// - http://en.wikipedia.org/wiki/Academy_ratio
	// - http://www.dvdactive.com/editorial/articles/aspect-ratios-explained-part-one.html
	// - http://en.wikipedia.org/wiki/Aspect_ratio_(image)
	// - While "flat" technically also means 1.65:1 and 1.75:1, 1.85:1 is the de facto standard.
	namespace Academy
	{
		const float kSilent = 1.33f;
		const float kStandard = 1.37f;
		const float kFlat = 1.85f; 
		const float kAnamorphic = 2.35f;
		const float kAnamorphicPost1970 = 2.39f; // Subtly changed and still often referred to as "2.35".
	};

	// Polyvision (prem. 1927)
	// - http://en.wikipedia.org/wiki/Polyvision
	//
	// This format is still used by Kraftwerk on tour.
	//
	const float kPolyvision = 4.f;

	// Polyvision Ultra (12/2014)
	//
	// A format I made up myself, to be pushed into mainstream if I can help it.
	// Wikipedia article currently up for review.
	//
	const float kPolyvisionUltra = 2.f;
	
	// Cinerama (prem. 1952)
	// - http://en.wikipedia.org/wiki/Cinerama
	//
	// There is an inconsistency in the Cinerama Wikipedia article.
	//
	// First, there's this:
	//
	// ''According to film historian Martin Hart, in the original Cinerama system "the camera aspect 
	//   ratio [was] 2.59:1" with an "optimum screen image, with no architectural constraints, 
	//   [of] about 2.65:1, with the extreme top and bottom cropped slightly to hide anomalies".''
	//
	// And further on:
	//
	// ''Cinerama continued through the rest of the 1960s as a brand name used initially with the 
	//   Ultra Panavision 70 widescreen process (which yielded a similar 2.76:1 aspect ratio to the
	//   original Cinerama, although it did not simulate the 146 degree field of view)".''
	//
	// This is contradictory, as the first excerpt claims a different original aspect ratio.
	//
	// However, single-film Cinerama was typically recorded using an Ultra Panavision 70 camera.
	// This camera does actually yield an aspect ratio of 2.76:1.
	//
	namespace Cinerama
	{
		const float kOriginalFilm = 2.59f;
		const float kOriginalTheatre = 2.65f; // Bottom and top of film slightly cropped.
		const float kSingleCamera = 2.76f;    // Single 70mm film, expanded to fit all 3 frames.
	};
	
	// Kinopanorama (a.k.a. "Soviet Cinerama", prem. 1958)
	// - http://en.wikipedia.org/wiki/Kinopanorama
	// - Aspect ratio taken from: http://www.imdb.com/title/tt0485751/
	//
	// A surprisingly cheap looking preservation website claims:
	//
	// ''The KINOPANORAMA aspect ratio, which is between 2.55:1 and 2.77:1, renders the widest screen
	//   image of any contemporary film format.''
	//
	// Also, in Soviet Russia films project you!
	//
	const float kKinopanorama = 2.75f;
	
	// Cinemiracle (competitor to Cinerama, prem. 1958)
	// - http://en.wikipedia.org/wiki/Cinemiracle
	const float kCinemiracle = 2.59f;
	
	// CinemaScope (a.k.a. 'Scope, prem. 1953)
	// - http://en.wikipedia.org/wiki/CinemaScope
	// - Premiere: http://en.wikipedia.org/wiki/The_Robe_(film)
	namespace CinemaScope
	{
		const float kFilmScope = 2.55f;              // Using 'Scope aperture plate.
		const float kFilmFull = 2.66f;               // Using Full/Silent aperture plate.
		const float kTheatre = Academy::kAnamorphic; // Film is matted to 2.40:1, 2.39:1 or, usually, 2.35:1.
	};
	
	// Technirama (competitor to CinemaScope, prem. 1957)
	// - http://en.wikipedia.org/wiki/Technirama
	namespace Technirama
	{
		const float kFilm = 2.25f;
		const float kTheatre = Academy::kAnamorphic;
		const float kExperimental = 2.55f;
		// ^ kExperimental:
		// The 2008 DVD and Blu-ray Disc release of Walt Disney's Sleeping Beauty was shown at an aspect-ratio of 2.55:1 for the first time.
	};
	
	// VistaVision (prem. 1954)
	// - http://en.wikipedia.org/wiki/Vistavision
	namespace VistaVision
	{
		const float kFilm = 1.5f;
		const float kTheatreSmall = 1.66f;
		const float kTheatreMedium = Academy::kFlat;
		const float kTheatreLarge = 2.f;
	};

	// Todd-AO
	// - http://en.wikipedia.org/wiki/Todd-AO
	//
	// Todd-AO set the 70mm standard, soon after adapted by Panavision's Super 70.
	//
	const float kToddAO = 2.2f;
	
	// Super/Ultra Panavision (prem. 1957)
	// - http://en.wikipedia.org/wiki/Super_Panavision
	// - http://en.wikipedia.org/wiki/Ultra_Panavision_70
	// - http://en.wikipedia.org/wiki/Aspect_ratio_(image)
	//
	// Ultra Panavision was originally developed as a single-film Cinerama substitute.
	//
	namespace Panavision
	{
		const float kSuper70 = kToddAO;
		const float kUltra70 = 2.76f;     // With 1.25:1 anamorphic squeeze.
		const float kMGMCamera65 = 2.93f; // With 1.33:1 anamorphic squeeze.
	};
	
	// Techniscope (prem. 1960, revived in 1999 as MultiVision 235)
	// - http://en.wikipedia.org/wiki/Techniscope
	//
	// This is a budget format (half a 35mm negative per frame) "invented" by Technicolor Italia.
	//
	namespace Techniscope
	{
		const float kFilm = 2.33f;
		const float kTheatre = Academy::kAnamorphic;                 // Also for MultiVision 235.
		const float kTheatrePost1970 = Academy::kAnamorphicPost1970; // Techniscope after SMPTE-revision.
	};
	
	// IMAX
	// - http://en.wikipedia.org/wiki/IMAX
	const float kIMAX = 1.44f;
	
	// -- monitor/TV --

	const unsigned int kNumMonitorRatios = 8;
	const float kMonitors[kNumMonitorRatios] = {
		1.33f, //   4:3 - Standard.
		1.25f, //   5:4 - Standard.
		1.6f,  // 16:10 - Standard.
		1.78f, //  16:9 - Standard.
		2.37f, //  21:9 - Philips Cinema Display.
		3.2f,  // 32:10 - NEC CRV43.
		4.f,   //  36:9 - Samsung LCD.
		2.1f,  // 2.1:1 - Planned futuristic aspect ratio for television and theatrical films.
	};
};

#endif // _ASPECTRATIOS_H_
