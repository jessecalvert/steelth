/*@H
* File: steelth_assets.h
* Author: Jesse Calvert
* Created: December 18, 2017, 17:09
* Last modified: April 11, 2019, 14:18
*/

#pragma once

struct asset_memory_header
{
	asset_memory_header *Next;
	asset_memory_header *Prev;

	b32 InUse;
	u32 BlockSize;
};

struct asset_file
{
	platform_file FileHandle;
	header_sla_v3 Header;
	u32 TotalAssetCount;
	b32 IsValid;

	b32 CanBeWritten;

	stream Errors;
};

enum asset_state
{
	Asset_Unloaded,
	Asset_Queued,
	Asset_Loaded,
};
struct asset
{
	asset *Next;
	asset *Prev;
	asset *NextInHash;

	asset_name Name;
	asset_type Type;
	u32 DataSize;
	u32 DataOffset;
	u32 MetaDataSize;
	u32 MetaDataOffset;
	asset_file *AssetFile;
	volatile asset_state State;
	union
	{
		renderer_texture Font;
		renderer_texture Bitmap;
		renderer_mesh Mesh;
	};
	union
	{
		bitmap_sla_v3 BitmapSLA;
		mesh_sla_v3 MeshSLA;
		entity_info *EntityInfo;

		// NOTE: The font info is pretty big and there won't be that many of them. So this
		//	is stored in the assets structure instead.
		u32 FontIndex;
	};

	b32 IsDirty;
	asset_memory_header *MemoryHeader;
	u32 ID;
};

#define MAX_FONTS 8
struct assets
{
	memory_region Region;
	struct game_state *GameState;

	u32 AssetFileCount;
	asset_file *AssetFiles;
	asset_file *LocalAssetFile;

	u32 NextFreeTextureHandle;
	u32 NextFreeMeshHandle;
	render_memory_op_queue *RenderOpQueue;

	u32 NextAssetID;
	u32 AssetCount;
	asset *Assets;

	ticket_mutex AssetMemoryMutex;
	asset_memory_header MemorySentinel;
	asset LoadedAssetsSentinel;

	u32 FontInfoCount;
	font_sla_v3 FontInfos[MAX_FONTS];

	entity_info EntityInfos[Entity_Count];

	stream *Info;
};

inline asset *
GetAsset(assets *Assets, u32 ID)
{
	asset *Result = 0;

	BeginTicketMutex(&Assets->AssetMemoryMutex);

	if(ID)
	{
		asset *Asset = Assets->Assets + ID;
		if(Asset->State == Asset_Loaded)
		{
			Result = Asset;

			if(Asset->Name != Asset_StaticGeometry)
			{
				DLIST_REMOVE(Asset);
				DLIST_INSERT(&Assets->LoadedAssetsSentinel, Asset);
			}
		}
	}

	EndTicketMutex(&Assets->AssetMemoryMutex);

	return Result;
}

inline asset *
GetAssetByName_(assets *Assets, asset_name Name)
{
	TIMED_FUNCTION();

	asset *Result = 0;
	for(u32 AssetIndex = 1;
		AssetIndex < Assets->AssetCount;
		++AssetIndex)
	{
		asset *Asset = Assets->Assets + AssetIndex;
		if(Asset->Name == Name)
		{
			Result = Asset;
			break;
		}
	}

	return Result;
}

inline bitmap_id
GetBitmapID(assets *Assets, asset_name Name)
{
	asset *Asset = GetAssetByName_(Assets, Name);
	Assert(Asset->Type == AssetType_Bitmap);
	bitmap_id ID = {Asset->ID};
	return ID;
}

inline font_id
GetFontID(assets *Assets, asset_name Name)
{
	asset *Asset = GetAssetByName_(Assets, Name);
	Assert(Asset->Type == AssetType_Font);
	font_id ID = {Asset->ID};
	return ID;
}

inline mesh_id
GetMeshID(assets *Assets, asset_name Name)
{
	asset *Asset = GetAssetByName_(Assets, Name);
	Assert(Asset->Type == AssetType_Mesh);
	mesh_id ID = {Asset->ID};
	return ID;
}

inline renderer_texture
GetBitmap(assets *Assets, bitmap_id ID)
{
	renderer_texture Result = {};
	asset *Asset = GetAsset(Assets, ID.Value);
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Bitmap);
		Result = Asset->Bitmap;
	}
	return Result;
}

inline renderer_texture
GetFontBitmap(assets *Assets, font_id ID)
{
	renderer_texture Result = {};
	asset *Asset = GetAsset(Assets, ID.Value);
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Font);
		Result = Asset->Font;
	}
	return Result;
}

inline renderer_mesh
GetMesh(assets *Assets, mesh_id ID)
{
	renderer_mesh Result = {};
	asset *Asset = GetAsset(Assets, ID.Value);
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Mesh);
		Result = Asset->Mesh;
	}
	return Result;
}

inline bitmap_sla_v3 *
GetBitmapInfo(assets *Assets, bitmap_id ID)
{
	bitmap_sla_v3 *Result = 0;
	asset *Asset = Assets->Assets + ID.Value;
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Bitmap);
		Result = &Asset->BitmapSLA;
	}
	return Result;
}

inline font_sla_v3 *
GetFontInfo(assets *Assets, font_id ID)
{
	font_sla_v3 *Result = 0;
	asset *Asset = Assets->Assets + ID.Value;
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Font);
		Result = &Assets->FontInfos[Asset->FontIndex];
	}
	return Result;
}

inline mesh_sla_v3 *
GetMeshInfo(assets *Assets, mesh_id ID)
{
	mesh_sla_v3 *Result = 0;
	asset *Asset = Assets->Assets + ID.Value;
	if(Asset)
	{
		Assert(Asset->Type == AssetType_Mesh);
		Result = &Asset->MeshSLA;
	}
	return Result;
}
