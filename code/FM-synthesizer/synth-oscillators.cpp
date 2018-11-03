

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
	// These parameters are tweaked to align the tone of the sample (using octave & semitone shift and a base frequency)
	// FIXME: either all these sample descriptions are lying or I am doing something wrong, but at least this is stable now
	static WavetableOscillator s_kickOsc(g_wavKick808, sizeof(g_wavKick808), 4, 0, 233.08f); // Tuned OK
	static WavetableOscillator s_snareOsc(g_wavSnare808, sizeof(g_wavSnare808), 1, 0, 440.f); // Tuned OK
	static WavetableOscillator s_guitarOsc(g_wavGuitar, sizeof(g_wavGuitar), 5, -3, 130.81f); // Tuned OK
	static WavetableOscillator s_elecPianoOsc(g_wavElecPiano, sizeof(g_wavElecPiano), 5, 4, 261.63f); // Tuned OK
	
	WavetableOscillator &getOscKick808()   { return s_kickOsc;   }
	WavetableOscillator &getOscSnare808()  { return s_snareOsc;  }
	WavetableOscillator &getOscGuitar()    { return s_guitarOsc; }
	WavetableOscillator &getOscElecPiano() { return s_elecPianoOsc; }
}
