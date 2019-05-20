/*@H
* File: steelth_editor.h
* Author: Jesse Calvert
* Created: December 6, 2018, 16:36
* Last modified: April 10, 2019, 17:31
*/

#pragma once

enum edit_change_type
{
	EditChange_From,
	EditChange_To,

	EditChange_Count,
};
struct entity_info_edit
{
	entity_info *Ptr;
	entity_info Edit[EditChange_Count];
};

enum edit_type
{
	Edit_EntityInfo,
};
struct editor_edit
{
	editor_edit *Next;
	editor_edit *Prev;

	edit_type Type;
	union
	{
		entity_info_edit EntityInfoEdit;
	};

	b32 MadeDirty;
};

struct game_editor
{
	memory_region Region;
	assets *Assets;

	editor_edit EditSentinel;
	editor_edit *LastEdit;
	editor_edit *LastSaveEdit;
	editor_edit *EditFreeList;

	entity_id EditingEntity;
};
