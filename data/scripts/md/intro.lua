local lua_file = nil

--load common functions
lua_file = nuvie_load("common/intro_common.lua"); lua_file();



local function origin_fx_sequence()
   local g_img_tbl = image_load_all("title.lzc")
   
   canvas_set_palette("strax.pal", 0)

   
   local stars = sprite_new(g_img_tbl[0][0], 0, 24, true)
   local logo_image = image_new(282,82)
   image_blit(logo_image, g_img_tbl[0][1],0,16)
   image_blit(logo_image, g_img_tbl[0][2],g_img_tbl[0][1].w,14)
   image_blit(logo_image, g_img_tbl[0][3],g_img_tbl[0][1].w+g_img_tbl[0][2].w,0)
   
   local  logo = sprite_new(logo_image, 20, 70, false)
   
   local planet = sprite_new(g_img_tbl[12], 160, 48, true)

   planet.clip_x = 0
   planet.clip_y = 0
   planet.clip_w = 320
   planet.clip_h = 152
   local players = {}
   players[1] = create_player_sprite(g_img_tbl[1][0], 58, 118)
   players[2] = create_player_sprite(g_img_tbl[2][0], 186, 118)
   players[3] = create_player_sprite(g_img_tbl[3][0], 278, 118)
   
   players[4] = create_player_sprite(g_img_tbl[4][0], 58, 126)
   players[5] = create_player_sprite(g_img_tbl[5][0], 186, 126)
   players[6] = create_player_sprite(g_img_tbl[6][0], 278, 126)
   
   players[7] = create_player_sprite(g_img_tbl[7][0], 58, 134)
   players[8] = create_player_sprite(g_img_tbl[8][0], 186, 134)
   players[9] = create_player_sprite(g_img_tbl[9][0], 278, 134)
   
   local conductor = sprite_new(g_img_tbl[10][0], 158, 98, true)
   conductor.clip_x = 0
   conductor.clip_y = 24
   conductor.clip_w = 320
   conductor.clip_h = 128

   music_play("strx_mus.lzc", 0)
   
   fade_in()
     
   local i = 0
   for i=0,6,1 do
      conductor.image = g_img_tbl[10][i]

      update_players(players, g_img_tbl)
      if poll_for_key_or_button(4) == true then return end
   end
   for i=7,13,1 do
      conductor.image = g_img_tbl[10][i]

      update_players(players, g_img_tbl)
      if poll_for_key_or_button(4) == true then return end
   end
   for i=7,12,1 do
      conductor.image = g_img_tbl[10][i]

      update_players(players, g_img_tbl)
      if poll_for_key_or_button(4) == true then return end
   end
   local j
   for i=1,4,1 do
      for j=13,15,1 do
         conductor.image = g_img_tbl[10][j]
         if poll_for_key_or_button(1) == true then return end
      end
   
      conductor.image = g_img_tbl[10][14]
      if poll_for_key_or_button(2) == true then return end
      conductor.image = g_img_tbl[10][13]
      if poll_for_key_or_button(2) == true then return end
      conductor.image = g_img_tbl[10][16]

      if poll_for_key_or_button(1) == true then return end
         play_sfx(38, false)
   end
   
   for i=16,20,1 do
      conductor.image = g_img_tbl[10][i]
      if poll_for_key_or_button(4) == true then return end
   end
   if poll_for_key_or_button(135) == true then return end
   
   --play_sfx(12, false)
   
   conductor.image = g_img_tbl[10][6]
   
   for i=1,21,1 do
      conductor.y = 98 + i * 12
      conductor.image.scale = 100 + i * 15
      
      for j=1,9,1 do
         players[j].y = players[j].y + 5
         players[j].image.scale = 100 + i * 5
         if j == 1 or j == 4 or j == 7 then
            players[j].x = players[j].x - 2
         end
         if j == 3 or j == 6 or j == 9 then
            players[j].x = players[j].x + 2
         end
      end
   
      if poll_for_esc(4) == true then return end
   end
   
   
   logo.visible = true
   logo.image.scale = 10   
   
   
   for i=1,18,1 do
      planet.y = planet.y + 6

      logo.image.scale = logo.image.scale + 5
      logo.x = math.floor((320 - logo.image.w) / 2)
      if i < 10 then
         logo.y = logo.y - 4
      else
         logo.y = logo.y + 1
      end
   
      if poll_for_key_or_button(4) == true then return end
   end
   
   if poll_for_esc(45) == true then return end
   
   fireworks(g_img_tbl, logo)
   fade_out()
end

function show_logos()
   local g_img_tbl = image_load_all("logos.lzc")
   local sprite = sprite_new(g_img_tbl[0], 0, 0, true)
   music_play("mdd_mus.lzc", 0)
   fade_in()

   poll_for_key_or_button(195)
   if should_exit() then return end
   --fade_out()
   sprite.image = g_img_tbl[1]
   fade_in()
   poll_for_key_or_button(195)
   if should_exit() then return end
   
   sprite.image = g_img_tbl[2]
   sprite.x = 15
   music_play("mdd_mus.lzc", 1)
   fade_in()
   poll_for_key_or_button(175)
   sprite.visible = false
end

function flash_effect(image)
   canvas_hide_all_sprites()

   local s = sprite_new(image, 0, 0, true)
   local bg = sprite_new(image_new(320,200,15), 0, 0, true)
   
   if poll_for_key_or_button(6) == true then return end
   
   local speed = 10
   local i
   for i=0x0,0xff,speed do
      bg.opacity = 0xff - i
      canvas_update()
   end

   return poll_for_key_or_button(300)
end

function show_fair_ground()
   local g_img_tbl = image_load_all("fair.lzc")

   canvas_hide_all_sprites()
   
   local fair_bg = sprite_new(g_img_tbl[0], 0, 24, true)
   local buildings = sprite_new(g_img_tbl[1][0], 0, 24, true)
   local buildings1 = sprite_new(g_img_tbl[1][1], 320, 24, true)
   local wheel = sprite_new(g_img_tbl[2][0], 0, 24, true)
   local column = sprite_new(g_img_tbl[3], 190, 24, true)
   local podium = sprite_new(g_img_tbl[4], 240, 24, true)
   local people = sprite_new(g_img_tbl[5][0], 0, 24, true)
   local people1 = sprite_new(g_img_tbl[5][1], 320, 24, true)
   
   local i
   for i=0,9 do
      wheel.image = g_img_tbl[2][i%5]
      if poll_for_key_or_button(7) == true then return end
   end
   
   for i=0,79 do
      buildings.x = 0 - i
      buildings1.x = 320 - i
      wheel.x = 0 - i * 2
      wheel.image = g_img_tbl[2][i%5]
      column.x = 190 - i * 2
      podium.x = 240 - i * 2
      people.x = 0 - i * 3
      people1.x = 320 - i * 3
      if poll_for_key_or_button(7) == true then return end
   end
   
   if poll_for_key_or_button(15) == true then return end

end

function show_lowell()   

   flash_effect(image_load("credits.lzc", 0))

   canvas_hide_all_sprites()
   
   g_img_tbl = image_load_all("lowell.lzc")
   local bg = sprite_new(g_img_tbl[0][0], 0, 24, true)
   local lowell = sprite_new(g_img_tbl[0][1], 0, 24, true)

   fade_in()
   
   for i=1,20 do
      lowell.image = g_img_tbl[0][i]
      if poll_for_key_or_button(7) == true then return end
   end
   
   flash_effect(image_load("credits.lzc", 1))
   
   if poll_for_key_or_button(60) == true then return end
end

function show_fuse()
   
   canvas_hide_all_sprites()
   
   g_img_tbl = image_load_all("fuse.lzc")
   bg = sprite_new(g_img_tbl[0][0], 0, 24, true)
   local fuse = sprite_new(g_img_tbl[0][1], 0, 24, true)
   
   music_play("mdd_mus.lzc", 2)
      
   fade_in()
         
   for i=1,28 do
      fuse.image = g_img_tbl[0][i]
      if poll_for_key_or_button(4) == true then return end
   end
   
      flash_effect(image_load("credits.lzc", 2))
end

function show_flash()

   canvas_hide_all_sprites()
   
      local g_img_tbl = image_load_all("fair.lzc")

      canvas_hide_all_sprites()
      
      local fair_bg = sprite_new(g_img_tbl[0], 0, 24, true)
      local buildings = sprite_new(g_img_tbl[1][0], -80, 24, true)
      local buildings1 = sprite_new(g_img_tbl[1][1], 240, 24, true)
      local column = sprite_new(g_img_tbl[3], 30, 24, true)
      local podium = sprite_new(g_img_tbl[4], 80, 24, true)
   
   music_play("mdd_mus.lzc", 3)
   
   g_img_tbl = image_load_all("flash.lzc")

   fade_in()
   local s = sprite_new(g_img_tbl[0][0], 80, 24, true)
   local i
   for i=0,9 do
      s.image = g_img_tbl[0][i]
      if poll_for_key_or_button(7) == true then return end
   end
   for i=0,8 do
      s.image = g_img_tbl[1][i]
      if poll_for_key_or_button(7) == true then return end
   end
   
   flash_effect(image_load("credits.lzc", 3))
end

function show_flight()
   canvas_hide_all_sprites()

   local g_img_tbl = image_load_all("flight.lzc")
   local bg = sprite_new(g_img_tbl[0], 0, 24, true)
   music_play("mdd_mus.lzc", 4)
   fade_in(12)
   local capsule = create_sprite(g_img_tbl[1], 270, 140) --sprite_new(g_img_tbl[1], 270, 140, true)

   local i
   for i=0,12 do
      capsule.image.scale = 1 + i * i + (i * 2)
      capsule.x = 270 - i * 4 * i
      capsule.y = 140 - i * 2 * i
      if poll_for_key_or_button(6) == true then return end
   end
   
   poll_for_key_or_button(105)
   --display_image_table(g_img_tbl)
end

function show_cabin()
   canvas_hide_all_sprites()

   local g_img_tbl = image_load_all("cabin.lzc")
   
   --display_image_table(g_img_tbl)
   
   local bg = sprite_new(g_img_tbl[0], 0, 24, true)
   local cloud = sprite_new(g_img_tbl[1][0], 116, 28, true)
   local cloud1 = sprite_new(g_img_tbl[1][0], 222, 53, true)
   local man = create_sprite(g_img_tbl[2], 0, 50)
   local silhouette = create_sprite(g_img_tbl[3], -900, -150)
   local hat1 = create_sprite(g_img_tbl[4], 62, -150)
   local hat2 = create_sprite(g_img_tbl[5], 62, 0)
   local hat3 = create_sprite(g_img_tbl[6], 62, 0)
             
   music_play("mdd_mus.lzc", 5)
   
   --FIXME rotate sprites.
   
   local i
   for i=0,30 do
      cloud.image = g_img_tbl[1][(i + 2) % 15]
      cloud1.image = g_img_tbl[1][i % 15]
      silhouette.x = i * 64 - 900
      silhouette.y = i * 16 - 150
      man.x = 0 - (i * 4)
      man.y = i * i * 2 + 50
      man.image.scale = 100 + i * 32
      hat1.x = i * 32 + 62
      if i > 6 then
         hat1.y = i * 4 + 15
      else
         hat1.y = i * 8 + 15
      end
      
      hat2.x = i * 32 - 150
      hat2.y = i * 8

      hat3.x = i * 32 - 500
      hat3.y = i * 4
            
      if poll_for_key_or_button(8) == true then return end
   end
   
   flash_effect(image_load("credits.lzc", 4))
end

function show_mars_flash()
   canvas_hide_all_sprites()

   local g_img_tbl = image_load_all("mars.lzc")
   
   music_play("mdd_mus.lzc", 6)
   
   local bg = sprite_new(g_img_tbl[0][0], 0, 24, true)
   local flash = sprite_new(g_img_tbl[0][1], 0, 24, true)
   
   fade_in(12)
   
   if poll_for_key_or_button(60) == false then

   local i
   for i=-1,1 do
      local j
      for j=0,2 do
      local k
         for k=-1,2 do
            flash.image = g_img_tbl[0][3-math.abs(k)]

            local delay = 3
            if i == 0 and k == 0 then
               delay = 10
            end
            if poll_for_key_or_button(delay) == true then
               flash_effect(image_load("credits.lzc", 5))
               return
            end
         end

         if poll_for_key_or_button(5) == true then
            flash_effect(image_load("credits.lzc", 5))
            return
         end
      end
      
      if poll_for_key_or_button(30) == true then
         flash_effect(image_load("credits.lzc", 5))
         return
      end
   end
   
   end
   
   poll_for_key_or_button(40)
   
   flash_effect(image_load("credits.lzc", 5))
end

function play_intro()
   canvas_set_palette("md_title.pal", 0)

   show_logos()
   if should_exit() then return end

   show_fair_ground()
   if should_exit() then return end
   
   show_lowell()
   if should_exit() then return end

   show_fuse()
   if should_exit() then return end

   show_flash()
   if should_exit() then return end

   show_flight()
   if should_exit() then return end
    
   show_cabin()
   if should_exit() then return end
 
   show_mars_flash()
   if should_exit() then return end
   
end

function run_introduction()
end

function create_character()
end

function about_martian_dreams()
end

function journey_onward()
end

local g_menu_idx = 0
local g_menu_cursor_sprite = nil

function execute_menu_item(cursor_pos)
   if cursor_pos ~= nil then
      set_menu_cursor_pos(cursor_pos)
   end
   
   if g_menu_idx == 0 then -- story so far
      run_introduction()
   elseif g_menu_idx == 1 then -- create char
      if create_character() == true then
         return "J" -- starting new game
      end
   elseif g_menu_idx == 2 then -- journey onward
      journey_onward()
      return "J"
   elseif g_menu_idx == 3 then -- about md
      about_martian_dreams()
   end
   
   return "";
end

function update_menu_cursor()
   local box_y_tbl = {[0]=130,[1]=147,[2]=164,[3]=181}
   g_menu_cursor_sprite.y = box_y_tbl[g_menu_idx]
end

function set_menu_cursor_pos(new_pos)
   g_menu_idx = new_pos
   update_menu_cursor()
end

function menu_cursor_down()
   g_menu_idx = (g_menu_idx + 1) % 4
   update_menu_cursor()
end

function menu_cursor_up()
   g_menu_idx = g_menu_idx - 1
   if g_menu_idx < 0 then
      g_menu_idx = 3
   end
   update_menu_cursor()
end

local g_mouse_cursor_visible = false

function show_mouse_cursor()
   if g_mouse_cursor_visible == false then
      mouse_cursor_set_pointer(9)
      mouse_cursor_visible(true)
      g_mouse_cursor_visible = true
   end
end

function hide_mouse_cursor()
   if g_mouse_cursor_visible == true then
      mouse_cursor_visible(false)
      g_mouse_cursor_visible = false
   end
end

function main_menu()
   canvas_hide_all_sprites()
   music_stop()
   local g_img_tbl = image_load_all("mdmenu.lzc")
      
   --display_image_table(g_img_tbl)

   local bg = sprite_new(g_img_tbl[0][0], 0, 0, true)

   fade_in()

   g_menu_cursor_sprite = sprite_new(g_img_tbl[0][2], 26, 0, true)
   update_menu_cursor()

   while true do
      canvas_update()
      input = input_poll(true)
      if input ~= nil then
         if input == SDLK_q then -- q
            return "Q"
         elseif input == SDLK_RETURN or input == SDLK_SPACE or input == KP_ENTER then -- space or return
            if execute_menu_item() == "J" then
               return "J"
            end
         elseif input == SDLK_r or input == SDLK_i then -- Run Introduction
            execute_menu_item(0)
         elseif input == SDLK_c then -- c (create char)
            if execute_menu_item(1) == "J" then
               return "J" -- starting new game
            end
         elseif input == SDLK_j or input == SDLK_g then -- j, g (journey onward, continue Game)
            execute_menu_item(2)
            return "J"
         elseif input == SDLK_a then -- a (about MD)
            execute_menu_item(3)
         elseif input == SDLK_DOWN or input == SDL_KP2 then -- down key
            menu_cursor_down()
         elseif input == SDLK_UP or input == SDL_KP8 then -- up key
            menu_cursor_up()
         elseif input == MOUSE_MOTION or input == MOUSE_CLICK then --mouse movement
            show_mouse_cursor()
            local x = get_mouse_x()
            local y = get_mouse_y()
            if x > 57 and x < 260 and y > 130 then
               if y > 130 and y < 148 then -- run introduction
                  set_menu_cursor_pos(0)
               elseif y > 147 and y < 164 then -- create new char
                  set_menu_cursor_pos(1)
               elseif y > 163 and y < 181 then -- continue game
                  set_menu_cursor_pos(2)
               elseif y > 180 then -- about MD
                  set_menu_cursor_pos(3)
               end
               if input == MOUSE_CLICK then
                  if execute_menu_item() == "J" then
                     return "J"
                  end
               end
            end
         end
      end
   end

end


mouse_cursor_visible(false)
canvas_set_update_interval(25)
canvas_set_bg_color(0)
canvas_set_opacity(0)

origin_fx_sequence()
canvas_hide_all_sprites()
play_intro()

if main_menu() == "Q" then -- returns "Q" for quit or "J" for Journey Onward
   hide_mouse_cursor()
   fade_out(6)
   config_set("config/quit", true)
end

canvas_hide()
