#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;
layout (location = 4) in int TextureIndex;
layout (location = 5) in v3 RME;

out v3 PositionToFragment;
out v2 TexCoordsToFragment;
out v4 ColorToFragment;
out v3 NormalToFragment;
out f32 EmissionToFragment;
flat out int TextureIndexToFragment;

uniform mat4 Projection;
uniform mat4 View;

uniform int MeshTextureIndex;

void main()
{
	TexCoordsToFragment = TexCoords;
	ColorToFragment = Color;

	PositionToFragment = Position;
	gl_Position = Projection * View * V4(Position, 1.0f);
	NormalToFragment = Normal;
	TextureIndexToFragment = MeshTextureIndex;
	EmissionToFragment = RME.z;
}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v3 Position;
layout(location = 1) out v3 Normal;
layout(location = 2) out v3 Albedo;
layout(location = 3) out f32 Emission;

in v3 PositionToFragment;
in v2 TexCoordsToFragment;
in v4 ColorToFragment;
in v3 NormalToFragment;
in f32 EmissionToFragment;
flat in int TextureIndexToFragment;

uniform f32 FarPlane;
uniform b32 Orthogonal;

uniform sampler2DArray TextureArray;

void main()
{
	gl_FragDepth = gl_FragCoord.z/(gl_FragCoord.w*FarPlane);
	Position = PositionToFragment;

	v3 TextureArrayUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
	v4 TexColor = texture(TextureArray, TextureArrayUV);
	Albedo = (TexColor*ColorToFragment).xyz;

	Normal = NormalToFragment;
	Emission = EmissionToFragment;
}

#endif
