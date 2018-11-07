
/*
	Syntherklaas FM -- Stateless oscillators.
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
	/*
		The parameters are tweaked to align these specific samples (octave & semitone shift, plus base frequency)
		to the procedural oscillators. Obviously the information I have about these Googled samples is incorrect
		and it's just a bit of trying; on release we'll need our own samples and do some proper tuning and maybe
		find a bug or 2.

		But for now it is fine (FIXME).
	*/

	static WavetableOscillator s_kickOsc(g_wavKick808, sizeof(g_wavKick808), 0, 0, 233.08f);            // Tuned OK
	static WavetableOscillator s_snareOsc(g_wavSnare808, sizeof(g_wavSnare808), 2, 0, 220.f);           // Tuned OK
	static WavetableOscillator s_guitarOsc(g_wavGuitar, sizeof(g_wavGuitar), 1, -4, 130.81f);           // Tuned OK
	static WavetableOscillator s_elecPianoOsc(g_wavElecPiano, sizeof(g_wavElecPiano), -2, -3, 261.63f); // Tuned OK
	
	WavetableOscillator &getOscKick808()   { return s_kickOsc;   }
	WavetableOscillator &getOscSnare808()  { return s_snareOsc;  }
	WavetableOscillator &getOscGuitar()    { return s_guitarOsc; }
	WavetableOscillator &getOscElecPiano() { return s_elecPianoOsc; }
}
