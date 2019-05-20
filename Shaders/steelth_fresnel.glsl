#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;

out v2 TexCoordsToFragment;

uniform mat4 Transform;

void main()
{
    gl_Position = Transform * V4(Position, 1.0f);
	TexCoordsToFragment = TexCoords;
}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v3 Fresnel;

in v2 TexCoordsToFragment;

uniform sampler2D AlbedoTexture;
uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform sampler2D RMETexture;

uniform v3 ViewRay;

v3 FresnelEquation(v3 Normal, v3 ViewDirection, v3 SurfaceColor, f32 Metalness)
{
	// NOTE: Fresnel-Schlick approximation.
	v3 F0 = V3(0.04f);
	F0 = Lerp(F0, Metalness, SurfaceColor);

	f32 Pow1 = 1.0f - Minimum(1.0f, Maximum(Inner(Normal, ViewDirection), 0.0f));
	f32 Pow2 = Pow1*Pow1;
	f32 Pow5 = Pow2*Pow2*Pow1;
	v3 Result = F0 + (1.0f - F0)*Pow5;

	// NOTE: Pow is undefined for negative x, which is annoying.
	// Result = F0 + (1.0f - F0)*Pow(1.0f - Maximum(Inner(Normal, ViewDirection), 0.0f), 5);

	return Result;
}

void main()
{
	v3 Albedo = texture(AlbedoTexture, TexCoordsToFragment).xyz;
	v3 Normal = texture(NormalTexture, TexCoordsToFragment).xyz;
	f32 Metalness = texture(RMETexture, TexCoordsToFragment).g;

	f32 CurrentDepth = texture(DepthTexture, TexCoordsToFragment).r;
	v3 CurrentPosition = CurrentDepth*ViewRay*V3(TexCoordsToFragment*2.0f - 1.0f, 1.0f);
	v3 ViewDirection = -Normalize(CurrentPosition);

	Fresnel = FresnelEquation(Normal, ViewDirection, Albedo, Metalness);
}

#endif
