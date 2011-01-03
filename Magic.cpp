/*
 *  Magic.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Mon Nov 15 2004.
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

#include <cassert>
#include <cstring>
//#include <iostream>
#include "SDL.h"
#include "nuvieDefs.h"
#include "U6misc.h"
#include "Party.h"
#include "MsgScroll.h"
#include "Configuration.h"
#include "NuvieIOFile.h"

#include "UseCode.h"

#include "GUI.h"
#include "GUI_YesNoDialog.h"

#include "Actor.h"
#include "ActorManager.h"
#include "ObjManager.h"
#include "U6objects.h"
#include "Magic.h"
#include "Game.h"
#include "misc/U6LList.h"
#include "Effect.h"
#include "Weather.h"
#include "Script.h"

#define MAGIC_ALL_SPELLS 255

/* Syllable            Meaning     Syllable            Meaning
 * An .......... Negate/Dispel     Nox ................ Poison
 * Bet ................. Small     Ort ................. Magic
 * Corp ................ Death     Por ......... Move/Movement
 * Des ............ Lower/Down     Quas ............. Illusion
 * Ex ................ Freedom     Rel ................ Change
 * Flam ................ Flame     Sanct .. Protect/Protection
 * Grav ......... Energy/Field     Tym .................. Time
 * Hur .................. Wind     Uus .............. Raise/Up
 * In ...... Make/Create/Cause     Vas ................. Great
 * Jux ...... Danger/Trap/Harm     Wis ........ Know/Knowledge
 * Kal ......... Summon/Invoke     Xen .............. Creature
 * Lor ................. Light     Ylem ............... Matter
 * Mani ......... Life/Healing     Zu .................. Sleep
 */

const char* syllable[26]={"An ", "Bet ", "Corp ", "Des ", "Ex ", "Flam ", "Grav ", "Hur ", "In ", "Jux ", "Kal ", "Lor ", "Mani ", "Nox ", "Ort ", "Por ", "Quas ", "Rel ", "Sanct ", "Tym ", "Uus ", "Vas ", "Wis ", "Xen ", "Ylem ", "Zu "};
const char* reagent[8]={"mandrake root", "nightshade", "black pearl", "blood moss", "spider silk", "garlic", "ginseng", "sulfurous ash"}; // check names
const int obj_n_reagent[8]={OBJ_U6_MANDRAKE_ROOT, OBJ_U6_NIGHTSHADE, OBJ_U6_BLACK_PEARL, OBJ_U6_BLOOD_MOSS, OBJ_U6_SPIDER_SILK, OBJ_U6_GARLIC, OBJ_U6_GINSENG, OBJ_U6_SULFUROUS_ASH};


Magic::Magic()
{
  magic_script = NULL;
  
 for (uint16 index=0;index<256;index++) spell[index]=NULL;
 clear_cast_buffer();
}

Magic::~Magic()
{
 for (uint16 index=0;index<256;index++) delete(spell[index]);
}

void Magic::init(Event *evt)
{
  event=evt;
  read_spell_list();
}

// FIXME This should be a member of the NuvieIOFileRead class.
bool Magic::read_line(NuvieIOFileRead *file, char *buf, size_t maxlen) {
  size_t buflen=0;

  for (buflen=0;buflen<maxlen;buflen++) 
  {
    buf[buflen]=char(file->read1());
    if (buf[buflen]=='\n') {
      if (buflen) // check for backslash
      {
	if (buf[buflen-1]=='\\')
	{
	  buf[--buflen]=' ';  // turn into a space, and ignore \n
	  continue;
	}
      }
      buf[buflen]='\0'; // end the line here
      return true; // no overflow.
    }
  }
  return false; // overflow.
}

bool Magic::read_script(NuvieIOFileRead *file, char *buf, size_t maxlen) {
  size_t buflen=0;
  
  for (buflen=0;buflen<maxlen;buflen++) 
  {
    buf[buflen]=char(file->read1());
    if (buf[buflen]=='\n') {
      if (buflen)
      {
        if (buf[buflen-1]=='~')
        {
          buf[buflen-1]='\0';
          file->seek(file->position() - 2); // wind back to the '~'
          return true;
        }
      }
    }
  }
  return false; // overflow.
}

void Magic::lc(char *str) // very crappy doesn't check much.
{
  for (char *conv=str;*conv!='\0';conv++)
  {
    if (*conv<'a') {*conv+=32;};
  }
}

void Magic::read_spell_list()
{
  uint16 index;
  bool complete=false;
  NuvieIOFileRead *spells=new(NuvieIOFileRead);
  char name[80]="";
  char invocation[26]="";
  uint8 reagents=0;
  char script[MAX_SCRIPT_LENGTH+1]="_end";
  char buf[MAX_SCRIPT_LENGTH+1];

  //read spell file from nuvie data directory.

  std::string datadir = GUI::get_gui()->get_data_dir();
  std::string spellfile;
  
  build_path(datadir, "spells.nsl", spellfile);
  
  if (!spells->open(spellfile)) 
  {
    throw "error opening spells.nsl";
  }
  
  /*read records into spell-array 1 by one.*/
  for (index=0;index<256;index++)
  {
    if (spell[index]) //clean up if something there already
    {
      delete spell[index]; 
    }
    complete=false;
    while (!complete)
    {
      /* read a line */ 
      if (!read_line(spells,buf,sizeof(buf)))
      {
	DEBUG(0,LEVEL_ERROR,"Line too long reading spell-list entry for spell %d:\n",index);
	throw "Buffer overflow\n";
      };
      /* parse the line */ // TODO far to quick and dirty...
      switch (buf[0]) 
      {
	case '~': // end of record, completed!
	  complete=true; break;
	case '#': // comment line, ignore
	  break;
	case 'n': // assume 'name: '
	  strncpy(name,buf+6,sizeof(name)-1);
	  break;
	case 'i': // assume 'invocation: '
	  strncpy(invocation,buf+12,sizeof(invocation)-1);
	  lc(invocation);
	  break;
	case 'r': // assume 'reagents: '
	    reagents=uint8(strtol(buf+10,(char **)NULL,0));
	  break;
	case 's': // assume 'script: '
    if(!read_script(spells,buf+7,sizeof(buf)-7))
      throw "Line too long reading lua script\n";
    
	  strncpy(script,buf+7,sizeof(script)-1);
	  break;
      }
    }
    /* store the result */
    spell[index]=new Spell(name,invocation,script,reagents);
  }
  /* close the spell-list-file*/
  spells->close();
  delete spells;
}

#if 0
bool Magic::handleSDL_KEYDOWN(const SDL_Event * sdl_event)
{
  if (state == MAGIC_STATE_SELECT_SPELL) {
    // TODO keys to handle escaping from this mode
    if (sdl_event->key.keysym.sym==SDLK_RETURN || sdl_event->key.keysym.sym==SDLK_KP_ENTER)
    {
      cast() ;

      event->scroll->display_string("\n");
      event->scroll->display_prompt();

      event->mode = MOVE_MODE;
      return true; // handled the event
    }
    if (sdl_event->key.keysym.sym>=SDLK_1 && sdl_event->key.keysym.sym<=SDLK_8)
    {
      cast(event->player->get_party()->get_actor(sdl_event->key.keysym.sym - 48-1));
      event->mode = MOVE_MODE;
      return true; // handled the event
    } 
    else if (sdl_event->key.keysym.sym>=SDLK_a && sdl_event->key.keysym.sym<=SDLK_z)
    {
      if (cast_buffer_len<4)
      { 
	cast_buffer_str[cast_buffer_len++]=sdl_event->key.keysym.sym;
	event->scroll->display_string(syllable[sdl_event->key.keysym.sym - SDLK_a]);
	return true; // handled the event
      }
      return true; // handled the event
    }
    else if (sdl_event->key.keysym.sym==SDLK_BACKSPACE)
    {
      if (cast_buffer_len>0)
      { 
	cast_buffer_len--; // back up a syllable FIXME, doesn't handle automatically inserted newlines, so we need to keep track more.
	size_t len=strlen(syllable[cast_buffer_str[cast_buffer_len]-SDLK_a]);
	while(len--) event->scroll->remove_char();
	event->scroll->Display(true);
	return true; // handled the event
      }
      return true; // handled the event
    }
  }
  return false; // didn't handle the event
} 
#endif

uint8 Magic::book_equipped()
{
  // book(s) equipped? Maybe should check all locations?
  Actor *caster=event->player->get_actor();
  Obj *right=caster->inventory_get_readied_object(ACTOR_ARM); 
  Obj *left=caster->inventory_get_readied_object(ACTOR_ARM_2); 
  uint8 books=0;

  if (right!=NULL && right->obj_n==OBJ_U6_SPELLBOOK) { books+=1; }; 
  if (left!=NULL && left->obj_n==OBJ_U6_SPELLBOOK) { books+=2; }; 
  
  return books;
}

bool Magic::start_new_spell() 
{
  if (book_equipped()) 
  {
    state=MAGIC_STATE_SELECT_SPELL;
    clear_cast_buffer();
    return true;
  }
  event->scroll->display_string("\nNo spellbook is readied.\n"); 
  state=MAGIC_STATE_READY;
  return false;
}


bool Magic::cast(Actor *Act)
{
  return cast();
}
bool Magic::cast(Obj *Obj)
{
  return cast();
}
bool Magic::cast()
{
  if(magic_script != NULL)
    return false;
  
  cast_buffer_str[cast_buffer_len]='\0';
  DEBUG(0,LEVEL_DEBUGGING,"Trying to cast '%s'\n",cast_buffer_str);
  /* decode the invocation */
  // FIXME? original allows random order of syllables, do we want that?

  uint16 index;
  for (index=0;index<256;index++) 
  {
    if (!strcmp(spell[index]->invocation,cast_buffer_str)) {
      break;
    }
  }
  if (index>=256) {
    DEBUG(0,LEVEL_DEBUGGING,"didn't find spell in spell list\n");
    event->scroll->display_string("\nThat spell is not in thy spellbook!\n"); 
    return false;
  }
  event->scroll->display_string("\n(");
  event->scroll->display_string(spell[index]->name);
  event->scroll->display_string(")\n");


  /* debug block */
  DEBUG(0,LEVEL_DEBUGGING,"matched spell #%d\n",index);
  DEBUG(0,LEVEL_DEBUGGING,"name: %s\n",spell[index]->name);
  DEBUG(0,LEVEL_DEBUGGING,"reagents: ");
  const char *comma="";
  for (uint8 shift=0;shift<8;shift++) 
  {
    if (1<<shift&spell[index]->reagents) {
      DEBUG(1,LEVEL_DEBUGGING,"%s%s",comma,reagent[shift]);
      comma=", ";
    }
  }
  DEBUG(1,LEVEL_DEBUGGING,"\n");
  DEBUG(0,LEVEL_DEBUGGING,"script: %s\n",spell[index]->script);
  /* end debug block */


  if(event->mode == WAIT_MODE)
    return false;
  
  // TODO "No magic at this time!" error. 
  
  // book(s) equipped? Maybe should check all locations?
  Actor *caster=event->player->get_actor();
  Obj *right=caster->inventory_get_readied_object(ACTOR_ARM); 
  Obj *left=caster->inventory_get_readied_object(ACTOR_ARM_2); 
  uint8 books=0;
  if (right!=NULL && right->obj_n==OBJ_U6_SPELLBOOK) { books+=1; }; 
  if (left!=NULL && left->obj_n==OBJ_U6_SPELLBOOK) { books+=2; }; 
  
  if(right && right->obj_n != OBJ_U6_SPELLBOOK)
    right = NULL;
  if(left && left->obj_n !=  OBJ_U6_SPELLBOOK)
    left = NULL;
  
  if(right == NULL && left == NULL)
  { 
    event->scroll->display_string("\nNo spellbook is readied.\n"); 
    return false;
  }

  // any spells available?
  uint32 spells=0;
  if ((books&1) && right->container) // hmm, relying on shortcut logic here.
  {
    spells=right->container->count();
  }
  if ((books&2) && left->container) 
  {
    spells+=left->container->count();
  }
  if (!spells)
  {
    event->scroll->display_string("\nNo spells in the spellbook.\n"); 
    return false;
  }
  
  // spell (or catch all spell 255) in (one of the) book(s)? 
  if(spellbook_has_spell(right,index) == false && spellbook_has_spell(left,index) == false) 
  {
    event->scroll->display_string("\nThat spell is not in thy spellbook!\n"); 
    return false;
  }

  // level of caster sufficient
  uint8 spell_level=MIN(8,(index/16)+1); 
  if (caster->get_level()<spell_level)
  {
    event->scroll->display_string("\nYour level is not high enough.\n"); 
    return false;
  }
  
  // enough Magic Points available
  if (caster->get_magic()<spell_level)
  {
    event->scroll->display_string("\nNot enough magic points.\n");
    return false;
  }

  
  // reagents available
  for (uint8 shift=0;shift<8;shift++) 
  {
    if (1<<shift&spell[index]->reagents) {
      if (!caster->inventory_has_object(obj_n_reagent[shift],0,false))
      {
	DEBUG(0,LEVEL_DEBUGGING,"Didn't have %s\n",reagent[shift]);
	event->scroll->display_string("\nNo Reagents.\n");
	return false;
      }
      DEBUG(0,LEVEL_DEBUGGING,"Ok, has %s\n",reagent[shift]);
    }
  }
    
  /* TODO check all pre-requisites before continue */
  // 'spell failed' because of bad luck
  // anything else?
  
  // consume the reagents and magic points; we checked so they must be there.
  caster->set_magic(caster->get_magic()-spell_level); // add a MAX (0, here?
  
  for (uint8 shift=0;shift<8;shift++) 
  {
    if (1<<shift&spell[index]->reagents)
    {
      // FIXME Although we just checked, maybe something is messed up, so we
      // should probably check that we're not passing NULL to delete_obj
      caster->inventory_del_object(obj_n_reagent[shift],1,0);
    }
  }
  
  string lua = "run_magic_script(\"";
  lua += spell[index]->invocation;
  lua += "\")";

  magic_script = Game::get_game()->get_script()->new_thread_from_string(lua.c_str());

  if(magic_script)
    process_script_return(magic_script->start());

  return true;
}

bool Magic::resume(MapCoord location)
{
	if(magic_script)
	{
		process_script_return(magic_script->resume_with_location(location));
	}

	return true;
}

bool Magic::spellbook_has_spell(Obj *book, uint8 spell_index)
{
  if(!book)
    return false;
  
  if(book->find_in_container(OBJ_U6_SPELL, MAGIC_ALL_SPELLS, OBJ_MATCH_QUALITY) ||
     book->find_in_container(OBJ_U6_SPELL, spell_index, OBJ_MATCH_QUALITY))
  {
    return true;
  }
  
  return false;
}

bool Magic::process_script_return(uint8 ret)
{
  if(ret == NUVIE_SCRIPT_ERROR)
  {
    delete magic_script; magic_script = NULL;
    return false;
  }
  
  uint32 nturns; uint8 *cb_msgid;
  switch(ret)
  {
    case NUVIE_SCRIPT_FINISHED : delete magic_script; magic_script = NULL; break;
    case NUVIE_SCRIPT_GET_TARGET : DEBUG(0,LEVEL_NOTIFICATION,"FIXME: Register Event get target callback here\n"); break;
    case NUVIE_SCRIPT_GET_DIRECTION : DEBUG(0,LEVEL_NOTIFICATION,"FIXME: Register Event get direction callback here\n"); break;
    case NUVIE_SCRIPT_GET_INV_OBJ : DEBUG(0,LEVEL_NOTIFICATION,"FIXME: Register Event get inv object callback here\n"); break;
      
    case NUVIE_SCRIPT_ADVANCE_GAME_TIME : nturns = magic_script->get_data();
      DEBUG(0,LEVEL_DEBUGGING,"Magic: Advance %d turns\n",nturns);
      cb_msgid = new uint8;
      *cb_msgid = NUVIE_SCRIPT_CB_ADV_GAME_TIME;
      new GameTimedCallback((CallBack *)this, cb_msgid, nturns);
      break;
      
    default : DEBUG(0,LEVEL_WARNING,"Unknown ScriptThread return code!\n"); break;
  }
  
  return true;
}

uint16 Magic::callback(uint16 msg, CallBack *caller, void *data)
{
    if(msg == CB_DATA_READY)
    {
        if(event->input.type != EVENTINPUT_KEY)
            return 0;
        SDLKey sym = event->input.key;

        if(state == MAGIC_STATE_SELECT_SPELL) {
            if(sym>=SDLK_a && sym<=SDLK_z)
            {
                if(cast_buffer_len<4)
                { 
                	cast_buffer_str[cast_buffer_len++]=sym;
                	event->scroll->display_string(syllable[sym - SDLK_a]);
                	return 1; // handled the event
                }
                return 1; // handled the event
            }
            else if(sym==SDLK_BACKSPACE)
            {
                if(cast_buffer_len>0)
                { 
                  	cast_buffer_len--; // back up a syllable FIXME, doesn't handle automatically inserted newlines, so we need to keep track more. (THAT SHOULD BE DONE BY MSGSCROLL)
                    size_t len=strlen(syllable[cast_buffer_str[cast_buffer_len]-SDLK_a]);
                	while(len--) event->scroll->remove_char();
                    	event->scroll->Display(true);
                	return 1; // handled the event
                }
                return 1; // handled the event
            }
        } // MAGIC_STATE_SELECT_SPELL
        if(state == MAGIC_STATE_ACQUIRE_TARGET) {
            if(sym>=SDLK_1 && sym<=SDLK_8)
            {
                cast(event->player->get_party()->get_actor(sym - 48-1));
                event->cancel_key_redirect();
                return 1; // handled the event
            } 
        }

        // We must handle all keys even those we may not want or else
        // we'll lose input focus, except for these three which end
        // Casting. (besides, not handling all keys means they go back
        // to global which could start another action)
        if(sym != SDLK_RETURN && sym != SDLK_ESCAPE && sym != SDLK_SPACE)
            return 1;
        return 0;
    }
    else if(magic_script)
    {
        switch(msg)
        {
            case NUVIE_SCRIPT_GET_TARGET : process_script_return(magic_script->resume_with_location(MapCoord(200,200,0))); //FIXME need to get real loc.
                break;
            case NUVIE_SCRIPT_GET_DIRECTION : process_script_return(magic_script->resume_with_direction(NUVIE_DIR_N)); //FIXME need to get real dir.
                break;
        }
    }
  
  return 1;
}
