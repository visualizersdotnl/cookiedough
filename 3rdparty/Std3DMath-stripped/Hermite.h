
/*
    Hermite spline functions.

    Hermite splines are a variation on Bezier splines:
    - They use and always pass through 2 control points (each with a corresponding direction or tangent, if you will), whereas
      in bezier splines the 4 control points more or less act like attractors.
    - Their mathematical basis is more or less identical (Bernstein polynomials).
 
    Two excellent resources on the matter:
    - Youtube video on Quadratric/Cubic Bezier splines by Freya Holmér: https://youtu.be/aVwxzDHniEw
    - The Orange Duck on implementing these for use with unit quaternions: https://theorangeduck.com/page/cubic-interpolation-quaternions
*/

#pragma once

#include "Dependencies.h"
namespace Std3DMath
{
    // To do: implement for Vector2, Vector3, Quaternion
}
