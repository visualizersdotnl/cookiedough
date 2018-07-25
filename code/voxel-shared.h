
// cookiedough -- voxel shared

#pragma once

__forceinline void normalize_vdeltas(float &dX, float &dY)
{
	const float rayHypo = 1.f/sqrtf(dX*dX + dY*dY);
	dX *= rayHypo; 
	dY *= rayHypo;
}

__forceinline void calc_fandeltas(float curAngle, float &dX, float &dY)
{
	dX = cosf(curAngle); 
	dY = sinf(curAngle);
	return normalize_vdeltas(dX, dY);
}
