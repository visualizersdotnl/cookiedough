
/*
	Syntherklaas FM -- Stateless oscillators.
*/

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

// Wavetable(s)
#include "wavetable/wavKick808.h"  
#include "wavetable/wavSnare808.h" 
#include "wavetable/wavGuitar.h"
#include "wavetable/wavElecPiano.h"
#include "wavetable/wavFemale.h"

namespace SFM
{
	/*
		The parameters are tweaked to align these specific samples (octave & semitone shift, plus base frequency)
		to the procedural oscillators. Obviously the information I have about these Googled samples is incorrect
		and it's just a bit of trying; on release we'll need our own samples and do some proper tuning and maybe
		find a bug or 2.

		But for now it is fine (FIXME).
	*/

	static WavetableOscillator s_kickOsc(g_wavKick808, sizeof(g_wavKick808), 0, 0, 2.f*233.08f);            // Tuned OK
	static WavetableOscillator s_snareOsc(g_wavSnare808, sizeof(g_wavSnare808), 2, 0, 2.f*220.f);           // Tuned OK
	static WavetableOscillator s_guitarOsc(g_wavGuitar, sizeof(g_wavGuitar), 1, -4, 2.f*130.81f);           // Tuned OK
	static WavetableOscillator s_elecPianoOsc(g_wavElecPiano, sizeof(g_wavElecPiano), -1, -3, 2.f*261.63f); // Tuned OK
	static WavetableOscillator s_femaleOsc(g_wavFemale, sizeof(g_wavFemale), -3, -1, 2.f*233.1f);           // Tuned OK (this sample just ain't very straight)
	
	WavetableOscillator &getOscKick808()   { return s_kickOsc;   }
	WavetableOscillator &getOscSnare808()  { return s_snareOsc;  }
	WavetableOscillator &getOscGuitar()    { return s_guitarOsc; }
	WavetableOscillator &getOscElecPiano() { return s_elecPianoOsc; }
	WavetableOscillator &getOscFemale()    { return s_femaleOsc; }
}
