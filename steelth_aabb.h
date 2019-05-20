/*@H
* File: steelth_aabb.h
* Author: Jesse Calvert
* Created: January 3, 2018, 16:14
* Last modified: April 10, 2019, 17:31
*/

#pragma once

enum aabb_data_type
{
	AABBType_None,
	AABBType_Entity,
	AABBType_Triangle,
	AABBType_Count,
};
struct aabb_data
{
	aabb_data_type Type;
	union
	{
		struct entity *Entity;
		struct static_triangle *Triangle;
		void *VoidPtr;
	};
};

struct aabb_query_result
{
	u32 Count;
	aabb_data *Data;
};

struct aabb_tree_node
{
	// TODO: These pointers could be indicies
	aabb_tree_node *Parent;
	union
	{
		aabb_tree_node *Left;
		aabb_tree_node *NextInFreeList;
	};
	union
	{
		aabb_tree_node *Right;
	};

	u32 Height;
	rectangle3 AABB;
	aabb_data Data;
};
inline b32
NodeIsLeaf(aabb_tree_node *Node)
{
	b32 Result = (Node->Left == 0);
	return Result;
}

struct aabb_tree
{
	memory_region *Region;
	aabb_tree_node *NodeFreeList;

	aabb_tree_node *Root;
};

struct ray_cast_result
{
	f32 t;
	v3 HitPoint;
	b32 RayHit;

	struct entity *EntityHit;

	// NOTE: Only calculated for triangles
	v3 HitNormal;
};
