/*
 * Copyright 2011-2022 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#ifndef ARX_CINEMATIC_CINEMATICKEYFRAMER_H
#define ARX_CINEMATIC_CINEMATICKEYFRAMER_H

#include "cinematic/Cinematic.h" // for CinematicLight
#include "math/Vector.h"
#include "platform/Platform.h"


struct CinematicKeyframe {
	
	int frame = 0;
	int numbitmap = 0;
	int fx = 0; // associated fx
	short typeinterp = 0;
	short force = 0;
	Vec3f pos = Vec3f(0.f);
	float angz = 0.f;
	Color color;
	Color colord;
	Color colorf;
	float speed = 0.f;
	CinematicLight light;
	Vec3f posgrille = Vec3f(0.f);
	float angzgrille = 0.f;
	float speedtrack = 0.f;
	int idsound = 0;
	
	CinematicKeyframe() arx_noexcept_default
	
};

struct CinematicTrack {
	
	int endframe;
	float currframe;
	float fps;
	bool pause;
	
	std::vector<CinematicKeyframe> key;
	
	CinematicTrack(int endframe_, float fps_) noexcept;
	
};

void DeleteTrack();
void AllocTrack(int ef, float fps);
void AddKeyLoad(const CinematicKeyframe & key);
void GereTrack(Cinematic * c, PlatformDuration frameDuration, bool resized, bool play);

void PlayTrack(Cinematic * c);
void SetCurrFrame(int frame);
float GetTrackFPS();

float GetTimeKeyFramer();
void UpDateAllKeyLight();

#endif // ARX_CINEMATIC_CINEMATICKEYFRAMER_H
