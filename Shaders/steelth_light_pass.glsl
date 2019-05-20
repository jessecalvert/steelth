#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;

out v4 PositionToFragment;

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

void main()
{
    gl_Position = Projection * View * Model * V4(Position, 1.0f);
	PositionToFragment = gl_Position;
}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v3 Color;

in v4 PositionToFragment;

uniform sampler2D AlbedoTexture;
uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform sampler2D RMETexture;
uniform sampler2D FresnelTexture;
uniform sampler2D ShadowMap;
uniform sampler2D BlueNoise;

uniform mat4 InvView;
uniform v3 ViewRay;

uniform light_shader_data Light;
uniform b32 CastsShadow;

uniform mat4 LightProjection[CASCADE_COUNT];
uniform mat4 LightView[CASCADE_COUNT];
uniform f32 LightFarPlane[CASCADE_COUNT];
uniform f32 CascadeBounds[CASCADE_COUNT];
uniform v2 SMRegionScale[CASCADE_COUNT];
uniform v2 SMRegionOffset[CASCADE_COUNT];

f32 SpecularNormalDistribution(v3 Normal, v3 HalfwayDirection, f32 Roughness)
{
	// NOTE: Trowbridge-Reitz GGX
	f32 Roughness4 = Pow(Roughness, 4);
	f32 NDotH = Maximum(Inner(Normal, HalfwayDirection), 0.0f);
	f32 NDotHSquare = NDotH*NDotH;
	f32 Denominator = NDotHSquare*(Roughness4 - 1.0f) + 1.0f;
	Denominator = Pi32 * Denominator * Denominator;

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

f32 CalculateShadow(v3 Direction, v3 WorldN, v3 WorldP, v3 ViewP)
{
	f32 Result = 1.0f;

	f32 ViewDepth = -ViewP.z;
	s32 CascadeIndex = 0;
	if(ViewDepth > CascadeBounds[1])
	{
		CascadeIndex = 1;
	}
	if(ViewDepth > CascadeBounds[2])
	{
		CascadeIndex = 2;
	}
	if(ViewDepth > CascadeBounds[3])
	{
		CascadeIndex = 3;
	}

	v4 LightSpaceP = LightView[CascadeIndex] * V4(WorldP, 1.0f);
	v4 ShadowUV_4 = LightProjection[CascadeIndex] * LightSpaceP;
	ShadowUV_4 = 0.5f*(ShadowUV_4/ShadowUV_4.w) + 0.5f;
	v2 ShadowUV = SMRegionScale[CascadeIndex] * ShadowUV_4.xy + SMRegionOffset[CascadeIndex];

#if 0
	// NOTE: Classic sharp shadows
	f32 ShadowDepth = texture(ShadowMap, ShadowUV.xy).x;
	f32 FragDistanceToLight = -LightSpaceP.z/LightFarPlane[CascadeIndex];
	f32 Bias = Maximum(0.00005f, 0.001f*(1.0f - Inner(WorldN, -Direction)));
	if(FragDistanceToLight > (ShadowDepth + Bias))
	{
		Result = 0.0f;
	}

#else
	// NOTE: VSM soft shadows
	v2 Moments = texture(ShadowMap, ShadowUV.xy).xy;
	f32 FragDistanceToLight = -LightSpaceP.z/LightFarPlane[CascadeIndex];
	f32 E_x2 = Moments.y;
	f32 Ex_2 = Square(Moments.x);
	f32 Variance = (E_x2 - Ex_2);
	f32 mD = Moments.x - FragDistanceToLight;
	f32 mD_2 = Square(mD);
	f32 p = Variance / (Variance + mD_2);
	Result = Maximum(p, (FragDistanceToLight <= Moments.x) ? 1.0f : 0.0f);
	// Result = Clamp01(Result);
#endif

	return Result;
}

v3 CalculateLightContribution(light_shader_data Light, v3 Albedo, v3 ViewN, v3 ViewP, f32 Metalness, v3 SpecularComponent, f32 Roughness, v2 TexCoords)
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
		v3 ViewDirection = -Normalize(ViewP);

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
			Direction = (Light.P - ViewP);
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

		f32 Shadow = 1.0f;
		if(CastsShadow)
		{
#if 0
			// NOTE: Blue noise shadow map offset
			v3 RandomDirection = Normalize(2.0f*texture(BlueNoise, TexCoords).xyz - 1.0f);
			v3 RandomOffset = RandomDirection - Inner(RandomDirection, ViewN)*RandomDirection;
			v3 JitteredViewP = ViewP + 0.02f*RandomOffset;
#else
			v3 JitteredViewP = ViewP;
#endif
			v3 JitteredWorldP = (InvView*V4(JitteredViewP, 1.0f)).xyz;
			Shadow = CalculateShadow(Direction, ViewN, JitteredWorldP, JitteredViewP);
		}

		v3 HalfwayDirection = Normalize(Direction + ViewDirection);
		f32 NormalDistribution = SpecularNormalDistribution(ViewN, HalfwayDirection, Roughness);
		f32 GeometryObstruction = GeometryFunction(ViewN, ViewDirection, Direction, Roughness);

		f32 NDotL = Maximum(Inner(ViewN, Direction), 0.0f);
		f32 NDotV = Maximum(Inner(ViewN, ViewDirection), 0.0f);
		f32 Numerator = NormalDistribution * GeometryObstruction;
		f32 Denominator = 4.0f * NDotL * NDotV;
		f32 CookTorranceSpecular = Numerator / Maximum(Denominator, 0.00001f);

		v3 DiffuseComponent = V3(1.0f) - SpecularComponent;
		DiffuseComponent *= 1.0f - Metalness;

		v3 Radiance = Shadow * Attenuation * Light.Intensity * Light.Color;
		FinalLightColor = (DiffuseComponent * Albedo / Pi32 + CookTorranceSpecular * SpecularComponent) * Radiance * NDotL;
	}

	return FinalLightColor;
}

void main()
{
	v2 TexCoords = PositionToFragment.xy/PositionToFragment.w;
	TexCoords = 0.5f*TexCoords + 0.5f;

	v3 Albedo = texture(AlbedoTexture, TexCoords).xyz;
	v3 Normal = texture(NormalTexture, TexCoords).xyz;
	v3 RME = texture(RMETexture, TexCoords).xyz;
	v3 Fresnel = texture(FresnelTexture, TexCoords).xyz;

	f32 CurrentDepth = texture(DepthTexture, TexCoords).r;
	v3 ViewP = CurrentDepth*ViewRay*V3(TexCoords*2.0f - 1.0f, 1.0f);

	Color = CalculateLightContribution(Light, Albedo, Normal, ViewP,
		RME.y, Fresnel, RME.x, TexCoords);
}

#endif
