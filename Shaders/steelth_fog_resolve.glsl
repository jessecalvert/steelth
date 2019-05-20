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

uniform sampler2D FogTexture;
uniform sampler2D DepthTexture;

void main()
{
#if 1
	v2 TexelSize = 1.0f/textureSize(DepthTexture, 0);
	v2i TexelCoords = V2i(TexCoordsToFragment/TexelSize);
	s32 OffsetX = ((TexelCoords.x % 2) == 0) ? -1 : 1;
	s32 OffsetY = ((TexelCoords.y % 2) == 0) ? -1 : 1;
	V2 Offset = 0.5f*V2(OffsetX*TexelSize.x, OffsetY*TexelSize.y);
	v2 Offsets[4] =
	{
		V2(1,1)*TexelSize + Offset,
		V2(1,-1)*TexelSize + Offset,
		V2(-1,1)*TexelSize + Offset,
		V2(-1,-1)*TexelSize + Offset,
	};

#if 0
	// NOTE: Seems like this doesn't do anything.
	s32 BilinearWeightIndex = ((2*OffsetX + OffsetY) + 3)/2;
	v4 BilinearWeights[4] =
	{
		(1.0f/16.0f)*V4(9,3,3,1),
		(1.0f/16.0f)*V4(3,9,1,3),
		(1.0f/16.0f)*V4(3,1,9,3),
		(1.0f/16.0f)*V4(1,3,3,9),
	};
	v4 BilinearWeight = BilinearWeights[BilinearWeightIndex];
#endif

	f32 UpscaledDepth = texture(DepthTexture, TexCoordsToFragment).x;
	f32 TotalWeight = 0.0f;
	v3 FinalColor = V3(0,0,0);
	for(s32 Index = 0;
		Index < 4;
		++Index)
	{
		v3 FogColor = texture(FogTexture, TexCoordsToFragment + Offsets[Index]).xyz;
		f32 DownscaledDepth = texture(DepthTexture, TexCoordsToFragment + Offsets[Index]).x;
		f32 Weight = 1.0f;
		Weight *= Maximum(0.0f, 1.0f - (10.0f)*AbsoluteValue(DownscaledDepth - UpscaledDepth));
		// Weight *= BilinearWeight[Index];

		FogColor *= Weight;
		TotalWeight	+= Weight;
		FinalColor += FogColor;
	}

	FinalColor /= (TotalWeight + 0.0001f);
	FragColor = FinalColor;

#else
	FragColor = texture(FogTexture, TexCoordsToFragment);

#endif
}

#endif
