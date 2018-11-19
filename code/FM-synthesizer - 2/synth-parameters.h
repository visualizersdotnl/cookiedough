
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

namespace SFM
{
	// Most global parameters; some are still "pulled" in place (FIXME)
	struct Parameters
	{
		// Master drive [0..N]
		float m_drive;

		// Modulator parameters [0..1]
		float m_modIndex;
		float m_modRatioC, m_modRatioM;

		void SetDefaults()
		{
			// Neutral
			m_drive = 1.f;

			// No FM
			m_modIndex = 0.f;
			m_modRatioC = 0.f;
			m_modRatioM = 0.f;
		}
	};
}
