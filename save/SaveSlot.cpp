/*
 *  SaveSlot.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Wed May 12 2004.
 *  Copyright (c) 2004. All rights reserved.
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

#include "SDL.h"
#include "nuvieDefs.h"
#include "U6misc.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_text.h"
#include "GUI_CallBack.h"

#include "Game.h"
#include "NuvieIOFile.h"
#include "MapWindow.h"

#include "SaveGame.h"
#include "SaveSlot.h"


SaveSlot::SaveSlot(GUI_CallBack *callback, GUI_Color bg_color) : GUI_Widget(NULL, 0, 0, 266, NUVIE_SAVESLOT_HEIGHT)
{
 callback_object = callback;
 background_color = bg_color;
 
 selected = false;
 thumbnail = NULL;
}

SaveSlot::~SaveSlot()
{
 if(thumbnail)
  SDL_FreeSurface(thumbnail);
}

bool SaveSlot::init(const char *directory, std::string *file)
{ 
 GUI_Widget *widget;
 GUI *gui = GUI::get_gui();

 if(file != NULL)
  {
   filename.assign(file->c_str());
   new_save = false;
  }
 else
  {
   filename.assign(""); //empty save slot.
   new_save = true;
  }

 if(new_save)
   {
    widget = (GUI_Widget *) new GUI_Text(MAPWINDOW_THUMBNAIL_SIZE + 2, 2, 255, 255, 255, "New Save.", gui->get_font());
    AddWidget(widget);
   }

 if(!new_save)
   {
    load_info(directory);
   }

 return true;
}

bool SaveSlot::load_info(const char *directory)
{
 NuvieIOFileRead loadfile;
 SaveGame *savegame;
 SaveHeader *header;
 GUI_Widget *widget;
 GUI *gui = GUI::get_gui();
 std::string full_path;
 char buf[30];
 
 savegame = new SaveGame(Game::get_game()->get_config());
 
 build_path(directory, filename.c_str(), full_path);
 
 if(loadfile.open(full_path.c_str()) == false)
   {
    printf("Error: Reading header from %s\n", filename.c_str());
    delete savegame;
    return false;
   }
 
 header = savegame->load_info(&loadfile);
 
 thumbnail = SDL_ConvertSurface(header->thumbnail, header->thumbnail->format, SDL_SWSURFACE);

 widget = (GUI_Widget *) new GUI_Text(MAPWINDOW_THUMBNAIL_SIZE + 2, 2, 255, 255, 255, (char *)header->save_description.c_str(), gui->get_font());
 AddWidget(widget);
 
 widget = (GUI_Widget *) new GUI_Text(MAPWINDOW_THUMBNAIL_SIZE + 2, 20, 200, 200, 200, (char *)filename.c_str(), gui->get_font()); //evil const cast lets remove this
 AddWidget(widget);
 
 sprintf(buf, "Num Saves: %d", header->num_saves);
 widget = (GUI_Widget *) new GUI_Text(MAPWINDOW_THUMBNAIL_SIZE + 2, 29, 200, 200, 200, buf, gui->get_font());
 AddWidget(widget);

 delete savegame;
 
 return true;
}

std::string *SaveSlot::get_filename()
{
 return &filename;
}

/* Map the color to the display */
void SaveSlot::SetDisplay(Screen *s)
{
	GUI_Widget::SetDisplay(s);
	background_color.map_color(surface);
}

void SaveSlot::Display(bool full_redraw)
{
 SDL_Rect framerect = area;
 SDL_Rect destrect = area;
 if(selected)
   {
    GUI *gui = GUI::get_gui();
    SDL_FillRect(surface, &framerect, gui->get_selected_color()->sdl_color);
   }
 else
   SDL_FillRect(surface, &framerect, background_color.sdl_color);

 if(thumbnail)
   {
    SDL_BlitSurface(thumbnail, NULL, surface, &destrect);
   }

 DisplayChildren();
 
}

GUI_status SaveSlot::KeyDown(SDL_keysym key)
{

 //if(key.sym == SDLK_ESCAPE)
 //  return GUI_YUM;

 /*
 return no_callback_object->callback(YESNODIALOG_CB_NO, this, this);
 */
 return GUI_PASS;
}

GUI_status SaveSlot::MouseDown(int x, int y, int button)
{
 if(selected != true)
   {
    selected = true;
    callback_object->callback(SAVESLOT_CB_SELECTED, this, NULL);
   }

 return GUI_YUM;
}

GUI_status SaveSlot::MouseUp(int x, int y, int button)
{
 return GUI_PASS;
}

GUI_status SaveSlot::callback(uint16 msg, GUI_CallBack *caller, void *data)
{
 //if(caller == (GUI_CallBack *)cancel_button)
 //   return close_dialog();

/*
 if(caller == (GUI_CallBack *)no_button)
   return no_callback_object->callback(YESNODIALOG_CB_NO, this, this);
*/
 return GUI_PASS;
}