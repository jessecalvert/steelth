/*@H
* File: steelth_ui.h
* Author: Jesse Calvert
* Created: December 3, 2018, 14:46
* Last modified: April 10, 2019, 18:40
*/

#pragma once

enum interaction_type
{
	UI_Interaction_None,
	UI_Interaction_Collapse,
	UI_Interaction_Resize,
	UI_Interaction_ChangeValue,
	UI_Interaction_DragValue,
	UI_Interaction_FrameSelect,
	UI_Interaction_DepthChange,

	UI_Interaction_SelectEntity,

	UI_Interaction_InlineButtonPress,

	UI_Interaction_Count,
};

struct ui_id
{
	void *Value[2];
};

#define UIIDFromPointer__(Pointer, File, Line) UIID(Pointer, File #Line)
#define UIIDFromPointer_(Pointer, File, Line) UIIDFromPointer__(Pointer, File, Line)
#define UIIDFromPointer(Pointer) UIIDFromPointer_(Pointer, __FILE__, __LINE__)
inline ui_id
UIID(void *A, void *B)
{
	ui_id Result = {A, B};
	return Result;
}

struct ui_interaction
{
	interaction_type Type;
	void *Identifier;

	union
	{
		void *Generic[2];

		void *Pointer;
		ui_id ID;
		struct debug_ui_element *UIElement;
		u32 FrameSelect;
		s32 DepthChange;

		struct
		{
			entity_id EntityID;
		};
	};
};

inline b32
operator==(ui_interaction A, ui_interaction B)
{
	b32 Result = ((A.Type == B.Type) &&
				  (A.Identifier == B.Identifier) &&
				  (A.Generic[0] == B.Generic[0]) &&
				  (A.Generic[1] == B.Generic[1]));
	return Result;
}

inline ui_interaction
NullInteraction()
{
	ui_interaction Result = {};
	return Result;
}

#define MAX_TARGETS 4

struct ui_state
{
	render_group *Group;
	v2 MouseP;
	v2 dMouseP;
	b32 LeftMousePressed;
	b32 LeftMouseReleased;

	string Tooltip;
	char TooltipBuffer[128];

	entity_id HoveredID;
	entity_id SelectedID;

	ui_interaction HotInteraction;
	ui_interaction NextInteraction;
	ui_interaction OperatingInteraction;
};

struct ui_layout
{
	ui_state *UI;

	font_id Font;
	f32 LineHeight;
	f32 Border;
	v4 Color;

	v2 AtY;
	v2 AtX;

	u32 Indent;
	f32 IndentWidth;
	rectangle2 Rect;
};
