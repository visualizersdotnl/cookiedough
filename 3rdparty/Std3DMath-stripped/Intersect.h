
/*
	Intersection test functions.

	FIXME:
	- Add 'is point in triangle' (using half-space (barycentric) check).
	- Add ray-AABB.
*/

#pragma once

namespace Std3DMath
{
	// FIXME: flesh out further
	struct Ray 
	{
		Vector3 origin;
		Vector3 direction;
		float t = -1.f; 
	};

	float DistancePointToLine(const Vector3 &lineDir, const Vector3 &lineOrigin, const Vector3 &point, Vector3 &outPoint);

	bool LineSphereIntersect(const Vector3 &lineDir, const Vector3 &lineOrigin, float lineLen,
		const Vector3 &spherePos, float sphereRadius,
		float &outT);

	bool RayTriangleIntersect(/* const */ Ray &ray, const Vector3 &V0, const Vector3 &V1, const Vector3 &V2, bool doubleSided);
}
