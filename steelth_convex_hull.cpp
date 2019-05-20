/*@H
* File: steelth_convex_hull.cpp
* Author: Jesse Calvert
* Created: January 4, 2018, 20:38
* Last modified: April 10, 2019, 17:31
*/

#if 0
internal v3
CalculateConvexHullFaceNormal(convex_hull *Hull, u8 FaceIndex)
{
	// NOTE: Newell's method.
	v3 Result = {};
	face *Face = Hull->Faces + FaceIndex;
	half_edge *Edge = Hull->Edges + Face->FirstEdge;
	v3 LastPoint = Hull->Vertices[Edge->Origin];
	do
	{
		Edge = Hull->Edges + Edge->Next;
		v3 Point = Hull->Vertices[Edge->Origin];

		Result.x += (LastPoint.y - Point.y)*(LastPoint.z + Point.z);
		Result.y += (LastPoint.z - Point.z)*(LastPoint.x + Point.x);
		Result.z += (LastPoint.x - Point.x)*(LastPoint.y + Point.y);

		LastPoint = Point;
	} while(Edge != Hull->Edges + Face->FirstEdge);

	Result = Normalize(Result);
	return Result;
}
#endif

internal void
CalculateFaceNormalAndCenter(qh_face *Face)
{
	// NOTE: Newell's method.
	v3 Normal = {};
	v3 VerticesSum = {};
	u32 VerticesCount = 0;

	qh_half_edge * Edge = Face->FirstEdge;
	v3 ReferencePoint = Edge->Origin;
	v3 LastPoint = {};
	do
	{
		Edge = Edge->Next;
		v3 Point = Edge->Origin - ReferencePoint;

		Normal.x += (LastPoint.y - Point.y)*(LastPoint.z + Point.z);
		Normal.y += (LastPoint.z - Point.z)*(LastPoint.x + Point.x);
		Normal.z += (LastPoint.x - Point.x)*(LastPoint.y + Point.y);

		VerticesSum += Point;
		++VerticesCount;

		LastPoint = Point;
	} while(Edge != Face->FirstEdge);

	Face->Normal = Normalize(Normal);
	Face->Center = ((1.0f/VerticesCount)*VerticesSum) + ReferencePoint;
}

internal qh_face *
QuickHullAddFace(memory_region *TempRegion, qh_convex_hull *Hull)
{
	qh_face *Result = 0; // State->FirstFreeQHFace;
	if(Result)
	{
		// State->FirstFreeQHFace = Result->NextFree;
		ZeroStruct(*Result);
	}
	else
	{
		Result = PushStruct(TempRegion, qh_face);
	}

	DLIST_INSERT(&Hull->FaceSentinel, Result);
	DLIST_INIT(&Result->ConflictSentinel);
	Result->Index = -1;
	++Hull->FaceCount;
	return Result;
}

internal qh_half_edge *
QuickHullAddEdge(memory_region *TempRegion, qh_convex_hull *Hull)
{
	qh_half_edge_list_node *EdgeNode = 0; //State->FirstFreeQHEdgeNode;
	if(EdgeNode)
	{
		// State->FirstFreeQHEdgeNode = EdgeNode->NextFree;
		ZeroStruct(*EdgeNode);
	}
	else
	{
		EdgeNode = PushStruct(TempRegion, qh_half_edge_list_node);
	}

	DLIST_INSERT(&Hull->EdgeSentinel, EdgeNode);
	qh_half_edge *Result = &EdgeNode->Edge;
	Result->Index = -1;
	Result->OriginIndex = -1;
	++Hull->EdgeCount;
	return Result;
}

internal qh_conflict_vertex *
QuickHullAddConflictVertex(memory_region *TempRegion, qh_convex_hull *Hull, qh_face *Face)
{
	qh_conflict_vertex *Result = PushStruct(TempRegion, qh_conflict_vertex);
	DLIST_INSERT(&Face->ConflictSentinel, Result);
	return Result;
}

internal void
QuickHullRemoveEdge(qh_convex_hull *Hull, qh_half_edge *Edge)
{
	qh_half_edge_list_node *ListNode = (qh_half_edge_list_node *)Edge;
	DLIST_REMOVE(ListNode);

	if(Edge->Twin && (Edge->Twin->Twin == Edge))
	{
		Edge->Twin->Twin = 0;
	}

	// ListNode->NextFree = State->FirstFreeQHEdgeNode;
	// State->FirstFreeQHEdgeNode = ListNode;
	--Hull->EdgeCount;
}

internal void
QuickHullRemoveFace(qh_convex_hull *Hull, qh_face *Face)
{
	Assert(Face->ConflictSentinel.Next == &Face->ConflictSentinel);
	DLIST_REMOVE(Face);

	qh_half_edge *Edge = Face->FirstEdge;
	if(Edge)
	{
		do
		{
			qh_half_edge *NextEdge = Edge->Next;
			QuickHullRemoveEdge(Hull, Edge);
			Edge = NextEdge;
		} while(Edge != Face->FirstEdge);
	}

	// Face->NextFree = State->FirstFreeQHFace;
	// State->FirstFreeQHFace = Face;
	Face->Prev = 0;
	--Hull->FaceCount;
}

internal void
QuickHullRemoveConflictVertex(qh_conflict_vertex *Vertex)
{
	DLIST_REMOVE(Vertex);
}

internal qh_convex_hull *
QuickHullInitialTetrahedron(memory_region *TempRegion, v3 *Points, u32 PointCount, b32 FillConflictLists=true)
{
	Assert(PointCount > 3);

	u32 InitialPointIndicies[4] = {};
	s32 MinPointX = -1;
	s32 MaxPointX = -1;
	s32 MinPointY = -1;
	s32 MaxPointY = -1;
	s32 MinPointZ = -1;
	s32 MaxPointZ = -1;

	v3 Min = V3(Real32Maximum, Real32Maximum, Real32Maximum);
	v3 Max = V3(Real32Minimum, Real32Minimum, Real32Minimum);
	for(u32 Index = 0;
		Index < PointCount;
		++Index)
	{
		v3 Point = Points[Index];
		if(Point.x < Min.x) {Min.x = Point.x; MinPointX = Index;}
		if(Point.x > Max.x) {Max.x = Point.x; MaxPointX = Index;}
		if(Point.y < Min.y) {Min.y = Point.y; MinPointY = Index;}
		if(Point.y > Max.y) {Max.y = Point.y; MaxPointY = Index;}
		if(Point.z < Min.z) {Min.z = Point.z; MinPointZ = Index;}
		if(Point.z > Max.z) {Max.z = Point.z; MaxPointZ = Index;}
	}

	v3 MaxDistances = Max - Min;
	if(MaxDistances.x > MaxDistances.y)
	{
		if(MaxDistances.x > MaxDistances.z)
		{
			InitialPointIndicies[0] = MinPointX;
			InitialPointIndicies[1] = MaxPointX;
		}
		else
		{
			InitialPointIndicies[0] = MinPointZ;
			InitialPointIndicies[1] = MaxPointZ;
		}
	}
	else
	{
		if(MaxDistances.y > MaxDistances.z)
		{
			InitialPointIndicies[0] = MinPointY;
			InitialPointIndicies[1] = MaxPointY;
		}
		else
		{
			InitialPointIndicies[0] = MinPointZ;
			InitialPointIndicies[1] = MaxPointZ;
		}
	}

	v3 Line = Points[InitialPointIndicies[1]] - Points[InitialPointIndicies[0]];
	v3 LineUnit = Normalize(Line);
	f32 FurthestDistanceSq = 0.0f;
	s32 FurthestDistanceFromLineIndex = -1;
	for(u32 Index = 0;
		Index < PointCount;
		++Index)
	{
		v3 Point = Points[Index];
		f32 DistanceSq = DistanceToLineSq(Point - Points[InitialPointIndicies[0]], LineUnit);
		if(DistanceSq > FurthestDistanceSq)
		{
			FurthestDistanceSq = DistanceSq;
			FurthestDistanceFromLineIndex = Index;
		}
	}
	InitialPointIndicies[2] = FurthestDistanceFromLineIndex;

	v3 PlaneNormalDirection = Cross(Points[InitialPointIndicies[1]] - Points[InitialPointIndicies[0]],
		Points[InitialPointIndicies[2]] - Points[InitialPointIndicies[0]]);
	f32 FurthestDistance = 0.0f;
	f32 FurthestSignedDistance = 0.0f;
	s32 FurthestDistanceFromPlaneIndex = -1;
	for(u32 Index = 0;
		Index < PointCount;
		++Index)
	{
		v3 Point = Points[Index];
		f32 SignedDistance = Inner(Point - Points[InitialPointIndicies[0]], PlaneNormalDirection);
		f32 Distance = AbsoluteValue(SignedDistance);
		if(Distance > FurthestDistance)
		{
			FurthestDistance = Distance;
			FurthestSignedDistance = SignedDistance;
			FurthestDistanceFromPlaneIndex = Index;
		}
	}
	InitialPointIndicies[3] = FurthestDistanceFromPlaneIndex;

	if(FurthestSignedDistance > 0.0f)
	{
		Swap(InitialPointIndicies[0], InitialPointIndicies[1], u32);
	}

	v3 Vertices[4] = {};
	Vertices[0] = Points[InitialPointIndicies[0]];
	Vertices[1] = Points[InitialPointIndicies[1]];
	Vertices[2] = Points[InitialPointIndicies[2]];
	Vertices[3] = Points[InitialPointIndicies[3]];

	qh_convex_hull *Result = PushStruct(TempRegion, qh_convex_hull);
	DLIST_INIT(&Result->EdgeSentinel);
	DLIST_INIT(&Result->FaceSentinel);
	Result->VertexCount = 4;

	qh_half_edge *Edges[12];
	for(u32 Index = 0;
		Index < ArrayCount(Edges);
		++Index)
	{
		Edges[Index] = QuickHullAddEdge(TempRegion, Result);
	}

	qh_face *Faces[4];
	for(u32 Index = 0;
		Index < ArrayCount(Faces);
		++Index)
	{
		Faces[Index] = QuickHullAddFace(TempRegion, Result);
	}

	*Edges[0] = {Vertices[0],Edges[1],Edges[3],Faces[0]};
	*Edges[1] = {Vertices[1],Edges[2],Edges[4],Faces[0]};
	*Edges[2] = {Vertices[2],Edges[0],Edges[5],Faces[0]};
	*Edges[3] = {Vertices[1],Edges[6],Edges[0],Faces[1]};
	*Edges[4] = {Vertices[2],Edges[8],Edges[1],Faces[2]};
	*Edges[5] = {Vertices[0],Edges[10],Edges[2],Faces[3]};
	*Edges[6] = {Vertices[0],Edges[7],Edges[11],Faces[1]};
	*Edges[7] = {Vertices[3],Edges[3],Edges[8],Faces[1]};
	*Edges[8] = {Vertices[1],Edges[9],Edges[7],Faces[2]};
	*Edges[9] = {Vertices[3],Edges[4],Edges[10],Faces[2]};
	*Edges[10] = {Vertices[2],Edges[11],Edges[9],Faces[3]};
	*Edges[11] = {Vertices[3],Edges[5],Edges[6],Faces[3]};

	for(u32 Index = 0;
		Index < ArrayCount(Edges);
		++Index)
	{
		Edges[Index]->Index = -1;
		Edges[Index]->OriginIndex = -1;
	}

	Faces[0]->FirstEdge = Edges[0];
	Faces[1]->FirstEdge = Edges[3];
	Faces[2]->FirstEdge = Edges[4];
	Faces[3]->FirstEdge = Edges[5];
	CalculateFaceNormalAndCenter(Faces[0]);
	CalculateFaceNormalAndCenter(Faces[1]);
	CalculateFaceNormalAndCenter(Faces[2]);
	CalculateFaceNormalAndCenter(Faces[3]);

	if(FillConflictLists)
	{
		for(u32 Index = 0;
			Index < PointCount;
			++Index)
		{
			if((Index != InitialPointIndicies[0]) &&
			   (Index != InitialPointIndicies[1]) &&
			   (Index != InitialPointIndicies[2]) &&
			   (Index != InitialPointIndicies[3]))
			{
				v3 Point = Points[Index];

				FurthestDistance = 0.0f;
				qh_face *FurthestFace = 0;
				for(qh_face *Face = Result->FaceSentinel.Next;
					Face != &Result->FaceSentinel;
					Face = Face->Next)
				{
					v3 PlanePoint = Face->FirstEdge->Origin;
					f32 SignedDistance = Inner(Point - PlanePoint, Face->Normal);
					if(SignedDistance > FurthestDistance)
					{
						FurthestDistance = SignedDistance;
						FurthestFace = Face;
					}
				}

				if(FurthestFace)
				{
					qh_conflict_vertex *Vertex = QuickHullAddConflictVertex(TempRegion, Result, FurthestFace);
					Vertex->P = Point;
					Vertex->Distance = FurthestDistance;
				}
			}
		}
	}

	return Result;
}

#if 0
inline b32
PointIsInFrontOfFace(convex_hull *Hull, u8 FaceIndex, v3 TestPoint)
{
	face *Face = Hull->Faces + FaceIndex;
	half_edge *FirstEdge = Hull->Edges + Face->FirstEdge;
	v3 PlanePoint = Hull->Vertices[FirstEdge->Origin];
	f32 SignedDistance = Inner(TestPoint - PlanePoint, Face->Normal);
	b32 Result = (SignedDistance > 0.0f);
	return Result;
}
#endif

inline b32
PointIsInFrontOfFace(qh_face *Face, v3 TestPoint)
{
	qh_half_edge *FirstEdge = Face->FirstEdge;
	v3 PlanePoint = FirstEdge->Origin;
	f32 SignedDistance = Inner(TestPoint - PlanePoint, Face->Normal);
	b32 Result = (SignedDistance > 0.0f);
	return Result;
}

inline b32
EdgeIsCoplanar(qh_half_edge *Edge)
{
	qh_face *FaceLeft = Edge->Face;
	qh_face *FaceRight = Edge->Twin->Face;

	// TODO: Tune this. A bigger Epsilon means the QuickHull will favor large
	// (but inaccurate) faces.
	f32 Epsilon = 0.00001f;

	f32 LeftDistance = Inner(FaceLeft->Center - Edge->Origin, FaceRight->Normal);
	f32 RightDistance = Inner(FaceRight->Center - Edge->Origin, FaceLeft->Normal);

	b32 Convex = (LeftDistance < -Epsilon) && (RightDistance < -Epsilon);
	b32 Concave = (LeftDistance > Epsilon) || (RightDistance > Epsilon);
	b32 Coplanar = !Concave && !Convex;

	if(Concave)
	{
		// TODO: Big ol' problem. Just say it's Coplanar so they merge?
		Coplanar = true;
	}
	return Coplanar;
}

inline qh_half_edge *
GetPrevEdge(qh_half_edge *Edge)
{
	qh_half_edge *EdgePrev = 0;
	qh_half_edge *FaceEdge = Edge->Face->FirstEdge;
	do
	{
		if(FaceEdge->Next == Edge)
		{
			EdgePrev = FaceEdge;
		}
		FaceEdge = FaceEdge->Next;
	} while(FaceEdge != Edge->Face->FirstEdge);

	return EdgePrev;
}

internal b32
QuickHullStep(memory_region *TempRegion, qh_convex_hull *Hull, qh_support *SupportFunction=0)
{
	b32 Result = false;

	qh_face *SeenFace = 0;
	f32 FurthestDistance = 0.0f;
	v3 NewPoint = {};

	if(SupportFunction)
	{
		for(qh_face *Face = Hull->FaceSentinel.Next;
			Face != &Hull->FaceSentinel;
			Face = Face->Next)
		{
			v3 Point = SupportFunction(Face->Normal);
			v3 PlanePoint = Face->FirstEdge->Origin;
			f32 SignedDistance = Inner(Point - PlanePoint, Face->Normal);
			if(SignedDistance > FurthestDistance)
			{
				FurthestDistance = SignedDistance;
				NewPoint = Point;
				SeenFace = Face;
			}
		}
	}
	else
	{
		qh_conflict_vertex *FurthestVertex = 0;
		for(qh_face *Face = Hull->FaceSentinel.Next;
			Face != &Hull->FaceSentinel;
			Face = Face->Next)
		{
			for(qh_conflict_vertex *Vertex = Face->ConflictSentinel.Next;
				Vertex != &Face->ConflictSentinel;
				Vertex = Vertex->Next)
			{
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
			NewPoint = FurthestVertex->P;
			QuickHullRemoveConflictVertex(FurthestVertex);
		}
	}

	if(FurthestDistance > 0.0f)
	{
		Result = true;
		++Hull->VertexCount;

		qh_conflict_vertex OrphanSentinel = {};
		DLIST_INIT(&OrphanSentinel);

		for(qh_face *Face = Hull->FaceSentinel.Next;
			Face != &Hull->FaceSentinel;
			Face = Face->Next)
		{
			Face->Visited = false;
		}
		for(qh_half_edge_list_node *EdgeNode = Hull->EdgeSentinel.Next;
			EdgeNode != &Hull->EdgeSentinel;
			EdgeNode = EdgeNode->Next)
		{
			qh_half_edge *Edge = &EdgeNode->Edge;
			Edge->Visited = false;
		}

		u32 HorizonCount = 0;
		qh_half_edge *Horizon[256] = {};

		u32 VisitedFacesCount = 0;
		qh_face *VisitedFaces[256] = {};
		VisitedFaces[VisitedFacesCount++] = SeenFace;
		SeenFace->Visited = true;

		u32 StackSize = 0;
		qh_half_edge *EdgeStack[256] = {};

		EdgeStack[StackSize++] = SeenFace->FirstEdge;
		while(StackSize)
		{
			qh_half_edge *Edge = EdgeStack[--StackSize];
			Edge->Visited = true;

			qh_half_edge *Next = Edge->Next;
			if(!Next->Visited)
			{
				EdgeStack[StackSize++] = Next;
			}

			qh_half_edge *Twin = Edge->Twin;
			qh_face *TwinFace = Twin->Face;
			if(!TwinFace->Visited)
			{
				if(PointIsInFrontOfFace(TwinFace, NewPoint))
				{
					EdgeStack[StackSize++] = Twin;
					VisitedFaces[VisitedFacesCount++] = TwinFace;
					TwinFace->Visited = true;
				}
				else
				{
					Horizon[HorizonCount++] = Twin;
				}
			}
		}

		for(u32 VisitedFaceIndex = 0;
			VisitedFaceIndex < VisitedFacesCount;
			++VisitedFaceIndex)
		{
			qh_face *Face = VisitedFaces[VisitedFaceIndex];
			DLIST_MERGE_LISTS(&OrphanSentinel, &Face->ConflictSentinel);
			QuickHullRemoveFace(Hull, Face);
		}

		u32 NewFacesCount = 0;
		qh_face *NewFaces[256] = {};

		qh_half_edge *FinalEdge = 0;
		qh_half_edge *LastEdge = 0;
		for(u32 HorizonIndex = 0;
			HorizonIndex < HorizonCount;
			++HorizonIndex)
		{
			qh_half_edge *HorizonEdge = Horizon[HorizonIndex];

			qh_face *NewFace = QuickHullAddFace(TempRegion, Hull);
			qh_half_edge *Edge0 = QuickHullAddEdge(TempRegion, Hull);
			qh_half_edge *Edge1 = QuickHullAddEdge(TempRegion, Hull);
			qh_half_edge *Edge2 = QuickHullAddEdge(TempRegion, Hull);

			Edge0->Origin = NewPoint;
			Edge0->Next = Edge1;
			Edge0->Twin = LastEdge;
			Edge0->Face = NewFace;
			if(LastEdge)
			{
				LastEdge->Twin = Edge0;
			}

			Edge1->Origin = HorizonEdge->Next->Origin;
			Edge1->Next = Edge2;
			Edge1->Twin = HorizonEdge;
			Edge1->Face = NewFace;
			HorizonEdge->Twin = Edge1;

			Edge2->Origin = HorizonEdge->Origin;
			Edge2->Next = Edge0;
			Edge2->Twin = 0;
			Edge2->Face = NewFace;

			NewFace->FirstEdge = Edge0;
			CalculateFaceNormalAndCenter(NewFace);

			LastEdge = Edge2;
			if(HorizonIndex == 0)
			{
				FinalEdge = Edge0;
			}

			NewFaces[NewFacesCount++] = NewFace;
		}
		LastEdge->Twin = FinalEdge;
		FinalEdge->Twin = LastEdge;

		for(u32 FaceIndex = 0;
			FaceIndex < NewFacesCount;
			++FaceIndex)
		{
			qh_face *Face = NewFaces[FaceIndex];
			if(FaceIsValid(Face))
			{
				b32 Finished = false;
				while(!Finished)
				{
					b32 MergeOccurred = false;
					qh_half_edge *Edge = Face->FirstEdge;
					do
					{
						if(EdgeIsCoplanar(Edge))
						{
							// NOTE: Merge
							qh_face *TwinFace = Edge->Twin->Face;
							qh_half_edge *TwinFaceEdge = TwinFace->FirstEdge;
							qh_half_edge *TwinPrev = 0;
							do
							{
								TwinFaceEdge->Face = Face;
								if(TwinFaceEdge->Next == Edge->Twin)
								{
									TwinPrev = TwinFaceEdge;
								}

								TwinFaceEdge = TwinFaceEdge->Next;
							} while(TwinFaceEdge != TwinFace->FirstEdge);

							qh_half_edge *EdgePrev = GetPrevEdge(Edge);

							Assert(TwinPrev);
							Assert(EdgePrev);

							TwinPrev->Next = Edge->Next;
							EdgePrev->Next = Edge->Twin->Next;

							Face->FirstEdge = Edge->Next;

							TwinFace->FirstEdge = 0;
							DLIST_MERGE_LISTS(&OrphanSentinel, &TwinFace->ConflictSentinel);
							QuickHullRemoveFace(Hull, TwinFace);
							QuickHullRemoveEdge(Hull, Edge->Twin);
							QuickHullRemoveEdge(Hull, Edge);

							// NOTE: Check for errors.
							b32 Errors = true;
							while(Errors)
							{
								qh_half_edge *NewFaceEdge = Face->FirstEdge;

								b32 ErrorFound = false;
								do
								{
									if(NewFaceEdge->Twin->Face == NewFaceEdge->Next->Twin->Face)
									{
										qh_face *ErrorFace = NewFaceEdge->Twin->Face;
										if(ErrorFace->FirstEdge->Next->Next->Next == ErrorFace->FirstEdge)
										{
											// NOTE: ErrorFace is a triangle.
											qh_half_edge *NewFaceEdgePrev = GetPrevEdge(NewFaceEdge);
											qh_half_edge *NonSharedEdge = NewFaceEdge->Twin->Next;

											NewFaceEdgePrev->Next = NonSharedEdge;
											NonSharedEdge->Next = NewFaceEdge->Next->Next;
											NonSharedEdge->Face = Face;
											Face->FirstEdge = NonSharedEdge;

											ErrorFace->FirstEdge = 0;
											DLIST_MERGE_LISTS(&OrphanSentinel, &ErrorFace->ConflictSentinel);
											QuickHullRemoveFace(Hull, ErrorFace);
											QuickHullRemoveEdge(Hull, NewFaceEdge->Twin);
											QuickHullRemoveEdge(Hull, NewFaceEdge->Next->Twin);
											QuickHullRemoveEdge(Hull, NewFaceEdge->Next);
											QuickHullRemoveEdge(Hull, NewFaceEdge);
											--Hull->VertexCount;
										}
										else
										{
											qh_half_edge *DeleteEdge0 = NewFaceEdge->Next;
											qh_half_edge *DeleteEdge1 = NewFaceEdge->Twin;

											NewFaceEdge->Twin = NewFaceEdge->Next->Twin;
											NewFaceEdge->Next = NewFaceEdge->Next->Next;
											NewFaceEdge->Twin->Twin = NewFaceEdge;
											NewFaceEdge->Twin->Next = NewFaceEdge->Twin->Next->Next;
											Face->FirstEdge = NewFaceEdge->Next;
											NewFaceEdge->Twin->Face->FirstEdge = NewFaceEdge->Twin;
											CalculateFaceNormalAndCenter(NewFaceEdge->Twin->Face);

											DLIST_MERGE_LISTS(&OrphanSentinel, &NewFaceEdge->Twin->Face->ConflictSentinel);
											QuickHullRemoveEdge(Hull, DeleteEdge0);
											QuickHullRemoveEdge(Hull, DeleteEdge1);
											--Hull->VertexCount;
										}

										ErrorFound = true;
									}
									else
									{
										NewFaceEdge = NewFaceEdge->Next;
									}
								} while(!ErrorFound && (NewFaceEdge != Face->FirstEdge));

								Errors = ErrorFound;
							}

							MergeOccurred = true;
						}
						else
						{
							Edge = Edge->Next;
						}
					} while(!MergeOccurred && (Edge != Face->FirstEdge));

					if(MergeOccurred)
					{
						CalculateFaceNormalAndCenter(Face);
					}

					Finished = !MergeOccurred;
				}
			}
		}

		for(qh_conflict_vertex *Vertex = OrphanSentinel.Next;
			Vertex != &OrphanSentinel;
			)
		{
			qh_conflict_vertex *NextVertex = Vertex->Next;

			FurthestDistance = 0.0f;
			qh_face *FurthestFace = 0;
			for(qh_face *Face = Hull->FaceSentinel.Next;
				Face != &Hull->FaceSentinel;
				Face = Face->Next)
			{
				v3 PlanePoint = Face->FirstEdge->Origin;
				f32 SignedDistance = Inner(Vertex->P - PlanePoint, Face->Normal);
				if(SignedDistance > FurthestDistance)
				{
					FurthestDistance = SignedDistance;
					FurthestFace = Face;
				}
			}

			if(FurthestFace)
			{
				DLIST_REMOVE(Vertex);
				DLIST_INSERT(&FurthestFace->ConflictSentinel, Vertex);
				Vertex->Distance = FurthestDistance;
			}
			else
			{
				QuickHullRemoveConflictVertex(Vertex);
			}

			Vertex = NextVertex;
		}
	}

	return Result;
}

internal convex_hull *
BuildPermanentConvexHull(memory_region *Region, qh_convex_hull *Hull)
{
	Assert(Hull->VertexCount < CONVEX_HULL_MAX_VERTICES);
	Assert(Hull->EdgeCount < CONVEX_HULL_MAX_VERTICES);
	Assert(Hull->FaceCount < CONVEX_HULL_MAX_VERTICES);

	// TODO: Memory leak.
	convex_hull *Result = PushStruct(Region, convex_hull);
	Result->Vertices = PushArray(Region, Hull->VertexCount, v3);
	Result->Edges = PushArray(Region, Hull->EdgeCount, half_edge);
	Result->Faces = PushArray(Region, Hull->FaceCount, face);

	for(qh_half_edge_list_node *EdgeNode = Hull->EdgeSentinel.Next;
		EdgeNode != &Hull->EdgeSentinel;
		EdgeNode = EdgeNode->Next)
	{
		v3 NewVertex = EdgeNode->Edge.Origin;

		b32 IsNewVertex = true;
		for(u32 Index = 0;
			IsNewVertex && (Index < Result->VertexCount);
			++Index)
		{
			v3 Vertex = Result->Vertices[Index];
			IsNewVertex = (Vertex != NewVertex);

			if(!IsNewVertex)
			{
				EdgeNode->Edge.OriginIndex = Index;
			}
		}

		if(IsNewVertex)
		{
			u32 Index = Result->VertexCount++;
			Result->Vertices[Index] = NewVertex;
			EdgeNode->Edge.OriginIndex = Index;
		}
	}

	u32 FaceIndex = 0;
	for(qh_face *Face = Hull->FaceSentinel.Next;
		Face != &Hull->FaceSentinel;
		Face = Face->Next)
	{
		Face->Index = FaceIndex++;
	}

	u32 EdgeIndex = 0;
	for(qh_half_edge_list_node *EdgeNode = Hull->EdgeSentinel.Next;
		EdgeNode != &Hull->EdgeSentinel;
		EdgeNode = EdgeNode->Next)
	{
		qh_half_edge *Edge = &EdgeNode->Edge;
		if(Edge->Index == -1)
		{
			Edge->Index = EdgeIndex++;
			Edge->Twin->Index = EdgeIndex++;
		}
	}

	for(qh_half_edge_list_node *EdgeNode = Hull->EdgeSentinel.Next;
		EdgeNode != &Hull->EdgeSentinel;
		EdgeNode = EdgeNode->Next)
	{
		qh_half_edge *Edge = &EdgeNode->Edge;
		Result->Edges[Edge->Index].Origin = SafeTruncateS32ToU8(Edge->OriginIndex);
		Result->Edges[Edge->Index].Next = SafeTruncateS32ToU8(Edge->Next->Index);
		Result->Edges[Edge->Index].Twin = SafeTruncateS32ToU8(Edge->Twin->Index);
		Result->Edges[Edge->Index].Face = SafeTruncateS32ToU8(Edge->Face->Index);
		++Result->EdgeCount;
	}

	for(qh_face *Face = Hull->FaceSentinel.Next;
		Face != &Hull->FaceSentinel;
		Face = Face->Next)
	{
		Result->Faces[Face->Index].FirstEdge = SafeTruncateS32ToU8(Face->FirstEdge->Index);
		Result->Faces[Face->Index].Normal = Face->Normal;
		Result->Faces[Face->Index].Center = Face->Center;
		++Result->FaceCount;
	}

	Assert(Result->VertexCount == Hull->VertexCount);
	Assert(Result->EdgeCount == Hull->EdgeCount);
	Assert(Result->FaceCount == Hull->FaceCount);

	return Result;
}

internal convex_hull *
QuickHull(memory_region *Region, memory_region *TempRegion, u32 MaxSteps, qh_support *SupportFunction)
{
	Assert(MaxSteps > 3);

	v3 Directions[] =
	{
		V3(1,0,0),
		V3(0,1,0),
		V3(0,0,1),
		V3(-1,0,0),
		V3(0,-1,0),
		V3(0,0,-1),
	};

	v3 Points[] =
	{
		SupportFunction(Directions[0]),
		SupportFunction(Directions[1]),
		SupportFunction(Directions[2]),
		SupportFunction(Directions[3]),
		SupportFunction(Directions[4]),
		SupportFunction(Directions[5]),
	};

	temporary_memory TempMem = BeginTemporaryMemory(TempRegion);

	qh_convex_hull *Hull = QuickHullInitialTetrahedron(TempRegion, Points, ArrayCount(Points), false);

	while(MaxSteps-- && QuickHullStep(TempRegion, Hull, SupportFunction)) {};

	convex_hull *Result = BuildPermanentConvexHull(Region, Hull);

	EndTemporaryMemory(&TempMem);

	return Result;
}

internal convex_hull *
QuickHull(memory_region *Region, memory_region *TempRegion, v3 *Points, u32 PointCount, u32 MaxSteps=0)
{
	Assert(PointCount > 2);

	convex_hull *Result = 0;

	if(PointCount == 3)
	{
		Assert(Points[0] != Points[1]);
		Assert(Points[1] != Points[2]);
		Assert(Points[2] != Points[0]);

		Result = PushStruct(Region, convex_hull);
		Result->VertexCount = 3;
		Result->EdgeCount = 6;
		Result->FaceCount = 2;
		Result->Vertices = PushArray(Region, Result->VertexCount, v3);
		Result->Edges = PushArray(Region, Result->EdgeCount, half_edge);
		Result->Faces = PushArray(Region, Result->FaceCount, face);

		Result->Vertices[0] = Points[0];
		Result->Vertices[1] = Points[1];
		Result->Vertices[2] = Points[2];

		Result->Edges[0] = {0,2,1,0};
		Result->Edges[1] = {1,5,0,1};
		Result->Edges[2] = {1,4,3,0};
		Result->Edges[3] = {2,1,2,1};
		Result->Edges[4] = {2,0,5,0};
		Result->Edges[5] = {0,3,4,1};

		v3 Center = (1.0f/3.0f)*(Points[0] + Points[1] + Points[2]);
		v3 Normal = Normalize(Cross(Points[1] - Points[0], Points[2] - Points[0]));

		Result->Faces[0].FirstEdge = 0;
		Result->Faces[0].Center = Center;
		Result->Faces[0].Normal = Normal;
		Result->Faces[1].FirstEdge = 1;
		Result->Faces[1].Center = Center;
		Result->Faces[1].Normal = -Normal;
	}
	else
	{
		temporary_memory TempMem = BeginTemporaryMemory(TempRegion);

		qh_convex_hull *Hull = 0;

		Hull = QuickHullInitialTetrahedron(TempRegion, Points, PointCount);

		if(MaxSteps)
		{
			while(MaxSteps-- && QuickHullStep(TempRegion, Hull)) {};
		}
		else
		{
			while(QuickHullStep(TempRegion, Hull)) {};
		}

		Result = BuildPermanentConvexHull(Region, Hull);

		EndTemporaryMemory(&TempMem);
	}

	return Result;
}
