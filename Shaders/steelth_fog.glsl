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

layout(location = 0) out v3 FogColor;

in v2 TexCoordsToFragment;

uniform sampler2D SceneColor;
uniform sampler2D DepthTexture;
uniform sampler2D ShadowMap;
uniform sampler3D IrradianceVolume;
uniform sampler2D BlueNoise;

uniform v3 ViewRay;
uniform mat4 View;
uniform mat4 InvView;
uniform f32 FarPlane;

uniform v3 SkyRadiance;

uniform light_shader_data Light;

uniform mat4 LightProjection[CASCADE_COUNT];
uniform mat4 LightView[CASCADE_COUNT];
uniform f32 LightFarPlane[CASCADE_COUNT];
uniform f32 CascadeBounds[CASCADE_COUNT];
uniform v2 SMRegionScale[CASCADE_COUNT];
uniform v2 SMRegionOffset[CASCADE_COUNT];

// NOTE: Copied from light_pass.glsl. #include pls
f32 CalculateShadow(v3 Direction, v3 WorldP, v3 ViewP)
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
	f32 Bias = 0.005f;
	if(FragDistanceToLight > (ShadowDepth + Bias))
	{
		Result = 0.0f;
	}

	return Result;
}

v3 CalculateAmbientCube(v3 CameraRelativeWorldP)
{
	v3 Result = V3(1,1,1);

	v3 VolumeCoords = 0.5f + V3(CameraRelativeWorldP.x / (IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM),
								CameraRelativeWorldP.y / (IRRADIANCE_VOLUME_HEIGHT * IRRADIANCE_VOXEL_DIM),
								CameraRelativeWorldP.z / (IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM));

	VolumeCoords.x *= (1.0f/6.0f);
	v3 Offset = V3((1.0f/6.0f), 0.0f, 0.0f);

	v4 Ambient_SkyVisibility =
		texture(IrradianceVolume, VolumeCoords) +
		texture(IrradianceVolume, VolumeCoords + 1*Offset) +
		texture(IrradianceVolume, VolumeCoords + 2*Offset) +
		texture(IrradianceVolume, VolumeCoords + 3*Offset) +
		texture(IrradianceVolume, VolumeCoords + 4*Offset) +
		texture(IrradianceVolume, VolumeCoords + 5*Offset);
	Ambient_SkyVisibility /= 6.0f;

	Ambient_SkyVisibility.w = Clamp01(Ambient_SkyVisibility.w);
	Ambient_SkyVisibility.xyz += Ambient_SkyVisibility.w * SkyRadiance;
	Result = Ambient_SkyVisibility.xyz;

	Result *= (1.0f/Pi32);

	return Result;
}

f32 CalculateScattering(f32 LDotV, f32 g)
{
	// NOTE: Mie scaterring approximated with Henyey-Greenstein phase function.
	//	http://www.alexandre-pestana.com/volumetric-lights/
	f32 gSq = Square(g);
	f32 Result = 1.0f - gSq;
	Result /= (4.0f * Pi32 * pow(1.0f + gSq - (2.0f * g) * LDotV, 1.5f));
	return Result;
}

v3 RayMarchFog(v3 ViewP, f32 Noise)
{
	s32 Steps = 50;
	f32 g = 0.9f;
	f32 Density = 0.05f;

	f32 Intensity = Light.Intensity;
	v3 LightDirection = -(Light.Direction);

	v3 ViewOrigin = ViewP;
	f32 TotalDistance = Length(ViewP);
	f32 StepLength = TotalDistance/(Steps+1);
	v3 ViewDirection = -(ViewP/TotalDistance);
	ViewOrigin = ViewOrigin + Noise*StepLength*ViewDirection;

	v3 WorldOrigin = (InvView*V4(ViewOrigin, 1.0f)).xyz;
	v3 WorldLightDirection = (InvView*V4(ViewDirection, 0.0f)).xyz;

	v3 FogAccum = V3(0,0,0);
	for(s32 StepIndex = 0;
		StepIndex < Steps;
		++StepIndex)
	{
		v3 ViewTestP = ViewOrigin + StepIndex*StepLength*ViewDirection;
		v3 WorldTestP = WorldOrigin + StepIndex*StepLength*WorldLightDirection;

		f32 Shadow = CalculateShadow(LightDirection, WorldTestP, ViewTestP);
		FogAccum += Light.Intensity*Light.Color*Shadow*CalculateScattering(Inner(-LightDirection, ViewDirection), g);

		v3 CameraRelativeWorldP = (InvView * V4(ViewTestP, 0.0f)).xyz;
		FogAccum += (1.0f/4.0f*Pi32)*CalculateAmbientCube(CameraRelativeWorldP);
	}

	v3 Result = (Density*TotalDistance/Steps)*FogAccum;

	return Result;
}

void main()
{
#if 1
	// NOTE: Raymarched fog
	v2 TexCoords = TexCoordsToFragment;
	f32 CurrentDepth = texture(DepthTexture, TexCoords).r;

	v3 ViewP = CurrentDepth*ViewRay*V3(TexCoords*2.0f - 1.0f, 1.0f);
	f32 Noise = texture(BlueNoise, TexCoords).x;

	FogColor = RayMarchFog(ViewP, Noise);

#else
	// NOTE: Simple linear fog
	// FogColor.w = Square(Square(CurrentDepth));
	// FogColor.xyz = SkyRadiance;

#endif
}

#endif
