#line 2
#define Pi32 3.14159265359f

#define b32 bool
#define f32 float
#define s32 int
#define u32 unsigned int
#define v2 vec2
#define v3 vec3
#define v4 vec4
#define V2 vec2
#define V3 vec3
#define V4 vec4
#define Matrix2 mat2
#define Matrix3 mat3
#define Matrix4 mat4
#define v2i ivec2
#define V2i ivec2
#define v3i ivec3
#define V3i ivec3
#define v2u uvec2
#define V2u uvec2
#define v3u uvec3
#define V3u uvec3

#define Lerp(a, t, b) mix(a, b, t)
#define Clamp01(t) clamp(t, 0, 1)
#define Clamp(t, min, max) clamp(t, min, max)
#define Inner(a, b) dot(a, b)
#define Length(a) length(a)
#define LengthSq(a) dot(a, a)
#define Normalize(a) normalize(a)
#define SquareRoot(a) sqrt(a)
#define Maximum(a, b) max(a, b)
#define Minimum(a, b) min(a, b)
#define AbsoluteValue(a) abs(a)
#define Pow(a, b) pow(a, b)
#define Square(a) (a*a)

struct light_shader_data
{
	b32 IsDirectional;
	v3 Color;
	f32 Intensity;

	v3 Direction;
	f32 FuzzFactor;
	v3 P;
	f32 Size;
	f32 LinearAttenuation;
	f32 QuadraticAttenuation;
	f32 Radius;
};

struct decal_shader_data
{
	mat4 Projection;
	mat4 View;
};

struct surface_element
{
	v3 P;
	f32 Area;
	v3 Normal;
	u32 ClosestProbe;
	v3 Albedo;
	f32 Emission;
	v3 LitColor;
};

v3 ProbeDirections[] =
{
	V3(1,0,0),
	V3(-1,0,0),
	V3(0,1,0),
	V3(0,-1,0),
	V3(0,0,1),
	V3(0,0,-1),
};
struct light_probe
{
	v3 P;
	u32 FirstSurfel;
	v4 LightContributions_SkyVisibility[6];
	u32 OnePastLastSurfel;
};

#line 1
