/*@H
* File: steelth_particles.h
* Author: Jesse Calvert
* Created: June 14, 2018, 17:51
* Last modified: April 22, 2019, 17:33
*/

#pragma once

#define MAX_PARTICLES 1024
#define MAX_PARTICLES_4 (MAX_PARTICLES/4)

struct particle_4x
{
	v3_4x P;
	v3_4x dP;
	v3_4x ddP;

	v4_4x Color;
	v4_4x dColor;

	f32 GroundY;
	u32 Pad[3];
};

struct particle_emitter
{
	particle_4x Particles[MAX_PARTICLES_4];
	u32 NextParticle;

	v2 Dim;
	bitmap_id BitmapID;
	f32 Roughness;
	f32 Metalness;
	f32 Emission;
};

enum emitter_type
{
	Emitter_Fountain,
	Emitter_Fire,

	Emitter_Count,
};

struct particle_system
{
	particle_emitter Emitters[Emitter_Count];

	random_series Series;
};
