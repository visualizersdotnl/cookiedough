
#include "Quaternion.h"
#include "Math.h"

/* static */ const Quaternion Quaternion::Identity()
{
	return Quaternion(Vector4(0.f, 0.f, 0.f, 1.f));
}

/* static */ const Quaternion Quaternion::AxisAngle(const Vector3 &axis, float angle)
{
	const Vector3 unitAxis = axis.Normalized();
	angle *= 0.5f;
	return Quaternion(Vector4(unitAxis*sinf(angle), cosf(angle)));
}

/* static */ const Quaternion Quaternion::YawPitchRoll(float yaw, float pitch, float roll)
{
	// Look up directional cosines if you want to know how this works
	const float halfYaw = yaw*0.5f, halfPitch = pitch*0.5f, halfRoll = roll*0.5f;
	const float halfCosYaw   = cosf(halfYaw);
	const float halfSinYaw   = sinf(halfYaw);
	const float halfCosPitch = cosf(halfPitch);
	const float halfSinPitch = sinf(halfPitch);
	const float halfCosRoll  = cosf(halfRoll);
	const float halfSinRoll  = sinf(halfRoll);

	// Remember: the scalar part (w) is stored last in Vector4 (let the compiler take out duplicate multiplies)
	return Quaternion(Vector4(
		halfCosYaw*halfCosPitch*halfSinRoll - halfSinYaw*halfSinPitch*halfCosRoll,
		halfCosYaw*halfSinPitch*halfCosRoll + halfSinYaw*halfCosPitch*halfSinRoll,
		halfSinYaw*halfCosPitch*halfCosRoll - halfCosYaw*halfSinPitch*halfSinRoll,
		halfCosYaw*halfCosPitch*halfCosRoll + halfCosYaw*halfSinPitch*halfSinRoll));
}

/* static */ const Quaternion Quaternion::Nlerp(const Quaternion &A, const Quaternion &B, float T)
{
	// Nlerp is faster than Slerp, but doesn't maintain constant angular velocity and isn't as accurate (especially for large angles)
	return lerpf<Vector4>(A, B, T).Normalized();
}

/* static */ const Quaternion Quaternion::Slerp(const Quaternion &A, const Quaternion &B, float T)
{
	const float dot = clampf(-1.f, 1.f, Dot(A, B)); // Clamp to acos() domain
	if (dot > 0.9995f)
		// If the quaternions are too close, use linear interpolation and normalize the result to avoid precision issues
		return Nlerp(A, B, T); 

	const float angle = acosf(dot); // Angle between A and B
	const float theta = angle*T;    // Angle between A and result

	// Create a vector orthogonal to A in the plane of A and B, then normalize it to get the second basis vector
	const Vector4 vBasis = (B - A*dot).Normalized(); 

	return A*cosf(theta) + vBasis*sinf(theta);
}
