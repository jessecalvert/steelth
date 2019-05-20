/*@H
* File: steelth_animation.cpp
* Author: Jesse Calvert
* Created: June 16, 2018, 14:16
* Last modified: April 10, 2019, 17:31
*/

internal animation *
NewAnimation(animator *Animator)
{
	animation *Result = Animator->FirstFreeAnimation;
	if(Result)
	{
		Animator->FirstFreeAnimation = Result->Next;
		ZeroStruct(*Result);
	}
	else
	{
		Result = PushStruct(Animator->Region, animation);
	}

	return Result;
}

internal void
FreeAnimation(animator *Animator, animation *ToFree)
{
	ToFree->Next = Animator->FirstFreeAnimation;
	Animator->FirstFreeAnimation = ToFree;
}

internal void
PushShakeAnimation(animator *Animator, entity *Entity, v3 Extent, f32 Duration)
{
	animation *Animation = NewAnimation(Animator);
	Animation->Type = Animation_Shake;
	Animation->ShakeAnim.Base = Entity->P;
	Animation->ShakeAnim.Extent = Extent;
	Animation->ShakeAnim.Duration = Duration;

	Animation->Next = Entity->Animation;
	Entity->Animation = Animation;
}

internal void
PushWaitAnimation(animator *Animator, entity *Entity, f32 Duration)
{
	animation *Animation = NewAnimation(Animator);
	Animation->Type = Animation_Wait;
	Animation->WaitAnim.Duration = Duration;

	Animation->Next = Entity->Animation;
	Entity->Animation = Animation;
}

internal void
PushInterpAnimation(animator *Animator,	entity *Entity,
	v3 Offset, quaternion Rotation, f32 Duration, interp_type Interp)
{
	animation *Animation = NewAnimation(Animator);
	Animation->Type = Animation_Interp;
	Animation->InterpAnim.StartP = Entity->P;
	Animation->InterpAnim.EndP = Entity->P + Offset;
	Animation->InterpAnim.StartRotation = Entity->Rotation;
	Animation->InterpAnim.EndRotation = Rotation;
	Animation->InterpAnim.Interp = Interp;
	Animation->InterpAnim.Duration = Duration;

	Animation->Next = Entity->Animation;
	Entity->Animation = Animation;
}

internal void
AnimateEntity(animator *Animator, entity *Entity, f32 dt)
{
	animation *Animation = Entity->Animation;
	if(!Animation)
	{
		Animation = &Entity->BaseAnimation;
	}

	v3 Up = V3(0,1,0);
	switch(Animation->Type)
	{
		case Animation_None:
		{

		} break;

		case Animation_Spring:
		{
			// TODO: Problem with the rotation spring! Doesn't always converge to the
			//	correct rotation.
			spring_animation *SpringAnim = &Animation->SpringAnim;
			f32 AngularFrequency = 2.0f*Pi32*SpringAnim->OscillationFrequency;
			f32 DampingRatio = Log(SpringAnim->OscillationReductionPerSecond)/(-AngularFrequency);

			f32 f = 1.0f + 2.0f*dt*DampingRatio*AngularFrequency;
			f32 oo = AngularFrequency*AngularFrequency;
			f32 hoo = dt*oo;
			f32 hhoo = dt*hoo;
			f32 DeltaInv = 1.0f/(f + hhoo);

			v3 PDeltaX = f*Entity->P + dt*Entity->dP + hhoo*SpringAnim->TargetP;
			v3 PDeltaV = Entity->dP + hoo*(SpringAnim->TargetP - Entity->P);

			Entity->P = PDeltaX*DeltaInv;
			Entity->dP = PDeltaV*DeltaInv;

			v3 Rotation = RotateBy(Up, Entity->Rotation);
			v3 TargetRotation = RotateBy(Up, SpringAnim->TargetRotation);
			v3 RotDeltaX = f*Rotation + dt*Entity->AngularVelocity + hhoo*TargetRotation;
			v3 RotDeltaV = Entity->AngularVelocity + hoo*(TargetRotation - Rotation);

			v3 NewRotation = RotDeltaX*DeltaInv;
			Entity->Rotation = RotationQuaternion(Up, NewRotation);
			Entity->AngularVelocity = RotDeltaV*DeltaInv;
		} break;

		case Animation_Shake:
		{
			random_shake_animation *ShakeAnim = &Animation->ShakeAnim;
			ShakeAnim->tAccum += dt;

			v3 Shake = V3(RandomRangeF32(&GlobalSeries, -1.0f, 1.0f)*ShakeAnim->Extent.x,
						  RandomRangeF32(&GlobalSeries, -1.0f, 1.0f)*ShakeAnim->Extent.y,
						  RandomRangeF32(&GlobalSeries, -1.0f, 1.0f)*ShakeAnim->Extent.z);
			Entity->P = ShakeAnim->Base + Shake;

			if(ShakeAnim->tAccum > ShakeAnim->Duration)
			{
				animation *ToFree = Entity->Animation;
				Entity->Animation = Animation->Next;
				FreeAnimation(Animator, ToFree);
			}
		} break;

		case Animation_Wait:
		{
			wait_animation *WaitAnim = &Animation->WaitAnim;
			WaitAnim->tAccum += dt;
			if(WaitAnim->tAccum > WaitAnim->Duration)
			{
				animation *ToFree = Entity->Animation;
				Entity->Animation = Animation->Next;
				FreeAnimation(Animator, ToFree);
			}
		} break;

		case Animation_Interp:
		{
			interp_animation *InterpAnim = &Animation->InterpAnim;
			InterpAnim->tAccum += dt;
			if(InterpAnim->tAccum > InterpAnim->Duration)
			{
				InterpAnim->tAccum = InterpAnim->Duration;
			}

			f32 t = (InterpAnim->tAccum / InterpAnim->Duration);
			switch(InterpAnim->Interp)
			{
				case Interp_Linear:
				{
					Entity->P = Lerp(InterpAnim->StartP, t, InterpAnim->EndP);
					Entity->Rotation = Lerp(InterpAnim->StartRotation, t, InterpAnim->EndRotation);
				} break;

				case Interp_Cubic:
				{
					f32 tCubic = -2.0f*t*t*t + 3.0f*t*t;
					Entity->P = Lerp(InterpAnim->StartP, tCubic, InterpAnim->EndP);
					Entity->Rotation = Lerp(InterpAnim->StartRotation, tCubic, InterpAnim->EndRotation);
				} break;
			}

			if(InterpAnim->tAccum == InterpAnim->Duration)
			{
				animation *ToFree = Entity->Animation;
				Entity->Animation = Animation->Next;
				FreeAnimation(Animator, ToFree);
			}
		} break;

		case Animation_Physics:
		{
			rigid_body *Body = &Entity->RigidBody;
			v3 GravityImpulse = dt*V3(0.0f, -9.81f, 0.0f);
			Assert(Body->IsInitialized);
			if(Body->InvMass)
			{
				f32 Mass = (1.0f/Body->InvMass);

				if(Animator->PhysicsState->GravityMode)
				{
					ApplyImpulse(Entity, Mass*GravityImpulse, Body->WorldCentroid);
				}

				v3 DragImpulse = dt*0.5f*Body->MoveSpec.LinearAirFriction*LengthSq(Entity->dP)*NOZ(Entity->dP);
				ApplyImpulse(Entity, -Mass*DragImpulse, Body->WorldCentroid);

				if(Determinant(Body->LocalInvInertialTensor) != 0.0f)
				{
					mat3 InertialTensor = Inverse(Body->LocalInvInertialTensor);
					ApplyTorque(Entity, -dt*Body->MoveSpec.RotationAirFriction*(InertialTensor*Entity->AngularVelocity));
				}
			}

			PhysicsIntegrateEntity(Animator->PhysicsState, Entity, dt);
		} break;

		InvalidDefaultCase;
	}
}

internal void
InitAnimator(animator *Animator, physics_state *PhysicsState, memory_region *Region)
{
	Animator->Region = Region;
	Animator->PhysicsState = PhysicsState;
	Animator->FirstFreeAnimation = 0;
}
