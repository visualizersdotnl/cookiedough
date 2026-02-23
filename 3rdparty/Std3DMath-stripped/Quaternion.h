
/*
	Basic (unit) quaternion implementation to describe & manipulate 3D rotation.

	Primary goal is compact storage and spherical interpolation (no gimbal lock).

	Despite their mythical status in the demoscene in the late 1990s, quaternions
	are actually "nothing more" than an axis-angle representation in disguise:

		q = [x, y, z, w] = [axis*sin(angle/2), cos(angle/2)]

	Important rules:
	- We always assume on interface level that a quaternion is unit length, ergo: it is a rotation.
	- Multiplying quaternions (or rotations) isn't commutative, ergo A*B != B*A.
	- Multiplication is (generally) associative, however, so A*(B*C) == (A*B)*C.
	- When performing successive multiplication, re-normalize periodically to counteract numerical drift.
	- Quaternion *is* derived from Vector4, but as it has it's own operators you can not use Vector4's without
	  being explicit; this should gaurantee that when the quaternion is modified through it's own interface
	  it will always be unit length.

	To do (FIXME):
	- Hermite (see Hermite.h/Hermite.cpp) interpolation.

	References:
	- https://lisyarus.github.io/blog/posts/introduction-to-quaternions.html
	- http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
*/
 
#pragma once

class Quaternion : public Vector4
{
public:
	static const Quaternion Identity();
	static const Quaternion AxisAngle(const Vector3 &axis, float angle);
	static const Quaternion YawPitchRoll(float yaw, float pitch, float roll);

	static const Quaternion Nlerp(const Quaternion &from, const Quaternion &to, float T);
	static const Quaternion Slerp(const Quaternion &from, const Quaternion &to, float T);

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
	// These remain private for now, as they can alter magnitude
	const Quaternion operator *(float B) const 
	{ 
		return Vector4::Mul(*this, Vector4(B)); 
	}

	Quaternion& operator *=(float B) { return *this = *this * B; }

public:
	float Angle() const
	{
		// Remember: w = cos(angle/2)
		return acosf(w)*2.f; 
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

	const Quaternion Conjugate() const
	{
		return Quaternion(Vector4(-x, -y, -z, w));
	}

	// Practical alias
	const Quaternion Inverse() const { return Conjugate(); }
};
