

/*
	Syntherklaas FM -- Oscillators.
*/

#include "synth-global.h"
#include "synth-oscillators.h"

// Wavetable(s)
// Not included in the workspace since Visual Studio 2017 kept crashing when I did
#include "wavetable/wavKick808.h" // 1 sec.
#include "wavetable/wavSnare808.h" // 0.2 sec.

namespace SFM
{
	/*
		Wavetable oscillator(s).
	*/

	class WavetableOscillator
	{
	public:
		WavetableOscillator(const uint8_t *pTable, unsigned length, unsigned speed = 1 /* Higher means slower */) :
			m_pTable(reinterpret_cast<const float*>(pTable))
,			m_length(length/sizeof(float))
,			m_rate(1.f/(float(m_length/kOscPeriod)*speed)) {}

	float Sample(float phase)
	{
		const float index = phase*m_rate;
		const unsigned from = unsigned(index);
		const unsigned to = from+1;
		const float delta = fracf(index);
		const float A = m_pTable[from%m_length];
		const float B = m_pTable[from%m_length];
		return lerpf<float>(A, B, delta);
		
//		const unsigned sample = unsigned(phase*m_rate);
//		return m_pTable[sample%m_length];
	}

	private:
		const float *m_pTable;
		const unsigned m_length;
		const float m_rate;
	};

	static WavetableOscillator s_kickOsc(s_wavKick808, sizeof(s_wavKick808));
	static WavetableOscillator s_snareOsc(s_wavSnare808, sizeof(s_wavSnare808), 12);

	float oscKick808(float phase)  { return s_kickOsc.Sample(phase); }
	float oscSnare808(float phase) { return s_snareOsc.Sample(phase); }
}
