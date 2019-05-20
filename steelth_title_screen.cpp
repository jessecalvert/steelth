/*@H
* File: steelth_title_screen.cpp
* Author: Jesse Calvert
* Created: May 21, 2018, 15:56
* Last modified: April 10, 2019, 18:21
*/

internal void
PlayTitleScreen(game_state *GameState)
{
	SetGameMode(GameState, GameMode_TitleScreen);

	game_mode_title_screen *TitleScreen = GameState->TitleScreen = PushStruct(&GameState->ModeRegion, game_mode_title_screen);

	TitleScreen->BackgroundColor = V4(0.1f, 0.5f, 0.6f, 1.0f);
	TitleScreen->TextColor = V4(0.6f, 0.4f, 0.3f, 1.0f);
}

internal void
UpdateAndRenderTitleScreen(game_state *GameState, game_mode_title_screen *TitleScreen,
	game_render_commands *RenderCommands, game_input *Input)
{
	v4 SceneClearColor = {1.0f, 0.2f, 0.2f, 1.0f};
	render_group SceneGroup = BeginRender(GameState->Assets, RenderCommands, Target_Scene, SceneClearColor);
	render_group *RenderGroup = &SceneGroup;

	camera WorldCamera = {};
	WorldCamera.Type = Camera_Perspective;
	SetCamera(RenderGroup, &WorldCamera);

	EndRender(RenderGroup);

	render_group OverlayGroup = BeginRender(GameState->Assets, RenderCommands, Target_Overlay, TitleScreen->BackgroundColor);
	RenderGroup = &OverlayGroup;

	camera ScreenCamera = DefaultScreenCamera(Input->ScreenResolution);
	SetCamera(RenderGroup, &ScreenCamera);
	SetRenderFlags(RenderGroup, RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost|RenderFlag_SDF);

	font_id TimesFont = GetFontID(RenderGroup->Assets, Asset_TimesFont);
	f32 FontHeight = 100.0f;
	v4 TextColor = TitleScreen->TextColor;
	v2 TextAt = {50.0f, 6*FontHeight};

	char Buffer[128];
	string Text = FormatString(Buffer, ArrayCount(Buffer), "Press N for a new game!");
	PushText(RenderGroup, TimesFont, TextAt, FontHeight, Text, TextColor);

	EndRender(RenderGroup);

	if(Input->KeyStates[Key_N].WasPressed)
	{
		PlayWorldMode(GameState);
	}
}
