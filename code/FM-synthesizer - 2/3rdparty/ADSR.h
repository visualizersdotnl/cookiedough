//
//  ADSR.h
//
//  Created by Nigel Redmon on 12/18/12.
//  EarLevel Engineering: earlevel.com
//  Copyright 2012 Nigel Redmon
//
//  For a complete explanation of the ADSR envelope generator and code,
//  read the series of articles by the author, starting here:
//  http://www.earlevel.com/main/2013/06/01/envelope-generators/
//
//  License:
//
//  This source code is provided as is, without warranty.
//  You may copy and distribute verbatim copies of this document.
//  You may modify and use this source code to create binary code for your own purposes, free or commercial.
//

//
// Adapted by syntherklaas.org for FM. BISON / FM. GENERALISSIMO, changes made:
// - Fixed warnings
// - Fixed process() to proceed to decay state immediately after attack to eliminate a click
// - Added attack level (variable instead of 1.0)
//

#ifndef ADRS_h
#define ADRS_h


class ADSR {
public:
	ADSR(void);
	~ADSR(void);
	float process(void);
    float getOutput(void);
    int getState(void);
	void gate(int on);
    void setAttackRate(float rate);
	void setAttackLevel(float level);
    void setDecayRate(float rate);
    void setReleaseRate(float rate);
	void setSustainLevel(float level);
    void setTargetRatioA(float targetRatio);
    void setTargetRatioDR(float targetRatio);
    void reset(void);

    enum envState {
        env_idle = 0,
        env_attack,
        env_decay,
        env_sustain,
        env_release,
		env_cooldown
    };

protected:
	int state;
	float output;
	float attackRate;
	float attackLevel;
	float decayRate;
	float releaseRate;
	float attackCoef;
	float decayCoef;
	float releaseCoef;
	float sustainLevel;
    float targetRatioA;
    float targetRatioDR;
    float attackBase;
    float decayBase;
    float releaseBase;
 
    float calcCoef(float rate, float targetRatio);
};

inline float ADSR::process() {
	switch (state) {
        case env_idle:
			output = 0.f;
            break;
        case env_attack:
            output = attackBase + output * attackCoef;
            if (output >= attackLevel) {
                output = attackLevel;
                state = env_decay;
            }
			else // Immediately go into decay state, eliminating a click when ADS is zero!
				break;
        case env_decay:
			output = decayBase + output * decayCoef;
			if (output <= sustainLevel) {
				output = sustainLevel;
				state = env_sustain;
			}
            break;
        case env_sustain:
            break;
        case env_release:
            output = releaseBase + output * releaseCoef;
            if (output <= 0.f) {
                output = 0.f;
                state = env_cooldown;
            }
			break;
		case env_cooldown:
			output = 0.f;
			state = env_idle;
			break;
	}
	return output;
}

inline void ADSR::gate(int gate) {
	if (gate)
		state = env_attack;
	else if (state != env_idle)
        state = env_release;
}

inline int ADSR::getState() {
    return state;
}

inline void ADSR::reset() {
    state = env_idle;
    output = 0.f;
}

inline float ADSR::getOutput() {
	return output;
}

#endif
