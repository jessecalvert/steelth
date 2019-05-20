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

layout(location = 0) out v4 Albedo;
layout(location = 1) out v3 Normal;
layout(location = 2) out v2 Roughness_Metalness;

in v4 PositionToFragment;

uniform sampler2DArray TextureArray;
uniform sampler2D DepthTexture;

uniform v3 ViewRay;
uniform mat4 InvView;

uniform int DecalTextureIndex;
uniform v2 DecalTextureScale;
uniform decal_shader_data Decal;

v4 GetDecalColor(decal_shader_data Decal, v3 CurrentPosition)
{
	v4 Result = V4(0,0,0,0);

	v4 DecalCoords = Decal.Projection * Decal.View * InvView * V4(CurrentPosition, 1.0f);
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
#if 1
	v2 TexCoords = PositionToFragment.xy/PositionToFragment.w;
	TexCoords = 0.5f*TexCoords.xy + 0.5f;

	f32 CurrentDepth = texture(DepthTexture, TexCoords).r;
	v3 CurrentPosition = CurrentDepth*ViewRay*V3(TexCoords*2.0f - 1.0f, 1.0f);

	v4 DecalColor = GetDecalColor(Decal, CurrentPosition);
	Albedo = DecalColor;
	Normal = V3(0,0,0);
	Roughness_Metalness = V2(0, 0);

#else

	Albedo = V4(0,1,1,1);
	Normal = V3(0,0,0);
	Roughness_Metalness = V2(0, 0);
#endif
}

#endif
