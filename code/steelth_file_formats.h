/*@H
* File: steelth_file_formats.h
* Author: Jesse Calvert
* Created: November 27, 2017, 15:33
* Last modified: April 11, 2019, 12:02
*/

#pragma once

#define MAGIC_CODE 0xabcd1234

#define MAX_VISIBLE_PIECES 4
#define MAX_ABILITIES 8

INTROSPECT()
enum entity_visible_piece_type
{
	PieceType_Cube,
	PieceType_Bitmap,
	PieceType_Bitmap_Upright,
	PieceType_Mesh,

	PieceType_Count,
};
struct entity_visible_piece
{
	v3 Dim;
	quaternion Rotation;
	v3 Offset;
	entity_visible_piece_type Type;

	bitmap_id BitmapID;
	mesh_id MeshID;
	surface_material Material;
};

INTROSPECT()
enum entity_type
{
	Entity_None,

	Entity_Count,
};

INTROSPECT()
enum asset_name
{
	Asset_None,

	Asset_Circle,
	Asset_Smile,
	Asset_Arrow,
	Asset_GoldIcon,
	Asset_HeartIcon,
	Asset_ManaIcon,
	Asset_SwordIcon,
	Asset_ClockIcon,

	Asset_LiberationMonoFont,
	Asset_TimesFont,

	Asset_RoundedBox,
	Asset_Sphere,
	Asset_Capsule,

	Asset_StaticGeometry,
	Asset_EntityInfo,

	Asset_Count,
};

INTROSPECT()
enum asset_type
{
	AssetType_None,

	AssetType_Bitmap,
	AssetType_Font,
	AssetType_Mesh,
	AssetType_EntityInfo,

	AssetType_Count,
};

struct header_sla
{
	u32 MagicCode;
	u32 Version;
};

/*
	NOTE: File format:Version 3

		header (points to data start and asset_header array start)
		asset data
		asset metadata
		asset_header_sla_v3 array (points to metadata and data)
*/

struct header_sla_v3
{
	header_sla Header;

	u32 TextureCount;
	u32 FontCount;
	u32 MeshCount;
	u32 EntityInfoCount;

	umm DataOffset;
	umm DataSize;

	umm MetaDataOffset;
	umm MetaDataSize;

	umm AssetHeadersOffset;
	umm AssetHeadersSize;
};

struct asset_header_sla_v3
{
	asset_type Type;
	asset_name Name;

	umm MetaDataOffset;
	umm MetaDataSize;

	umm DataOffset;
	umm DataSize;
};

struct entity_info
{
	entity_type Type;
	string DisplayName;
	char Buffer[64];

	u32 PieceCount;
	entity_visible_piece Pieces[MAX_VISIBLE_PIECES];

	v4 Color;
	u32 Flags;
};

struct bitmap_sla_v3
{
	u32 Width;
	u32 Height;
};

struct font_sla_v3
{
	f32 Scale;
	f32 Ascent;
	f32 Descent;
	f32 Linegap;

	v2 Offset[128];
	f32 Advance[128];
	f32 Kerning[128][128];

	v2 TexCoordMin[128];
	v2 TexCoordMax[128];
	s32 Apron;

	asset_header_sla_v3 BitmapHeader;
	bitmap_sla_v3 BitmapSLA;
};

struct mesh_sla_v3
{
	u32 VertexCount;
	u32 FaceCount;

	umm VerticesSize;
	umm FacesSize;
};

/*
	NOTE: File format:Version 2

		Header

		texture_sla[]
		font_sla[]
		mesh_sla[]

		data block
*/

struct header_sla_v2
{
	header_sla Header;

	u32 MeshCount;
	u32 TextureCount;
	u32 FontCount;

	u32 DataOffset;
};

struct bitmap_sla_v2
{
	asset_name Name;

	u32 Width;
	u32 Height;

	u32 PixelsSize;
	u32 PixelsOffset;
};

struct font_sla_v2
{
	asset_name Name;

	f32 Scale;
	f32 Ascent;
	f32 Descent;
	f32 Linegap;

	v2 Offset[128];
	f32 Advance[128];
	f32 Kerning[128][128];

	v2 TexCoordMin[128];
	v2 TexCoordMax[128];
	s32 Apron;

	bitmap_sla_v2 BitmapSLA;
};

struct mesh_sla_v2
{
	asset_name Name;
	u32 VertexCount;
	u32 VerticesOffset;
	u32 VerticesSize;

	u32 FaceCount;
	u32 FacesOffset;
	u32 FacesSize;
};
