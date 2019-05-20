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

layout(location = 0) out v3 FragColor;

in v2 TexCoordsToFragment;

uniform sampler2D SceneTexture;

void main()
{
	v3 SceneColor = texture(SceneTexture, TexCoordsToFragment).rgb;
	f32 Brightness = Inner(SceneColor, V3(0.2126f, 0.7152f, 0.0722f));
	f32 MaxBrightness = 2.0f;
	FragColor = (Brightness > MaxBrightness) ? SceneColor : V3(0.0f);
}

#endif
