/*@H
* File: steelth_world.h
* Author: Jesse Calvert
* Created: April 18, 2019, 14:30
* Last modified: April 24, 2019, 17:08
*/

#pragma once

struct world
{
	u32 VertexCount;
	vertex_format *Vertices;
	u32 FaceCount;
	v3u *Faces;

	surface_material Material;
	mesh_id MeshID;

	u32 ProbeCount;
	v3 *ProbePositions;

	u32 LightCount;
	light *Lights;
	v3 SkyRadiance;

	lighting_solution LightingData;
	b32 DebugDisplayLightingData;
};
