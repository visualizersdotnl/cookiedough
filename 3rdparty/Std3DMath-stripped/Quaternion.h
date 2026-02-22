
/*
	Basic (unit) quaternion implementation to describe & manipulate 3D rotation.

	Primary goal is compact storage and spherical interpolation (no gimbal lock).

	Despite their mythical status in the demoscene in the late 1990s, quaternions
	are actually "nothing more" than an axis-angle representation in disguise:

		q = [x, y, z, w] = [axis*sin(angle/2), cos(angle/2)]

	Where the vector part (x,y,z) describes the axis of rotation and the scalar part (w).

	Important rules:
	- Multiplying quaternions (or rotations) isn't commutative, ergo A*B != B*A.
	- Multiplication is (generally) associative, however, so A*(B*C) == (A*B)*C.
	- Quaternion is derived from Vector4 but as it has it's own operators you can not
	  use Vector4's without some sort of (implicit) casting. This should gaurantee that
	  if a quaternion is only modifed through it's own interface, it's always unit length.
	- When performing many successive multiplications it's wise to re-normalize periodically
	  to counteract numerical drift.

	Have a look at Slerp() to see how it's still easy to do vector algebra :-)

	This class is not complete, please implement:
	- Hermite (cubic) interpolation.

	Good (basic) ref.: https://lisyarus.github.io/blog/posts/introduction-to-quaternions.html
*/
 
#pragma once

class Quaternion : public Vector4
{
public:
	static const Quaternion Identity();
	static const Quaternion AxisAngle(const Vector3 &axis, float angle);
	static const Quaternion YawPitchRoll(float yaw, float pitch, float roll);
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
	// Private for now as it alters magnitude.
	const Quaternion operator *(float B) const 
	{ 
		return Vector4::Mul(*this, Vector4(B)); 
	}

	Quaternion& operator *=(float B) { return *this = *this * B; }

public:
	const Quaternion Normalized() const
	{
		return Vector4::Normalized();
	}

	const Quaternion Conjugate() const
	{
		return Quaternion(Vector4(-x, -y, -z, w));
	}

	const Quaternion Inverse() const
	{
		return Normalized().Conjugate();
	}
};
