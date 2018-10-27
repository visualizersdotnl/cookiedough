

/*
	Syntherklaas FM -- Oscillators.
*/

#include "synth-global.h"
#include "synth-oscillators.h"

// Wavetable(s)
#include "wavetable/wavKick808.h"  
#include "wavetable/wavSnare808.h" 
#include "wavetable//wavGuitar.h"

namespace SFM
{
	static WavetableOscillator s_kickOsc(g_wavKick808, sizeof(g_wavKick808), 4, 233.08f  /* A3-minor */);
	static WavetableOscillator s_snareOsc(g_wavSnare808, sizeof(g_wavSnare808), 1, 440.f /* A4 */);
	static WavetableOscillator s_guitarOsc(g_wavGuitar, sizeof(g_wavGuitar), 6, 196.f    /* G3 */); 

	WavetableOscillator &getOscKick808()  { return s_kickOsc;   }
	WavetableOscillator &getOscSnare808() { return s_snareOsc;  }
	WavetableOscillator &getOscGuitar()   { return s_guitarOsc; }
}
