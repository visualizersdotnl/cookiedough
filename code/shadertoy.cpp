
// cookiedough -- simple Shadertoy ports

/*
	to do:
		- fix a *working* OpenMP implementation of the plasma (are it the split writes in 64-bit?)
		- a minor optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP
		- just optimize as needed, since all of this is a tad slow
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"
#include "boxblur.h"

static uint32_t *s_pEnvMap1 = nullptr;

bool Shadertoy_Create()
{
	s_pEnvMap1 = Image_Load32("assets/shadertoy/nauti_env.png");
	if (nullptr == s_pEnvMap1)
		return false;
	
	return true;
}

void Shadertoy_Destroy()
{
	Image_Free(s_pEnvMap1);
}

//
// Plasma (https://www.shadertoy.com/view/ldSfzm)
//

VIZ_INLINE float fPlasma(Vector3 point, float time)
{
	point.z += 5.f*time;
	const float sine = 0.2f*lutsinf(point.x-point.y);
	const float fX = sine + lutcosf(point.x*0.33f);
	const float fY = sine + lutcosf(point.y*0.33f);
	const float fZ = sine + lutcosf(point.z*0.33f);
//	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
	return 1.f/Q_rsqrt(fX*fX + fY*fY + fZ*fZ)-0.8f;
}

static void RenderPlasmaMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.1f, 0.15f, 0.3f);
	const Vector3 colMulB(0.05f, 0.05f, 0.1f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{

				auto& UV = Shadertoy::ToUV_FX_2x2(iX+iColor, iY, 2.f);

				const int cosIndex = tocosindex(time*0.314f);
				const float dirCos = lutcosf(cosIndex);
				const float dirSin = lutsinf(cosIndex);

				Vector3 direction(
					dirCos*UV.x - dirSin*0.6f,
					UV.y,
					dirSin*UV.x + dirCos*0.6f);

//				Vector3 direction(Shadertoy::ToUV_FX_2x2(iX+iColor, iY, 2.f), 0.6f);

				Vector3 origin = direction;
				for (int step = 0; step < 46; ++step)
					origin += direction*fPlasma(origin, time);
				
				colors[iColor] = colMulA*fPlasma(origin+direction, time) + colMulB*fPlasma(origin*0.5f, time); 
				colors[iColor] *= 8.f - origin.x*0.5f;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	RenderPlasmaMap(g_pFXFine, time);
	MapBlitter_Colors_2x2(pDest, g_pFXFine);
//	HorizontalBoxBlur32(pDest, pDest, kResX, kResY, 0.01f);
}

//
// Nautilus Redux by Michiel v/d Berg
//

VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	float cosX, cosY, cosZ;
	float cosTime7 = lutcosf(time/7.f); // ** i'm getting an internal compiler error if I write this where it should be **
	cosX = lutcosf(lutcosf(position.x + time*0.125f)*position.x - lutcosf(position.y + time/9.f)*position.y);
	cosY = lutcosf(position.z*0.33f*position.x - cosTime7*position.y);
	cosZ = lutcosf(position.x + position.y + position.z/1.25f + time);
	const float dotted = cosX*cosX + cosY*cosY + cosZ*cosZ;
	return dotted*0.5f - .7f;
};

static void RenderNautilusMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colorization(
		.1f-lutcosf(time/3.f)/19.f, 
		.1f, 
		.1f+lutcosf(time/14.f)/8.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 1.614f);

				Vector3 origin(0.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				direction.Normalize();

				Shadertoy::rot2D(kPI*cos(time*0.06234f), direction.x, direction.y);

				Vector3 hit(0.f);

				float march, total = 0.f;
				for (int iStep = 0; iStep < 64; ++iStep)
				{
					hit = origin + direction*total;
					march = fNautilus(hit, time);
					total += march*0.7f;

//					if (fabsf(march) < 0.001f*(total*0.125f + 1.f) || total>20.f)
//						break;
				}

				float nOffs = 0.001f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));

				// Vector3 lightA(0.f, 0.f, -0.5f), lightB(0.f, -0.5f, 0.5f);
				// float diffuse = std::max(0.f, lightA*normal) + std::max(0.f, lightB*normal);
				float diffuse = -0.5f*normal.z + -0.5f*normal.y + 0.5f*normal.z;
				
				Vector3 color(diffuse*0.1f);
				color += colorization*(1.56f*total);

				const float gamma = 2.20f;
				color.x = powf(color.x, gamma);
				color.y = powf(color.y, gamma);
				color.z = powf(color.z, gamma);

				colors[iColor].vSIMD = color.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

static void RenderNautilusMap_4x4(uint32_t *pDest, float time)
{
	// FIXME: implement
	VIZ_ASSERT(false);
}

void Nautilus_Draw(uint32_t *pDest, float time, float delta)
{
	RenderNautilusMap_2x2(g_pFXFine, time);
	
	// FIXME: this looks amazing if timed!
	// HorizontalBoxBlur32(g_pFXFine, g_pFXFine, kFineResX, kFineResY, fabsf(0.0314f*2.f*sin(time)));
	
	MapBlitter_Colors_2x2(pDest, g_pFXFine);

//	RenderNautilusMap_4x4(g_pFXCoarse, time);
//	MapBlitter_Colors_4x4(pDest, g_pFXCoarse);
}

//
// Confetti canon by Flopine (https://www.shadertoy.com/view/MddyWX)
//

void Flowerine_Draw(uint32_t *pDest, float time, float delta)
{
}

#if 0

#define PI 3.141592
#define TAU 2.*PI

mat2 rot (float a)
{
    float c = cos(a);
    float s = sin(a);
    return mat2(c,s,-s,c);
}


vec2 moda (vec2 p, float per)
{
    float a = atan(p.y,p.x);
    float l = length(p);
    a = mod(a-per/2.,per)-per/2.;
    return vec2 (cos(a),sin(a))*l;
}

// iq's palette http://www.iquilezles.org/www/articles/palettes/palettes.htm
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos(TAU*(c*t+d));
}


float sphe (vec3 p, float r)
{
    return length(p)-r;
}

float box (vec3 p, vec3 c)
{
    return length(max(abs(p)-c,0.));
}

float prim (vec3 p)
{
    float b = box(p, vec3(1.));
    float s = sphe(p,1.3);
    return max(-s, b);
}

float row (vec3 p, float per)
{
	p.y = mod(p.y-per/2., per)-per/2.;
    return prim(p);
}

float squid (vec3 p)
{
    p.xz *= rot(PI/2.);
    p.yz = moda(p.yz, TAU/5.);
    p.z += sin(p.y+iTime*2.);
    return row(p,1.5);
}

float SDF(vec3 p)
{
    p.xz *= rot (iTime*.8);
    p.yz *= rot(iTime*0.2);
    p.xz = moda(p.xz, TAU/12.);
    return squid(p);
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = 2.*(fragCoord/iResolution.xy)-1.;
	uv.x *= iResolution.x/iResolution.y;
    
    vec3 p = vec3 (0.01,3.,-8.);
    vec3 dir = normalize(vec3(uv*2.,1.));
    
    float shad = 1.;
    
    for (int i=0;i<60;i++)
    {
        float d = SDF(p);
        if (d<0.001)
        {
            shad = float(i)/60.;
            break;
        }
        p += d*dir*0.5;
    }
    
    vec3 pal = palette(p.z,
        				vec3(0.5),
                      vec3(0.5),
                      vec3(.2),
                      vec3(0.,0.3,0.5)
                      );
    // Time varying pixel color
    vec3 col = vec3(1.-shad)*pal;

    // Output to screen
    fragColor = vec4(pow(col, vec3(0.45)),1.0);
}

#endif
