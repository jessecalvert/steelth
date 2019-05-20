#ifdef COMPUTE_SHADER

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer SurfelBuffer
{
	surface_element Surfels[MAX_SURFELS];
};

layout(std430, binding = 1) buffer ProbeBuffer
{
	light_probe Probes[MAX_PROBES];
};

uniform b32 Initialize;

uniform mat4 View;

uniform light_shader_data Light;
uniform v3 SkyRadiance;

uniform sampler2D ShadowMap;

uniform mat4 LightProjection[CASCADE_COUNT];
uniform mat4 LightView[CASCADE_COUNT];
uniform f32 LightFarPlane[CASCADE_COUNT];
uniform f32 CascadeBounds[CASCADE_COUNT];
uniform v2 SMRegionScale[CASCADE_COUNT];
uniform v2 SMRegionOffset[CASCADE_COUNT];

// NOTE: Copied from light_pass.glsl. Include files would be a cool thing.
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

	f32 ShadowDepth = LightFarPlane[CascadeIndex]*texture(ShadowMap, ShadowUV.xy).x;
	f32 FragDistanceToLight = -LightSpaceP.z;
	f32 Bias = Maximum(0.005f, 0.01f*(1.0f - Inner(WorldN, -Direction)));
	if(FragDistanceToLight > (ShadowDepth + Bias))
	{
		Result = 0.0f;
	}

	return Result;
}

void main()
{
	u32 Index = gl_GlobalInvocationID.x;
	surface_element Surfel = Surfels[Index];
	v3 LitColor;

	if(Initialize)
	{
		LitColor = V3(0,0,0);

#if 1
		// NOTE: Multiple bounce
		light_probe Probe = Probes[Surfel.ClosestProbe];
		v3 NormalSq = Square(Surfel.Normal);
		v3i IsNegative = V3i(Surfel.Normal.x < 0.0f,
							 Surfel.Normal.y < 0.0f,
							 Surfel.Normal.z < 0.0f);
		v4 Ambient_SkyVisibility =
			NormalSq.x * Probe.LightContributions_SkyVisibility[IsNegative.x + 0] +
			NormalSq.y * Probe.LightContributions_SkyVisibility[IsNegative.y + 2] +
			NormalSq.z * Probe.LightContributions_SkyVisibility[IsNegative.z + 4];

		f32 SkyVisibility = Clamp01(Ambient_SkyVisibility.w);
		v3 Ambient = Ambient_SkyVisibility.xyz + (SkyVisibility*SkyRadiance);
		LitColor = (1.0f/Pi32)*Ambient * Surfel.Albedo;
#endif

		LitColor += Surfel.Emission*Surfel.Albedo;
	}
	else
	{
		LitColor = Surfel.LitColor;

		v3 LightDirection;
		f32 Attenuation = 1.0f;
		f32 Intensity = Light.Intensity;
		v3 Color = Light.Color;

		if(Light.IsDirectional)
		{
			LightDirection = -(Light.Direction);
		}
		else
		{
			LightDirection = Light.P - Surfel.P;
			f32 Distance = Length(LightDirection);
			LightDirection /= Distance;
			Attenuation = Maximum(0.0f, Intensity + Light.LinearAttenuation*Distance);
		}

		v3 ViewP = (View * V4(Surfel.P, 1.0f)).xyz;
		f32 Shadow = 1.0f;
		if(Light.IsDirectional)
		{
			Shadow = CalculateShadow(LightDirection, Surfel.Normal, Surfel.P, ViewP);
		}

		f32 Diffuse = Maximum(Inner(LightDirection, Surfel.Normal), 0.0f);

		LitColor += Shadow * Attenuation * Diffuse * Intensity *
			Color * Surfel.Albedo / Pi32;
	}

	Surfels[Index].LitColor = LitColor;
}

#endif
