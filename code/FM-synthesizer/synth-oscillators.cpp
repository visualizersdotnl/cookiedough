

/*
	Syntherklaas FM -- Oscillators.
*/

#include "synth-global.h"
#include "synth-oscillators.h"

// Wavetable(s)
#include "wavetable/wavKick808.h"  
#include "wavetable/wavSnare808.h" 
#include "wavetable/wavGuitar.h"
#include "wavetable/wavElecPiano.h"

namespace SFM
{
	// These parameters are tweaked to align the tone of the sample (using octave shift and a base frequency)
	static WavetableOscillator s_kickOsc(g_wavKick808, sizeof(g_wavKick808), 4, 233.08f); 
	static WavetableOscillator s_snareOsc(g_wavSnare808, sizeof(g_wavSnare808), 1, 440.f);
	static WavetableOscillator s_guitarOsc(g_wavGuitar, sizeof(g_wavGuitar), 6, 196.f);
	static WavetableOscillator s_elecPianoOsc(g_wavElecPiano, sizeof(g_wavElecPiano), -9, 32.7f);

	WavetableOscillator &getOscKick808()   { return s_kickOsc;   }
	WavetableOscillator &getOscSnare808()  { return s_snareOsc;  }
	WavetableOscillator &getOscGuitar()    { return s_guitarOsc; }
	WavetableOscillator &getOscElecPiano() { return s_elecPianoOsc; }
}
