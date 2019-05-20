/*@H
* File: steelth_world_mode.cpp
* Author: Jesse Calvert
* Created: May 21, 2018, 14:49
* Last modified: April 26, 2019, 19:10
*/

internal void
EntityRevertToInfo(game_mode_world *WorldMode, entity *Entity)
{
	entity_info *Info = Entity->Info;
	if(Info)
	{
		Entity->PieceCount = Info->PieceCount;
		for(u32 Index = 0;
			Index < Entity->PieceCount;
			++Index)
		{
			Entity->Pieces[Index] = Info->Pieces[Index];
		}
		Entity->Color = Info->Color;
		Entity->Flags = Info->Flags;
	}
}

internal entity *
CreateEntity(game_mode_world *WorldMode, entity_manager *Manager, assets *Assets, entity_type Type)
{
	entity *Result = AddEntity(Manager);
	entity_info *Info = GetEntityInfo(Assets, Type);
	Result->Info = Info;
	EntityRevertToInfo(WorldMode, Result);
	return Result;
}

internal entity *
PickEntityRayCast(game_mode_world *WorldMode, v3 Origin, v3 Direction)
{
	entity *Result = 0;
	ray_cast_result RayCast = AABBRayCastQuery(&WorldMode->Physics.DynamicAABBTree, Origin, Direction);
	if(RayCast.RayHit && RayCast.EntityHit)
	{
		Result = RayCast.EntityHit;
	}

	return Result;
}

internal entity *
AddPhysicsBox(game_state *GameState, game_mode_world *WorldMode, v3 P, v3 Dim)
{
	entity *Result = AddEntity(WorldMode->EntityManager);
	Result->P = P;
	Result->Rotation = Q(1,0,0,0);

	Result->BaseAnimation.Type = Animation_Physics;

	collider Sentinel = {};
	DLIST_INIT(&Sentinel);
	collider *Collider0 = BuildBoxCollider(&GameState->FrameRegion, &WorldMode->Physics, V3(0,0,0), Dim, Q(1,0,0,0));
	// collider *Collider0 = BuildSphereCollider(&WorldMode->Physics, V3(0,0,0), Dim.x);
	// collider *Collider0 = BuildCapsuleCollider(&WorldMode->Physics, V3(0,0,0), 0.5f*Dim.x, Dim.x, Q(1,0,0,0));
	DLIST_INSERT(&Sentinel, Collider0);

	move_spec MoveSpec = DefaultMoveSpec();
	InitializeRigidBody(&WorldMode->Physics, Result, &Sentinel, true, MoveSpec);

	surface_material Material = {};
	Material.Roughness = 0.5f;
	Material.Metalness = 0.8f;
	Material.Color = V4(0.8f, 0.8f, 0.8f, 1.0f);

	AddPieceMesh(Result, Dim, NoRotation(), {}, {0}, GetMeshID(GameState->Assets, Asset_RoundedBox), Material);

	return Result;
}

internal b32
EntityIsOnGround(entity *Entity)
{
	b32 Result = Entity->RigidBody.OnSolidGround;
	return Result;
}

internal void
AddDecal(game_mode_world *WorldMode, v3 P, v3 N)
{
	Assert(WorldMode->DecalCount < ArrayCount(WorldMode->Decals));
	oriented_box *NewDecalBox = WorldMode->Decals + WorldMode->DecalCount++;

	v3 Dim = V3(0.5f, 0.5f, 0.05f);

	v3 Up = V3(0,1,0);
	v3 Right = V3(0,0,1);
	v3 Z = N;
	v3 X = NormalizeOr(Cross(Up, Z), Right);
	v3 Y = Cross(Z, X);
	quaternion Rotation = RotationQuaternion(X, Y, Z);

	NewDecalBox->P = P;
	NewDecalBox->Dim = Dim;
	NewDecalBox->Rotation = Rotation;
}

internal v2
WorldCoordinatesToScreenCoordinates(camera *Camera, v3 WorldP, v2i ScreenResolution)
{
	v3 WorldRay = WorldP - Camera->P;
	v3 CameraRay = RotateBy(WorldRay, Conjugate(Camera->Rotation));
	CameraRay.x /= -CameraRay.z;
	CameraRay.y /= -CameraRay.z;
	f32 CameraWidth = 2.0f*Tan(0.5f*Camera->HorizontalFOV);
	f32 CameraHeight = CameraWidth/Camera->AspectRatio;
	v2 NormalizedScreenCoords = V2((CameraRay.x*ScreenResolution.x) / CameraWidth,
								   (CameraRay.y*ScreenResolution.y) / CameraHeight);
	v2 Result = NormalizedScreenCoords + 0.5f*ScreenResolution;

	return Result;
}

internal v3
ScreenPToWorldDirection(camera *Camera, v2 ScreenP, v2i ScreenResolution)
{
	v2 NormalizedP = ScreenP - 0.5f*ScreenResolution;
	f32 CameraWidth = 2.0f*Camera->Near*Tan(0.5f*Camera->HorizontalFOV);
	f32 CameraHeight = CameraWidth/Camera->AspectRatio;
	v3 CameraCoordsP = V3(NormalizedP.x*CameraWidth/ScreenResolution.x,
		NormalizedP.y*CameraHeight/ScreenResolution.y,
		-Camera->Near);
	v3 Result = Normalize(RotateBy(CameraCoordsP, Camera->Rotation));

	return Result;
}

internal ui_panel_positions
GetUIPanelPositions(v2i Dim, s32 Spacing, u32 PanelCount, v2i ScreenResolution, v2i LeftCenterP)
{
	ui_panel_positions Result = {};
	Result.Dim = Dim;
	Result.Spacing = Spacing;
	Result.LeftCenterP = LeftCenterP;
	v2i TotalDim = V2i(Result.Dim.x, PanelCount*(Result.Dim.y + Result.Spacing));
	v2i MinP = LeftCenterP + V2i(Result.Spacing, -TotalDim.y/2);
	MinP.x = Clamp(MinP.x, 0, ScreenResolution.x);
	MinP.y = Clamp(MinP.y, 0, ScreenResolution.y);
	v2i MaxP = MinP + TotalDim;
	MaxP.x = Clamp(MaxP.x, 0, ScreenResolution.x);
	MaxP.y = Clamp(MaxP.y, 0, ScreenResolution.y);
	MinP = MaxP - TotalDim;

	Result.At = V2i(MinP.x, MaxP.y - Result.Dim.y);
	Result.Advance = V2i(0, -(Result.Spacing + Result.Dim.y));

	return Result;
}

internal void
PlayWorldMode(game_state *GameState)
{
	SetGameMode(GameState, GameMode_WorldMode);

	game_mode_world *WorldMode = GameState->WorldMode = PushStruct(&GameState->ModeRegion, game_mode_world);
	RandomSeriesSeed(&WorldMode->Entropy);

	WorldMode->EntityManager = PushStruct(&GameState->ModeRegion, entity_manager);
	InitEntityManager(WorldMode, WorldMode->EntityManager, &GameState->ModeRegion, GameState->Assets);
	InitializePhysics(&WorldMode->Physics, WorldMode->EntityManager, &GameState->ModeRegion);
	InitAnimator(&WorldMode->Animator, &WorldMode->Physics, &GameState->ModeRegion);
	WorldMode->EntityManager->PhysicsState = &WorldMode->Physics;
	WorldMode->EntityManager->Animator = &WorldMode->Animator;

	WorldMode->ParticleSystem = PushStruct(&GameState->ModeRegion, particle_system, AlignNoClear(16));
	InitializeParticleSystem(WorldMode->ParticleSystem, GameState->Assets);

#if GAME_DEBUG
	// NOTE: This was too many ray casts for one frame.
	DEBUGSetRecording(false);
#endif
	GenerateWorld(GameState, WorldMode, &WorldMode->World, GameState->Assets->RenderOpQueue);
#if GAME_DEBUG
	DEBUGSetRecording(true);
#endif

	WorldMode->WorldCamera.Type = Camera_Perspective;
	f32 WorldNear = 0.01f;
	f32 WorldFar = 100.0f;
	f32 WorldHorizontalFOV = 90.0f*(Pi32/180.0f);
	f32 AspectRatio = 16.0f/9.0f;

	ChangeCameraSettings(&WorldMode->WorldCamera, WorldHorizontalFOV, AspectRatio,
		WorldNear, WorldFar);
	MoveCamera(&WorldMode->WorldCamera, V3(0, 0, 0));
	RotateCamera(&WorldMode->WorldCamera, Q(1,0,0,0));
}

internal void
UpdateEntities(game_state *GameState, game_mode_world *WorldMode, f32 Simdt)
{
	FOR_EACH(Entity, WorldMode->EntityManager, entity)
	{
		AnimateEntity(&WorldMode->Animator, Entity, Simdt);

		rigid_body *Body = &Entity->RigidBody;
		if(Body->IsInitialized)
		{
			UpdateWorldCentroidFromPosition(Entity);
			UpdateRotation(Entity);
			AABBUpdate(&WorldMode->Physics.DynamicAABBTree, Entity);
		}
	}

	PhysicsFindAndResolveConstraints(GameState, &WorldMode->Physics, WorldMode->EntityManager, Simdt);

#if 1
	// NOTE: Particle test
	SpawnFire(WorldMode->ParticleSystem, V3(0,0,0));
#endif
}

internal void
RenderEntities(game_state *GameState, game_mode_world *WorldMode,
	render_group *RenderGroup, v2i ScreenResolution)
{
	camera *WorldCamera = &WorldMode->WorldCamera;
	world *World = &WorldMode->World;
	SetCamera(RenderGroup, WorldCamera);

	PushMesh(RenderGroup, V3(0,0,0), NoRotation(), V3(1,1,1),
		World->MeshID, RenderGroup->WhiteTexture, World->Material);
	PushSkyRadiance(RenderGroup, World->SkyRadiance);

	if(World->LightingData.Valid)
	{
		PushLightingSolution(RenderGroup, &World->LightingData);
	}

	for(u32 LightIndex = 0;
		LightIndex < World->LightCount;
		++LightIndex)
	{
		PushLight(RenderGroup, World->Lights[LightIndex]);
	}

	FOR_EACH(Entity, WorldMode->EntityManager, entity)
	{
		for(u32 PieceIndex = 0;
			PieceIndex < Entity->PieceCount;
			++PieceIndex)
		{
			entity_visible_piece *Piece = Entity->Pieces + PieceIndex;
			v3 RenderP = LocalPointToWorldPoint(Entity, Piece->Offset);
			quaternion RenderRotation = Entity->Rotation*Piece->Rotation;
			surface_material *Material = &Piece->Material;
			v4 Color = Material->Color;

			switch(Piece->Type)
			{
				case PieceType_Cube:
				{
					PushCube(RenderGroup, RenderP, Piece->Dim, RenderRotation,
						Material->Roughness, Material->Metalness, Material->Emission, Color);
				} break;

				case PieceType_Bitmap:
				{
					PushTexture(RenderGroup, RenderP, RenderRotation, Piece->Dim.xy,
						Piece->BitmapID, Material->Roughness, Material->Metalness, Material->Emission, Color);
				} break;

				case PieceType_Bitmap_Upright:
				{
					PushUprightTexture(RenderGroup, RenderP, Piece->Dim.xy,
						Piece->BitmapID, Material->Roughness, Material->Metalness, Material->Emission, Color);
				} break;

				case PieceType_Mesh:
				{
					PushMesh(RenderGroup, RenderP, RenderRotation, Piece->Dim,
						Piece->MeshID, Piece->BitmapID, Piece->Material);
				} break;

				InvalidDefaultCase;
			}
		}
	}

#if 0
	u32 Rows = 5;
	u32 SpheresPerRow = 5;
	v3 Scale = V3(0.5f, 0.5f, 0.5f);
	f32 XGap = 1.1f;
	f32 YGap = 1.1f;
	v3 AtP = V3(-0.5f*SpheresPerRow*XGap, 0, 0);
	f32 XInit = AtP.x;
	for(u32 Row = 0;
		Row < Rows;
		++Row)
	{
		for(u32 Index = 0;
			Index < SpheresPerRow;
			++Index)
		{
			surface_material Material = {};
			Material.Roughness = ((f32)Index + 1)/(f32)(SpheresPerRow);
			Material.Metalness = ((f32)Row)/(f32)(Rows - 1);
			Material.Color = V4(COLOR_COPPER, 1.0f);

			PushMesh(RenderGroup, AtP, Q(1,0,0,0), Scale,
				GetMeshID(GameState->Assets, Asset_Sphere),
				RenderGroup->WhiteTexture, Material);
			AtP.x += XGap;
		}

		AtP.x = XInit;
		AtP.y += YGap;
	}
#endif

	for(u32 DecalIndex = 0;
		DecalIndex < WorldMode->DecalCount;
		++DecalIndex)
	{
		oriented_box *DecalBox = WorldMode->Decals + DecalIndex;
		PushDecal(RenderGroup, DecalBox->P, DecalBox->Dim, DecalBox->Rotation, GetBitmapID(RenderGroup->Assets, Asset_Smile));
	}
}

internal void
UpdateAndRenderHUD(game_mode_world *WorldMode, render_group *RenderGroup, v2i ScreenResolution)
{
	WorldMode->ScreenCamera = DefaultScreenCamera(ScreenResolution);
	SetCamera(RenderGroup, &WorldMode->ScreenCamera);
	SetRenderFlags(RenderGroup, RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost|RenderFlag_SDF);

#if 0
	f32 LayoutHeight = 25.0f;
	v2 LayoutAt = V2(25.0f, 5.5f*LayoutHeight);
	rectangle2 LayoutRect = Rectangle2(LayoutAt, LayoutAt);
	ui_layout Layout = BeginUILayout(UI, Asset_TimesFont, LayoutHeight, V4(1,1,1,1), LayoutRect);

	BeginRow(&Layout);
	Label(&Layout, ConstZ("jlaseflisjef"));
	EndRow(&Layout);
#endif

#if 0
	// NOTE: Font texture test.
	renderer_texture LibmonoFont = GetFontBitmap(GameState->Assets, GetFontID(GameState->Assets, Asset_LiberationMonoFont));
	renderer_texture TimesFont = GetFontBitmap(GameState->Assets, GetFontID(GameState->Assets, Asset_TimesFont));
	if(LibmonoFont.Index && TimesFont.Index)
	{
		PushSetup(RenderGroup, &WorldMode->ScreenCamera, Target_Overlay, ScreenResolution, RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost);

		PushTexture(RenderGroup, V2(400, 400), V2(600, 0), V2(0, 600), LibmonoFont);
		PushTexture(RenderGroup, V2(1100, 400), V2(600, 0), V2(0, 600), TimesFont);
	}
#endif
}

internal void
UpdateAndRenderWorldMode(game_state *GameState, game_mode_world *WorldMode,
	game_render_commands *RenderCommands, game_input *Input)
{
	v2i ScreenResolution = Input->ScreenResolution;
	camera *WorldCamera = &WorldMode->WorldCamera;
	f32 AspectRatio = (f32)ScreenResolution.x/(f32)ScreenResolution.y;
	f32 WorldHorizontalFOV = 90.0f*(Pi32/180.0f);
	if(ScreenResolution.x < ScreenResolution.y)
	{
		WorldHorizontalFOV *= AspectRatio;
	}

	ChangeCameraSettings(WorldCamera, WorldHorizontalFOV, AspectRatio,
		WorldCamera->Near, WorldCamera->Far);

	v3 Up = V3(0,1,0);
	v3 Forward = V3(0,0,-1);
	v3 Right = Cross(Forward, Up);
	v3 CameraFacingDirection = Forward;
	f32 CameraDistance = 10.0f;
	if(GameState->UseDebugCamera)
	{
		if(Input->KeyStates[Key_RightClick].Pressed)
		{
			f32 Sensitivity = 0.005f;
			WorldMode->Pitch += Sensitivity*Input->dMouseP.y;
			WorldMode->Pitch = Clamp(WorldMode->Pitch, -0.5f*Pi32, 0.5f*Pi32);
			WorldMode->Yaw += -Sensitivity*Input->dMouseP.x;
		}

		quaternion PitchQ = RotationQuaternion(Right, WorldMode->Pitch);
		quaternion YawQ = RotationQuaternion(Up, WorldMode->Yaw);
		quaternion NewCameraRotation = NormalizeIfNeeded(YawQ*PitchQ);
		CameraFacingDirection = RotateBy(Forward, NewCameraRotation);
		v3 CameraRight = RotateBy(Right, YawQ);

		v3 WorldCameraMove = {};
		r32 CameraMoveScale = 15.0f*Input->Simdt;
		if(Input->KeyStates[Key_Up].Pressed)
		{
			WorldCameraMove += CameraMoveScale*CameraFacingDirection;
		}
		if(Input->KeyStates[Key_Down].Pressed)
		{
			WorldCameraMove += -CameraMoveScale*CameraFacingDirection;
		}
		if(Input->KeyStates[Key_Left].Pressed)
		{
			WorldCameraMove += -CameraMoveScale*CameraRight;
		}
		if(Input->KeyStates[Key_Right].Pressed)
		{
			WorldCameraMove += CameraMoveScale*CameraRight;
		}
		if(Input->KeyStates[Key_Space].Pressed)
		{
			WorldCameraMove += CameraMoveScale*Up;
		}
		if(Input->KeyStates[Key_Ctrl].Pressed)
		{
			WorldCameraMove += -CameraMoveScale*Up;
		}

		v3 NewCameraP = {};
		NewCameraP = WorldCamera->P + WorldCameraMove;
		MoveCamera(WorldCamera, NewCameraP);
		RotateCamera(WorldCamera, NewCameraRotation);
	}
	else
	{
		CameraFacingDirection = Normalize(V3(0.0f, -1.5f, -1.0f));
		v3 CameraLookAt = V3(0.0f, 0.0f, 2.5f);
		v3 NewCameraP = CameraLookAt + -CameraDistance*CameraFacingDirection;
		v3 Z = -CameraFacingDirection;
		v3 X = NormalizeOr(Cross(Up, Z), Right);
		v3 Y = Cross(Z, X);
		quaternion NewCameraRotation = RotationQuaternion(X, Y, Z);

		MoveCamera(WorldCamera, NewCameraP);
		RotateCamera(WorldCamera, NewCameraRotation);
	}

	v3 MouseRayDirection = ScreenPToWorldDirection(WorldCamera, Input->MouseP, ScreenResolution);

	if(Input->KeyStates[Key_K].WasPressed)
	{
		AddPhysicsBox(GameState, WorldMode, V3(0,6,0), V3(1,1,1));
	}

	if(Input->KeyStates[Key_N].WasPressed)
	{
		ray_cast_result RayCast = AABBRayCastQuery(&WorldMode->Physics.StaticAABBTree,
			WorldCamera->P, MouseRayDirection);
		if(RayCast.RayHit && (RayCast.HitNormal != V3(0,0,0)))
		{
			AddDecal(WorldMode, RayCast.HitPoint, RayCast.HitNormal);
		}
	}

	if(Input->KeyStates[Key_D].WasPressed)
	{
		WorldMode->World.DebugDisplayLightingData = !WorldMode->World.DebugDisplayLightingData;
	}

	render_group OverlayGroup = BeginRender(GameState->Assets, RenderCommands, Target_Overlay);
	UpdateAndRenderHUD(WorldMode, &OverlayGroup, ScreenResolution);
	EndRender(&OverlayGroup);

#if 0
	// NOTE: Moving light test
	light *Light = WorldMode->World.Lights + 0;
	if(Light->Type == LightType_Directional)
	{
		directional_light *Sunlight = &Light->Directional;
		Sunlight->Direction = RotateBy(Sunlight->Direction, RotationQuaternion(V3(0,1,0), 0.001f*Pi32));
	}
	else if(Light->Type == LightType_Point)
	{
		point_light *Spot = &Light->Point;
		local_persist f32 t = 0.0f;
		f32 Amplitude = 14.5f;
		f32 Period = 3.0f;
		Spot->P.x = Amplitude * Sin(t/Period);
		t += Input->Simdt;
	}
#endif

	v4 SceneClearColor = {0.1f, 0.2f, 0.2f, 1.0f};
	render_group SceneGroup = BeginRender(GameState->Assets, RenderCommands, Target_Scene, SceneClearColor);
	UpdateEntities(GameState, WorldMode, Input->Simdt);
	UpdateParticleSystem(GameState, WorldMode->ParticleSystem, Input->Simdt);

	if(WorldMode->World.DebugDisplayLightingData)
	{
		RenderLightingSolution(GameState, WorldMode, &SceneGroup);
	}
	else
	{
		RenderEntities(GameState, WorldMode, &SceneGroup, ScreenResolution);
		RenderParticleSystem(GameState, WorldMode->ParticleSystem, &SceneGroup, WorldCamera->P);
	}
	EndRender(&SceneGroup);

	if(Input->KeyStates[Key_M].WasPressed)
	{
		PlayTitleScreen(GameState);
	}
}
