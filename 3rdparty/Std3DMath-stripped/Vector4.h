
/*
	4D (homogenous) vector.
*/

#pragma once

class Vector4
{
public:
	S3D_INLINE static const Vector4 Add(const Vector4 &A, const Vector4 &B) { return {A.x+B.x, A.y+B.y, A.z+B.z, A.w+B.w}; }
	S3D_INLINE static const Vector4 Sub(const Vector4 &A, const Vector4 &B) { return {A.x-B.x, A.y-B.y, A.z-B.z, A.w-B.w}; }
	S3D_INLINE static const Vector4 Mul(const Vector4 &A, const Vector4 &B) { return {A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w}; }
	S3D_INLINE static const Vector4 Div(const Vector4 &A, const Vector4 &B) { return {A.x/B.x, A.y/B.y, A.z/B.z, A.w/B.w}; }

	S3D_INLINE static const Vector4 Scale(const Vector4 &A, float B)
	{
		return { A.x*B, A.y*B, A.z*B, A.w*B };
	}

	S3D_INLINE static float Dot(const Vector4 &A, const Vector4 &B)
	{
		return A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
	}

public:
	// 03/08/2018 - Added for Bevacqua.
	operator __m128() const { return vSSE; }

	union
	{
		struct {
			float x, y, z, w;
		};

		__m128 vSSE;
	};
	
	Vector4() {}

	explicit Vector4(float scalar) : 
		x(scalar), y(scalar), z(scalar), w(scalar) {}

	explicit Vector4(float x, float y, float z) :
		x(x), y(y), z(z), w(1.f) {}

	Vector4(float x, float y, float z, float w) :
		x(x), y(y), z(z), w(w) {}

	Vector4(const Vector3 &vec3D, float w = 1.f) :
		x(vec3D.x), y(vec3D.y), z(vec3D.z), w(w) {}

	const Vector4 operator +(const Vector4 &B) const { return Add(*this, B); }
	const Vector4 operator +(float B)          const { return Add(*this, Vector4(B)); }
	const Vector4 operator -(const Vector4 &B) const { return Sub(*this, B); }
	const Vector4 operator -(float B)          const { return Sub(*this, Vector4(B)); }
	const float   operator *(const Vector4 &B) const { return Dot(*this, B); }
	const Vector4 operator *(float B)          const { return Scale(*this, B); }
	const Vector4 operator /(const Vector4 &B) const { return Div(*this, B); }
	const Vector4 operator /(float B)          const { return Div(*this, Vector4(B)); }

	Vector4& operator +=(const Vector4 &B) { return *this = *this + B; }
	Vector4& operator +=(float B)          { return *this = *this + B; }
	Vector4& operator -=(const Vector4 &B) { return *this = *this - B; }
	Vector4& operator -=(float B)          { return *this = *this - B; }
	Vector4& operator *=(float B)          { return *this = *this * B; }
	Vector4& operator /=(const Vector4 &B) { return *this = *this / B; }
	Vector4& operator /=(float B)          { return *this = *this / B; }

	bool operator ==(const Vector4 &B) const
	{
		return comparef(x, B.x) && comparef(y, B.y) && comparef(z, B.z) && comparef(w, B.w);
	}

	bool operator !=(const Vector4 &B) const
	{
		return false == (*this == B);
	}

	bool operator <(const Vector4 &B) const
	{
		return LengthSq() < B.LengthSq();
	}

	S3D_INLINE float LengthSq() const
	{
		return Dot(*this, *this);
	}

	S3D_INLINE float Length() const
	{
		return sqrtf(LengthSq());
	}
	
	const Vector4 Normalized() const
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

	const Vector4 Multiplied(const Vector4 &B) const
	{
		return Mul(*this, B);
	}

	void Multiply(const Vector4 &B)
	{
		*this = Mul(*this, B);
	}

	// Chiefly intended for constant uploads.
	const float *GetData() const { return &x; }
};
