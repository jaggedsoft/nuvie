/*
 *  Script.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Wed Aug 23 2006.
 *  Copyright (c) 2006. All rights reserved.
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

#include <list>
#include <cassert>
#include "nuvieDefs.h"
#include "Configuration.h"
#include "U6misc.h"

#include "Game.h"
#include "Effect.h"
#include "MsgScroll.h"
#include "Player.h"
#include "ActorManager.h"
#include "Actor.h"
#include "Weather.h"
#include "UseCode.h"

#include "Script.h"
extern "C"
{
#include "lualib.h"
}

struct ScriptObjRef
{
  uint16 refcount;
  iAVLKey key;
  
  ScriptObjRef() { refcount = 0; };
};

static iAVLTree *script_obj_list;

static iAVLKey get_iAVLKey(const void *item)
{
  return ((ScriptObjRef *)item)->key;
}

// used for garbage collection.
//returns current object reference count. Or -1 on error.
static sint32 nscript_inc_obj_ref_count(Obj *obj);
static sint32 nscript_dec_obj_ref_count(Obj *obj);

static bool nscript_get_location_from_args(lua_State *L, uint16 *x, uint16 *y, uint8 *z, int lua_stack_offset=1);

inline bool nscript_obj_init_from_obj(lua_State *L, Obj *dst_obj);
inline bool nscript_obj_init_from_args(lua_State *L, int nargs, Obj *s_obj);
static int nscript_obj_new(lua_State *L);
static int nscript_obj_new(lua_State *L, Obj *obj);
static int nscript_obj_gc(lua_State *L);
static int nscript_obj_get(lua_State *L);
static int nscript_obj_set(lua_State *L);
static int nscript_obj_movetomap(lua_State *L);
static int nscript_obj_movetoinv(lua_State *L);
static int nscript_obj_movetocont(lua_State *L);
static int nscript_obj_use(lua_State *L);
static int nscript_obj_removefromengine(lua_State *L);

static int nscript_container_remove_obj(lua_State *L);

static const struct luaL_Reg nscript_objlib_f[] = 
{ 
  { "new", nscript_obj_new },
  { "moveToMap", nscript_obj_movetomap }, 
  { "moveToInv", nscript_obj_movetoinv },
  { "moveToCont", nscript_obj_movetocont },
  { "removeFromCont", nscript_container_remove_obj },
  { "use", nscript_obj_use },
  
  { NULL, NULL }
};
static const struct luaL_Reg nscript_objlib_m[] =
{
  { "__index", nscript_obj_get },
  { "__newindex", nscript_obj_set },
  { "__gc", nscript_obj_gc },
  { NULL, NULL }
};

static bool nscript_container_new(lua_State *L, Obj *parent_obj);
static int nscript_container_gc(lua_State *L);
static int nscript_container_get(lua_State *L);
static int nscript_container_length(lua_State *L);

static const struct luaL_Reg nscript_containerlib_m[] =
{
  { "__len", nscript_container_length },
  { "__index", nscript_container_get },
  { "__gc", nscript_container_gc },
  { NULL, NULL }
};

bool nscript_new_actor_var(lua_State *L, uint16 actor_num);

static int nscript_actor_new(lua_State *L);
static int nscript_get_actor_from_num(lua_State *L);

inline Actor *nscript_get_actor_from_args(lua_State *L, int lua_stack_offset=1);
static int nscript_actor_set(lua_State *L);
static int nscript_actor_get(lua_State *L);
static int nscript_actor_kill(lua_State *L);
static int nscript_actor_hit(lua_State *L);
static int nscript_actor_resurrect(lua_State *L);
static int nscript_actor_move(lua_State *L);

static const struct luaL_Reg nscript_actorlib_f[] = 
{ 
  { "new", nscript_actor_new },
  { "kill", nscript_actor_kill }, 
  { "hit", nscript_actor_hit },
  { "resurrect", nscript_actor_resurrect },
  { "move", nscript_actor_move },
  { "get", nscript_get_actor_from_num },
  { NULL, NULL }
};
static const struct luaL_Reg nscript_actorlib_m[] =
{
  { "__index", nscript_actor_get },
  { "__newindex", nscript_actor_set },
  { NULL, NULL }
};

static int nscript_print(lua_State *L);
//no longer used -- static int nscript_get_target(lua_State *L);
static int nscript_load(lua_State *L);

static int nscript_player_get_location(lua_State *L);

//obj manager
static int nscript_map_get_obj(lua_State *L);
static int nscript_map_remove_obj(lua_State *L);
static int nscript_map_get_actor(lua_State *L);

//Actor
static int nscript_actor_poison(lua_State *L);
static int nscript_actor_add_hp(lua_State *L);
static int nscript_actor_add_mp(lua_State *L);

//Misc
static int nscript_eclipse_start(lua_State *L);
static int nscript_quake_start(lua_State *L);
static int nscript_explosion_start(lua_State *L);

Script *Script::script = NULL;

uint8 ScriptThread::resume_with_location(MapCoord loc)
{
  lua_newtable(L);
  lua_pushstring(L,"x");
  lua_pushinteger(L,loc.x);
  lua_settable(L, -3);
  
  lua_pushstring(L,"y");
  lua_pushinteger(L,loc.y);
  lua_settable(L, -3);
  
  lua_pushstring(L,"z");
  lua_pushinteger(L,loc.z);
  lua_settable(L, -3);
  
  return resume(1);
}

uint8 ScriptThread::resume_with_direction(uint8 dir)
{
  lua_pushinteger(L,dir);
  
  return resume(1);
}

uint8 ScriptThread::resume(int narg)
{
  const char *s;
  int ret = lua_resume(L, narg);
  
  if(ret == 0)
  {
    lua_gc(L, LUA_GCCOLLECT,0); //FIXME! How often should we collect the garbage?
    return NUVIE_SCRIPT_FINISHED;
  }
  
  if(ret == LUA_YIELD)
  {
    if(lua_gettop(L) >= 1)
    {
      s = lua_tostring(L, 1);
      if(s)
      {
        if(!strcmp(s, "target"))
          return NUVIE_SCRIPT_GET_TARGET;
        if(!strcmp(s, "dir"))
          return NUVIE_SCRIPT_GET_DIRECTION;
        if(!strcmp(s, "adv_game_time"))
        {
          if(lua_gettop(L) < 2)
            data = 0;
          data = lua_tointeger(L,2);
          return NUVIE_SCRIPT_ADVANCE_GAME_TIME;
        }
      }
    }
  }
  
  DEBUG(0,LEVEL_ERROR,"%s\n", lua_tostring(L, lua_gettop(L)));
  
  return NUVIE_SCRIPT_ERROR;
}


Script::Script(Configuration *cfg, nuvie_game_t type)
{
  const char *path;
  size_t len;
  config = cfg;
  gametype = type;
  script = this;

  script_obj_list = iAVLAllocTree(get_iAVLKey);
  
  L = lua_open();
  luaL_openlibs(L); 

  luaL_newmetatable(L,"nuvie.Obj");
  //lua_pushvalue(L, -1); //duplicate metatable
  //lua_setfield(L, -2, "__index"); // add __index to metatable
  luaL_register(L, NULL, nscript_objlib_m);
  
  luaL_register(L, "Obj", nscript_objlib_f);

  luaL_newmetatable(L,"nuvie.Container");
  luaL_register(L, NULL, nscript_containerlib_m);

  luaL_newmetatable(L,"nuvie.Actor");

  luaL_register(L, NULL, nscript_actorlib_m);
  
  luaL_register(L, "Actor", nscript_actorlib_f);
  
  lua_pushcfunction(L, nscript_load);
  lua_setglobal(L, "nuvie_load");

  lua_pushcfunction(L, nscript_print);
  lua_setglobal(L, "print");

  lua_pushcfunction(L, nscript_map_get_obj);
  lua_setglobal(L, "map_get_obj");

  lua_pushcfunction(L, nscript_map_remove_obj);
  lua_setglobal(L, "map_remove_obj");

  lua_pushcfunction(L, nscript_map_get_actor);
  lua_setglobal(L, "map_get_actor");
  
  lua_pushcfunction(L, nscript_actor_poison);
  lua_setglobal(L, "actor_poison");
  lua_pushcfunction(L, nscript_actor_add_hp);
  lua_setglobal(L, "actor_add_hp");
  lua_pushcfunction(L, nscript_actor_add_mp);
  lua_setglobal(L, "actor_add_mp");
  
  lua_pushcfunction(L, nscript_player_get_location);
  lua_setglobal(L, "player_get_location");
 
  lua_pushcfunction(L, nscript_eclipse_start);
  lua_setglobal(L, "eclipse_start");
 
  lua_pushcfunction(L, nscript_quake_start);
  lua_setglobal(L, "quake_start");
 
  lua_pushcfunction(L, nscript_explosion_start);
  lua_setglobal(L, "explosion_start");
 
  seed_random();
 
  lua_getglobal(L, "package");
  lua_pushstring(L, "path");
  lua_gettable(L, -2);
 
  path = lua_tolstring(L,-1,&len);
  DEBUG(0,LEVEL_INFORMATIONAL,"lua path = %s\n", path);
 
  run_script("init = nuvie_load(\"u6/init.lua\"); init()");
 
}

Script::~Script()
{
 if(L)
   lua_close(L);
}


void Script::seed_random()
{
  //Seed the lua random number generator.
  //seed with a random number from NUVIE_RAND()
  //this should be seeded at this point.
  
  lua_getfield(L, LUA_GLOBALSINDEX, "math");
  lua_getfield(L, -1, "randomseed");
  lua_remove(L, -2);
  lua_pushnumber(L, NUVIE_RAND());
  lua_pcall(L, 1, 0, 0);

  return;
}

bool Script::run_script(const char *script)
{
  
  if(luaL_dostring(L,script) != 0)
  {
    DEBUG(0,LEVEL_ERROR,"Script Error: %s\n", luaL_checkstring(L,-1));
  }
  
  return true;
}

ScriptThread *Script::new_thread(const char *scriptfile)
{
  ScriptThread *t = NULL;
  lua_State *s;
  
  s = lua_newthread(L);
  lua_getglobal(s, "run_script");
  lua_pushstring(s, scriptfile);

  t = new ScriptThread(s, 1);
  
  return t;
}

ScriptThread *Script::new_thread_from_string(const char *script)
{
  ScriptThread *t = NULL;
  lua_State *s;
  
  s = lua_newthread(L);

  if(luaL_loadbuffer(s, script, strlen(script), "nuvie") != 0)
    return NULL;

  t = new ScriptThread(s, 0);
  
  return t;
}

static bool get_tbl_field_uint16(lua_State *L, const char *index, uint16 *field)
{
  lua_pushstring(L, index);
  lua_gettable(L, -2);
  
  if(!lua_isnumber(L,-1))
    return false;
  *field = (uint16)lua_tonumber(L,-1);
  lua_pop(L, 1);
  return true;
}

static bool get_tbl_field_uint8(lua_State *L, const char *index, uint8 *field)
{
  lua_pushstring(L, index);
  lua_gettable(L, -2);

  if(!lua_isnumber(L,-1))
    return false;
  *field = (uint8)lua_tonumber(L,-1);
  lua_pop(L, 1);
  return true;
}

static bool nscript_get_location_from_args(lua_State *L, uint16 *x, uint16 *y, uint8 *z, int lua_stack_offset)
{
  if(lua_istable(L,lua_stack_offset))
  {
    if(!get_tbl_field_uint16(L, "x", x)) return false;
    if(!get_tbl_field_uint16(L, "y", y)) return false;
    if(!get_tbl_field_uint8(L, "z", z)) return false;
  }
  else
  {
    *x = (uint16)luaL_checkinteger(L, lua_stack_offset);
    *y = (uint16)luaL_checkinteger(L, lua_stack_offset + 1);
    *z = (uint8)luaL_checkinteger(L,  lua_stack_offset + 2);
  }
  
  return true;
}

static int nscript_obj_new(lua_State *L)
{
  return nscript_obj_new(L, NULL);
}

static int nscript_obj_new(lua_State *L, Obj *obj)
{
  Obj **p_obj;
  
  p_obj = (Obj **)lua_newuserdata(L, sizeof(Obj *));
  
  luaL_getmetatable(L, "nuvie.Obj");
  lua_setmetatable(L, -2);
  
  if(obj == NULL)
  {
    obj = new Obj();

    if(lua_gettop(L) > 1) // do we have arguments?
    {
      if(lua_isuserdata(L,1)) // do we have an obj
      {
        if(nscript_obj_init_from_obj(L, obj) == false)
          return 0;
      }
      else // init object from arguments
      {
        if(nscript_obj_init_from_args(L, lua_gettop(L) - 1, obj) == false)
          return 0;
      }
    }
  }

  *p_obj = obj;
  
  nscript_inc_obj_ref_count(obj);
    
  return 1;
}

sint32 nscript_inc_obj_ref_count(Obj *obj)
{
  ScriptObjRef *obj_ref;
  iAVLKey key = (iAVLKey)obj;
  
  obj_ref = (ScriptObjRef *)iAVLSearch(script_obj_list, key);
  if(obj_ref == NULL)
  {
    obj->set_in_script(true); // mark as being used by script engine.
    obj_ref =  new ScriptObjRef();
    obj_ref->key = (iAVLKey)obj;
    iAVLInsert(script_obj_list,obj_ref);
  }
  
  obj_ref->refcount++;

  return (sint32)obj_ref->refcount;
}

sint32 nscript_dec_obj_ref_count(Obj *obj)
{
  ScriptObjRef *obj_ref;
  iAVLKey key = (iAVLKey)obj;
  
  obj_ref = (ScriptObjRef *)iAVLSearch(script_obj_list, key);
  if(obj_ref == NULL)
    return -1;
  
  
  obj_ref->refcount--;
  
  if(obj_ref->refcount == 0)
  {
    iAVLDelete(script_obj_list, key);
    delete obj_ref;
    obj->set_in_script(false); //nolonger being referenced by the script engine.
    return 0;
  }
  
  return obj_ref->refcount;
}

inline bool nscript_obj_init_from_obj(lua_State *L, Obj *s_obj)
{
  Obj *ptr = (Obj *)luaL_checkudata(L, 1, "nuvie.Obj");
  
  if(ptr == NULL)
    return false;
  
  s_obj->obj_n = ptr->obj_n;
  s_obj->frame_n = ptr->frame_n;
  s_obj->quality = ptr->quality;
  s_obj->qty = ptr->qty;
  s_obj->x = ptr->x;
  s_obj->y = ptr->y;
  s_obj->z = ptr->z;
  
  return true;
}

inline bool nscript_obj_init_from_args(lua_State *L, int nargs, Obj *s_obj)
{
  uint8 i = nargs;

  if(i)
  {
    if(!lua_isnil(L, 1))
      s_obj->obj_n = (uint16)lua_tointeger(L,1);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 2))
      s_obj->frame_n = (uint8)lua_tointeger(L,2);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 3))
      s_obj->quality = (uint8)lua_tointeger(L,3);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 4))
      s_obj->qty = (uint16)lua_tointeger(L,4);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 5))
      s_obj->x = (uint16)lua_tointeger(L,5);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 6))
      s_obj->y = (uint16)lua_tointeger(L,6);
    i--;
  }
  
  if(i)
  {
    if(!lua_isnil(L, 7))
      s_obj->z = (uint8)lua_tointeger(L,7);
    i--;
  }

  return true;
}

static int nscript_obj_gc(lua_State *L)
{
  DEBUG(0,LEVEL_INFORMATIONAL,"\nObj garbage Collection!\n");
  
  Obj **p_obj = (Obj **)lua_touserdata(L,1);
  Obj *obj;
  
  if(p_obj == NULL)
    return false;
  
  obj = *p_obj;
  
  if(nscript_dec_obj_ref_count(obj) == 0) // no longer referenced by the script engine
  {
    // remove object if it is not referenced by the game engine.
    if(obj->get_engine_loc() == OBJ_LOC_NONE)
      delete_obj(obj);
  }

  return 0;
}

/*
static inline Obj *nscript_get_obj_ptr(ScriptObj *s_obj)
{
  if(s_obj)
  {
    if(s_obj->obj_ptr)
      return s_obj->obj_ptr;
    else
      return &s_obj->script_obj;
  }
  
  return NULL;
}
*/
static int nscript_obj_set(lua_State *L)
{
  Obj **s_obj;
  Obj *obj;
  //Obj *ptr;
  const char *key;
  
  s_obj = (Obj **)lua_touserdata(L,1);
  if(s_obj == NULL)
    return 0;
  
  obj = *s_obj;
  if(obj == NULL)
    return 0;
  
 // ptr = nscript_get_obj_ptr(s_obj);
  
  key = lua_tostring(L, 2);

  if(!strcmp(key, "x"))
  {
    obj->x = (uint16)lua_tointeger(L,3);
    return 0;
  }
  
  if(!strcmp(key, "y"))
  {
    obj->y = (uint16)lua_tointeger(L,3);
    return 0;
  }
  
  if(!strcmp(key, "z"))
  {
    obj->z = (uint8)lua_tointeger(L,3);
    return 0;
  }
  
  if(!strcmp(key, "obj_n"))
  {
    obj->obj_n = (uint16)lua_tointeger(L,3);
    return 0;
  }
  
  if(!strcmp(key, "frame_n"))
  {
    obj->frame_n = (uint8)lua_tointeger(L,3);
    return 0;
  }

  if(!strcmp(key, "quality"))
  {
    obj->quality = (uint8)lua_tointeger(L,3);
    return 0;
  }

  if(!strcmp(key, "qty"))
  {
    obj->qty = (uint8)lua_tointeger(L,3);
    return 0;
  }

  return 0;
}

static int nscript_obj_get(lua_State *L)
{
  Obj **s_obj;
  Obj *obj;
  const char *key;
  
  s_obj = (Obj **)lua_touserdata(L,1);
  if(s_obj == NULL)
    return 0;
  obj = *s_obj;
  if(obj == NULL)
    return 0;
  
  //ptr = nscript_get_obj_ptr(s_obj);
  
  key = lua_tostring(L, 2);
  
  if(!strcmp(key, "x"))
  {
    lua_pushinteger(L,obj->x); return 1;
  }
  
  if(!strcmp(key, "y"))
  {
    lua_pushinteger(L,obj->y); return 1;
  }
  
  if(!strcmp(key, "z"))
  {
    lua_pushinteger(L,obj->z); return 1;
  }
  
  if(!strcmp(key, "obj_n"))
  {
    lua_pushinteger(L,obj->obj_n); return 1;
  }
  
  if(!strcmp(key, "frame_n"))
  {
    lua_pushinteger(L,obj->frame_n); return 1;
  }
  
  if(!strcmp(key, "quality"))
  {
    lua_pushinteger(L,obj->quality); return 1;
  }
  
  if(!strcmp(key, "qty"))
  {
    lua_pushinteger(L,obj->qty); return 1;
  }

  if(!strcmp(key, "name"))
  {
    ObjManager *obj_manager = Game::get_game()->get_obj_manager();
    lua_pushstring(L, obj_manager->get_obj_name(obj->obj_n));
    return 1;
  }

  if(!strcmp(key, "container"))
  {
    if(nscript_container_new(L, obj))
      return 1;
  }
  
  return 0;
}

static int nscript_obj_movetomap(lua_State *L)
{
  ObjManager *obj_manager = Game::get_game()->get_obj_manager();
  
  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  
  obj = *s_obj;
  
  if(obj)
  {
    if(obj_manager->moveto_map(obj) == false)
    {
      //delete map_obj;
      return luaL_error(L,"moving obj to map!");
    }
    
    //s_obj->obj_ptr = map_obj;
  }
  
  return 0;
}

static int nscript_obj_movetoinv(lua_State *L)
{
  ObjManager *obj_manager = Game::get_game()->get_obj_manager();
  ActorManager *actor_manager = Game::get_game()->get_actor_manager();
  Actor *actor;

  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  
  obj = *s_obj;
  
  if(lua_gettop(L) < 2)
    return luaL_error(L, "You must supply an Actor # to Obj.moveToInv()");
  
  actor = actor_manager->get_actor(lua_tointeger(L,2));
  
  if(actor == NULL)
    return luaL_error(L, "Getting Actor (%d)",lua_tointeger(L, 2));
  
  if(obj)
  {
    if(obj_manager->moveto_inventory(obj, actor) == false)
    {
      //delete inv_obj;
      return luaL_error(L,"moving obj to actor inventory!");
    }
    
    //s_obj->obj_ptr = inv_obj;
  }
  
  
  return 0;
}

static int nscript_obj_movetocont(lua_State *L)
{
  ObjManager *obj_manager = Game::get_game()->get_obj_manager();
  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  Obj *container_obj;
  
  obj = *s_obj;
  
  if(obj == NULL)
    return 0;
  
  if(lua_gettop(L) < 2)
    return luaL_error(L, "You must supply an Object to move into in Obj.moveToCont()");

  s_obj = (Obj **)luaL_checkudata(L, 2, "nuvie.Obj");
  container_obj = *s_obj;
  
  if(container_obj)
  {
    if(obj_manager->moveto_container(obj, container_obj) == false)
    {
      return luaL_error(L,"moving obj into container!");
    }
  }
  
  //pos = lua_tointeger(L, 2);
    
  return 0;
}

static int nscript_container_remove_obj(lua_State *L)
{
  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  Obj *cont_obj;
  
  obj = *s_obj;
  
  if(obj == NULL)
    return luaL_error(L,"getting obj!");
  
  cont_obj = obj->get_container_obj();
  
  if(cont_obj == NULL)
    return luaL_error(L,"obj not in a container!");
  
  if(cont_obj->remove(obj) == false)
    return luaL_error(L,"removing obj from container!");
  
  return 0;
}

static int nscript_obj_use(lua_State *L)
{
  
  UseCode *usecode = Game::get_game()->get_usecode();
  Player *player = Game::get_game()->get_player();
  Actor *actor = player->get_actor();
  
  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  
  obj = *s_obj;
  
  if(obj)
  {
    usecode->use_obj(obj,actor);
  }
  
  
  return 0;
}

static int nscript_obj_removefromengine(lua_State *L)
{
  Obj *s_obj = (Obj *)luaL_checkudata(L, 1, "nuvie.Obj");
  if(s_obj == NULL)
    return luaL_error(L, "expected Obj\n");
      
  return 0;
}

static bool nscript_container_new(lua_State *L, Obj *parent_obj)
{
  Obj **p_obj;
  
  p_obj = (Obj **)lua_newuserdata(L, sizeof(Obj *));
  
  luaL_getmetatable(L, "nuvie.Container");
  lua_setmetatable(L, -2);
  
  *p_obj = parent_obj;
  
  nscript_inc_obj_ref_count(parent_obj);

  return true;
}

static int nscript_container_gc(lua_State *L)
{
  return nscript_obj_gc(L); //a container is just another Obj.
}

static int nscript_container_get(lua_State *L)
{
  Obj **s_obj;
  Obj *obj;
  U6Link *link;
  
  int key;
  
  s_obj = (Obj **)lua_touserdata(L,1);
  if(s_obj == NULL)
    return 0;
  obj = *s_obj;
  if(obj == NULL)
    return 0;
  
  //ptr = nscript_get_obj_ptr(s_obj);
  if(obj->container == NULL)
    return 0;
  
  key = lua_tointeger(L, 2);
  
  DEBUG(0,LEVEL_DEBUGGING,"Key = %d\n", key);
  
  link = obj->container->gotoPos(key-1);

  if(link && link->data)
    return nscript_obj_new(L,(Obj *)link->data); //return container child Obj to script.

  return 0;
}

static int nscript_container_length(lua_State *L)
{
  Obj **s_obj;
  Obj *obj;
  uint32 count = 0;
  
  s_obj = (Obj **)lua_touserdata(L,1);
  if(s_obj == NULL)
    return 0;
  obj = *s_obj;
  if(obj == NULL)
    return 0;

  if(obj->container != NULL)
    count = obj->container->count();
  
  lua_pushinteger(L,(int)count);
  return 1;
}

bool nscript_new_actor_var(lua_State *L, uint16 actor_num)
{
  uint16 *userdata;
  
  userdata = (uint16 *)lua_newuserdata(L, sizeof(uint16));
  
  luaL_getmetatable(L, "nuvie.Actor");
  lua_setmetatable(L, -2);
  
  *userdata = actor_num;
  
  return true;
}

static int nscript_actor_new(lua_State *L)
{
  Actor *actor;
  uint16 obj_n = 0;
  uint16 x = 0;
  uint16 y = 0;
  uint8 z = 0;
  uint8 alignment = ACTOR_ALIGNMENT_NEUTRAL;
  uint8 worktype = 0; //motionless

  int nargs = lua_gettop(L);
  
  if(nargs > 1) // do we have arguments?
  {
    uint8 i = nargs;
    
    if(i)
    {
      if(!lua_isnil(L, 1))
        obj_n = (uint16)lua_tointeger(L,1);
      i--;
    }
    
    if(i)
    {
      if(!lua_isnil(L, 2))
        x = (uint16)lua_tointeger(L,2);
      i--;
    }
    
    if(i)
    {
      if(!lua_isnil(L, 3))
        y = (uint16)lua_tointeger(L,3);
      i--;
    }

    if(i)
    {
      if(!lua_isnil(L, 4))
        z = (uint8)lua_tointeger(L,4);
      i--;
    }

    if(i)
    {
      if(!lua_isnil(L, 5))
        alignment = (uint8)lua_tointeger(L,5);
      i--;
    }

    if(i)
    {
      if(!lua_isnil(L, 6))
        worktype = (uint8)lua_tointeger(L,6);
      i--;
    }

    //init here.
    if(Game::get_game()->get_actor_manager()->create_temp_actor(obj_n, x, y, z, alignment, worktype, &actor) == false)
      return 0;

    //create the new lua Actor variable
    if(nscript_new_actor_var(L, actor->get_actor_num()) == false)
      return 0;
  }
  
  return 1;
}

static int nscript_get_actor_from_num(lua_State *L)
{
  uint16 actor_num;
  actor_num = (uint16)lua_tointeger(L,1);
  
  return nscript_new_actor_var(L,actor_num);
}

inline Actor *nscript_get_actor_from_args(lua_State *L, int lua_stack_offset)
{
  uint16 *actor_num;
  Actor *actor = NULL;

  actor_num = (uint16 *)luaL_checkudata(L, lua_stack_offset, "nuvie.Actor");

  if(actor_num != NULL)
    actor = Game::get_game()->get_actor_manager()->get_actor(*actor_num);

  return actor;
}

static int nscript_actor_set(lua_State *L)
{
  Actor *actor;
  const char *key;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;

  key = lua_tostring(L, 2);
  
  if(!strcmp(key, "hp"))
  {
    actor->set_hp((uint8)lua_tointeger(L,3));
    return 0;
  }
  
  if(!strcmp(key, "obj_n"))
  {
    //actor->obj_n = (uint16)lua_tointeger(L,3);
    return 0;
  }
  
  if(!strcmp(key, "frame_n"))
  {
    //actor->frame_n = (uint8)lua_tointeger(L,3);
    return 0;
  }

  return 0;
}

static int nscript_actor_get(lua_State *L)
{
  Actor *actor;
  const char *key;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;
  
  key = lua_tostring(L, 2);
  
  if(!strcmp(key, "x"))
  {
    lua_pushinteger(L,actor->get_x()); return 1;
  }
  
  if(!strcmp(key, "y"))
  {
    lua_pushinteger(L,actor->get_y()); return 1;
  }
  
  if(!strcmp(key, "z"))
  {
    lua_pushinteger(L,actor->get_z()); return 1;
  }
  
  if(!strcmp(key, "hp"))
  {
    lua_pushinteger(L,actor->get_hp()); return 1;
  }
  
  if(!strcmp(key, "obj_n"))
  {
    //lua_pushinteger(L,actor->obj_n); return 1;
  }
  
  if(!strcmp(key, "frame_n"))
  {
   // lua_pushinteger(L,actor->frame_n); return 1;
  }

  return 0;
}

static int nscript_actor_kill(lua_State *L)
{
  Actor *actor;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;
  
  actor->die();
  
  return 0;
}

static int nscript_actor_hit(lua_State *L)
{
  Actor *actor;
  uint8 damage;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;
  
  damage = (uint8)luaL_checkinteger(L,2);
  
  actor->hit(damage);
  
  return 0;
}

static int nscript_actor_move(lua_State *L)
{
  Actor *actor;
  uint16 x, y;
  uint8 z;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;
  
  if(nscript_get_location_from_args(L,&x, &y, &z, 2) == false)
    return 0;

  actor->move(x,y,z);
  
  return 0;
}

static int nscript_actor_resurrect(lua_State *L)
{
  Actor *actor;
  MapCoord loc;
  
  actor = nscript_get_actor_from_args(L);
  if(actor == NULL)
    return 0;

  if(nscript_get_location_from_args(L, &loc.x, &loc.y, &loc.z, 2) == false)
    return 0;

  actor->resurrect(loc);
  
  return 0;
}

static int nscript_print(lua_State *L)
{
  MsgScroll *scroll = Game::get_game()->get_scroll();
  const char *string = luaL_checkstring(L, 1);
  
  scroll->display_string(string);

  return 0;
}

static int nscript_load(lua_State *L)
{
  
  const char *file = luaL_checkstring(L, 1);
  string dir;
  string path;
  
  Script::get_script()->get_config()->value("config/datadir", dir, "");
  
  build_path(dir, "scripts", path);
  dir = path;
  build_path(dir, file, path);
  
  luaL_loadfile(L, path.c_str());
  
  return 1;
}


static int nscript_player_get_location(lua_State *L)
{
  Player *player = Game::get_game()->get_player();
  
  uint16 x,y;
  uint8 z;
  
  player->get_actor()->get_location(&x, &y, &z);
  
  lua_newtable(L);
  lua_pushstring(L,"x");
  lua_pushinteger(L,x);
  lua_settable(L, -3);
  
  lua_pushstring(L,"y");
  lua_pushinteger(L,y);
  lua_settable(L, -3);
  
  lua_pushstring(L,"z");
  lua_pushinteger(L,z);
  lua_settable(L, -3);
  
  return 1;
}

static int nscript_map_get_obj(lua_State *L)
{
  ObjManager *obj_manager = Game::get_game()->get_obj_manager();
  Obj **p_obj;
  Obj *obj;

  uint16 x, y;
  uint8 z;
  
  if(nscript_get_location_from_args(L, &x, &y, &z) == false)
    return 0;
  

  obj = obj_manager->get_obj(x,y,z);
  
  if(obj)
  {
    p_obj = (Obj **)lua_newuserdata(L, sizeof(Obj *));
  
    luaL_getmetatable(L, "nuvie.Obj");
    lua_setmetatable(L, -2);
    
    *p_obj = obj;
  
    nscript_inc_obj_ref_count(obj);
    return 1;
  }
  
  return 0;
}

static int nscript_map_remove_obj(lua_State *L)
{
  ObjManager *obj_manager = Game::get_game()->get_obj_manager();
  
  Obj **s_obj = (Obj **)luaL_checkudata(L, 1, "nuvie.Obj");
  Obj *obj;
  
  obj = *s_obj;
  
  if(obj_manager->remove_obj_from_map(obj))
    lua_pushboolean(L,true);
  else
    lua_pushboolean(L,false);
    
  return 1;
}

static int nscript_map_get_actor(lua_State *L)
{
  ActorManager *actor_manager = Game::get_game()->get_actor_manager();
  Actor *actor;
  
  uint16 x, y;
  uint8 z;
  
  if(nscript_get_location_from_args(L, &x, &y, &z) == false)
    return 0;

  actor = actor_manager->get_actor(x,y,z);
  
  if(actor == NULL)
    return 0;
  
  if(nscript_new_actor_var(L,actor->get_actor_num()) == false)
    return 0;
    
  return 1;
}

static int nscript_actor_poison(lua_State *L)
{
  ActorManager *actor_manager = Game::get_game()->get_actor_manager();
  uint16 actor_num;
  Actor *actor;
  
  actor_num = (uint16)luaL_checkinteger(L, 1);
  
  actor = actor_manager->get_actor(actor_num);
  
  actor->set_poisoned(true);
  
  return 0;
}

static int nscript_actor_add_hp(lua_State *L)
{
  ActorManager *actor_manager = Game::get_game()->get_actor_manager();
  uint16 actor_num;
  uint8 hp;
  Actor *actor;
  
  actor_num = (uint16)luaL_checkinteger(L, 1);
  hp = (uint8)luaL_checkinteger(L, 2);
  
  actor = actor_manager->get_actor(actor_num);
  
  actor->set_hp(MAX((actor->get_hp() + hp), actor->get_maxhp()));
  
  return 0;
}

static int nscript_actor_add_mp(lua_State *L)
{
  ActorManager *actor_manager = Game::get_game()->get_actor_manager();
  uint16 actor_num;
  uint8 mp;
  Actor *actor;
  
  actor_num = (uint16)luaL_checkinteger(L, 1);
  mp = (uint8)luaL_checkinteger(L, 2);
  
  actor = actor_manager->get_actor(actor_num);
  
  actor->set_magic(MAX((actor->get_magic() + mp), actor->get_maxmagic()));
  
  return 0;
}

static int nscript_eclipse_start(lua_State *L)
{
  Weather *weather = Game::get_game()->get_weather();
  uint8 length = (uint8)luaL_checkinteger(L, 1);
  
	weather->start_eclipse(length);
  
  lua_pushboolean(L,true);
  return 1;
}

static int nscript_quake_start(lua_State *L)
{
  Player *player = Game::get_game()->get_player();

  uint8 magnitude = (uint8)luaL_checkinteger(L, 1);
  uint32 duration = (uint32)luaL_checkinteger(L, 2);
    
  new QuakeEffect(magnitude,duration,player->get_actor());
  
  lua_pushboolean(L,true);
  return 1;
}

static int nscript_explosion_start(lua_State *L)
{
  uint16 x = (uint16)luaL_checkinteger(L, 1);
  uint16 y = (uint16)luaL_checkinteger(L, 2);  
  uint32 size = (uint32)luaL_checkinteger(L, 3);
  uint16 dmg = (uint16)luaL_checkinteger(L, 4);
  
  new ExplosiveEffect(x, y, size, dmg);
  
  lua_pushboolean(L,true);
  return 1;
}