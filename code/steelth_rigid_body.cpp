/*@H
* File: steelth_rigid_body.cpp
* Author: Jesse Calvert
* Created: December 6, 2017
* Last modified: April 11, 2019, 14:05
*/

internal void
AddPotentialCollision(game_state *GameState, physics_state *State, entity *A, entity *B)
{
	pointer_pair Pair = {};
	Pair.A = A;
	Pair.B = B;

	u32 Collideable = true;
	for(u32 UncollideableIndex = 0;
		Collideable && (UncollideableIndex < State->UncollideableCount);
		++UncollideableIndex)
	{
		pointer_pair *Uncollideable = State->Uncollideables + UncollideableIndex;
		if(*Uncollideable == Pair)
		{
			Collideable = false;
		}
	}

	if(Collideable)
	{
		bucket *CurrentBucket = 0;
		if(State->BucketCount > 0)
		{
			CurrentBucket = State->PotentialCollisionBuckets + (State->BucketCount - 1);
		}

		if(!CurrentBucket || CurrentBucket->PotentialCollisionCount == BUCKET_SIZE)
		{
			Assert(State->BucketCount < ArrayCount(State->PotentialCollisionBuckets));
			CurrentBucket = State->PotentialCollisionBuckets + State->BucketCount++;
			CurrentBucket->PotentialCollisions = PushArray(&GameState->FrameRegion, BUCKET_SIZE, pointer_pair);
			CurrentBucket->PotentialCollisionCount = 0;
		}

		CurrentBucket->PotentialCollisions[CurrentBucket->PotentialCollisionCount++] = Pair;
	}
}

inline void
ContactBuildBasisFromNormal(contact_data *Contact)
{
	// NOTE: From Erin Catto. Builds tangent vectors in a consistent way.
	if(AbsoluteValue(Contact->Normal.x) >= 0.57735f)
	{
		Contact->Tangent0 = V3(Contact->Normal.y, -Contact->Normal.x, 0.0f);
	}
	else
	{
		Contact->Tangent0 = V3(0.0f, Contact->Normal.z, -Contact->Normal.y);
	}
	Contact->Tangent0 = Normalize(Contact->Tangent0);
	Contact->Tangent1 = Cross(Contact->Normal, Contact->Tangent0);
}

internal contact_manifold
GenerateOneShotManifoldSphereSphere(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	Assert(ColliderA->Shape.Type == Shape_Sphere);
	Assert(ColliderB->Shape.Type == Shape_Sphere);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	contact_manifold Manifold = {};

	v3 CenterA = LocalPointToWorldPoint(EntityA, ColliderA->Shape.Sphere.Center);
	v3 CenterB = LocalPointToWorldPoint(EntityB, ColliderB->Shape.Sphere.Center);

	f32 RadiusA = ColliderA->Shape.Sphere.Radius;
	f32 RadiusB = ColliderB->Shape.Sphere.Radius;

	if(CenterA == CenterB)
	{
		Manifold.ContactCount = 1;
		contact_data *Contact = Manifold.Contacts + 0;

		Contact->WorldContactPointA = CenterA;
		Contact->WorldContactPointB = CenterB;

		Contact->LocalContactPointA = ColliderA->Shape.Sphere.Center;
		Contact->LocalContactPointB = ColliderB->Shape.Sphere.Center;

		Contact->Normal = V3(0,1,0);
		ContactBuildBasisFromNormal(Contact);

		Contact->Penetration = RadiusA + RadiusB;

		Contact->FeatureA.Type = Feature_Vertex;
		Contact->FeatureA.Index[0] = 0;
		Contact->FeatureA.Index[1] = 0;
		Contact->FeatureB.Type = Feature_Vertex;
		Contact->FeatureB.Index[0] = 0;
		Contact->FeatureB.Index[1] = 0;

		Manifold.FeatureTypeA = Feature_Vertex;
		Manifold.FeatureTypeB = Feature_Vertex;
	}
	else
	{
		v3 AtoB = CenterB - CenterA;
		f32 Distance = Length(AtoB);
		f32 Penetration = RadiusA + RadiusB - Distance;
		if(Penetration > 0.0f)
		{
			Manifold.ContactCount = 1;
			contact_data *Contact = Manifold.Contacts + 0;

			Contact->Normal = (1.0f/Distance)*AtoB;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = CenterA + RadiusA*Contact->Normal;
			Contact->WorldContactPointB = CenterB - RadiusB*Contact->Normal;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->Penetration = Penetration;

			Contact->FeatureA.Type = Feature_Vertex;
			Contact->FeatureA.Index[0] = 0;
			Contact->FeatureA.Index[1] = 0;
			Contact->FeatureB.Type = Feature_Vertex;
			Contact->FeatureB.Index[0] = 0;
			Contact->FeatureB.Index[1] = 0;

			Manifold.FeatureTypeA = Feature_Vertex;
			Manifold.FeatureTypeB = Feature_Vertex;
		}
		else
		{
			Manifold.SeparatingAxis = AtoB;
		}
	}

	return Manifold;
}

internal contact_manifold
GenerateOneShotManifoldSphereHull(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	Assert(ColliderA->Shape.Type == Shape_Sphere);
	Assert(ColliderB->Shape.Type == Shape_ConvexHull);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	contact_manifold Manifold = {};

	v3 CenterA = LocalPointToWorldPoint(EntityA, ColliderA->Shape.Sphere.Center);
	f32 RadiusA = ColliderA->Shape.Sphere.Radius;

	convex_hull *HullB = ColliderB->Shape.ConvexHull;

	collider PointCollider = {};
	PointCollider.Shape.Type = Shape_Sphere;

	entity SphereCenter = {};
	SphereCenter.P = SphereCenter.RigidBody.WorldCentroid = CenterA;
	SphereCenter.Rotation = Q(1,0,0,0);
	UpdateRotation(&SphereCenter);

	gjk_result GJKResult = GJK(&SphereCenter, &PointCollider, EntityB, ColliderB, 1.0f);
	if(GJKResult.ContainsOrigin)
	{
		// NOTE: Deep
		convex_hull *Hull = ColliderB->Shape.ConvexHull;

		f32 ClosestDistance = Real32Maximum;
		v3 ClosestFaceNormal = {};
		u32 ClosestFaceIndex = 0;
		for(u32 FaceIndex = 0;
			FaceIndex < Hull->FaceCount;
			++FaceIndex)
		{
			face *Face = Hull->Faces + FaceIndex;
			v3 LocalNormal = Face->Normal;
			v3 PlaneNormal = LocalVectorToWorldVector(EntityB, LocalNormal);
			v3 PlanePoint = LocalPointToWorldPoint(EntityB, Face->Center);

			f32 DistanceToPlane = -Inner(CenterA - PlanePoint, PlaneNormal);
			Assert(DistanceToPlane >= 0.0f);
			if(DistanceToPlane < ClosestDistance)
			{
				ClosestDistance = DistanceToPlane;
				ClosestFaceNormal = PlaneNormal;
				ClosestFaceIndex = FaceIndex;
			}
		}

		Manifold.ContactCount = 1;
		contact_data *Contact = Manifold.Contacts + 0;

		Contact->Normal = -ClosestFaceNormal;
		ContactBuildBasisFromNormal(Contact);

		Contact->WorldContactPointA = CenterA + RadiusA*Contact->Normal;
		Contact->WorldContactPointB = CenterA - ClosestDistance*Contact->Normal;

		Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
		Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

		Contact->Penetration = ClosestDistance + RadiusA;

		Contact->FeatureA.Type = Feature_Vertex;
		Contact->FeatureA.Index[0] = 0;
		Contact->FeatureA.Index[1] = 0;
		Contact->FeatureB.Type = Feature_Face;
		Contact->FeatureB.Index[0] = ClosestFaceIndex;
		Contact->FeatureB.Index[1] = 0;

		Manifold.FeatureTypeA = Feature_Vertex;
		Manifold.FeatureTypeB = Feature_Face;
	}
	else
	{
		TIMED_BLOCK("SphereHull Shallow");

		// NOTE: Shallow
		minkowski_difference_support ClosestPointToOrigin = GJKResult.ClosestPoint;

		v3 AtoB = ClosestPointToOrigin.SupportB - CenterA;
		f32 Distance = Length(AtoB);
		f32 Penetration = RadiusA - Distance;
		if(Penetration > 0.0f)
		{
			Manifold.ContactCount = 1;
			contact_data *Contact = Manifold.Contacts + 0;

			u32 FeaturePointCount = GJKResult.Simplex.Size;
			u32 FeaturePointsIndexes[3] = {};
			for(u32 Index = 0;
				Index < FeaturePointCount;
				++Index)
			{
				v3 FeaturePoint = GJKResult.Simplex.Points[Index].SupportB;

				for(u32 VertexIndex = 0;
					VertexIndex < HullB->VertexCount;
					++VertexIndex)
				{
					v3 HullVertex = HullB->Vertices[VertexIndex];
					if(HullVertex == FeaturePoint)
					{
						FeaturePointsIndexes[Index] = VertexIndex;
						break;
					}
				}
			}

			u32 FeatureIndexes[2] = {};

			// @Robustness: I hate this and it seems dumb and slow. Find a way to get this information
			// without iterating through the convex_hull. Maybe pass more info from GJK.
			switch(FeaturePointCount)
			{
				case 1:
				{
					FeatureIndexes[0] = FeaturePointsIndexes[0];
					Contact->FeatureB.Type = Feature_Vertex;
					Contact->FeatureB.Index[0] = FeatureIndexes[0];
					Contact->FeatureB.Index[1] = FeatureIndexes[1];
				} break;

				case 2:
				{
					for(u32 EdgeIndex = 0;
						EdgeIndex < HullB->EdgeCount;
						EdgeIndex += 2)
					{
						half_edge *Edge = HullB->Edges + EdgeIndex;
						half_edge *Twin = HullB->Edges + EdgeIndex + 1;
						Assert(HullB->Edges + Edge->Twin == Twin);

						if(((Edge->Origin == FeaturePointsIndexes[0]) && (Twin->Origin == FeaturePointsIndexes[1])) ||
						   ((Edge->Origin == FeaturePointsIndexes[1]) && (Twin->Origin == FeaturePointsIndexes[0])))
						{
							FeatureIndexes[0] = EdgeIndex;
							FeatureIndexes[1] = EdgeIndex + 1;
							break;
						}
					}

					Contact->FeatureB.Type = Feature_Edge;
					Contact->FeatureB.Index[0] = FeatureIndexes[0];
					Contact->FeatureB.Index[1] = FeatureIndexes[1];
				} break;

				case 3:
				{
					for(u32 FaceIndex = 0;
						FaceIndex < HullB->FaceCount;
						++FaceIndex)
					{
						face *Face = HullB->Faces + FaceIndex;
						u32 VerticesFound = 0;
						half_edge *Edge = HullB->Edges + Face->FirstEdge;
						do
						{
							if(Edge->Origin == FeaturePointsIndexes[0])
							{
								++VerticesFound;
							}
							if(Edge->Origin == FeaturePointsIndexes[1])
							{
								++VerticesFound;
							}
							if(Edge->Origin == FeaturePointsIndexes[2])
							{
								++VerticesFound;
							}

							Edge = HullB->Edges + Edge->Next;
						} while(Edge != HullB->Edges + Face->FirstEdge);

						if(VerticesFound == FeaturePointCount)
						{
							FeatureIndexes[0] = FaceIndex;
						}
					}

					Contact->FeatureB.Type = Feature_Face;
					Contact->FeatureB.Index[0] = FeatureIndexes[0];
					Contact->FeatureB.Index[1] = FeatureIndexes[1];
				} break;

				InvalidDefaultCase;
			}

			Contact->Normal = (1.0f/Distance)*AtoB;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = ClosestPointToOrigin.SupportA + RadiusA*Contact->Normal;
			Contact->WorldContactPointB = ClosestPointToOrigin.SupportB;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->Penetration = Penetration;

			Contact->FeatureA.Type = Feature_Vertex;
			Contact->FeatureA.Index[0] = 0;
			Contact->FeatureA.Index[1] = 0;

			Manifold.FeatureTypeA = Feature_Vertex;
			Manifold.FeatureTypeB = Contact->FeatureB.Type;
		}
		else
		{
			Manifold.SeparatingAxis = AtoB;
		}
	}

	return Manifold;
}

internal void
ReduceManifoldPoints(manifold_point *Vertices, u32 *VertexCount, v3 Normal)
{
	TIMED_FUNCTION();

	for(u32 Index = 0;
		Index < *VertexCount;
		++Index)
	{
		for(u32 TestIndex = Index + 1;
			TestIndex < *VertexCount;
			++TestIndex)
		{
			if(Vertices[Index] == Vertices[TestIndex])
			{
				Vertices[TestIndex--] = Vertices[--*VertexCount];
				break;
			}
		}
	}

	if(*VertexCount > 4)
	{
		manifold_point FinalVertices[4] = {};
		f32 MaxPenetration = 0.0f;
		for(u32 VertexIndex = 0;
			VertexIndex < *VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = Vertices[VertexIndex];
			f32 Penetration = Vertex.Penetration;
			if(Penetration > MaxPenetration)
			{
				MaxPenetration = Penetration;
				FinalVertices[0] = Vertex;
			}
		}
		Assert(MaxPenetration > 0.0f);

		f32 FurthestDistanceSq = 0.0f;
		for(u32 VertexIndex = 0;
			VertexIndex < *VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = Vertices[VertexIndex];
			f32 DistanceSq = LengthSq(Vertex.P - FinalVertices[0].P);
			if(DistanceSq > FurthestDistanceSq)
			{
				FurthestDistanceSq = DistanceSq;
				FinalVertices[1] = Vertex;
			}
		}
		Assert(FurthestDistanceSq > 0.0f);

		f32 MaxAreaSq = 0.0f;
		f32 MaxSignedArea = 0.0f;
		for(u32 VertexIndex = 0;
			VertexIndex < *VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = Vertices[VertexIndex];
			f32 SignedArea = Inner(Cross(FinalVertices[0].P - Vertex.P, FinalVertices[1].P - Vertex.P), Normal);
			f32 AreaSq = Square(SignedArea);
			if(AreaSq > MaxAreaSq)
			{
				MaxAreaSq = AreaSq;
				MaxSignedArea = SignedArea;
				FinalVertices[2] = Vertex;
			}
		}
		if(MaxSignedArea < 0.0f)
		{
			// NOTE: Fix winding.
			Swap(FinalVertices[0], FinalVertices[1], manifold_point);
		}
		Assert(MaxAreaSq > 0.0f);

		f32 MinArea = 0.0f;
		u32 RemoveEdge = 3; // NOTE: Invalid edge.
		manifold_point FinalVertex = {};
		for(u32 VertexIndex = 0;
			VertexIndex < *VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = Vertices[VertexIndex];
			f32 SignedArea01 = Inner(Cross(FinalVertices[0].P - Vertex.P, FinalVertices[1].P - Vertex.P), Normal);
			f32 SignedArea12 = Inner(Cross(FinalVertices[1].P - Vertex.P, FinalVertices[2].P - Vertex.P), Normal);
			f32 SignedArea20 = Inner(Cross(FinalVertices[2].P - Vertex.P, FinalVertices[0].P - Vertex.P), Normal);

			if(SignedArea01 < MinArea)
			{
				MinArea = SignedArea01;
				RemoveEdge = 0;
				FinalVertex = Vertex;
			}
			if(SignedArea12 < MinArea)
			{
				MinArea = SignedArea12;
				RemoveEdge = 1;
				FinalVertex = Vertex;
			}
			if(SignedArea20 < MinArea)
			{
				MinArea = SignedArea20;
				RemoveEdge = 2;
				FinalVertex = Vertex;
			}
		}
		switch(RemoveEdge)
		{
			case 0:
			{
				FinalVertices[3] = FinalVertices[2];
				FinalVertices[2] = FinalVertices[1];
				FinalVertices[1] = FinalVertex;
			} break;

			case 1:
			{
				FinalVertices[3] = FinalVertices[2];
				FinalVertices[2] = FinalVertex;
			} break;

			case 2:
			{
				FinalVertices[3] = FinalVertex;
			} break;

			InvalidDefaultCase;
		}
		Assert(MinArea < 0.0f);

		// TODO: Ping pong buffer to avoid copy.
		*VertexCount = ArrayCount(FinalVertices);
		for(u32 VertexIndex = 0;
			VertexIndex < ArrayCount(FinalVertices);
			++VertexIndex)
		{
			Vertices[VertexIndex] = FinalVertices[VertexIndex];
		}
	}
}

inline void
FlipManifold(contact_manifold *Manifold)
{
	for(u32 Index = 0;
		Index < Manifold->ContactCount;
		++Index)
	{
		contact_data *Contact = Manifold->Contacts + Index;

		Contact->Normal = -Contact->Normal;
		Contact->Tangent0 = -Contact->Tangent0;

		Swap(Contact->WorldContactPointA, Contact->WorldContactPointB, v3);
		Swap(Contact->LocalContactPointA, Contact->LocalContactPointB, v3);
		Swap(Contact->FeatureA, Contact->FeatureB, feature_tag);
	}

	Swap(Manifold->FeatureTypeA, Manifold->FeatureTypeB, feature_type);
}

#define MAX_CLIP_VERTICES 32

internal u32
ClipAgainstFace(manifold_point *ClipVertices, u32 VertexCountInit,
	entity *Entity, convex_hull *Hull, face *ClipFace)
{
	rigid_body *Body = &Entity->RigidBody;

	manifold_point ClipVerticesScratch[MAX_CLIP_VERTICES];
	manifold_point *PingPongBuffers[2] = {};
	PingPongBuffers[0] = ClipVertices;
	PingPongBuffers[1] = ClipVerticesScratch;

	u32 Ping = 0;
	u32 Pong = 1;
	u32 VertexCount = VertexCountInit;

	v3 ReferenceFaceNormal = LocalVectorToWorldVector(Entity, ClipFace->Normal);
	v3 ReferenceFacePoint = LocalPointToWorldPoint(Entity, ClipFace->Center);

	// NOTE: Sutherland-Hodgman clip.
	u32 EdgeIndex = ClipFace->FirstEdge;
	half_edge *Edge = Hull->Edges + EdgeIndex;
	do
	{
		v3 FaceNormal = {};
		v3 FacePoint = {};

		v3 EdgeOrigin = Hull->Vertices[Edge->Origin];
		half_edge *NextEdge = Hull->Edges + Edge->Next;
		v3 EdgeDirection = Hull->Vertices[NextEdge->Origin] - EdgeOrigin;
		v3 LocalNormal = Cross(EdgeDirection, ClipFace->Normal);
		FaceNormal = LocalVectorToWorldVector(Entity, LocalNormal);
		FacePoint = LocalPointToWorldPoint(Entity, EdgeOrigin);

		u32 ClippedVertexCount = 0;
		manifold_point LastVertex = PingPongBuffers[Ping][VertexCount - 1];
		b32 LastVertexClipped = Inner(LastVertex.P - FacePoint, FaceNormal) > 0.0f;

		for(u32 VertexIndex = 0;
			VertexIndex < VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = PingPongBuffers[Ping][VertexIndex];
			b32 VertexClipped = Inner(Vertex.P - FacePoint, FaceNormal) > 0.0f;

			if(!LastVertexClipped && !VertexClipped)
			{
				Assert(ClippedVertexCount < MAX_CLIP_VERTICES);
				u32 ClipIndex = ClippedVertexCount++;
				PingPongBuffers[Pong][ClipIndex] = Vertex;
			}
			else if(!LastVertexClipped && VertexClipped)
			{
				f32 Denominator = Inner(LastVertex.P-FacePoint, FaceNormal) - Inner(Vertex.P-FacePoint, FaceNormal);
				Assert(Denominator != 0.0f);
				f32 t = Inner(LastVertex.P-FacePoint, FaceNormal)/Denominator;
				manifold_point NewVertex = {};
				NewVertex.P = Lerp(LastVertex.P, t, Vertex.P);

				Assert(ClippedVertexCount < MAX_CLIP_VERTICES);
				u32 ClipIndex = ClippedVertexCount++;
				PingPongBuffers[Pong][ClipIndex] = NewVertex;
				PingPongBuffers[Pong][ClipIndex].EdgeIndexes[0] = LastVertex.EdgeIndexes[0];
				PingPongBuffers[Pong][ClipIndex].EdgeIndexes[1] = EdgeIndex;
			}
			else if(LastVertexClipped && !VertexClipped)
			{
				f32 Denominator = Inner(LastVertex.P-FacePoint, FaceNormal) - Inner(Vertex.P-FacePoint, FaceNormal);
				Assert(Denominator != 0.0f);
				f32 t = Inner(LastVertex.P-FacePoint, FaceNormal)/Denominator;
				manifold_point NewVertex = {};
				NewVertex.P = Lerp(LastVertex.P, t, Vertex.P);

				if(VertexCount != 2)
				{
					Assert(ClippedVertexCount < MAX_CLIP_VERTICES);
					u32 ClipIndex = ClippedVertexCount++;
					PingPongBuffers[Pong][ClipIndex] = NewVertex;
					PingPongBuffers[Pong][ClipIndex].EdgeIndexes[0] = LastVertex.EdgeIndexes[0];
					PingPongBuffers[Pong][ClipIndex].EdgeIndexes[1] = EdgeIndex;
				}

				Assert(ClippedVertexCount < MAX_CLIP_VERTICES);
				u32 ClipIndex = ClippedVertexCount++;
				PingPongBuffers[Pong][ClipIndex] = Vertex;
			}

			LastVertex = Vertex;
			LastVertexClipped = VertexClipped;
		}

		VertexCount = ClippedVertexCount;
		Ping = (Ping+1) % ArrayCount(PingPongBuffers);
		Pong = (Pong+1) % ArrayCount(PingPongBuffers);

		Edge = Hull->Edges + Edge->Next;
	} while(VertexCount && (Edge != Hull->Edges + ClipFace->FirstEdge));

	if(VertexCount)
	{
		u32 PenetratedVerticesCount = 0;
		for(u32 VertexIndex = 0;
			VertexIndex < VertexCount;
			++VertexIndex)
		{
			manifold_point Vertex = PingPongBuffers[Ping][VertexIndex];
			f32 Penetration = -Inner(Vertex.P - ReferenceFacePoint, ReferenceFaceNormal);
			if(Penetration > 0.0f)
			{
				u32 Index = PenetratedVerticesCount++;
				PingPongBuffers[Pong][Index] = Vertex;
				PingPongBuffers[Pong][Index].Penetration = Penetration;
			}
		}

		VertexCount = PenetratedVerticesCount;
		Ping = (Ping+1) % ArrayCount(PingPongBuffers);
		Pong = (Pong+1) % ArrayCount(PingPongBuffers);
	}

	// @Speed: don't really need to copy, but VertexCount is at most 8.
	if(Ping != 0)
	{
		for(u32 Index = 0;
			Index < VertexCount;
			++Index)
		{
			ClipVertices[Index] = PingPongBuffers[Ping][Index];
		}

		Ping = (Ping+1) % ArrayCount(PingPongBuffers);
		Pong = (Pong+1) % ArrayCount(PingPongBuffers);
	}

	return VertexCount;
}
internal contact_manifold
CreateFaceContact(entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB,
	face *BestFaceA)
{
	TIMED_FUNCTION();

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	convex_hull *HullA = ColliderA->Shape.ConvexHull;
	convex_hull *HullB = ColliderB->Shape.ConvexHull;

	contact_manifold Manifold = {};

	v3 ReferenceLocalNormal = BestFaceA->Normal;
	v3 ReferenceFaceNormal = LocalVectorToWorldVector(EntityA, ReferenceLocalNormal);
	v3 ReferenceFacePoint = LocalPointToWorldPoint(EntityA, BestFaceA->Center);

	face *IncidentFace = 0;
	f32 MinInnerProduct = Real32Maximum;
	v3 IncidentNormal = {};
	for(u32 FaceIndex = 0;
		FaceIndex < HullB->FaceCount;
		++FaceIndex)
	{
		face *FaceB = HullB->Faces + FaceIndex;
		v3 LocalNormal = FaceB->Normal;
		v3 PlaneNormalB = LocalVectorToWorldVector(EntityB, LocalNormal);
		f32 InnerProduct = Inner(PlaneNormalB, ReferenceFaceNormal);
		if(InnerProduct < MinInnerProduct)
		{
			MinInnerProduct = InnerProduct;
			IncidentFace = FaceB;
			IncidentNormal = PlaneNormalB;
		}
	}

	u32 VertexCount = 0;
	manifold_point ClipVertices[MAX_CLIP_VERTICES] = {};
	u32 EdgeIndexB = IncidentFace->FirstEdge;
	half_edge *EdgeB = HullB->Edges + EdgeIndexB;
	do
	{
		Assert(VertexCount < MAX_CLIP_VERTICES);
		u32 Index = VertexCount++;
		ClipVertices[Index].P = LocalPointToWorldPoint(EntityB, HullB->Vertices[EdgeB->Origin]);
		ClipVertices[Index].EdgeIndexes[0] = EdgeIndexB;
		ClipVertices[Index].EdgeIndexes[1] = HullA->EdgeCount; // NOTE: Invalid Edge Index;

		EdgeIndexB = EdgeB->Next;
		EdgeB = HullB->Edges + EdgeIndexB;
	} while(EdgeB != HullB->Edges + IncidentFace->FirstEdge);

	VertexCount = ClipAgainstFace(ClipVertices, VertexCount, EntityA, HullA, BestFaceA);

	if(VertexCount)
	{
		ReduceManifoldPoints(ClipVertices, &VertexCount, IncidentNormal);

		Assert(VertexCount <= 4);
		v3 ContactNormal = ReferenceFaceNormal;

		// TODO: This Assert fires when a centroid outside or on the edge of a
		// Hull. That can mean that we placed a centroid wrong, or that the Hull
		// is a triangle, which is annoying.
		// Assert(Inner(ReferenceFacePoint - BodyA->WorldCentroid, ContactNormal) > 0.0f);

		Manifold.ContactCount = VertexCount;
		for(u32 Index = 0;
			Index < VertexCount;
			++Index)
		{
			manifold_point Vertex = ClipVertices[Index];
			contact_data *Contact = Manifold.Contacts + Index;

			Contact->Penetration = Vertex.Penetration;

			Contact->Normal = ContactNormal;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = Vertex.P + Contact->Penetration*ReferenceFaceNormal;
			Contact->WorldContactPointB = Vertex.P;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->FeatureA.Type = Feature_Edge;
			Contact->FeatureA.Index[0] = Vertex.EdgeIndexes[1];
			Contact->FeatureA.Index[1] = 0;
			Contact->FeatureB.Type = Feature_Edge;
			Contact->FeatureB.Index[0] = Vertex.EdgeIndexes[0];
			Contact->FeatureB.Index[1] = 0;
		}

		switch(VertexCount)
		{
			case 0:
			{
				Manifold.FeatureTypeA = Feature_None;
				Manifold.FeatureTypeB = Feature_None;
			} break;

			case 1:
			{
				Manifold.FeatureTypeA = Feature_Face;
				Manifold.FeatureTypeB = Feature_Vertex;
			} break;

			case 2:
			{
				Manifold.FeatureTypeA = Feature_Face;
				Manifold.FeatureTypeB = Feature_Edge;
			} break;

			case 3:
			case 4:
			{
				Manifold.FeatureTypeA = Feature_Face;
				Manifold.FeatureTypeB = Feature_Face;
			} break;

			InvalidDefaultCase;
		}
	}

	return Manifold;
}

internal contact_manifold
GenerateOneShotManifoldHullHull(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB,
	v3 CachedSeparatingAxis)
{
	// TODO: Is there some way to cache the best separating axis when the objects overlap?

	TIMED_FUNCTION();

	Assert(ColliderA->Shape.Type == Shape_ConvexHull);
	Assert(ColliderB->Shape.Type == Shape_ConvexHull);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	convex_hull *HullA = ColliderA->Shape.ConvexHull;
	convex_hull *HullB = ColliderB->Shape.ConvexHull;

	f32 Epsilon = 0.00001f;

	contact_manifold Manifold = {};

	// NOTE: Test CachedSeparatingAxis.
	b32 CachedSeparates = false;
	if(LengthSq(CachedSeparatingAxis) != 0.0f)
	{
		// TODO: If we keep track of the separating features, we don't have to
		//	do as many Support calls.
		v3 SupportA0 = Support(EntityA, ColliderA, CachedSeparatingAxis);
		v3 SupportA1 = Support(EntityA, ColliderA, -CachedSeparatingAxis);
		v3 SupportB0 = Support(EntityB, ColliderB, CachedSeparatingAxis);
		v3 SupportB1 = Support(EntityB, ColliderB, -CachedSeparatingAxis);

		f32 ProjectedSupportA0 = Inner(SupportA0, CachedSeparatingAxis);
		f32 ProjectedSupportA1 = Inner(SupportA1, CachedSeparatingAxis);
		f32 ProjectedSupportB0 = Inner(SupportB0, CachedSeparatingAxis);
		f32 ProjectedSupportB1 = Inner(SupportB1, CachedSeparatingAxis);

		f32 MinA = Minimum(ProjectedSupportA0, ProjectedSupportA1);
		f32 MaxA = Maximum(ProjectedSupportA0, ProjectedSupportA1);
		f32 MinB = Minimum(ProjectedSupportB0, ProjectedSupportB1);
		f32 MaxB = Maximum(ProjectedSupportB0, ProjectedSupportB1);

		CachedSeparates = ((MaxA < MinB) || (MaxB < MinA));
	}

	if(!CachedSeparates)
	{
		// NOTE: Test face separation of A.
		v3 BestSeparatingAxisFaceA = {};
		f32 BestSeparationFaceA = Real32Minimum;
		face *BestFaceA = 0;
		for(u32 FaceIndex = 0;
			FaceIndex < HullA->FaceCount;
			++FaceIndex)
		{
			face *FaceA = HullA->Faces + FaceIndex;
			v3 LocalNormal = FaceA->Normal;
			v3 PlaneNormalA = LocalVectorToWorldVector(EntityA, LocalNormal);
			v3 PlanePointA = LocalPointToWorldPoint(EntityA, FaceA->Center);

			v3 WorldSupportB = Support(EntityB, ColliderB, -PlaneNormalA);

			f32 Separation = Inner(WorldSupportB - PlanePointA, PlaneNormalA);
			if(Separation > BestSeparationFaceA)
			{
				BestSeparatingAxisFaceA = PlaneNormalA;
				BestSeparationFaceA = Separation;
				BestFaceA = FaceA;
			}
		}

		if(BestSeparationFaceA <= 0.0f)
		{
			// NOTE: Test face separation of B.
			v3 BestSeparatingAxisFaceB = {};
			f32 BestSeparationFaceB = Real32Minimum;
			face *BestFaceB = 0;
			for(u32 FaceIndex = 0;
				FaceIndex < HullB->FaceCount;
				++FaceIndex)
			{
				face *FaceB = HullB->Faces + FaceIndex;
				v3 LocalNormal = FaceB->Normal;
				v3 PlaneNormalB = LocalVectorToWorldVector(EntityB, LocalNormal);
				v3 PlanePointB = LocalPointToWorldPoint(EntityB, FaceB->Center);

				v3 WorldSupportA = Support(EntityA, ColliderA, -PlaneNormalB);

				f32 Separation = Inner(WorldSupportA - PlanePointB, PlaneNormalB);
				if(Separation > BestSeparationFaceB)
				{
					BestSeparatingAxisFaceB = PlaneNormalB;
					BestSeparationFaceB = Separation;
					BestFaceB = FaceB;
				}
			}

			if(BestSeparationFaceB <= 0.0f)
			{
				// NOTE: Test edge separation.
				v3 BestSeparatingAxisEdges = {};
				f32 BestSeparationEdges = Real32Minimum;

				v3 BestEdgeDirectionA = {};
				v3 BestEdgePointA = {};
				v3 BestEdgeDirectionB = {};
				v3 BestEdgePointB = {};
				v3 BestPlaneNormal = {};
				u32 BestEdgeIndexA = 0;
				u32 BestEdgeIndexB = 0;

				{
					TIMED_BLOCK("Test Edge Separation");
					for(u32 EdgeIndexA = 0;
						EdgeIndexA < HullA->EdgeCount;
						EdgeIndexA +=2)
					{
						half_edge *EdgeA = HullA->Edges + EdgeIndexA;
						half_edge *EdgeTwinA = HullA->Edges + EdgeIndexA + 1;
						face *FaceA = HullA->Faces + EdgeA->Face;
						face *TwinFaceA = HullA->Faces + EdgeTwinA->Face;
						v3 LocalEdgeStartA = HullA->Vertices[EdgeA->Origin];
						v3 LocalEdgeEndA = HullA->Vertices[EdgeTwinA->Origin];
						v3 LocalEdgeDirectionA = (LocalEdgeEndA - LocalEdgeStartA);
						v3 EdgeDirectionA = LocalVectorToWorldVector(EntityA, LocalEdgeDirectionA);
						v3 EdgePointA = LocalPointToWorldPoint(EntityA, LocalEdgeStartA);
						v3 FaceNormalA0 = FaceA->Normal;
						v3 FaceNormalA1 = TwinFaceA->Normal;

						for(u32 EdgeIndexB = 0;
							EdgeIndexB < HullB->EdgeCount;
							EdgeIndexB += 2)
						{
							half_edge *EdgeB = HullB->Edges + EdgeIndexB;
							half_edge *EdgeTwinB = HullB->Edges + EdgeIndexB + 1;
							face *FaceB = HullB->Faces + EdgeB->Face;
							face *TwinFaceB = HullB->Faces + EdgeTwinB->Face;
							v3 LocalEdgeStartB = HullB->Vertices[EdgeB->Origin];
							v3 LocalEdgeEndB = HullB->Vertices[EdgeTwinB->Origin];
							v3 LocalEdgeDirectionB = (LocalEdgeEndB - LocalEdgeStartB);
							v3 EdgeDirectionB = LocalVectorToWorldVector(EntityB, LocalEdgeDirectionB);
							v3 EdgePointB = LocalPointToWorldPoint(EntityB, LocalEdgeStartB);
							v3 FaceNormalB0 = FaceB->Normal;
							v3 FaceNormalB1 = TwinFaceB->Normal;

							// NOTE: Gauss edge pruning thanks to Dirk Gregorius.
							f32 CBA = Inner(FaceNormalB0, EdgeDirectionA);
							f32 DBA = Inner(FaceNormalB1, EdgeDirectionA);
							f32 ADC = Inner(FaceNormalA0, EdgeDirectionB);
							f32 BDC = Inner(FaceNormalA1, EdgeDirectionB);
							b32 IsMinkowskiFace = (CBA*DBA < 0.0f) && (ADC*BDC < 0.0f) && (CBA*BDC > 0.0f);

							if(IsMinkowskiFace)
							{
								v3 PlaneNormal = Cross(EdgeDirectionA, EdgeDirectionB);
								if(LengthSq(PlaneNormal) > Square(Epsilon))
								{
									PlaneNormal = Normalize(PlaneNormal);

									if(SameDirection(PlaneNormal, BodyA->WorldCentroid - EdgePointA))
									{
										PlaneNormal = -PlaneNormal;
									}
									// TODO: Do we need this check anymore?
									if((Inner(Support(EntityA, ColliderA, PlaneNormal) - EdgePointA, PlaneNormal) < Epsilon) &&
									   (Inner(Support(EntityB, ColliderB, -PlaneNormal) - EdgePointB, -PlaneNormal) < Epsilon))
									{
										v3 SupportB = Support(EntityB, ColliderB, -PlaneNormal);

										f32 Separation = Inner(SupportB - EdgePointA, PlaneNormal);
										if(Separation > BestSeparationEdges)
										{
											BestSeparatingAxisEdges = PlaneNormal;
											BestSeparationEdges = Separation;
											BestEdgeDirectionA = EdgeDirectionA;
											BestEdgePointA = EdgePointA;
											BestEdgeDirectionB = EdgeDirectionB;
											BestEdgePointB = EdgePointB;
											BestPlaneNormal = PlaneNormal;

											BestEdgeIndexA = EdgeIndexA;
											BestEdgeIndexB = EdgeIndexB;
										}
									}
								}
							}
						}
					}
				}

				if(BestSeparationEdges <= 0.0f)
				{
					// NOTE: Bias towards FaceContact
					f32 Bias = 0.0001f;
					b32 FaceContactA = BestSeparationFaceA >= (BestSeparationEdges - Bias);
					b32 FaceContactB = BestSeparationFaceB >= (BestSeparationEdges - Bias);
					if(FaceContactA || FaceContactB)
					{
						// NOTE: Face contact.
						if(BestSeparationFaceA > BestSeparationFaceB)
						{
							Manifold = CreateFaceContact(EntityA, ColliderA, EntityB, ColliderB, BestFaceA);
						}
						else
						{
							Manifold = CreateFaceContact(EntityB, ColliderB, EntityA, ColliderA, BestFaceB);
							FlipManifold(&Manifold);
						}
					}
					else
					{
						// NOTE: Edge contact.
						Manifold.ContactCount = 1;
						contact_data *Contact = Manifold.Contacts + 0;

						Contact->Normal = BestPlaneNormal;
						ContactBuildBasisFromNormal(Contact);

						Contact->Penetration = -BestSeparationEdges;

						// TODO: Clean this up, a lot of terms cancel out here.
						v3 EdgeNormalB = Cross(BestPlaneNormal, BestEdgeDirectionB);
						f32 DenomA = Inner(BestEdgePointA - BestEdgePointB, EdgeNormalB) - Inner(BestEdgePointA + BestEdgeDirectionA - BestEdgePointB, EdgeNormalB);
						Assert(DenomA != 0.0f);
						f32 tA = Inner(BestEdgePointA - BestEdgePointB, EdgeNormalB)/DenomA;
						Contact->WorldContactPointA = Lerp(BestEdgePointA, tA, BestEdgePointA + BestEdgeDirectionA);

						// TODO: Clean this up, a lot of terms cancel out here.
						v3 EdgeNormalA = Cross(BestPlaneNormal, BestEdgeDirectionA);
						f32 DenomB = Inner(BestEdgePointB - BestEdgePointA, EdgeNormalA) - Inner(BestEdgePointB + BestEdgeDirectionB - BestEdgePointA, EdgeNormalA);
						Assert(DenomB != 0.0f);
						f32 tB = Inner(BestEdgePointB - BestEdgePointA, EdgeNormalA)/DenomB;
						Contact->WorldContactPointB = Lerp(BestEdgePointB, tB, BestEdgePointB + BestEdgeDirectionB);

						Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
						Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

						Assert(BestEdgeIndexA % 2 == 0);
						Assert(BestEdgeIndexB % 2 == 0);
						Contact->FeatureA.Type = Feature_Edge;
						Contact->FeatureA.Index[0] = BestEdgeIndexA;
						Contact->FeatureA.Index[1] = BestEdgeIndexA + 1;
						Contact->FeatureB.Type = Feature_Edge;
						Contact->FeatureB.Index[0] = BestEdgeIndexB;
						Contact->FeatureB.Index[1] = BestEdgeIndexB + 1;

						Manifold.FeatureTypeA = Feature_Edge;
						Manifold.FeatureTypeB = Feature_Edge;
					}
				}
				else
				{
					Manifold.SeparatingAxis = BestSeparatingAxisEdges;
				}
			}
			else
			{
				Manifold.SeparatingAxis = BestSeparatingAxisFaceB;
			}
		}
		else
		{
			Manifold.SeparatingAxis = BestSeparatingAxisFaceA;
		}
	}
	else
	{
		Manifold.SeparatingAxis = CachedSeparatingAxis;
	}

	return Manifold;
}

internal contact_manifold
GenerateOneShotManifoldSphereCapsule(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	Assert(ColliderA->Shape.Type == Shape_Sphere);
	Assert(ColliderB->Shape.Type == Shape_Capsule);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	contact_manifold Manifold = {};

	v3 CenterA = LocalPointToWorldPoint(EntityA, ColliderA->Shape.Sphere.Center);
	f32 RadiusA = ColliderA->Shape.Sphere.Radius;

	v3 B0 = LocalPointToWorldPoint(EntityB, ColliderB->Shape.Capsule.P0);
	v3 B1 = LocalPointToWorldPoint(EntityB, ColliderB->Shape.Capsule.P1);
	f32 RadiusB = ColliderB->Shape.Capsule.Radius;

	v3 ClosestSegmentPointToCenter = SegmentClosestPoint(CenterA, B0, B1);
	v3 AtoB = ClosestSegmentPointToCenter - CenterA;
	f32 DistanceFromCenterToSegment = Length(AtoB);
	if(DistanceFromCenterToSegment == 0.0f)
	{
		Manifold.ContactCount = 1;
		contact_data *Contact = Manifold.Contacts + 0;

		Contact->WorldContactPointA = CenterA;
		Contact->WorldContactPointB = CenterA;

		Contact->LocalContactPointA = CenterA;
		Contact->LocalContactPointA = WorldPointToLocalPoint(EntityB, CenterA);

		Contact->Normal = V3(0,1,0);
		ContactBuildBasisFromNormal(Contact);

		Contact->Penetration = RadiusA + RadiusB;

		Contact->FeatureA.Type = Feature_Vertex;
		Contact->FeatureA.Index[0] = 0;
		Contact->FeatureA.Index[1] = 0;
		Contact->FeatureB.Type = Feature_Vertex;
		Contact->FeatureB.Index[0] = 0;
		Contact->FeatureB.Index[1] = 0;

		Manifold.FeatureTypeA = Feature_Vertex;
		Manifold.FeatureTypeB = Feature_Vertex;
	}
	else if(DistanceFromCenterToSegment < (RadiusA + RadiusB))
	{
		f32 Penetration = RadiusA + RadiusB - DistanceFromCenterToSegment;

		Manifold.ContactCount = 1;
		contact_data *Contact = Manifold.Contacts + 0;

		Contact->Normal = (1.0f/DistanceFromCenterToSegment)*AtoB;
		ContactBuildBasisFromNormal(Contact);

		Contact->WorldContactPointA = CenterA + RadiusA*Contact->Normal;
		Contact->WorldContactPointB = ClosestSegmentPointToCenter - RadiusB*Contact->Normal;

		Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
		Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

		Contact->Penetration = Penetration;

		Contact->FeatureA.Type = Feature_Vertex;
		Contact->FeatureA.Index[0] = 0;
		Contact->FeatureA.Index[1] = 0;
		Contact->FeatureB.Type = Feature_Vertex;
		Contact->FeatureB.Index[0] = 0;
		Contact->FeatureB.Index[1] = 0;

		Manifold.FeatureTypeA = Feature_Vertex;
		Manifold.FeatureTypeB = Feature_Vertex;
	}
	else
	{
		Manifold.SeparatingAxis = AtoB;
	}

	return Manifold;
}

internal contact_manifold
GenerateOneShotManifoldCapsuleCapsule(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	Assert(ColliderA->Shape.Type == Shape_Capsule);
	Assert(ColliderB->Shape.Type == Shape_Capsule);
	f32 Epsilon = 0.0001f;

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	contact_manifold Manifold = {};

	v3 A[] =
	{
		LocalPointToWorldPoint(EntityA, ColliderA->Shape.Capsule.P0),
		LocalPointToWorldPoint(EntityA, ColliderA->Shape.Capsule.P1),
	};
	f32 RadiusA = ColliderA->Shape.Capsule.Radius;

	v3 B[] =
	{
		LocalPointToWorldPoint(EntityB, ColliderB->Shape.Capsule.P0),
		LocalPointToWorldPoint(EntityB, ColliderB->Shape.Capsule.P1),
	};
	f32 RadiusB = ColliderB->Shape.Capsule.Radius;

	v3 PlaneNormal = Cross(A[1]-A[0], B[1]-B[0]);
	v3 ClosestPointA = {};
	v3 ClosestPointB = {};
	b32 Finished = false;
	if(LengthSq(PlaneNormal) < Square(Epsilon))
	{
		// NOTE: Segments are parallel
		if(!SameDirection(A[1]-A[0], B[1]-B[0]))
		{
			Swap(A[1], A[0], v3);
		}

		v3 EdgeB = NOZ(B[1] - B[0]);
		f32 DenomA = Inner(A[1] - A[0], EdgeB);
		Assert(DenomA != 0.0f);

		f32 tA0 = Inner(B[0] - A[0], EdgeB)/DenomA;
		f32 tA1 = Inner(B[1] - A[0], EdgeB)/DenomA;
		tA0 = Clamp01(tA0);
		tA1 = Clamp01(tA1);
		v3 ClipA[] = {Lerp(A[0], tA0, A[1]), Lerp(A[0], tA1, A[1])};

		v3 EdgeA = NOZ(A[1] - A[0]);
		f32 DenomB = Inner(B[1] - B[0], EdgeA);
		Assert(DenomB != 0.0f);

		f32 tB0 = Inner(A[0] - B[0], EdgeA)/DenomB;
		f32 tB1 = Inner(A[1] - B[0], EdgeA)/DenomB;
		tB0 = Clamp01(tB0);
		tB1 = Clamp01(tB1);
		v3 ClipB[] = {Lerp(B[0], tB0, B[1]), Lerp(B[0], tB1, B[1])};

		if(ClipA[0] != ClipA[1])
		{
			Manifold.ContactCount = 0;

			for(u32 Index = 0;
				Index < 2;
				++Index)
			{
				contact_data *Contact = Manifold.Contacts + Manifold.ContactCount;

				v3 AtoB = ClipB[Index] - ClipA[Index];
				f32 Distance = Length(AtoB);
				f32 Penetration = RadiusA + RadiusB - Distance;
				if(Penetration > 0.0f)
				{
					++Manifold.ContactCount;

					Contact->Normal = (1.0f/Distance)*AtoB;
					ContactBuildBasisFromNormal(Contact);

					Contact->WorldContactPointA = ClipA[Index] + RadiusA*Contact->Normal;
					Contact->WorldContactPointB = ClipB[Index] - RadiusB*Contact->Normal;

					Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
					Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

					Contact->Penetration = Penetration;

					Contact->FeatureA.Type = Feature_Edge;
					Contact->FeatureA.Index[0] = 0;
					Contact->FeatureA.Index[1] = 0;
					Contact->FeatureB.Type = Feature_Edge;
					Contact->FeatureB.Index[0] = 0;
					Contact->FeatureB.Index[1] = 0;

					Manifold.FeatureTypeA = Feature_Edge;
					Manifold.FeatureTypeB = Feature_Edge;
				}
			}

			if(!Manifold.ContactCount)
			{
				Manifold.SeparatingAxis = Cross(Cross(EdgeA, A[1]-A[0]), EdgeA);
			}

			Finished = true;
		}
		else
		{
			Assert(ClipB[0] == ClipB[1]);
			ClosestPointB = ClipB[0];
			ClosestPointA = ClipA[0];
		}
	}
	else
	{
		collider SegmentColliderA = {};
		SegmentColliderA.Shape.Type = Shape_Capsule;
		SegmentColliderA.Shape.Capsule.P0 = A[0];
		SegmentColliderA.Shape.Capsule.P1 = A[1];

		entity SegmentA = {};
		SegmentA.P = SegmentA.RigidBody.WorldCentroid = {};
		SegmentA.Rotation = Q(1,0,0,0);
		UpdateRotation(&SegmentA);

		collider SegmentColliderB = {};
		SegmentColliderB.Shape.Type = Shape_Capsule;
		SegmentColliderB.Shape.Capsule.P0 = B[0];
		SegmentColliderB.Shape.Capsule.P1 = B[1];

		entity SegmentB = {};
		SegmentB.P = SegmentB.RigidBody.WorldCentroid = {};
		SegmentB.Rotation = Q(1,0,0,0);
		UpdateRotation(&SegmentB);

		gjk_result GJKResult = GJK(&SegmentA, &SegmentColliderA, &SegmentB, &SegmentColliderB);
		ClosestPointA = GJKResult.ClosestPoint.SupportA;
		ClosestPointB = GJKResult.ClosestPoint.SupportB;
	}

	if(!Finished)
	{
		v3 AtoB = ClosestPointB - ClosestPointA;
		f32 Distance = Length(AtoB);
		if(Distance == 0.0f)
		{
			f32 Penetration = RadiusA + RadiusB;

			Manifold.ContactCount = 1;
			contact_data *Contact = Manifold.Contacts + 0;

			Contact->Normal = PlaneNormal;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = ClosestPointA;
			Contact->WorldContactPointB = ClosestPointB;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->Penetration = Penetration;

			Contact->FeatureA.Type = Feature_Vertex;
			Contact->FeatureA.Index[0] = 0;
			Contact->FeatureA.Index[1] = 0;
			Contact->FeatureB.Type = Feature_Vertex;
			Contact->FeatureB.Index[0] = 0;
			Contact->FeatureB.Index[1] = 0;

			Manifold.FeatureTypeA = Feature_Vertex;
			Manifold.FeatureTypeB = Feature_Vertex;
		}
		else if(Distance < (RadiusA + RadiusB))
		{
			f32 Penetration = RadiusA + RadiusB - Distance;

			Manifold.ContactCount = 1;
			contact_data *Contact = Manifold.Contacts + 0;

			Contact->Normal = (1.0f/Distance)*AtoB;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = ClosestPointA + RadiusA*Contact->Normal;
			Contact->WorldContactPointB = ClosestPointB - RadiusB*Contact->Normal;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->Penetration = Penetration;

			Contact->FeatureA.Type = Feature_Vertex;
			Contact->FeatureA.Index[0] = 0;
			Contact->FeatureA.Index[1] = 0;
			Contact->FeatureB.Type = Feature_Vertex;
			Contact->FeatureB.Index[0] = 0;
			Contact->FeatureB.Index[1] = 0;

			Manifold.FeatureTypeA = Feature_Vertex;
			Manifold.FeatureTypeB = Feature_Vertex;
		}
		else
		{
			Manifold.SeparatingAxis = AtoB;
		}
	}

	return Manifold;
}

internal contact_manifold
GenerateOneShotManifoldCapsuleHull(physics_state *State,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	// @Robustness: Sometimes we can make a 2-manifold for an edge contact.

	Assert(ColliderA->Shape.Type == Shape_Capsule);
	Assert(ColliderB->Shape.Type == Shape_ConvexHull);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	contact_manifold Manifold = {};

	f32 Epsilon = 0.0001f;
	f32 EpsilonSq = Square(Epsilon);

	v3 A0 = LocalPointToWorldPoint(EntityA, ColliderA->Shape.Capsule.P0);
	v3 A1 = LocalPointToWorldPoint(EntityA, ColliderA->Shape.Capsule.P1);
	f32 RadiusA = ColliderA->Shape.Capsule.Radius;

	convex_hull *HullB = ColliderB->Shape.ConvexHull;

	collider SegmentCollider = {};
	SegmentCollider.Shape.Type = Shape_Capsule;
	SegmentCollider.Shape.Capsule.P0 = A0;
	SegmentCollider.Shape.Capsule.P1 = A1;

	entity Segment = {};
	Segment.P = Segment.RigidBody.WorldCentroid = {};
	Segment.Rotation = Q(1,0,0,0);
	UpdateRotation(&Segment);

	gjk_result GJKResult = GJK(&Segment, &SegmentCollider, EntityB, ColliderB, 1.0f);
	if(GJKResult.ContainsOrigin)
	{
		// NOTE: Deep
		// @Speed: Find a way to reuse the previous separating axis if possible.
		f32 FaceSeparation = Real32Minimum;
		face *SeparatingFace = 0;
		v3 SeparatingAxisFace = {};
		for(u32 FaceIndex = 0;
			FaceIndex < HullB->FaceCount;
			++FaceIndex)
		{
			face *Face = HullB->Faces + FaceIndex;
			v3 LocalFaceVertex = HullB->Vertices[HullB->Edges[Face->FirstEdge].Origin];
			v3 FaceVertex = LocalPointToWorldPoint(EntityB, LocalFaceVertex);
			v3 FaceNormal = LocalVectorToWorldVector(EntityB, Face->Normal);
			f32 Separation0 = Inner(A0 - FaceVertex, FaceNormal);
			f32 Separation1 = Inner(A1 - FaceVertex, FaceNormal);
			f32 Separation = Minimum(Separation0, Separation1);
			if(Separation > FaceSeparation)
			{
				FaceSeparation = Separation;
				SeparatingFace = Face;
				SeparatingAxisFace = -FaceNormal;
			}
		}

		Assert(FaceSeparation < 0.0f);

		f32 EdgeSeparation = Real32Minimum;
		half_edge *SeparatingEdge = 0;
		v3 SeparatingAxisEdge = {};
		for(u32 EdgeIndex = 0;
			EdgeIndex < HullB->EdgeCount;
			EdgeIndex = EdgeIndex + 2)
		{
			half_edge *Edge = HullB->Edges + EdgeIndex;
			half_edge *Twin = HullB->Edges + EdgeIndex + 1;
			v3 Tail = LocalPointToWorldPoint(EntityB, HullB->Vertices[Edge->Origin]);
			v3 Head = LocalPointToWorldPoint(EntityB, HullB->Vertices[Twin->Origin]);
			v3 EdgeDirection = (Head - Tail);
			v3 PlaneNormal = NOZ(Cross(EdgeDirection, A1-A0));
			if(PlaneNormal != V3(0,0,0))
			{
				if(Inner(PlaneNormal, Head - BodyB->WorldCentroid) < 0.0f)
				{
					PlaneNormal = -PlaneNormal;
				}

				f32 SeparationA = Inner(A0 - Head, PlaneNormal);
				f32 SeparationB = Inner(Support(EntityB, ColliderB, PlaneNormal) - Head, PlaneNormal);
				f32 Separation = SeparationA - SeparationB;
				if(Separation > EdgeSeparation)
				{
					EdgeSeparation = Separation;
					SeparatingEdge = Edge;
					SeparatingAxisEdge = -PlaneNormal;
				}
			}
		}

		Assert(EdgeSeparation < 0.0f);

		// NOTE: Bias towards FaceContact
		f32 Bias = 0.0001f;
		if(FaceSeparation >= (EdgeSeparation - Bias))
		{
			// NOTE: Face contact
			face *ReferenceFace = SeparatingFace;

			u32 VertexCount = 2;
			manifold_point Clip[2] = {};
			Clip[0].P = A0;
			Clip[1].P = A1;

			VertexCount = ClipAgainstFace(Clip, VertexCount, EntityB, HullB,ReferenceFace);

			for(u32 Index = 0;
				Index < VertexCount;
				++Index)
			{
				contact_data *Contact = Manifold.Contacts + Manifold.ContactCount++;
				v3 SegmentPoint = Clip[Index].P;
				f32 DistanceToFace = Clip[Index].Penetration - RadiusA;
				Assert(Clip[Index].Penetration > 0.0f);

				Contact->Normal = SeparatingAxisFace;
				ContactBuildBasisFromNormal(Contact);

				Contact->WorldContactPointA = SegmentPoint + RadiusA*Contact->Normal;
				Contact->WorldContactPointB = SegmentPoint + DistanceToFace*Contact->Normal;

				Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
				Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

				Contact->Penetration = Clip[Index].Penetration;

				Contact->FeatureA.Type = Feature_Vertex;
				Contact->FeatureA.Index[0] = Clip[Index].EdgeIndexes[0];
				Contact->FeatureA.Index[1] = Clip[Index].EdgeIndexes[1];

				Contact->FeatureB.Type = Feature_Face;
				Contact->FeatureB.Index[0] = HullB->Edges[ReferenceFace->FirstEdge].Face;
				Contact->FeatureB.Index[1] = 0;

				Manifold.FeatureTypeA = Feature_Vertex;
				Manifold.FeatureTypeB = Feature_Face;
			}
		}
		else
		{
			// NOTE: Edge contact
			collider SegmentColliderA = {};
			SegmentColliderA.Shape.Type = Shape_Capsule;
			SegmentColliderA.Shape.Capsule.P0 = A0;
			SegmentColliderA.Shape.Capsule.P1 = A1;

			entity SegmentA = {};
			SegmentA.P = SegmentA.RigidBody.WorldCentroid = {};
			SegmentA.Rotation = Q(1,0,0,0);
			UpdateRotation(&SegmentA);

			half_edge *Twin = HullB->Edges + SeparatingEdge->Twin;
			v3 B0 = LocalPointToWorldPoint(EntityB, HullB->Vertices[SeparatingEdge->Origin]);
			v3 B1 = LocalPointToWorldPoint(EntityB, HullB->Vertices[Twin->Origin]);
			collider SegmentColliderB = {};
			SegmentColliderB.Shape.Type = Shape_Capsule;
			SegmentColliderB.Shape.Capsule.P0 = B0;
			SegmentColliderB.Shape.Capsule.P1 = B1;

			entity SegmentB = {};
			SegmentB.P = SegmentB.RigidBody.WorldCentroid = {};
			SegmentB.Rotation = Q(1,0,0,0);
			UpdateRotation(&SegmentB);

			gjk_result GJKEdgeResult = GJK(&SegmentA, &SegmentColliderA, &SegmentB, &SegmentColliderB);
			v3 ClosestPointA = GJKEdgeResult.ClosestPoint.SupportA;
			v3 ClosestPointB = GJKEdgeResult.ClosestPoint.SupportB;

			v3 AtoB = ClosestPointB - ClosestPointA;
			f32 Distance = Length(AtoB);
			f32 Penetration = -EdgeSeparation;

			Manifold.ContactCount = 1;
			contact_data *Contact = Manifold.Contacts + 0;

			Contact->Normal = SeparatingAxisEdge;
			ContactBuildBasisFromNormal(Contact);

			Contact->WorldContactPointA = ClosestPointA + RadiusA*Contact->Normal;
			Contact->WorldContactPointB = ClosestPointB;

			Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
			Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

			Contact->Penetration = Penetration;

			Contact->FeatureA.Type = Feature_Vertex;
			Contact->FeatureA.Index[0] = 0;
			Contact->FeatureA.Index[1] = 0;
			Contact->FeatureB.Type = Feature_Edge;
			Contact->FeatureB.Index[0] = Twin->Twin;
			Contact->FeatureB.Index[1] = SeparatingEdge->Twin;

			Manifold.FeatureTypeA = Feature_Vertex;
			Manifold.FeatureTypeB = Feature_Edge;
		}
	}
	else
	{
		TIMED_BLOCK("CapsuleHull Shallow");

		// NOTE: Shallow
		minkowski_difference_support ClosestPointToOrigin = GJKResult.ClosestPoint;

		v3 CenterA = ClosestPointToOrigin.SupportA;
		v3 AtoB = ClosestPointToOrigin.SupportB - CenterA;
		f32 Distance = Length(AtoB);
		v3 ContactNormal = (1.0f/Distance)*AtoB;
		f32 Penetration = RadiusA - Distance;

		if(Penetration > 0.0f)
		{
			u32 ReferenceFaceIndex = 0;
			face *ReferenceFace = 0;
			for(u32 Index = 0;
				Index < HullB->FaceCount;
				++Index)
			{
				face *Face = HullB->Faces + Index;
				// NOTE: ContactNormal points from capsule to hull, so we want a face with
				// opposite normal.
				v3 WorldFaceNormal = LocalVectorToWorldVector(EntityB, Face->Normal);
				if(LengthSq(WorldFaceNormal + ContactNormal) < EpsilonSq)
				{
					ReferenceFace = Face;
					ReferenceFaceIndex = Index;
					break;
				}
			}

			u32 VertexCount = 1;
			manifold_point Clip[2] = {};
			Clip[0].P = CenterA;
			if(ReferenceFace)
			{
				VertexCount = 2;
				Clip[0].P = A0;
				Clip[1].P = A1;
				VertexCount = ClipAgainstFace(Clip, VertexCount, EntityB, HullB, ReferenceFace);
			}

			// @Robustness: I hate this and it seems dumb and slow. Find a way to get this information
			// without iterating through the convex_hull. Maybe pass more info from GJK.
			if(VertexCount == 1)
			{
				Manifold.ContactCount = 1;
				contact_data *Contact = Manifold.Contacts + 0;

				u32 FeaturePointCount = GJKResult.Simplex.Size;
				u32 FeaturePointsIndexes[3] = {};
				for(u32 Index = 0;
					Index < FeaturePointCount;
					++Index)
				{
					v3 FeaturePoint = GJKResult.Simplex.Points[Index].SupportB;

					for(u32 VertexIndex = 0;
						VertexIndex < HullB->VertexCount;
						++VertexIndex)
					{
						v3 HullVertex = HullB->Vertices[VertexIndex];
						if(HullVertex == FeaturePoint)
						{
							FeaturePointsIndexes[Index] = VertexIndex;
							break;
						}
					}
				}

				u32 FeatureIndexes[2] = {};

				switch(FeaturePointCount)
				{
					case 1:
					{
						FeatureIndexes[0] = FeaturePointsIndexes[0];
						Contact->FeatureB.Type = Feature_Vertex;
						Contact->FeatureB.Index[0] = FeatureIndexes[0];
						Contact->FeatureB.Index[1] = FeatureIndexes[1];
					} break;

					case 2:
					{
						for(u32 EdgeIndex = 0;
							EdgeIndex < HullB->EdgeCount;
							EdgeIndex += 2)
						{
							half_edge *Edge = HullB->Edges + EdgeIndex;
							half_edge *Twin = HullB->Edges + EdgeIndex + 1;
							Assert(HullB->Edges + Edge->Twin == Twin);

							if(((Edge->Origin == FeaturePointsIndexes[0]) && (Twin->Origin == FeaturePointsIndexes[1])) ||
							   ((Edge->Origin == FeaturePointsIndexes[1]) && (Twin->Origin == FeaturePointsIndexes[0])))
							{
								FeatureIndexes[0] = EdgeIndex;
								FeatureIndexes[1] = EdgeIndex + 1;
								break;
							}
						}

						Contact->FeatureB.Type = Feature_Edge;
						Contact->FeatureB.Index[0] = FeatureIndexes[0];
						Contact->FeatureB.Index[1] = FeatureIndexes[1];
					} break;

					case 3:
					{
						for(u32 FaceIndex = 0;
							FaceIndex < HullB->FaceCount;
							++FaceIndex)
						{
							face *Face = HullB->Faces + FaceIndex;
							u32 VerticesFound = 0;
							half_edge *Edge = HullB->Edges + Face->FirstEdge;
							do
							{
								if(Edge->Origin == FeaturePointsIndexes[0])
								{
									++VerticesFound;
								}
								if(Edge->Origin == FeaturePointsIndexes[1])
								{
									++VerticesFound;
								}
								if(Edge->Origin == FeaturePointsIndexes[2])
								{
									++VerticesFound;
								}

								Edge = HullB->Edges + Edge->Next;
							} while(Edge != HullB->Edges + Face->FirstEdge);

							if(VerticesFound == FeaturePointCount)
							{
								FeatureIndexes[0] = FaceIndex;
							}
						}

						Contact->FeatureB.Type = Feature_Face;
						Contact->FeatureB.Index[0] = FeatureIndexes[0];
						Contact->FeatureB.Index[1] = FeatureIndexes[1];
					} break;

					InvalidDefaultCase;
				}

				Contact->Normal = ContactNormal;
				ContactBuildBasisFromNormal(Contact);

				Contact->WorldContactPointA = ClosestPointToOrigin.SupportA + RadiusA*Contact->Normal;
				Contact->WorldContactPointB = ClosestPointToOrigin.SupportB;

				Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
				Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

				Contact->Penetration = Penetration;

				Contact->FeatureA.Type = Feature_Vertex;
				Contact->FeatureA.Index[0] = 0;
				Contact->FeatureA.Index[1] = 0;

				Manifold.FeatureTypeA = Feature_Vertex;
				Manifold.FeatureTypeB = Contact->FeatureB.Type;
			}
			else
			{
				for(u32 Index = 0;
					Index < 2;
					++Index)
				{
					contact_data *Contact = Manifold.Contacts + Manifold.ContactCount;
					v3 SegmentPoint = Clip[Index].P;
					v3 PlanePoint = LocalPointToWorldPoint(EntityB, ReferenceFace->Center);
					v3 PlaneNormal = LocalVectorToWorldVector(EntityB, ReferenceFace->Normal);
					f32 DistanceToFace = Inner(SegmentPoint - PlanePoint, PlaneNormal);
					Clip[Index].Penetration = RadiusA - DistanceToFace;
					if(Clip[Index].Penetration > 0.0f)
					{
						++Manifold.ContactCount;

						Contact->Normal = -PlaneNormal;
						ContactBuildBasisFromNormal(Contact);

						Contact->WorldContactPointA = SegmentPoint + RadiusA*Contact->Normal;
						Contact->WorldContactPointB = SegmentPoint + DistanceToFace*Contact->Normal;

						Contact->LocalContactPointA = WorldPointToLocalPoint(EntityA, Contact->WorldContactPointA);
						Contact->LocalContactPointB = WorldPointToLocalPoint(EntityB, Contact->WorldContactPointB);

						Contact->Penetration = Clip[Index].Penetration;

						Contact->FeatureA.Type = Feature_Vertex;
						Contact->FeatureA.Index[0] = Clip[Index].EdgeIndexes[0];
						Contact->FeatureA.Index[1] = Clip[Index].EdgeIndexes[1];

						Contact->FeatureB.Type = Feature_Face;
						Contact->FeatureB.Index[0] = ReferenceFaceIndex;
						Contact->FeatureB.Index[1] = 0;

						Manifold.FeatureTypeA = Feature_Vertex;
						Manifold.FeatureTypeB = Feature_Face;
					}
				}
			}
		}
		else
		{
			Manifold.SeparatingAxis = AtoB;
		}
	}

	return Manifold;
}

internal b32
ContactDataIsDifferent(physics_state *State, contact_data *ContactA, contact_data *ContactB)
{
	b32 Result =
		!((ContactA->FeatureA.Type == ContactB->FeatureA.Type) &&
		(ContactA->FeatureA.Index[0] == ContactB->FeatureA.Index[0]) &&
		(ContactA->FeatureA.Index[1] == ContactB->FeatureA.Index[1]) &&
		(ContactA->FeatureB.Type == ContactB->FeatureB.Type) &&
		(ContactA->FeatureB.Index[0] == ContactB->FeatureB.Index[0]) &&
		(ContactA->FeatureB.Index[1] == ContactB->FeatureB.Index[1]));

	return Result;
}

inline v3
CalculateRelativeVelocityForContact(entity *EntityA, entity *EntityB, contact_data *Contact)
{
	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	v3 WorldRadiusA = Contact->WorldContactPointA - BodyA->WorldCentroid;
	v3 WorldRadiusB = Contact->WorldContactPointB - BodyB->WorldCentroid;

	v3 Result = (EntityB->dP - EntityA->dP) +
			Cross(EntityB->AngularVelocity, WorldRadiusB) -
			Cross(EntityA->AngularVelocity, WorldRadiusA);
	return Result;
}

internal void
GenerateOneShotManifold(physics_state *State, contact_manifold *Manifold,
	entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB)
{
	contact_manifold OneShotManifold = {};

	switch(ColliderA->Shape.Type)
	{
		case Shape_Sphere:
		{
			switch(ColliderB->Shape.Type)
			{
				case Shape_Sphere:
				{
					OneShotManifold = GenerateOneShotManifoldSphereSphere(State, EntityA, ColliderA, EntityB, ColliderB);
				} break;

				case Shape_Capsule:
				{
					OneShotManifold = GenerateOneShotManifoldSphereCapsule(State, EntityA, ColliderA, EntityB, ColliderB);
				} break;

				case Shape_ConvexHull:
				{
					OneShotManifold = GenerateOneShotManifoldSphereHull(State, EntityA, ColliderA, EntityB, ColliderB);
				} break;

				InvalidDefaultCase;
			}
		} break;

		case Shape_Capsule:
		{
			switch(ColliderB->Shape.Type)
			{
				case Shape_Sphere:
				{
					OneShotManifold = GenerateOneShotManifoldSphereCapsule(State, EntityB, ColliderB, EntityA, ColliderA);
					FlipManifold(&OneShotManifold);
				} break;

				case Shape_Capsule:
				{
					OneShotManifold = GenerateOneShotManifoldCapsuleCapsule(State, EntityA, ColliderA, EntityB, ColliderB);
				} break;

				case Shape_ConvexHull:
				{
					OneShotManifold = GenerateOneShotManifoldCapsuleHull(State, EntityA, ColliderA, EntityB, ColliderB);
				} break;
			}
		} break;

		case Shape_ConvexHull:
		{
			switch(ColliderB->Shape.Type)
			{
				case Shape_Sphere:
				{
					OneShotManifold = GenerateOneShotManifoldSphereHull(State, EntityB, ColliderB, EntityA, ColliderA);
					FlipManifold(&OneShotManifold);
				} break;

				case Shape_Capsule:
				{
					OneShotManifold = GenerateOneShotManifoldCapsuleHull(State, EntityB, ColliderB, EntityA, ColliderA);
					FlipManifold(&OneShotManifold);
				} break;

				case Shape_ConvexHull:
				{
					OneShotManifold = GenerateOneShotManifoldHullHull(State, EntityA, ColliderA, EntityB, ColliderB, Manifold->SeparatingAxis);
				} break;

				InvalidDefaultCase;
			}
		} break;

		InvalidDefaultCase;
	}

	if(OneShotManifold.ContactCount == 0)
	{
		Manifold->SeparatingAxis = OneShotManifold.SeparatingAxis;
		Manifold->ContactCount = 0;
	}
	else
	{
		for(u32 NewContactIndex = 0;
			NewContactIndex < OneShotManifold.ContactCount;
			++NewContactIndex)
		{
			contact_data *NewContact = OneShotManifold.Contacts + NewContactIndex;

			for(u32 OldContactIndex = 0;
				OldContactIndex < Manifold->ContactCount;
				++OldContactIndex)
			{
				contact_data *OldContact = Manifold->Contacts + OldContactIndex;
				if(!ContactDataIsDifferent(State, NewContact, OldContact))
				{
					NewContact->NormalImpulseSum = OldContact->NormalImpulseSum;
					NewContact->Tangent0ImpulseSum = OldContact->Tangent0ImpulseSum;
					NewContact->Tangent1ImpulseSum = OldContact->Tangent1ImpulseSum;

					NewContact->Persistent = true;
					NewContact->PersistentCount = OldContact->PersistentCount + 1;
				}
			}
		}

		Manifold->ContactCount = OneShotManifold.ContactCount;
		Manifold->FeatureTypeA = OneShotManifold.FeatureTypeA;
		Manifold->FeatureTypeB = OneShotManifold.FeatureTypeB;
		for(u32 Index = 0;
			Index < OneShotManifold.ContactCount;
			++Index)
		{
			contact_data *Contact = Manifold->Contacts + Index;
			*Contact = OneShotManifold.Contacts[Index];
			Contact->InitialRelativeVelocity = CalculateRelativeVelocityForContact(EntityA, EntityB, Contact);
		}
	}
}

inline u32
HashCollision(collision Collision)
{
	// TODO: Better hash function.
	u32 A = U32FromPointer(Collision.EntityA);
	u32 B = U32FromPointer(Collision.EntityB);
	u32 C = U32FromPointer(Collision.ColliderA);
	u32 D = U32FromPointer(Collision.ColliderB);
	u32 Result = (13*A + 17*B + 83*C + 19*D + 237) % ArrayCount(((physics_state*)0)->ContactManifoldHashTable);
	return Result;
}

internal contact_manifold *
PhysicsGetOrCreateContactManifold(physics_state *State, collision Collision)
{
	u32 HashValue = HashCollision(Collision);
	contact_manifold *Result = State->ContactManifoldHashTable[HashValue];
	if(Result)
	{
		while(Result && (Result->Collision != Collision))
		{
			Result = Result->NextInHash;
		}
	}

	if(!Result)
	{
		Result = State->ManifoldFreeList;
		if(Result)
		{
			State->ManifoldFreeList = Result->NextInHash;
			ZeroStruct(*Result);
		}
		else
		{
			Result = PushStruct(State->Region, contact_manifold);
		}

		Result->Collision = Collision;
		Result->NextInHash = State->ContactManifoldHashTable[HashValue];
		State->ContactManifoldHashTable[HashValue] = Result;
		DLIST_INSERT(&State->ManifoldSentinel, Result);
	}

	Result->FramesSinceRead = 0;

	return Result;
}

internal void
RemoveManifold(physics_state *State, contact_manifold *Manifold)
{
	u32 HashValue = HashCollision(Manifold->Collision);
	contact_manifold *ManifoldToRemove = State->ContactManifoldHashTable[HashValue];
	contact_manifold *PreviousInHash = 0;
	if(ManifoldToRemove)
	{
		while(ManifoldToRemove && (ManifoldToRemove->Collision != Manifold->Collision))
		{
			PreviousInHash = ManifoldToRemove;
			ManifoldToRemove = ManifoldToRemove->NextInHash;
		}
	}

	Assert(ManifoldToRemove);
	Assert(ManifoldToRemove == Manifold);
	if(PreviousInHash)
	{
		PreviousInHash->NextInHash = ManifoldToRemove->NextInHash;
	}
	else
	{
		State->ContactManifoldHashTable[HashValue] = ManifoldToRemove->NextInHash;
	}

	DLIST_REMOVE(ManifoldToRemove);
	ManifoldToRemove->NextInHash = State->ManifoldFreeList;
	State->ManifoldFreeList = ManifoldToRemove;
}

internal contact_manifold *
ManifoldCheckContacts(physics_state *State, contact_manifold *Manifold)
{
	contact_manifold *NextManifold = Manifold->Next;

	f32 PersistentThreshold = State->PersistentThreshold;
	f32 PersistentThresholdSq = Square(PersistentThreshold);

	entity *EntityA = Manifold->Collision.EntityA;
	entity *EntityB = Manifold->Collision.EntityB;

	if(Manifold->ContactCount)
	{
		for(u32 Index = 0;
			Index < Manifold->ContactCount;
			++Index)
		{
			contact_data *Contact = Manifold->Contacts + Index;

			v3 LocalToWorldA = LocalPointToWorldPoint(EntityA, Contact->LocalContactPointA);
			v3 LocalToWorldB = LocalPointToWorldPoint(EntityB, Contact->LocalContactPointB);

			v3 WorldChangedContactDistance = LocalToWorldB - LocalToWorldA;
			v3 LocalChangeA = Contact->WorldContactPointA - LocalToWorldA;
			v3 LocalChangeB = Contact->WorldContactPointB - LocalToWorldB;

			b32 StillPenetrating = Inner(WorldChangedContactDistance, Contact->Normal) < PersistentThreshold;
			b32 CloseEnoughA = (LengthSq(LocalChangeA) < PersistentThresholdSq);
			b32 CloseEnoughB = (LengthSq(LocalChangeB) < PersistentThresholdSq);

			if(StillPenetrating && CloseEnoughA && CloseEnoughB)
			{
				Contact->Penetration = -Inner(WorldChangedContactDistance, Contact->Normal);
				Contact->Persistent = true;
			}
			else
			{
				// NOTE: Remove Contact.
				switch(Manifold->ContactCount)
				{
					case 1:
					{
						RemoveManifold(State, Manifold);
					} break;

					case 2:
					case 3:
					case 4:
					{
						Manifold->Contacts[Index] = Manifold->Contacts[--Manifold->ContactCount];
						// NOTE: Make the for loop go to the newly copied ContactData.
						// TODO: Is there a reason to care about the order of contact_data points?
						--Index;
					} break;

					InvalidDefaultCase;
				}
			}
		}
	}
	else
	{
		++Manifold->FramesSinceRead;
		u32 FrameLifetime = 2;
		if(Manifold->FramesSinceRead > FrameLifetime)
		{
			RemoveManifold(State, Manifold);
		}
	}

	return NextManifold;
}

internal void
SolveControllerJointConstraints(physics_state *State, entity *Entity, controller_joint *Controller)
{
	v3 BodydP = Entity->dP;
	v3 LinearMotor = Controller->LinearMotor;
	if(LengthSq(Controller->IgnoredAxis))
	{
		BodydP -= Inner(BodydP, Controller->IgnoredAxis)*Controller->IgnoredAxis;
		LinearMotor -= Inner(LinearMotor, Controller->IgnoredAxis)*Controller->IgnoredAxis;
	}

	v3 MotorChange = BodydP - LinearMotor;
	f32 EffectiveMass = Entity->RigidBody.InvMass;

	v3 Lagrangian = -(1.0f/EffectiveMass)*MotorChange;
	v3 PrevLagrangian = Controller->ImpulseSum;
	Controller->ImpulseSum += Lagrangian;
	if(LengthSq(Controller->ImpulseSum) > Square(Controller->MaxImpulse))
	{
		Controller->ImpulseSum = (Controller->MaxImpulse/Length(Controller->ImpulseSum))*Controller->ImpulseSum;
	}
	Lagrangian = Controller->ImpulseSum - PrevLagrangian;

	ApplyImpulse(Entity, Lagrangian, Entity->RigidBody.WorldCentroid);
}

internal void
SolveDistanceJointConstraints(physics_state *State, entity *EntityA, entity *EntityB, distance_joint *Joint, f32 dt)
{
	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	v3 WorldAnchorA = LocalPointToWorldPoint(EntityA, Joint->LocalAnchorA);
	v3 WorldAnchorB = LocalPointToWorldPoint(EntityB, Joint->LocalAnchorB);

	v3 AnchorSeparation = WorldAnchorB - WorldAnchorA;

	r32 AnchorSeparationLength = Length(AnchorSeparation);

	v3 WorldRadiusA = WorldAnchorA - BodyA->WorldCentroid;
	v3 WorldRadiusB = WorldAnchorB - BodyB->WorldCentroid;

	v3 RelativeVelocity = (EntityB->dP - EntityA->dP) +
		Cross(EntityB->AngularVelocity, WorldRadiusB) -
		Cross(EntityA->AngularVelocity, WorldRadiusA);

	v3 AnchorSeparationNormal = V3(0,0,0);
	if(AnchorSeparationLength != 0.0f)
	{
		AnchorSeparationNormal = (1.0f/AnchorSeparationLength)*AnchorSeparation;
	}

	v3 RadiusCrossNormalA = Cross(WorldRadiusA, AnchorSeparationNormal);
	v3 RadiusCrossNormalB = Cross(WorldRadiusB, AnchorSeparationNormal);

	f32 EffectiveMass = BodyA->InvMass + BodyB->InvMass +
		Inner(RadiusCrossNormalA, BodyA->LocalInvInertialTensor*RadiusCrossNormalA) +
		Inner(RadiusCrossNormalB, BodyB->LocalInvInertialTensor*RadiusCrossNormalB);

	f32 RelativeAnchorVelocity = Inner(RelativeVelocity, AnchorSeparationNormal);
	f32 PositionError = (AnchorSeparationLength - Joint->Distance);

	f32 Omega = 2.0f*Pi32*Joint->AngularFrequency;
	f32 SpringCoeff = EffectiveMass*Square(Omega);
	f32 DampingCoeff = 2.0f*EffectiveMass*Joint->DampingRatio*Omega;
	f32 Denominator = (DampingCoeff + dt*SpringCoeff);
	f32 Softness = 0.0f;
	f32 BiasFactor = State->Baumgarte;
	if(Denominator != 0.0f)
	{
		Softness = 1.0f/(dt*Denominator);
		BiasFactor = (dt*SpringCoeff)/Denominator;
	}

	f32 Bias = (BiasFactor/dt)*PositionError;

	f32 PrevLagrangianMultiplier = Joint->ImpulseSum;
	f32 LagrangianMultiplier = -(RelativeAnchorVelocity + Softness*PrevLagrangianMultiplier + Bias)/
		(EffectiveMass + Softness);

	Joint->ImpulseSum += LagrangianMultiplier;

	ApplyImpulse(EntityA, -LagrangianMultiplier*AnchorSeparationNormal, WorldAnchorA);
	ApplyImpulse(EntityB,  LagrangianMultiplier*AnchorSeparationNormal, WorldAnchorB);
}

internal void
SolveContactConstraints(physics_state *State, entity *EntityA, entity *EntityB, contact_data *Contact, f32 dt)
{
	if(Contact->Penetration > 0.0f)
	{
		rigid_body *BodyA = &EntityA->RigidBody;
		rigid_body *BodyB = &EntityB->RigidBody;
#if 0
		v3 WorldRadiusA = LocalVectorToWorldVector(BodyA, Contact->LocalContactPointA - BodyA->LocalCentroid);
		v3 WorldRadiusB = LocalVectorToWorldVector(BodyB, Contact->LocalContactPointB - BodyB->LocalCentroid);

		v3 WorldContactPointA = LocalPointToWorldPoint(BodyA, Contact->LocalContactPointA);
		v3 WorldContactPointB = LocalPointToWorldPoint(BodyB, Contact->LocalContactPointB);

#else
		v3 WorldRadiusA = Contact->WorldContactPointA - BodyA->WorldCentroid;
		v3 WorldRadiusB = Contact->WorldContactPointB - BodyB->WorldCentroid;

		v3 WorldContactPointA = Contact->WorldContactPointA;
		v3 WorldContactPointB = Contact->WorldContactPointB;

#endif
		v3 RadiusCrossNormalA = Cross(WorldRadiusA, Contact->Normal);
		v3 RadiusCrossNormalB = Cross(WorldRadiusB, Contact->Normal);

		v3 RelativeVelocity = CalculateRelativeVelocityForContact(EntityA, EntityB, Contact);

		f32 BaumgarteSlop = State->BaumgarteSlop;
		f32 BaumgarteTerm = -State->Baumgarte/dt;
		f32 RestitutionCoeff = State->RestitutionCoeff;
		f32 RestitutionSlop = State->RestitutionSlop;

		f32 BaumgarteBias = BaumgarteTerm*Maximum(Contact->Penetration - BaumgarteSlop, 0.0f);
		f32 InitialRelativeVelocityNormal = Inner(Contact->InitialRelativeVelocity, Contact->Normal);
		f32 RestitutionBias = -RestitutionCoeff*Maximum(-InitialRelativeVelocityNormal - RestitutionSlop, 0.0f);
		f32 ContactBias = BaumgarteBias + RestitutionBias;

		f32 RelativeVelocityNormal = Inner(RelativeVelocity, Contact->Normal);
		f32 ContactConstraint = RelativeVelocityNormal + ContactBias;
		f32 InvEffectiveMassNormal = 1.0f/(BodyA->InvMass + BodyB->InvMass +
			Inner(RadiusCrossNormalA, BodyA->LocalInvInertialTensor*RadiusCrossNormalA) +
			Inner(RadiusCrossNormalB, BodyB->LocalInvInertialTensor*RadiusCrossNormalB));

		f32 NormalLagrangianMultiplier = -InvEffectiveMassNormal*ContactConstraint;
		f32 PrevNormalImpulse = Contact->NormalImpulseSum;
		Contact->NormalImpulseSum = Maximum(Contact->NormalImpulseSum + NormalLagrangianMultiplier, 0.0f);
		NormalLagrangianMultiplier = (Contact->NormalImpulseSum - PrevNormalImpulse);

		ApplyImpulse(EntityA, -NormalLagrangianMultiplier*Contact->Normal, WorldContactPointA);
		ApplyImpulse(EntityB,  NormalLagrangianMultiplier*Contact->Normal, WorldContactPointB);


		v3 RadiusCrossTangent0A = Cross(WorldRadiusA, Contact->Tangent0);
		v3 RadiusCrossTangent0B = Cross(WorldRadiusB, Contact->Tangent0);
		v3 RadiusCrossTangent1A = Cross(WorldRadiusA, Contact->Tangent1);
		v3 RadiusCrossTangent1B = Cross(WorldRadiusB, Contact->Tangent1);

		f32 RelativeVelocityTangent0 = Inner(RelativeVelocity, Contact->Tangent0);
		f32 RelativeVelocityTangent1 = Inner(RelativeVelocity, Contact->Tangent1);

		f32 StaticFrictionToleranceSq = Square(0.1f);
		// TODO: What is the right way to do this?
		// we want to be physically accurate (which might mean using minimum),
		// but we also want to be able to control entity movement without having
		// to worry about what surface they are standing on.
		f32 StaticFrictionCoeff = 0.5f*(BodyA->MoveSpec.StaticFriction + BodyB->MoveSpec.StaticFriction);
		f32 KineticFrictionCoeff = 0.5f*(BodyA->MoveSpec.KineticFriction + BodyB->MoveSpec.KineticFriction);
		f32 FrictionCoeff = ((Square(RelativeVelocityTangent0) + Square(RelativeVelocityTangent1)) < StaticFrictionToleranceSq) ? StaticFrictionCoeff : KineticFrictionCoeff;
		f32 FrictionBias = 0.0f;

		f32 FrictionConstraint0 = RelativeVelocityTangent0 + FrictionBias;
		f32 FrictionConstraint1 = RelativeVelocityTangent1 + FrictionBias;
		f32 InvEffectiveMassTangent0 = 1.0f/(BodyA->InvMass + BodyB->InvMass +
			Inner(RadiusCrossTangent0A, BodyA->LocalInvInertialTensor*RadiusCrossTangent0A) +
			Inner(RadiusCrossTangent0B, BodyB->LocalInvInertialTensor*RadiusCrossTangent0B));
		f32 InvEffectiveMassTangent1 = 1.0f/(BodyA->InvMass + BodyB->InvMass +
			Inner(RadiusCrossTangent1A, BodyA->LocalInvInertialTensor*RadiusCrossTangent1A) +
			Inner(RadiusCrossTangent1B, BodyB->LocalInvInertialTensor*RadiusCrossTangent1B));

		f32 Tangent0LagrangianMultiplier = -InvEffectiveMassTangent0*FrictionConstraint0;
		f32 Tangent1LagrangianMultiplier = -InvEffectiveMassTangent1*FrictionConstraint1;
		f32 MaximumFrictionImpulse = FrictionCoeff*Contact->NormalImpulseSum;
		f32 PrevTangent0Impulse = Contact->Tangent0ImpulseSum;
		f32 PrevTangent1Impulse = Contact->Tangent1ImpulseSum;
		Contact->Tangent0ImpulseSum = Clamp(Contact->Tangent0ImpulseSum + Tangent0LagrangianMultiplier,
			-MaximumFrictionImpulse, MaximumFrictionImpulse);
		Contact->Tangent1ImpulseSum = Clamp(Contact->Tangent1ImpulseSum + Tangent1LagrangianMultiplier,
			-MaximumFrictionImpulse, MaximumFrictionImpulse);
		Tangent0LagrangianMultiplier = (Contact->Tangent0ImpulseSum - PrevTangent0Impulse);
		Tangent1LagrangianMultiplier = (Contact->Tangent1ImpulseSum - PrevTangent1Impulse);

		v3 TotalTangentImpulse = Tangent0LagrangianMultiplier*Contact->Tangent0 + Tangent1LagrangianMultiplier*Contact->Tangent1;
		ApplyImpulse(EntityA, -TotalTangentImpulse, WorldContactPointA);
		ApplyImpulse(EntityB,  TotalTangentImpulse, WorldContactPointB);
	}
}

internal collider *
AddCollider(physics_state *State)
{
	collider *Collider = State->FirstFreeCollider;
	if(Collider)
	{
		State->FirstFreeCollider = Collider->Next;
		ZeroStruct(*Collider);
	}
	else
	{
		Collider = PushStruct(State->Region, collider);
	}

	return Collider;
}

internal collider *
BuildConvexHullCollider(game_state *GameState, physics_state *State, v3 *Points, u32 PointCount)
{
	// TODO: We are calculating Mass and LocalInertialTensor and Centroid
	//	as if the object was its AABB.
	collider *Collider = AddCollider(State);

	Collider->Shape = {};
	Collider->Shape.Type = Shape_ConvexHull;
	convex_hull *Hull = Collider->Shape.ConvexHull = QuickHull(State->Region, &GameState->FrameRegion, Points, PointCount);

	v3 Min = V3(Real32Maximum, Real32Maximum, Real32Maximum);
	v3 Max = V3(Real32Minimum, Real32Minimum, Real32Minimum);
	for(u32 PointIndex = 0;
		PointIndex < Hull->VertexCount;
		++PointIndex)
	{
		v3 Point = Hull->Vertices[PointIndex];
		if(Point.x < Min.x) {Min.x = Point.x;}
		if(Point.x > Max.x) {Max.x = Point.x;}
		if(Point.y < Min.y) {Min.y = Point.y;}
		if(Point.y > Max.y) {Max.y = Point.y;}
		if(Point.z < Min.z) {Min.z = Point.z;}
		if(Point.z > Max.z) {Max.z = Point.z;}
	}

	rectangle3 AABB = Rectangle3(Min, Max);
	v3 Dimension = Dim(AABB);
	v3 Centroid = Center(AABB);

	f32 Density = 1.0f;
	Collider->LocalCentroid = Centroid;
	Collider->Mass = (Density*Dimension.x*Dimension.y*Dimension.z);
	Collider->LocalInertialTensor =
	{
		Collider->Mass*(Square(Dimension.y) + Square(Dimension.z))/12.0f, 0, 0,
		0, Collider->Mass*(Square(Dimension.x) + Square(Dimension.z))/12.0f, 0,
		0, 0, Collider->Mass*(Square(Dimension.x) + Square(Dimension.y))/12.0f,
	};

	return Collider;
}

internal collider *
BuildBoxCollider(memory_region *TempRegion, physics_state *State, v3 Centroid, v3 Dim, quaternion Rotation)
{
	collider *Collider = AddCollider(State);

	f32 Density = 1.0f;
	Collider->LocalCentroid = Centroid;
	Collider->Mass = (Density*Dim.x*Dim.y*Dim.z);
	Collider->LocalInertialTensor =
	{
		Collider->Mass*(Square(Dim.y) + Square(Dim.z))/12.0f, 0, 0,
		0, Collider->Mass*(Square(Dim.x) + Square(Dim.z))/12.0f, 0,
		0, 0, Collider->Mass*(Square(Dim.x) + Square(Dim.y))/12.0f,
	};

	Collider->Shape = {};
	Collider->Shape.Type = Shape_ConvexHull;
	v3 HalfDim = 0.5f*Dim;
	v3 Points[] =
	{
		Collider->LocalCentroid + V3(-HalfDim.x, -HalfDim.y, -HalfDim.z),
		Collider->LocalCentroid + V3( HalfDim.x, -HalfDim.y, -HalfDim.z),
		Collider->LocalCentroid + V3(-HalfDim.x,  HalfDim.y, -HalfDim.z),
		Collider->LocalCentroid + V3(-HalfDim.x, -HalfDim.y,  HalfDim.z),
		Collider->LocalCentroid + V3( HalfDim.x,  HalfDim.y, -HalfDim.z),
		Collider->LocalCentroid + V3( HalfDim.x, -HalfDim.y,  HalfDim.z),
		Collider->LocalCentroid + V3(-HalfDim.x,  HalfDim.y,  HalfDim.z),
		Collider->LocalCentroid + V3( HalfDim.x,  HalfDim.y,  HalfDim.z),
	};

	Collider->Shape.ConvexHull = QuickHull(State->Region, TempRegion, Points, ArrayCount(Points));

	return Collider;
}

internal collider *
BuildCapsuleCollider(physics_state *State, v3 Centroid, f32 Radius, f32 Length, quaternion Rotation)
{
	collider *Collider = AddCollider(State);

	f32 Density = 1.0f;
	f32 CylinderMass = Density*Pi32*Radius*Radius*Length;
	f32 SphereMass = Density*(4.0f/3.0f)*Pi32*Radius*Radius*Radius;
	Collider->Mass = CylinderMass + SphereMass;
	Collider->LocalCentroid = Centroid;
	Collider->LocalInertialTensor =
	{
		CylinderMass*(Length*Length/12.0f + Radius*Radius/4.0f) + SphereMass*(2.0f*Radius*Radius/5.0f + Length*Length/2.0f + 3.0f*Length*Radius/8.0f), 0, 0,
		0, CylinderMass*Radius*Radius/2.0f + SphereMass*2.0f*Radius*Radius/5.0f, 0,
		0, 0, CylinderMass*(Length*Length/12.0f + Radius*Radius/4.0f) + SphereMass*(2.0f*Radius*Radius/5.0f + Length*Length/2.0f + 3.0f*Length*Radius/8.0f),
	};

	Collider->Shape = {};
	Collider->Shape.Type = Shape_Capsule;
	Collider->Shape.Capsule.P0 = V3(0, -0.5f*Length, 0);
	Collider->Shape.Capsule.P1 = V3(0, +0.5f*Length, 0);
	Collider->Shape.Capsule.Radius = Radius;

	return Collider;
}

internal collider *
BuildSphereCollider(physics_state *State, v3 Centroid, f32 Radius)
{
	collider *Collider = AddCollider(State);

	f32 Density = 1.0f;
	Collider->Mass = (Density*(4.0f/3.0f)*Pi32*Radius*Radius*Radius);
	Collider->LocalCentroid = Centroid;
	Collider->LocalInertialTensor =
	{
		(2.0f*Collider->Mass*Square(Radius))/(5.0f), 0, 0,
		0, (2.0f*Collider->Mass*Square(Radius))/(5.0f), 0,
		0, 0, (2.0f*Collider->Mass*Square(Radius))/(5.0f),
	};

	Collider->Shape = {};
	Collider->Shape.Type = Shape_Sphere;
	Collider->Shape.Sphere.Center = Collider->LocalCentroid;
	Collider->Shape.Sphere.Radius = Radius;

	return Collider;
}

internal void
CalculateMassInertiaAndLocalCentroid(rigid_body *Body)
{
	Body->LocalCentroid = {};
	f32 Mass = 0.0f;
	for(collider *Collider = Body->ColliderSentinel.Next;
		Collider != &Body->ColliderSentinel;
		Collider = Collider->Next)
	{
		Mass += Collider->Mass;
		Body->LocalCentroid += Collider->Mass*Collider->LocalCentroid;
	}

	Assert(Mass > 0.0f);
	Body->InvMass = 1.0f/Mass;
	Body->LocalCentroid *= Body->InvMass;

	mat3 LocalInertialTensor = {};
	for(collider *Collider = Body->ColliderSentinel.Next;
		Collider != &Body->ColliderSentinel;
		Collider = Collider->Next)
	{
		v3 R = Body->LocalCentroid - Collider->LocalCentroid;
		f32 InnerProduct = Inner(R, R);
		mat3 OuterProduct = Outer(R, R);

		// NOTE: Parallel Axis theorem.
		LocalInertialTensor += Collider->LocalInertialTensor + Collider->Mass*(InnerProduct*IdentityMat3() - OuterProduct);
	}

	Body->LocalInvInertialTensor = Inverse(LocalInertialTensor);
}

internal void
EntityAddCollider(physics_state *State, entity *Entity, collider *NewCollider)
{
	DLIST_INSERT(&Entity->RigidBody.ColliderSentinel, NewCollider);
	CalculateMassInertiaAndLocalCentroid(&Entity->RigidBody);
	AABBUpdate(&State->DynamicAABBTree, Entity);
}

inline move_spec
DefaultMoveSpec()
{
	move_spec Result = {};
	Result.StaticFriction = 1.5f;
	Result.KineticFriction = 0.5f;
	Result.LinearAirFriction = 1.1f;
	Result.RotationAirFriction = 1.1f;
	return Result;
}

internal void
InitializeRigidBody(physics_state *Physics, entity *Entity,
	collider *ColliderSentinel,
	b32 Collideable, move_spec MoveSpec)
{
	if(Entity)
	{
		rigid_body *RigidBody = &Entity->RigidBody;
		Physics->RigidBodyCount++;

		ZeroStruct(*RigidBody);

		RigidBody->Collideable = Collideable;
		RigidBody->MoveSpec = MoveSpec;

		DLIST_INIT(&RigidBody->ColliderSentinel);
		if(ColliderSentinel)
		{
			DLIST_MERGE_LISTS(&RigidBody->ColliderSentinel, ColliderSentinel);
			CalculateMassInertiaAndLocalCentroid(RigidBody);
			UpdateRotation(Entity);
			UpdateWorldCentroidFromPosition(Entity);
			AABBInsert(&Physics->DynamicAABBTree, Entity);
		}
		else
		{
			UpdateRotation(Entity);
			UpdateWorldCentroidFromPosition(Entity);
		}

		RigidBody->IsInitialized = true;
	}
}

internal void
RemoveRigidBody(physics_state *State, entity *Entity)
{
	if(Entity)
	{
		rigid_body *Body = &Entity->RigidBody;
		if(Body->IsInitialized)
		{
			Body->IsInitialized = false;

			if(Body->AABBNode)
			{
				AABBRemove(&State->DynamicAABBTree, Body->AABBNode);
			}

			for(collider *Collider = Body->ColliderSentinel.Next;
				Collider != &Body->ColliderSentinel;
				)
			{
				collider *NextCollider = Collider->Next;

				DLIST_REMOVE(Collider);
				Collider->Next = State->FirstFreeCollider;
				State->FirstFreeCollider = Collider;

				Collider = NextCollider;
			}

			--State->RigidBodyCount;
		}
	}
}

internal joint *
AddJoint(physics_state *State)
{
	joint *Result = State->FirstFreeJoint;
	if(Result)
	{
		State->FirstFreeJoint = Result->Next;
		ZeroStruct(*Result);
	}
	else
	{
		Result = PushStruct(State->Region, joint);
	}

	DLIST_INSERT(&State->JointSentinel, Result);
	return Result;
}

internal void
RemoveJoint(physics_state *State, joint *Joint)
{
	DLIST_REMOVE(Joint);
	Joint->Next = State->FirstFreeJoint;
	State->FirstFreeJoint = Joint;
}

internal void
RemoveJoint(physics_state *State, distance_joint *DistanceJoint)
{
	if(DistanceJoint)
	{
		joint *Joint = (joint *)DistanceJoint;
		RemoveJoint(State, Joint);
	}
}

internal void
RemoveJoint(physics_state *State, controller_joint *Controller)
{
	if(Controller)
	{
		joint *Joint = (joint *)Controller;
		RemoveJoint(State, Joint);
	}
}

internal distance_joint *
AddDistanceJoint(physics_state *Physics, entity *EntityA, entity *EntityB, v3 LocalAnchorA, v3 LocalAnchorB, f32 Distance, b32 Collideable, f32 AngularFrequency = 0.0f, f32 DampingRatio = 0.0f)
{
	Assert(EntityA);
	Assert(EntityB);

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	joint *Joint = AddJoint(Physics);
	Joint->Type = Joint_Distance;

	distance_joint *DistanceJoint = &Joint->DistanceJoint;
	DistanceJoint->EntityA = EntityA;
	DistanceJoint->EntityB = EntityB;
	DistanceJoint->LocalAnchorA = LocalAnchorA;
	DistanceJoint->LocalAnchorB = LocalAnchorB;
	DistanceJoint->Distance = Distance;
	DistanceJoint->Collideable = Collideable;
	DistanceJoint->AngularFrequency = AngularFrequency;
	DistanceJoint->DampingRatio = DampingRatio;

	if(!DistanceJoint->Collideable)
	{
		Assert(Physics->UncollideableCount < ArrayCount(Physics->Uncollideables));
		pointer_pair *Pair = Physics->Uncollideables + Physics->UncollideableCount++;
		Pair->A = BodyA;
		Pair->B = BodyB;
	}

	return DistanceJoint;
}

internal controller_joint *
AddControllerJoint(physics_state *Physics, entity *Entity, v3 LinearMotor, f32 MaxImpulse, v3 IgnoredAxis = V3(0,0,0))
{
	joint *Joint = AddJoint(Physics);
	Joint->Type = Joint_Controller;

	controller_joint *Controller = &Joint->ControllerJoint;
	Controller->Entity = Entity;
	Controller->LinearMotor = LinearMotor;
	Controller->MaxImpulse = MaxImpulse;
	Controller->IgnoredAxis = IgnoredAxis;

	return Controller;
}

inline void
MoveEntity(physics_state *State, entity *Entity, v3 P)
{
	Entity->P = P;
	UpdateWorldCentroidFromPosition(Entity);
	AABBUpdate(&State->DynamicAABBTree, Entity);
}

internal void
PhysicsIntegrateEntity(physics_state *State, entity *Entity, f32 dt)
{
	rigid_body *Body = &Entity->RigidBody;
	Assert(Body->IsInitialized);

	Body->WorldCentroid += Entity->dP*dt;
	Entity->Rotation = Entity->Rotation + 0.5f*dt*Q(0, Entity->AngularVelocity)*Entity->Rotation;

	UpdatePositionFromWorldCentroid(Entity);
	UpdateRotation(Entity);
	AABBUpdate(&State->DynamicAABBTree, Entity);
}

internal void
AddStaticCollision(physics_state *State, entity *Entity, static_triangle *Triangle)
{
	Assert(State->StaticCollisionCount < ArrayCount(State->StaticCollisions));
	static_collision *Collision = State->StaticCollisions + State->StaticCollisionCount++;
	Collision->Entity = Entity;
	Collision->Triangle = Triangle;
}

internal void
PhysicsFindAndResolveConstraints(game_state *GameState, physics_state *State, entity_manager *EntityManager, f32 dt)
{
	{DEBUG_DATA_BLOCK("Physics");
		DEBUG_VALUE(&State->GravityMode);
		DEBUG_VALUE(&State->RigidBodyCount);

		{DEBUG_DATA_BLOCK("Constants");
			DEBUG_VALUE(&State->Baumgarte);
			DEBUG_VALUE(&State->BaumgarteSlop);
			DEBUG_VALUE(&State->RestitutionCoeff);
			DEBUG_VALUE(&State->RestitutionSlop);
			DEBUG_VALUE(&State->PersistentThreshold);
			DEBUG_VALUE(&State->IterationCount);
		}
		{DEBUG_DATA_BLOCK("Contacts");
			DEBUG_VALUE(&State->DisplayContacts);
			DEBUG_VALUE(&State->DisplayContactNormals);
			DEBUG_VALUE(&State->DisplayImpulses);
			DEBUG_VALUE(&State->DisplayJoints);
		}
		{DEBUG_DATA_BLOCK("AABB");
			DEBUG_VALUE(&State->DisplayDynamicAABB);
			DEBUG_VALUE(&State->DisplayStaticAABB);

			{DEBUG_DATA_BLOCK("Tree Diagram");
				DEBUG_UI_ELEMENT(Event_AABBTree, "AABB Tree Diagram");
			}
		}
	}

	{
		TIMED_BLOCK("Broadphase");

		// NOTE: Broadphase
		State->BucketCount = 0;
		State->StaticCollisionCount = 0;

		FOR_EACH(Entity, EntityManager, entity)
		{
			rigid_body *Body = &Entity->RigidBody;
			Body->OnSolidGround = false;

			if(Body->Collideable) // && (Entity->Animation.Type == Animation_Physics))
			{
				// NOTE: O(nlogn) AABB tree Query.

				rectangle3 BodyAABB = GetWorldAABB(Entity);
				aabb_query_result DynamicQuery = AABBQuery(&GameState->FrameRegion, &State->DynamicAABBTree, BodyAABB, State->RigidBodyCount);
				for(u32 TestIndex = 0;
					TestIndex < DynamicQuery.Count;
					++TestIndex)
				{
					aabb_data *QueryData = DynamicQuery.Data + TestIndex;
					Assert(QueryData->Type == AABBType_Entity);
					entity *QueryEntity = QueryData->Entity;
					u32 TestBodyIndex = QueryEntity->ID.Value;
					if(TestBodyIndex > Entity->ID.Value)
					{
						rigid_body *TestBody = &QueryEntity->RigidBody;
						if(TestBody->Collideable)
						{
							if(IsPhysicsEnabled(QueryEntity) ||
							   IsPhysicsEnabled(Entity))
							{
								AddPotentialCollision(GameState, State, Entity, QueryEntity);
							}
						}
					}
				}

				if(IsPhysicsEnabled(Entity))
				{
					// NOTE: Static Geometry
					aabb_query_result StaticQuery = AABBQuery(&GameState->FrameRegion, &State->StaticAABBTree, BodyAABB, 32);
					for(u32 TestIndex = 0;
						TestIndex < StaticQuery.Count;
						++TestIndex)
					{
						aabb_data *QueryData = StaticQuery.Data + TestIndex;
						Assert(QueryData->Type == AABBType_Triangle);
						static_triangle *QueryTriangle = QueryData->Triangle;
						AddStaticCollision(State, Entity, QueryTriangle);
					}
				}
			}
		}
	}

	{
		TIMED_BLOCK("Build Contacts");

		for(contact_manifold *Manifold = State->ManifoldSentinel.Next;
				Manifold != &State->ManifoldSentinel;
				)
		{
			Manifold = ManifoldCheckContacts(State, Manifold);
		}

		for(u32 Index = 0;
			Index < State->StaticCollisionCount;
			++Index)
		{
			static_collision *StaticCollision = State->StaticCollisions + Index;
			entity *EntityA = StaticCollision->Entity;
			rigid_body *BodyA = &EntityA->RigidBody;

			for(collider *ColliderA = BodyA->ColliderSentinel.Next;
				ColliderA != &BodyA->ColliderSentinel;
				ColliderA = ColliderA->Next)
			{
				collision DummyCollision = {};
				DummyCollision.EntityA = EntityA;
				DummyCollision.EntityB = State->StaticsDummy;
				DummyCollision.ColliderA = ColliderA;
				DummyCollision.ColliderB = (collider *)StaticCollision->Triangle; // TODO: This is very weird. I think it is just so hashes are more unique.

				static_triangle *Triangle = StaticCollision->Triangle;
				Assert(Triangle->Collider.Shape.ConvexHull);

				contact_manifold *Manifold = PhysicsGetOrCreateContactManifold(State, DummyCollision);
				GenerateOneShotManifold(State, Manifold, EntityA, ColliderA, State->StaticsDummy, &Triangle->Collider);

				if(Manifold->ContactCount)
				{
					// TODO: Are all the normals the same? Why not hold this in the manifold then.
					v3 Normal = Manifold->Contacts[0].Normal;
					v3 Up = V3(0,1,0);
					f32 InnerProduct = Inner(-Normal, Up);
					f32 CosAngle = Cos(Pi32/6.0f);
					if(InnerProduct > CosAngle)
					{
						BodyA->OnSolidGround = true;
					}
				}
			}
		}

		for(u32 BucketIndex = 0;
			BucketIndex < State->BucketCount;
			++BucketIndex)
		{
			bucket *Bucket = State->PotentialCollisionBuckets + BucketIndex;
			for(u32 PairIndex = 0;
				PairIndex < Bucket->PotentialCollisionCount;
				++PairIndex)
			{
				pointer_pair *Pair = Bucket->PotentialCollisions + PairIndex;
				entity *EntityA = (entity *)Pair->A;
				entity *EntityB = (entity *)Pair->B;
				rigid_body *BodyA = &EntityA->RigidBody;
				rigid_body *BodyB = &EntityB->RigidBody;

				for(collider *ColliderA = BodyA->ColliderSentinel.Next;
					ColliderA != &BodyA->ColliderSentinel;
					ColliderA = ColliderA->Next)
				{
					for(collider *ColliderB = BodyB->ColliderSentinel.Next;
						ColliderB != &BodyB->ColliderSentinel;
						ColliderB = ColliderB->Next)
					{
						collision Collision = {};
						Collision.EntityA = EntityA;
						Collision.EntityB = EntityB;
						Collision.ColliderA = ColliderA;
						Collision.ColliderB = ColliderB;

						contact_manifold *Manifold = PhysicsGetOrCreateContactManifold(State, Collision);
						GenerateOneShotManifold(State, Manifold, EntityA, ColliderA, EntityB, ColliderB);

						if(Manifold->ContactCount)
						{
							// TODO: Are all the normals the same? Why not hold this in the manifold then.
							v3 Normal = Manifold->Contacts[0].Normal;
							v3 Up = V3(0,1,0);
							f32 InnerProduct = Inner(Normal, Up);
							if(InnerProduct > Cos(Pi32/6.0f))
							{
								BodyB->OnSolidGround = true;
							}
							else if(InnerProduct < -Cos(Pi32/6.0f))
							{
								BodyA->OnSolidGround = true;
							}
						}
					}
				}
			}
		}
	}

	{
		TIMED_BLOCK("Resolve Contacts");

		// NOTE: Warm start.
		f32 WarmStartCoeff = 0.75f;

		for(contact_manifold *Manifold = State->ManifoldSentinel.Next;
				Manifold != &State->ManifoldSentinel;
				Manifold = Manifold->Next)
		{
			entity *EntityA = Manifold->Collision.EntityA;
			entity *EntityB = Manifold->Collision.EntityB;

			for(u32 Index = 0;
				Index < Manifold->ContactCount;
				++Index)
			{
				contact_data *Contact = Manifold->Contacts + Index;
				if(Contact->Persistent)
				{
					Contact->NormalImpulseSum *= WarmStartCoeff;
					ApplyImpulse(EntityA, -Contact->NormalImpulseSum*Contact->Normal, Contact->WorldContactPointA);
					ApplyImpulse(EntityB,  Contact->NormalImpulseSum*Contact->Normal, Contact->WorldContactPointB);

					Contact->Tangent0ImpulseSum *= WarmStartCoeff;
					ApplyImpulse(EntityA, -Contact->Tangent0ImpulseSum*Contact->Tangent0, Contact->WorldContactPointA);
					ApplyImpulse(EntityB,  Contact->Tangent0ImpulseSum*Contact->Tangent0, Contact->WorldContactPointB);

					Contact->Tangent1ImpulseSum *= WarmStartCoeff;
					ApplyImpulse(EntityA, -Contact->Tangent1ImpulseSum*Contact->Tangent1, Contact->WorldContactPointA);
					ApplyImpulse(EntityB,  Contact->Tangent1ImpulseSum*Contact->Tangent1, Contact->WorldContactPointB);
				}
			}
		}

		for(joint *Joint = State->JointSentinel.Next;
			Joint != &State->JointSentinel;
			Joint = Joint->Next)
		{
			switch(Joint->Type)
			{
				case Joint_Distance:
				{
					distance_joint *DistanceJoint = &Joint->DistanceJoint;
					entity *EntityA = DistanceJoint->EntityA;
					entity *EntityB = DistanceJoint->EntityB;
					rigid_body *BodyA = &EntityA->RigidBody;
					rigid_body *BodyB = &EntityB->RigidBody;

					v3 WorldAnchorA = LocalPointToWorldPoint(EntityA, DistanceJoint->LocalAnchorA);
					v3 WorldAnchorB = LocalPointToWorldPoint(EntityB, DistanceJoint->LocalAnchorB);

					v3 JointNormal = NOZ(WorldAnchorB - WorldAnchorA);

					DistanceJoint->ImpulseSum *= WarmStartCoeff;
					ApplyImpulse(EntityA, -DistanceJoint->ImpulseSum*JointNormal, WorldAnchorA);
					ApplyImpulse(EntityB,  DistanceJoint->ImpulseSum*JointNormal, WorldAnchorB);
				} break;

				case Joint_Controller:
				{
					// TODO: Do we want warmstarting for these?
#if 0
					controller_joint *Controller = &Joint->ControllerJoint;
					rigid_body *Body = Controller->Body;
					v3 WorldAnchor = Body->WorldCentroid;
					v3 MotorNormal = NOZ(Controller->LinearMotor);

					Controller->ImpulseSum *= WarmStartCoeff;
					ApplyImpulse(Body, Controller->ImpulseSum*MotorNormal, WorldAnchor);

#endif
				} break;

				InvalidDefaultCase;
			}
		}

		// NOTE: Resolve
		for(u32 Iteration = 0;
			Iteration < State->IterationCount;
			++Iteration)
		{
			for(contact_manifold *Manifold = State->ManifoldSentinel.Next;
				Manifold != &State->ManifoldSentinel;
				Manifold = Manifold->Next)
			{
				entity *EntityA = Manifold->Collision.EntityA;
				entity *EntityB = Manifold->Collision.EntityB;

				for(u32 Index = 0;
					Index < Manifold->ContactCount;
					++Index)
				{
					contact_data *Contact = Manifold->Contacts + Index;
					SolveContactConstraints(State, EntityA, EntityB, Contact, dt);
				}
			}

			for(joint *Joint = State->JointSentinel.Next;
				Joint != &State->JointSentinel;
				Joint = Joint->Next)
			{
				switch(Joint->Type)
				{
					case Joint_Distance:
					{
						distance_joint *DistanceJoint = &Joint->DistanceJoint;
						entity *EntityA = DistanceJoint->EntityA;
						entity *EntityB = DistanceJoint->EntityB;

						SolveDistanceJointConstraints(State, EntityA, EntityB, DistanceJoint, dt);
					} break;

					case Joint_Controller:
					{
						controller_joint *Controller = &Joint->ControllerJoint;
						entity *Entity = Controller->Entity;

						SolveControllerJointConstraints(State, Entity, Controller);
					} break;

					InvalidDefaultCase;
				}
			}
		}
	}
}

internal void
DEBUGDisplayContactInfo(physics_state *State, render_group *Group)
{
	if(State->DisplayContacts)
	{
		for(contact_manifold *Manifold = State->ManifoldSentinel.Next;
			Manifold != &State->ManifoldSentinel;
			Manifold = Manifold->Next)
		{
			contact_data *LastContact = (Manifold->ContactCount > 1) ? (Manifold->Contacts + Manifold->ContactCount - 1) : 0;
			for(u32 Index = 0;
				Index < Manifold->ContactCount;
				++Index)
			{
				contact_data *Contact = Manifold->Contacts + Index;

				v4 ColorA = V4(1.0f, 1.0f, 0.0f, 1.0f);
				v4 ColorB = V4(1.0f, 0.0f, 0.0f, 1.0f);
				if(Contact->Persistent)
				{
					v4 PersistentColorA = V4(1.0f, 1.0f, 1.0f, 1.0f);
					v4 PersistentColorB = V4(1.0f, 0.0f, 1.0f, 1.0f);
					f32 t = Clamp01(Contact->PersistentCount / 256.0f);
					ColorA = Lerp(ColorA, t, PersistentColorA);
					ColorB = Lerp(ColorB, t, PersistentColorB);
				}

				PushPoint(Group, Contact->WorldContactPointA, ColorA.rgb);
				PushPoint(Group, Contact->WorldContactPointB, ColorB.rgb);

				if(State->DisplayContactNormals)
				{
					f32 Scale = 1.0f;
					PushLine(Group, Contact->WorldContactPointA, Contact->WorldContactPointA + Scale*Contact->Normal, 0.05f, V4(1.0f, 1.0f, 0.0f, 1.0f));
					PushLine(Group, Contact->WorldContactPointB, Contact->WorldContactPointB + Scale*Contact->Normal, 0.05f, V4(0.0f, 1.0f, 1.0f, 1.0f));
				}

				if(State->DisplayImpulses)
				{
					f32 Scale = 0.5f;
					v3 ImpulseVector = Scale*(Contact->NormalImpulseSum*Contact->Normal + Contact->Tangent0ImpulseSum*Contact->Tangent0 + Contact->Tangent1ImpulseSum*Contact->Tangent1);
					if(LengthSq(ImpulseVector) > 0)
					{
						PushLine(Group, Contact->WorldContactPointA, Contact->WorldContactPointA + -ImpulseVector, 0.05f, V4(1.0f, 1.0f, 0.0f, 1.0f));
						PushLine(Group, Contact->WorldContactPointB, Contact->WorldContactPointB +  ImpulseVector, 0.05f, V4(0.0f, 1.0f, 1.0f, 1.0f));
					}
				}

				if(LastContact)
				{
					contact_data *ContactData0 = LastContact;
					contact_data *ContactData1 = Contact;
					PushLine(Group, ContactData0->WorldContactPointA, ContactData1->WorldContactPointA, 0.05f, V4(0.5f, 0.5f, 0.5f, 1.0f));
					PushLine(Group, ContactData0->WorldContactPointB, ContactData1->WorldContactPointB, 0.05f, V4(0.5f, 0.5f, 0.5f, 1.0f));
				}

				LastContact = Contact;
			}
		}
	}

	if(State->DisplayJoints)
	{
		for(joint *Joint = State->JointSentinel.Next;
			Joint != &State->JointSentinel;
			Joint = Joint->Next)
		{
			if(Joint->Type == Joint_Distance)
			{
				distance_joint *DistanceJoint = &Joint->DistanceJoint;
				entity *EntityA = DistanceJoint->EntityA;
				entity *EntityB = DistanceJoint->EntityB;
				v3 WorldAnchorA = LocalPointToWorldPoint(EntityA, DistanceJoint->LocalAnchorA);
				v3 WorldAnchorB = LocalPointToWorldPoint(EntityB, DistanceJoint->LocalAnchorB);

				PushLine(Group, WorldAnchorA, WorldAnchorB, 0.05f, V4(0.1f, 0.1f, 0.1f, 1.0f));

				PushPoint(Group, WorldAnchorA);
				PushPoint(Group, WorldAnchorB, V3(1.0f, 0.0f, 0.0f));
			}
		}
	}
}

internal void
PhysicsAddStaticGeometry(game_state *GameState, physics_state *State,
	vertex_format *Vertices, u32 VertexCount,
	v3u *Faces, u32 FaceCount)
{
	u32 TriangleCount = 0;
	static_triangle *Triangles = PushArray(State->Region, FaceCount, static_triangle);

	for(u32 FaceIndex = 0;
		FaceIndex < FaceCount;
		++FaceIndex)
	{
		v3u Face = Faces[FaceIndex];
		static_triangle *Triangle = Triangles + FaceIndex;
		Triangle->Points[0] = Vertices[Face.x].Position;
		Triangle->Points[1] = Vertices[Face.y].Position;
		Triangle->Points[2] = Vertices[Face.z].Position;
		Triangle->Normal = Normalize(Cross(Triangle->Points[1] - Triangle->Points[0], Triangle->Points[2] - Triangle->Points[0]));

		Triangle->Collider.Shape.Type = Shape_ConvexHull;
		Triangle->Collider.Shape.ConvexHull = QuickHull(State->Region, &GameState->FrameRegion, Triangle->Points, 3);

		AABBInsert(&State->StaticAABBTree, Triangle);
	}
}

internal void
InitializePhysics(physics_state *State, entity_manager *EntityManager, memory_region *Region)
{
	State->Region = Region;

	DLIST_INIT(&State->ManifoldSentinel);
	DLIST_INIT(&State->JointSentinel);

	State->DynamicAABBTree.Region = State->Region;
	State->StaticAABBTree.Region = State->Region;

	State->Baumgarte = 0.2f;
	State->BaumgarteSlop = 0.01f;
	State->RestitutionCoeff = 0.1f;
	State->RestitutionSlop = 0.5f;
	State->PersistentThreshold = 0.05f;
	State->IterationCount = 4;

	State->StaticsDummy = AddEntity(EntityManager);
	InitializeRigidBody(State, State->StaticsDummy, 0, false, DefaultMoveSpec());
	State->StaticsDummy->RigidBody.InvMass = 0.0f;
	State->StaticsDummy->RigidBody.LocalInvInertialTensor = {};

	State->GravityMode = true;
	State->DisplayStaticAABB = false;
	State->DisplayDynamicAABB = false;
	State->DisplayContacts = false;
	State->DisplayContactNormals = false;
	State->DisplayImpulses = false;
	State->DisplayJoints = false;
}
