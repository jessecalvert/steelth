#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;
layout (location = 1) in v3 Normal;
layout (location = 2) in v2 TexCoords;
layout (location = 3) in v4 Color;
layout (location = 4) in int TextureIndex;

out v2 TexCoordsToFragment;
out v4 ColorToFragment;
flat out int TextureIndexToFragment;

uniform mat4 Transform;

void main()
{
    gl_Position = Transform * V4(Position, 1.0f);
	TexCoordsToFragment = TexCoords;
	ColorToFragment = Color;
	TextureIndexToFragment = TextureIndex;
}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v4 FragColor;

in v2 TexCoordsToFragment;
in v4 ColorToFragment;
flat in int TextureIndexToFragment;

uniform sampler2DArray TextureArray;
uniform b32 IsSDFTexture;

void main()
{
	if(IsSDFTexture)
	{
		v3 TextureUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
		f32 Distance = texture(TextureArray, TextureUV).a;

		f32 Buffer = 0.45f;
		f32 Gamma = 0.05f;
		f32 Alpha = smoothstep(Buffer - Gamma, Buffer + Gamma, Distance);
		FragColor = ColorToFragment;
		FragColor.a *= Alpha;
	}
	else
	{
		v3 TextureUV = V3(TexCoordsToFragment, f32(TextureIndexToFragment));
		v4 TexColor = texture(TextureArray, TextureUV);
		FragColor = ColorToFragment*TexColor;
	}
}

#endif
