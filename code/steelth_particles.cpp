/*@H
* File: steelth_particles.cpp
* Author: Jesse Calvert
* Created: June 14, 2018, 17:51
* Last modified: April 22, 2019, 17:38
*/

internal void
SpawnFountain(particle_system *System, v3 P, u32 ParticleAddCount_4 = 1)
{
	if(System)
	{
		particle_emitter *Emitter = System->Emitters + Emitter_Fountain;

		v3_4x P_4x = V3_4x(P);
		for(u32 Index = 0;
			Index < ParticleAddCount_4;
			++Index)
		{
			particle_4x *A = Emitter->Particles + Emitter->NextParticle++;
			Emitter->NextParticle = (Emitter->NextParticle % ArrayCount(Emitter->Particles));

			A->P.x = MMSetWithExpr(RandomRangeF32(&System->Series, -0.25f, 0.25f));
			A->P.y = ZeroF32_4x();
			A->P.z = MMSetWithExpr(RandomRangeF32(&System->Series, -0.25f, 0.25f));
			A->P += P_4x;

			A->dP.x = MMSetWithExpr(RandomRangeF32(&System->Series, -1.0f, 1.0f));
			A->dP.y = MMSetWithExpr(RandomRangeF32(&System->Series, 8.0f, 14.0f));
			A->dP.z = MMSetWithExpr(RandomRangeF32(&System->Series, -1.0f, 1.0f));

			A->ddP.x = ZeroF32_4x();
			A->ddP.y = F32_4x(-20.0f);
			A->ddP.z = ZeroF32_4x();

			A->Color.r = MMSetWithExpr(RandomRangeF32(&System->Series, 0.25f, 1.0f));
			A->Color.g = MMSetWithExpr(RandomRangeF32(&System->Series, 0.25f, 1.0f));
			A->Color.b = MMSetWithExpr(RandomRangeF32(&System->Series, 0.25f, 1.0f));
			A->Color.a = F32_4x(1.0f);

			A->dColor.r = ZeroF32_4x();
			A->dColor.g = ZeroF32_4x();
			A->dColor.b = ZeroF32_4x();
			A->dColor.a = F32_4x(-1.5f);

			A->GroundY = P.y;
		}
	}
}

internal void
SpawnFire(particle_system *System, v3 P, u32 ParticleAddCount_4 = 1)
{
	if(System)
	{
		particle_emitter *Emitter = System->Emitters + Emitter_Fire;

		v3_4x P_4x = V3_4x(P);
		for(u32 Index = 0;
			Index < ParticleAddCount_4;
			++Index)
		{
			particle_4x *A = Emitter->Particles + Emitter->NextParticle++;
			Emitter->NextParticle = (Emitter->NextParticle % ArrayCount(Emitter->Particles));

			A->P.x = MMSetWithExpr(RandomRangeF32(&System->Series, -0.5f, 0.5f));
			A->P.y = ZeroF32_4x();
			A->P.z = MMSetWithExpr(RandomRangeF32(&System->Series, -0.5f, 0.5f));
			A->P += P_4x;

			A->dP.x = MMSetWithExpr(RandomRangeF32(&System->Series, -0.5f, 0.5f));
			A->dP.y = MMSetWithExpr(RandomRangeF32(&System->Series, -0.5f, 0.5f));
			A->dP.z = MMSetWithExpr(RandomRangeF32(&System->Series, -0.5f, 0.5f));

			A->ddP.x = ZeroF32_4x();
			A->ddP.y = F32_4x(15.0f);
			A->ddP.z = ZeroF32_4x();

			A->Color.r = MMSetWithExpr(RandomRangeF32(&System->Series, 0.75f, 1.0f));
			A->Color.g = MMSetWithExpr(RandomRangeF32(&System->Series, 0.0f, 0.7f));
			A->Color.b = MMSetWithExpr(RandomRangeF32(&System->Series, 0.0f, 0.7f));
			A->Color.a = MMSetWithExpr(RandomRangeF32(&System->Series, 0.8f, 1.0f));

			A->dColor.r = F32_4x(-0.5f);
			A->dColor.g = F32_4x(-5.0f);
			A->dColor.b = F32_4x(-5.0f);
			A->dColor.a = F32_4x(-2.0f);

			A->GroundY = P.y;
		}
	}
}

internal void
UpdateParticleChunk(particle_emitter *Emitter, f32 dt, u32 FirstIndex, u32 OnePastLastIndex)
{
	TIMED_FUNCTION();

	for(u32 ParticleIndex_4 = FirstIndex;
		ParticleIndex_4 < OnePastLastIndex;
		++ParticleIndex_4)
	{
		particle_4x *A = Emitter->Particles + ParticleIndex_4;

		A->P += dt*A->dP + (0.5f*dt*dt)*A->ddP;
		A->dP += dt*A->ddP;
		A->Color += dt*A->dColor;

		f32_4x BouncedPy = -A->dP.y;
		f32_4x HitGroundMask = (A->P.y < F32_4x(A->GroundY));
		A->dP.y = Select(A->dP.y, HitGroundMask, BouncedPy);
	}
}

struct update_particle_chunk_data
{
	task_memory *TaskMemory;

	particle_emitter *Emitter;
	f32 dt;
	u32 FirstIndex;
	u32 OnePastLastIndex;
};

WORK_QUEUE_CALLBACK(UpdateParticleChunkWork)
{
	update_particle_chunk_data *WorkData = (update_particle_chunk_data *) Data;
	UpdateParticleChunk(WorkData->Emitter, WorkData->dt, WorkData->FirstIndex, WorkData->OnePastLastIndex);
	EndTaskWithMemory(WorkData->TaskMemory);
}

internal void
RenderParticleEmitter(particle_emitter *Emitter, render_group *RenderGroup, v3 CameraP)
{
	TIMED_FUNCTION();

	for(u32 ParticleIndex_4 = 0;
		ParticleIndex_4 < ArrayCount(Emitter->Particles);
		++ParticleIndex_4)
	{
		particle_4x *A = Emitter->Particles + ParticleIndex_4;

		if(AnyTrue(A->Color.a > ZeroF32_4x()))
		{
			for(u32 LaneIndex = 0;
				LaneIndex < 4;
				++LaneIndex)
			{
				v3 P = GetComponent(A->P, LaneIndex);
				v4 Color = GetComponent(A->Color, LaneIndex);

				if(Color.a > 0.0f)
				{
					PushUprightTexture(RenderGroup, P, Emitter->Dim, Emitter->BitmapID, Emitter->Roughness, Emitter->Metalness, Emitter->Emission, Color);
				}
			}
		}
	}
}

internal void
UpdateParticleSystem(game_state *GameState, particle_system *System, f32 dt)
{
	u32 ChunkCount = 8;
	u32 ChunkSize = (MAX_PARTICLES_4/ChunkCount);

	for(u32 EmitterIndex = 0;
		EmitterIndex < Emitter_Count;
		++EmitterIndex)
	{
		particle_emitter *Emitter = System->Emitters + EmitterIndex;
#if 1
		for(u32 ChunkIndex = 0;
			ChunkIndex < ChunkCount;
			++ChunkIndex)
		{
			task_memory *TaskMemory = BeginTaskWithMemory(GameState);
			if(TaskMemory)
			{
				update_particle_chunk_data *Data = PushStruct(&TaskMemory->Region, update_particle_chunk_data);
				Data->TaskMemory = TaskMemory;
				Data->Emitter = Emitter;
				Data->dt = dt;
				Data->FirstIndex = ChunkSize*ChunkIndex;
				Data->OnePastLastIndex = ChunkSize*(ChunkIndex + 1);

				CompletePreviousWritesBeforeFutureWrites;
				Platform->AddWork(GameState->HighPriorityQueue, UpdateParticleChunkWork, Data);
			}
		}

#else
		UpdateParticleChunk(Emitter, dt, 0, MAX_PARTICLES_4);
#endif
	}
}

internal void
RenderParticleSystem(game_state *GameState, particle_system *System, render_group *RenderGroup, v3 CameraP)
{
	Platform->CompleteAllWork(GameState->HighPriorityQueue);

	for(u32 EmitterIndex = 0;
		EmitterIndex < Emitter_Count;
		++EmitterIndex)
	{
		particle_emitter *Emitter = System->Emitters + EmitterIndex;
		RenderParticleEmitter(Emitter, RenderGroup, CameraP);
	}
}

internal void
InitializeParticleSystem(particle_system *System, assets *Assets)
{
	RandomSeriesSeed(&System->Series, 234768);

	particle_emitter *FountainEmitter = System->Emitters + Emitter_Fountain;
	FountainEmitter->NextParticle = 0;
	FountainEmitter->Dim = V2(0.5f, 0.5f);
	FountainEmitter->BitmapID = GetBitmapID(Assets, Asset_Smile);
	FountainEmitter->Roughness = 0.2f;
	FountainEmitter->Metalness = 0.0f;
	FountainEmitter->Emission = 0.0f;

	particle_emitter *FireEmitter = System->Emitters + Emitter_Fire;
	FireEmitter->NextParticle = 0;
	FireEmitter->Dim = V2(0.25f, 0.25f);
	FireEmitter->BitmapID = {0};
	FireEmitter->Roughness = 1.0f;
	FireEmitter->Metalness = 0.0f;
	FireEmitter->Emission = 4.0f;
}
