/*@H
* File: steelth_asset_rendering.cpp
* Author: Jesse Calvert
* Created: October 23, 2018, 15:22
* Last modified: April 22, 2019, 17:33
*/

internal void
PushDecal(render_group *Group, v3 P, v3 Dim, quaternion Rotation, bitmap_id BitmapID)
{
	renderer_texture Bitmap = GetBitmap(Group->Assets, BitmapID);
	if(Bitmap.Index)
	{
		PushDecal(Group, P, Dim, Rotation, Bitmap);
	}
	else
	{
		LoadBitmap(Group->Assets, BitmapID);
		++Group->MissingAssetsCount;
	}
}

internal void
PushMesh(render_group *Group, v3 P, quaternion Rotation, v3 Scale,
	mesh_id MeshID, renderer_texture TextureHandle,
	surface_material Material, b32 Wireframe = false)
{
	renderer_mesh Mesh = GetMesh(Group->Assets, MeshID);

	if(Mesh.Index)
	{
		PushMesh(Group, P, Rotation, Scale, Mesh, TextureHandle, Material, Wireframe);
	}
	else
	{
		LoadMesh(Group->Assets, MeshID);
		++Group->MissingAssetsCount;
	}
}

internal void
PushMesh(render_group *Group, v3 P, quaternion Rotation, v3 Scale,
	mesh_id MeshID, bitmap_id BitmapID,
	surface_material Material, b32 Wireframe = false)
{
	renderer_texture Bitmap = GetBitmap(Group->Assets, BitmapID);

	renderer_texture TextureHandle = {};
	if(Bitmap.Index)
	{
		TextureHandle = Bitmap;
	}
	else
	{
		TextureHandle = Group->WhiteTexture;

		LoadBitmap(Group->Assets, BitmapID);
		++Group->MissingAssetsCount;
	}

	PushMesh(Group, P, Rotation, Scale, MeshID, TextureHandle, Material, Wireframe);
}

internal v2
PushText(render_group *Group, font_id Font, v2 P, f32 Height,
		 string String, v4 Color = V4(1,1,1,1), b32 DropShadow = true)
{
	TIMED_FUNCTION();

	font_sla_v3 *FontInfo = GetFontInfo(Group->Assets, Font);
	Assert(FontInfo);

	f32 Scale = GetFontScale(Group, Font, Height);
	v2 SDFOffset = V2(-Scale*FontInfo->Apron, -Scale*FontInfo->Apron);
	P += SDFOffset;

	v2 DropShadowOffset = 0.06f*V2(Height, -Height);
	v4 DropShadowColor = V4(0,0,0,1);

	renderer_texture FontBitmap = GetFontBitmap(Group->Assets, Font);
	if(FontBitmap.Index)
	{

		char LastChar = 0;
		for(u32 CharIndex = 0;
			CharIndex < String.Length;
			++CharIndex)
		{
			char Character = String.Text[CharIndex];
			if(Character == '\n')
			{
				InvalidCodePath;
				// Layout->AtY.y -= Layout->Height;
				// Position = Layout->AtY + SDFOffset;
			}
			else
			{
				P.x += Scale*FontInfo->Kerning[LastChar][Character];

				v2 CharacterTexelDim = FontInfo->TexCoordMax[Character] - FontInfo->TexCoordMin[Character];
				v2 CharacterPixelDim = Scale*Hadamard(V2((f32)FontInfo->BitmapSLA.Width,
					(f32)FontInfo->BitmapSLA.Height),
					CharacterTexelDim);
				v2 CharacterPosition = P + Scale*FontInfo->Offset[Character];

				if(DropShadow)
				{
					PushQuad_(Group, CharacterPosition + DropShadowOffset,
						CharacterPosition + CharacterPixelDim + DropShadowOffset,
						FontInfo->TexCoordMin[Character], FontInfo->TexCoordMax[Character],
						0.0f, FontBitmap, 1.0f, 0.0f, 0.0f, DropShadowColor);
				}

				PushQuad_(Group, CharacterPosition, CharacterPosition + CharacterPixelDim,
					FontInfo->TexCoordMin[Character], FontInfo->TexCoordMax[Character], 0.0f, FontBitmap, 1.0f, 0.0f, 0.0f, Color);

				P.x += Scale*FontInfo->Advance[Character];
			}

			LastChar = Character;
		}
	}
	else
	{
		LoadFont(Group->Assets, Font);
		++Group->MissingAssetsCount;
	}

	P -= SDFOffset;
	return P;
}

internal void
PushColliders(render_group *RenderGroup, entity *Entity)
{
	renderer_texture WhiteTexture = RenderGroup->WhiteTexture;
	rigid_body *Body = &Entity->RigidBody;
	for(collider *Collider = Body->ColliderSentinel.Next;
		Collider != &Body->ColliderSentinel;
		Collider = Collider->Next)
	{
		shape *Shape = &Collider->Shape;
		switch(Shape->Type)
		{
			case Shape_ConvexHull:
			{
				convex_hull *Hull = Shape->ConvexHull;

				for(u32 FaceIndex = 0;
					FaceIndex < Hull->FaceCount;
					++FaceIndex)
				{
					face *Face = Hull->Faces + FaceIndex;
					half_edge *FirstEdge = Hull->Edges + Face->FirstEdge;
					v3 OriginPoint = LocalPointToWorldPoint(Entity, Hull->Vertices[FirstEdge->Origin]);

					v4 BackHullColor = V4(0.7f, 0.2f, 0.2f, 1.0f);
					v4 EdgeColor = V4(0.5f, 0.5f, 0.2f, 1.0f);
					v4 HullColor = V4(0.2f, 0.2f, 0.7f, 1.0f);

					half_edge *PrevEdge = Hull->Edges + FirstEdge->Next;
					half_edge *Edge = Hull->Edges + PrevEdge->Next;
					PushLine(RenderGroup, OriginPoint, LocalPointToWorldPoint(Entity, Hull->Vertices[PrevEdge->Origin]), 0.05f, EdgeColor);
					while(Edge != Hull->Edges + Face->FirstEdge)
					{
						v3 PrevPoint = LocalPointToWorldPoint(Entity, Hull->Vertices[PrevEdge->Origin]);
						v3 Point = LocalPointToWorldPoint(Entity, Hull->Vertices[Edge->Origin]);
						PushQuad_(RenderGroup, OriginPoint, PrevPoint, Point, OriginPoint, V2(0,0), V2(1,1),
							WhiteTexture, true, 1.0f, 0.0f, 0.0f, HullColor);
						PushLine(RenderGroup, PrevPoint, Point, 0.05f, EdgeColor);

						PrevEdge = Edge;
						Edge = Hull->Edges + Edge->Next;
					}

					v3 SecondToLastPoint = LocalPointToWorldPoint(Entity, Hull->Vertices[PrevEdge->Origin]);
					PushLine(RenderGroup, OriginPoint, SecondToLastPoint, 0.05f, EdgeColor);

					PushLine(RenderGroup, LocalPointToWorldPoint(Entity, Face->Center),
						LocalPointToWorldPoint(Entity, Face->Center + 0.5f*Face->Normal), 0.025f);
				}
			} break;

			case Shape_Sphere:
			{
				sphere *Sphere = &Shape->Sphere;
				v3 Center = LocalPointToWorldPoint(Entity, Sphere->Center);
				v3 Scale = V3(Sphere->Radius, Sphere->Radius, Sphere->Radius);
				surface_material Material = {};
				Material.Color = V4(0.5f, 0.5f, 0.5f, 1.0f);
				PushMesh(RenderGroup, Center, Q(1,0,0,0), Scale,
					GetMeshID(RenderGroup->Assets, Asset_Sphere), RenderGroup->WhiteTexture, Material);
			} break;

			case Shape_Capsule:
			{
				capsule *Capsule = &Shape->Capsule;
				v3 A = LocalPointToWorldPoint(Entity, Capsule->P0);
				v3 B = LocalPointToWorldPoint(Entity, Capsule->P1);
				v3 Scale = V3(2.0f*Capsule->Radius, Length(B - A), 2.0f*Capsule->Radius);
				surface_material Material = {};
				Material.Color = V4(0.5f, 0.5f, 0.5f, 1.0f);
				PushMesh(RenderGroup, 0.5f*(A + B), Entity->Rotation, Scale, GetMeshID(RenderGroup->Assets, Asset_Capsule), RenderGroup->WhiteTexture, Material);
			} break;

			InvalidDefaultCase;
		}
	}
}

internal void
PushIntermediateConvexHull(render_group *RenderGroup, qh_convex_hull *Hull)
{
	if(Hull)
	{
		f32 FurthestDistance = 0.0f;
		qh_conflict_vertex *FurthestVertex = 0;
		qh_face *SeenFace = 0;

		for(qh_face *Face = Hull->FaceSentinel.Next;
			Face != &Hull->FaceSentinel;
			Face = Face->Next)
		{
			for(qh_conflict_vertex *Vertex = Face->ConflictSentinel.Next;
				Vertex != &Face->ConflictSentinel;
				Vertex = Vertex->Next)
			{
				v3 WorldPoint = Vertex->P;
				PushPoint(RenderGroup, WorldPoint, V3(0.5f, 0.2f, 0.2f));
				PushLine(RenderGroup, WorldPoint, Face->Center, 0.01f, V4(0.25f, 0.25f, 0.25f, 1.0f));
				if(Vertex->Distance > FurthestDistance)
				{
					FurthestDistance = Vertex->Distance;
					FurthestVertex = Vertex;
					SeenFace = Face;
				}
			}
		}

		if(FurthestVertex)
		{
			v3 Color = V3(1,1,0);
			PushPoint(RenderGroup, FurthestVertex->P, Color);
		}

		for(qh_face *Face = Hull->FaceSentinel.Next;
			Face != &Hull->FaceSentinel;
			Face = Face->Next)
		{
			qh_half_edge *FirstEdge = Face->FirstEdge;
			v3 OriginPoint = FirstEdge->Origin;

			v4 BackHullColor = V4(0.7f, 0.2f, 0.2f, 1.0f);
			v4 EdgeColor = V4(0.5f, 0.5f, 0.2f, 1.0f);
			v4 HullColor = V4(0.2f, 0.2f, 0.7f, 1.0f);
			if(Face == SeenFace)
			{
				HullColor = V4(0.5f, 0.5f, 0.7f, 1.0f);
			}

			qh_half_edge *PrevEdge = FirstEdge->Next;
			qh_half_edge *Edge = PrevEdge->Next;
			PushLine(RenderGroup, OriginPoint, PrevEdge->Origin, 0.05f, EdgeColor);
			while(Edge != Face->FirstEdge)
			{
				v3 PrevPoint = PrevEdge->Origin;
				v3 Point = Edge->Origin;
				PushQuad_(RenderGroup, OriginPoint, PrevPoint, Point, OriginPoint, V2(0,0), V2(1,1),
					RenderGroup->WhiteTexture, true, 1.0f, 0.0f, 0.0f, HullColor);
				// PushQuad_(RenderGroup, OriginPoint, OriginPoint, Point, PrevPoint, V2(0,0), V2(1,1),
				//	RenderGroup->RenderCommands->WhiteTexture, BackHullColor);
				PushLine(RenderGroup, PrevPoint, Point, 0.05f, EdgeColor);

				PrevEdge = Edge;
				Edge = Edge->Next;
			}

			v3 SecondToLastPoint = PrevEdge->Origin;
			PushLine(RenderGroup, OriginPoint, SecondToLastPoint, 0.05f, EdgeColor);

			PushLine(RenderGroup, Face->Center, Face->Center + 0.5f*Face->Normal, 0.025f);
		}
	}
}

internal void
PushTexture(render_group *Group, v3 P, quaternion Rotation, v2 Scale, bitmap_id BitmapID,
	f32 Roughness, f32 Metalness, f32 Emission, v4 Color = V4(1,1,1,1))
{
	renderer_texture Bitmap = GetBitmap(Group->Assets, BitmapID);

	if(Bitmap.Index)
	{
		v3 HalfXAxis = 0.5f*RotateBy(V3(Scale.x, 0, 0), Rotation);
		v3 HalfYAxis = 0.5f*RotateBy(V3(0, Scale.y, 0), Rotation);

		v3 Verts[] =
		{
			P + -HalfXAxis + -HalfYAxis,
			P + HalfXAxis + -HalfYAxis,
			P + HalfXAxis + HalfYAxis,
			P + -HalfXAxis + HalfYAxis,
		};

		PushQuad_(Group, Verts[0], Verts[1], Verts[2], Verts[3], V2(0,0), V2(1,1), Bitmap,
			true, Roughness, Metalness, Emission, Color);
	}
	else
	{
		LoadBitmap(Group->Assets, BitmapID);
		++Group->MissingAssetsCount;
	}
}

internal void
PushTexture(render_group *Group, v2 P, v2 XAxis, v2 YAxis, bitmap_id BitmapID)
{
	renderer_texture Bitmap = GetBitmap(Group->Assets, BitmapID);

	if(Bitmap.Index)
	{
		v2 HalfXAxis = 0.5f * XAxis;
		v2 HalfYAxis = 0.5f * YAxis;
		PushQuad_(Group,
			V3(P - HalfXAxis - HalfYAxis, 0),
			V3(P + HalfXAxis - HalfYAxis, 0),
			V3(P + HalfXAxis + HalfYAxis, 0),
			V3(P - HalfXAxis + HalfYAxis, 0),
			V2(0,0), V2(1,1), Bitmap, true, 1.0f, 0.0f, 0.0f);
	}
	else
	{
		LoadBitmap(Group->Assets, BitmapID);
		++Group->MissingAssetsCount;
	}
}

internal void
PushUprightTexture(render_group *Group, v3 P, v2 Scale,
	bitmap_id BitmapID, f32 Roughness, f32 Metalness, f32 Emission, v4 Color = V4(1,1,1,1))
{
	renderer_texture Bitmap = GetBitmap(Group->Assets, BitmapID);

	if(Bitmap.Index)
	{
		PushUprightTexture(Group, P, Scale, Bitmap, Roughness, Metalness, Emission, Color);
	}
	else
	{
		PushUprightTexture(Group, P, Scale, Group->WhiteTexture, Roughness, Metalness, Emission, Color);
		LoadBitmap(Group->Assets, BitmapID);
		++Group->MissingAssetsCount;
	}
}

internal void
PushTextBillboard_(render_group *Group, v3 Position, v3 Down, v3 Advance, v3 Normal, string String, font_sla_v3 *FontInfo, renderer_texture FontBitmap, f32 Scale, v4 Color)
{
	f32 LineHeight = GetFontTotalLineHeight(FontInfo, Scale);

	char LastChar = 0;
	for(u32 CharIndex = 0;
		CharIndex < String.Length;
		++CharIndex)
	{
		char Character = String.Text[CharIndex];
		Position.x += FontInfo->Kerning[LastChar][Character];

		v2 CharacterTexelDim = FontInfo->TexCoordMax[Character] - FontInfo->TexCoordMin[Character];
		v2 CharacterPixelDim = Scale*Hadamard(V2((r32)FontInfo->BitmapSLA.Width, (r32)FontInfo->BitmapSLA.Height),
			CharacterTexelDim);
		v2 CharOffset = Scale*FontInfo->Offset[Character];
		v3 CharacterPosition = Position + CharOffset.x*Advance + CharOffset.y*(-Down);

		PushQuad_(Group, CharacterPosition,
			CharacterPosition + CharacterPixelDim.x*Advance,
			CharacterPosition + CharacterPixelDim.x*Advance + CharacterPixelDim.y*(-Down),
			CharacterPosition + CharacterPixelDim.y*(-Down),
			FontInfo->TexCoordMin[Character], FontInfo->TexCoordMax[Character],
			FontBitmap, true, 1.0f, 0.0f, 0.0f, Color);

		Position += Scale*FontInfo->Advance[Character]*Advance + 0.001f*Normal;
	}
}

internal void
PushTextBillboard(render_group *Group, font_id Font, f32 Height,
	v3 At, v3 Normal, v3 Down, string String, v4 Color = V4(1,1,1,1))
{
	v3 Advance = Cross(Normal, Down);

	font_sla_v3 *FontInfo = GetFontInfo(Group->Assets, Font);
	Assert(FontInfo);
	renderer_texture FontBitmap = GetFontBitmap(Group->Assets, Font);
	if(FontBitmap.Index)
	{
		f32 Scale = GetFontScale(Group, Font, Height);
		v2 SDFOffset = V2(-Scale*FontInfo->Apron, -Scale*FontInfo->Apron);
		v3 SDFOffsetV3 = SDFOffset.x*Advance + SDFOffset.y*(-Down);

		PushTextBillboard_(Group, At + SDFOffsetV3, Down, Advance, Normal,
			String, FontInfo, FontBitmap, Scale, Color);
	}
	else
	{
		LoadFont(Group->Assets, Font);
		++Group->MissingAssetsCount;
	}
}

