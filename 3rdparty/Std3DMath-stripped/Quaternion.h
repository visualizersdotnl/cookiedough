
/*
	Basic (unit) quaternion implementation to describe & manipulate 3D rotation.

	Primary goal is compact storage and spherical interpolation (no gimbal lock).

	Despite their mythical status in the demoscene in the late 1990s, quaternions
	are actually "nothing more" than an axis-angle representation in disguise:

		q = [x, y, z, w] = [axis*sin(angle/2), cos(angle/2)]

	Important rules:
	- We always assume on interface level *only* that a quaternion is unit length, ergo: it is a rotation.
	- Multiplying quaternions (or rotations) isn't commutative, ergo A*B != B*A.
	- Multiplication is (generally) associative, however, so A*(B*C) == (A*B)*C.
	- When performing successive multiplication, re-normalize periodically to counteract numerical drift.
	- Quaternion *is* derived from Vector4, but as it has it's own operators you can not use Vector4's without
	  being explicit; this should gaurantee that when the quaternion is modified through it's own interface
	  it will always be unit length.

	References:
	- https://lisyarus.github.io/blog/posts/introduction-to-quaternions.html
	- A classic rant about Shoemake's spherical interpolation: http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
	- The Orange Duck: https://theorangeduck.com/page/exponential-map-angle-axis-angular-velocity
*/
 
#pragma once

#include "Math.h"

class Quaternion : public Vector4
{
public:
	static const Quaternion Identity();
	static const Quaternion AxisAngle(const Vector3 &axis, float angle);
	static const Quaternion YawPitchRoll(float yaw, float pitch, float roll);

	// Fast but non-linear velocity and less accurate (especially for large angles)
	static const Quaternion Nlerp(const Quaternion &from, const Quaternion &to, float T);

	// Spherical linear interpolation: constant angular velocity and more accurate, but more expensive (and not commutative!)
	static const Quaternion Slerp(const Quaternion &from, const Quaternion &to, float T);

	// For Log() and Exp():
	// When for ex. integrating *small* rotations the epsilon can be relaxed a lot to make these functions faster by effectively just
	// returning an approximation (branch predictor will likely do a good enough job ignoring the conditional jump)

	// Reduces to axis-angle form, where the result is the axis scaled by the half-angle 
	static const Vector3 Log(const Quaternion &quaternion, float epsilon = 1e-9f);

	// Performs the inverse of Log()
	static const Quaternion Exp(const Vector3 &vector, float epsilon = kAngleEpsilon*kAngleEpsilon); 

	static const Vector3 ScaledAngleAxis(const Quaternion &quaternion) { 
		return Log(quaternion)*2.f; // Compensate for half-angle
	}

	static const Quaternion ScaledAngleAxis(const Vector3 &vector) { 
		return Exp(vector*0.5f); // Compensate for full angle
	}

public:
	Quaternion() 
	{
		*this = Identity();
	}

	~Quaternion() {}

	// Non-explicit so it plays nice with Vector4.
	Quaternion(const Vector4 &V) : Vector4(V) {}

	const Quaternion operator *(const Quaternion &B) const
	{
		// Note that this can be considered "naive", if you know certain components to be zero, the multiplicative load drops
		// I've cross-checked this with several sources, one screw-up here is fatal
		return Quaternion(Vector4(
			w*B.x + x*B.w + y*B.z - z*B.y,
			w*B.y + y*B.w + z*B.x - x*B.z,
			w*B.z + z*B.w + x*B.y - y*B.x,
			w*B.w - x*B.x - y*B.y - z*B.z));
	}

private: 
	// These remain private (for now) as they serve to alter magnitude
	const Quaternion operator *(float B) const 
	{ 
		return Vector4::Mul(*this, Vector4(B)); 
	}

	Quaternion& operator *=(float B) { return *this = *this * B; }

public:
	float Angle() const
	{
		// Remember: w = cos(angle/2)
		const float clampedW = clampf(-1.f, 1.f, w); // Clamp to acos() domain (avoid NaN)
		return acosf(clampedW)*2.f; 
	}

	// Two quaternions are perpendicular if their vector parts are perpendicular (axis dot product is zero)
	bool Perpendicular(const Quaternion &B) const
	{
		return fabsf(x*B.x + y*B.y + z*B.z) < kEpsilon;
	}

	const Quaternion Normalized() const
	{
		return Vector4::Normalized();
	}

	// The conjugate *is* the inverse of a unit quaternion
	const Quaternion Conjugate() const
	{
		return Quaternion(Vector4(-x, -y, -z, w));
	}

	// Can be used to for ex. calculate rotational difference between A and B (= A.Inverse()*B)
	const Quaternion Inverse() const 
	{
		const float normSq = x*x + y*y + z*z + w*w; // Keep sign (no LengthSq())

		if (fabsf(normSq) < kEpsilon)
			return Identity();
		

		const float oneOverNormSq = 1.f/normSq;
		return Quaternion(Vector4(-x, -y, -z, w) * oneOverNormSq);
	}

	// Force rotation to one side of the hemisphere
	const Quaternion Abs() const
	{
		return (w >= 0.f)
			? *this 
			: *this * -1.f; // Flip components
	}

	const Quaternion Nlerp(const Quaternion &to, float T) const { return Nlerp(*this, to, T); }
	const Quaternion Slerp(const Quaternion &to, float T) const { return Slerp(*this, to, T); }
};
