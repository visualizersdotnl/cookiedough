
/*
	Basic (unit) quaternion implementation to describe & manipulate 3D rotation.

	- It's primary goal is compact storage and spherical interpolation (no gimbal lock).
	- Multiplying quaternions (or rotations) isn't commutative, ergo A*B != B*A.

	Quaternion is derived from Vector4 but as it has it's own operators you can not
	use Vector4's without some sort of (implicit) casting. This should gaurantee that
	if a quaternion is only modifed through it's own interface, it's always unit length.

	Have a look at Slerp() to see how it's still easy to do vector algebra.

	To do:
	- Create from Euler angles.
	- Create from matrix.
*/
 
#pragma once

class Quaternion : public Vector4
{
public:
	static const Quaternion Identity();
	static const Quaternion AxisAngle(const Vector3 &axis, float angle);
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
		return Quaternion(Vector4(
			 x*B.w + y*B.z - z*B.y + w*B.x,
			-x*B.z + y*B.w + z*B.x + w*B.y,
			 x*B.y - y*B.x + z*B.w + w*B.z,
			-x*B.x - y*B.y - z*B.z + w*B.w));
	}

	Quaternion& operator *=(const Quaternion &B) { return *this = *this * B; }

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
