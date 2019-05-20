/*@H
* File: steelth_aabb.cpp
* Author: Jesse Calvert
* Created: January 3, 2018, 16:11
* Last modified: April 18, 2019, 13:10
*/

inline v3
LocalSupport(shape *Shape, v3 Direction, f32 Scale = 1.0f)
{
	v3 Result = {};

	switch(Shape->Type)
	{
		case Shape_Sphere:
		{
			sphere *Sphere = &Shape->Sphere;
			v3 DirectionNormal = Normalize(Direction);
			Result = Sphere->Center + Scale*Sphere->Radius*DirectionNormal;
		} break;

		case Shape_Capsule:
		{
			capsule *Capsule = &Shape->Capsule;
			v3 DirectionNormal = Normalize(Direction);
			if(SameDirection(Direction, Capsule->P1 - Capsule->P0))
			{
				Result = Capsule->P1 + Scale*Capsule->Radius*DirectionNormal;
			}
			else
			{
				Result = Capsule->P0 + Scale*Capsule->Radius*DirectionNormal;
			}
		} break;

		case Shape_ConvexHull:
		{
			convex_hull *Hull = Shape->ConvexHull;

			f32 LargestInnerProduct = Real32Minimum;
			for(u32 VertexIndex = 0;
				VertexIndex < Hull->VertexCount;
				++VertexIndex)
			{
				v3 Vertex = Hull->Vertices[VertexIndex];
				f32 InnerProduct = Inner(Vertex, Direction);
				if(InnerProduct > LargestInnerProduct)
				{
					LargestInnerProduct = InnerProduct;
					Result = Vertex;
				}
			}
		} break;

		InvalidDefaultCase;
	}

	return Result;
}

inline v3
Support(entity *Entity, collider *Collider, v3 WorldDirection, f32 Scale=1.0f)
{
	v3 LocalDirection = WorldVectorToLocalVector(Entity, WorldDirection);
	v3 SupportLocal = LocalSupport(&Collider->Shape, LocalDirection, Scale);
	v3 Result = LocalPointToWorldPoint(Entity, SupportLocal);
	return Result;
}

internal rectangle3
GetWorldAABB(entity *Entity, v3 Margin = V3(0.0f, 0.0f, 0.0f))
{
	rectangle3 Result = {};
	b32 IsFirst = true;

	rigid_body *Body = &Entity->RigidBody;
	for(collider *Collider = Body->ColliderSentinel.Next;
		Collider != &Body->ColliderSentinel;
		Collider = Collider->Next)
	{
		shape *Shape = &Collider->Shape;
		switch(Shape->Type)
		{
			case Shape_Sphere:
			{
				sphere *Sphere = &Shape->Sphere;
				v3 HalfDim = V3(Sphere->Radius, Sphere->Radius, Sphere->Radius);
				v3 Center = LocalPointToWorldPoint(Entity, Sphere->Center);

				rectangle3 AABB = Rectangle3(Center - HalfDim, Center + HalfDim);
				if(IsFirst)
				{
					Result = AABB;
					IsFirst = false;
				}
				else
				{
					Result = Combine(AABB, Result);
				}
			} break;

			case Shape_ConvexHull:
			{
				convex_hull *Hull = Shape->ConvexHull;

				v3 Min = V3(Real32Maximum, Real32Maximum, Real32Maximum);
				v3 Max = V3(Real32Minimum, Real32Minimum, Real32Minimum);
				for(u32 PointIndex = 0;
					PointIndex < Hull->VertexCount;
					++PointIndex)
				{
					v3 Point = LocalPointToWorldPoint(Entity, Hull->Vertices[PointIndex]);
					if(Point.x < Min.x) {Min.x = Point.x;}
					if(Point.x > Max.x) {Max.x = Point.x;}
					if(Point.y < Min.y) {Min.y = Point.y;}
					if(Point.y > Max.y) {Max.y = Point.y;}
					if(Point.z < Min.z) {Min.z = Point.z;}
					if(Point.z > Max.z) {Max.z = Point.z;}
				}

				rectangle3 AABB = Rectangle3(Min, Max);
				if(IsFirst)
				{
					Result = AABB;
					IsFirst = false;
				}
				else
				{
					Result = Combine(AABB, Result);
				}
			} break;

			default:
			{
				v3 Min, Max;
				Min.x = Support(Entity, Collider, V3(-1,0,0)).x;
				Min.y = Support(Entity, Collider, V3(0,-1,0)).y;
				Min.z = Support(Entity, Collider, V3(0,0,-1)).z;
				Max.x = Support(Entity, Collider, V3(1,0,0)).x;
				Max.y = Support(Entity, Collider, V3(0,1,0)).y;
				Max.z = Support(Entity, Collider, V3(0,0,1)).z;

				rectangle3 AABB = Rectangle3(Min, Max);
				if(IsFirst)
				{
					Result = AABB;
					IsFirst = false;
				}
				else
				{
					Result = Combine(AABB, Result);
				}
			}
		}
	}

	Result = Rectangle3(Result.Min - Margin, Result.Max + Margin);
	return Result;
}

internal aabb_tree_node *
AABBCreateNewNode(aabb_tree *Tree)
{
	aabb_tree_node *Result = Tree->NodeFreeList;
	if(Result)
	{
		Tree->NodeFreeList = Result->NextInFreeList;
		ZeroStruct(*Result);
	}
	else
	{
		Result = PushStruct(Tree->Region, aabb_tree_node);
	}
	return Result;
}

internal aabb_tree_node *
AABBChooseBestBranch(aabb_tree_node *Branch, aabb_tree_node *NewLeaf)
{
	aabb_tree_node *Result = 0;
	f32 LeftCost = SurfaceArea(Combine(NewLeaf->AABB, Branch->Left->AABB));
	f32 RightCost = SurfaceArea(Combine(NewLeaf->AABB, Branch->Right->AABB));
	if(LeftCost < RightCost)
	{
		Result = Branch->Left;
	}
	else
	{
		Result = Branch->Right;
	}

	return Result;
}

internal void
AABBNodeRotate(aabb_tree *Tree, aabb_tree_node *Node, b32 RotateLeft)
{
	TIMED_FUNCTION();
	aabb_tree_node *NewParent = RotateLeft ? Node->Right : Node->Left;
	b32 NewChildLeft = NewParent->Left->Height < NewParent->Right->Height;
	aabb_tree_node *NewChild = NewChildLeft ? NewParent->Left : NewParent->Right;
	aabb_tree_node *GrandParent = Node->Parent;

	if(GrandParent)
	{
		if(GrandParent->Left == Node)
		{
			GrandParent->Left = NewParent;
		}
		else
		{
			GrandParent->Right = NewParent;
		}
	}
	else
	{
		Tree->Root = NewParent;
	}

	NewParent->Parent = GrandParent;
	if(NewChildLeft)
	{
		NewParent->Left = Node;
	}
	else
	{
		NewParent->Right = Node;
	}
	if(RotateLeft)
	{
		Node->Right = NewChild;
	}
	else
	{
		Node->Left = NewChild;
	}
	Node->Parent = NewParent;
	NewChild->Parent = Node;
}

internal void
AABBSyncHierarchy(aabb_tree *Tree, aabb_tree_node *Node)
{
	TIMED_FUNCTION();
	while(Node)
	{
		Assert(!NodeIsLeaf(Node));
		if(Node->Left->Height > (Node->Right->Height + 1))
		{
			AABBNodeRotate(Tree, Node, false);
		}
		else if(Node->Right->Height > (Node->Left->Height + 1))
		{
			AABBNodeRotate(Tree, Node, true);
		}

		Node->AABB = Combine(Node->Left->AABB, Node->Right->AABB);
		Node->Height = 1 + Maximum(Node->Left->Height, Node->Right->Height);
		Node = Node->Parent;
	}
}

internal aabb_tree_node *
AABBInsert(aabb_tree *Tree, rectangle3 AABB, void *Data, aabb_data_type Type)
{
	TIMED_FUNCTION();

	aabb_tree_node *NewLeaf = AABBCreateNewNode(Tree);
	NewLeaf->AABB = AABB;
	NewLeaf->Data.Type = Type;
	NewLeaf->Data.VoidPtr = Data;

	aabb_tree_node *Sibling = Tree->Root;
	if(Sibling)
	{
		while(!NodeIsLeaf(Sibling))
		{
			Sibling = AABBChooseBestBranch(Sibling, NewLeaf);
		}

		aabb_tree_node *NewBranch = AABBCreateNewNode(Tree);
		NewBranch->Parent = Sibling->Parent;
		NewBranch->Left = NewLeaf;
		NewBranch->Right = Sibling;
		if(NewBranch->Parent)
		{
			if(NewBranch->Parent->Left == Sibling)
			{
				NewBranch->Parent->Left = NewBranch;
			}
			else
			{
				NewBranch->Parent->Right = NewBranch;
			}
		}
		else
		{
			Tree->Root = NewBranch;
		}

		NewLeaf->Parent = NewBranch;
		Sibling->Parent = NewBranch;

		AABBSyncHierarchy(Tree, NewBranch);
	}
	else
	{
		Tree->Root = NewLeaf;
	}

	return NewLeaf;
}

internal aabb_tree_node *
AABBInsert(aabb_tree *Tree, entity *Entity, v3 Margin = V3(0,0,0))
{
	rectangle3 AABB = GetWorldAABB(Entity, Margin);
	aabb_tree_node *Result = AABBInsert(Tree, AABB, Entity, AABBType_Entity);
	Entity->RigidBody.AABBNode = Result;
	return Result;
}

internal aabb_tree_node *
AABBInsert(aabb_tree *Tree, static_triangle *Triangle)
{
	v3 Margin = V3(0.001f, 0.001f, 0.001f);
	v3 Min = {Real32Maximum, Real32Maximum, Real32Maximum};
	v3 Max = {Real32Minimum, Real32Minimum, Real32Minimum};
	for(u32 Index = 0;
		Index < ArrayCount(Triangle->Points);
		++Index)
	{
		v3 Point = Triangle->Points[Index];
		if(Point.x < Min.x) {Min.x = Point.x;}
		if(Point.x > Max.x) {Max.x = Point.x;}
		if(Point.y < Min.y) {Min.y = Point.y;}
		if(Point.y > Max.y) {Max.y = Point.y;}
		if(Point.z < Min.z) {Min.z = Point.z;}
		if(Point.z > Max.z) {Max.z = Point.z;}
	}

	rectangle3 AABB = {Min - Margin, Max + Margin};
	aabb_tree_node *Result = AABBInsert(Tree, AABB, Triangle, AABBType_Triangle);
	return Result;
}

internal void
AABBRemove(aabb_tree *Tree, aabb_tree_node *NodeToRemove)
{
	TIMED_FUNCTION();
	aabb_tree_node *Parent = NodeToRemove->Parent;
	if(Parent)
	{
		aabb_tree_node *Sibling = ((Parent->Left == NodeToRemove) ? Parent->Right : Parent->Left);
		Assert(Sibling);

		aabb_tree_node *GrandParent = Parent->Parent;
		if(GrandParent)
		{
			if(GrandParent->Left == Parent)
			{
				GrandParent->Left = Sibling;
			}
			else
			{
				GrandParent->Right = Sibling;
			}

			Sibling->Parent = GrandParent;
			AABBSyncHierarchy(Tree, GrandParent);
		}
		else
		{
			Tree->Root = Sibling;
			Sibling->Parent = 0;
		}

		Parent->NextInFreeList = Tree->NodeFreeList;
		Tree->NodeFreeList = Parent;
	}
	else
	{
		Tree->Root = 0;
	}

	NodeToRemove->NextInFreeList = Tree->NodeFreeList;
	Tree->NodeFreeList = NodeToRemove;
}

internal void
AABBUpdate(aabb_tree *Tree, entity *Entity)
{
	TIMED_FUNCTION();

	aabb_tree_node *Node = Entity->RigidBody.AABBNode;
	if(Node)
	{
		v3 Margin = V3(0.1f, 0.1f, 0.1f);
		rectangle3 AABB = GetWorldAABB(Entity, Margin);
		b32 NeedsUpdate = !Contains(Node->AABB, AABB);
		if(NeedsUpdate)
		{
			AABBRemove(Tree, Node);
			Entity->RigidBody.AABBNode = 0;
			AABBInsert(Tree, Entity, Margin);
		}
	}
}

internal aabb_query_result
AABBQuery(memory_region	*Region, aabb_tree *Tree, rectangle3 AABB, u32 MaxHits)
{
	// NOTE: This function pushes memory and returns it. Make sure to start temporary
	//	memory before calling this!

	TIMED_FUNCTION();

	aabb_query_result Result = {};
	Result.Data = PushArray(Region, MaxHits, aabb_data);

	u32 FirstNode = 0;
	u32 OnePastLastNode = 0;
	aabb_tree_node *NodeQueue[256];
	NodeQueue[OnePastLastNode++] = Tree->Root;

	while(FirstNode < OnePastLastNode)
	{
		aabb_tree_node *Node = NodeQueue[FirstNode++ % ArrayCount(NodeQueue)];

		if(Node)
		{
			if(RectangleIntersect(AABB, Node->AABB))
			{
				if(NodeIsLeaf(Node))
				{
					if(MaxHits && (Result.Count == MaxHits))
					{
						// NOTE: No more space.
						// TODO: Reserve more space? Send some kind of warning?
					}
					else
					{
						Result.Data[Result.Count++] = Node->Data;
					}
				}
				else
				{
					Assert((OnePastLastNode - FirstNode + 2) < ArrayCount(NodeQueue));
					NodeQueue[OnePastLastNode++ % ArrayCount(NodeQueue)] = Node->Left;
					NodeQueue[OnePastLastNode++ % ArrayCount(NodeQueue)] = Node->Right;
				}
			}
		}
	}

	return Result;
}

#if 0
internal rigid_body *
AABBQueryPoint(physics_state *State, v3 Point)
{
	rigid_body *Result = 0;
	temporary_memory TempMem = BeginTemporaryMemory(&State->TempRegion);
	aabb_query_result QueryResult = AABBQuery(State, Rectangle3(Point, Point));
	for(u32 ResultIndex = 0;
		ResultIndex < QueryResult.Count;
		++ResultIndex)
	{
		rigid_body *TestBody = State->RigidBodies + QueryResult.BodyIndices[ResultIndex];
		if(RigidBodyContainsPoint(TestBody, Point))
		{
			Result = TestBody;
			break;
		}
	}

	EndTemporaryMemory(&TempMem);
	return Result;
}
#endif

internal ray_cast_result
RayCastAABB(rectangle3 AABB, v3 Origin, v3 Direction)
{
	TIMED_FUNCTION();

	ray_cast_result Result = {};
	Result.t = Real32Maximum;

	v3 FrontNormal = V3(0,0,1);
	v3 TopNormal = V3(0,1,0);
	v3 RightNormal = V3(1,0,0);

	f32 FrontDenom = Inner(FrontNormal, Direction);
	if(FrontDenom != 0.0f)
	{
		f32 tFront = Inner(FrontNormal, AABB.Max - Origin)/FrontDenom;
		v3 FrontHitPoint = Origin + tFront*Direction;
		b32 HitFront = ((FrontHitPoint.x > AABB.Min.x) && (FrontHitPoint.x < AABB.Max.x) &&
						(FrontHitPoint.y > AABB.Min.y) && (FrontHitPoint.y < AABB.Max.y));
		if(HitFront && (tFront < Result.t) && (tFront > 0.0f))
		{
			Result.t = tFront;
			Result.HitPoint = FrontHitPoint;
			Result.RayHit = true;
		}

		f32 tBack = Inner(FrontNormal, AABB.Min - Origin)/FrontDenom;
		v3 BackHitPoint = Origin + tBack*Direction;
		b32 HitBack = ((BackHitPoint.x > AABB.Min.x) && (BackHitPoint.x < AABB.Max.x) &&
						(BackHitPoint.y > AABB.Min.y) && (BackHitPoint.y < AABB.Max.y));
		if(HitBack && (tBack < Result.t) && (tBack > 0.0f))
		{
			Result.t = tBack;
			Result.HitPoint = BackHitPoint;
			Result.RayHit = true;
		}
	}

	f32 TopDenom = Inner(TopNormal, Direction);
	if(TopDenom != 0.0f)
	{
		f32 tTop = Inner(TopNormal, AABB.Max - Origin)/TopDenom;
		v3 TopHitPoint = Origin + tTop*Direction;
		b32 HitTop = ((TopHitPoint.x > AABB.Min.x) && (TopHitPoint.x < AABB.Max.x) &&
						(TopHitPoint.z > AABB.Min.z) && (TopHitPoint.z < AABB.Max.z));
		if(HitTop && (tTop < Result.t) && (tTop > 0.0f))
		{
			Result.t = tTop;
			Result.HitPoint = TopHitPoint;
			Result.RayHit = true;
		}

		f32 tBottom = Inner(TopNormal, AABB.Min - Origin)/TopDenom;
		v3 BottomHitPoint = Origin + tBottom*Direction;
		b32 HitBottom = ((BottomHitPoint.x > AABB.Min.x) && (BottomHitPoint.x < AABB.Max.x) &&
						(BottomHitPoint.z > AABB.Min.z) && (BottomHitPoint.z < AABB.Max.z));
		if(HitBottom && (tBottom < Result.t) && (tBottom > 0.0f))
		{
			Result.t = tBottom;
			Result.HitPoint = BottomHitPoint;
			Result.RayHit = true;
		}
	}

	f32 RightDenom = Inner(RightNormal, Direction);
	if(RightDenom != 0.0f)
	{
		f32 tRight = Inner(RightNormal, AABB.Max - Origin)/RightDenom;
		v3 RightHitPoint = Origin + tRight*Direction;
		b32 HitRight = ((RightHitPoint.z > AABB.Min.z) && (RightHitPoint.z < AABB.Max.z) &&
						(RightHitPoint.y > AABB.Min.y) && (RightHitPoint.y < AABB.Max.y));
		if(HitRight && (tRight < Result.t) && (tRight > 0.0f))
		{
			Result.t = tRight;
			Result.HitPoint = RightHitPoint;
			Result.RayHit = true;
		}

		f32 tLeft = Inner(RightNormal, AABB.Min - Origin)/RightDenom;
		v3 LeftHitPoint = Origin + tLeft*Direction;
		b32 HitLeft = ((LeftHitPoint.z > AABB.Min.z) && (LeftHitPoint.z < AABB.Max.z) &&
						(LeftHitPoint.y > AABB.Min.y) && (LeftHitPoint.y < AABB.Max.y));
		if(HitLeft && (tLeft < Result.t) && (tLeft > 0.0f))
		{
			Result.t = tLeft;
			Result.HitPoint = LeftHitPoint;
			Result.RayHit = true;
		}
	}

	return Result;
}

internal ray_cast_result
RayCastEntity(entity *Entity, v3 Origin, v3 Direction)
{
	ray_cast_result Result = {};
	rigid_body *Body = &Entity->RigidBody;
	Result.t = Real32Maximum;

	v3 LocalOrigin = WorldPointToLocalPoint(Entity, Origin);
	v3 LocalDirection = WorldVectorToLocalVector(Entity, Direction);

	for(collider *Collider = Body->ColliderSentinel.Next;
		Collider != &Body->ColliderSentinel;
		Collider = Collider->Next)
	{
		shape *Shape = &Collider->Shape;
		switch(Shape->Type)
		{
			case Shape_Sphere:
			{
				sphere *Sphere = &Shape->Sphere;
				v3 RelativeLocalOrigin = LocalOrigin - Sphere->Center;
				f32 A = Inner(LocalDirection, LocalDirection);
				f32 B = 2.0f*Inner(LocalDirection, RelativeLocalOrigin);
				f32 C = Inner(RelativeLocalOrigin, RelativeLocalOrigin) - Square(Sphere->Radius);
				f32 Determinant = Square(B) - 4.0f*A*C;
				if(Determinant >= 0.0f)
				{
					f32 tPos = (-B + SquareRoot(Determinant))/(2.0f*A);
					f32 tNeg = (-B - SquareRoot(Determinant))/(2.0f*A);
					if(tPos <= 0.0f) {tPos = Real32Maximum;}
					if(tNeg <= 0.0f) {tNeg = Real32Maximum;}
					f32 t = Minimum(tPos, tNeg);

					if(t < Result.t)
					{
						Result.t = t;
						Result.HitPoint = Origin + Result.t*Direction;
						Result.EntityHit = Entity;
						Result.RayHit = true;
					}
				}
			} break;

			case Shape_Capsule:
			{
				capsule *Capsule = &Shape->Capsule;

				{
					v3 RelativeLocalOrigin = LocalOrigin - Capsule->P0;
					f32 A = Inner(LocalDirection, LocalDirection);
					f32 B = 2.0f*Inner(LocalDirection, RelativeLocalOrigin);
					f32 C = Inner(RelativeLocalOrigin, RelativeLocalOrigin) - Square(Capsule->Radius);
					f32 Determinant = Square(B) - 4.0f*A*C;
					if(Determinant >= 0.0f)
					{
						f32 tPos = (-B + SquareRoot(Determinant))/(2.0f*A);
						f32 tNeg = (-B - SquareRoot(Determinant))/(2.0f*A);
						if(tPos <= 0.0f) {tPos = Real32Maximum;}
						if(tNeg <= 0.0f) {tNeg = Real32Maximum;}
						f32 t = Minimum(tPos, tNeg);

						if(t < Result.t)
						{
							Result.t = t;
							Result.HitPoint = Origin + Result.t*Direction;
							Result.EntityHit = Entity;
							Result.RayHit = true;
						}
					}
				}

				{
					v3 RelativeLocalOrigin = LocalOrigin - Capsule->P1;
					f32 A = Inner(LocalDirection, LocalDirection);
					f32 B = 2.0f*Inner(LocalDirection, RelativeLocalOrigin);
					f32 C = Inner(RelativeLocalOrigin, RelativeLocalOrigin) - Square(Capsule->Radius);
					f32 Determinant = Square(B) - 4.0f*A*C;
					if(Determinant >= 0.0f)
					{
						f32 tPos = (-B + SquareRoot(Determinant))/(2.0f*A);
						f32 tNeg = (-B - SquareRoot(Determinant))/(2.0f*A);
						if(tPos <= 0.0f) {tPos = Real32Maximum;}
						if(tNeg <= 0.0f) {tNeg = Real32Maximum;}
						f32 t = Minimum(tPos, tNeg);

						if(t < Result.t)
						{
							Result.t = t;
							Result.HitPoint = Origin + Result.t*Direction;
							Result.EntityHit = Entity;
							Result.RayHit = true;
						}
					}
				}

				{
					v3 O = LocalOrigin - Capsule->P0;
					v3 D = LocalDirection;
					v3 L = NOZ(Capsule->P1 - Capsule->P0);
					f32 DdotD = Inner(D, D);
					f32 OdotO = Inner(O, O);
					f32 LdotL = Inner(L, L);
					f32 OdotD = Inner(O, D);
					f32 DdotL = Inner(D, L);
					f32 OdotL = Inner(O, L);
					f32 A = DdotD + Square(DdotL)*(LdotL - 2.0f);
					f32 B = 2.0f*(OdotD - DdotL*OdotL - OdotL*DdotL*(1.0f - LdotL));
					f32 C = OdotO + Square(OdotL)*LdotL - 2.0f*Square(OdotL) - Square(Capsule->Radius);
					f32 Determinant = Square(B) - 4.0f*A*C;
					if(Determinant >= 0.0f)
					{
						f32 tPos = (-B + SquareRoot(Determinant))/(2.0f*A);
						f32 tNeg = (-B - SquareRoot(Determinant))/(2.0f*A);
						if(tPos <= 0.0f) {tPos = Real32Maximum;}
						if(tNeg <= 0.0f) {tNeg = Real32Maximum;}
						f32 t = Minimum(tPos, tNeg);

						if(t < Result.t)
						{
							v3 RelativeLocalHitPoint = O + t*D;
							v3 Projection = Inner(RelativeLocalHitPoint, L)*L;
							f32 ProjLengthSq = LengthSq(Projection);
							f32 HeightSq = LengthSq(Capsule->P1 - Capsule->P0);
							if((ProjLengthSq < HeightSq) &&
								SameDirection(Projection, Capsule->P1 - Capsule->P0))
							{
								Result.t = t;
								Result.HitPoint = Origin + Result.t*Direction;
								Result.EntityHit = Entity;
								Result.RayHit = true;
							}
						}
					}
				}
			} break;

			case Shape_ConvexHull:
			{
				convex_hull *Hull = Shape->ConvexHull;

				v3 TrimRay[2] = {LocalOrigin, LocalOrigin + LocalDirection};
				b32 IsInfinite = true;

				b32 RayHit = true;
				for(u32 Index = 0;
					RayHit && (Index < Hull->FaceCount);
					++Index)
				{
					face *Face = Hull->Faces + Index;
					f32 Denominator = Inner(TrimRay[0] - Face->Center, Face->Normal) - Inner(TrimRay[1] - Face->Center, Face->Normal);
					if(Denominator == 0.0f)
					{
						if(Inner(TrimRay[0]- Face->Center, Face->Normal) > 0.0f)
						{
							RayHit = false;
						}
					}
					else
					{
						f32 tIntersection = Inner(TrimRay[0] - Face->Center, Face->Normal)/Denominator;
						v3 Intersection = Lerp(TrimRay[0], tIntersection, TrimRay[1]);

						if(SameDirection(Intersection - TrimRay[0], LocalDirection))
						{
							if(IsInfinite)
							{
								if(SameDirection(Face->Normal, LocalDirection))
								{
									TrimRay[1] = Intersection;
									IsInfinite = false;
								}
								else
								{
									TrimRay[0] = Intersection;
									TrimRay[1] = Intersection + LocalDirection;
								}
							}
							else
							{
								if(SameDirection(Intersection - TrimRay[1], LocalDirection))
								{
									if(SameDirection(Face->Normal, LocalDirection))
									{
										// NOTE: No trim.
									}
									else
									{
										RayHit = false;
									}
								}
								else
								{
									if(SameDirection(Face->Normal, LocalDirection))
									{
										TrimRay[1] = Intersection;
									}
									else
									{
										TrimRay[0] = Intersection;
									}
								}
							}
						}
						else
						{
							if(SameDirection(Face->Normal, LocalDirection))
							{
								RayHit = false;
							}
							else
							{
								// NOTE: No trim.
							}
						}
					}
				}

				if(RayHit)
				{
					v3 LocalHitPoint = TrimRay[0];
					if(TrimRay[0] == LocalOrigin)
					{
						LocalHitPoint = TrimRay[1];
					}

					f32 t = Inner(LocalHitPoint - LocalOrigin, LocalDirection)/Inner(LocalDirection, LocalDirection);
					if(t < Result.t)
					{
						Result.t = t;
						Result.HitPoint = LocalPointToWorldPoint(Entity, LocalHitPoint);
						Result.EntityHit = Entity;
						Result.RayHit = true;
					}
				}
			} break;

			InvalidDefaultCase;
		}
	}

	return Result;
}

internal ray_cast_result
AABBRayCastQuery(aabb_tree *Tree, v3 Origin, v3 Direction)
{
	ray_cast_result Result = {};
	Result.t = Real32Maximum;

	u32 FirstNode = 0;
	u32 OnePastLastNode = 0;
	aabb_tree_node *NodeQueue[256];
	NodeQueue[OnePastLastNode++] = Tree->Root;

	while(FirstNode < OnePastLastNode)
	{
		aabb_tree_node *Node = NodeQueue[FirstNode++ % ArrayCount(NodeQueue)];

		if(Node)
		{
			if(NodeIsLeaf(Node))
			{
				switch(Node->Data.Type)
				{
					case AABBType_Entity:
					{
						entity *Entity = Node->Data.Entity;
						ray_cast_result RayCast = RayCastEntity(Entity, Origin, Direction);
						if(RayCast.RayHit && (RayCast.t < Result.t))
						{
							Result = RayCast;
						}
					} break;

					case AABBType_Triangle:
					{
						static_triangle *Triangle = Node->Data.Triangle;
						f32 Denom = Inner(Direction, Triangle->Normal);
						if(Denom != 0.0f)
						{
							f32 tPlane = Inner(Triangle->Points[0] - Origin, Triangle->Normal)/Denom;
							if(tPlane > 0.0f && tPlane < Result.t)
							{
								v3 TestPoint = Origin + tPlane*Direction;
								v3 BaryCoords = TriangleBarycentricCoords(TestPoint, Triangle->Points[0], Triangle->Points[1], Triangle->Points[2]);
								if((BaryCoords.x >= 0.0f) && (BaryCoords.y >= 0.0f) && (BaryCoords.z >= 0.0f))
								{
									Result.t = tPlane;
									Result.HitPoint = TestPoint;
									Result.RayHit = true;
									Result.HitNormal = Triangle->Normal;
								}
							}
						}
					} break;

					InvalidDefaultCase;
				}
			}
			else
			{
				ray_cast_result RayCast = RayCastAABB(Node->AABB, Origin, Direction);
				if(RayCast.RayHit && (RayCast.t <= Result.t))
				{
					Assert((OnePastLastNode - FirstNode + 2) < ArrayCount(NodeQueue));
					NodeQueue[OnePastLastNode++ % ArrayCount(NodeQueue)] = Node->Left;
					NodeQueue[OnePastLastNode++ % ArrayCount(NodeQueue)] = Node->Right;
				}
			}
		}
	}

	return Result;
}

internal ray_cast_result
AABBHorizontalLineCastQuery(aabb_tree *Tree, v3 Origin, v3 Direction, f32 Length)
{
	v3 Normal = Normalize(Direction);
	v3 Up = V3(0,1,0);
	v3 XAxisDirection = Cross(Normal, Up);
	if(LengthSq(XAxisDirection) == 0.0f)
	{
		Up = V3(1,0,0);
		XAxisDirection = Cross(Normal, Up);
	}
	v3 XAxis = Normalize(XAxisDirection);
	f32 HalfLength = 0.5f*Length;

	ray_cast_result Result = AABBRayCastQuery(Tree, Origin - HalfLength*XAxis, Direction);
	ray_cast_result Query1 = AABBRayCastQuery(Tree, Origin + HalfLength*XAxis, Direction);
	if(Query1.t < Result.t)
	{
		Result = Query1;
	}

	return Result;
}

internal void
DEBUGDisplayAABBTreeRegionsRecursive(render_group *Group, aabb_tree_node *Node, u32 Depth = 0)
{
	b32 IsLeaf = NodeIsLeaf(Node);
	v4 Color = IsLeaf ? V4(0.1f, 0.1f, 0.8f, 1.0f) : V4(0.8f, 0.1f, 0.1f, 1.0f);
	PushRectangle3Outline(Group, Node->AABB.Min, Node->AABB.Max, Color);

	if(!IsLeaf)
	{
		DEBUGDisplayAABBTreeRegionsRecursive(Group, Node->Left,  Depth + 1);
		DEBUGDisplayAABBTreeRegionsRecursive(Group, Node->Right, Depth + 1);
	}
}

internal void
DEBUGDisplayDynamicAABBTreeRegions(physics_state *State, render_group *Group)
{
	if(State->DisplayDynamicAABB)
	{
		aabb_tree *Tree = &State->DynamicAABBTree;
		if(Tree->Root)
		{
			DEBUGDisplayAABBTreeRegionsRecursive(Group, Tree->Root);
		}
	}
}

internal void
DEBUGDisplayStaticAABBTreeRegions(physics_state *State, render_group *Group)
{
	if(State->DisplayStaticAABB)
	{
		aabb_tree *Tree = &State->StaticAABBTree;
		if(Tree->Root)
		{
			DEBUGDisplayAABBTreeRegionsRecursive(Group, Tree->Root);
		}
	}
}

internal void
DEBUGDisplayAABBTreeDiagramRecursive(render_group *Group, aabb_tree_node *Node,	u32 Depth, v2 ParentDiagramP, v2 TotalDimension, b32 IsLeft = false)
{
	b32 IsLeaf = NodeIsLeaf(Node);
	v2 NodeDiagramP = ParentDiagramP;
	f32 TotalDiagramWidth = TotalDimension.x;
	f32 TotalDiagramHeight = TotalDimension.y;
	f32 Height = TotalDiagramHeight/(1 << (Depth));
	f32 Width = TotalDiagramWidth/(1 << (Depth+1));
	v2 NodeDim = 0.1f*Width*V2(1.0f, 1.0f);
	if(Depth > 0)
	{
		NodeDiagramP = ParentDiagramP + V2(IsLeft ? -Width : Width, -Height);
	}

	PushLine(Group, ParentDiagramP, NodeDiagramP, 1.0f, V4(0.3f, 0.3f, 0.3f, 1.0f));
	PushQuad(Group, NodeDiagramP, NodeDim, RotationMat2(0.0f), V4(0.8f, 0.2f, 0.2f, 1));

	if(!IsLeaf)
	{
		DEBUGDisplayAABBTreeDiagramRecursive(Group, Node->Left,  Depth + 1, NodeDiagramP, TotalDimension, true);
		DEBUGDisplayAABBTreeDiagramRecursive(Group, Node->Right, Depth + 1, NodeDiagramP, TotalDimension, false);
	}
}

internal void
DEBUGDisplayAABBTreeDiagram(aabb_tree *Tree, render_group *Group, v2 Min, v2 Max)
{
	v2 Dimension = Max - Min;
	v2 RootP = V2(Min.x + 0.5f*Dimension.x, Max.y);

	f32 BorderWidth = 2.0f;
	PushRectangleOutline(Group, Min, Max, V4(0.75f, 0.75f, 0.75f, 0.75f), BorderWidth);

	TemporaryClipRectChange(Group, {V2ToV2i(Min), V2ToV2i(Max)});
	PushQuad(Group, Min, Max, V4(0.1f, 0.1f, 0.1f, 0.5f));

	if(Tree->Root)
	{
		DEBUGDisplayAABBTreeDiagramRecursive(Group, Tree->Root, 0, RootP, Dimension);
	}
}
