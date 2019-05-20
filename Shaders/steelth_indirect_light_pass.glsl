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

layout(location = 0) out v3 Color;

in v2 TexCoordsToFragment;

uniform sampler2D AlbedoTexture;
uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform sampler2D RMETexture;
uniform sampler2D FresnelTexture;

uniform sampler3D IrradianceVolume;

uniform v3 ViewRay;
uniform mat4 InvView;
uniform v3 SkyRadiance;

v3 CalculateAmbientCube(v3 Albedo, v3 WorldNormal, v3 CameraRelativeWorldP)
{
	v3 Result = V3(1,1,1);

	v3 VolumeCoords = 0.5f + V3(CameraRelativeWorldP.x / (IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM),
								CameraRelativeWorldP.y / (IRRADIANCE_VOLUME_HEIGHT * IRRADIANCE_VOXEL_DIM),
								CameraRelativeWorldP.z / (IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM));

	if((VolumeCoords.x >= 0.0f) && (VolumeCoords.x <= 1.0f) &&
		(VolumeCoords.y >= 0.0f) && (VolumeCoords.y <= 1.0f) &&
		(VolumeCoords.z >= 0.0f) && (VolumeCoords.z <= 1.0f))
	{
		VolumeCoords.x *= (1.0f/6.0f);
		v3 NegativeOffset = V3((1.0f/6.0f), 0.0f, 0.0f);

		v3 NormalSq = WorldNormal * WorldNormal;
		v3i IsNegative = V3i(WorldNormal.x < 0.0f,
							 WorldNormal.y < 0.0f,
							 WorldNormal.z < 0.0f);
		v4 Ambient_SkyVisibility =
			NormalSq.x * texture(IrradianceVolume, VolumeCoords + (IsNegative.x + 0)*NegativeOffset) +
			NormalSq.y * texture(IrradianceVolume, VolumeCoords + (IsNegative.y + 2)*NegativeOffset) +
			NormalSq.z * texture(IrradianceVolume, VolumeCoords + (IsNegative.z + 4)*NegativeOffset);

		Ambient_SkyVisibility.w = Clamp01(Ambient_SkyVisibility.w);
		Ambient_SkyVisibility.xyz += Ambient_SkyVisibility.w * SkyRadiance;
		Result = Ambient_SkyVisibility.xyz;
	}
	else
	{
		Result = V3(1,0,1);
	}

	Result *= (1.0f/Pi32) * Albedo;

	return Result;
}

void main()
{
	v2 TexCoords = TexCoordsToFragment;
	f32 CurrentDepth = texture(DepthTexture, TexCoords).r;
	if(CurrentDepth == 1.0f)
	{
		discard;
	}
	else
	{
		v3 ViewP = CurrentDepth*ViewRay*V3(TexCoords*2.0f - 1.0f, 1.0f);
		v3 CameraRelativeWorldP = (InvView * V4(ViewP, 0.0f)).xyz;

		v3 Albedo = texture(AlbedoTexture, TexCoords).xyz;
		v3 Normal = texture(NormalTexture, TexCoords).xyz;
		v3 WorldNormal = (InvView * V4(Normal, 0.0f)).xyz;
		v3 RME = texture(RMETexture, TexCoords).xyz;
		v3 Fresnel = texture(FresnelTexture, TexCoords).xyz;

		Color = CalculateAmbientCube(Albedo, WorldNormal, CameraRelativeWorldP);
		Color += RME.z*Albedo;
	}
}

#endif
