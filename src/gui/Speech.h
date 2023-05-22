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

#ifndef ARX_GUI_SPEECH_H
#define ARX_GUI_SPEECH_H

#include <string>
#include <string_view>

#include "core/TimeTypes.h"
#include "game/GameTypes.h"
#include "audio/AudioTypes.h"
#include "math/Angle.h"
#include "platform/Platform.h"


struct EERIE_SCRIPT;
class Entity;

enum CinematicSpeechMode {
	ARX_CINE_SPEECH_NONE,
	ARX_CINE_SPEECH_ZOOM, // uses start/endangle alpha & beta, startpos & endpos
	ARX_CINE_SPEECH_CCCTALKER_L,
	ARX_CINE_SPEECH_CCCTALKER_R,
	ARX_CINE_SPEECH_CCCLISTENER_L,
	ARX_CINE_SPEECH_CCCLISTENER_R,
	ARX_CINE_SPEECH_SIDE,
	ARX_CINE_SPEECH_KEEP,
	ARX_CINE_SPEECH_SIDE_LEFT
};

struct CinematicSpeech {
	
	CinematicSpeechMode type = ARX_CINE_SPEECH_NONE;
	Anglef startangle;
	Anglef endangle;
	float startpos = 0.f;
	float endpos = 0.f;
	float m_startdist = 0.f;
	float m_enddist = 0.f;
	float m_heightModifier = 0.f;
	EntityHandle ionum = EntityHandle_Player; // TODO is this correct?
	Vec3f pos1 = Vec3f(0.f);
	Vec3f pos2 = Vec3f(0.f);
	
	constexpr CinematicSpeech() arx_noexcept_default
	
};

enum SpeechFlag {
	ARX_SPEECH_FLAG_UNBREAKABLE = 1 << 0,
	ARX_SPEECH_FLAG_OFFVOICE    = 1 << 1,
	ARX_SPEECH_FLAG_NOTEXT      = 1 << 2,
	ARX_SPEECH_FLAG_DIRECT_TEXT = 1 << 3
};
DECLARE_FLAGS(SpeechFlag, SpeechFlags)
DECLARE_FLAGS_OPERATORS(SpeechFlags)

struct Speech {
	
	audio::SourcedSample sample;
	long mood = 0;
	SpeechFlags flags;
	GameInstant time_creation;
	GameDuration duration;
	float fDeltaY = 0.f;
	int iTimeScroll = 0;
	float fPixelScroll = 0.f;
	std::string text;
	Entity * speaker = nullptr;
	CinematicSpeech cine;
	
	Entity * scriptEntity = nullptr;
	const EERIE_SCRIPT * script = nullptr;
	size_t scriptPos = 0;
	
};

void ARX_SPEECH_Reset();
void ARX_SPEECH_Update();

Speech * getSpeechForEntity(const Entity & entity);

/*!
 * Add an entry to the conversation view.
 * \param data is a sample name / localised string id
 */
Speech * ARX_SPEECH_AddSpeech(Entity & speaker, std::string_view data, long mood, SpeechFlags flags = 0);
void ARX_SPEECH_ReleaseIOSpeech(const Entity & entity);
void ARX_SPEECH_ClearIOSpeech(const Entity & entity);

Speech * getCinematicSpeech();

#endif // ARX_GUI_SPEECH_H
