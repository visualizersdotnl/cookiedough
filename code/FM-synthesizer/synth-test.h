
/*
	Syntherklaas FM -- Test functions (very useful when lacking a scope).
*/

#pragma once

namespace SFM
{
	// Writes a single oscillator to a RAW file
	void OscTest();

	// Triggers 2 notes quickly and repeats
	void ClickTest(float time, float speed);
}
