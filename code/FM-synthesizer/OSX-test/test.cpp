
/*
	Syntherklaas FM: hacky OSX stub to show Pieter v/d Meer something in 30 minutes?
*/

#include "FM_BISON.h"

int main(int argC, char **argV)
{
	// FIXME: try to dump a sample, for now			
	Syntherklaas_Create();
	Syntherklaas_Render(nullptr, 0.f, 0.f);
	Syntherklaas_Destroy();

	return 0;

}
