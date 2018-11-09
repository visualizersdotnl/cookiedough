
/*
	Resonant lowpass Butterworth filter (optimized), taken from http://www.musicdsp.org
	Written by 'Zxform', optimized by 'neotec'
	Adapted by syntherklaas.org for FM. BISON

	Parameters: 
		- Cutoff [0..Nyquist]
		- Resonance (Q) [0..1]
*/

#pragma once

class CFilterButterworth24dB
{
public:
    CFilterButterworth24dB(void);
	void Reset();
    void SetSampleRate(float fs);
    void Set(float cutoff, float q);
    float Run(float input);

private:
    float t0, t1, t2, t3;
    float coef0, coef1, coef2, coef3;
    float history1, history2, history3, history4;
    float gain;
    float min_cutoff, max_cutoff;
};
