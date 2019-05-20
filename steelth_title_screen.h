/*@H
* File: steelth_title_screen.h
* Author: Jesse Calvert
* Created: May 21, 2018, 15:56
* Last modified: April 10, 2019, 17:31
*/

#pragma once

struct game_mode_title_screen
{
	v4 BackgroundColor;
	v4 TextColor;
};

internal void PlayTitleScreen(game_state *GameState);
