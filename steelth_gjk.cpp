/*@H
* File: steelth_gjk.cpp
* Author: Jesse Calvert
* Created: February 9, 2018, 12:11
* Last modified: April 10, 2019, 17:31
*/

internal minkowski_difference_support
MinkowskiDifferenceSupport(entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB,
	 v3 Direction, f32 Scale = 1.0f)
{
	minkowski_difference_support Result = {};

	Result.SupportA = Support(EntityA, ColliderA, Direction, Scale);
	Result.SupportB = Support(EntityB, ColliderB, -Direction, Scale);
	Result.Support = Result.SupportA - Result.SupportB;

	return Result;
}

internal b32
PointIsInSimplex(minkowski_simplex *Simplex, minkowski_difference_support NewSupport)
{
	b32 Result = false;
	for(u32 Index = 0;
		Index < Simplex->Size;
		++Index)
	{
		minkowski_difference_support *Support = Simplex->Points + Index;
		if(Support->Support == NewSupport.Support)
		{
			Result = true;
			break;
		}
	}

	return Result;
}

internal b32
SimplexIsNotDegenerate(minkowski_simplex *Simplex)
{
	b32 Result = true;
	for(u32 Index = 0;
		Result && (Index < Simplex->Size);
		++Index)
	{
		for(u32 IndexB = Index + 1;
			Result && (IndexB < Simplex->Size);
			++IndexB)
		{
			v3 A = Simplex->Points[Index].Support;
			v3 B = Simplex->Points[IndexB].Support;
			if(A == B)
			{
				Result = false;
			}
		}
	}

	return Result;
}

internal gjk_result
GJK(entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB,
	f32 Scale = 1.0f)
{
	TIMED_FUNCTION();

	rigid_body *BodyA = &EntityA->RigidBody;
	rigid_body *BodyB = &EntityB->RigidBody;

	f32 Epsilon = 0.0001f;
	f32 EpsilonSq = Square(Epsilon);

	gjk_result Result = {};
	minkowski_simplex *Simplex = &Result.Simplex;

	v3 InitialDirection = BodyB->WorldCentroid - BodyA->WorldCentroid;
	if(LengthSq(InitialDirection) == 0.0f)
	{
		InitialDirection = V3(1.0f, 0.0f, 0.0f);
	}
	minkowski_difference_support MinkowskiSupport = MinkowskiDifferenceSupport(EntityA, ColliderA, EntityB, ColliderB, InitialDirection, Scale);
	Simplex->Points[Simplex->Size++] = MinkowskiSupport;

	f32 DistanceSq = Real32Maximum;

	while(1)
	{
		v3 NextDirection = {};

		// @Speed: if we make sure to always add the new point in the direction of the origin
		// to the end of the simplex (which we are), then there are a lot of cases here that
		// we don't need to be checking. (Like in case 2, there should be no way that [A] is the
		// new simplex when we just added B in the previous step)
		switch(Simplex->Size)
		{
			case 1:
			{
				// NOTE: [A]
				NextDirection = -Simplex->Points[0].Support;
				Result.ClosestPoint = Simplex->Points[0];
			} break;

			case 2:
			{
				v3 A = Simplex->Points[0].Support;
				v3 B = Simplex->Points[1].Support;
				v2 uvAB = SegmentBarycentricCoords(V3(0,0,0), A, B);
				if(uvAB.x <= 0.0f)
				{
					// NOTE: [B]
					Result.ClosestPoint = Simplex->Points[1];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[1];
					NextDirection = -B;
				}
				else if(uvAB.y <= 0.0f)
				{
					// NOTE: [A]
					Result.ClosestPoint = Simplex->Points[0];
					Simplex->Size = 1;
					NextDirection = -A;
				}
				else
				{
					// NOTE: [A, B]
					Result.ClosestPoint = uvAB.x*Simplex->Points[0] + uvAB.y*Simplex->Points[1];
					NextDirection = Cross(Cross(B-A, -A), B-A);
				}
			} break;

			case 3:
			{
				v3 A = Simplex->Points[0].Support;
				v3 B = Simplex->Points[1].Support;
				v3 C = Simplex->Points[2].Support;

				v2 uvAB = SegmentBarycentricCoords(V3(0,0,0), A, B);
				v2 uvBC = SegmentBarycentricCoords(V3(0,0,0), B, C);
				v2 uvCA = SegmentBarycentricCoords(V3(0,0,0), C, A);

				if(uvCA.x <= 0.0f && uvAB.y <= 0.0f )
				{
					// NOTE: [A]
					Result.ClosestPoint = Simplex->Points[0];
					Simplex->Size = 1;
					NextDirection = -A;
				}
				else if(uvAB.x <= 0.0f && uvBC.y <= 0.0f)
				{
					// NOTE: [B]
					Result.ClosestPoint = Simplex->Points[1];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[1];
					NextDirection = -B;
				}
				else if(uvBC.x <= 0.0f && uvCA.y <= 0.0f)
				{
					// NOTE: [C]
					Result.ClosestPoint = Simplex->Points[2];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[2];
					NextDirection = -C;
				}
				else
				{
					v3 uvwABC = TriangleBarycentricCoords(V3(0,0,0), A, B, C);

					if(uvAB.x > 0.0f && uvAB.y > 0.0f && uvwABC.z <= 0.0f)
					{
						// NOTE: [A, B]
						Result.ClosestPoint = uvAB.x*Simplex->Points[0] + uvAB.y*Simplex->Points[1];
						Simplex->Size = 2;
						NextDirection = Cross(Cross(B-A, -A), B-A);
					}
					else if(uvBC.x > 0.0f && uvBC.y > 0.0f && uvwABC.x <= 0.0f)
					{
						// NOTE: [B, C]
						Result.ClosestPoint = uvBC.x*Simplex->Points[1] + uvBC.y*Simplex->Points[2];
						Simplex->Size = 2;
						Simplex->Points[0] = Simplex->Points[2];
						NextDirection = Cross(Cross(C-B, -B), C-B);
					}
					else if(uvCA.x > 0.0f && uvCA.y > 0.0f && uvwABC.y <= 0.0f)
					{
						// NOTE: [C, A]
						Result.ClosestPoint = uvCA.x*Simplex->Points[2] + uvCA.y*Simplex->Points[0];
						Simplex->Size = 2;
						Simplex->Points[1] = Simplex->Points[2];
						NextDirection = Cross(Cross(A-C, -C), A-C);
					}
					else
					{
						Assert(uvwABC.x > 0.0f && uvwABC.y > 0.0f && uvwABC.z > 0.0f);
						// NOTE: [A, B, C]
						Result.ClosestPoint = uvwABC.x*Simplex->Points[0] + uvwABC.y*Simplex->Points[1] + uvwABC.z*Simplex->Points[2];
						NextDirection = Cross(B-A, C-A);
						if(Inner(NextDirection, -A) < 0.0f) {NextDirection = -NextDirection;}
					}
				}
			} break;

			case 4:
			{
				v3 A = Simplex->Points[0].Support;
				v3 B = Simplex->Points[1].Support;
				v3 C = Simplex->Points[2].Support;
				v3 D = Simplex->Points[3].Support;

				v2 uvAB = SegmentBarycentricCoords(V3(0,0,0), A, B);
				v2 uvBC = SegmentBarycentricCoords(V3(0,0,0), B, C);
				v2 uvCA = SegmentBarycentricCoords(V3(0,0,0), C, A);
				v2 uvDA = SegmentBarycentricCoords(V3(0,0,0), D, A);
				v2 uvBD = SegmentBarycentricCoords(V3(0,0,0), B, D);
				v2 uvCD = SegmentBarycentricCoords(V3(0,0,0), C, D);

				if(uvCA.x <= 0.0f && uvAB.y <= 0.0f && uvDA.x < 0.0f)
				{
					// NOTE: [A]
					Result.ClosestPoint = Simplex->Points[0];
					Simplex->Size = 1;
					NextDirection = -A;
				}
				else if(uvAB.x <= 0.0f && uvBC.y <= 0.0f && uvBD.y < 0.0f)
				{
					// NOTE: [B]
					Result.ClosestPoint = Simplex->Points[1];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[1];
					NextDirection = -B;
				}
				else if(uvBC.x <= 0.0f && uvCA.y <= 0.0f && uvCD.y < 0.0f)
				{
					// NOTE: [C]
					Result.ClosestPoint = Simplex->Points[2];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[2];
					NextDirection = -C;
				}
				else if(uvDA.y <= 0.0f && uvBD.x <= 0.0f && uvCD.x <= 0.0f)
				{
					// NOTE: [D]
					Result.ClosestPoint = Simplex->Points[3];
					Simplex->Size = 1;
					Simplex->Points[0] = Simplex->Points[3];
					NextDirection = -D;
				}
				else
				{
					v3 uvwABC = TriangleBarycentricCoords(V3(0,0,0), A, B, C);
					v3 uvwABD = TriangleBarycentricCoords(V3(0,0,0), A, B, D);
					v3 uvwCDA = TriangleBarycentricCoords(V3(0,0,0), C, D, A);
					v3 uvwBCD = TriangleBarycentricCoords(V3(0,0,0), B, C, D);

					if(uvAB.x > 0.0f && uvAB.y > 0.0f && uvwABC.z <= 0.0f && uvwABD.z <= 0.0f)
					{
						// NOTE: [A, B]
						Result.ClosestPoint = uvAB.x*Simplex->Points[0] + uvAB.y*Simplex->Points[1];
						Simplex->Size = 2;
						NextDirection = Cross(Cross(B-A, -A), B-A);
					}
					else if(uvBC.x > 0.0f && uvBC.y > 0.0f && uvwABC.x <= 0.0f && uvwBCD.z <= 0.0f)
					{
						// NOTE: [B, C]
						Result.ClosestPoint = uvBC.x*Simplex->Points[1] + uvBC.y*Simplex->Points[2];
						Simplex->Size = 2;
						Simplex->Points[0] = Simplex->Points[2];
						NextDirection = Cross(Cross(C-B, -B), C-B);
					}
					else if(uvCA.x > 0.0f && uvCA.y > 0.0f && uvwABC.y <= 0.0f && uvwCDA.y <= 0.0f)
					{
						// NOTE: [C, A]
						Result.ClosestPoint = uvCA.x*Simplex->Points[2] + uvCA.y*Simplex->Points[0];
						Simplex->Size = 2;
						Simplex->Points[1] = Simplex->Points[2];
						NextDirection = Cross(Cross(A-C, -C), A-C);
					}
					else if(uvDA.x > 0.0f && uvDA.y > 0.0f && uvwABD.y <= 0.0f && uvwCDA.x <= 0.0f)
					{
						// NOTE: [D, A]
						Result.ClosestPoint = uvDA.x*Simplex->Points[3] + uvDA.y*Simplex->Points[0];
						Simplex->Size = 2;
						Simplex->Points[1] = Simplex->Points[3];
						NextDirection = Cross(Cross(A-D, -D), A-D);
					}
					else if(uvBD.x > 0.0f && uvBD.y > 0.0f && uvwABD.x <= 0.0f && uvwBCD.y <= 0.0f)
					{
						// NOTE: [B, D]
						Result.ClosestPoint = uvBD.x*Simplex->Points[1] + uvBD.y*Simplex->Points[3];
						Simplex->Size = 2;
						Simplex->Points[0] = Simplex->Points[3];
						NextDirection = Cross(Cross(D-B, -B), D-B);
					}
					else if(uvCD.x > 0.0f && uvCD.y > 0.0f && uvwCDA.z <= 0.0f && uvwBCD.x <= 0.0f)
					{
						// NOTE: [C, D]
						Result.ClosestPoint = uvCD.x*Simplex->Points[2] + uvCD.y*Simplex->Points[3];
						Simplex->Size = 2;
						Simplex->Points[0] = Simplex->Points[2];
						Simplex->Points[1] = Simplex->Points[3];
						NextDirection = Cross(Cross(D-C, -C), D-C);
					}
					else
					{
						v4 uvwtABCD = TetrahedronBarycentricCoords(V3(0,0,0), A, B, C, D);

						if(uvwBCD.x > 0.0f && uvwBCD.y > 0.0f && uvwBCD.z > 0.0f && uvwtABCD.x <= 0.0f)
						{
							// NOTE: [B, C, D]
							Result.ClosestPoint = uvwBCD.x*Simplex->Points[1] + uvwBCD.y*Simplex->Points[2] + uvwBCD.z*Simplex->Points[3];
							Simplex->Size = 3;
							Simplex->Points[0] = Simplex->Points[3];
							NextDirection = Cross(C-B, D-B);
							if(Inner(NextDirection, -B) < 0.0f) {NextDirection = -NextDirection;}
						}
						else if(uvwCDA.x > 0.0f && uvwCDA.y > 0.0f && uvwCDA.z > 0.0f&& uvwtABCD.y <= 0.0f)
						{
							// NOTE: [C, D, A]
							Result.ClosestPoint = uvwCDA.x*Simplex->Points[2] + uvwCDA.y*Simplex->Points[3] + uvwCDA.z*Simplex->Points[0];
							Simplex->Size = 3;
							Simplex->Points[1] = Simplex->Points[3];
							NextDirection = Cross(D-C, A-C);
							if(Inner(NextDirection, -C) < 0.0f) {NextDirection = -NextDirection;}
						}
						else if(uvwABD.x > 0.0f && uvwABD.y > 0.0f && uvwABD.z > 0.0f&& uvwtABCD.z <= 0.0f)
						{
							// NOTE: [A, B, D]
							Result.ClosestPoint = uvwABD.x*Simplex->Points[0] + uvwABD.y*Simplex->Points[1] + uvwABD.z*Simplex->Points[3];
							Simplex->Size = 3;
							Simplex->Points[2] = Simplex->Points[3];
							NextDirection = Cross(B-A, D-A);
							if(Inner(NextDirection, -A) < 0.0f) {NextDirection = -NextDirection;}
						}
						else if(uvwABC.x > 0.0f && uvwABC.y > 0.0f && uvwABC.z > 0.0f&& uvwtABCD.w <= 0.0f)
						{
							// NOTE: [A, B, C]
							Result.ClosestPoint = uvwABC.x*Simplex->Points[0] + uvwABC.y*Simplex->Points[1] + uvwABC.z*Simplex->Points[2];
							Simplex->Size = 3;
							NextDirection = Cross(B-A, C-A);
							if(Inner(NextDirection, -A) < 0.0f) {NextDirection = -NextDirection;}
						}
						else
						{
							Assert(uvwtABCD.x > 0.0f && uvwtABCD.y > 0.0f && uvwtABCD.z > 0.0f && uvwtABCD.w > 0.0f);
							// NOTE: [A, B, C, D]
							Result.ClosestPoint = uvwtABCD.x*Simplex->Points[0] + uvwtABCD.y*Simplex->Points[1] + uvwtABCD.z*Simplex->Points[2] + uvwtABCD.w*Simplex->Points[3];
							NextDirection = {};
						}
					}
				}
			} break;

			InvalidDefaultCase;
		}

		if(NextDirection == V3(0,0,0))
		{
			Result.ContainsOrigin = true;
			break;
		}

		f32 NewDistanceSq = LengthSq(Result.ClosestPoint.Support);
		if(NewDistanceSq >= (DistanceSq - Epsilon))
		{
			break;
		}

		DistanceSq = NewDistanceSq;

		minkowski_difference_support NewSupport = MinkowskiDifferenceSupport(EntityA, ColliderA, EntityB, ColliderB, NextDirection, Scale);

		v3 Q = NewSupport.Support;
		b32 DegenerateSimplex = false;
		switch(Simplex->Size)
		{
			case 1:
			{
				v3 A = Simplex->Points[0].Support;

				if(LengthSq(A-Q) < EpsilonSq)
				{
					DegenerateSimplex = true;
				}
			} break;

			case 2:
			{
				v3 A = Simplex->Points[0].Support;
				v3 B = Simplex->Points[1].Support;

				if(LengthSq(Cross(A-Q, B-Q)) < EpsilonSq)
				{
					DegenerateSimplex = true;
				}
			} break;

			case 3:
			{
				v3 A = Simplex->Points[0].Support;
				v3 B = Simplex->Points[1].Support;
				v3 C = Simplex->Points[2].Support;
				v3 N = Normalize(Cross(B-A, C-A));

				if(Square(Inner(Q-A, N)) < EpsilonSq)
				{
					DegenerateSimplex = true;
				}
			} break;
		}

		if(DegenerateSimplex)
		{
			break;
		}

		Assert(Simplex->Size < ArrayCount(Simplex->Points));
		Simplex->Points[Simplex->Size++] = NewSupport;
	}

	return Result;
}

internal b32
EntitiesIntersect(entity *EntityA, collider *ColliderA,
	entity *EntityB, collider *ColliderB,
	f32 Scale = 1.0f)
{
	gjk_result GJKResult = GJK(EntityA, ColliderA, EntityB, ColliderB, Scale);
	return GJKResult.ContainsOrigin;
}

internal b32
EntityContainsPoint(entity *Entity, v3 Point)
{
	b32 Result = false;
	rigid_body *Body = &Entity->RigidBody;

	collider PointCollider = {};
	PointCollider.Shape.Type = Shape_Sphere;

	entity Dummy = {};
	Dummy.P = Dummy.RigidBody.WorldCentroid = Point;
	Dummy.Rotation = Q(1,0,0,0);
	UpdateRotation(&Dummy);

	for(collider *Collider = Body->ColliderSentinel.Next;
		!Result && (Collider != &Body->ColliderSentinel);
		Collider = Collider->Next)
	{
		Result = EntitiesIntersect(Entity, Collider, &Dummy, &PointCollider);
	}

	return Result;
}
