/*@H
* File: steelth_rigid_body.h
* Author: Jesse Calvert
* Created: October 6, 2017, 16:48
* Last modified: April 10, 2019, 17:31
*/

#pragma once

struct capsule
{
	v3 P0, P1;
	f32 Radius;
};

INTROSPECT()
enum shape_type
{
	Shape_None,

	Shape_Sphere,
	Shape_Capsule,
	Shape_ConvexHull,

	Shape_Count,
};

INTROSPECT()
struct shape
{
	shape_type Type;
	union
	{
		sphere Sphere;
		capsule Capsule;
		struct convex_hull *ConvexHull;
	};
};

INTROSPECT()
struct collider
{
	collider *Next;
	collider *Prev;

	f32 Mass;
	mat3 LocalInertialTensor;
	v3 LocalCentroid;

	shape Shape;
};

struct move_spec
{
	f32 StaticFriction;
	f32 KineticFriction;
	f32 LinearAirFriction;
	f32 RotationAirFriction;

	f32 MaxSpeed;
};

INTROSPECT()
struct rigid_body
{
	b32 OnSolidGround;

	b32 Collideable;
	move_spec MoveSpec;

	f32 InvMass;
	mat3 LocalInvInertialTensor;

	v3 WorldCentroid;
	v3 LocalCentroid;

	collider ColliderSentinel;

	struct aabb_tree_node *AABBNode;

	b32 IsInitialized;
};

struct pointer_pair
{
	void *A;
	void *B;
};
inline b32
operator==(pointer_pair First, pointer_pair Second)
{
	b32 Result = (((First.A == Second.A) && (First.B == Second.B)) ||
		((First.A == Second.B) && (First.B == Second.A)));
	return Result;
}
inline b32
operator!=(pointer_pair A, pointer_pair B)
{
	b32 Result = !(A == B);
	return Result;
}

enum feature_type
{
	Feature_None,
	Feature_Vertex,
	Feature_Edge,
	Feature_Face,
	Feature_Count,
};
struct feature_tag
{
	feature_type Type;
	u32 Index[2];
};

struct collision
{
	entity *EntityA;
	collider *ColliderA;

	entity *EntityB;
	collider *ColliderB;
};
inline b32
operator==(collision A, collision B)
{
	b32 Result = (((A.EntityA == B.EntityA) && (A.EntityB == B.EntityB) && (A.ColliderA == B.ColliderA) && (A.ColliderB == B.ColliderB)) ||
		((A.EntityA == B.EntityB) && (A.EntityB == B.EntityA) && (A.ColliderA == B.ColliderB) && (A.ColliderB == B.ColliderA)));
	return Result;
}
inline b32
operator!=(collision A, collision B)
{
	b32 Result = !(A == B);
	return Result;
}

struct manifold_point
{
	v3 P;
	f32 Penetration;

	u32 EdgeIndexes[2];
};

inline b32
operator==(manifold_point A, manifold_point B)
{
	b32 Result = ((A.P == B.P) && (A.Penetration == B.Penetration));
	return Result;
}

struct contact_data
{
	v3 WorldContactPointA;
	v3 WorldContactPointB;

	v3 LocalContactPointA;
	v3 LocalContactPointB;

	v3 Normal; // NOTE: Points from A to B.
	v3 Tangent0;
	v3 Tangent1;

	f32 Penetration;

	v3 InitialRelativeVelocity;

	f32 NormalImpulseSum;
	f32 Tangent0ImpulseSum;
	f32 Tangent1ImpulseSum;

	feature_tag FeatureA;
	feature_tag FeatureB;

	b32 Persistent;
	u32 PersistentCount;
};

struct contact_manifold
{
	contact_manifold *Next;
	contact_manifold *Prev;
	contact_manifold *NextInHash;

	collision Collision;

	u32 ContactCount;
	contact_data Contacts[4];

	feature_type FeatureTypeA;
	feature_type FeatureTypeB;

	v3 SeparatingAxis;
	u32 FramesSinceRead;
};

struct distance_joint
{
	entity *EntityA;
	entity *EntityB;

	v3 LocalAnchorA;
	v3 LocalAnchorB;
	f32 Distance;

	f32 AngularFrequency;
	f32 DampingRatio;

	f32 ImpulseSum;

	b32 Collideable;
};

struct controller_joint
{
	entity *Entity;
	v3 LinearMotor;
	v3 IgnoredAxis;

	f32 MaxImpulse;
	v3 ImpulseSum;
};

enum joint_type
{
	Joint_Distance,
	Joint_Controller,

	Joint_Count,
};
struct joint
{
	union
	{
		distance_joint DistanceJoint;
		controller_joint ControllerJoint;
	};

	joint *Next;
	joint *Prev;

	joint_type Type;
};

struct static_triangle
{
	v3 Points[3];
	v3 Normal;
	collider Collider;
};

struct static_collision
{
	entity *Entity;
	static_triangle *Triangle;
};

#define BUCKET_SIZE 64
struct bucket
{
	u32 PotentialCollisionCount;
	pointer_pair *PotentialCollisions;
};

#define MAX_RIGID_BODY_COUNT 1024
#define MAX_BUCKETS	64
struct physics_state
{
	memory_region *Region;

	u32 RigidBodyCount;
	entity *StaticsDummy;

	collider *FirstFreeCollider;

	u32 BucketCount;
	bucket PotentialCollisionBuckets[MAX_BUCKETS];

	u32 StaticCollisionCount;
	static_collision StaticCollisions[4096];

	contact_manifold *ContactManifoldHashTable[1024];
	contact_manifold *ManifoldFreeList;
	contact_manifold ManifoldSentinel;

	joint JointSentinel;
	joint *FirstFreeJoint;

	u32 UncollideableCount;
	pointer_pair Uncollideables[128];

	aabb_tree DynamicAABBTree;
	aabb_tree StaticAABBTree;

	f32 Baumgarte;
	f32 BaumgarteSlop;
	f32 RestitutionCoeff;
	f32 RestitutionSlop;
	f32 PersistentThreshold;
	u32 IterationCount;

	b32 GravityMode;
	b32 DisplayStaticAABB;
	b32 DisplayDynamicAABB;
	b32 DisplayContacts;
	b32 DisplayContactNormals;
	b32 DisplayImpulses;
	b32 DisplayJoints;
};
