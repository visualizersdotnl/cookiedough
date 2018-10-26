
/*
	Syntherklaas FM -- Frequency modulator (can also be used as LFO).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// This envelope causes a cutoff & resonance-like effect which can be effective to smooth out the modulator(s)
	struct IndexEnvelope
	{
		alignas(16) float buffer[kOscPeriod];

		struct Parameters
		{
			float frequency;
			float shape;      // [0..1]
			float curve;      // [0..N]
		};

		void Calculate(const Parameters &parameters);
	};

	struct Modulator
	{
		float m_frequency;
		float m_index;
		float m_pitch;
		unsigned m_sampleOffs;
		float m_phaseShift;
		IndexEnvelope m_envelope;
		
		void Initialize(unsigned sampleCount, float index, float frequency, float phaseShift /* In radians */, const IndexEnvelope::Parameters *pIndexEnvParams);
		float Sample(unsigned sampleCount, float brightness);

		// Version without envelope and brightness
		float SimpleSample(unsigned sampleCount);
	};
}
