/*@H
* File: steelth_animation.h
* Author: Jesse Calvert
* Created: June 16, 2018, 14:16
* Last modified: April 10, 2019, 17:31
*/

#pragma once

enum animation_type
{
	Animation_None,
	Animation_Interp,
	Animation_Spring,
	Animation_Physics,
	Animation_Wait,
	Animation_Shake,

	Animation_Count,
};

struct spring_animation
{
	v3 TargetP;
	quaternion TargetRotation;

	f32 OscillationFrequency;
	f32 OscillationReductionPerSecond;
};

enum interp_type
{
	Interp_Linear,
	Interp_Cubic,
};

struct interp_animation
{
	v3 StartP;
	v3 EndP;
	quaternion StartRotation;
	quaternion EndRotation;

	interp_type Interp;
	f32 Duration;
	f32 tAccum;
};

struct wait_animation
{
	f32 Duration;
	f32 tAccum;
};

struct random_shake_animation
{
	v3 Base;
	v3 Extent;

	f32 Duration;
	f32 tAccum;
};

struct animation
{
	animation_type Type;
	union
	{
		spring_animation SpringAnim;
		interp_animation InterpAnim;
		wait_animation WaitAnim;
		random_shake_animation ShakeAnim;
	};

	animation *Next;
};

struct animator
{
	memory_region *Region;
	physics_state *PhysicsState;
	animation *FirstFreeAnimation;
};
