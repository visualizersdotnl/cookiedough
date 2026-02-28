
/*
    Hermite spline functions.

    The Hermite spline has the right properties in terms of continuity and determinism (passes through the control points) for
    many applications, and by using the Catmull-Rom approach we can use 4 points and take their central differences so you need 
    not bother with these velocity vectors yourself, unless you want to.

    This technique lends itself well to camera paths, animation (rotations), geometry.

    But there's more, and I highly recommend watching Freya's video(s) if you haven't already, she explains it better
    than I can in a few comments.

    - Youtube video on splines by Freya Holmér: https://youtu.be/jvPPXbo87ds (*)
    - The Orange Duck on implementing this for quaternions: https://theorangeduck.com/page/cubic-interpolation-quaternions

    (*) She also made one that completely demystifies this to what it actually is: nesting linear interpolation to higher order.

    FIXME: 
    - Simplified versions that don't calculate (angular) velocity vectors
    - Potential overshoot
*/

#pragma once

namespace Std3DMath
{
    // Hermite (cubic) basis functions (Bernstein polynomials)

    S3D_INLINE static const std::array<float, 3> Hermite_Pos(float t)
    {
        return {
             3.f*t*t - 2.f*t*t*t, 
             t*t*t - 2.f*t*t + t, 
             t*t*t - t*t };
    }

    // First derivative: velocity (tangent)
    S3D_INLINE static const std::array<float, 3> Hermite_Vel(float t)
    {
        return {
             6.f*t - 6.f*t*t, 
             3.f*t*t - 4.f*t + 1.f, 
             3.f*t*t - 2.f*t };
    }

    // Vectors

    template<typename T>
    S3D_INLINE static const std::array<T, 2> Hermite_Vec(const T &P0, const T &P1, const T &V1, const T &V2, float t)
    {
        const auto w = Hermite_Pos(t);
        const auto q = Hermite_Vel(t);

        const T vP1P0 = P1 - P0;
        return {
            vP1P0*w[0] + V1*w[1] + V2*w[2] + P0,
            vP1P0*q[0] + V1*q[1] + V2*q[2]
        };
    }

    template<typename T>
    S3D_INLINE static const std::array<T, 2> CatmullRom_Vec(const T &P0, const T &P1, const T &P2, const T &P3, float t)
    {
        // Central differences (est. tangent vectors)
        const T V1 = ((P1-P0)+(P2-P1))*0.5f;
        const T V2 = ((P2-P1)+(P3-P2))*0.5f;
        
        return Hermite_Vec<T>(P1, P2, V1, V2, t);
    }

    // Shortcuts
    S3D_INLINE static const std::array<Vector2, 2> CatmullRom_Vec2(
        const Vector2 &P0, const Vector2 &P1, const Vector2 &P2, const Vector2 &P3, float t) 
    {
        return CatmullRom_Vec<Vector2>(P0, P1, P2, P3, t);
    }

    S3D_INLINE static const std::array<Vector3, 2> CatmullRom_Vec3(
        const Vector2 &P0, const Vector3 &P1, const Vector3 &P2, const Vector3 &P3, float t) 
    {
        return CatmullRom_Vec<Vector3>(P0, P1, P2, P3, t);
    }

    // Quaternions
    
    // What happens here isn't much different from pure vector algebra: it just uses a different intermediate representation
    // and we do not add or subtract quaternions: we multiply them in a certain order to get from A to B

    S3D_INLINE static const std::tuple<Quaternion, Vector3> Hermite_Quat(
        const Quaternion &Q0, const Quaternion &Q1, const Vector3 &V1, const Vector3 &V2, float t)
    {
        const auto w = Hermite_Pos(t);
        const auto q = Hermite_Vel(t);

        // Take absolute (curtailed to one hemisphere) rotational difference (Q1'*Q0) in angle-axis (vector) form
        const Vector3 vQ1Q0 = Quaternion::ScaledAngleAxis((Q1.Inverse()*Q0).Abs());
        
        return {
            Quaternion::ScaledAngleAxis(vQ1Q0*w[0] + V1*w[1] + V2*w[2]) * Q0,
            vQ1Q0*q[0] + V1*q[1] + V2*q[2] 
        };
    }

    S3D_INLINE static const std::tuple<Quaternion, Vector3> CatmullRom_Quat(
        const Quaternion &Q0, const Quaternion &Q1, const Quaternion &Q2, const Quaternion &Q3, float t) 
    {
        // Take all rotational differences
        const Vector3 vQ1Q0 = Quaternion::ScaledAngleAxis((Q1.Inverse()*Q0).Abs());
        const Vector3 vQ2Q1 = Quaternion::ScaledAngleAxis((Q2.Inverse()*Q1).Abs());
        const Vector3 vQ3Q2 = Quaternion::ScaledAngleAxis((Q3.Inverse()*Q2).Abs());
        
        // Take their central differences
        const Vector3 V1 = (vQ1Q0 - vQ2Q1)*0.5f;
        const Vector3 V2 = (vQ2Q1 - vQ3Q2)*0.5f;

        return Hermite_Quat(Q1, Q2, V1, V2, t);
    }
}
