/*
 *  ActorManager.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Thu Mar 20 2003.
 *  Copyright (c) 2003. All rights reserved.
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
#include "Configuration.h"
 
#include "Actor.h"
#include "U6Actor.h"
#include "TileManager.h"
#include "U6misc.h"
#include "U6LList.h"
#include "ActorManager.h"
#include "misc.h"
#include "NuvieIOFile.h"
#include "GameClock.h"

void config_get_path(Configuration *config, std::string filename, std::string &path);

ActorManager::ActorManager(Configuration *cfg, Map *m, TileManager *tm, ObjManager *om, GameClock *c)
{
 config = cfg;
 map = m;
 tile_manager = tm;
 obj_manager = om;
 clock = c;
 
 game_hour = 0;
 
 player_actor = 1;

 update = true;
}

ActorManager::~ActorManager()
{
 uint16 i;
 
 for(i = 0;i < 256;i++) //we assume actors[] have been created by a call to loadActors()
   delete actors[i];

}
 
bool ActorManager::loadActors()
{
 uint16 i;
 uint8 b1, b2, b3;
 int game_type;
 std::string filename;
 NuvieIOFileRead objlist;
 //NuvieIOFileRead schedule;
 
 config->value("config/GameType",game_type);
 config_get_path(config,"/savegame/objlist",filename);
 if(objlist.open(filename) == false)
   return false;


 objlist.seek(0x100); // Start of Actor position info
 
 for(i=0; i < 256; i++)
   {
    switch(game_type)
     {
      case NUVIE_GAME_U6 : actors[i] = new U6Actor(map,obj_manager,clock); break;
      case NUVIE_GAME_MD : actors[i] = new Actor(map,obj_manager,clock); break;
      case NUVIE_GAME_SE : actors[i] = new Actor(map,obj_manager,clock); break;
     }
     
    b1 = objlist.read1();
    b2 = objlist.read1();
    b3 = objlist.read1();
    
    actors[i]->x = b1;
    actors[i]->x += (b2 & 0x3) << 8; 
   
    actors[i]->y = (b2 & 0xfc) >> 2;
    actors[i]->y += (b3 & 0xf) << 6;
   
    actors[i]->z = (b3 & 0xf0) >> 4;
    actors[i]->id_n = (uint8)i;
/* Force avatar to gargoyle world
    if(i == 1)
     {
      actors[i]->x = 607;
      actors[i]->y = 285;
      actors[i]->z = 0;
     }
*/     
   }

// objlist.seek(0x15f1);
 
 for(i=0;i < 256; i++)
   {
    b1 = objlist.read1();
    b2 = objlist.read1();
    actors[i]->obj_n = b1;
    actors[i]->obj_n += (b2 & 0x3) << 8;
    
    actors[i]->frame_n = (b2 & 0xfc) >> 2;
    actors[i]->direction = actors[i]->frame_n / 4;
   }

 //old obj_n & frame_n values
 
 objlist.seek(0x15f1);
 
 for(i=0;i < 256; i++)
   {
    b1 = objlist.read1();
    b2 = objlist.read1();
    actors[i]->base_obj_n = b1;
    actors[i]->base_obj_n += (b2 & 0x3) << 8;
    
    actors[i]->old_frame_n = (b2 & 0xfc) >> 2;

    if(actors[i]->obj_n == 0)
     {
      actors[i]->obj_n = actors[i]->base_obj_n;
      actors[i]->frame_n = actors[i]->old_frame_n;
     }

    if(actors[i]->base_obj_n == 0)
     {
      actors[i]->base_obj_n = actors[i]->obj_n;
      actors[i]->old_frame_n = actors[i]->frame_n;
     }
   }
 // Strength
 
 objlist.seek(0x900);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->strength = objlist.read1();
   }
 
 // Dexterity
 
 objlist.seek(0xa00);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->dex = objlist.read1();
   }
 
 // Intelligence
 
 objlist.seek(0xb00);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->intelligence = objlist.read1();
   }

  // Experience
 
 objlist.seek(0xc00);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->exp = objlist.read2();
   }
 
 // Health
 
 objlist.seek(0xe00);

 for(i=0;i < 256; i++)
   {
    actors[i]->hp = objlist.read1();
   }
   
 // Experience Level
 
 objlist.seek(0xff1);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->level = objlist.read1();
   }
 

 // Combat mode
 
 objlist.seek(0x12f1);

 for(i=0;i < 256; i++)
   {
    actors[i]->combat_mode = objlist.read1();
   }
    
 // Magic Points
 
 objlist.seek(0x13f1);
 
 for(i=0;i < 256; i++)
   {
    actors[i]->magic = objlist.read1();
   }

 objlist.seek(0x17f1); // Start of Actor flags

 for(i=0;i < 256; i++)
   {
    actors[i]->flags = objlist.read1();
   }
 
 loadActorSchedules();
 
 for(i=0;i < 256; i++)
   {
    actors[i]->inventory_parse_readied_objects();

    actors[i]->init(); //let the actor object do some init
   }

 // Current Worktype
 
 objlist.seek(0x11f1);

 for(i=0;i < 256; i++)
   {
    actors[i]->set_worktype(objlist.read1());
   }

 return true;
}
 
Actor *ActorManager::get_actor(uint8 actor_num)
{
 return actors[actor_num];
}

Actor *ActorManager::get_actor(uint16 x, uint16 y, uint8 z)
{
 uint16 i;

 for(i=0;i<256;i++)
  {
   if(actors[i]->x == x && actors[i]->y == y && actors[i]->z == z)
     return actors[i];
  }

 return NULL;
}

Actor *ActorManager::get_player()
{
 return actors[player_actor]; //FIX here for dead party leader etc.
}

void ActorManager::set_player(Actor *a)
{
 player_actor = a->id_n;
}

/* Returns an actor's "look-string," a general description of their occupation
 * or appearance.
 */
const char *ActorManager::look_actor(Actor *a)
{
    return(tile_manager->lookAtTile(
                           obj_manager->get_obj_tile_num(a->get_tile_num()),
                                    0, false));
}


void ActorManager::updateActors()
{
 uint8 cur_hour;
 uint16 i;

 if(!update)
  return;

 cur_hour = clock->get_hour();

 if(cur_hour != game_hour)
   {
    game_hour = cur_hour;

    for(i=0;i<256;i++)
      if(!actors[i]->in_party) // don't do scheduled activities while partying
        actors[i]->updateSchedule(cur_hour);
   }

 for(i=0;i<256;i++)
  actors[i]->update();

 return;  
}

void ActorManager::twitchActors()
{
 uint16 i;
 
 for(i=0;i<256;i++)
  actors[i]->twitch();
}

bool ActorManager::loadActorSchedules()
{
 std::string filename;
 NuvieIOFileRead schedule;
 uint16 i;
 uint16 index[256];
 uint16 s_num;
 uint32 bytes_read;
 unsigned char *sched_data;
 unsigned char *s_ptr;
 
 config_get_path(config,"schedule",filename);
 if(schedule.open(filename) == false)
   return false;
 
 for(i=0;i<256;i++)
   {
    index[i] = schedule.read2();
   }
 
 sched_data = schedule.readBuf(schedule.get_size() - 0x202, &bytes_read);
 
 s_ptr = sched_data + 2;

 for(i=0;i<256;i++)
  {
   if(i == 255) //Hmm a bit of a hack. might want to check if there are and scheduled events for Actor 255
     s_num = 0;
   else  
     s_num = index[i+1] - index[i];

   actors[i]->loadSchedule(s_ptr,s_num);
   s_ptr += s_num * 5;
  }
 
 free(sched_data);
 
 return true;
}


/* Print Actor data to stdout.
 */
void ActorManager::print_actor(Actor *actor)
{
    printf("\n");
    printf("%s at %x, %x, %x\n", look_actor(actor), actor->x, actor->y, actor->z);
    printf("id_n: %d\n", actor->id_n);

    printf("obj_n: %03d    frame_n: %d\n", actor->obj_n, actor->frame_n);
    printf("base_obj_n: %03d    old_frame_n: %d\n", actor->base_obj_n, actor->old_frame_n);

    uint8 direction = actor->direction;
    printf("direction: %d (%s)\n", direction, (direction==ACTOR_DIR_U)?"north":
                                              (direction==ACTOR_DIR_R)?"east":
                                              (direction==ACTOR_DIR_D)?"south":
                                              (direction==ACTOR_DIR_L)?"west":"???");
    printf("walk_frame: %d\n", actor->walk_frame);

    printf("can_twitch: %s\n", actor->can_twitch ? "true" : "false");
//    printf("alive: %s\n", actor->alive ? "true" : "false");
    printf("in_party: %s\n", actor->in_party ? "true" : "false");
    printf("visible_flag: %s\n", actor->visible_flag ? "true" : "false");
//    printf("met_player: %s\n", actor->met_player ? "true" : "false");

    printf("moves: %d\n", actor->moves);

    const char *wt_string = get_worktype_string(actor->worktype);
    if(!wt_string) wt_string = "???";
    printf("worktype: 0x%02x/%03d (%s)\n", actor->worktype, actor->worktype, wt_string);

    printf("NPC stats:\n");
    printf(" level: %d    exp: %d    hp: %d / %d\n", actor->level, actor->exp,
           actor->hp, actor->get_maxhp());
    printf(" strength: %d    dex: %d    int: %d\n", actor->strength, actor->dex,
           actor->intelligence);
    printf(" magic: %d\n", actor->magic);

    uint8 combat_mode = actor->combat_mode;
    printf("combat_mode: %d (%s)\n", combat_mode,
           (combat_mode == 0x02) ? "command"
           : (combat_mode == 0x03) ? "front"
           : (combat_mode == 0x04) ? "rear"
           : (combat_mode == 0x05) ? "flank"
           : (combat_mode == 0x06) ? "berserk"
           : (combat_mode == 0x07) ? "retreat"
           : (combat_mode == 0x08) ? "assault" : "???");

    printf("NPC flags: ");
    print_b(actor->flags);
    printf("\n");
//    printf("Status flags: ");
//    print_b();
//    printf("\n");

    uint32 inv = actor->inventory_count_objects(true);
    if(inv)
    {
        printf("Inventory (+readied): %d objects\n", inv);
        U6LList *inv_list = actor->get_inventory_list();
        for(U6Link *link = inv_list->start(); link != NULL; link=link->next)
        {
            Obj *obj = (Obj *)link->data;
            printf(" %24s (%03d:%d) qual=%d qty=%d    (weighs %f)\n",
                   obj_manager->look_obj(obj), obj->obj_n, obj->frame_n, obj->quality,
                   obj->qty, obj_manager->get_obj_weight(obj, false));
        }
        printf("(weight %f / %f)\n", actor->get_inventory_weight(),
               actor->inventory_get_max_weight());
    }
    if(actor->sched && *actor->sched)
    {
        printf("Schedule:\n");
        Schedule **s = actor->sched;
        uint32 sp = 0;
        do
        {
            wt_string = get_worktype_string(s[sp]->worktype);
            if(!wt_string) wt_string = "???";
            if(sp == actor->sched_pos && s[sp]->worktype == actor->worktype)
                printf("*%d: location=0x%03x,0x%03x,0x%x  time=%02d:00  day=%d  worktype=0x%02x(%s)*\n", sp, s[sp]->x, s[sp]->y, s[sp]->z, s[sp]->hour, s[sp]->day_of_week, s[sp]->worktype, wt_string);
            else
                printf(" %d: location=0x%03x,0x%03x,0x%x  time=%02d:00  day=%d  worktype=0x%02x(%s)\n", sp, s[sp]->x, s[sp]->y, s[sp]->z, s[sp]->hour, s[sp]->day_of_week, s[sp]->worktype, wt_string);
        } while(s[++sp]);
    }

    if(!actor->surrounding_objects.empty())
        printf("Actor has multiple tiles\n");
    if(actor->pathfinder)
        printf("Actor is walking\n");
    printf("\n");
}


/* Returns name of NPC worktype/activity (game specific) or NULL.
 */
const char *ActorManager::get_worktype_string(uint32 wt)
{
    const char *wt_string = NULL;
    if(wt == WORKTYPE_U6_IN_PARTY) wt_string = "in_party";
    else if(wt == WORKTYPE_U6_ANIMAL_WANDER) wt_string = "a_wander";
    else if(wt == WORKTYPE_U6_WALK_TO_LOCATION) wt_string = "walkto";
    else if(wt == WORKTYPE_U6_FACE_NORTH) wt_string = "face_n";
    else if(wt == WORKTYPE_U6_FACE_SOUTH) wt_string = "face_s";
    else if(wt == WORKTYPE_U6_FACE_EAST) wt_string = "face_e";
    else if(wt == WORKTYPE_U6_FACE_WEST) wt_string = "face_w";
    else if(wt == WORKTYPE_U6_WALK_NORTH_SOUTH) wt_string = "v_walk";
    else if(wt == WORKTYPE_U6_WALK_EAST_WEST) wt_string = "h_walk";
    else if(wt == WORKTYPE_U6_WANDER_AROUND) wt_string = "wander";
    else if(wt == WORKTYPE_U6_WORK) wt_string = "work_move";
    else if(wt == WORKTYPE_U6_SLEEP) wt_string = "sleep";
    else if(wt == WORKTYPE_U6_PLAY_LUTE) wt_string = "play_lute";
    else if(wt == WORKTYPE_U6_BEG) wt_string = "beg";
    else if(wt == 0x02) wt_string = "player";
    else if(wt >= 0x03 && wt <= 0x08) wt_string = "combat";
    else if(wt == 0x8e) wt_string = "waiter?";
    else if(wt == 0x92) wt_string = "work_still";
    else if(wt == 0x93) wt_string = "eat";
    else if(wt == 0x94) wt_string = "farmer";
    else if(wt == 0x98) wt_string = "bell";
    else if(wt == 0x99) wt_string = "spar";
    else if(wt == 0x9a) wt_string = "mousing";
    return(wt_string);
}
