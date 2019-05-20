/*@H
* File: steelth_convex_hull.h
* Author: Jesse Calvert
* Created: January 4, 2018, 20:39
* Last modified: January 29, 2018, 17:58
*/

#pragma once

struct qh_half_edge
{
	v3 Origin;
	qh_half_edge *Next;
	qh_half_edge *Twin;
	struct qh_face *Face;

	b32 Visited;

	s32 Index;
	s32 OriginIndex;
};

struct qh_conflict_vertex
{
	qh_conflict_vertex *Next;
	qh_conflict_vertex *Prev;

	v3 P;
	f32 Distance;
};

struct qh_face
{
	qh_face *Next;
	qh_face *Prev;

	qh_half_edge *FirstEdge;
	v3 Center;
	v3 Normal;

	b32 Visited;

	qh_conflict_vertex ConflictSentinel;

	s32 Index;
};
inline b32 FaceIsValid(qh_face  *Face)
{
	b32 Result = (Face->Prev != 0);
	return Result;
}

struct qh_half_edge_list_node
{
	qh_half_edge Edge;

	qh_half_edge_list_node *Next;
	qh_half_edge_list_node *Prev;
};

struct qh_convex_hull
{
	qh_half_edge_list_node EdgeSentinel;
	qh_face FaceSentinel;

	u32 VertexCount;
	u32 EdgeCount;
	u32 FaceCount;
};

struct half_edge
{
	u8 Origin;
	u8 Next;
	u8 Twin;
	u8 Face;
};

struct face
{
	u8 FirstEdge;
	v3 Normal;
	v3 Center;
};

#define CONVEX_HULL_MAX_VERTICES 256

INTROSPECT()
struct convex_hull
{
	u8 VertexCount;
	v3 *Vertices;

	// NOTE: Edges are stored with twins next to each other.
	u8 EdgeCount;
	half_edge *Edges;

	u8 FaceCount;
	face *Faces;
};

#define QH_SUPPORT(Name) v3 Name(v3 Direction)
typedef QH_SUPPORT(qh_support);
