
#include "Math.h"

/* static */ const Matrix44 Matrix44::Identity()
{
	Matrix44 matrix;
	matrix.rows[0] = Vector4(1.f, 0.f, 0.f, 0.f);
	matrix.rows[1] = Vector4(0.f, 1.f, 0.f, 0.f);
	matrix.rows[2] = Vector4(0.f, 0.f, 1.f, 0.f);
	matrix.rows[3] = Vector4(0.f, 0.f, 0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::Scaling(const Vector3 &scale)
{
	Matrix44 matrix;
	matrix.rows[0] = Vector4(scale.x,     0.f,     0.f, 0.f);
	matrix.rows[1] = Vector4(    0.f, scale.y,     0.f, 0.f);
	matrix.rows[2] = Vector4(    0.f,     0.f, scale.z, 0.f);
	matrix.rows[3] = Vector4(    0.f,     0.f,     0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::Translation(const Vector3 &translation)
{
	Matrix44 matrix = Identity();
	matrix.SetTranslation(translation);
	return matrix;
}

/* static */ const Matrix44 Matrix44::Rotation(const Quaternion &rotation)
{
	// Should already be a unit quaternion!																																																																																						;;;;
	const Quaternion unit = rotation.Normalized();

	const float XX = unit.x*unit.x;
	const float YY = unit.y*unit.y;
	const float ZZ = unit.z*unit.z;
	const float XY = unit.x*unit.y;
	const float XZ = unit.x*unit.z;
	const float YZ = unit.y*unit.z;
	const float XW = unit.x*unit.w;
	const float YW = unit.y*unit.w;
	const float ZW = unit.z*unit.w;

	Matrix44 matrix;
	matrix.rows[0] = Vector4(1.f - 2.f*(YY+ZZ),       2.f*(XY+ZW),       2.f*(XZ-YW), 0.f);
	matrix.rows[1] = Vector4(      2.f*(XY-ZW), 1.f - 2.f*(XX+ZZ),       2.f*(YZ+XW), 0.f);
	matrix.rows[2] = Vector4(      2.f*(XZ+YW),       2.f*(YZ-XW), 1.f - 2.f*(XX+YY), 0.f);
	matrix.rows[3] = Vector4(              0.f,               0.f,               0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::RotationX(float angle)
{
	const float sine = sinf(angle);
	const float cosine = cosf(angle);

	Matrix44 matrix;
	matrix.rows[0] = Vector4(1.f,    0.f,    0.f, 0.f);
	matrix.rows[1] = Vector4(0.f, cosine,   sine, 0.f);
	matrix.rows[2] = Vector4(0.f,  -sine, cosine, 0.f);
	matrix.rows[3] = Vector4(0.f,    0.f,    0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::RotationY(float angle)
{
	const float sine = sinf(angle);
	const float cosine = cosf(angle);

	Matrix44 matrix;
	matrix.rows[0] = Vector4(cosine, 0.f,  -sine, 0.f);
	matrix.rows[1] = Vector4(   0.f, 1.f,    0.f, 0.f);
	matrix.rows[2] = Vector4(  sine, 0.f, cosine, 0.f);	
	matrix.rows[3] = Vector4(   0.f, 0.f,    0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::RotationZ(float angle)
{
	const float sine = sinf(angle);
	const float cosine = cosf(angle);

	Matrix44 matrix;
	matrix.rows[0] = Vector4(cosine,   sine, 0.f, 0.f);
	matrix.rows[1] = Vector4( -sine, cosine, 0.f, 0.f);
	matrix.rows[2] = Vector4(   0.f,    0.f, 1.f, 0.f);
	matrix.rows[3] = Vector4(   0.f,    0.f, 0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::RotationAxis(const Vector3 &axis, float angle)
{
	return Matrix44::Rotation(Quaternion::AxisAngle(axis, angle));
}

// Taken from: http://source.winehq.org/source/dlls/d3dx9_36/math.c
/* static */ const Matrix44 Matrix44::RotationYawPitchRoll(float yaw, float pitch, float roll)
{
	const float sRoll = sinf(roll), cRoll = cosf(roll);
	const float sPitch = sinf(pitch), cPitch = cosf(pitch);
	const float sYaw = sinf(yaw), cYaw = cosf(yaw);

	const float ssPitchYaw = sPitch*sYaw;
	const float scPitchYaw = sPitch*cYaw;

	// Three rotations folded into one (can probably lose another multiply or two).
	Matrix44 matrix;
	matrix.rows[0] = Vector4(sRoll*ssPitchYaw + cRoll*cYaw, sRoll*cPitch, sRoll*scPitchYaw - cRoll*sYaw, 0.f);
	matrix.rows[1] = Vector4(cRoll*ssPitchYaw - sRoll*cYaw, cRoll*cPitch, cRoll*scPitchYaw + sRoll*sYaw, 0.f);
	matrix.rows[2] = Vector4(                  cPitch*sYaw,      -sPitch,                   cPitch*cYaw, 0.f);
	matrix.rows[3] = Vector4(                          0.f,          0.f,                           0.f, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::View(const Vector3 &from, const Vector3 &to, const Vector3 &up)
{
	assert(true == comparef(1.f, up.Length()));

	// Derive orthonormal basis.
	const Vector3 zAxis = (to-from).Normalized();
	const Vector3 xAxis = (up % zAxis).Normalized();	
	const Vector3 yAxis = zAxis % xAxis;

	// Invert & rotate translation.
	const Vector3 eye(-(xAxis*from), -(yAxis*from), -(zAxis*from));

	Matrix44 matrix;
	matrix.rows[0] = Vector4(xAxis.x, yAxis.x, zAxis.x, 0.f);
	matrix.rows[1] = Vector4(xAxis.y, yAxis.y, zAxis.y, 0.f);
	matrix.rows[2] = Vector4(xAxis.z, yAxis.z, zAxis.z, 0.f);
	matrix.rows[3] = Vector4(  eye.x,   eye.y,   eye.z, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::Perspective(float yFOV, float aspectRatio, float zNear /* = 0.1f */, float zFar /* = 10000.f */)
{
	const float yScale = 1.f/tanf(yFOV*0.5f);
	const float xScale = yScale/aspectRatio;
	const float zRange = zFar-zNear;
	const float zTrans = -zNear*zFar / zRange;

	Matrix44 matrix;
	matrix.rows[0] = Vector4(xScale,     0.f,        0.f, 0.f);
	matrix.rows[1] = Vector4(   0.f, yScale,         0.f, 0.f);
	matrix.rows[2] = Vector4(   0.f,    0.f, zFar/zRange, 0.f);
	matrix.rows[3] = Vector4(   0.f,    0.f,      zTrans, 0.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::Orthographic(const Vector2 &topLeft, const Vector2 &bottomRight, float zNear, float zFar)
{
	const float xScale = 2.f/(bottomRight.x-topLeft.x);
	const float xTrans = -(bottomRight.x+topLeft.x) / (bottomRight.x-topLeft.x);
	const float yScale = 2.f/(topLeft.y-bottomRight.y);
	const float yTrans = -(topLeft.y+bottomRight.y) / (topLeft.y-bottomRight.y);
	const float zRange = zFar-zNear;
	const float zScale = 2.f/zRange;
	const float zTrans = -(zFar+zNear)/zRange;

	Matrix44 matrix;
	matrix.rows[0] = Vector4(xScale,    0.f,    0.f, 0.f);
	matrix.rows[1] = Vector4(   0.f, yScale,    0.f, 0.f);
	matrix.rows[2] = Vector4(   0.f,    0.f, zScale, 0.f);
	matrix.rows[3] = Vector4(xTrans, yTrans, zTrans, 1.f);
	return matrix;
}

/* static */ const Matrix44 Matrix44::FromArray(const float floats[16])
{
	Matrix44 matrix;
	memcpy(&matrix.rows[0], floats, 16*sizeof(float));
	return matrix;
}

/* static */ const Matrix44 Matrix44::FromArray33(const float floats[9])
{
	Matrix44 matrix;
	matrix.rows[0] = Vector4(floats[0], floats[1], floats[2], 0.f);
	matrix.rows[1] = Vector4(floats[3], floats[4], floats[5], 0.f);
	matrix.rows[2] = Vector4(floats[6], floats[7], floats[8], 0.f);
	matrix.rows[3] = Vector4(      0.f,       0.f,       0.f, 1.f);
	return matrix;
}

Matrix44& Matrix44::Scale(const Vector3 &scale)
{
	Multiply(Scaling(scale)); // FIXME: do this in-place.
	return *this;
}

Matrix44& Matrix44::Translate(const Vector3 &translation)
{
	rows[3].x += translation.x;
	rows[3].y += translation.y;
	rows[3].z += translation.z;
	return *this;
}

void Matrix44::SetTranslation(const Vector3 &V)
{
	rows[3].x = V.x;
	rows[3].y = V.y;
	rows[3].z = V.z;
}

const Matrix44 Matrix44::Multiply(const Matrix44 &B) const
{
	Matrix44 matrix;

	// FIXME: for the sake of readability, just roll this up.
	matrix.rows[0].x = rows[0].x*B.rows[0].x + rows[0].y*B.rows[1].x + rows[0].z*B.rows[2].x + rows[0].w*B.rows[3].x;
	matrix.rows[0].y = rows[0].x*B.rows[0].y + rows[0].y*B.rows[1].y + rows[0].z*B.rows[2].y + rows[0].w*B.rows[3].y;
	matrix.rows[0].z = rows[0].x*B.rows[0].z + rows[0].y*B.rows[1].z + rows[0].z*B.rows[2].z + rows[0].w*B.rows[3].z;
	matrix.rows[0].w = rows[0].x*B.rows[0].w + rows[0].y*B.rows[1].w + rows[0].z*B.rows[2].w + rows[0].w*B.rows[3].w;
	matrix.rows[1].x = rows[1].x*B.rows[0].x + rows[1].y*B.rows[1].x + rows[1].z*B.rows[2].x + rows[1].w*B.rows[3].x;
	matrix.rows[1].y = rows[1].x*B.rows[0].y + rows[1].y*B.rows[1].y + rows[1].z*B.rows[2].y + rows[1].w*B.rows[3].y;
	matrix.rows[1].z = rows[1].x*B.rows[0].z + rows[1].y*B.rows[1].z + rows[1].z*B.rows[2].z + rows[1].w*B.rows[3].z;
	matrix.rows[1].w = rows[1].x*B.rows[0].w + rows[1].y*B.rows[1].w + rows[1].z*B.rows[2].w + rows[1].w*B.rows[3].w;
	matrix.rows[2].x = rows[2].x*B.rows[0].x + rows[2].y*B.rows[1].x + rows[2].z*B.rows[2].x + rows[2].w*B.rows[3].x;
	matrix.rows[2].y = rows[2].x*B.rows[0].y + rows[2].y*B.rows[1].y + rows[2].z*B.rows[2].y + rows[2].w*B.rows[3].y;
	matrix.rows[2].z = rows[2].x*B.rows[0].z + rows[2].y*B.rows[1].z + rows[2].z*B.rows[2].z + rows[2].w*B.rows[3].z;
	matrix.rows[2].w = rows[2].x*B.rows[0].w + rows[2].y*B.rows[1].w + rows[2].z*B.rows[2].w + rows[2].w*B.rows[3].w;
	matrix.rows[3].x = rows[3].x*B.rows[0].x + rows[3].y*B.rows[1].x + rows[3].z*B.rows[2].x + rows[3].w*B.rows[3].x;
	matrix.rows[3].y = rows[3].x*B.rows[0].y + rows[3].y*B.rows[1].y + rows[3].z*B.rows[2].y + rows[3].w*B.rows[3].y;
	matrix.rows[3].z = rows[3].x*B.rows[0].z + rows[3].y*B.rows[1].z + rows[3].z*B.rows[2].z + rows[3].w*B.rows[3].z;
	matrix.rows[3].w = rows[3].x*B.rows[0].w + rows[3].y*B.rows[1].w + rows[3].z*B.rows[2].w + rows[3].w*B.rows[3].w;

	return matrix;
}

const Vector3 Matrix44::Transform3(const Vector3 &B) const
{
	return Vector3(
		rows[0].x*B.x + rows[1].y*B.x + rows[2].x*B.z,
		rows[0].y*B.x + rows[1].y*B.y + rows[2].y*B.z,
		rows[0].z*B.x + rows[1].y*B.z + rows[2].z*B.z);
}

const Vector3 Matrix44::Transform4(const Vector3 &B) const
{
	return Vector3(
		rows[0].x*B.x + rows[1].y*B.x + rows[2].x*B.z + rows[3].x,
		rows[0].y*B.x + rows[1].y*B.y + rows[2].y*B.z + rows[3].y,
		rows[0].z*B.x + rows[1].y*B.z + rows[2].z*B.z + rows[3].z);
}

const Vector4 Matrix44::Transform4(const Vector4 &B) const
{
	return Vector4(
		rows[0]*B,
		rows[1]*B,
		rows[2]*B,
		rows[3]*B);
}

const Matrix44 Matrix44::Transpose() const
{
	Matrix44 matrix;
	matrix.rows[0] = Vector4(rows[0].x, rows[1].x, rows[2].x, rows[3].x);
	matrix.rows[1] = Vector4(rows[0].y, rows[1].y, rows[2].y, rows[3].y);
	matrix.rows[2] = Vector4(rows[0].z, rows[1].z, rows[2].z, rows[3].z);
	matrix.rows[3] = Vector4(rows[0].w, rows[1].w, rows[2].w, rows[3].w);
	return matrix;
}

const Matrix44 Matrix44::OrthoInverse() const
{
	Matrix44 matrix;

	// Transpose 3x3 part, set rest to identity.
	matrix.rows[0] = Vector4(rows[0].x, rows[1].x, rows[2].x, 0.f);
	matrix.rows[1] = Vector4(rows[0].y, rows[1].y, rows[2].y, 0.f);
	matrix.rows[2] = Vector4(rows[0].z, rows[1].z, rows[2].z, 0.f);
	matrix.rows[3] = Vector4(      0.f,       0.f,       0.f, 1.f);

	// Invert translation, rotate it, then store.
	const Vector3 translation = matrix.Transform3(Vector3(-rows[0].w, -rows[1].w, -rows[2].w));
	matrix.rows[0].w = translation.x;
	matrix.rows[1].w = translation.y;
	matrix.rows[2].w = translation.z;

	return matrix;
}

const Matrix44 Matrix44::AffineInverse() const
{
	// FIXME: implement constrained inverse.
	return GeneralInverse();
}

// FIXME: this is one of those cases that could actually benefit from general SIMD optimization.
const Matrix44 Matrix44::GeneralInverse() const
{
	Matrix44 matrix;

	// FIXME: this is too hacky.
	const float *pSrc = reinterpret_cast<const float *>(this);
	float *pInv = reinterpret_cast<float *>(&matrix);

	pInv[ 0] =  pSrc[5] * pSrc[10] * pSrc[15] - pSrc[5] * pSrc[11] * pSrc[14] - pSrc[9] * pSrc[6] * pSrc[15] + pSrc[9] * pSrc[7] * pSrc[14] + pSrc[13] * pSrc[6] * pSrc[11] - pSrc[13] * pSrc[7] * pSrc[10];
	pInv[ 4] = -pSrc[4] * pSrc[10] * pSrc[15] + pSrc[4] * pSrc[11] * pSrc[14] + pSrc[8] * pSrc[6] * pSrc[15] - pSrc[8] * pSrc[7] * pSrc[14] - pSrc[12] * pSrc[6] * pSrc[11] + pSrc[12] * pSrc[7] * pSrc[10];
	pInv[ 8] =  pSrc[4] * pSrc[ 9] * pSrc[15] - pSrc[4] * pSrc[11] * pSrc[13] - pSrc[8] * pSrc[5] * pSrc[15] + pSrc[8] * pSrc[7] * pSrc[13] + pSrc[12] * pSrc[5] * pSrc[11] - pSrc[12] * pSrc[7] * pSrc[ 9];
	pInv[12] = -pSrc[4] * pSrc[ 9] * pSrc[14] + pSrc[4] * pSrc[10] * pSrc[13] + pSrc[8] * pSrc[5] * pSrc[14] - pSrc[8] * pSrc[6] * pSrc[13] - pSrc[12] * pSrc[5] * pSrc[10] + pSrc[12] * pSrc[6] * pSrc[ 9];
	pInv[ 1] = -pSrc[1] * pSrc[10] * pSrc[15] + pSrc[1] * pSrc[11] * pSrc[14] + pSrc[9] * pSrc[2] * pSrc[15] - pSrc[9] * pSrc[3] * pSrc[14] - pSrc[13] * pSrc[2] * pSrc[11] + pSrc[13] * pSrc[3] * pSrc[10];
	pInv[ 5] =  pSrc[0] * pSrc[10] * pSrc[15] - pSrc[0] * pSrc[11] * pSrc[14] - pSrc[8] * pSrc[2] * pSrc[15] + pSrc[8] * pSrc[3] * pSrc[14] + pSrc[12] * pSrc[2] * pSrc[11] - pSrc[12] * pSrc[3] * pSrc[10];
	pInv[ 9] = -pSrc[0] * pSrc[ 9] * pSrc[15] + pSrc[0] * pSrc[11] * pSrc[13] + pSrc[8] * pSrc[1] * pSrc[15] - pSrc[8] * pSrc[3] * pSrc[13] - pSrc[12] * pSrc[1] * pSrc[11] + pSrc[12] * pSrc[3] * pSrc[ 9];
	pInv[13] =  pSrc[0] * pSrc[ 9] * pSrc[14] - pSrc[0] * pSrc[10] * pSrc[13] - pSrc[8] * pSrc[1] * pSrc[14] + pSrc[8] * pSrc[2] * pSrc[13] + pSrc[12] * pSrc[1] * pSrc[10] - pSrc[12] * pSrc[2] * pSrc[ 9];
	pInv[ 2] =  pSrc[1] * pSrc[ 6] * pSrc[15] - pSrc[1] * pSrc[ 7] * pSrc[14] - pSrc[5] * pSrc[2] * pSrc[15] + pSrc[5] * pSrc[3] * pSrc[14] + pSrc[13] * pSrc[2] * pSrc[ 7] - pSrc[13] * pSrc[3] * pSrc[ 6];
	pInv[ 6] = -pSrc[0] * pSrc[ 6] * pSrc[15] + pSrc[0] * pSrc[ 7] * pSrc[14] + pSrc[4] * pSrc[2] * pSrc[15] - pSrc[4] * pSrc[3] * pSrc[14] - pSrc[12] * pSrc[2] * pSrc[ 7] + pSrc[12] * pSrc[3] * pSrc[ 6];
	pInv[10] =  pSrc[0] * pSrc[ 5] * pSrc[15] - pSrc[0] * pSrc[ 7] * pSrc[13] - pSrc[4] * pSrc[1] * pSrc[15] + pSrc[4] * pSrc[3] * pSrc[13] + pSrc[12] * pSrc[1] * pSrc[ 7] - pSrc[12] * pSrc[3] * pSrc[ 5];
	pInv[14] = -pSrc[0] * pSrc[ 5] * pSrc[14] + pSrc[0] * pSrc[ 6] * pSrc[13] + pSrc[4] * pSrc[1] * pSrc[14] - pSrc[4] * pSrc[2] * pSrc[13] - pSrc[12] * pSrc[1] * pSrc[ 6] + pSrc[12] * pSrc[2] * pSrc[ 5];
	pInv[ 3] = -pSrc[1] * pSrc[ 6] * pSrc[11] + pSrc[1] * pSrc[ 7] * pSrc[10] + pSrc[5] * pSrc[2] * pSrc[11] - pSrc[5] * pSrc[3] * pSrc[10] - pSrc[ 9] * pSrc[2] * pSrc[ 7] + pSrc[ 9] * pSrc[3] * pSrc[ 6];
	pInv[ 7] =  pSrc[0] * pSrc[ 6] * pSrc[11] - pSrc[0] * pSrc[ 7] * pSrc[10] - pSrc[4] * pSrc[2] * pSrc[11] + pSrc[4] * pSrc[3] * pSrc[10] + pSrc[ 8] * pSrc[2] * pSrc[ 7] - pSrc[ 8] * pSrc[3] * pSrc[ 6];
	pInv[11] = -pSrc[0] * pSrc[ 5] * pSrc[11] + pSrc[0] * pSrc[ 7] * pSrc[ 9] + pSrc[4] * pSrc[1] * pSrc[11] - pSrc[4] * pSrc[3] * pSrc[ 9] - pSrc[ 8] * pSrc[1] * pSrc[ 7] + pSrc[ 8] * pSrc[3] * pSrc[ 5];
	pInv[15] =  pSrc[0] * pSrc[ 5] * pSrc[10] - pSrc[0] * pSrc[ 6] * pSrc[ 9] - pSrc[4] * pSrc[1] * pSrc[10] + pSrc[4] * pSrc[2] * pSrc[ 9] + pSrc[ 8] * pSrc[1] * pSrc[ 6] - pSrc[ 8] * pSrc[2] * pSrc[ 5];
	 
    float determinant = pSrc[0]*pInv[0] + pSrc[1]*pInv[4] + pSrc[2]*pInv[8] + pSrc[3]*pInv[12];
 	if (0.f == determinant)
 	{
 		// FIXME: assert?
 		return Matrix44::Identity();
 	}

	determinant = 1.f/determinant;
	for (unsigned int iElem = 0; iElem < 16; ++iElem)
		pInv[iElem] *= determinant;

	return matrix;
}
