/*
 *  SEActor.cpp
 *  Nuvie
 *
 *  Created by the Nuvie Team on Thur July 12 2012.
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

#include "SEActor.h"

SEActor::SEActor(Map *m, ObjManager *om, GameClock *c): WOUActor(m,om,c)
{
}

SEActor::~SEActor()
{
}

bool SEActor::init()
{
 Actor::init();
 return true;
}

bool SEActor::will_not_talk()
{
	if(worktype == 0x07 || worktype == 0x8 || worktype == 0x9b)
		return true;
	return false;
}
