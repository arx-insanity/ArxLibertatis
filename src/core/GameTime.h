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
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved

#ifndef ARX_CORE_GAMETIME_H
#define ARX_CORE_GAMETIME_H

#include "core/TimeTypes.h"
#include "platform/Platform.h"
#include "util/Flags.h"


class PlatformTime {
	
	PlatformInstant m_frameStartTime;
	ShortPlatformDuration m_lastFrameDuration;
	
public:
	
	static inline constexpr ShortPlatformDuration MaxFrameDuration = 100ms;
	
	PlatformTime()
		: m_frameStartTime(0)
		, m_lastFrameDuration(0)
	{ }
	
	void updateFrame();
	
	void overrideFrameDuration(ShortPlatformDuration duration) {
		m_lastFrameDuration = duration;
	}
	
	[[nodiscard]] PlatformInstant frameStart() {
		return m_frameStartTime;
	}
	
	[[nodiscard]] ShortPlatformDuration lastFrameDuration() {
		arx_assume(m_lastFrameDuration >= 0 && m_lastFrameDuration <= MaxFrameDuration);
		return m_lastFrameDuration;
	}
	
};

extern PlatformTime g_platformTime;


class GameTime {
	
public:
	
	enum PauseFlag {
		PauseInitial   = (1 << 0),
		PauseMenu      = (1 << 1),
		PauseCinematic = (1 << 2),
		PauseUser      = (1 << 3),
	};
	DECLARE_FLAGS(PauseFlag, PauseFlags)
	
private:
	
	GameInstant m_now;
	ShortGameDuration m_lastFrameDuration;
	float m_speed;
	PauseFlags m_paused;
	
public:
	
	static inline constexpr ShortGameDuration MaxFrameDuration = 100ms;
	
	GameTime();
	
	void pause(PauseFlags flags) {
		m_paused |= flags;
	}
	
	void resume(PauseFlags flags) {
		m_paused &= ~flags;
	}
	
	void reset(GameInstant time);
	
	[[nodiscard]] GameInstant now() const {
		arx_assume(m_now >= 0);
		return m_now;
	}
	
	void update(ShortPlatformDuration frameDuration);
	
	PauseFlags isPaused() const {
		return m_paused;
	}
	
	[[nodiscard]] ShortGameDuration lastFrameDuration() const {
		arx_assume(m_lastFrameDuration >= 0 && m_lastFrameDuration <= MaxFrameDuration);
		return m_lastFrameDuration;
	}
	
	[[nodiscard]] float speed() const {
		arx_assume(m_speed >= 0.f && m_speed <= 1.f);
		return m_speed;
	}
	
	void setSpeed(float speed) {
		arx_assume(speed >= 0.f && speed <= 1.f);
		m_speed = speed;
	}
	
};

DECLARE_FLAGS_OPERATORS(GameTime::PauseFlags)

extern GameTime g_gameTime;

#endif // ARX_CORE_GAMETIME_H
