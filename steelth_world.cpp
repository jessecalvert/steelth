/*@H
* File: steelth_world.cpp
* Author: Jesse Calvert
* Created: April 18, 2019, 14:28
* Last modified: April 26, 2019, 23:55
*/

internal void
GenAddQuad(world *World, v3 P, v3 XAxis, v3 YAxis, v3 Color, f32 Roughness, f32 Metalness, f32 Emission)
{
	u32 FirstIndex = World->VertexCount;
	vertex_format *Vertex0 = World->Vertices + World->VertexCount++;
	vertex_format *Vertex1 = World->Vertices + World->VertexCount++;
	vertex_format *Vertex2 = World->Vertices + World->VertexCount++;
	vertex_format *Vertex3 = World->Vertices + World->VertexCount++;

	Vertex0->Position = P;
	Vertex1->Position = P + XAxis;
	Vertex2->Position = P + YAxis;
	Vertex3->Position = P + XAxis + YAxis;

	v3 Normal = NOZ(Cross(XAxis, YAxis));
	Vertex0->Normal = Normal;
	Vertex1->Normal = Normal;
	Vertex2->Normal = Normal;
	Vertex3->Normal = Normal;

	u32 ColorU32 = PackV3ToU32(Color);
	Vertex0->Color = ColorU32;
	Vertex1->Color = ColorU32;
	Vertex2->Color = ColorU32;
	Vertex3->Color = ColorU32;

	v3 RME = V3(Roughness, Metalness, Emission);
	Vertex0->RME = RME;
	Vertex1->RME = RME;
	Vertex2->RME = RME;
	Vertex3->RME = RME;

	World->Faces[World->FaceCount++] = {FirstIndex, FirstIndex+1, FirstIndex+3};
	World->Faces[World->FaceCount++] = {FirstIndex, FirstIndex+3, FirstIndex+2};
}

internal void
GenAddQuad(world *World, v3 Base, v3 XAxis, v3 YAxis, f32 Width, f32 Height, v3 Color, f32 Roughness, f32 Metalness, f32 Emission)
{
	GenAddQuad(World, Base - 0.5f*Width*XAxis, Width*XAxis, Height*YAxis, Color, Roughness, Metalness, Emission);
}

internal void
GenAddBlock(world *World, v3 Min, v3 Max, v3 Color, f32 Roughness, f32 Metalness, f32 Emission)
{
	v3 Dim = (Max - Min);
	GenAddQuad(World, Min, V3(0, 0, Dim.z), V3(0, Dim.y, 0), Color, Roughness, Metalness, Emission);
	GenAddQuad(World, Min + V3(Dim.x, 0, 0), V3(-Dim.x, 0, 0), V3(0, Dim.y, 0), Color, Roughness, Metalness, Emission);
	GenAddQuad(World, Min + V3(Dim.x, 0, Dim.z), V3(0, 0, -Dim.z), V3(0, Dim.y, 0), Color, Roughness, Metalness, Emission);
	GenAddQuad(World, Min + V3(0, 0, Dim.z), V3(Dim.x, 0, 0), V3(0, Dim.y, 0), Color, Roughness, Metalness, Emission);
	GenAddQuad(World, Min, V3(Dim.x, 0, 0), V3(0, 0, Dim.z), Color, Roughness, Metalness, Emission);
	GenAddQuad(World, Min + V3(0, Dim.y, Dim.z), V3(Dim.x, 0, 0), V3(0, 0, -Dim.z), Color, Roughness, Metalness, Emission);
}

internal void
GenAddWall(world *World, v3 Start, v3 End, f32 Width, f32 Height, v3 Color, f32 Roughness, f32 Metalness, f32 Emission)
{
	f32 WallLength = Length(End - Start);
	v3 Direction = (1/WallLength)*(End - Start);
	v3 Up = V3(0,1,0);
	v3 XAxis = Cross(Up, Direction);
	v3 MidPoint = 0.5f*(Start + End);

	GenAddQuad(World, Start, -XAxis, Up, Width, Height, Color, Roughness, Metalness, Emission);
	GenAddQuad(World, End, XAxis, Up, Width, Height, Color, Roughness, Metalness, Emission);
	GenAddQuad(World, MidPoint + (0.5f*Width)*XAxis, -Direction, Up, WallLength, Height, Color, Roughness, Metalness, Emission);
	GenAddQuad(World, MidPoint + (-0.5f*Width)*XAxis, Direction, Up, WallLength, Height, Color, Roughness, Metalness, Emission);
	GenAddQuad(World, End + Height*Up, XAxis, -Direction, Width, WallLength, Color, Roughness, Metalness, Emission);
}

internal void
GenPlaceProbes(game_state *GameState, game_mode_world *WorldMode, world *World,
	v3 Min, v3 Max)
{
	u32 MaxProbes = MAX_PROBES;
	f32 ProbeSpacing = 5.0f;
	f32 ProbeHeight = 2.0f;

	World->ProbeCount = 0;
	World->ProbePositions = PushArray(&GameState->ModeRegion, MaxProbes, v3);

	v3 Start = Min;
	v3 At = Start;
	while(At.z <= Max.z)
	{
		while(At.y <= Max.y)
		{
			while(At.x <= Max.x)
			{
				v3 Position = At;
				v3 CheckDirections[] =
				{
					NOZ(V3(1, 0, 0)),
					NOZ(V3(-1, 0, 0)),
					NOZ(V3(0, 1, 0)),
					NOZ(V3(0, -1, 0)),
					NOZ(V3(0, 0, 1)),
					NOZ(V3(0, 0, -1)),
				};

				u32 MaxIter = 32;
				u32 Iter = 0;
				for(u32 DirectionIndex = 0;
					DirectionIndex < ArrayCount(CheckDirections);
					)
				{
					v3 CheckDirection = CheckDirections[DirectionIndex];
					ray_cast_result RayCast = AABBRayCastQuery(&WorldMode->Physics.StaticAABBTree,
						Position, CheckDirection);
					f32 HitDistanceSq = LengthSq(Position - RayCast.HitPoint);
					if(RayCast.RayHit)
					{
						if(Inner(RayCast.HitNormal, CheckDirection) > 0.0f)
						{
							// NOTE: Inside geometry
							Iter = MaxIter;
						}
						else if((HitDistanceSq < Square(ProbeHeight)))
						{
							f32 HitDistance = SquareRoot(HitDistanceSq);
							Position = Position + -(ProbeHeight - HitDistance)*CheckDirection;
							DirectionIndex = 0;
						}
						else
						{
							++DirectionIndex;
						}
					}
					else
					{
						++DirectionIndex;
					}

					++Iter;
					if(Iter >= MaxIter)
					{
						break;
					}
				}

				if(Iter < MaxIter)
				{
					Assert(World->ProbeCount < MaxProbes);
					World->ProbePositions[World->ProbeCount++] = Position;
				}

				At.x += ProbeSpacing;
			}

			At.x = Start.x;
			At.y += ProbeSpacing;
		}

		At.y = Start.y;
		At.z += ProbeSpacing;
	}

	Assert(World->ProbeCount <= MAX_PROBES);
}

internal void
GenerateWorld(game_state *GameState, game_mode_world *WorldMode, world *World, render_memory_op_queue *RenderOpQueue)
{
	World->VertexCount = 0;
	World->FaceCount = 0;
	u32 MaxTriangles = 4096;
	World->Vertices = PushArray(&GameState->ModeRegion, 3*MaxTriangles, vertex_format);
	World->Faces = PushArray(&GameState->ModeRegion, MaxTriangles, v3u);

	f32 GroundRadius = 15.0f;
	f32 MaxTriangleSize = 2.0f;
	u32 Steps = (u32)(2*GroundRadius/MaxTriangleSize);
	v3 GroundMin = V3(-GroundRadius, 0.0f, -GroundRadius);

	v3 At = GroundMin;
	for(u32 Z = 0;
		Z <= Steps;
		++Z)
	{
		for(u32 X = 0;
			X <= Steps;
			++X)
		{
			vertex_format *Vertex = World->Vertices + World->VertexCount++;
			Vertex->Position = At;
			Vertex->Normal = V3(0,1,0);
			v4 Color = V4(0.4f, 0.4f, 0.4f, 1);
			Vertex->Color = PackV4ToU32(Color);
			Vertex->RME = V3(1.0f, 0, 0);

			At += V3(MaxTriangleSize, 0, 0);

			if((X < Steps) &&
				(Z < Steps))
			{
				World->Faces[World->FaceCount++] = {Z*(Steps+1) + X, (Z+1)*(Steps+1) + X, Z*(Steps+1) + X + 1};
				World->Faces[World->FaceCount++] = {(Z+1)*(Steps+1) + X, (Z+1)*(Steps+1) + X + 1, Z*(Steps+1) + X + 1};
			}
		}

		At.x = GroundMin.x;
		At += V3(0, 0, MaxTriangleSize);
	}

	u32 MaxLights = 128;
	World->Lights = PushArray(&GameState->ModeRegion, MaxLights, light);

	v3 SkyRadianceDay = 2*(1.0f/256.0f)*V3(115.0f, 136.0f, 250.0f);
	v3 SkyRadianceNight = 0.01f*(1.0f/256.0f)*V3(8.0f, 22.0f, 64.0f);
	u32 DEBUGSceneIndex = 5;
	switch(DEBUGSceneIndex)
	{
		case 0:
		{
			// NOTE: Double wall
			f32 WallWidth = 6.0f;
			f32 WallHeight = 5.0f;

			v3 WallStart = 0.5f*GroundMin;
			v3 WallEnd = WallStart + V3(0, 0, GroundRadius);

			GenAddWall(World, WallStart, WallEnd, WallWidth, WallHeight, V3(1,0.1f,0.1f), 1.0f, 0.0f, 0.0f);
			GenAddWall(World, WallStart + V3(GroundRadius, 0, 0), WallEnd + V3(GroundRadius, 0, 0), WallWidth, WallHeight, V3(0.1f,0.1f,1), 1.0f, 0.0f, 0.0f);

			v3 Min = WallStart + V3(0, 2.0f*WallHeight, 0);
			v3 Max = Min + V3(GroundRadius, WallWidth, GroundRadius);
			GenAddBlock(World, Min, Max, V3(1,1,1), 1.0f, 0.0f, 0.0f);

			v3 LightColor = V3(1.0f, 1.0f, 1.0f);
			f32 Intensity = 5.0f;
			v3 WorldLightDirection = NOZ(V3(0.5f, -1.5f, -1.0f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceDay;
		} break;

		case 1:
		{
			// NOTE: Indoors test
			f32 WallWidth = 2.0f;
			f32 WallHeight = 5.0f;

			v3 Room0 = 0.5f*GroundMin;
			v3 Room1 = Room0 + V3(0, 0, GroundRadius);
			v3 Room2 = Room0 + V3(GroundRadius, 0, GroundRadius);
			v3 Room3 = Room0 + V3(GroundRadius, 0, 0);

			GenAddWall(World, Room0, Room1, WallWidth, WallHeight, V3(0.4f, 0.4f, 0.4f), 1.0f, 0.0f, 0.0f);
			GenAddWall(World, Room1, Room2, WallWidth, WallHeight, V3(0.4f, 0.4f, 0.4f), 1.0f, 0.0f, 0.0f);
			GenAddWall(World, Room2, Room3, WallWidth, WallHeight, V3(0.4f, 0.4f, 0.4f), 1.0f, 0.0f, 0.0f);
			GenAddWall(World, Room3, Room0, WallWidth, WallHeight, V3(0.4f, 0.4f, 0.4f), 1.0f, 0.0f, 0.0f);

			GenAddBlock(World, Room0 + V3(0,WallHeight,0), Room2 + V3(0, WallHeight + WallWidth, -5.0f), V3(0.4f, 0.4f, 0.4f), 1.0f, 0.0f, 0.0f);

			v3 LightColor = V3(1.0f, 1.0f, 1.0f);
			f32 Intensity = 5.0f;
			v3 WorldLightDirection = NOZ(V3(0.5f, -1.5f, -1.0f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceDay;
		} break;

		case 2:
		{
			// NOTE: Outdoors test
			f32 BuildingHeight = 15.0f;
			f32 BuildingWidth = 8.0f;

			v3 Building0 = 0.6f*GroundMin;
			v3 Building1 = V3(-Building0.x, Building0.y, Building0.z);
			v3 Building2 = V3(-Building0.x, Building0.y, -Building0.z);
			v3 Building3 = V3(Building0.x, Building0.y, -Building0.z);

			Building0 = Building0 - 0.5f*V3(BuildingWidth, 0, BuildingWidth);
			Building1 = Building1 - 0.5f*V3(BuildingWidth, 0, BuildingWidth);
			Building2 = Building2 - 0.5f*V3(BuildingWidth, 0, BuildingWidth);
			Building3 = Building3 - 0.5f*V3(BuildingWidth, 0, BuildingWidth);

			GenAddBlock(World, Building0, Building0 + V3(BuildingWidth, BuildingHeight, BuildingWidth), V3(1,0,0), 1.0f, 0.0f, 0.0f);
			GenAddBlock(World, Building1, Building1 + V3(BuildingWidth, BuildingHeight, BuildingWidth), V3(0,1,0), 1.0f, 0.0f, 0.0f);
			GenAddBlock(World, Building2, Building2 + V3(BuildingWidth, BuildingHeight, BuildingWidth), V3(0,0,1), 1.0f, 0.0f, 0.0f);
			GenAddBlock(World, Building3, Building3 + V3(BuildingWidth, BuildingHeight, BuildingWidth), V3(1,1,1), 1.0f, 0.0f, 0.0f);

#if 0
			v3 LightColor = V3(0.8f, 0.8f, 1.0f);
			f32 Intensity = 0.001f;
			v3 WorldLightDirection = NOZ(V3(0.5f, -1.5f, -1.0f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceNight;

#else
			v3 LightColor = V3(1.0f, 1.0f, 1.0f);
			f32 Intensity = 5.0f;
			v3 WorldLightDirection = NOZ(V3(0.5f, -1.5f, -1.0f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceDay;

#endif

		} break;

		case 3:
		{
			// NOTE: Spotlight test.
			f32 WallHeight = 7.0f;

			v3 Colors[] =
			{
				V3(1,0,0),
				V3(0,1,0),
				V3(0,0,1),
				V3(1,1,0),
				V3(1,0,1),
				V3(0,1,1),
				V3(1,1,1),
			};

			GenAddQuad(World, GroundMin + V3(0, WallHeight, 0), V3(2.0f*GroundRadius, 0, 0), V3(0, 0, 2.0f*GroundRadius), V3(1,1,1), 1.0f, 0.0f, 0.0f);

			GenAddQuad(World, GroundMin + V3(0, 0, 2.0f*GroundRadius), V3(0, 0, -2.0f*GroundRadius), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);
			GenAddQuad(World, GroundMin + V3(2.0f*GroundRadius, 0, 2.0f*GroundRadius), V3(-2.0f*GroundRadius, 0, 0), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);
			GenAddQuad(World, GroundMin + V3(2.0f*GroundRadius, 0, 0), V3(0, 0, 2.0f*GroundRadius), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);

			u32 ColorSections = (u32)(2.0f*GroundRadius/MaxTriangleSize);
			f32 SectionLength = 2.0f*GroundRadius/ColorSections;
			for(u32 SectionIndex = 0;
				SectionIndex < ColorSections;
				++SectionIndex)
			{
				v3 Color = Colors[SectionIndex % ArrayCount(Colors)];
				GenAddQuad(World, GroundMin + V3(SectionIndex*SectionLength, 0, 0), V3((f32)SectionLength, 0, 0), V3(0, WallHeight, 0), Color, 1.0f, 0.0f, 0.0f);
			}

			World->Lights[World->LightCount++] = PointLight(V3(0, 3.5f, -GroundRadius + 0.5f), V3(1,1,1), 10.0f, 0.0f, 0.5f*MaxTriangleSize);
			World->SkyRadiance = SkyRadianceDay;
		} break;

		case 4:
		{
			// NOTE: Area light test.
			f32 WallHeight = 7.0f;

			v3 Colors[] =
			{
				V3(1,0,0),
				V3(0,1,0),
				V3(0,0,1),
				V3(1,1,0),
				V3(1,0,1),
				V3(0,1,1),
				V3(1,1,1),
			};

			GenAddQuad(World, GroundMin + V3(0, WallHeight, 0), V3(2.0f*GroundRadius, 0, 0), V3(0, 0, 2.0f*GroundRadius), V3(1,1,1), 1.0f, 0.0f, 0.0f);

			GenAddQuad(World, GroundMin + V3(0, 0, 2.0f*GroundRadius), V3(0, 0, -2.0f*GroundRadius), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);
			GenAddQuad(World, GroundMin + V3(2.0f*GroundRadius, 0, 2.0f*GroundRadius), V3(-2.0f*GroundRadius, 0, 0), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);
			GenAddQuad(World, GroundMin + V3(2.0f*GroundRadius, 0, 0), V3(0, 0, 2.0f*GroundRadius), V3(0, WallHeight, 0), V3(1,1,1), 1.0f, 0.0f, 0.0f);

			u32 ColorSections = (u32)(2.0f*GroundRadius/MaxTriangleSize);
			f32 SectionLength = 2.0f*GroundRadius/ColorSections;
			for(u32 SectionIndex = 0;
				SectionIndex < ColorSections;
				++SectionIndex)
			{
				v3 Color = Colors[SectionIndex % ArrayCount(Colors)];
				f32 Emission = Color.r*Color.b;
				GenAddQuad(World, GroundMin + V3(SectionIndex*SectionLength, 0, 0), V3((f32)SectionLength, 0, 0), V3(0, WallHeight, 0), Color, 1.0f, 0.0f, Emission);
			}
			World->SkyRadiance = SkyRadianceDay;
		} break;

		case 5:
		{
			// NOTE: Fog test
			v3 PillarDim = V3(3.0f, 15.0f, 1.0f);
			f32 PillarSpacing = 1.5f;
			u32 PillarCount = (u32)(2.0f*GroundRadius/(PillarDim.x + PillarSpacing));
			v3 PillarAt = GroundMin;
			for(u32 Index = 0;
				Index < PillarCount;
				++Index)
			{
				GenAddBlock(World, PillarAt, PillarAt + PillarDim, V3(1,1,1), 1.0f, 0.0f, 0.0f);
				PillarAt.x += (PillarDim.x + PillarSpacing);
			}

			GenAddBlock(World, GroundMin + V3(0, PillarDim.y, 0), GroundMin + V3(2.0f*GroundRadius, PillarDim.y + 1.0f, 2.0f*GroundRadius), V3(1,1,1), 1.0f, 0.0f, 0.0f);

#if 0
			v3 LightColor = V3(0.8f, 0.8f, 1.0f);
			f32 Intensity = 0.001f;
			v3 WorldLightDirection = NOZ(V3(0.25f, -1.5f, 2.5f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceNight;

#else
			v3 LightColor = V3(1.0f, 1.0f, 1.0f);
			f32 Intensity = 5.0f;
			v3 WorldLightDirection = NOZ(V3(0.25f, -1.5f, 2.5f));
			f32 LightArcDegrees = 0.0f;
			World->Lights[World->LightCount++] = DirectionalLight(WorldLightDirection, LightColor, Intensity, LightArcDegrees);
			World->SkyRadiance = SkyRadianceDay;

#endif

		} break;
	}

	Assert(World->VertexCount < 3*MaxTriangles);
	Assert(World->FaceCount < MaxTriangles);

	PhysicsAddStaticGeometry(GameState, &WorldMode->Physics,
		World->Vertices, World->VertexCount, World->Faces, World->FaceCount);
	renderer_mesh Mesh = LoadStaticGeometry(GameState->Assets,
		World->Vertices, World->VertexCount, World->Faces, World->FaceCount);

	World->MeshID = GetMeshID(GameState->Assets, Asset_StaticGeometry);
	World->Material.Roughness = 0.0f;
	World->Material.Metalness = 0.0f;
	World->Material.Emission = 0.0f;
	World->Material.Color = V4(1,1,1,1);

#if 0
	// NOTE: PointLight test.
	v3 Colors[] =
	{
		V3(1,1,1),
		V3(0,1,1),
		V3(1,0,1),
		V3(1,1,0),
		V3(1,0,0),
		V3(0,1,0),
		V3(0,0,1),
	};

	f32 Radius = 5.0f;
	f32 Size = 0.0f;
	f32 IntensityPL = 2.0f;
	f32 Step = 4.5f;
	v3 Start = V3(-4.5f, 0.25f, -4.5f);
	u32 XCount = 5;
	u32 ZCount = 5;

	At = Start;
	for(u32 Z = 0;
		Z < ZCount;
		++Z)
	{
		for(u32 X = 0;
			X < XCount;
			++X)
		{
			World->Lights[World->LightCount++] = PointLight(At, Colors[(X+Z) % ArrayCount(Colors)], IntensityPL, Size, Radius);
			At += V3(Step, 0, 0);
		}

		At = Start;
		At += V3(0, 0, Z*Step);
	}
#endif

	Assert(World->LightCount <= MaxLights);

	GenPlaceProbes(GameState, WorldMode, World,
		V3(-GroundRadius + 1.0f, 1.0f, -GroundRadius + 1.0f),
		V3(GroundRadius - 1.0f, GroundRadius + 1.0f, GroundRadius - 1.0f));

	render_memory_op Op = {};
	Op.Type = RenderOp_BakeProbes;
	bake_probes_op *BakeProbes = &Op.BakeProbes;
	BakeProbes->Geometry = Mesh;
	// BakeProbes->Texture = ???;
	BakeProbes->ProbePositions = World->ProbePositions;
	BakeProbes->ProbeCount = World->ProbeCount;
	BakeProbes->Solution = &World->LightingData;
	AddRenderOp(RenderOpQueue, &Op);
}

internal void
RenderLightingSolution(game_state *GameState, game_mode_world *WorldMode, render_group *RenderGroup)
{
	camera *WorldCamera = &WorldMode->WorldCamera;
	SetCamera(RenderGroup, WorldCamera);
	world *World = &WorldMode->World;
	lighting_solution *Solution = &World->LightingData;
	SetFullbright(RenderGroup);

	for(u32 ProbeIndex = 0;
		ProbeIndex < Solution->ProbeCount;
		++ProbeIndex)
	{
		light_probe *Probe = Solution->Probes + ProbeIndex;

		for(u32 DirIndex = 0;
			DirIndex < ArrayCount(ProbeDirections);
			++DirIndex)
		{
			v3 Direction = ProbeDirections[DirIndex];
			f32 SkyVisibility = Probe->LightContributions[DirIndex].w;
			f32 LineLength = 1.0f;
			PushLine(RenderGroup, Probe->P, Probe->P + LineLength*Direction, 0.15f, V4(SkyVisibility, SkyVisibility, SkyVisibility, 1));
		}
	}

	for(u32 SurfelIndex = 0;
		SurfelIndex < Solution->SurfelCount;
		++SurfelIndex)
	{
		surface_element *Surfel = Solution->Surfels + SurfelIndex;

		f32 SideLength = SquareRoot(Surfel->Area);
		quaternion Rotation = RotationQuaternion(V3(0,1,0), Surfel->Normal);
		PushCube(RenderGroup, Surfel->P, V3(SideLength, 0.01f, SideLength), Rotation,
			1.0f, 0.0f, 0.0f,
			V4(Surfel->Albedo, 1.0f));

		f32 NormalLength = 0.3f;
		PushLine(RenderGroup, Surfel->P, Surfel->P + NormalLength*Surfel->Normal, 0.05f, V4(0.5f, 0.1f, 0.1f, 1.0f));
	}
}
