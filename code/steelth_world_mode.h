/*@H
* File: steelth_world_mode.h
* Author: Jesse Calvert
* Created: May 21, 2018, 14:48
* Last modified: April 18, 2019, 14:30
*/

#pragma once

struct entity_list
{
	u32 EntityCount;
	entity_id EntityIDs[16];
};

struct ui_panel_positions
{
	v2i At;
	v2i Advance;
	v2i Dim;
	s32 Spacing;
	v2i LeftCenterP;
};

struct game_mode_world
{
	physics_state Physics;
	particle_system *ParticleSystem;
	animator Animator;
	struct entity_manager *EntityManager;

	random_series Entropy;

	f32 Pitch;
	f32 Yaw;
	camera WorldCamera;
	camera ScreenCamera;

	u32 DecalCount;
	oriented_box Decals[256];

	world World;
};

internal void PlayWorldMode(game_state *GameState);
