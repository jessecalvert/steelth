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

layout(location = 0) out v4 FragColor;

in v2 TexCoordsToFragment;

uniform sampler2D Texture;

uniform s32 Kernel;

v4 KawaseBlurSample(v2 TexCoord, s32 k)
{
	v2 TexelSize = 1.0f/textureSize(Texture, 0);
	v2 Offset = TexelSize*(k + 0.5f);
	v4 Sample0 = texture(Texture, TexCoord + V2(Offset.x, Offset.y));
	v4 Sample1 = texture(Texture, TexCoord + V2(-Offset.x, Offset.y));
	v4 Sample2 = texture(Texture, TexCoord + V2(Offset.x, -Offset.y));
	v4 Sample3 = texture(Texture, TexCoord + V2(-Offset.x, -Offset.y));
	v4 Result = 0.25f*(Sample0 + Sample1 + Sample2 + Sample3);
	return Result;
}

void main()
{
	FragColor = KawaseBlurSample(TexCoordsToFragment, Kernel);
}

#endif
