/*@H
* File: steelth_entity_manager.cpp
* Author: Jesse Calvert
* Created: November 5, 2018, 13:59
* Last modified: April 10, 2019, 18:32
*/

internal void
AddPiece(entity *Entity, v3 Dim, quaternion Rotation, v3 Offset, entity_visible_piece_type Type,
	bitmap_id BitmapID, mesh_id MeshID, surface_material Material)
{
	Assert(Entity->PieceCount < ArrayCount(Entity->Pieces));
	entity_visible_piece *Piece = Entity->Pieces + Entity->PieceCount++;
	Piece->Dim = Dim;
	Piece->Rotation = Rotation;
	Piece->Offset = Offset;
	Piece->Type = Type;
	Piece->BitmapID = BitmapID;
	Piece->MeshID = MeshID;
	Piece->Material = Material;
}

internal void
AddPieceBitmap(entity *Entity, v2 Scale, quaternion Rotation, v3 Offset,
	bitmap_id BitmapID, surface_material Material)
{
	AddPiece(Entity, V3(Scale.x, Scale.y, 0), Rotation, Offset, PieceType_Bitmap, BitmapID, {0}, Material);
}

internal void
AddPieceBitmapUpright(entity *Entity, v2 Scale, v3 Offset,
	bitmap_id BitmapID, surface_material Material)
{
	// TODO: Add rotation!
	AddPiece(Entity, V3(Scale.x, Scale.y, 0), Q(1,0,0,0), Offset, PieceType_Bitmap_Upright, BitmapID, {0}, Material);
}

internal void
AddPieceCube(entity *Entity, v3 Dim, quaternion Rotation, v3 Offset,
	bitmap_id BitmapID, surface_material Material)
{
	AddPiece(Entity, Dim, Rotation, Offset, PieceType_Cube, BitmapID, {0}, Material);
}

internal void
AddPieceMesh(entity *Entity, v3 Scale, quaternion Rotation, v3 Offset,
	bitmap_id BitmapID, mesh_id MeshID, surface_material Material)
{
	AddPiece(Entity, Scale, Rotation, Offset, PieceType_Mesh, BitmapID, MeshID, Material);
}

internal entity *
AddEntity(entity_manager *EntityManager)
{
	entity_hash_entry *NewHashEntry = EntityManager->FirstFreeEntityHashEntry;
	if(NewHashEntry)
	{
		EntityManager->FirstFreeEntityHashEntry = NewHashEntry->NextInHash;
		ZeroStruct(*NewHashEntry);
	}
	else
	{
		NewHashEntry = PushStruct(EntityManager->Region, entity_hash_entry);
	}

	// TODO: Do we even need a hash function for this?
	u32 HashValue = EntityManager->NextEntityID++;
	NewHashEntry->NextInHash = EntityManager->EntityHashTable[HashValue];
	EntityManager->EntityHashTable[HashValue] = NewHashEntry;

	DLIST_INSERT(&EntityManager->HashEntrySentinel, NewHashEntry);
	++EntityManager->EntityCount;
	entity *Result = &NewHashEntry->Entity;
	Result->ID.Value = HashValue;
	Result->Rotation = Q(1,0,0,0);
	Result->InvRotation = Q(1,0,0,0);

	return Result;
}

internal void
RemoveEntity(entity_manager *EntityManager, entity *Entity)
{
	if(Entity)
	{
		if(Entity->RigidBody.IsInitialized)
		{
			Assert(EntityManager->PhysicsState);
			RemoveRigidBody(EntityManager->PhysicsState, Entity);
		}

		if(Entity->Animation)
		{
			Assert(EntityManager->Animator);
			FreeAnimation(EntityManager->Animator, Entity->Animation);
		}

		u32 HashValue = Entity->ID.Value;
		entity_hash_entry *HashEntry = EntityManager->EntityHashTable[HashValue];
		entity_hash_entry *PrevHashEntry = 0;
		while(HashEntry->Entity.ID.Value != HashValue)
		{
			PrevHashEntry = HashEntry;
			HashEntry = HashEntry->NextInHash;
		}

		if(PrevHashEntry)
		{
			PrevHashEntry->NextInHash = HashEntry->NextInHash;
		}
		else
		{
			EntityManager->EntityHashTable[HashValue] = HashEntry->NextInHash;
		}

		--EntityManager->EntityCount;
		DLIST_REMOVE(HashEntry);
		HashEntry->NextInHash = EntityManager->FirstFreeEntityHashEntry;
		EntityManager->FirstFreeEntityHashEntry = HashEntry;
	}
}

internal entity *
GetEntity(entity_manager *EntityManager, entity_id ID)
{
	entity *Result = 0;

	u32 HashValue = ID.Value;
	entity_hash_entry *HashEntry = EntityManager->EntityHashTable[HashValue];
	while(HashEntry && (HashEntry->Entity.ID != ID))
	{
		HashEntry = HashEntry->NextInHash;
	}

	if(HashEntry)
	{
		Result = &HashEntry->Entity;
	}

	return Result;
}

inline b32
IsValidID(entity_manager *Manager, entity_id ID)
{
	entity *Entity = GetEntity(Manager, ID);
	b32 Result = (Entity ? true : false);
	return Result;
}

internal void
InitEntityManager(game_mode_world *WorldMode, entity_manager *EntityManager, memory_region *Region, assets *Assets)
{
	EntityManager->Region = Region;
	EntityManager->NextEntityID = 1;
	DLIST_INIT(&EntityManager->HashEntrySentinel);
}
