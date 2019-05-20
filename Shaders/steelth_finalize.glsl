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

uniform sampler2D SceneTexture;
uniform sampler2D OverlayTexture;
uniform sampler2D BloomTexture;

v3 Uncharted2ToneMap(v3 Color)
{
	// NOTE: http://filmicworlds.com/blog/filmic-tonemapping-operators/
	f32 A = 0.15f;
	f32 B = 0.50f;
	f32 C = 0.10f;
	f32 D = 0.20f;
	f32 E = 0.02f;
	f32 F = 0.30f;

	v3 Result = ((Color*(A*Color + C*B) + D*E)/(Color*(A*Color + B) + D*F)) - E/F;
	return Result;
}

v3 ToneMap(v3 SceneColor)
{
	v3 Result = SceneColor;
#if 0
	// NOTE: Reinhard
	Result = Result/(Result + V3(1.0f));

#else
	// NOTE: Uncharted 2 tone map.
	f32 Exposure = 1.0f;
	f32 ExposureBias = 2.0f;
	f32 W = 11.2f;

	Result *= Exposure;
	Result = Uncharted2ToneMap(ExposureBias*Result);
	v3 WhiteScale = 1.0f/Uncharted2ToneMap(V3(W));
	Result *= WhiteScale;

#endif
	return Result;
}

void main()
{
	v4 SceneColor = texture(SceneTexture, TexCoordsToFragment);
	v4 BloomColor = texture(BloomTexture, TexCoordsToFragment);
	SceneColor += BloomColor;

	SceneColor.rgb = ToneMap(SceneColor.rgb);

	// NOTE: Gamma correction.
	SceneColor = Pow(SceneColor, V4(1.0f / 2.2f));

	v4 OverlayColor = texture(OverlayTexture, TexCoordsToFragment);
	v4 TotalColor = Lerp(SceneColor, OverlayColor.a, OverlayColor);
	FragColor = TotalColor;
}

#endif
