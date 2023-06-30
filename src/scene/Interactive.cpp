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

#include "scene/Interactive.h"

#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/strided.hpp>

#include "ai/Anchors.h"
#include "ai/Paths.h"

#include "animation/Animation.h"
#include "animation/AnimationRender.h"

#include "core/Application.h"
#include "core/GameTime.h"
#include "core/Config.h"
#include "core/Core.h"

#include "game/Camera.h"
#include "game/Damage.h"
#include "game/EntityManager.h"
#include "game/Equipment.h"
#include "game/Inventory.h"
#include "game/Item.h"
#include "game/Levels.h"
#include "game/NPC.h"
#include "game/Player.h"
#include "game/npc/Dismemberment.h"

#include "gui/Cursor.h"
#include "gui/Dragging.h"
#include "gui/Speech.h"
#include "gui/Interface.h"
#include "gui/book/Book.h"
#include "gui/hud/SecondaryInventory.h"

#include "graphics/Draw.h"
#include "graphics/Math.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/data/MeshManipulation.h"
#include "graphics/particle/ParticleEffects.h"
#include "graphics/particle/MagicFlare.h"
#include "graphics/texture/TextureStage.h"

#include "io/fs/FilePath.h"
#include "io/fs/Filesystem.h"
#include "io/fs/SystemPaths.h"
#include "io/resource/ResourcePath.h"
#include "io/resource/PakReader.h"
#include "io/log/Logger.h"

#include "math/Random.h"

#include "physics/Collisions.h"
#include "physics/CollisionShapes.h"
#include "physics/Physics.h"

#include "platform/Thread.h"
#include "platform/profiler/Profiler.h"

#include "scene/ChangeLevel.h"
#include "scene/GameSound.h"
#include "scene/Scene.h"
#include "scene/LinkedObject.h"
#include "scene/LoadLevel.h"
#include "scene/Light.h"
#include "scene/Object.h"

#include "script/ScriptEvent.h"

#include "util/Range.h"


long HERO_SHOW_1ST = 1;

static bool IsCollidingInter(Entity * io, const Vec3f & pos);
static Entity * AddCamera(const res::path & classPath, EntityInstance instance = -1);
static Entity * AddMarker(const res::path & classPath, EntityInstance instance = -1);

bool ValidIOAddress(const Entity * io) {
	
	if(!io) {
		return false;
	}
	
	for(Entity & entity : entities) {
		if(io == &entity) {
			return true;
		}
	}
	
	return false;
}

s32 ARX_INTERACTIVE_GetPrice(Entity * io, Entity * shop) {
	
	if(!io || !(io->ioflags & IO_ITEM)) {
		return 0;
	}
	
	float shop_multiply = shop ? shop->shop_multiply : 1.f;
	float durability_ratio = io->durability / io->max_durability;
	
	return s32(float(io->_itemdata->price) * shop_multiply * durability_ratio);
}

s32 ARX_INTERACTIVE_GetSellValue(Entity * item, Entity * shop, long count) {
	
	float price = float(ARX_INTERACTIVE_GetPrice(item, shop) / 3 * count);
	
	return s32(price + price * player.m_skillFull.intuition * 0.005f);
}

static void ARX_INTERACTIVE_ForceIOLeaveZone(Entity * io) {
	
	Zone * op = io->inzone;
	if(!op || op->controled.empty()) {
		return;
	}
	
	if(Entity * controller = entities.getById(op->controled)) {
		ScriptParameters parameters;
		parameters.emplace_back(io->idString());
		parameters.emplace_back(op->name);
		SendIOScriptEvent(nullptr, controller, SM_CONTROLLEDZONE_LEAVE, parameters);
	}
	
}

void ARX_INTERACTIVE_DestroyDynamicInfo(Entity * io) {
	
	if(!io) {
		return;
	}
	
	ARX_INTERACTIVE_ForceIOLeaveZone(io);
	
	for(EntityHandle & equipment : player.equiped) {
		if(equipment == io->index()) {
			ARX_EQUIPMENT_UnEquip(entities.player(), io, 1);
			equipment = EntityHandle();
			arx_assert(!isEquippedByPlayer(io));
			break;
		}
	}
	
	ARX_SCRIPT_EventStackClearForIo(io);
	
	spells.endByCaster(io->index());
	
	if(io->flarecount) {
		MagicFlareReleaseEntity(io);
	}
	
	if(io->ioflags & IO_NPC) {
		// to check again later...
		long count = 50;
		while((io->_npcdata->pathfind.pathwait == 1) && count--) {
			Thread::sleep(1ms);
		}
		delete[] io->_npcdata->pathfind.list;
		io->_npcdata->pathfind = IO_PATHFIND();
	}
	
	lightHandleDestroy(io->dynlight);
	
	IO_UnlinkAllLinkedObjects(io);
	
	if((io->ioflags & IO_NPC) && io->_npcdata->weapon) {
		Entity * weapon = io->_npcdata->weapon;
		arx_assert(ValidIOAddress(weapon));
		if(locateInInventories(weapon)) {
			io->_npcdata->weapon = nullptr;
			// Owner hasn't changed
		} else {
			weapon->setOwner(nullptr);
			weapon->show = SHOW_FLAG_IN_SCENE;
			weapon->ioflags |= IO_NO_NPC_COLLIDE;
			weapon->pos = weapon->obj->vertexWorldPositions[weapon->obj->origin].v;
			// TODO old broken code suggested that physics sim might be enabled here
		}
	}
	
}

void ARX_INTERACTIVE_Show_Hide_1st(Entity * io, bool hide1st) {
	
	ARX_PROFILE_FUNC();
	
	if(!io || HERO_SHOW_1ST == long(hide1st)) {
		return;
	}
	
	HERO_SHOW_1ST = long(hide1st);
	
	if(VertexSelectionId selection = EERIE_OBJECT_GetSelection(io->obj, "1st")) {
		for(EERIE_FACE & face : io->obj->facelist) {
			for(VertexId vertex : face.vid) {
				if(IsInSelection(io->obj, vertex, selection)) {
					if(hide1st) {
						face.facetype |= POLY_HIDE;
					} else {
						face.facetype &= ~POLY_HIDE;
					}
					break;
				}
			}
		}
	}
	
	ARX_INTERACTIVE_HideGore(entities.player(), false);
	
}

void ARX_INTERACTIVE_RemoveGoreOnIO(Entity * io) {
	
	if(!io || !io->obj) {
		return;
	}
	
	if(MaterialId gore = getGoreMaterial(*io->obj)) {
		for(EERIE_FACE & face : io->obj->facelist) {
			if(face.material == gore) {
				face.facetype |= POLY_HIDE;
				face.material = { };
			}
		}
	}
	
}

void ARX_INTERACTIVE_HideGore(Entity * io, bool unhideNonGore) {
	
	if(!io || !io->obj) {
		return;
	}
	
	if(io == entities.player() && unhideNonGore) {
		return;
	}
	
	if(MaterialId gore = getGoreMaterial(*io->obj)) {
		for(EERIE_FACE & face : io->obj->facelist) {
			if(face.material == gore) {
				face.facetype |= POLY_HIDE;
			} else if(unhideNonGore) {
				face.facetype &= ~POLY_HIDE;
			}
		}
	}
	
}

bool ForceNPC_Above_Ground(Entity * io) {
	
	arx_assert(io && (io->ioflags & IO_NPC) && !(io->ioflags & IO_PHYSICAL_OFF));
	
	io->physics.cyl.origin = io->pos;
	AttemptValidCylinderPos(io->physics.cyl, io, CFLAG_NO_INTERCOL);
	if(glm::abs(io->pos.y - io->physics.cyl.origin.y) < 45.f) {
		io->pos.y = io->physics.cyl.origin.y;
		return true;
	}
	
	return false;
}

// Unlinks all linked objects from all IOs
void UnlinkAllLinkedObjects() {
	
	for(Entity & entity : entities) {
		EERIE_LINKEDOBJ_ReleaseData(entity.obj);
	}
	
}

// First is always the player
std::vector<TREATZONE_IO> treatio;

void TREATZONE_Clear() {
	treatio.clear();
}

void TREATZONE_RemoveIO(const Entity * io) {
	for(TREATZONE_IO & entry : treatio) {
		if(entry.io == io) {
			entry.io = nullptr;
			entry.ioflags = 0;
			entry.show = SHOW_FLAG_MEGAHIDE;
		}
	}
}

void TREATZONE_AddIO(Entity * io, bool justCollide) {
	
	for(const auto & entry : treatio) {
		if(entry.io == io) {
			return;
		}
	}
	
	TREATZONE_IO entry;
	entry.io = io;
	entry.ioflags = io->ioflags;
	if(justCollide) {
		entry.ioflags |= IO_JUST_COLLIDE;
	}
	entry.show = io->show;
	
	treatio.push_back(entry);
	
}

void CheckSetAnimOutOfTreatZone(Entity * io, AnimLayer & layer) {
	
	arx_assert(io);
	
	if(layer.cur_anim && !(io->gameFlags & GFLAG_ISINTREATZONE)
	   && fartherThan(io->pos, g_camera->m_pos, 2500.f)) {
		layer.ctime = layer.currentAltAnim()->anim_time - 1ms;
	}
	
}

void PrepareIOTreatZone(long flag) {
	
	ARX_PROFILE_FUNC();
	
	static long status = -1;
	static Vec3f lastpos(0.f);
	
	const Vec3f cameraPos = g_camera->m_pos;
	
	if(flag || status == -1) {
		status = 0;
		lastpos = cameraPos;
	} else if(status == 3) {
		status = 0;
	}
	
	if(fartherThan(cameraPos, lastpos, 100.f)) {
		status = 0;
		lastpos = cameraPos;
	}
	
	if(status++) {
		return;
	}
	
	TREATZONE_Clear();
	TREATZONE_AddIO(entities.player());
	
	RoomHandle cameraRoom = ARX_PORTALS_GetRoomNumForPosition(cameraPos, RoomPositionForCamera);
	RoomHandle playerRoom = ARX_PORTALS_GetRoomNumForPosition(player.pos, RoomPositionForCamera);
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment)) {
			toequip->room = playerRoom;
			toequip->requestRoomUpdate = false;
		}
	}
	
	if(g_draggedEntity) {
		TREATZONE_AddIO(g_draggedEntity);
	}
	
	float TREATZONE_LIMIT = 3200;
	if(!g_roomDistance.empty()) {
		TREATZONE_LIMIT += 600;
		if(g_currentArea == AreaId(4)) {
			TREATZONE_LIMIT += 1200;
		}
		if(g_camera->cdepth > 3000) {
			TREATZONE_LIMIT += 500;
		}
		if(g_camera->cdepth > 4000) {
			TREATZONE_LIMIT += 500;
		}
		if(g_camera->cdepth > 6000) {
			TREATZONE_LIMIT += 500;
		}
	}
	
	for(Entity & entity : entities) {
		
		if(entity.show != SHOW_FLAG_IN_SCENE
		   && entity.show != SHOW_FLAG_TELEPORTING
		   && entity.show != SHOW_FLAG_ON_PLAYER
		   && entity.show != SHOW_FLAG_HIDDEN) {
			continue;
		}
		
		if(entity == *entities.player()) {
			continue;
		}
		
		bool treat;
		if(entity.ioflags & IO_CAMERA) {
			treat = false;
		} else if(entity.ioflags & IO_MARKER) {
			treat = false;
		} else if((entity.ioflags & IO_NPC) && (entity._npcdata->pathfind.flags & PATHFIND_ALWAYS)) {
			treat = true;
		} else {
			float dists;
			if(cameraRoom) {
				if(entity.show == SHOW_FLAG_TELEPORTING) {
					Vec3f pos = GetItemWorldPosition(&entity);
					dists = arx::distance2(cameraPos, pos);
				} else {
					if(entity.requestRoomUpdate) {
						UpdateIORoom(&entity);
					}
					dists = square(SP_GetRoomDist(entity.pos, cameraPos, entity.room, cameraRoom));
				}
			} else {
				if(entity.show == SHOW_FLAG_TELEPORTING) {
					Vec3f pos = GetItemWorldPosition(&entity);
					dists = arx::distance2(cameraPos, pos);
				} else {
					dists = arx::distance2(entity.pos, cameraPos);
				}
			}
			treat = (dists < square(TREATZONE_LIMIT));
		}
		if(&entity == g_draggedEntity) {
			treat = true;
		}
		
		if(treat) {
			entity.gameFlags |= GFLAG_ISINTREATZONE;
			TREATZONE_AddIO(&entity);
			if((entity.ioflags & IO_NPC) && entity._npcdata->weapon) {
				Entity & weapon = *entity._npcdata->weapon;
				weapon.room = entity.room;
				weapon.requestRoomUpdate = entity.requestRoomUpdate;
			}
		} else if((entity.gameFlags & GFLAG_ISINTREATZONE)
		          && SendIOScriptEvent(nullptr, &entity, SM_TREATOUT) != REFUSE) {
			// Going away
			if(entity.ioflags & IO_NPC) {
				entity._npcdata->pathfind.flags &= ~PATHFIND_ALWAYS;
			}
			entity.gameFlags &= ~GFLAG_ISINTREATZONE;
		}
		
	}
	
	size_t M_TREAT = treatio.size();
	
	for(Entity & entity : entities) {
		
		if(entity.show != SHOW_FLAG_IN_SCENE
		   && entity.show != SHOW_FLAG_TELEPORTING
		   && entity.show != SHOW_FLAG_ON_PLAYER
		   && entity.show != SHOW_FLAG_HIDDEN) {
			continue;
		}
		
		if(!(entity.gameFlags & GFLAG_ISINTREATZONE) || (entity.ioflags & (IO_CAMERA | IO_ITEM | IO_MARKER))) {
			continue;
		}
		
		if(entity == *entities.player()) {
			continue;
		}
		
		bool toadd = false;
		for(size_t i = 1; i < M_TREAT; i++) {
			Entity * treat = treatio[i].io;
			if(treat && closerThan(entity.pos, treat->pos, 300.f)) {
				toadd = true;
				break;
			}
		}
		
		if(toadd) {
			TREATZONE_AddIO(&entity, true);
		}
		
	}
	
}

/*!
 * \brief Removes an IO loaded by a script command
 */
void CleanScriptLoadedIO() {
	
	for(Entity & entity : entities) {
		
		if(entity == *entities.player()) {
			continue;
		}
		
		if(entity.scriptload) {
			delete &entity;
		} else if(entity.show != SHOW_FLAG_IN_INVENTORY
		          && entity.show != SHOW_FLAG_ON_PLAYER
		          && entity.show != SHOW_FLAG_LINKED) {
			// TODO why not jus leave it as is?
			entity.show = SHOW_FLAG_IN_SCENE;
			arx_assert(!locateInInventories(&entity));
		}
		
	}
	
}

/*!
 * \brief Restores an IO to its initial status (Game start Status)
 */
void RestoreInitialIOStatus() {
	
	arx_assert(entities.player());
	
	resetAllNpcBehaviors();
	
	entities.player()->spellcast_data.castingspell = SPELL_NONE;
	
	for(Entity & entity : entities) {
		if(entity != *entities.player()) {
			RestoreInitialIOStatusOfIO(&entity);
		}
	}
	
}

bool ARX_INTERACTIVE_USEMESH(Entity * io, const res::path & temp) {
	
	if(!io || temp.empty()) {
		return false;
	}
	
	if(io->ioflags & IO_NPC) {
		io->usemesh = "graph/obj3d/interactive/npc" / temp;
	} else if(io->ioflags & IO_FIX) {
		io->usemesh = "graph/obj3d/interactive/fix_inter" / temp;
	} else if(io->ioflags & IO_ITEM) {
		io->usemesh = "graph/obj3d/interactive/items" / temp;
	} else {
		io->usemesh.clear();
	}
	
	if(io->usemesh.empty()) {
		return false;
	}
	
	delete io->obj, io->obj = nullptr;
	
	bool pbox = (!(io->ioflags & IO_FIX) && !(io->ioflags & IO_NPC));
	io->obj = loadObject(io->usemesh, pbox).release();
	
	EERIE_COLLISION_Cylinder_Create(io);
	return true;
}

void ARX_INTERACTIVE_MEMO_TWEAK(Entity * io, TweakType type, const res::path & param1, const res::path & param2) {
	
	io->tweaks.resize(io->tweaks.size() + 1);
	
	io->tweaks.back().type = type;
	io->tweaks.back().param1 = param1;
	io->tweaks.back().param2 = param2;
}

void ARX_INTERACTIVE_APPLY_TWEAK_INFO(Entity * io) {
	
	for(const TWEAK_INFO & tweak : io->tweaks) {
		switch(tweak.type) {
			case TWEAK_REMOVE: EERIE_MESH_TWEAK_Do(io, TWEAK_REMOVE, res::path()); break;
			case TWEAK_TYPE_SKIN: EERIE_MESH_TWEAK_Skin(io->obj, tweak.param1, tweak.param2); break;
			case TWEAK_TYPE_ICON: ARX_INTERACTIVE_TWEAK_Icon(io, tweak.param1); break;
			case TWEAK_TYPE_MESH: ARX_INTERACTIVE_USEMESH(io, tweak.param1); break;
			default: EERIE_MESH_TWEAK_Do(io, tweak.type, tweak.param1);
		}
	}
	
}

static void ARX_INTERACTIVE_ClearIODynData(Entity & entity) {
	
	lightHandleDestroy(entity.dynlight);
	
	delete entity.symboldraw;
	entity.symboldraw = nullptr;
	
	entity.spellcast_data.castingspell = SPELL_NONE;
	
}

static void ARX_INTERACTIVE_ClearIODynData_II(Entity * io) {
	
	if(!io) {
		return;
	}
	
	ARX_INTERACTIVE_ClearIODynData(*io);
	
	io->shop_category.clear();
	io->inventory_skin.clear();
	
	io->tweaks.clear();
	io->groups.clear();
	ARX_INTERACTIVE_HideGore(io);
	ARX_SCRIPT_Timer_Clear_For_IO(io);
	
	io->stepmaterial.clear();
	io->armormaterial.clear();
	io->weaponmaterial.clear();
	io->strikespeech.clear();
	
	io->animBlend.m_active = false;
	
	for(ANIM_HANDLE * & animation : io->anims) {
		EERIE_ANIMMANAGER_ReleaseHandle(animation);
		animation = nullptr;
	}
	
	spells.removeTarget(io);
	ARX_EQUIPMENT_ReleaseAll(io);
	
	if(io->ioflags & IO_NPC) {
		delete[] io->_npcdata->pathfind.list;
		io->_npcdata->pathfind = IO_PATHFIND();
		io->_npcdata->pathfind.truetarget = EntityHandle();
		io->_npcdata->pathfind.listnb = -1;
		resetNpcBehavior(*io);
	}
	
	delete io->tweakerinfo;
	io->tweakerinfo = nullptr;
	
	g_secondaryInventoryHud.clear(io);
	
	if(io->inventory != nullptr) {
		for(auto slot : io->inventory->slots()) {
			if(slot.entity) {
				slot.entity->destroy();
			}
			arx_assert(slot.entity == nullptr);
			arx_assert(slot.show == false);
		}
		io->inventory.reset();
	}
	
	io->inventory = nullptr;
	io->gameFlags |= GFLAG_INTERACTIVITY;
	
	if(io->tweaky) {
		delete io->obj;
		io->obj = io->tweaky;
		io->tweaky = nullptr;
	}
}

void ARX_INTERACTIVE_ClearAllDynData() {
	
	resetAllNpcBehaviors();
	
	for(Entity & entity : entities) {
		if(entity != *entities.player()) {
			ARX_INTERACTIVE_ClearIODynData(entity);
		}
	}
	
}

static void RestoreIOInitPos(Entity * io) {
	if(!io) {
		return;
	}

	ARX_INTERACTIVE_Teleport(io, io->initpos);
	io->pos = io->lastpos = io->initpos;
	io->move = Vec3f(0.f);
	io->lastmove = Vec3f(0.f);
	io->angle = io->initangle;
}

void ARX_HALO_SetToNative(Entity * io) {
	io->halo.color = io->halo_native.color;
	io->halo.radius = io->halo_native.radius;
	io->halo.flags = io->halo_native.flags;
}

void RestoreInitialIOStatusOfIO(Entity * io)
{
	if(!io) {
		return;
	}

	ARX_INTERACTIVE_ClearIODynData_II(io);

	io->shop_multiply = 1.f;

	ARX_INTERACTIVE_HideGore(io, false);

	io->halo_native.color = Color3f(0.2f, 0.5f, 1.f);
	io->halo_native.radius = 45.f;
	io->halo_native.flags = 0;

	ARX_HALO_SetToNative(io);

	io->forcedmove = Vec3f(0.f);
	io->ioflags &= ~IO_NO_COLLISIONS;
	io->ioflags &= ~IO_INVERTED;
	io->lastspeechflag = 2;

	io->no_collide = { };

	MagicFlareReleaseEntity(io);

	io->flarecount = 0;
	io->inzone = nullptr;
	io->speed_modif = 0.f;
	io->basespeed = 1.f;
	io->sfx_flag = 0;
	io->max_durability = io->durability = 100;
	io->gameFlags &= ~GFLAG_INVISIBILITY;
	io->gameFlags &= ~GFLAG_MEGAHIDE;
	io->gameFlags &= ~GFLAG_NOGORE;
	io->gameFlags &= ~GFLAG_ISINTREATZONE;
	io->gameFlags &= ~GFLAG_PLATFORM;
	io->gameFlags &= ~GFLAG_ELEVATOR;
	io->gameFlags &= ~GFLAG_HIDEWEAPON;
	io->gameFlags &= ~GFLAG_NOCOMPUTATION;
	io->gameFlags &= ~GFLAG_DOOR;
	io->gameFlags &= ~GFLAG_GOREEXPLODE;
	io->invisibility = 0.f;
	io->rubber = BASE_RUBBER;
	io->scale = 1.f;
	io->move = Vec3f(0.f);
	io->type_flags = 0;
	io->m_sound = { };
	io->soundtime = 0;
	io->soundcount = 0;
	io->material = MATERIAL_STONE;
	io->collide_door_time = 0;
	io->ouch_time = 0;
	io->dmg_sum = 0;
	io->ignition = 0.f;
	io->ignit_light = { };
	io->ignit_sound = { };
	
	if(io->obj && io->obj->pbox) {
		io->obj->pbox->active = 0;
	}
	
	io->room = { };
	io->requestRoomUpdate = true;
	RestoreIOInitPos(io);
	ARX_INTERACTIVE_Teleport(io, io->initpos);
	io->animBlend.lastanimtime = GameInstant(0) + 1ms;
	io->secretvalue = -1;
	
	io->poisonous = 0;
	io->poisonous_count = 0;

	for(size_t count = 0; count < MAX_ANIM_LAYERS; count++) {
		io->animlayer[count] = AnimLayer();
	}

	if(io->obj && io->obj->pbox) {
		io->obj->pbox->storedtiming = 0;
	}
	
	io->physics.cyl.origin = io->pos;
	io->physics.cyl.radius = io->original_radius;
	io->physics.cyl.height = io->original_height;
	io->fall = 0;
	io->setOwner(nullptr);
	io->show = SHOW_FLAG_IN_SCENE;
	io->targetinfo = EntityHandle(TARGET_NONE);
	io->spellcast_data.castingspell = SPELL_NONE;
	io->spark_n_blood = 0;

	if(io->ioflags & IO_NPC) {
		io->_npcdata->climb_count = 0;
		io->_npcdata->vvpos = -99999.f;
		io->_npcdata->SPLAT_DAMAGES = 0;
		io->_npcdata->speakpitch = 1.f;
		io->_npcdata->behavior = BEHAVIOUR_NONE;
		io->_npcdata->cut = 0;
		io->_npcdata->cuts = 0;
		io->_npcdata->poisonned = 0.f;
		io->_npcdata->blood_color = Color::red;
		io->_npcdata->stare_factor = 1.f;

		io->_npcdata->weapon = nullptr;
		io->_npcdata->weaponinhand = 0;
		io->_npcdata->weapontype = 0;
		io->_npcdata->weaponinhand = 0;
		io->_npcdata->fightdecision = 0;
		io->_npcdata->walk_start_time = 0;

		io->_npcdata->reachedtarget = 0;
		io->_npcdata->lifePool.max = 20.f;
		io->_npcdata->lifePool.current = io->_npcdata->lifePool.max;
		io->_npcdata->manaPool.max = 10.f;
		io->_npcdata->manaPool.current = io->_npcdata->manaPool.max;
		io->_npcdata->critical = 5.f;
		io->infracolor = Color3f(1.f, 0.f, 0.2f);
		io->_npcdata->detect = 0;
		io->_npcdata->movemode = WALKMODE;
		io->_npcdata->reach = 20.f;
		io->_npcdata->armor_class = 0;
		io->_npcdata->absorb = 0;
		io->_npcdata->damages = 20;
		io->_npcdata->tohit = 50;
		io->_npcdata->aimtime = 0;
		io->_npcdata->aiming_start = 0;
		io->_npcdata->npcflags = 0;
		io->_npcdata->backstab_skill = 0;
		io->_npcdata->fDetect = -1;
		io->_npcdata->summoner = EntityHandle();
	}
	
	if(io->ioflags & IO_ITEM) {
		io->collision = COLLIDE_WITH_PLAYER;
		io->_itemdata->count = 1;
		io->_itemdata->maxcount = 1;
		io->_itemdata->food_value = 0;
		io->_itemdata->playerstacksize = 1;
		io->_itemdata->stealvalue = -1;
		io->_itemdata->LightValue = -1;
	} else {
		io->collision = 0;
	}
	
	if(io->ioflags & IO_FIX) {
		io->_fixdata->trapvalue = -1;
	}

}

void ARX_INTERACTIVE_TWEAK_Icon(Entity * io, const res::path & s1) {
	
	if(!io || s1.empty()) {
		return;
	}
	
	res::path icontochange = io->classPath().parent() / s1;
	
	TextureContainer * tc = TextureContainer::LoadUI(icontochange,
	                                                 TextureContainer::Level);
	if(!tc) {
		tc = TextureContainer::LoadUI("graph/interface/misc/default[icon]");
	}
	
	if(tc) {
		InventoryPos pos = removeFromInventories(io);
		io->m_inventorySize = inventorySizeFromTextureSize(tc->size());
		io->m_icon = tc;
		if(pos) {
			insertIntoInventoryAtNoEvent(io, pos);
		}
	}
	
}

// Be careful with this func...
Entity * CloneIOItem(Entity * src) {
	
	Entity * dest = AddItem(src->classPath());
	if(!dest) {
		return nullptr;
	}
	
	SendInitScriptEvent(dest);
	dest->m_icon = src->m_icon;
	InventoryPos pos = removeFromInventories(dest);
	dest->m_inventorySize = src->m_inventorySize;
	if(pos) {
		insertIntoInventoryAtNoEvent(dest, pos);
	}
	delete dest->obj;
	dest->obj = Eerie_Copy(src->obj);
	CloneLocalVars(dest, src);
	dest->_itemdata->price = src->_itemdata->price;
	dest->_itemdata->maxcount = src->_itemdata->maxcount;
	dest->_itemdata->count = src->_itemdata->count;
	dest->_itemdata->food_value = src->_itemdata->food_value;
	dest->_itemdata->stealvalue = src->_itemdata->stealvalue;
	dest->_itemdata->playerstacksize = src->_itemdata->playerstacksize;
	dest->_itemdata->LightValue = src->_itemdata->LightValue;
	
	if(src->_itemdata->equipitem) {
		dest->_itemdata->equipitem = new IO_EQUIPITEM;
		*dest->_itemdata->equipitem = *src->_itemdata->equipitem;
	}
	
	dest->locname = src->locname;
	
	if(dest->obj->pbox == nullptr && src->obj->pbox != nullptr) {
		dest->obj->pbox = std::make_unique<PHYSICS_BOX_DATA>();
		*dest->obj->pbox = *src->obj->pbox;
		dest->obj->pbox->vert = src->obj->pbox->vert;
	}
	
	return dest;
}

bool ARX_INTERACTIVE_ConvertToValidPosForIO(Entity * io, Vec3f * target) {
	
	Cylinder phys;
	if(io && io != entities.player()) {
		phys.height = io->original_height * io->scale;
		phys.radius = io->original_radius * io->scale;
	} else {
		phys.height = -200;
		phys.radius = 50;
	}
	
	phys.origin = *target;
	float count = 0;
	
	while(count < 600) {
		Vec3f mod = angleToVectorXZ(count) * count * (1.f / 3);
		
		phys.origin.x = target->x + mod.x;
		phys.origin.z = target->z + mod.z;
		float anything = CheckAnythingInCylinder(phys, io, CFLAG_JUST_TEST);

		if(glm::abs(anything) < 150.f) {
			EERIEPOLY * ep = CheckInPoly(phys.origin + Vec3f(0.f, -20.f, 0.f));
			EERIEPOLY * ep2 = CheckTopPoly(phys.origin + Vec3f(0.f, anything, 0.f));

			if(ep && ep2 && glm::abs((phys.origin.y + anything) - ep->center.y) < 20.f) {
				target->x = phys.origin.x;
				target->y = phys.origin.y + anything;
				target->z = phys.origin.z;
				return true;
			}
		}

		count += 5.f;
	}

	return false;
}

void ARX_INTERACTIVE_TeleportBehindTarget(Entity * io) {
	
	if(!io || scriptTimerExists(io, "_r_a_t_")) {
		return;
	}
	
	SCR_TIMER & timer = createScriptTimer(io, "_r_a_t_");
	timer.interval = Random::get(3000ms, 6000ms);
	timer.start = g_gameTime.now();
	timer.count = 1;
	
	io->setOwner(nullptr);
	io->show = SHOW_FLAG_TELEPORTING;
	AddRandomSmoke(*io, 10);
	ARX_PARTICLES_Add_Smoke(io->pos, 3, 20);
	io->requestRoomUpdate = true;
	io->room = { };
	ARX_PARTICLES_Add_Smoke(io->pos + Vec3f(0.f, io->physics.cyl.height * 0.5f, 0.f), 3, 20);
	MakeCoolFx(io->pos);
	io->gameFlags |= GFLAG_INVISIBILITY;
	
}

void ResetVVPos(Entity * io)
{
	if(io && (io->ioflags & IO_NPC)) {
		io->_npcdata->vvpos = io->pos.y;
	}
}

void ComputeVVPos(Entity * io) {
	if(io->ioflags & IO_NPC) {
		float vvp = io->_npcdata->vvpos;

		if(vvp == -99999.f || vvp == io->pos.y) {
			io->_npcdata->vvpos = io->pos.y;
			return;
		}

		float diff = io->pos.y - vvp;
		float fdiff = glm::abs(diff);
		float eediff = fdiff;

		if(fdiff > 120.f) {
			fdiff = 120.f;
		} else {
			float mul = ((fdiff * (1.0f / 120)) * 0.9f + 0.6f);
			
			float val;
			if(io == entities.player()) {
				val = toMsf(g_platformTime.lastFrameDuration());
			} else {
				val = toMsf(g_gameTime.lastFrameDuration());
			}
			val *= (1.0f / 4) * mul;
			
			if(eediff < 15.f) {
				if(eediff < 10.f) {
					val *= (1.0f / 10);
				} else {
					float ratio = (eediff - 10.f) * (1.0f / 5);
					val = val * ratio + val * (1.f - ratio);
				}
			}
			fdiff -= val;
		}
		
		if(fdiff > eediff) {
			fdiff = eediff;
		}
		
		if(fdiff < 0.f) {
			fdiff = 0.f;
		}
		
		if(diff < 0.f) {
			io->_npcdata->vvpos = io->pos.y + fdiff;
		} else {
			io->_npcdata->vvpos = io->pos.y - fdiff;
		}
	}
}

void ARX_INTERACTIVE_Teleport(Entity * io, const Vec3f & target, bool flag) {
	
	if(!io) {
		return;
	}
	
	arx_assert(isallfinite(target));
	
	io->gameFlags &= ~GFLAG_NOCOMPUTATION;
	io->requestRoomUpdate = true;
	io->room = { };
	
	if(io == entities.player()) {
		g_moveto = player.pos = target + player.baseOffset();
	}
	
	// In case it is being dragged... (except for drag teleport update)
	if(!flag && io == g_draggedEntity) {
		setDraggedEntity(nullptr);
	}
	
	if(io->ioflags & IO_NPC) {
		io->_npcdata->vvpos = io->pos.y;
	}
	
	Vec3f translate = target - io->pos;
	io->lastpos = io->physics.cyl.origin = io->pos = target;
	
	if(io->obj) {
		if(io->obj->pbox) {
			if(io->obj->pbox->active) {
				for(PhysicsParticle & vertex : io->obj->pbox->vert) {
					vertex.pos += translate;
				}
				io->obj->pbox->active = 0;
			}
		}
		for(EERIE_VERTEX & vertex : io->obj->vertexWorldPositions) {
			vertex.v += translate;
		}
	}
	
	ResetVVPos(io);
}

Entity * AddInteractive(const res::path & classPath, EntityInstance instance, AddInteractiveFlags flags) {
	
	std::string_view ficc = classPath.string();
	
	Entity * io = nullptr;
	if(boost::contains(ficc, "items")) {
		io = AddItem(classPath, instance, flags);
	} else if(boost::contains(ficc, "npc")) {
		io = AddNPC(classPath, instance, flags);
	} else if(boost::contains(ficc, "fix")) {
		io = AddFix(classPath, instance, flags);
	} else if(boost::contains(ficc, "camera")) {
		io = AddCamera(classPath, instance);
	} else if(boost::contains(ficc, "marker")) {
		io = AddMarker(classPath, instance);
	}
	
	return io;
}

/*!
 * \brief Links an object designed by path "temp" to the primary attach of interactive object "io"
 */
void SetWeapon_On(Entity * io) {
	
	if(!io || !io->obj || !(io->ioflags & IO_NPC) || !io->_npcdata->weapon || !io->_npcdata->weapon->obj) {
		return;
	}
	
	linkEntities(*io, "primary_attach", *io->_npcdata->weapon, "primary_attach");
	
}

void SetWeapon_Back(Entity * io) {
	
	if(!io || !io->obj || !(io->ioflags & IO_NPC) || !io->_npcdata->weapon || !io->_npcdata->weapon->obj) {
		return;
	}
	
	Entity & weapon = *io->_npcdata->weapon;
	
	if(io->gameFlags & GFLAG_HIDEWEAPON) {
		unlinkEntity(weapon);
	} else if(io->obj->fastaccess.weapon_attach) {
		linkEntities(*io, "weapon_attach", weapon, "primary_attach");
	} else {
		linkEntities(*io, "secondary_attach", weapon, "primary_attach");
	}
	
}

void Prepare_SetWeapon(Entity * io, const res::path & temp) {
	
	arx_assert(io && io->obj && (io->ioflags & IO_NPC));
	
	delete io->_npcdata->weapon;
	arx_assert(!io->_npcdata->weapon);
	
	res::path file = "graph/obj3d/interactive/items/weapons" / temp / temp;
	Entity * ioo = io->_npcdata->weapon = AddItem(file);
	if(ioo) {
		SendInitScriptEvent(ioo);
		io->_npcdata->weapontype = ioo->type_flags;
		ioo->scriptload = 2;
		ioo->setOwner(io);
		SetWeapon_Back(io);
	}
	
}

/*!
 * \brief Creates a Temporary IO Ident
 */
static EntityInstance getFreeEntityInstance(const res::path & classPath) {
	
	std::string_view className = classPath.filename();
	res::path classDir = classPath.parent();
	
	for(EntityInstance instance = 1; ; instance++) {
		
		std::string idString = EntityId(className, instance).string();
		
		// Check if the candidate instance number is used in the current scene
		if(entities.getById(idString)) {
			continue;
		}
		
		// Check if the candidate instance number is used in any visited area
		if(currentSavedGameHasEntity(idString)) {
			continue;
		}
		
		// Check if the candidate instance number is reserved for any scene
		if(g_resources->getDirectory(classDir / idString)) {
			continue;
		}
		
		return instance;
	}
}

Entity * AddFix(const res::path & classPath, EntityInstance instance, AddInteractiveFlags flags) {
	
	res::path object = classPath + ".teo";
	res::path script = classPath + ".asl";
	
	if(!g_resources->getFile(("game" / classPath) + ".ftl")
	   && !g_resources->getFile(object) && !g_resources->getFile(script)) {
		return nullptr;
	}
	
	if(instance == -1) {
		instance = getFreeEntityInstance(classPath);
	}
	arx_assert(instance > 0);
	
	Entity * io = new Entity(classPath, instance);
	
	io->_fixdata = new IO_FIXDATA;
	io->ioflags = IO_FIX;
	io->_fixdata->trapvalue = -1;
	
	loadScript(io->script, g_resources->getFile(script));
	
	if(!(flags & NO_ON_LOAD)) {
		SendIOScriptEvent(nullptr, io, SM_LOAD);
	}
	
	io->spellcast_data.castingspell = SPELL_NONE;
	
	io->pos = player.pos;
	io->pos += angleToVectorXZ(player.angle.getYaw()) * 140.f;
	
	io->lastpos = io->initpos = io->pos;
	io->lastpos.x = io->initpos.x = glm::abs(io->initpos.x / 20) * 20.f;
	io->lastpos.z = io->initpos.z = glm::abs(io->initpos.z / 20) * 20.f;
	
	float tempo;
	EERIEPOLY * ep = CheckInPoly(io->pos + Vec3f(0.f, player.baseHeight(), 0.f));
	if(ep && GetTruePolyY(ep, io->pos, &tempo)) {
		io->lastpos.y = io->initpos.y = io->pos.y = tempo;
	}
	
	ep = CheckInPoly(io->pos);
	if(ep) {
		io->pos.y = std::min(ep->v[0].p.y, ep->v[1].p.y);
		io->lastpos.y = io->initpos.y = io->pos.y = std::min(io->pos.y, ep->v[2].p.y);
	}
	
	if(!io->obj && !(flags & NO_MESH)) {
		io->obj = loadObject(object, false).release();
	}
	
	io->infracolor = Color3f(0.6f, 0.f, 1.f);
	
	TextureContainer * tc = TextureContainer::LoadUI("graph/interface/misc/default[icon]");
	
	if(tc) {
		io->m_inventorySize = inventorySizeFromTextureSize(tc->size());
		io->m_icon = tc;
	}
	
	io->collision = COLLIDE_WITH_PLAYER;
	
	return io;
}

static Entity * AddCamera(const res::path & classPath, EntityInstance instance) {
	
	res::path object = classPath + ".teo";
	res::path script = classPath + ".asl";
	
	if(!g_resources->getFile(("game" / classPath) + ".ftl")
	   && !g_resources->getFile(object) && !g_resources->getFile(script)) {
		return nullptr;
	}
	
	if(instance == -1) {
		instance = getFreeEntityInstance(classPath);
	}
	arx_assert(instance > 0);
	
	Entity * io = new Entity(classPath, instance);
	
	loadScript(io->script, g_resources->getFile(script));
	
	io->pos = player.pos;
	io->pos += angleToVectorXZ(player.angle.getYaw()) * 140.f;
	
	io->lastpos = io->initpos = io->pos;
	io->lastpos.x = io->initpos.x = glm::abs(io->initpos.x / 20) * 20.f;
	io->lastpos.z = io->initpos.z = glm::abs(io->initpos.z / 20) * 20.f;
	
	float tempo;
	EERIEPOLY * ep;
	ep = CheckInPoly(io->pos + Vec3f(0.f, player.baseHeight(), 0.f), &tempo);
	if(ep) {
		io->lastpos.y = io->initpos.y = io->pos.y = tempo;
	}
	
	ep = CheckInPoly(io->pos);
	if(ep) {
		io->pos.y = std::min(ep->v[0].p.y, ep->v[1].p.y);
		io->lastpos.y = io->initpos.y = io->pos.y = std::min(io->pos.y, ep->v[2].p.y);
	}
	
	io->lastpos.y = io->initpos.y = io->pos.y += player.baseHeight();
	
	io->obj = cameraobj.get();
	
	io->_camdata = new IO_CAMDATA();
	io->_camdata->cam = g_playerCamera;
	io->_camdata->cam.focal = 350.f;
	io->ioflags = IO_CAMERA;
	io->collision = 0;
	
	return io;
}

static Entity * AddMarker(const res::path & classPath, EntityInstance instance) {
	
	res::path object = classPath + ".teo";
	res::path script = classPath + ".asl";
	
	if(!g_resources->getFile(("game" / classPath) + ".ftl")
	   && !g_resources->getFile(object) && !g_resources->getFile(script)) {
		return nullptr;
	}
	
	if(instance == -1) {
		instance = getFreeEntityInstance(classPath);
	}
	arx_assert(instance > 0);
	
	Entity * io = new Entity(classPath, instance);
	
	loadScript(io->script, g_resources->getFile(script));
	
	io->pos = player.pos;
	io->pos += angleToVectorXZ(player.angle.getYaw()) * 140.f;
	
	io->lastpos = io->initpos = io->pos;
	io->lastpos.x = io->initpos.x = glm::abs(io->initpos.x / 20) * 20.f;
	io->lastpos.z = io->initpos.z = glm::abs(io->initpos.z / 20) * 20.f;
	
	float tempo;
	EERIEPOLY * ep;
	ep = CheckInPoly(io->pos + Vec3f(0.f, player.baseHeight(), 0.f));
	if(ep && GetTruePolyY(ep, io->pos, &tempo)) {
		io->lastpos.y = io->initpos.y = io->pos.y = tempo;
	}
	
	ep = CheckInPoly(io->pos);
	if(ep) {
		io->pos.y = std::min(ep->v[0].p.y, ep->v[1].p.y);
		io->lastpos.y = io->initpos.y = io->pos.y = std::min(io->pos.y, ep->v[2].p.y);
	}
	
	io->lastpos.y = io->initpos.y = io->pos.y += player.baseHeight();
	
	io->obj = markerobj.get();
	io->ioflags = IO_MARKER;
	io->collision = 0;
	
	return io;
}

IO_NPCDATA::IO_NPCDATA()
	: lifePool(20.f)
	, manaPool(0.f)
	, reachedtime(0)
	, reachedtarget(0)
	, weapon(nullptr)
	, detect(0)
	, movemode(WALKMODE)
	, armor_class(0.f)
	, absorb(0.f)
	, damages(0.f)
	, tohit(0.f)
	, aimtime(0)
	, critical(0.f)
	, reach(0.f)
	, backstab_skill(0.f)
	, behavior(0)
	, behavior_param(0.f)
	, xpvalue(0)
	, cut(0)
	, moveproblem(0.f)
	, weapontype(0)
	, weaponinhand(0)
	, fightdecision(0)
	, look_around_inc(0.f)
	, speakpitch(1.f)
	, lastmouth(0.f)
	, ltemp(0)
	, poisonned(0.f)
	, resist_poison(0)
	, resist_magic(0)
	, resist_fire(0)
	, walk_start_time(0)
	, aiming_start(0)
	, npcflags(0)
	, ex_rotate(0)
	, blood_color(Color::red)
	, SPLAT_DAMAGES(0)
	, SPLAT_TOT_NB(0)
	, last_splat_pos(0.f)
	, vvpos(0.f)
	, climb_count(0.f)
	, stare_factor(0.f)
	, fDetect(0.f)
	, cuts(0)
	, m_magicalDamageTime(0)
{ }

IO_NPCDATA::~IO_NPCDATA() {
	delete ex_rotate;
	delete[] pathfind.list;
}

Entity * AddNPC(const res::path & classPath, EntityInstance instance, AddInteractiveFlags flags) {
	
	res::path object = classPath + ".teo";
	res::path script = classPath + ".asl";
	
	if(!g_resources->getFile(("game" / classPath) + ".ftl")
	   && !g_resources->getFile(object) && !g_resources->getFile(script)) {
		return nullptr;
	}
	
	if(instance == -1) {
		instance = getFreeEntityInstance(classPath);
	}
	arx_assert(instance > 0);
	
	Entity * io = new Entity(classPath, instance);
	
	io->forcedmove = Vec3f(0.f);
	
	io->_npcdata = new IO_NPCDATA;
	io->ioflags = IO_NPC;
	
	loadScript(io->script, g_resources->getFile(script));
	
	io->spellcast_data.castingspell = SPELL_NONE;
	io->_npcdata->manaPool.current = io->_npcdata->manaPool.max = 10.f;
	io->_npcdata->critical = 5.f;
	io->_npcdata->reach = 20.f;
	io->_npcdata->stare_factor = 1.f;
	
	if(!(flags & NO_ON_LOAD)) {
		SendIOScriptEvent(nullptr, io, SM_LOAD);
	}
	
	io->pos = player.pos;
	io->pos += angleToVectorXZ(player.angle.getYaw()) * 140.f;
	
	io->lastpos = io->initpos = io->pos;
	io->lastpos.x = io->initpos.x = glm::abs(io->initpos.x / 20) * 20.f;
	io->lastpos.z = io->initpos.z = glm::abs(io->initpos.z / 20) * 20.f;
	
	float tempo;
	EERIEPOLY * ep = CheckInPoly(io->pos + Vec3f(0.f, player.baseHeight(), 0.f));
	if(ep && GetTruePolyY(ep, io->pos, &tempo)) {
		io->lastpos.y = io->initpos.y = io->pos.y = tempo;
	}
	
	ep = CheckInPoly(io->pos);
	if(ep) {
		io->pos.y = std::min(ep->v[0].p.y, ep->v[1].p.y);
		io->lastpos.y = io->initpos.y = io->pos.y = std::min(io->pos.y, ep->v[2].p.y);
	}
	
	if(!io->obj && !(flags & NO_MESH)) {
		io->obj = loadObject(object, false).release();
	}
	
	io->_npcdata->pathfind.listnb = -1;
	io->_npcdata->behavior = BEHAVIOUR_NONE;
	io->_npcdata->pathfind.truetarget = EntityHandle();
	
	if(!(flags & NO_MESH) && (flags & IO_IMMEDIATELOAD)) {
		EERIE_COLLISION_Cylinder_Create(io);
	}
	
	io->infracolor = Color3f(1.f, 0.f, 0.2f);
	io->collision = COLLIDE_WITH_PLAYER;
	io->m_icon = nullptr;
	
	ARX_INTERACTIVE_HideGore(io);
	return io;
}

Entity * AddItem(const res::path & classPath_, EntityInstance instance, AddInteractiveFlags flags) {
	
	EntityFlags type = IO_ITEM;

	res::path classPath = classPath_;
	
	if(boost::starts_with(classPath.filename(), "gold_coin")) {
		classPath.up() /= "gold_coin";
		type = IO_ITEM | IO_GOLD;
	}

	if(boost::contains(classPath.string(), "movable")) {
		type = IO_ITEM | IO_MOVABLE;
	}
	
	res::path object = classPath + ".teo";
	res::path script = classPath + ".asl";
	res::path icon = classPath + "[icon]";
	
	if(!g_resources->getFile(("game" / classPath) + ".ftl")
	   && !g_resources->getFile(object) && !g_resources->getFile(script)) {
		return nullptr;
	}
	
	if(instance == -1) {
		instance = getFreeEntityInstance(classPath);
	}
	arx_assert(instance > 0);
	
	Entity * io = new Entity(classPath, instance);
	
	io->ioflags = type;
	io->_itemdata = new IO_ITEMDATA();
	io->_itemdata->count = 1;
	io->_itemdata->maxcount = 1;
	io->_itemdata->food_value = 0;
	io->_itemdata->LightValue = -1;

	if(io->ioflags & IO_GOLD) {
		io->_itemdata->price = 1;
	} else {
		io->_itemdata->price = 10;
	}

	io->_itemdata->playerstacksize = 1;

	loadScript(io->script, g_resources->getFile(script));
	
	if(!(flags & NO_ON_LOAD)) {
		SendIOScriptEvent(nullptr, io, SM_LOAD);
	}
	
	io->spellcast_data.castingspell = SPELL_NONE;
	
	io->pos = player.pos;
	io->pos += angleToVectorXZ(player.angle.getYaw()) * 140.f;
	
	io->lastpos.x = io->initpos.x = std::floor(io->pos.x / 20.f) * 20.f;
	io->lastpos.z = io->initpos.z = std::floor(io->pos.z / 20.f) * 20.f;

	EERIEPOLY * ep;
	ep = CheckInPoly(io->pos + Vec3f(0.f, -60.f, 0.f));
	
	if(ep) {
		float tempo;
		if(GetTruePolyY(ep, io->pos, &tempo)) {
			io->lastpos.y = io->initpos.y = io->pos.y = tempo;
		}
	}
	
	ep = CheckInPoly(io->pos);

	if(ep) {
		io->pos.y = std::min(ep->v[0].p.y, ep->v[1].p.y);
		io->lastpos.y = io->initpos.y = io->pos.y = std::min(io->pos.y, ep->v[2].p.y);
	}

	if(io->ioflags & IO_GOLD) {
		io->obj = GoldCoinsObj[0].get();
	}
	
	if(!io->obj && !(flags & NO_MESH)) {
		io->obj = loadObject(object).release();
	}
	
	TextureContainer * tc;
	if(io->ioflags & IO_MOVABLE) {
		tc = cursorMovable;
	} else if(io->ioflags & IO_GOLD) {
		tc = GoldCoinsTC[0];
	} else {
		tc = TextureContainer::LoadUI(icon, TextureContainer::Level);
	}

	if(!tc) {
		tc = TextureContainer::LoadUI("graph/interface/misc/default[icon]");
	}
	
	if(tc) {
		io->m_inventorySize = inventorySizeFromTextureSize(tc->size());
		io->m_icon = tc;
	}

	io->infracolor = Color3f(0.2f, 0.2f, 1.f);
	io->collision = 0;

	return io;
}

/*!
 * \brief Returns nearest interactive object found at position x, y
 */
Entity * GetFirstInterAtPos(const Vec2s & pos) {
	
	float _fdist = 9999999999.f;
	float fdistBB = 9999999999.f;
	float fMaxDist = 350;
	Entity * foundBB = nullptr;
	Entity * foundPixel = nullptr;
	
	if(player.m_telekinesis) {
		fMaxDist = 850;
	}
	
	for(Entity & entity : entities) {
		
		if(entity.ioflags & (IO_CAMERA | IO_MARKER) || !(entity.gameFlags & GFLAG_INTERACTIVITY)) {
			continue;
		}
		
		if(entity == *entities.player()) {
			continue;
		}
		
		bool bPlayerEquiped = IsEquipedByPlayer(&entity);
		if(!((bPlayerEquiped && (player.Interface & INTER_PLAYERBOOK))
		   || (entity.gameFlags & GFLAG_ISINTREATZONE))) {
			continue;
		}
		
		if(!(entity.show == SHOW_FLAG_IN_SCENE
		     || (bPlayerEquiped && (player.Interface & INTER_PLAYERBOOK)
		         && (g_playerBook.currentPage() == BOOKMODE_STATS)))) {
			continue;
		}
		
		if(pos.x < entity.bbox2D.min.x || pos.x > entity.bbox2D.max.x
		   || pos.y < entity.bbox2D.min.y || pos.y > entity.bbox2D.max.y) {
			continue;
		}
		
		float fp = fdist(entity.pos, player.pos);
		if(fp <= fMaxDist && (!foundBB || fp < fdistBB)) {
			fdistBB = fp;
			foundBB = &entity;
		}
		
		if((entity.ioflags & (IO_CAMERA | IO_MARKER | IO_GOLD)) || bPlayerEquiped) {
			
			fp = bPlayerEquiped ? 0.f : fdist(entity.pos, player.pos);
			if(fp < fdistBB || !foundBB) {
				fdistBB = fp;
				foundBB = &entity;
				foundPixel = &entity;
			}
			
		} else {
			
			for(EERIE_FACE & face : entity.obj->facelist) {
				float n = PtIn2DPolyProj(entity.obj->vertexClipPositions, face , pos.x, pos.y);
				if(n > 0.f) {
					fp = bPlayerEquiped ? 0.f : fdist(entity.pos, player.pos);
					if(fp <= fMaxDist && (fp < _fdist || !foundPixel)) {
						_fdist = fp;
						foundPixel = &entity;
						break;
					}
				}
			}
			
		}
		
	}
	
	return foundPixel ? foundPixel : foundBB;
}

bool IsEquipedByPlayer(const Entity * io) {
	
	if(io && (io->ioflags & IO_ICONIC) && (io->show == SHOW_FLAG_ON_PLAYER)) {
		return true;
	}
	
	return isEquippedByPlayer(io);
}

extern long LOOKING_FOR_SPELL_TARGET;
Entity * InterClick(const Vec2s & pos) {
	
	float dist_Threshold;
	
	if(LOOKING_FOR_SPELL_TARGET) {
		dist_Threshold = 550.f;
	} else {
		dist_Threshold = 360.f;
	}
	
	if(Entity * io = GetFirstInterAtPos(pos)) {
		if(io->ioflags & IO_NPC) {
			if(closerThan(player.pos, io->pos, dist_Threshold)) {
				return io;
			}
		} else if(player.m_telekinesis) {
			return io;
		} else if(IsEquipedByPlayer(io) || closerThan(player.pos, io->pos, dist_Threshold)) {
			return io;
		}
	}
	
	return nullptr;
}

// Need To upgrade to a more precise collision.
Entity * getCollidingEntityAt(const Vec3f & pos, const Vec3f & size) {
	
	for(Entity & entity : entities.inScene(IO_NPC | IO_FIX)) {
		
		if((entity.ioflags & IO_NO_COLLISIONS)
		   || !entity.collision
		   || !(entity.gameFlags & GFLAG_ISINTREATZONE)
		   || ((entity.ioflags & IO_NPC) && entity._npcdata->lifePool.current <= 0.f)) {
			continue;
		}
		
		if(IsCollidingInter(&entity, pos)) {
			return &entity;
		}
		
		if(IsCollidingInter(&entity, pos + Vec3f(0.f, size.y, 0.f))) {
			return &entity;
		}
		
	}
	
	return nullptr;
}

static bool IsCollidingInter(Entity * io, const Vec3f & pos) {
	
	if(!io || !io->obj || !closerThan(pos, io->pos, 190.f)) {
		return false;
	}
	
	if(io->obj->grouplist.size() > 4) {
		for(const VertexGroup & group : io->obj->grouplist) {
			if(!fartherThan(pos, io->obj->vertexWorldPositions[group.origin].v, 50.f)) {
				return true;
			}
		}
	} else {
		const EERIE_VERTEX & origin = io->obj->vertexWorldPositions[io->obj->origin];
		for(const EERIE_VERTEX & vertex : io->obj->vertexWorldPositions) {
			if(&vertex != &origin && !fartherThan(pos, vertex.v, 30.f)) {
				return true;
			}
		}
	}
	
	return false;
}

void SetYlsideDeath(Entity * io) {
	io->sfx_flag = SFX_TYPE_YLSIDE_DEATH;
	io->sfx_time = g_gameTime.now();
}

void UpdateCameras() {
	
	ARX_PROFILE_FUNC();
	
	for(Entity & entity : entities) {
		
		if(entity.usepath) {
			
			ARX_USE_PATH * aup = entity.usepath;
			GameDuration elapsed = g_gameTime.now() - aup->_curtime;
			
			if(aup->aupflags & ARX_USEPATH_FORWARD) {
				if(aup->aupflags & ARX_USEPATH_FLAG_FINISHED) {
				} else {
					aup->_curtime += elapsed;
				}
			}
			
			if(aup->aupflags & ARX_USEPATH_BACKWARD) {
				aup->_starttime += elapsed * 2;
				aup->_curtime += elapsed;
				if(aup->_starttime >= aup->_curtime) {
					aup->_curtime = aup->_starttime + 1ms;
				}
			}
			
			if(aup->aupflags & ARX_USEPATH_PAUSE) {
				aup->_starttime += elapsed;
				aup->_curtime += elapsed;
			}
			
			long last = ARX_PATHS_Interpolate(aup, &entity.pos);
			if(aup->lastWP != last) {
				if(last == -2) {
					std::string waypoint = std::to_string(aup->path->pathways.size() - 1);
					SendIOScriptEvent(nullptr, &entity, SM_WAYPOINT, waypoint);
					SendIOScriptEvent(nullptr, &entity, ScriptEventName("waypoint" + waypoint));
					SendIOScriptEvent(nullptr, &entity, SM_PATHEND);
				} else {
					long ii = aup->lastWP + 1;
					if(ii < 0 || ii > last) {
						ii = 0;
					}
					std::string waypoint = std::to_string(ii);
					SendIOScriptEvent(nullptr, &entity, SM_WAYPOINT, waypoint);
					SendIOScriptEvent(nullptr, &entity, ScriptEventName("waypoint" + waypoint));
					if(size_t(ii) == aup->path->pathways.size()) {
						SendIOScriptEvent(nullptr, &entity, SM_PATHEND);
					}
				}
				aup->lastWP = last;
			}
			
			if(entity.damager_damages > 0 && entity.show == SHOW_FLAG_IN_SCENE) {
				for(Entity & other : entities.inScene(IO_NPC)) {
					
					if(other == entity || !closerThan(entity.pos, other.pos, 600.f)) {
						continue;
					}
					
					bool touched = false;
					for(const EERIE_VERTEX & i : entity.obj->vertexWorldPositions | boost::adaptors::strided(3)) {
						for(const EERIE_VERTEX & j : other.obj->vertexWorldPositions | boost::adaptors::strided(3)) {
							if(closerThan(i.v, j.v, 20.f)) {
								touched = true;
								break;
							}
						}
						if(touched) {
							break;
						}
					}
					
					if(touched) {
						damageCharacter(other, entity.damager_damages, entity, nullptr, entity.damager_type, &other.pos);
					}
					
				}
			}
			
		}
		
		if(entity.ioflags & IO_CAMERA) {
			
			arx_assert(isallfinite(entity.pos));
			entity._camdata->cam.m_pos = entity.pos;
			
			if(entity.targetinfo != EntityHandle(TARGET_NONE)) {
				
				// Follows target
				GetTargetPos(&entity, static_cast<unsigned long>(entity._camdata->smoothing));
				entity.target += entity._camdata->translatetarget;
				
				Vec3f target = entity.target;
				if(entity._camdata->lastinfovalid && entity._camdata->smoothing != 0.f) {
					float vv = (8000.f - std::min(entity._camdata->smoothing, 8000.f)) * (1.0f / 4000.f);
					float f1 = std::min(g_gameTime.lastFrameDuration() / 1s * vv, 1.f);
					target = entity.target * (1.f - f1) + entity._camdata->lasttarget * f1;
				}
				
				entity._camdata->cam.lookAt(target);
				entity._camdata->lasttarget = target;
				entity.angle.setPitch(0.f);
				entity.angle.setYaw(entity._camdata->cam.angle.getYaw() + 90.f);
				entity.angle.setRoll(0.f);
				
			} else {
				
				// no target...
				entity.target = entity.pos;
				entity.target += angleToVectorXZ(entity.angle.getYaw() + 90) * 20.f;
				entity._camdata->cam.lookAt(entity.target);
				entity._camdata->cam.angle.setPitch(MAKEANGLE(-entity._camdata->cam.angle.getPitch()));
				entity._camdata->cam.angle.setYaw(MAKEANGLE(entity._camdata->cam.angle.getYaw() + 180.f));
				entity._camdata->lasttarget = entity.target;
				
			}
			
			entity._camdata->lastinfovalid = true;
			
		}
		
	}
	
}

void UpdateIOInvisibility(Entity * io)
{
	if(io && io->invisibility <= 1.f) {
		if((io->gameFlags & GFLAG_INVISIBILITY) && io->invisibility < 1.f) {
			io->invisibility += g_gameTime.lastFrameDuration() / 1s;
		} else if(!(io->gameFlags & GFLAG_INVISIBILITY) && io->invisibility != 0.f) {
			io->invisibility -= g_gameTime.lastFrameDuration() / 1s;
		}
		
		io->invisibility = glm::clamp(io->invisibility, 0.f, 1.f);
	}
}

void UpdateInter() {
	
	for(Entity & entity : entities.inScene()) {
		
		if(&entity == g_draggedEntity
		   || !(entity.gameFlags & GFLAG_ISINTREATZONE)
		   || (entity.ioflags & (IO_CAMERA | IO_MARKER))
		   || entity == *entities.player()) {
			continue;
		}
		
		UpdateIOInvisibility(&entity);
		
		Anglef temp = entity.angle;
		if(entity.ioflags & IO_NPC) {
			temp.setYaw(MAKEANGLE(180.f - temp.getYaw()));
		} else {
			temp.setYaw(MAKEANGLE(270.f - temp.getYaw()));
		}
		
		if(entity.animlayer[0].cur_anim) {
			
			entity.bbox2D.min.x = 9999;
			entity.bbox2D.max.x = -1;
			
			AnimationDuration diff;
			if(entity.animlayer[0].flags & EA_PAUSED) {
				diff = 0;
			} else {
				diff = toAnimationDuration(g_gameTime.lastFrameDuration());
			}
			
			Vec3f pos = entity.pos;
			if(entity.ioflags & IO_NPC) {
				ComputeVVPos(&entity);
				pos.y = entity._npcdata->vvpos;
			}
			
			EERIEDrawAnimQuatUpdate(entity.obj, entity.animlayer.data(), temp, pos, diff, &entity, true);
			
		}
		
	}
	
}

void RenderInter() {
	
	ARX_PROFILE_FUNC();
	
	UseTextureState textureState(TextureStage::FilterLinear, TextureStage::WrapClamp);
	
	for(Entity & entity : entities.inScene()) {
		
		if(!(entity.gameFlags & GFLAG_ISINTREATZONE)
		   || (entity.ioflags & (IO_CAMERA | IO_MARKER))
		   || entity == *entities.player()) {
			continue;
		}
		
		float invisibility = Cedric_GetInvisibility(&entity);
		
		if(entity.animlayer[0].cur_anim) {
			
			Vec3f pos = entity.pos;
			if(entity.ioflags & IO_NPC) {
				pos.y = entity._npcdata->vvpos;
			}
			
			EERIEDrawAnimQuatRender(entity.obj, pos, &entity, invisibility);
			
		} else {
			
			entity.bbox2D.min.x = 9999;
			entity.bbox2D.max.x = -1;
			
			if(entity.obj) {
				UpdateGoldObject(&entity);
			}
			
			if(!(entity.ioflags & IO_NPC) && entity.obj) {
				Anglef angle = entity.angle;
				angle.setYaw(MAKEANGLE(270.f - angle.getYaw()));
				glm::quat rotation = toQuaternion(angle);
				TransformInfo t(entity.pos, rotation, entity.scale);
				DrawEERIEInter(entity.obj, t, &entity, false, invisibility);
			}
			
		}
		
	}
	
}

static std::vector<Entity *> toDestroy;

bool ARX_INTERACTIVE_DestroyIOdelayed(Entity * entity) {
	
	if((entity->ioflags & IO_ITEM) && entity->_itemdata->count > 1) {
		entity->_itemdata->count--;
		return false;
	}
	
	LogDebug("will destroy entity " << entity->idString());
	if(std::find(toDestroy.begin(), toDestroy.end(), entity) == toDestroy.end()) {
		toDestroy.push_back(entity);
	}
	
	return true;
}

void ARX_INTERACTIVE_DestroyIOdelayedRemove(Entity * entity) {
	Entity * null = nullptr;
	// Remove the entity from the list but don't invalidate iterators
	std::replace(toDestroy.begin(), toDestroy.end(), entity, null);
}

void ARX_INTERACTIVE_DestroyIOdelayedExecute() {
	
	if(!toDestroy.empty()) {
		LogDebug("executing delayed entity destruction");
	}
	
	for(Entity * entity : toDestroy) {
		if(entity) {
			entity->destroy();
		}
	}
	
	toDestroy.clear();
	
}

bool IsSameObject(Entity * io, Entity * ioo) {
	
	if(!io || !ioo || io->classPath() != ioo->classPath() || (io->ioflags & IO_UNIQUE)
	   || io->durability != ioo->durability || io->max_durability != ioo->max_durability) {
		return false;
	}
	
	if((io->ioflags & IO_ITEM) && (ioo->ioflags & IO_ITEM)
	   && !io->over_script.valid && !ioo->over_script.valid) {
		if(io->locname == ioo->locname) {
			return true;
		}
	}
	
	return false;
}

template <typename Set>
static bool intersect(const Set & set1, const Set & set2) {
	
	if(set1.empty() || set2.empty()) {
		return false;
	}
	
	auto it1 = set1.begin();
	auto it2 = set2.begin();
	
	if(*it1 > *set2.rbegin() || *it2 > *set1.rbegin()) {
		return false;
	}
	
	while(it1 != set1.end() && it2 != set2.end()) {
		if(*it1 == *it2) {
			return true;
		} else if(*it1 < *it2) {
			++it1;
		} else {
			++it2;
		}
	}
	
	return false;
}

bool HaveCommonGroup(Entity * io, Entity * ioo) {
	return io && ioo && intersect(io->groups, ioo->groups);
}

float ARX_INTERACTIVE_GetArmorClass(Entity * io) {
	
	arx_assert(io && (io->ioflags & IO_NPC));
	
	float ac = io->_npcdata->armor_class;
	
	ac += spells.getTotalSpellCasterLevelOnTarget(io->index(), SPELL_ARMOR);
	ac -= spells.getTotalSpellCasterLevelOnTarget(io->index(), SPELL_LOWER_ARMOR);
	
	return std::max(ac, 0.f);
}

void ARX_INTERACTIVE_ActivatePhysics(Entity & entity) {
	
	if(&entity == g_draggedEntity || entity.show != SHOW_FLAG_IN_SCENE || !entity.obj || !entity.obj->pbox) {
		return;
	}
	
	arx_assert(!locateInInventories(&entity));
	
	float yy;
	EERIEPOLY * ep = CheckInPoly(entity.pos, &yy);
	if(ep && (yy - entity.pos.y < 10.f)) {
		return;
	}
	
	entity.obj->pbox->active = 1;
	entity.obj->pbox->stopcount = 0;
	Vec3f fallvector = Vec3f(0.0f, 0.000001f, 0.f);
	entity.show = SHOW_FLAG_IN_SCENE;
	entity.soundtime = 0;
	entity.soundcount = 0;
	EERIE_PHYSICS_BOX_Launch(entity.obj, entity.pos, entity.angle, fallvector);
	
}

std::string_view GetMaterialString(const res::path & texture) {
	
	std::string_view origin = texture.string();
	
	// need to be precomputed !!!
	if(boost::contains(origin, "stone")) return "stone";
	else if(boost::contains(origin, "marble")) return "stone";
	else if(boost::contains(origin, "rock")) return "stone";
	else if(boost::contains(origin, "wood")) return "wood";
	else if(boost::contains(origin, "wet")) return "wet";
	else if(boost::contains(origin, "mud")) return "wet";
	else if(boost::contains(origin, "blood")) return "wet";
	else if(boost::contains(origin, "bone")) return "wet";
	else if(boost::contains(origin, "flesh")) return "wet";
	else if(boost::contains(origin, "shit")) return "wet";
	else if(boost::contains(origin, "soil")) return "gravel";
	else if(boost::contains(origin, "gravel")) return "gravel";
	else if(boost::contains(origin, "earth")) return "gravel";
	else if(boost::contains(origin, "dust")) return "gravel";
	else if(boost::contains(origin, "sand")) return "gravel";
	else if(boost::contains(origin, "straw")) return "gravel";
	else if(boost::contains(origin, "metal")) return "metal";
	else if(boost::contains(origin, "iron")) return "metal";
	else if(boost::contains(origin, "glass")) return "metal";
	else if(boost::contains(origin, "rust")) return "metal";
	else if(boost::contains(origin, "ice")) return "ice";
	else if(boost::contains(origin, "fabric")) return "carpet";
	else if(boost::contains(origin, "moss")) return "carpet";
	else return "unknown";
}

void UpdateGoldObject(Entity * io) {
	
	if(io->ioflags & IO_GOLD) {
		
		long num = 0;
		if(io->_itemdata->price <= 3) {
			num = io->_itemdata->price - 1;
		} else if(io->_itemdata->price <= 8) {
			num = 3;
		} else if(io->_itemdata->price <= 20) {
			num = 4;
		} else if(io->_itemdata->price <= 50) {
			num = 5;
		} else {
			num = 6;
		}
		
		io->obj = GoldCoinsObj[num].get();
		io->m_icon = GoldCoinsTC[num];
		
	}
	
}
