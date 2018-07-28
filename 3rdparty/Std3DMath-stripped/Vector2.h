
/*
	2D vector.
*/

#pragma once

class Vector2
{
public:
	static const Vector2 Add(const Vector2 &A, const Vector2 &B) { return Vector2(A.x+B.x, A.y+B.y); }
	static const Vector2 Sub(const Vector2 &A, const Vector2 &B) { return Vector2(A.x-B.x, A.y-B.y); }
	static const Vector2 Mul(const Vector2 &A, const Vector2 &B) { return Vector2(A.x*B.x, A.y*B.y); }
	static const Vector2 Div(const Vector2 &A, const Vector2 &B) { return Vector2(A.x/B.x, A.y/B.y); }

	static const Vector2 Scale(const Vector2 &A, float B)
	{
		return Vector2(A.x*B, A.y*B);
	}

	static float Dot(const Vector2 &A, const Vector2 &B)
	{
		return A.x*B.x + A.y*B.y;
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
	const Vector2 operator *(float B)          const { return Mul(*this, Vector2(B)); }
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

	float LengthSq() const
	{
		return Dot(*this, *this);
	}

	float Length() const
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
		if (length > 0.f)
		{
			*this *= 1.f/length;
		}
	}

	float Angle(const Vector2 &B) const
	{
		return acosf(Dot(*this, B));
	}

	const Vector2 Project(const Vector2 &B) const
	{
		const Vector2 unitB = B.Normalized();
		return B.Normalized() * Dot(*this, unitB);
	}

	const Vector2 Reflect(const Vector2 &normal) const
	{
		const float R = 2.f*Dot(*this, normal);
		return *this - normal*R;
	}

	const Vector2 Perpendicular() const
	{
		return Vector2(-y, x);
	}
};
