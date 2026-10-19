
/*
	Intersection test functions.

	FIXME:
	- Add 'vector from point on line to closest point to other line'.
	- Add 'is point in triangle' (using half-space (barycentric) check).
	- Add ray-AABB.
*/

#pragma once

namespace Std3DMath
{
	struct Ray 
	{
		Vector3 origin;
		Vector3 direction;
	};

	// FIXME: could overload, return boolean and number in a tuple, but there's such a thing as 'too much'
	constexpr float kNoIntersect = -1.f;

	float DistancePointToRay(const Ray &ray, const Vector3 point, Vector3 &outPoint);
	
	S3D_INLINE const std::tuple<float, const Vector3> DistancePointToRay(const Ray &ray, const Vector3 point)
	{
		Vector3 vIntersect;

		return {
			DistancePointToRay(ray, point, vIntersect),
			vIntersect
		};
	}

	bool LineSphereIntersect(const Vector3 &lineDir, const Vector3 &lineOrigin, float lineLen,
		const Vector3 &spherePos, float sphereRadius,
		float &outT);

	S3D_INLINE const std::tuple<bool, float> LineSphereIntersect(const Ray &lineOrig, float lineLen, const Vector3 &spherePos, float sphereRadius)
	{
		float t = kNoIntersect;

		return { 
			LineSphereIntersect(lineOrig.direction, lineOrig.origin, lineLen, spherePos, sphereRadius, t),
			t 
		};
	}

	float RayTriangleIntersect(const Ray &ray, const Vector3 &V0, const Vector3 &V1, const Vector3 &V2, bool doubleSided);
}
