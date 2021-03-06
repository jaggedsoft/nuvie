/*
 *  MapEditorView.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Wed Mar 28 2012.
 *  Copyright (c) 2012. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "nuvieDefs.h"
#include "U6misc.h"
#include "GUI.h"
#include "GUI_button.h"
#include "Configuration.h"
#include "Map.h"
#include "MapWindow.h"
#include "ViewManager.h"
#include "MapEditorView.h"
#include "Keys.h"

#define TILES_W 5
#define TILES_H 10

MapEditorView::MapEditorView(Configuration *config)
: View(config)
{
 roof_tiles = NULL; map_window = NULL; up_button = NULL; down_button = NULL;
}

MapEditorView::~MapEditorView()
{
}

bool MapEditorView::init(Screen *tmp_screen, void *view_manager, uint16 x, uint16 y, Font *f, Party *p, TileManager *tm, ObjManager *om)
{
	View::init(x,y,f,p,tm,om);

	SetRect(area.x, area.y, 90, 200);
	bg_color = 119;

	std::string datadir = GUI::get_gui()->get_data_dir();
	std::string path;

	build_path(datadir, "images", path);
	datadir = path;
	build_path(datadir, "gumps", path);
	datadir = path;
	build_path(datadir, "mapeditor", path);
	datadir = path;

	up_button = loadButton(datadir, "up", 0, 7);
	down_button = loadButton(datadir, "down", 0, 186);

	map_window = Game::get_game()->get_map_window();
	roof_tiles = map_window->get_roof_tiles();

	map_window->set_show_cursor(true);
	map_window->moveCursor(7,6);
	map_window->set_roof_display_mode(ROOF_DISPLAY_FORCE_ON);
	map_window->set_enable_blacking(false);
	map_window->set_show_grid(false);

	Game::get_game()->set_mouse_pointer(1); //crosshairs

	tile_offset = 0;
	selectedTile = 3;
	return true;
}

void MapEditorView::Display(bool full_redraw)
{
	SDL_Rect src, dst;
	src.w = 16;
	src.h = 16;
	dst.w = 16;
	dst.h = 16;

	screen->fill(bg_color, area.x, area.y, area.w, area.h);

	DisplayChildren(full_redraw);

	uint16 tile_num = tile_offset;

	for(int i=0;i< TILES_H;i++)
	{
		for(int j=0;j<TILES_W;j++)
		{
			dst.x = area.x + 3 + (j*17);
			dst.y = area.y + 16 + (i*17);

			src.x = (tile_num % MAPWINDOW_ROOFTILES_IMG_W) * 16;
			src.y = (tile_num / MAPWINDOW_ROOFTILES_IMG_W) * 16;

			if(tile_num == selectedTile)
				screen->fill(15, dst.x-1,dst.y-1, 18, 18);

			SDL_BlitSurface(roof_tiles, &src, surface, &dst);
			tile_num++;
		}
	}
	screen->update(area.x, area.y, area.w, area.h);
}

GUI_status MapEditorView::KeyDown(SDL_keysym key)
{
	MapCoord loc;
	uint16 *roof_data;
	KeyBinder *keybinder = Game::get_game()->get_keybinder();
	ActionType a = keybinder->get_ActionType(key);
	ActionKeyType action_type = keybinder->GetActionKeyType(a);

	// alt input
	if(key.mod & KMOD_ALT)
	{
		SDL_keysym key_without_alt = key; // need to see what action is without alt
		uint16 mod_without_alt = key_without_alt.mod; // this and next 2 lines are due SDLMod not wanting to do the bitwise ~ operation
		mod_without_alt &= ~KMOD_ALT;
		key_without_alt.mod = (SDLMod)mod_without_alt;
		ActionType action_without_alt = keybinder->get_ActionType(key_without_alt);

		if(keybinder->GetActionKeyType(action_without_alt) <= SOUTH_WEST_KEY)
			action_type = keybinder->GetActionKeyType(action_without_alt);

		switch(action_type)
		{
		case NORTH_KEY:
			if(selectedTile >= TILES_W)
				update_selected_tile_relative(-TILES_W);
			return GUI_YUM;
		case SOUTH_KEY:
			update_selected_tile_relative(TILES_W);
			return GUI_YUM;
		case WEST_KEY:
			update_selected_tile_relative(-1);
			return GUI_YUM;
		case EAST_KEY:
			update_selected_tile_relative(1);
			return GUI_YUM;
		case SOUTH_WEST_KEY:
			update_selected_tile_relative(TILES_W);
			if(selectedTile%TILES_W) // don't wrap
				update_selected_tile_relative(-1);
			return GUI_YUM;
		case SOUTH_EAST_KEY:
			update_selected_tile_relative(TILES_W);
			if((selectedTile+1)%TILES_W) // don't wrap
				update_selected_tile_relative(1);
			return GUI_YUM;
		case NORTH_WEST_KEY:
			if(selectedTile >= TILES_W)
				update_selected_tile_relative(-TILES_W);
			if(selectedTile%TILES_W) // don't wrap
					update_selected_tile_relative(-1);
			return GUI_YUM;
		case NORTH_EAST_KEY:
			if(selectedTile >= TILES_W)
				update_selected_tile_relative(-TILES_W);
			if((selectedTile+1)%TILES_W) // don't wrap
				update_selected_tile_relative(1);
			return GUI_YUM;
		default:
			break;
		}
	}
	else if(key.mod == KMOD_NONE)
	{
		if(key.sym == SDLK_g) {
			toggleGrid();
			return GUI_YUM;
		} else if(key.sym == SDLK_s){
			Game::get_game()->get_game_map()->saveRoofData();
			return GUI_YUM;
		}
	}

    switch(action_type)
    {
    	case MSGSCROLL_UP_KEY:
    		update_selected_tile_relative(-(TILES_W * TILES_H));
    		break;
    	case MSGSCROLL_DOWN_KEY:
    		update_selected_tile_relative(TILES_W * TILES_H);
    		break;
        case SOUTH_WEST_KEY:
        	map_window->moveMapRelative(-1,1);
            break;
        case SOUTH_EAST_KEY:
        	map_window->moveMapRelative(1,1);
            break;
        case NORTH_WEST_KEY:
        	map_window->moveMapRelative(-1,-1);
            break;
        case NORTH_EAST_KEY:
        	map_window->moveMapRelative(1,-1);
            break;

        case NORTH_KEY:
        	map_window->moveMapRelative(0,-1);
            break;
        case SOUTH_KEY:
        	map_window->moveMapRelative(0,1);
            break;
        case WEST_KEY:
        	map_window->moveMapRelative(-1, 0);
            break;
        case EAST_KEY:
        	map_window->moveMapRelative(1, 0);
            break;
        case DO_ACTION_KEY:

        	loc = map_window->get_cursorCoord();
        	roof_data = Game::get_game()->get_game_map()->get_roof_data(loc.z);
        	if(roof_data)
        	{
        		roof_data[loc.y * 1024 + loc.x] = selectedTile;
        	}
            break;
        case TOGGLE_CURSOR_KEY :

        	break;
        case HOME_KEY:
        	selectedTile = 0;
        	tile_offset = 0;
        	break;
        case END_KEY:
        	selectedTile = MAPWINDOW_ROOFTILES_IMG_W * MAPWINDOW_ROOFTILES_IMG_H -1;
        	tile_offset = (TILES_W * TILES_H) * (selectedTile / (TILES_W * TILES_H));
        	break;
        case CANCEL_ACTION_KEY:
        	//Game::get_game()->get_view_manager()->close_gump(this);
        	close_view();
        	GUI::get_gui()->removeWidget((GUI_Widget *)this);
        	break;
        default:
            keybinder->handle_always_available_keys(a);
            break; // was GUI_PASS pefore action_type change
    }
    return(GUI_YUM);
}

GUI_status MapEditorView::MouseDown(int x, int y, int button)
{

 return GUI_YUM;
}

GUI_status MapEditorView::MouseUp(int x, int y, int button)
{

 if(SDL_BUTTON(button) & SDL_BUTTON_RMASK)
 {
 	//Game::get_game()->get_view_manager()->close_gump(this);
	close_view();
	GUI::get_gui()->removeWidget((GUI_Widget *)this);
 }
 else if(HitRect(x, y))
 {
	 x -= area.x;
	 x -= 3;
	 y -= area.y;
	 y -= 16;
	 selectedTile = tile_offset + (y / 17) * TILES_W + (x / 17);
 }
 else
 {
	 int wx, wy;
	 uint8 level;
	 map_window->get_level(&level);
	map_window->mouseToWorldCoords(x, y, wx, wy);
	setTile((uint16)wx, (uint16)wy, level);

 }

 return GUI_YUM;
}

GUI_status MapEditorView::MouseMotion(int x,int y,Uint8 state)
{
	return GUI_YUM;
}

void MapEditorView::setTile(uint16 x, uint16 y, uint8 level)
{
	uint16 *roof_data = Game::get_game()->get_game_map()->get_roof_data(level);
        	if(roof_data)
        	{
        		roof_data[y * ((level == 0) ? 1024 : 256) + x] = selectedTile;
        	}
}

void MapEditorView::toggleGrid()
{
	map_window->set_show_grid(!map_window->is_grid_showing());
}

void MapEditorView::close_view()
{
	map_window->set_show_cursor(false);
	map_window->set_roof_display_mode(ROOF_DISPLAY_NORMAL);
	map_window->set_enable_blacking(true);
	map_window->set_show_grid(false);
	release_focus();
	Hide();
}

void MapEditorView::update_selected_tile_relative(sint32 rel_value)
{
	if((sint32)selectedTile + rel_value < 0 || (sint32)selectedTile + rel_value >= (MAPWINDOW_ROOFTILES_IMG_W * MAPWINDOW_ROOFTILES_IMG_H))
		return;

	selectedTile = selectedTile + rel_value;
	tile_offset = (TILES_W * TILES_H) * (selectedTile / (TILES_W * TILES_H));
}

GUI_status MapEditorView::callback(uint16 msg, GUI_CallBack *caller, void *data)
{
	if(caller == up_button)
	{
		update_selected_tile_relative(-(TILES_W * TILES_H));
		return GUI_YUM;
	}
	else if(caller == down_button)
	{
		update_selected_tile_relative(TILES_W * TILES_H);
		return GUI_YUM;
	}
    return GUI_PASS;
}
