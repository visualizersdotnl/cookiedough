
/*
	4x4 matrix.

	Uses column-major layout (translation lives in the bottom-left corner).
	This model reflects the one used by many 3D APIs.
	However, row-major is potentially nicer for (SIMD) CPU transform.

	- Assumes left-handed coordinate system.
	- Consider using Quaternion for rotations, as it saves memory & cycles.

	To do:
	- Unionize Vector4s with floats.
	- Implement affine inverse.
	- Optimize; seemingly simple operations may end up being too costly.
*/

#pragma once

class Matrix44
{
public:
	static const Matrix44 Identity();
	static const Matrix44 Scaling(const Vector3 &scale);
	static const Matrix44 Translation(const Vector3 &translation);
	static const Matrix44 Rotation(const Quaternion &rotation);
	static const Matrix44 RotationX(float angle);
	static const Matrix44 RotationY(float angle);
	static const Matrix44 RotationZ(float angle);
	static const Matrix44 RotationAxis(const Vector3 &axis, float angle);
	static const Matrix44 RotationYawPitchRoll(float yaw, float pitch, float roll);
	static const Matrix44 View(const Vector3 &from, const Vector3 &to, const Vector3 &up);
	static const Matrix44 Perspective(float yFOV, float aspectRatio, float zNear = 0.1f, float zFar = 10000.f);
	static const Matrix44 Orthographic(const Vector2 &topLeft, const Vector2 &bottomRight, float zNear, float zFar);
	static const Matrix44 FromArray(const float floats[16]);
	static const Matrix44 FromArray33(const float floats[9]);

public:
	Vector4 rows[4];

	// In-place operations (much faster than a mere multiplication).
	Matrix44& Scale(const Vector3 &scale);
	Matrix44& Translate(const Vector3 &translation);

	void SetTranslation(const Vector3 &translation);
	
	// - Multiplying matrices isn't commutative, so A*B != B*A.
	// - Transformation order is left to right.
	const Matrix44 Multiply(const Matrix44 &B) const;

	const Vector3 Transform3(const Vector3 &B) const; // Transform w/3x3 part (no translation, for vectors).
	const Vector3 Transform4(const Vector3 &B) const; // Transform w/3x4 part (points).
	const Vector4 Transform4(const Vector4 &B) const;

	const Matrix44 Transpose() const;

	// Invert orthogonal matrix (euclidian transform; may rotate, translate, reflect).
	const Matrix44 OrthoInverse() const;

	// Invert affine matrix (may also scale, shear).
	const Matrix44 AffineInverse() const;

	// General inverse (prefixed to encourage use of specific inverse).
	// Rule of thumb: use when bottom row isn't (0, 0, 0, 1).
	const Matrix44 GeneralInverse() const;
	
	// operator: V' = M*V
	const Vector3 operator *(const Vector3 &B) const { return Transform4(B); }
	const Vector4 operator *(const Vector4 &B) const { return Transform4(B); }

	// operator: M' = M*M
	const Matrix44 operator *(const Matrix44 &B) const { return Multiply(B); }
	Matrix44& operator *=(const Matrix44 &B) { return *this = *this * B; }

private:
	// You can't have an uninitialized matrix.
	Matrix44() {}
};
