
/*
	Syntherklaas FM -- Test functions (very useful when lacking a scope).
*/

#pragma once

namespace SFM
{
	// Writes a single oscillator to a RAW file
	void OscTest();

	// Plays alternating note to hunt down crackle & pop artifacts (FIXME: lower speed actually means faster alternating notes)
	void CrackleTest(float time, float speed);
}
