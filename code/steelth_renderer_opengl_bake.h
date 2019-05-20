/*@H
* File: steelth_renderer_opengl_bake.h
* Author: Jesse Calvert
* Created: April 23, 2019, 16:16
* Last modified: April 24, 2019, 17:10
*/

#pragma once

struct surfel_link
{
	surfel_link	*Next;
	surfel_link	*Prev;

	surface_element *Surfel;
};

struct surfel_grid_cell
{
	surfel_grid_cell *NextInHash;

	v3i PositionIndex;
	u32 PrincipalNormal;

	surfel_link Sentinel;
	u32 Index;
};

struct light_probe_baking
{
	v3 P;
	f32 SkyVisibility[6];

	u32 SurfelCount;
	u32 *SurfelRefs;
};

struct opengl_baked_data
{
	v2i Resolution;
	v2i TotalResolution;
	opengl_framebuffer ProbeGBuffer;

	f32 GridCellDim;
	surfel_grid_cell *SurfelGridHash[4096];

	u32 MaxSurfelCells;
	u32 SurfelCellCount;
	surfel_grid_cell *SurfelCells;
};
