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

uniform s32 LOD;

void main()
{
	FragColor = textureLod(Texture, TexCoordsToFragment, f32(LOD));
}

#endif
