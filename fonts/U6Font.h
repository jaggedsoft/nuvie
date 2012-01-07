#ifndef __U6Font_h__
#define __U6Font_h__

/*
 *  U6Font.h
 *  Nuvie
 *
 *  Created by Eric Fry on Wed Jan 28 2003.
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

#define FONT_COLOR_U6_NORMAL    0x48
#define FONT_COLOR_U6_HIGHLIGHT 0x0c

#include "Font.h"

class Configuration;
class Screen;

class U6Font : public Font
{

 public:

   U6Font();
   ~U6Font();

   bool init(unsigned char *data, uint16 num_chars, uint16 char_offset);

   uint16 drawChar(Screen *screen, uint8 char_num, uint16 x, uint16 y,
                 uint8 color = FONT_COLOR_U6_NORMAL);
  protected:

};

#endif /* __U6Font_h__ */