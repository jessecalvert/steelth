/*@H
* File: steelth_renderer_opengl_bake.cpp
* Author: Jesse Calvert
* Created: April 23, 2019, 16:15
* Last modified: April 24, 2019, 20:46
*/

internal u32
HashSurfel(opengl_baked_data *BakedData, v3i PositionIndex, u32 PrincipalNormal)
{
	u32 HashValue = 2*(u32)(PositionIndex.x) +
		3*(u32)(PositionIndex.y) +
		7*(u32)(PositionIndex.z) +
		13*PrincipalNormal;
	HashValue = HashValue % ArrayCount(BakedData->SurfelGridHash);
	return HashValue;
}

internal u32
GetPrincipalDirIndex(v3 Vector)
{
	u32 PrincipalDirIndex = (Vector.x > 0.0f) ? 0 : 1;
	f32 MaxNormalExtentSq = Square(Vector.x);
	f32 yExtent = Square(Vector.y);
	f32 zExtent = Square(Vector.z);
	if(yExtent > MaxNormalExtentSq)
	{
		MaxNormalExtentSq = yExtent;
		PrincipalDirIndex = (Vector.y > 0.0f) ? 2 : 3;
	}
	if(zExtent > MaxNormalExtentSq)
	{
		PrincipalDirIndex = (Vector.z > 0.0f) ? 4 : 5;
	}

	return PrincipalDirIndex;
}

internal v3i
GetPositionIndex(opengl_baked_data *BakedData, v3 P)
{
	v3i PositionIndex = V3i(FloorReal32ToInt32(P.x/BakedData->GridCellDim),
							FloorReal32ToInt32(P.y/BakedData->GridCellDim),
							FloorReal32ToInt32(P.z/BakedData->GridCellDim));
	return PositionIndex;
}

internal surfel_grid_cell *
GetSurfelGridCell(opengl_baked_data *BakedData, surface_element *Surfel)
{
	v3i PositionIndex = GetPositionIndex(BakedData, Surfel->P);
	u32 PrincipalNormal = GetPrincipalDirIndex(Surfel->Normal);
	u32 HashValue = HashSurfel(BakedData, PositionIndex, PrincipalNormal);

	surfel_grid_cell *Result = BakedData->SurfelGridHash[HashValue];
	while(Result)
	{
		if((Result->PositionIndex == PositionIndex) &&
			(Result->PrincipalNormal == PrincipalNormal))
		{
			break;
		}

		Result = Result->NextInHash;
	}

	if(!Result)
	{
		u32 Index = BakedData->SurfelCellCount++;
		Assert(Index < BakedData->MaxSurfelCells);

		Result = BakedData->SurfelCells + Index;
		Result->Index = Index;
		Result->PositionIndex = PositionIndex;
		Result->PrincipalNormal = PrincipalNormal;
		DLIST_INIT(&Result->Sentinel);

		Result->NextInHash = BakedData->SurfelGridHash[HashValue];
		BakedData->SurfelGridHash[HashValue] = Result;
	}

	return Result;
}

internal u32
AddSurfelToCell(opengl_baked_data *BakedData, memory_region *TempRegion, surface_element *Surfel)
{
	surfel_link *Link = PushStruct(TempRegion, surfel_link);
	Link->Surfel = Surfel;

	surfel_grid_cell *Cell = GetSurfelGridCell(BakedData, Surfel);
	DLIST_INSERT(&Cell->Sentinel, Link);

	return Cell->Index;
}

internal opengl_baked_data *
InitOpenGLBakedData(opengl_state *State)
{
	opengl_baked_data *BakedData = State->BakedData = PushStruct(&State->Region, opengl_baked_data);
	BakedData->GridCellDim = 1.0f;

	BakedData->Resolution = V2i(32, 32);
	BakedData->TotalResolution = V2i(6*BakedData->Resolution.x, BakedData->Resolution.y);

	opengl_framebuffer *ProbeGBuffer = &BakedData->ProbeGBuffer;
	texture_settings Settings[] =
	{
		TextureSettings(0, 0, 3, 4, Texture_Float), // Position
		TextureSettings(0, 0, 1, 4, Texture_Float|Texture_DepthBuffer), // Depth
		TextureSettings(0, 0, 3, 4, Texture_Float), // Normal
		TextureSettings(0, 0, 3, 1, 0), // Albedo
		TextureSettings(0, 0, 1, 4, Texture_Float), // Emission
	};

	u32 TextureCount = ArrayCount(Settings);
	OpenGLCreateFramebuffer(ProbeGBuffer, Settings, TextureCount, BakedData->TotalResolution);

	return BakedData;
}

internal void
OpenGLBakeLightProbes(opengl_state *State,
	renderer_mesh StaticGeometry, renderer_texture Texture,
	v3 *ProbePositions, u32 ProbeCount,
	lighting_solution *Solution)
{
	TIMED_FUNCTION();

	Assert(ProbeCount < MAX_PROBES);
	Assert(Solution);

	memory_region TempRegion = {};

	opengl_baked_data *BakedData = State->BakedData;
	if(!BakedData)
	{
		BakedData = InitOpenGLBakedData(State);
	}

	rectangle3 SurfelExtent = InvertedInfinityRectangle3();
	for(u32 ProbeIndex = 0;
		ProbeIndex < ProbeCount;
		++ProbeIndex)
	{
		v3 P = ProbePositions[ProbeIndex];
		if(P.x < SurfelExtent.Min.x) {SurfelExtent.Min.x = P.x;}
		if(P.y < SurfelExtent.Min.y) {SurfelExtent.Min.y = P.y;}
		if(P.z < SurfelExtent.Min.z) {SurfelExtent.Min.z = P.z;}
		if(P.x > SurfelExtent.Max.x) {SurfelExtent.Max.x = P.x;}
		if(P.y > SurfelExtent.Max.y) {SurfelExtent.Max.y = P.y;}
		if(P.z > SurfelExtent.Max.z) {SurfelExtent.Max.z = P.z;}
	}
	v3 ExtentDim = Dim(SurfelExtent);
	v3i GridDim = V3i((s32)(ExtentDim.x/BakedData->GridCellDim),
					  (s32)(ExtentDim.y/BakedData->GridCellDim),
					  (s32)(ExtentDim.z/BakedData->GridCellDim));
	BakedData->MaxSurfelCells = (1 + GridDim.x)*(1 + GridDim.y)*(1 + GridDim.z)*6;
	BakedData->SurfelCells = PushArray(&TempRegion, BakedData->MaxSurfelCells, surfel_grid_cell);

	light_probe_baking *Probes = PushArray(&TempRegion, ProbeCount, light_probe_baking);

	camera ProbeCamera = {};
	ProbeCamera.Type = Camera_Perspective;
	ChangeCameraSettings(&ProbeCamera, 0.5f*Pi32, 1.0f, 0.01f, 100.0f);

	v3 ViewRay = V3(1, 1, -1);
	v3 XAxis = V3(1,0,0);
	v3 YAxis = V3(0,1,0);
	v3 ZAxis = V3(0,0,1);
	quaternion Rotations[] =
	{
		RotationQuaternion(ZAxis, YAxis, -XAxis),
		RotationQuaternion(-ZAxis, YAxis, XAxis),
		RotationQuaternion(XAxis, ZAxis, -YAxis),
		RotationQuaternion(XAxis, -ZAxis, YAxis),
		RotationQuaternion(-XAxis, YAxis, -ZAxis),
		NoRotation(),
	};

	v2i Resolution = BakedData->Resolution;
	v2i TotalResolution = BakedData->TotalResolution;
	u32 SurfelCount = TotalResolution.x*TotalResolution.y;
	u32 BufferSize = SurfelCount*sizeof(v4);
	u32 DepthBufferSize = SurfelCount*sizeof(f32);
	u32 MaxSurfelRefs = TotalResolution.x*TotalResolution.y;
	f32 PixelSolidAngle = 1.0f / (3*Resolution.x*Resolution.y);

	v4 *Positions = (v4 *)PushSize(&TempRegion, BufferSize);
	f32 *Depths = (f32 *)PushSize(&TempRegion, DepthBufferSize);
	v4 *Normals = (v4 *)PushSize(&TempRegion, BufferSize);
	v4 *Albedos = (v4 *)PushSize(&TempRegion, BufferSize);
	v4 *Emissions = (v4 *)PushSize(&TempRegion, BufferSize);

	u32 SurfelsAllocated = 0;
	u32 TotalRefCount = 0;
	for(u32 ProbeIndex = 0;
		ProbeIndex < ProbeCount;
		++ProbeIndex)
	{
		TIMED_BLOCK("Process probe");

		light_probe_baking *Probe = Probes + ProbeIndex;
		Probe->P = ProbePositions[ProbeIndex];
		Probe->SurfelRefs = PushArray(&TempRegion, MaxSurfelRefs, u32);
		MoveCamera(&ProbeCamera, Probe->P);

		shader_bake_probe *ProbeShader = (shader_bake_probe *) BeginShader(State, Shader_BakeProbe);

		glDepthFunc(GL_LEQUAL);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_SCISSOR_TEST);

		opengl_framebuffer *ProbeGBuffer = &BakedData->ProbeGBuffer;
		OpenGLBindFramebuffer(State, ProbeGBuffer);
		f32 MaxDepth = 1.0f;
		f32 Emission = 0.0f;
		glClearBufferfv(GL_DEPTH, 0, &MaxDepth);
		glClearBufferfv(GL_COLOR, 0, V3(0,0,0).E);
		glClearBufferfv(GL_COLOR, 1, V3(0,0,0).E);
		glClearBufferfv(GL_COLOR, 2, V3(0,0,0).E);
		glClearBufferfv(GL_COLOR, 3, &Emission);

		for(u32 DirIndex = 0;
			DirIndex < ArrayCount(Rotations);
			++DirIndex)
		{
			RotateCamera(&ProbeCamera, Rotations[DirIndex]);
			CameraSetMatrices(&ProbeCamera);

			rectangle2i ClipRect =
				Rectangle2i(V2i(DirIndex * Resolution.x, 0), Resolution + V2i(DirIndex * Resolution.x, 0));
			OpenGLBindFramebuffer(State, ProbeGBuffer, ClipRect);

			SetUniform(ProbeShader->Projection, ProbeCamera.Projection_);
			SetUniform(ProbeShader->View, ProbeCamera.View_);
			SetUniform(ProbeShader->FarPlane, ProbeCamera.Far);
			SetUniform(ProbeShader->Orthogonal, false);
			BindTexture(0, State->TextureArray, GL_TEXTURE_2D_ARRAY);

			SetUniform(ProbeShader->MeshTextureIndex, Texture.Index);
			DrawMesh(State, StaticGeometry.Index);
		}

		BindTexture(0, 0, GL_TEXTURE_2D_ARRAY);

		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		EndShader(State);

		glGetTextureImage(ProbeGBuffer->Textures[0], 0, GL_RGBA, GL_FLOAT, BufferSize, Positions);
		glGetTextureImage(ProbeGBuffer->Textures[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, DepthBufferSize, Depths);
		glGetTextureImage(ProbeGBuffer->Textures[2], 0, GL_RGBA, GL_FLOAT, BufferSize, Normals);
		glGetTextureImage(ProbeGBuffer->Textures[3], 0, GL_RGBA, GL_FLOAT, BufferSize, Albedos);
		glGetTextureImage(ProbeGBuffer->Textures[4], 0, GL_RGBA, GL_FLOAT, BufferSize, Emissions);

		for(s32 Y = 0;
			Y < Resolution.y;
			Y++)
		{
			for(s32 X = 0;
				X < Resolution.x;
				++X)
			{
				u32 PixelIndex = Y * (TotalResolution.x) + X;
				for(u32 DirIndex = 0;
					DirIndex < ArrayCount(ProbeDirections);
					++DirIndex)
				{
					f32 Depth = Depths[PixelIndex];

					if(Depth < 1.0f)
					{
						surface_element *Surfel = PushStruct(&TempRegion, surface_element);
						++SurfelsAllocated;
						Surfel->P = Positions[PixelIndex].xyz;
						Surfel->Normal = Normals[PixelIndex].xyz;
						Surfel->Albedo = Albedos[PixelIndex].xyz;
						Surfel->Emission = Emissions[PixelIndex].x;

						u32 CellIndex = AddSurfelToCell(BakedData, &TempRegion, Surfel);

						b32 Found = false;
						for(u32 Index = 0;
							!Found && (Index < Probe->SurfelCount);
							++Index)
						{
							Found = (Probe->SurfelRefs[Index] == CellIndex);
						}

						if(!Found)
						{
							Probe->SurfelRefs[Probe->SurfelCount++] = CellIndex;
							++TotalRefCount;
						}
					}
					else
					{
						v2 TexCoords = V2((f32)X/(f32)Resolution.x, (f32)Y/(f32)Resolution.y);
						TexCoords = 2.0f*TexCoords - V2(1.0f, 1.0f);
						v3 SurfelDirection = NOZ(RotateBy(
							Hadamard(ViewRay, V3(TexCoords, 1.0f)),
							Rotations[DirIndex]));

						// TODO: Pixels close to the cube borders should have a smaller solid angle.
						Probe->SkyVisibility[(SurfelDirection.x > 0.0f) ? 0 : 1] +=
							AbsoluteValue(SurfelDirection.x) * PixelSolidAngle * (6.17f/Pi32);
						Probe->SkyVisibility[(SurfelDirection.y > 0.0f) ? 2 : 3] +=
							AbsoluteValue(SurfelDirection.y) * PixelSolidAngle * (6.17f/Pi32);
						Probe->SkyVisibility[(SurfelDirection.z > 0.0f) ? 4 : 5] +=
							AbsoluteValue(SurfelDirection.z) * PixelSolidAngle * (6.17f/Pi32);
					}

					PixelIndex += (Resolution.x);
				}
			}
		}

		for(u32 DirectionIndex = 0;
			DirectionIndex < ArrayCount(ProbeDirections);
			++DirectionIndex)
		{
			Probe->SkyVisibility[DirectionIndex] = Clamp01(Probe->SkyVisibility[DirectionIndex]);
		}
	}

	Solution->SurfelCount = BakedData->SurfelCellCount;
	Solution->ProbeCount = ProbeCount;
	Solution->SurfelRefCount = TotalRefCount;

	Solution->Surfels = PushArray(&State->Region, Solution->SurfelCount, surface_element);
	Solution->Probes = PushArray(&State->Region, Solution->ProbeCount, light_probe);
	Solution->SurfelRefs = PushArray(&State->Region, Solution->SurfelRefCount, u32);

	for(u32 SurfelIndex = 0;
		SurfelIndex < Solution->SurfelCount;
		++SurfelIndex)
	{
		surface_element *Surfel = Solution->Surfels + SurfelIndex;
		surfel_grid_cell *Cell = BakedData->SurfelCells + SurfelIndex;

		u32 LinkCount = 0;
		for(surfel_link *Link = Cell->Sentinel.Next;
			Link != &Cell->Sentinel;
			Link = Link->Next)
		{
			surface_element *ContainedSurfel = Link->Surfel;
			Surfel->P += ContainedSurfel->P;
			Surfel->Normal += ContainedSurfel->Normal;
			Surfel->Albedo += ContainedSurfel->Albedo;
			Surfel->Emission += ContainedSurfel->Emission;
			++LinkCount;
		}

		f32 InvSurfelCount = (1.0f/LinkCount);
		Surfel->P = InvSurfelCount*Surfel->P;
		Surfel->Normal = NOZ(InvSurfelCount*Surfel->Normal);
		Surfel->Albedo = InvSurfelCount*Surfel->Albedo;
		Surfel->Emission = InvSurfelCount*Surfel->Emission;
		Surfel->Area = Square(BakedData->GridCellDim);

		Assert(Surfel->Albedo != V3(0,0,0));

		f32 MinDistSq = Real32Maximum;
		for(u32 ProbeIndex = 0;
			ProbeIndex < ProbeCount;
			++ProbeIndex)
		{
			v3 P = ProbePositions[ProbeIndex];
			v3 ProbeRelP = P - Surfel->P;
			if(Inner(Surfel->Normal, ProbeRelP) > 0.0f)
			{
				f32 DistSq = LengthSq(ProbeRelP);
				if(DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					Surfel->ClosestProbe = ProbeIndex;
				}
			}
		}
	}

	u32 CurrentRef = 0;
	for(u32 ProbeIndex = 0;
		ProbeIndex < Solution->ProbeCount;
		++ProbeIndex)
	{
		light_probe *Probe = Solution->Probes + ProbeIndex;
		light_probe_baking *ProbeBaking = Probes + ProbeIndex;
		Probe->P = ProbeBaking->P;
		Probe->LightContributions[0].w = ProbeBaking->SkyVisibility[0];
		Probe->LightContributions[1].w = ProbeBaking->SkyVisibility[1];
		Probe->LightContributions[2].w = ProbeBaking->SkyVisibility[2];
		Probe->LightContributions[3].w = ProbeBaking->SkyVisibility[3];
		Probe->LightContributions[4].w = ProbeBaking->SkyVisibility[4];
		Probe->LightContributions[5].w = ProbeBaking->SkyVisibility[5];

		Probe->FirstSurfel = CurrentRef;
		Probe->OnePastLastSurfel = Probe->FirstSurfel + ProbeBaking->SurfelCount;
		for(u32 RefIndex = 0;
			RefIndex < ProbeBaking->SurfelCount;
			++RefIndex)
		{
			Solution->SurfelRefs[CurrentRef++] = ProbeBaking->SurfelRefs[RefIndex];
		}
	}

	Clear(&TempRegion);

	Solution->Valid = true;
}
