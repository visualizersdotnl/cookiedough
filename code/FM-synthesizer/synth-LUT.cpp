
/*
	Syntherklaas FM -- Lookup tables.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

// For the Farey generator (C++11); can do away with it easily if it's an issue for a limited platform (you'd likely just precalculate nearly everything)
#include <vector>

namespace SFM
{
	alignas(16) float g_sinLUT[kOscPeriod];
	alignas(16) float g_noiseLUT[kOscPeriod];

	/*
		Farey sequence generator (ham-fisted but it does the job).
		This sequence gives the most sonically pleasing FM C:M ratios.

		I learned about this here: http://noyzelab.blogspot.com/2016/04/farey-sequence-tables-for-fm-synthesis.html
	*/

	const unsigned kFareyOrder = 15;

	alignas(16) unsigned g_CM_table[512][2];
	unsigned g_CM_table_size = -1;

	class Ratio
	{
	public:
		Ratio(double x, double y) : x(unsigned(x)), y(unsigned(y)) {}
		unsigned x, y;

		bool IsEqual(const Ratio &ratio) const
		{
			return ratio.x*y == ratio.y*x;
		}
	};

	static void GenerateFareySequence()
	{
		std::vector<Ratio> sequence;

		// First two terms are 0/1 and 1/n (we only need the latter) 
		double x1 = 0.0, y1 = 1.0, x2 = 1.0, y2 = kFareyOrder; 
		sequence.push_back({x2, y2});

		double x, y = 0.0; // For next terms to be evaluated 
		while (1.0 != y) 
		{ 
			// Use recurrence relation to find the next term 
			x = floor((y1 + kFareyOrder) / y2) * x2-x1; 
			y = floor((y1 + kFareyOrder) / y2) * y2-y1; 

			sequence.push_back({x, y});
  
			x1 = x2, x2 = x, y1 = y2, y2 = y;
		} 

		// Sort by magnitude first, then by carrier
		std::sort(sequence.begin(), sequence.end(), [](const Ratio &a, const Ratio &b) -> bool { return a.x*a.y < b.x*b.y; });
		std::sort(sequence.begin(), sequence.end(), [](const Ratio &a, const Ratio &b) -> bool { return a.x < b.x; });

		// Copy to LUT (sequence plus appended sequence with inverse ratios) & store size
		const size_t size = sequence.size();
		SFM_ASSERT(size*2 < 512);
		
		for (unsigned iRatio = 0; iRatio < sequence.size(); ++iRatio)
		{
			const Ratio &ratio = sequence[iRatio];

			g_CM_table[iRatio][0] = ratio.x;
			g_CM_table[iRatio][1] = ratio.y;
			g_CM_table[iRatio+size][0] = ratio.y;
			g_CM_table[iRatio+size][1] = ratio.x;

//			Log("Farey C:M = " + std::to_string(ratio.x) + ":" + std::to_string(ratio.y));
		}

//		Log("Generated " + std::to_string(size*2));
		g_CM_table_size = unsigned(size*2);
	}

	/*
		Detune table.

		Goes one octave up and one down in discrete steps (designed to be used with a potmeter).
	*/

	alignas(16) float g_detuneTab[24];
	unsigned g_detuneTabSize = sizeof(g_detuneTab)/sizeof(float);

	static void CalculateDetuneTable()
	{
		for (unsigned iHalf = 0; iHalf < 12; ++iHalf)
		{
			const unsigned semitone = iHalf*7;
			const float freqMod = powf(2.f, iHalf/12.f);
			g_detuneTab[11-iHalf] = 1.f/freqMod;
		}

		for (unsigned iHalf = 12; iHalf < 24; ++iHalf)
		{
			const unsigned semitone = (iHalf-12)*7;
			const float freqMod = powf(2.f, iHalf/12.f);
			g_detuneTab[iHalf] = freqMod;
		}

		// Dump
//		for (auto detune : g_detuneTab)
//		{
//			Log("Detune table entry: " + std::to_string(detune));
//		}
	}

	/*
		All calculation goes here.
		Depending on target platform/hardware I may want to dump this to disk/ROM.
	*/

	void CalculateLUTs()
	{
		// Detune table
		CalculateDetuneTable();

		// Generate FM C:M ratio table
		GenerateFareySequence();

#if 0
		// Plain sinus
		float angle = 0.f;
		const float angleStep = k2PI/kOscPeriod;
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_sinLUT[iStep] = sinf(angle);
			angle += angleStep;
		}
#endif

#if 1
		/* 
			Gordon-Smith oscillator
			Allows for frequency changes with minimal artifacts (first-order filter)
		*/

		const float frequency = 1.f;
		const float theta = k2PI*frequency/kOscPeriod;
		const float epsilon = 2.f*sinf(theta/2.f);
		
		float N, prevN = sinf(-1.f*theta);
		float Q, prevQ = cosf(-1.f*theta);

		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			Q = prevQ - epsilon*prevN;
			N = epsilon*Q + prevN;
			prevQ = Q;
			prevN = N;
			g_sinLUT[iStep] = Clamp(N);
		}
#endif

		// White noise (Mersenne-Twister)
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_noiseLUT[iStep] = -1.f + 2.f*mt_randf();
		}
	}
}
