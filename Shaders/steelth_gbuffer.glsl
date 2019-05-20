#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;
layout (location = 4) in int TextureIndex;
layout (location = 5) in v3 RME;

out v2 TexCoordsToFragment;
out v4 ColorToFragment;
out v3 NormalToFragment;
out f32 RoughnessToFragment;
out f32 MetalnessToFragment;
out f32 EmissionToFragment;
flat out int TextureIndexToFragment;

uniform mat4 Projection;
uniform mat4 View;

uniform mat4 Model;
uniform mat3 NormalMatrix;
uniform int MeshTextureIndex;
uniform f32 Roughness;
uniform f32 Metalness;
uniform f32 Emission;

void main()
{
	TexCoordsToFragment = TexCoords;
	ColorToFragment = Color;

	gl_Position = Projection * View * Model * V4(Position, 1.0f);
	NormalToFragment = Normalize((View * V4(NormalMatrix * Normal, 0.0f)).xyz);
	TextureIndexToFragment = TextureIndex + MeshTextureIndex; // NOTE: Only one of these should be set, the other 0.
	RoughnessToFragment = Roughness + RME.r;
	MetalnessToFragment = Metalness + RME.g;
	EmissionToFragment = Emission + RME.b;

}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v4 Albedo;
layout(location = 1) out v3 Normal;
layout(location = 2) out v3 RME;

in v2 TexCoordsToFragment;
in v4 ColorToFragment;
in v3 NormalToFragment;
in f32 RoughnessToFragment;
in f32 MetalnessToFragment;
in f32 EmissionToFragment;
flat in int TextureIndexToFragment;

uniform v2 Jitter;
uniform f32 FarPlane;
uniform b32 Orthogonal;

uniform v4 Color;

uniform sampler2DArray TextureArray;

void main()
{

	if(Orthogonal)
	{
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		gl_FragDepth = gl_FragCoord.z/(gl_FragCoord.w*FarPlane);
	}

	v3 TextureArrayUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
	v4 TexColor = texture(TextureArray, TextureArrayUV);
	Albedo = TexColor*Color*ColorToFragment;

	Normal = NormalToFragment;
	RME.x = RoughnessToFragment;
	RME.y = MetalnessToFragment;
	RME.z = EmissionToFragment;
}

#endif
