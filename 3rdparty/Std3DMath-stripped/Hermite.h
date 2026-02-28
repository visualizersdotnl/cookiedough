
/*
    Hermite spline functions (camera paths, animation, rotation, geometry).

    The Hermite spline has the right properties in terms of continuity and determinism (passes through the control points) for
    many applications, and by using the Catmull-Rom approach we can use 4 points and take their central differences so you need 
    not bother with these velocity vectors yourself, unless you want to.

    These functions are designed to calculate both position and velocity (first derivative); I'm banking on any proper optimizing
    C++ compiler to do away with redundant calculations entirely if this output isn't used. In case you need raw performance
    I'd suggest lifting these calculations into a tight custom loop, because there's more that can be eliminated and cached,
    especially for quaternions.

    - Youtube video on splines by Freya Holmér: https://youtu.be/jvPPXbo87ds (*)
    - The Orange Duck on implementing this for quaternions: https://theorangeduck.com/page/cubic-interpolation-quaternions

    (*) She also made one that completely demystifies this to what it actually is: nesting linear interpolation to higher order.

    FIXME: 
    - Potential overshoot
    - Wrap 'take rotational difference' (?)
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

    // What happens for quaternions isn't much different from pure vector algebra.

    // The calculation is already written so that it first calculates the position relative to zero and then 
    // adds the origin to translate it back to where it should be.
    
    // This means we can use rotational differences between quaternions transformed to vector space
    // to do the meat of the calculation and then once transformed back to a quaternion use multiplication
    // to apply the correct amount of rotation to the origin (since multiplying quaternion A with B is what
    // adding vector B to A is, in this context).

    S3D_INLINE static const std::tuple<Quaternion, Vector3> Hermite_Quat(
        const Quaternion &Q0, const Quaternion &Q1, const Vector3 &V1, const Vector3 &V2, float t)
    {
        const auto w = Hermite_Pos(t);
        const auto q = Hermite_Vel(t);

        // Take absolute (curtailed to one hemisphere) rotational difference (Q1'*Q0) in angle-axis (vector) form
        const Vector3 vQ1Q0 = Q0.Diff(Q1);
        
        return {
            Quaternion::ScaledAngleAxis(vQ1Q0*w[0] + V1*w[1] + V2*w[2]) * Q0,
            vQ1Q0*q[0] + V1*q[1] + V2*q[2] 
        };
    }

    S3D_INLINE static const std::tuple<Quaternion, Vector3> CatmullRom_Quat(
        const Quaternion &Q0, const Quaternion &Q1, const Quaternion &Q2, const Quaternion &Q3, float t) 
    {
        // Take all rotational differences
        const Vector3 vQ1Q0 = Q0.Diff(Q1);
        const Vector3 vQ2Q1 = Q1.Diff(Q2);
        const Vector3 vQ3Q2 = Q2.Diff(Q3);
        
        // Take their central differences
        const Vector3 V1 = (vQ1Q0 - vQ2Q1)*0.5f;
        const Vector3 V2 = (vQ2Q1 - vQ3Q2)*0.5f;

        return Hermite_Quat(Q1, Q2, V1, V2, t);
    }
}
