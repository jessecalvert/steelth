#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;
layout (location = 4) in int TextureIndex;
layout (location = 5) in v2 Roughness_Metalness;

out v2 TexCoordsToFragment;
out v4 ColorToFragment;
out v3 NormalToFragment;
out v4 CameraRay;
out f32 RoughnessToFragment;
out f32 MetalnessToFragment;
flat out int TextureIndexToFragment;

uniform mat4 Projection;
uniform mat4 View;

#ifdef MESH_SHADER
uniform mat4 Model;
uniform mat3 NormalMatrix;
uniform int MeshTextureIndex;
uniform f32 Roughness;
uniform f32 Metalness;

#endif

void main()
{
	TexCoordsToFragment = TexCoords;
	ColorToFragment = Color;

#ifdef MESH_SHADER
	CameraRay = View * Model * V4(Position, 1.0f);
	gl_Position = Projection * CameraRay;
	NormalToFragment = Normalize((View * V4(NormalMatrix * Normal, 0.0f)).xyz);
	TextureIndexToFragment = MeshTextureIndex;
	RoughnessToFragment = Roughness;
	MetalnessToFragment = Metalness;

#else
	CameraRay = View * V4(Position, 1.0f);
	gl_Position = Projection * CameraRay;
	NormalToFragment = (View * V4(Normal, 0)).xyz;
	TextureIndexToFragment = TextureIndex;
	RoughnessToFragment = Roughness_Metalness.r;
	MetalnessToFragment = Roughness_Metalness.g;

#endif
}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v4 FragColor;

in v2 TexCoordsToFragment;
in v4 ColorToFragment;
in v3 NormalToFragment;
in v4 CameraRay;
in f32 RoughnessToFragment;
in f32 MetalnessToFragment;
flat in int TextureIndexToFragment;

uniform mat4 View;

uniform v2 Jitter;
uniform f32 FarPlane;

uniform v4 Color;

uniform sampler2DArray TextureArray;

uniform s32 LightCount;
uniform light_shader_data Lights[MAX_LIGHTS];

uniform s32 DecalCount;
uniform decal_shader_data Decals[MAX_DECALS];

// TODO: More than one decal texture.
uniform int DecalTextureIndex;
uniform v2 DecalTextureScale;

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

f32 SpecularNormalDistribution(v3 Normal, v3 HalfwayDirection, f32 Roughness)
{
	// NOTE: Trowbridge-Reitz GGX
	f32 Roughness4 = Pow(Roughness, 4);
	f32 NDotH = Maximum(Inner(Normal, HalfwayDirection), 0.0f);
	f32 NDotHSquare = NDotH*NDotH;
	f32 Denominator = NDotHSquare*(Roughness4 - 1.0f) + 1.0f;
	Denominator = PI * Denominator * Denominator;

	f32 Result = Roughness4 / Denominator;
	return Result;
}

f32 GeometrySchlickGGX(f32 NDotV, f32 RoughnessRemap)
{
    f32 Numerator = NDotV;
    f32 Denominator = NDotV * (1.0f - RoughnessRemap) + RoughnessRemap;

    f32 Result = Numerator / Denominator;
    return Result;
}

f32 GeometryFunction(v3 Normal, v3 ViewDirection, v3 LightDirection, f32 Roughness)
{
	// NOTE: Schlick-GGX using Smith's method.
	// NOTE: This is for direct lighting. It would be different if doing IBL!
	f32 RoughnessRemap = Roughness + 1.0f;
	RoughnessRemap = 0.125f * RoughnessRemap * RoughnessRemap;

	f32 NDotV = Maximum(Inner(Normal, ViewDirection), 0.0f);
	f32 NDotL = Maximum(Inner(Normal, LightDirection), 0.0f);
	f32 GGX1 = GeometrySchlickGGX(NDotV, RoughnessRemap);
	f32 GGX2 = GeometrySchlickGGX(NDotL, RoughnessRemap);

	f32 Result = GGX1*GGX2;
	return Result;
}

v3 CalculateLightContribution(light_shader_data Light, v3 Albedo, v3 CurrentNormal, v3 CurrentPosition, f32 Metalness, v3 SpecularComponent, f32 Roughness)
{
	v3 FinalLightColor = V3(0,0,0);

	if(Light.IsDirectional && (Light.Direction == V3(0,0,0)))
	{
		// NOTE: Ambient light;
		v3 Ambient = Light.FuzzFactor * Light.Color;
		v3 AmbientColor = Ambient * Albedo;
		FinalLightColor = AmbientColor;
	}
	else
	{
		v3 ViewDirection = -Normalize(CurrentPosition);

		v3 Direction;
		f32 FuzzFactor;
		f32 MaxDistance = 10.0f;
		f32 Attenuation = 1.0f;
		if(Light.IsDirectional)
		{
			Direction = -Normalize(Light.Direction);
			FuzzFactor = Light.FuzzFactor;
		}
		else
		{
			Direction = (Light.P - CurrentPosition);
			f32 Distance = Length(Direction);
			Direction /= Distance;

			FuzzFactor = Light.Size/Distance;

			MaxDistance = Distance; // Minimum(MaxDistance, Distance);
			// Attenuation = 1.0f/(1.0f + LightLinearAttenuation*Distance + LightQuadraticAttenuation*Distance*Distance);
			Attenuation = Light.Intensity + Light.LinearAttenuation*Distance;
			// Attenuation = (Distance < LightRadius) ? 1.0f : 0.0f;
			if(MaxDistance > Light.Radius)
			{
				return V3(0,0,0);
			}
		}

		v3 LightDirectionFuzzed = Normalize(Direction);

		f32 Shadow = 1.0f;

		Direction = LightDirectionFuzzed;

		v3 HalfwayDirection = Normalize(Direction + ViewDirection);
		f32 NormalDistribution = SpecularNormalDistribution(CurrentNormal, HalfwayDirection, Roughness);
		f32 GeometryObstruction = GeometryFunction(CurrentNormal, ViewDirection, Direction, Roughness);

		f32 NDotL = Maximum(Inner(CurrentNormal, Direction), 0.0f);
		f32 NDotV = Maximum(Inner(CurrentNormal, ViewDirection), 0.0f);
		f32 Numerator = NormalDistribution * GeometryObstruction;
		f32 Denominator = 4.0f * NDotL * NDotV;
		f32 CookTorranceSpecular = Numerator / Maximum(Denominator, 0.00001f);

		v3 DiffuseComponent = V3(1.0f) - SpecularComponent;
		DiffuseComponent *= 1.0f - Metalness;

		v3 Radiance = Shadow * Attenuation * Light.Intensity * Light.Color;
		FinalLightColor = (DiffuseComponent * Albedo / PI + CookTorranceSpecular * SpecularComponent) * Radiance * NDotL;
	}

	return FinalLightColor;
}

v4 GetDecalColor(decal_shader_data Decal, v3 CurrentPosition)
{
	v4 Result = V4(0,0,0,0);

	v4 DecalCoords = Decal.Projection * Decal.View * inverse(View) * V4(CurrentPosition, 1.0f);
	DecalCoords.xyz /= DecalCoords.w;
	DecalCoords.xyz = 0.5f*DecalCoords.xyz + 0.5f;
	if((DecalCoords.x > 0.0f) && (DecalCoords.x < 1.0f) &&
	   (DecalCoords.y > 0.0f) && (DecalCoords.y < 1.0f) &&
	   (DecalCoords.z > 0.0f) && (DecalCoords.z < 1.0f))
	{
		v3 TextureArrayUV = V3(DecalCoords.xy*DecalTextureScale, f32(DecalTextureIndex));
		v4 TexColor = texture(TextureArray, TextureArrayUV);
		Result = TexColor;
	}

	return Result;
}

void main()
{
	gl_FragDepth = gl_FragCoord.z/(gl_FragCoord.w*FarPlane);

#ifdef SIGNED_DISTANCE_FIELD
	v3 TextureArrayUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
	f32 Distance = texture(TextureArray, TextureArrayUV).a;

	f32 Buffer = 0.45f;
	f32 Gamma = 0.05f;
	f32 Alpha = smoothstep(Buffer - Gamma, Buffer + Gamma, Distance);
	v3 Albedo = ColorToFragment.rgb;
	FragColor.a = Alpha*ColorToFragment.a;

#else
	v3 TextureArrayUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
	v4 TexColor = texture(TextureArray, TextureArrayUV);
	v3 Albedo = TexColor.rgb*Color.rgb*ColorToFragment.rgb;
	FragColor.a = TexColor.a*Color.a*ColorToFragment.a;

#endif

#if 1
	for(s32 DecalIndex = 0;
		DecalIndex < DecalCount;
		++DecalIndex)
	{
		v4 DecalColor = GetDecalColor(Decals[DecalIndex], CameraRay.xyz);
		Albedo = Lerp(Albedo, DecalColor.a, DecalColor.rgb);
	}

	v3 Normal = NormalToFragment;
	v3 ViewDirection = -Normalize(CameraRay.xyz);

	v3 Fresnel = FresnelEquation(Normal, ViewDirection, Albedo, MetalnessToFragment);

	FragColor.rgb = V3(0,0,0);
	for(s32 LightIndex = 0;
		LightIndex < LightCount;
		++LightIndex)
	{
		FragColor.rgb += CalculateLightContribution(Lights[LightIndex], Albedo, Normal, CameraRay.xyz, MetalnessToFragment, Fresnel, RoughnessToFragment);
	}
#else
	FragColor.rgb = Albedo;
#endif
}

#endif
