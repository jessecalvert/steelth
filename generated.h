/*@H
* File: generated.h
* Author: Jesse Calvert
* Created: November 27, 2017, 11:00
* Last modified: April 26, 2019, 19:25
*/

#pragma once

#define MAX_CIRCULAR_REFERENCE_DEPTH 2

#include "steelth_debug.h"
#include "steelth_editor.h"
#include "steelth_title_screen.h"
#include "steelth_entity_manager.h"
#include "steelth_world_mode.h"
#include "steelth_world.h"
#include "steelth_ui.h"
#include "steelth_particles.h"
#include "steelth_animation.h"
#include "steelth_convex_hull.h"
#include "steelth_rigid_body.h"
#include "steelth_gjk.h"
#include "steelth_aabb.h"
#include "steelth_random.h"
#include "steelth_sort.h"
#include "steelth_assets.h"
#include "steelth_file_formats.h"
#include "steelth_config.h"
#include "steelth_renderer.h"
#include "steelth_stream.h"
#include "steelth_memory.h"
#include "steelth_math.h"
#include "steelth_simd.h"
#include "steelth_intrinsics.h"
#include "steelth_debug_interface.h"
#include "steelth_platform.h"
#include "steelth_shared.h"
#include "steelth_types.h"
#include "steelth.h"

global char * Names_shader_type[] =
{
    "Shader_Header",
    "Shader_GBuffer",
    "Shader_Skybox",
    "Shader_BakeProbe",
    "Shader_ShadowMap",
    "Shader_Fresnel",
    "Shader_LightPass",
    "Shader_IndirectLightPass",
    "Shader_Decal",
    "Shader_Overlay",
    "Shader_Overbright",
    "Shader_Blur",
    "Shader_Fog",
    "Shader_FogResolve",
    "Shader_Finalize",
    "Shader_DebugView",
    "Shader_Surfels",
    "Shader_Probes",
    "Shader_Irradiance_Volume",
    "Shader_Count",
};

global char * Names_entity_visible_piece_type[] =
{
    "PieceType_Cube",
    "PieceType_Bitmap",
    "PieceType_Bitmap_Upright",
    "PieceType_Mesh",
    "PieceType_Count",
};

global char * Names_entity_type[] =
{
    "Entity_None",
    "Entity_Count",
};

global char * Names_asset_name[] =
{
    "Asset_None",
    "Asset_Circle",
    "Asset_Smile",
    "Asset_Arrow",
    "Asset_GoldIcon",
    "Asset_HeartIcon",
    "Asset_ManaIcon",
    "Asset_SwordIcon",
    "Asset_ClockIcon",
    "Asset_LiberationMonoFont",
    "Asset_TimesFont",
    "Asset_RoundedBox",
    "Asset_Sphere",
    "Asset_Capsule",
    "Asset_StaticGeometry",
    "Asset_EntityInfo",
    "Asset_Count",
};

global char * Names_asset_type[] =
{
    "AssetType_None",
    "AssetType_Bitmap",
    "AssetType_Font",
    "AssetType_Mesh",
    "AssetType_EntityInfo",
    "AssetType_Count",
};

global char * Names_shape_type[] =
{
    "Shape_None",
    "Shape_Sphere",
    "Shape_Capsule",
    "Shape_ConvexHull",
    "Shape_Count",
};

inline void RecordDebugValue_(render_settings *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(camera *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(shape *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(collider *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(rigid_body *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(convex_hull *ValuePtr, char *GUID, u32 Depth=0);
inline void RecordDebugValue_(entity *ValuePtr, char *GUID, u32 Depth=0);

inline void RecordDebugValue_(shader_type *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Header"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_GBuffer"), Event_enum);} break;
        case 2: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Skybox"), Event_enum);} break;
        case 3: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_BakeProbe"), Event_enum);} break;
        case 4: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_ShadowMap"), Event_enum);} break;
        case 5: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Fresnel"), Event_enum);} break;
        case 6: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_LightPass"), Event_enum);} break;
        case 7: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_IndirectLightPass"), Event_enum);} break;
        case 8: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Decal"), Event_enum);} break;
        case 9: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Overlay"), Event_enum);} break;
        case 10: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Overbright"), Event_enum);} break;
        case 11: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Blur"), Event_enum);} break;
        case 12: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Fog"), Event_enum);} break;
        case 13: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_FogResolve"), Event_enum);} break;
        case 14: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Finalize"), Event_enum);} break;
        case 15: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_DebugView"), Event_enum);} break;
        case 16: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Surfels"), Event_enum);} break;
        case 17: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Probes"), Event_enum);} break;
        case 18: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Irradiance_Volume"), Event_enum);} break;
        case 19: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shader_Count"), Event_enum);} break;
	}
}

inline void RecordDebugValue_(entity_visible_piece_type *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("PieceType_Cube"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("PieceType_Bitmap"), Event_enum);} break;
        case 2: {RECORD_DEBUG_EVENT(DEBUG_NAME("PieceType_Bitmap_Upright"), Event_enum);} break;
        case 3: {RECORD_DEBUG_EVENT(DEBUG_NAME("PieceType_Mesh"), Event_enum);} break;
        case 4: {RECORD_DEBUG_EVENT(DEBUG_NAME("PieceType_Count"), Event_enum);} break;
	}
}

inline void RecordDebugValue_(entity_type *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("Entity_None"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("Entity_Count"), Event_enum);} break;
	}
}

inline void RecordDebugValue_(asset_name *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_None"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Circle"), Event_enum);} break;
        case 2: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Smile"), Event_enum);} break;
        case 3: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Arrow"), Event_enum);} break;
        case 4: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_GoldIcon"), Event_enum);} break;
        case 5: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_HeartIcon"), Event_enum);} break;
        case 6: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_ManaIcon"), Event_enum);} break;
        case 7: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_SwordIcon"), Event_enum);} break;
        case 8: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_ClockIcon"), Event_enum);} break;
        case 9: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_LiberationMonoFont"), Event_enum);} break;
        case 10: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_TimesFont"), Event_enum);} break;
        case 11: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_RoundedBox"), Event_enum);} break;
        case 12: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Sphere"), Event_enum);} break;
        case 13: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Capsule"), Event_enum);} break;
        case 14: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_StaticGeometry"), Event_enum);} break;
        case 15: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_EntityInfo"), Event_enum);} break;
        case 16: {RECORD_DEBUG_EVENT(DEBUG_NAME("Asset_Count"), Event_enum);} break;
	}
}

inline void RecordDebugValue_(asset_type *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_None"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_Bitmap"), Event_enum);} break;
        case 2: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_Font"), Event_enum);} break;
        case 3: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_Mesh"), Event_enum);} break;
        case 4: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_EntityInfo"), Event_enum);} break;
        case 5: {RECORD_DEBUG_EVENT(DEBUG_NAME("AssetType_Count"), Event_enum);} break;
	}
}

inline void RecordDebugValue_(shape_type *ValuePtr, char *GUID)
{
	switch(*ValuePtr)
	{
        case 0: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shape_None"), Event_enum);} break;
        case 1: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shape_Sphere"), Event_enum);} break;
        case 2: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shape_Capsule"), Event_enum);} break;
        case 3: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shape_ConvexHull"), Event_enum);} break;
        case 4: {RECORD_DEBUG_EVENT(DEBUG_NAME("Shape_Count"), Event_enum);} break;
	}
}


inline void
RecordDebugValue_(render_settings *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    DEBUG_VALUE_NAME(&ValuePtr->Resolution, "Resolution");
    DEBUG_VALUE_NAME(&ValuePtr->ShadowMapResolution, "ShadowMapResolution");
    DEBUG_VALUE_NAME(&ValuePtr->SwapInterval, "SwapInterval");
    DEBUG_VALUE_NAME(&ValuePtr->RenderScale, "RenderScale");
    DEBUG_VALUE_NAME(&ValuePtr->Bloom, "Bloom");
}

inline void
RecordDebugValue_(camera *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    {DEBUG_DATA_BLOCK("Type");
        DEBUG_VALUE_NAME(&ValuePtr->Type, "Type");
    }
    DEBUG_VALUE_NAME(&ValuePtr->HorizontalFOV, "HorizontalFOV");
    DEBUG_VALUE_NAME(&ValuePtr->AspectRatio, "AspectRatio");
    DEBUG_VALUE_NAME(&ValuePtr->HalfWidth, "HalfWidth");
    DEBUG_VALUE_NAME(&ValuePtr->HalfHeight, "HalfHeight");
    DEBUG_VALUE_NAME(&ValuePtr->Near, "Near");
    DEBUG_VALUE_NAME(&ValuePtr->Far, "Far");
    DEBUG_VALUE_NAME(&ValuePtr->P, "P");
    DEBUG_VALUE_NAME(&ValuePtr->Rotation, "Rotation");
    DEBUG_VALUE_NAME(&ValuePtr->Projection_, "Projection_");
    DEBUG_VALUE_NAME(&ValuePtr->View_, "View_");
    DEBUG_VALUE_NAME(&ValuePtr->InvView_, "InvView_");
}

inline void
RecordDebugValue_(shape *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    {DEBUG_DATA_BLOCK("Type");
        DEBUG_VALUE_NAME(&ValuePtr->Type, "Type");
    }
    {DEBUG_DATA_BLOCK("Sphere");
        DEBUG_VALUE_NAME(&ValuePtr->Sphere, "Sphere");
    }
    {DEBUG_DATA_BLOCK("Capsule");
        DEBUG_VALUE_NAME(&ValuePtr->Capsule, "Capsule");
    }
    {DEBUG_DATA_BLOCK("ConvexHull");
        if(ValuePtr->ConvexHull) {RecordDebugValue_(ValuePtr->ConvexHull, DEBUG_NAME("ConvexHull"), Depth+1);}
    }
}

inline void
RecordDebugValue_(collider *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    {DEBUG_DATA_BLOCK("Next");
        if(ValuePtr->Next) {RecordDebugValue_(ValuePtr->Next, DEBUG_NAME("Next"), Depth+1);}
    }
    {DEBUG_DATA_BLOCK("Prev");
        if(ValuePtr->Prev) {RecordDebugValue_(ValuePtr->Prev, DEBUG_NAME("Prev"), Depth+1);}
    }
    DEBUG_VALUE_NAME(&ValuePtr->Mass, "Mass");
    DEBUG_VALUE_NAME(&ValuePtr->LocalInertialTensor, "LocalInertialTensor");
    DEBUG_VALUE_NAME(&ValuePtr->LocalCentroid, "LocalCentroid");
    {DEBUG_DATA_BLOCK("Shape");
        DEBUG_VALUE_NAME(&ValuePtr->Shape, "Shape");
    }
}

inline void
RecordDebugValue_(rigid_body *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    DEBUG_VALUE_NAME(&ValuePtr->OnSolidGround, "OnSolidGround");
    DEBUG_VALUE_NAME(&ValuePtr->Collideable, "Collideable");
    {DEBUG_DATA_BLOCK("MoveSpec");
        DEBUG_VALUE_NAME(&ValuePtr->MoveSpec, "MoveSpec");
    }
    DEBUG_VALUE_NAME(&ValuePtr->InvMass, "InvMass");
    DEBUG_VALUE_NAME(&ValuePtr->LocalInvInertialTensor, "LocalInvInertialTensor");
    DEBUG_VALUE_NAME(&ValuePtr->WorldCentroid, "WorldCentroid");
    DEBUG_VALUE_NAME(&ValuePtr->LocalCentroid, "LocalCentroid");
    {DEBUG_DATA_BLOCK("ColliderSentinel");
        DEBUG_VALUE_NAME(&ValuePtr->ColliderSentinel, "ColliderSentinel");
    }
    {DEBUG_DATA_BLOCK("AABBNode");
        if(ValuePtr->AABBNode) {RecordDebugValue_(ValuePtr->AABBNode, DEBUG_NAME("AABBNode"), Depth+1);}
    }
    DEBUG_VALUE_NAME(&ValuePtr->IsInitialized, "IsInitialized");
}

inline void
RecordDebugValue_(convex_hull *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    DEBUG_VALUE_NAME(&ValuePtr->VertexCount, "VertexCount");
    DEBUG_VALUE_NAME(ValuePtr->Vertices, "Vertices");
    DEBUG_VALUE_NAME(&ValuePtr->EdgeCount, "EdgeCount");
    {DEBUG_DATA_BLOCK("Edges");
        if(ValuePtr->Edges) {RecordDebugValue_(ValuePtr->Edges, DEBUG_NAME("Edges"), Depth+1);}
    }
    DEBUG_VALUE_NAME(&ValuePtr->FaceCount, "FaceCount");
    {DEBUG_DATA_BLOCK("Faces");
        if(ValuePtr->Faces) {RecordDebugValue_(ValuePtr->Faces, DEBUG_NAME("Faces"), Depth+1);}
    }
}

inline void
RecordDebugValue_(entity *ValuePtr, char *GUID, u32 Depth)
{
	if(Depth > MAX_CIRCULAR_REFERENCE_DEPTH) return;
    {DEBUG_DATA_BLOCK("ID");
        DEBUG_VALUE_NAME(&ValuePtr->ID, "ID");
    }
    {DEBUG_DATA_BLOCK("Info");
        if(ValuePtr->Info) {RecordDebugValue_(ValuePtr->Info, DEBUG_NAME("Info"), Depth+1);}
    }
    DEBUG_VALUE_NAME(&ValuePtr->P, "P");
    DEBUG_VALUE_NAME(&ValuePtr->Rotation, "Rotation");
    DEBUG_VALUE_NAME(&ValuePtr->InvRotation, "InvRotation");
    DEBUG_VALUE_NAME(&ValuePtr->dP, "dP");
    DEBUG_VALUE_NAME(&ValuePtr->AngularVelocity, "AngularVelocity");
    {DEBUG_DATA_BLOCK("RigidBody");
        DEBUG_VALUE_NAME(&ValuePtr->RigidBody, "RigidBody");
    }
    {DEBUG_DATA_BLOCK("Animation");
        if(ValuePtr->Animation) {RecordDebugValue_(ValuePtr->Animation, DEBUG_NAME("Animation"), Depth+1);}
    }
    {DEBUG_DATA_BLOCK("BaseAnimation");
        DEBUG_VALUE_NAME(&ValuePtr->BaseAnimation, "BaseAnimation");
    }
    DEBUG_VALUE_NAME(&ValuePtr->Color, "Color");
    DEBUG_VALUE_NAME(&ValuePtr->Flags, "Flags");
    DEBUG_VALUE_NAME(&ValuePtr->PieceCount, "PieceCount");
    {DEBUG_DATA_BLOCK("Pieces");
        if(ValuePtr->Pieces) {RecordDebugValue_(ValuePtr->Pieces, DEBUG_NAME("Pieces"), Depth+1);}
    }
}
