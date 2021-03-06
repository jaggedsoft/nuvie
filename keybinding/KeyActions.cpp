/*
 *  Copyright (C) 2012 The Nuvie Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "KeyActions.h"
#include "Keys.h"
#include "Event.h"
#include "Game.h"
#include "Party.h"
#include "Player.h"
#include "ViewManager.h"
#include "MsgScroll.h"
#include "SaveManager.h"
#include "InventoryView.h"
#include "CommandBar.h"
#include "ActorView.h"
#include "MapWindow.h"
#include "Effect.h"
#include "EggManager.h"
#include "Screen.h"
#include "GUI.h"
#include "SoundManager.h"
#include "Background.h"
#include "Configuration.h"
#include "U6misc.h"

#define game Game::get_game()
#define event Game::get_game()->get_event()
#define party Game::get_game()->get_party()
#define player Game::get_game()->get_player()
#define view_manager Game::get_game()->get_view_manager()
#define inventory_view Game::get_game()->get_view_manager()->get_inventory_view()
#define actor_view Game::get_game()->get_view_manager()->get_actor_view()

void ActionWalkWest(int const *params)
{
	event->move(-1,0);
}

void ActionWalkEast(int const *params)
{
	event->move(1,0);
}

void ActionWalkNorth(int const *params)
{
	event->move(0,-1);
}

void ActionWalkSouth(int const *params)
{
	event->move(0,1);
}

void ActionWalkNorthEast(int const *params)
{
	event->move(1,-1);
}

void ActionWalkSouthEast(int const *params)
{
	event->move(1,1);
}

void ActionWalkNorthWest(int const *params)
{
	event->move(-1,-1);
}

void ActionWalkSouthWest(int const *params)
{
	event->move(-1,1);
}

void ActionCast(int const *params)
{
	if(game->get_game_type() != NUVIE_GAME_U6)
	{
		game->get_keybinder()->handle_wrong_key_pressed();
		return;
	}
	else if(player->is_in_vehicle())
		event->display_not_aboard_vehicle();
	else
		event->newAction(CAST_MODE);
}

void ActionLook(int const *params)
{
	event->newAction(LOOK_MODE);
}

void ActionTalk(int const *params)
{
	event->newAction(TALK_MODE);
}

void ActionUse(int const *params)
{
	event->newAction(USE_MODE);
}

void ActionGet(int const *params)
{
	event->newAction(GET_MODE);
}

void ActionMove(int const *params)
{
	event->newAction(PUSH_MODE);
}

void ActionDrop(int const *params)
{
	event->set_drop_from_key(true);
	event->newAction(DROP_MODE);
}

void ActionToggleCombat(int const *params)
{
	event->newAction(COMBAT_MODE);
}

void ActionAttack(int const *params)
{
	event->newAction(ATTACK_MODE);
}

void ActionRest(int const *params)
{
	event->newAction(REST_MODE);
}

void ActionMultiUse(int const *params)
{
	if(event->get_mode() == ATTACK_MODE)
		event->doAction();
	else
		event->newAction(MULTIUSE_MODE);
}

static const sint8 SE_command_tbl[] = {6, -1, 4, 5, 1, 2, 0, 3, 7, 8}; // convert U6 indexes
static const sint8 MD_command_tbl[] = {0, -1, 1, 2, 3, 4, 5, 6, -1, 7};

void ActionSelectCommandBar(int const *params)
{
	CommandBar *cb = game->get_command_bar();
	if(params[0] < 0 || params[0] > 9) // deactivate
		cb->select_action(-1);
	else if(game->get_game_type() == NUVIE_GAME_U6)
		cb->select_action(params[0]);
	else if(game->get_game_type() == NUVIE_GAME_SE)
		cb->select_action(SE_command_tbl[params[0]]);
	else // MD
		cb->select_action(MD_command_tbl[params[0]]);
}

void ActionSelectNewCommandBar(int const *params)
{
	CommandBar *cb = game->get_new_command_bar();
	if(!cb)
		return;

	cb->grab_focus();
	cb->Show();
#ifdef HAVE_JOYSTICK_SUPPORT
	game->get_keybinder()->set_enable_joy_repeat(false);
#endif
}

void ActionDollGump(int const *params)
{
	if(params[0] > 0)
	{
		Actor *party_member = party->get_actor(params[0] -1);
		if(party_member)
			view_manager->open_doll_view(party_member);
	}
	else
		view_manager->open_doll_view(NULL);
}

void ActionShowStats(int const *params)
{
	if(event->using_control_cheat())
		return;
	Actor *party_member = party->get_actor(params[0] -1);
	if(party_member == NULL)
		return;
	if(!game->is_new_style())
	{
		actor_view->set_party_member(params[0] -1);
		view_manager->set_actor_mode();
	}
	else
		view_manager->open_portrait_gump(party_member);
}

void ActionInventory(int const *params)
{
	if(event->using_control_cheat() || params[0] == 0)
		return;
	if(party->get_party_size() >= params[0])
	{
		if(!game->is_new_style())
		{
			view_manager->set_inventory_mode();
			inventory_view->set_party_member(params[0] -1);
		}
		else
		{
			view_manager->open_container_view(party->get_actor(params[0] -1));
		}
	}
}

void ActionPartyView(int const *params)
{
	if(!event->using_control_cheat())
		view_manager->set_party_mode();
}

void ActionNextPartyMember(int const *params)
{
	if(event->using_control_cheat())
		return;
	if(!game->is_new_style())
	{
		if(view_manager->get_current_view() == actor_view)
		{
			uint8 party_num = actor_view->get_party_member_num();
			if(party->get_party_size() >= party_num+2)
				actor_view->set_party_member(party_num+1);
		}
		else if(!inventory_view->is_picking_pocket())
		{
			uint8 party_num = inventory_view->get_party_member_num();
			if(party->get_party_size() >= party_num+2
			   && inventory_view->set_party_member(party_num+1))
				view_manager->set_inventory_mode();
		}
	}
}

void ActionPreviousPartyMember(int const *params)
{
	if(event->using_control_cheat())
		return;
	if(!game->is_new_style())
	{
		if(view_manager->get_current_view() == actor_view)
		{
			uint8 party_num = actor_view->get_party_member_num();
			if(party_num >= 1)
				actor_view->set_party_member(party_num-1);
		}
		else if(!inventory_view->is_picking_pocket())
		{
			uint8 party_num = inventory_view->get_party_member_num();
			if(party_num >= 1 && inventory_view->set_party_member(party_num-1))
				view_manager->set_inventory_mode();
		}
	}
}

void ActionHome(int const *params)
{
	if(event->using_control_cheat())
		return;
	if(!game->is_new_style())
	{
		if(view_manager->get_current_view() == actor_view)
			actor_view->set_party_member(0);
		else if(!inventory_view->is_picking_pocket() && inventory_view->set_party_member(0))
			view_manager->set_inventory_mode();
	}
}

void ActionEnd(int const *params)
{
	if(event->using_control_cheat())
		return;
	if(!game->is_new_style())
	{
		uint8 mem_num = party->get_party_size() - 1;
		if(view_manager->get_current_view() == actor_view)
			actor_view->set_party_member(mem_num);
		else if(!inventory_view->is_picking_pocket())
		{
			if(view_manager->get_current_view() != inventory_view)
				view_manager->set_inventory_mode();
			if(inventory_view->set_party_member(mem_num))
				view_manager->set_inventory_mode();
		}
	}
}

void ActionToggleView(int const *params)
{
	if(!game->is_new_style())
	{
		if(view_manager->get_current_view() == actor_view)
			view_manager->set_inventory_mode();
		else if(view_manager->get_current_view() == inventory_view && !inventory_view->is_picking_pocket())
			view_manager->set_actor_mode();
	}
}

void ActionSoloMode(int const *params)
{
	if(params[0] == 0)
	{
		if(player->in_party_mode())
			event->solo_mode(0);
		else
		{
			uint8 party_size = party->get_party_size() - 1;
			sint8 new_mem_num = party->get_member_num(player->get_actor()) + 1;
			if(new_mem_num > party_size)
			{
				if(!event->party_mode())
					event->solo_mode(0); // failed so start over again
			}
			else
				event->solo_mode((uint32)new_mem_num);
		}
		return;
	}
	if(event->get_mode() == INPUT_MODE)
		event->select_party_member(params[0] -1);
	else if(player->is_in_vehicle())
		event->display_not_aboard_vehicle();
	else
		event->solo_mode(params[0] - 1);
}

void ActionPartyMode(int const *params)
{
	if(event->get_mode() == MOVE_MODE)
		event->party_mode();
	else
		event->cancelAction();
}

void ActionSaveDialog(int const *params)
{
	event->saveDialog();
}

void ActionLoadLatestSave(int const *params)
{
	event->close_gumps();
	SaveManager *save_manager = game->get_save_manager();
	game->get_scroll()->display_string("Load game!\n");
	save_manager->load_latest_save();
}

void ActionQuickSave(int const *params)
{
	game->get_save_manager()->quick_save(params[0], QUICK_SAVE);
}

void ActionQuickLoad(int const *params)
{
	game->get_save_manager()->quick_save(params[0], QUICK_LOAD);
}

void ActionQuitDialog(int const *params)
{
	if(!event) // intro or used view ending command line
		{} // FIXME need way to quit
	else
		event->quitDialog();
}

void ActionQuitNODialog(int const *params)
{
	game->quit();
}

void ActionGameMenuDialog(int const *params)
{
	event->gameMenuDialog();
}

void ActionToggleFullscreen(int const *params)
{
	if(!game->get_screen()->toggle_fullscreen())
		new TextEffect("Couldn't toggle fullscreen");
	else
		game->get_gui()->force_full_redraw();
}

void ActionToggleCursor(int const *params)
{
	if(!game->is_new_style())
	{
		if(event->get_input()->select_from_inventory == false)
			event->moveCursorToInventory();
		else // cursor is on inventory
			event->moveCursorToMapWindow(true);
	}
	else
	{
		Actor *actor;
		if(player->is_in_vehicle())
			actor = party->get_actor(0);
		else
			actor = player->get_actor();
		game->get_view_manager()->open_container_view(actor);
	}
}

void ActionToggleCombatStrategy(int const *params)
{
	if(!game->is_new_style() && view_manager->get_current_view() == inventory_view)
		inventory_view->simulate_CB_callback();
}

void ActionToggleFps(int const *params)
{
	if(event)
		event->toggleFpsDisplay();
}

void ActionToggleAudio(int const *params)
{
	bool audio = !game->get_sound_manager()->is_audio_enabled();
	game->get_sound_manager()->set_audio_enabled(audio);
	string message = audio ? "Audio enabled" : "Audio disabled";
	new TextEffect(message);
}

void ActionToggleMusic(int const *params)
{
	bool music = !game->get_sound_manager()->is_music_enabled();
	game->get_sound_manager()->set_music_enabled(music);
	string message = music ? "Music enabled" : "Music disabled";
	new TextEffect(message);
}

void ActionToggleSFX(int const *params)
{
	bool sfx = !game->get_sound_manager()->is_sfx_enabled();
	game->get_sound_manager()->set_sfx_enabled(sfx);
	string message = sfx ? "Sfx enabled" : "Sfx disabled";
	new TextEffect(message);
}

void ActionToggleOriginalStyleCommandBar(int const *params)
{
	if(Game::get_game()->is_orig_style())
		return;
	CommandBar *cb = game->get_command_bar();
	Configuration *config = game->get_config();
	bool hide = cb->Status() == WIDGET_VISIBLE;
	if(hide) {
		cb->Hide();
		game->get_screen()->clear(cb->X(), cb->Y(), cb->W(), cb->H(), NULL); // can be over null background so need to not leave corruption
		game->get_screen()->update(cb->X(), cb->Y(), cb->W(), cb->H());
	} else {
		cb->Show();
	}
	config->set(config_get_game_key(config) + "/show_orig_style_cb", !hide);
	config->write();
}

void ActionDoAction(int const *params)
{
	event->doAction();
}

void ActionCancelAction(int const *params)
{
	event->cancelAction();
}

void ActionMsgScrollUP(int const *params)
{
	if(!game->is_new_style())
		game->get_scroll()->page_up();
	else
		game->get_scroll()->move_scroll_up();
}

void ActionMsgScrollDown(int const *params)
{
	if(!game->is_new_style())
		game->get_scroll()->page_down();
	else
		game->get_scroll()->move_scroll_down();
}

void ActionShowKeys(int const *params)
{
	game->get_keybinder()->ShowKeys();
}

void ActionDecreaseDebug(int const *params)
{
	DEBUG(0,LEVEL_EMERGENCY,"!!decrease!!\n");
}
void ActionIncreaseDebug(int const *params)
{
	DEBUG(0,LEVEL_EMERGENCY,"!!increase!!\n");
}

void ActionCloseGumps(int const *params)
{
	event->close_gumps();
}

void ActionUseItem(int const *params)
{
	if(event->get_mode() != MOVE_MODE && event->get_mode() !=EQUIP_MODE)
		return;
	uint16 obj_n = params[0] > 0 ? params[0] : 0;
#if 0 // need to increase c_maxparams to 5 to use this many parameters
	uint8 qual = params[1] > 0 ? params[1] : 0;
	bool match_qual = params[2] == 1 ? true: false;
	uint8 frame_n = params[3] > 0 ? params[3] : 0;
	bool match_frame_n = params[4] == 1 ? true: false;
#else
	uint8 qual = 0;
	bool match_qual = false;
	uint8 frame_n = 0;
	bool match_frame_n = false;
#endif

	// try player first
	Obj *obj = player->get_actor()->inventory_get_object(obj_n, qual, match_qual, frame_n, match_frame_n);
	if(!obj && !event->using_control_cheat())
		obj =  party->get_obj(obj_n, qual, match_qual, frame_n, match_frame_n);
	if(obj)
	{
		game->get_scroll()->display_string("Use-", MSGSCROLL_NO_MAP_DISPLAY);
		event->set_mode(USE_MODE);
		event->use(obj);
	}
	// printf("ActionUseItem obj_n = %d, qual = %d, match_qual = %s, frame_n = %d, match_frame_n = %s\n", obj_n, qual, match_qual ? "true": "false", frame_n, match_frame_n ? "true": "false");
}

void ActionShowEggs(int const *params)
{
	bool show_eggs = !game->get_obj_manager()->is_showing_eggs();
	game->get_obj_manager()->set_show_eggs(show_eggs);
	game->get_egg_manager()->set_egg_visibility(show_eggs);
	string message = show_eggs ? "Showing eggs" : "Eggs invisible";
	new TextEffect(message);
}

void ActionToggleHackmove(int const *params)
{
	bool hackmove = !game->using_hackmove();
	game->set_hackmove(hackmove);
	string message = hackmove ? "Hack move enabled" : "Hack move disabled";
	new TextEffect(message);
}

void ActionToggleEggSpawn(int const *params)
{
	EggManager *egg_manager= game->get_obj_manager()->get_egg_manager();
	bool spawning = !egg_manager->is_spawning_actors();
	egg_manager->set_spawning_actors(spawning);
	string message = spawning ? "Will spawn actors" : "Won't spawn actors";
	new TextEffect(message);
}

void ActionToggleUnlimitedCasting(int const *params)
{
	bool unlimited = !game->has_unlimited_casting();
	game->set_unlimited_casting(unlimited);
	string message = unlimited ? "Unlimited casting" : "Normal casting";
	new TextEffect(message);
}

void ActionToggleNoDarkness(int const *params)
{
	bool no_darkness = game->get_screen()->toggle_darkness_cheat();
	string message = no_darkness ? "No more darkness" : "Normal lighting";
	new TextEffect(message);
}

void ActionTogglePickpocket(int const *params)
{
	bool pickpocket = (event->using_pickpocket_cheat = !event->using_pickpocket_cheat);
	string message = pickpocket ? "Pickpocket mode" : "Pickpocket disabled";
	new TextEffect(message);
}

void ActionToggleGodMode(int const *params)
{
	bool god_mode = game->toggle_god_mode();
	string message = god_mode ? "God mode enabled" : "God mode disabled";
	new TextEffect(message);
}

void ActionToggleEthereal(int const *params)
{
	bool ethereal = !game->is_ethereal();
	game->set_ethereal(ethereal);
	party->set_ethereal(ethereal);
	string message = ethereal ? "Ethereal movement" : "Normal movement";
	new TextEffect(message);
}

void ActionToggleX_Ray(int const *params)
{
	bool x_ray = game->get_map_window()->get_x_ray_view() <= X_RAY_OFF;
	game->get_map_window()->set_x_ray_view(x_ray ? X_RAY_CHEAT_ON: X_RAY_OFF, true);
	string message = x_ray ? "X-ray mode" : "X-ray mode off";
	new TextEffect(message);
}

void ActionHealParty(int const *params)
{
	party->heal();
	party->cure();
	new TextEffect("Party healed");
}

void ActionTeleportToCursor(int const *params)
{
	game->get_map_window()->teleport_to_cursor();
}

void ActionToggleCheats(int const *params)
{
	bool cheats = !game->are_cheats_enabled();
	game->set_cheats_enabled(cheats);
	string message = cheats ? "Cheats enabled" : "Cheats disabled";
	new TextEffect(message);

	if(game->is_ethereal()) // doesn't change the bool's value
		party->set_ethereal(cheats);
	if(game->get_obj_manager()->is_showing_eggs())
		game->get_egg_manager()->set_egg_visibility(cheats);

	X_RayType xray = game->get_map_window()->get_x_ray_view();
	if(xray == X_RAY_CHEAT_OFF)
		game->get_map_window()->set_x_ray_view(X_RAY_CHEAT_ON);
	else if(xray == X_RAY_CHEAT_ON)
		game->get_map_window()->set_x_ray_view(X_RAY_CHEAT_OFF);
}

void ActionDoNothing(int const *params)
{
}
