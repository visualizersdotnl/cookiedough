
/*
	2D vector.
*/

#pragma once

class Vector2
{
public:
	S3D_INLINE static const Vector2 Add(const Vector2 &A, const Vector2 &B) { return {A.x+B.x, A.y+B.y}; }
	S3D_INLINE static const Vector2 Sub(const Vector2 &A, const Vector2 &B) { return {A.x-B.x, A.y-B.y}; }
	S3D_INLINE static const Vector2 Mul(const Vector2 &A, const Vector2 &B) { return {A.x*B.x, A.y*B.y}; }
	S3D_INLINE static const Vector2 Div(const Vector2 &A, const Vector2 &B) { return {A.x/B.x, A.y/B.y}; }

	static const Vector2 Scale(const Vector2 &A, float B)
	{
		return { A.x*B, A.y*B };
	}

	static float Dot(const Vector2 &A, const Vector2 &B)
	{
		return A.x*B.x + A.y*B.y;
	}

	// AxB != BxA
	// In 2D, the result is the signed magnitude of the imaginary perpendicular vector
	// This is at the root of essentials like triangle rasterization and volume clipping (overlaps with what's done in RayTriangleIntersect())
	static float Cross(const Vector2 &A, const Vector2 &B)
	{
		return A.x*B.y - B.x*A.y;
	}

public:
	float x, y;
	
	explicit Vector2(float scalar) : 
		x(scalar), y(scalar) {}

	Vector2(float x, float y) :
		x(x), y(y) {}

	const Vector2 operator +(const Vector2 &B) const { return Add(*this, B); }
	const Vector2 operator +(float B)          const { return Add(*this, Vector2(B)); }
	const Vector2 operator -(const Vector2 &B) const { return Sub(*this, B); }
	const Vector2 operator -(float B)          const { return Sub(*this, Vector2(B)); }
	const float   operator *(const Vector2 &B) const { return Dot(*this, B); }
	const Vector2 operator *(float B)          const { return Scale(*this, B); }
	const Vector2 operator /(const Vector2 &B) const { return Div(*this, B); }
	const Vector2 operator /(float B)          const { return Div(*this, Vector2(B)); }

	Vector2& operator +=(const Vector2 &B) { return *this = *this + B; }
	Vector2& operator +=(float B)          { return *this = *this + B; }
	Vector2& operator -=(const Vector2 &B) { return *this = *this - B; }
	Vector2& operator -=(float B)          { return *this = *this - B; }
	Vector2& operator *=(float B)          { return *this = *this * B; }
	Vector2& operator /=(const Vector2 &B) { return *this = *this / B; }
	Vector2& operator /=(float B)          { return *this = *this / B; }

	bool operator ==(const Vector2 &B) const
	{
		return comparef(x, B.x) && comparef(y, B.y);
	}

	bool operator !=(const Vector2 &B) const
	{
		return false == (*this == B);
	}

	bool operator <(const Vector2 &B) const
	{
		return LengthSq() < B.LengthSq();
	}

	S3D_INLINE float LengthSq() const
	{
		return fabsf(Dot(*this, *this));
	}

	S3D_INLINE float Length() const
	{
		return sqrtf(Dot(*this, *this));
	}
	
	const Vector2 Normalized() const
	{
		auto result = *this;
		result.Normalize();
		return result;
	}

	void Normalize()
	{
		const float length = Length();
		if (length > kEpsilon)
		{
			*this *= 1.f/length;
		}
	}

	S3D_INLINE float Angle(const Vector2 &B) const
	{
		return acosf(Dot(*this, B));
	}

	// Project A (this) onto B
	const Vector2 Project(const Vector2 &B) const
	{
		const Vector2 unitB = B.Normalized();
		return unitB * Dot(*this, unitB);
	}

	const Vector2 Reflect(const Vector2 &normal) const
	{
		const float R = 2.f*Dot(*this, normal);
		return *this - normal*R;
	}

	S3D_INLINE const Vector2 Perpendicular() const
	{
		return Vector2(-y, x);
	}
};
