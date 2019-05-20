/*@H
* File: steelth_ui.cpp
* Author: Jesse Calvert
* Created: December 3, 2018, 14:46
* Last modified: April 10, 2019, 17:31
*/

internal b32
ButtonInteractionSelected(ui_state *UI, b32 MouseHover, ui_interaction Interaction, game_input *Input)
{
	b32 Selected = false;

	v2 MouseP = Input->MouseP;
	if(MouseHover)
	{
		UI->HotInteraction = Interaction;
		if(UI->NextInteraction.Type == UI_Interaction_None)
		{
			Selected = true;
		}
		if(Input->KeyStates[Key_LeftClick].WasPressed)
		{
			UI->NextInteraction = Interaction;
		}
	}
	if(UI->NextInteraction == Interaction)
	{
		Selected = true;
	}

	return Selected;
}

internal b32
ButtonInteractionSelected(ui_state *UI, rectangle2 ButtonRect, ui_interaction Interaction, game_input *Input)
{
	b32 Result = ButtonInteractionSelected(UI, InsideRectangle(ButtonRect, Input->MouseP), Interaction, Input);
	return Result;
}

internal rectangle2
GetTextBounds(render_group *Group, font_id Font, v2 P, f32 Height, string String)
{
	rectangle2 TextRect = {};
	font_sla_v3 *FontInfo = GetFontInfo(Group->Assets, Font);
	Assert(FontInfo);
	{
		v2 TotalMin = {Real32Maximum, Real32Maximum};
		v2 TotalMax = {Real32Minimum, Real32Minimum};

		v2 Position = P;
		f32 Scale = GetFontScale(Group, Font, Height);
		v2 ApronOffset = Scale*V2((f32)FontInfo->Apron, (f32)FontInfo->Apron);

		char LastChar = 0;
		for(u32 CharIndex = 0;
			CharIndex < String.Length;
			++CharIndex)
		{
			char Character = String.Text[CharIndex];
			Position.x += FontInfo->Kerning[LastChar][Character];

			v2 CharacterTexelDim = FontInfo->TexCoordMax[Character] - FontInfo->TexCoordMin[Character];
			v2 CharacterPixelDim = Scale*Hadamard(V2((r32)FontInfo->BitmapSLA.Width, (r32)FontInfo->BitmapSLA.Height),
				CharacterTexelDim);
			CharacterPixelDim = CharacterPixelDim - 2*ApronOffset;
			v2 CharacterPosition = Position + Scale*FontInfo->Offset[Character];
			v2 MinPixels = CharacterPosition;
			v2 MaxPixels = CharacterPosition + CharacterPixelDim;

			TotalMin.x = Minimum(TotalMin.x, MinPixels.x);
			TotalMin.y = Minimum(TotalMin.y, MinPixels.y);
			TotalMax.x = Maximum(TotalMax.x, MaxPixels.x);
			TotalMax.y = Maximum(TotalMax.y, MaxPixels.y);

			Position.x += Scale*FontInfo->Advance[Character];
		}

		TextRect = Rectangle2(TotalMin, TotalMax);
	}

	return TextRect;
}

internal v2
GetTextDim(render_group *Group, font_id Font, f32 Height, string String)
{
	rectangle2 TextRect = GetTextBounds(Group, Font, V2(0,0), Height, String);
	v2 Result = Dim(TextRect);
	return Result;

}

internal void
BeginFrameUI(ui_state *UI, render_group *Group, game_input *Input)
{
	UI->Group = Group;
	UI->MouseP = Input->MouseP;
	UI->dMouseP = Input->dMouseP;
	UI->LeftMousePressed = Input->KeyStates[Key_LeftClick].WasPressed;
	UI->LeftMouseReleased = Input->KeyStates[Key_LeftClick].WasReleased;

	ui_interaction NullInteraction = {};
	UI->HotInteraction = NullInteraction;

	UI->Tooltip.Length = 0;
}

internal void
EndFrameUI(ui_state *UI)
{
	UI->OperatingInteraction = NullInteraction();
	if(UI->LeftMouseReleased)
	{
		if(UI->NextInteraction == UI->HotInteraction)
		{
			UI->OperatingInteraction = UI->NextInteraction;
		}

		UI->NextInteraction = NullInteraction();
	}

	if(UI->Tooltip.Length)
	{
		v2 TooltipOffset = V2(10.0f, 10.0f);
		v2 RectExpand = V2(5.0f, 5.0f);

		font_id FontID = GetFontID(UI->Group->Assets, Asset_LiberationMonoFont);
		f32 FontHeight = 18.0f;
		v4 TextColor = V4(1.0f, 1.0f, 0.0f, 1.0f);
		v2 At = UI->MouseP + TooltipOffset;

		rectangle2 TooltipRect = GetTextBounds(UI->Group, FontID, At, FontHeight, UI->Tooltip);
		PushQuad(UI->Group, TooltipRect.Min - RectExpand, TooltipRect.Max + RectExpand, V4(0.25f, 0.25f, 0.25f, 1.0f));
		PushText(UI->Group, FontID, At, FontHeight, UI->Tooltip, TextColor);
	}

	UI->Group = 0;
}

internal ui_layout
BeginUILayout(ui_state *UI, asset_name FontName, f32 Height, v4 Color, rectangle2 Rect)
{
	ui_layout Result = {};
	Result.UI = UI;
	Result.Font = GetFontID(UI->Group->Assets, FontName);
	Result.LineHeight = Height;
	Result.Color = Color;
	Result.AtY = V2(Rect.Min.x, Rect.Max.y);
	Result.AtY.y -= Result.LineHeight;
	Result.AtX = Result.AtY;
	Result.IndentWidth = Result.LineHeight;
	Result.Border = 0.3f*Result.LineHeight;
	Result.Rect = Rect;

	// PushQuad(Group, Rect.Min, Rect.Max, Color);

	return Result;
}

internal void
EndUILayout(ui_layout *Layout)
{

}

internal void
BeginRow(ui_layout *Layout)
{
	Layout->AtX = Layout->AtY;
	Layout->AtX.x += Layout->Indent*Layout->IndentWidth;
}

internal void
EndRow(ui_layout *Layout)
{
	Layout->AtY.y -= Layout->LineHeight;
}

internal void
BeginIndent(ui_layout *Layout)
{
	Layout->Indent++;
}

internal void
EndIndent(ui_layout *Layout)
{
	Assert(Layout->Indent > 0);
	Layout->Indent--;
}

internal void
Label(ui_layout *Layout, string Text)
{
	render_group *Group = Layout->UI->Group;
	Layout->AtX = PushText(Group, Layout->Font, Layout->AtX, Layout->LineHeight, Text, Layout->Color);
}

internal void
Labelf(ui_layout *Layout, char *Format, ...)
{
	char Buffer[256];
	va_list ArgList;
	va_start(ArgList, Format);
	string FormattedString = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
	va_end(ArgList);

	Label(Layout, FormattedString);
}

internal b32
Button(ui_layout *Layout, string Text, ui_id UIID, void *Identifier, b32 Enabled = true, v4 Color = V4(0.5f, 0.5f, 1, 1))
{
	render_group *Group = Layout->UI->Group;

	v2 TextDim = GetTextDim(Group, Layout->Font, Layout->LineHeight, Text);
	f32 Descent = GetFontDescent(Group, Layout->Font, Layout->LineHeight);
	v2 BottomLeft = Layout->AtX + V2(Layout->Border, -Descent);
	rectangle2 ButtonRect = Rectangle2(BottomLeft,
									   BottomLeft + V2(TextDim.x + 2.0f*Layout->Border, Layout->LineHeight));

	ui_interaction Interaction = {};
	Interaction.Type = UI_Interaction_InlineButtonPress;
	Interaction.ID = UIID;
	Interaction.Identifier = Identifier;

	b32 Selected = false;
	if(Enabled)
	{
		v2 MouseP = Layout->UI->MouseP;
		b32 MouseHover = InsideRectangle(ButtonRect, MouseP);
		if(MouseHover)
		{
			Layout->UI->HotInteraction = Interaction;
			if(Layout->UI->NextInteraction.Type == UI_Interaction_None)
			{
				Selected = true;
			}
			if(Layout->UI->LeftMousePressed)
			{
				Layout->UI->NextInteraction = Interaction;
			}
		}
		if(Layout->UI->NextInteraction == Interaction)
		{
			Selected = true;
		}
	}

	v4 ButtonColor = Color;
	if(Enabled && !Selected)
	{
		f32 ColorScale = 0.75f;
		ButtonColor.rgb = ColorScale*ButtonColor.rgb;
	}
	else if(!Enabled)
	{
		ButtonColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
	}

	PushQuad(Group, ButtonRect.Min, ButtonRect.Max, ButtonColor);
	Layout->AtX.x += 2.0f*Layout->Border;
	Label(Layout, Text);
	Layout->AtX.x += 2.0f*Layout->Border;

	b32 Result = false;
	if(Enabled &&
	  (Layout->UI->OperatingInteraction == Interaction))
	{
		Result = true;
	}

	return Result;
}

internal b32
Button(ui_layout *Layout, string Text, ui_id UIID, b32 Enabled = true)
{
	b32 Result = Button(Layout, Text, UIID, 0, Enabled);
	return Result;
}

internal b32
ButtonToggle(ui_layout *Layout, string Text, ui_id UIID, b32 Toggle, b32 Enabled = true)
{
	b32 Result = Button(Layout, Text, UIID, 0, Enabled, Toggle ? V4(0.5f, 0.5f, 1, 1) : V4(1.0f, 0.5f, 0.5f, 1));
	return Result;
}

internal void
EditableType(ui_layout *Layout, string Text, ui_id UIID, u32 *Type, u32 MaxType)
{
	if(Button(Layout, ConstZ("<"), UIID, (void *)0, true))
	{
		if(*Type == 0)
		{
			*Type = (MaxType - 1);
		}
		else
		{
			(*Type)--;
		}
	}
	if(Button(Layout, ConstZ(">"), UIID, (void *)1, true))
	{
		if(*Type == (MaxType - 1))
		{
			*Type = 0;
		}
		else
		{
			(*Type)++;
		}
	}
	Label(Layout, Text);
}

internal void
EditableU32(ui_layout *Layout, string Text, ui_id UIID, u32 *Value)
{
	char Buffer[128];
	string ButtonText = FormatString(Buffer, ArrayCount(Buffer), "%s : %u", Text, *Value);
	EditableType(Layout, ButtonText, UIID, Value, U32Maximum);
}

internal void
EditableS32(ui_layout *Layout, string Text, ui_id UIID, s32 *Value)
{
	char Buffer[128];
	string ButtonText = FormatString(Buffer, ArrayCount(Buffer), "%s : %d", Text, *Value);
	EditableType(Layout, ButtonText, UIID, (u32 *)Value, 0);
}

internal void
EditableF32(ui_layout *Layout, ui_id UIID, f32 *Value, f32 Min, f32 Max, b32 Enabled = true, void *Identifier = 0)
{
	ui_state *UI = Layout->UI;
	render_group *Group = Layout->UI->Group;

	char Buffer[32] = {};
	string ButtonText = FormatString(Buffer, ArrayCount(Buffer), "%f", *Value);

	v2 TextDim = GetTextDim(Group, Layout->Font, Layout->LineHeight, ButtonText);
	f32 Descent = GetFontDescent(Group, Layout->Font, Layout->LineHeight);
	v2 BottomLeft = Layout->AtX + V2(Layout->Border, -Descent);
	rectangle2 ButtonRect = Rectangle2(BottomLeft,
									   BottomLeft + V2(TextDim.x + 2.0f*Layout->Border, Layout->LineHeight));

	ui_interaction Interaction = {};
	Interaction.Type = UI_Interaction_InlineButtonPress;
	Interaction.ID = UIID;
	Interaction.Identifier = Identifier;

	b32 Selected = false;
	if(Enabled)
	{
		v2 MouseP = Layout->UI->MouseP;
		b32 MouseHover = InsideRectangle(ButtonRect, MouseP);
		if(MouseHover)
		{
			Layout->UI->HotInteraction = Interaction;
			if(Layout->UI->NextInteraction.Type == UI_Interaction_None)
			{
				Selected = true;
			}
			if(Layout->UI->LeftMousePressed)
			{
				Layout->UI->NextInteraction = Interaction;
			}
		}

		if(Layout->UI->NextInteraction == Interaction)
		{
			*Value = Clamp(*Value + 0.001f*UI->dMouseP.y, Min, Max);
			Selected = true;
		}
	}

	v4 ButtonColor = V4(0.5f, 0.5f, 1, 1);
	if(Enabled && !Selected)
	{
		f32 ColorScale = 0.75f;
		ButtonColor.rgb = ColorScale*ButtonColor.rgb;
	}
	else if(!Enabled)
	{
		ButtonColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
	}

	PushQuad(Group, ButtonRect.Min, ButtonRect.Max, ButtonColor);
	Layout->AtX.x += 2.0f*Layout->Border;
	Label(Layout, ButtonText);
	Layout->AtX.x += 2.0f*Layout->Border;
}

internal void
EditableV2(ui_layout *Layout, ui_id UIID, v2 *Value, f32 Min, f32 Max, b32 Enabled = true)
{
	EditableF32(Layout, UIID, &Value->x, Min, Max, true, (void *)0);
	EditableF32(Layout, UIID, &Value->y, Min, Max, true, (void *)1);
}

internal void
EditableV3(ui_layout *Layout, ui_id UIID, v3 *Value, f32 Min, f32 Max, b32 Enabled = true)
{
	EditableF32(Layout, UIID, &Value->x, Min, Max, true, (void *)0);
	EditableF32(Layout, UIID, &Value->y, Min, Max, true, (void *)1);
	EditableF32(Layout, UIID, &Value->z, Min, Max, true, (void *)2);
}

internal void
EditableV4(ui_layout *Layout, ui_id UIID, v4 *Value, f32 Min, f32 Max, b32 Enabled = true)
{
	EditableF32(Layout, UIID, &Value->x, Min, Max, true, (void *)0);
	EditableF32(Layout, UIID, &Value->y, Min, Max, true, (void *)1);
	EditableF32(Layout, UIID, &Value->z, Min, Max, true, (void *)2);
	EditableF32(Layout, UIID, &Value->w, Min, Max, true, (void *)3);
}

internal void
EditableQuaternion(ui_layout *Layout, ui_id UIID, quaternion *Value, b32 Enabled = true)
{
	f32 Min = -1.0f;
	f32 Max = 1.0f;
	EditableF32(Layout, UIID, &Value->r, Min, Max, true, (void *)0);
	EditableF32(Layout, UIID, &Value->i, Min, Max, true, (void *)1);
	EditableF32(Layout, UIID, &Value->j, Min, Max, true, (void *)2);
	EditableF32(Layout, UIID, &Value->k, Min, Max, true, (void *)3);
	*Value = NormalizeOrNoRotation(*Value);
}

internal void
Tooltip(ui_state *UI, string Text)
{
	Assert(Text.Length < ArrayCount(UI->TooltipBuffer));
	CopySize(Text.Text, UI->TooltipBuffer, Text.Length);
	UI->Tooltip.Text = UI->TooltipBuffer;
	UI->Tooltip.Length = Text.Length;
}

internal void
Tooltipf(ui_state *UI, char *Format, ...)
{
	char Buffer[256];
	va_list ArgList;
	va_start(ArgList, Format);
	string FormattedString = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
	va_end(ArgList);

	Tooltip(UI, FormattedString);
}

internal b32
Hoverf(ui_layout *Layout, char *Format, ...)
{
	char Buffer[256];
	va_list ArgList;
	va_start(ArgList, Format);
	string FormattedString = FormatStringList(Buffer, ArrayCount(Buffer), Format, ArgList);
	va_end(ArgList);

	rectangle2 TextRect = GetTextBounds(Layout->UI->Group, Layout->Font, Layout->AtX, Layout->LineHeight, FormattedString);
	b32 Result = InsideRectangle(TextRect, Layout->UI->MouseP);
	Label(Layout, FormattedString);
	return Result;
}
