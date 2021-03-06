/*
 *  SfxManager.h
 *  Nuvie
 *
 *  Created by Eric Fry on Tue Feb 1 2011.
 *  Copyright (c) 2011. All rights reserved.
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

#ifndef SFXMANAGER_H
#define SFXMANAGER_H

#include "nuvieDefs.h"
#include "Configuration.h"
#include "mixer.h"
#include "sfx.h"



class SfxManager {
public:
	SfxManager(Configuration *cfg, Audio::Mixer *m) : config(cfg), mixer(m) { sfx_duration = 0; };
	~SfxManager() {};


virtual bool playSfx(SfxIdType sfx_id, uint8 volume) = 0;
virtual bool playSfxLooping(SfxIdType sfx_id, Audio::SoundHandle *handle, uint8 volume) = 0;

uint32 getLastSfxDuration() { return sfx_duration; }

protected:
	Configuration *config;
	Audio::Mixer *mixer;
	uint32 sfx_duration; //duration of the last sfx played in milliseconds.
};

#endif
