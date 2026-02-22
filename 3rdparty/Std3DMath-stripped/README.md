# Std3DMath - Standard math for but not limited to 2D/3D rendering (C++).

What it says on the tin.
First drafted over 15 years ago.

Jan/Feb 2026: I've started to flesh it out with a few more basics I've seen/needed throughout the years.

It's not meant to:
- Be overly optimized, I believe in solving heavy specific processing is best and most optimally done in place.
- Do more than what I need it to do.

Please take heed:
- Not overly complete nor intended to be; I add & refactor on demand.
- Aligned with Intel SIMD registers for practical convenience, making either Intel x64 or for ex. SSE2NEON.h a prerequisite (for now).
- Resides in global namespace (change if necessary) with the exception of function groups (like intersection tests).
- Library uses standard assert(), though not religiously so.
- Warning: has Intel-specific header dependency (will be fixed!).

I usually just copy a whatever is recent and dump it in a third party folder of a project instead of using a proper Git submodule.
