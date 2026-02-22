

/*
	Intersection test functions.
*/

#include "Intersect.h"
#include "Math.h"

namespace Std3DMath
{
	float DistancePointToLine(const Vector3 &lineDir, const Vector3 &lineOrigin, const Vector3 &point, Vector3 &outPoint)
	{
		const Vector3 toLine = point - lineOrigin;
		const Vector3 projected = toLine.Project(lineDir); // Will normalize lineDir, which I'd rather assert on top since the name infers it

		// This is what you're usually after: closest point on the actual line
		outPoint = lineOrigin + projected;
		
		// And this is literally how far we're removed from it (FIXME: a lot of practical cases can work with the squared distance)
		return (toLine-projected).Length();
	}

	bool LineSphereIntersect(const Vector3 &lineDir, const Vector3 &lineOrigin, float lineLen,
		const Vector3 &spherePos, float sphereRadius,
		float &outT)
	{
		const Vector3 toSphere = spherePos - lineOrigin;

		// Scalar projection gives us the component (magnitude), which is all we need
		const float projLen = toSphere*lineDir;
		
		// Early rejection (behind or beyond)
		if (projLen < kEpsilon || projLen > lineLen)
			return false;		

		// Distance from sphere center to line
		const float distSq =
			toSphere.LengthSq() - projLen*projLen;

		const float radiusSq = sphereRadius*sphereRadius;
		if (distSq > radiusSq)
			return false;
		
		const float offset = sqrtf(radiusSq - distSq); // To sphere surface along line
		outT = projLen-offset;

		return true;
	}

	// Using Möller–Trumbore algorithm (see https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf)
	// FIXME: make it optional to pass back U and V (using std::optional<T>)
	bool RayTriangleIntersect(/* const */ Ray &ray, const Vector3 &V0, const Vector3 &V1, const Vector3 &V2, bool doubleSided)
	{
		assert(-1.f == ray.T);

		// Two triangle edges
		const Vector3 BA = V1-V0;
		const Vector3 CA = V2-V0;

		// Find out how ray and plane correlate (determinant)
		const Vector3 vDet = Vector3::Cross(ray.direction, CA);
		const float rayPlaneDet = BA*vDet;

		if (false == doubleSided)
		{
			// Perpendicular or behind?
			if (rayPlaneDet < kEpsilon)
				return false;

			// Note: from now on we know that the scalar determinant is positive, so the following calculations can be
			// checked outside of unit domain and the divisions moved all the way down (mul. w/invDet), however since
			// the performance gain is close to nothing on a modern CPU I just keep it as-is

			// When performing this in batches (raytracing for ex.) please just consider performing these tests in-place
			// for further optimization (architecture, local constraints)
		}
		else 
		{
			// Perpendicular?
			if (fabsf(rayPlaneDet) < kEpsilon)
				return false;
		}

		const float invDet = 1.f/rayPlaneDet;
		const Vector3 vT = V0-ray.origin; // Basis

		// Calculate/evaluate U
		const float U = invDet*(vT*vDet);
		if (U < 0.f || U > 1.f)
			return false;

		// ... now V
		const Vector3 vQ = Vector3::Cross(vT, BA);
		const float V = invDet*(ray.direction*vQ);
		if (V < 0.f || U+V > 1.f) // <- Remember, barycentrics form a weighted sum
			return false;

		// We're inside the triangle (within barycentric bounds): solve for T
		const float T = invDet*(CA*vQ);
		if (T > kEpsilon)
		{
			ray.T = T;
			return true;
		}

		// It's a line intersection
		return false;
	}

	// Naive implementation as reference (code never reached so eliminated *but* evaluated)
	static bool RayTriangleIntersect_Naive(/* const */ Ray &ray, const Vector3 &V0, const Vector3 &V1, const Vector3 &V2, bool doubleSided)
	{
		assert(-1.f == ray.T);

		// Calculate triangle (plane) normal (predominant CCW order assumed)
		const Vector3 BA = V1-V0;
		const Vector3 CB = V2-V1;
		const Vector3 faceNormal = Vector3::Cross(BA, CB);

		// Find out how ray and plane correlate
		const float rayPlaneDet = ray.direction*faceNormal;

		if (false == doubleSided)
		{
			// Perpendicular or behind?
			if (rayPlaneDet < kEpsilon)
				return false;
		}
		else 
		{
			// Perpendicular?
			if (fabsf(rayPlaneDet) < kEpsilon)
				return false;
		}

		// Solve for T (distance ray to V0/plane) and we've got our intersection point
		const float T = (faceNormal*(V0-ray.origin)) / rayPlaneDet;
		if (T <= kEpsilon)
			return false; // Behind origin

		const Vector3 intersect = ray.origin + T*ray.direction; // On plane

		// Calculate third edge and intersection deltas
		const Vector3 AC = V0-V2;
		const Vector3 dVA = intersect-V0;
		const Vector3 dVB = intersect-V1;
		const Vector3 dVC = intersect-V2;

		// If the vectors perpendicular to the edge and intersection deltas face in the same direction...
		// Note: this is a naive implementation otherwise the evaluations below would be nested or early-out
		const bool isInsideBA = Vector3::Cross(BA, dVA)*faceNormal > 0.f;
		const bool isInsideCB = Vector3::Cross(CB, dVB)*faceNormal > 0.f;
		const bool isInsideAC = Vector3::Cross(AC, dVC)*faceNormal > 0.f;

		if (isInsideBA && isInsideCB && isInsideAC)
		{
			ray.T = T;
			return true;
		}
		
		return false;
	}
}
