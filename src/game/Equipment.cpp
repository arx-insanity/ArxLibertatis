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
// Copyright (c) 1999-2001 ARKANE Studios SA. All rights reserved

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>

#include "animation/Animation.h"

#include "core/Core.h"

#include "game/Damage.h"
#include "game/EntityManager.h"
#include "game/Equipment.h"
#include "game/Inventory.h"
#include "game/Item.h"
#include "game/NPC.h"
#include "game/Player.h"
#include "game/Spells.h"

#include "gui/Dragging.h"
#include "gui/Interface.h"

#include "graphics/BaseGraphicsTypes.h"
#include "graphics/Color.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Math.h"
#include "graphics/Vertex.h"
#include "graphics/data/Mesh.h"
#include "graphics/data/MeshManipulation.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/effects/Decal.h"
#include "graphics/particle/ParticleEffects.h"
#include "graphics/particle/Spark.h"

#include "io/resource/ResourcePath.h"

#include "math/Random.h"
#include "math/Vector.h"

#include "physics/Collisions.h"

#include "platform/Platform.h"
#include "platform/profiler/Profiler.h"

#include "scene/Object.h"
#include "scene/LinkedObject.h"
#include "scene/GameSound.h"
#include "scene/Interactive.h"

#include "script/Script.h"

#include "util/Number.h"
#include "util/String.h"


struct EQUIP_INFO {
	const char * name;
};

#define SP_SPARKING 1
#define SP_BLOODY 2

extern Vec3f PUSH_PLAYER_FORCE;
extern bool EXTERNALVIEW;

EQUIP_INFO equipinfo[IO_EQUIPITEM_ELEMENT_Number];

//! \brief Returns the object type flag corresponding to a string
ItemType ARX_EQUIPMENT_GetObjectTypeFlag(std::string_view temp) {
	
	if(temp.empty()) {
		return 0;
	}
	
	char c = temp[0];
	
	arx_assert(util::toLowercase(c) == c);
	
	switch(c) {
		case 'w': return OBJECT_TYPE_WEAPON;
		case 'd': return OBJECT_TYPE_DAGGER;
		case '1': return OBJECT_TYPE_1H;
		case '2': return OBJECT_TYPE_2H;
		case 'b': return OBJECT_TYPE_BOW;
		case 's': return OBJECT_TYPE_SHIELD;
		case 'r': return OBJECT_TYPE_RING;
		case 'a': return OBJECT_TYPE_ARMOR;
		case 'h': return OBJECT_TYPE_HELMET;
		case 'l': return OBJECT_TYPE_LEGGINGS;
		default:  return 0;
	}
	
}

//! \brief Releases Equipment Structure
void ARX_EQUIPMENT_ReleaseAll(Entity * io) {
	
	if(!io || !(io->ioflags & IO_ITEM)) {
		return;
	}
	
	delete io->_itemdata->equipitem;
	io->_itemdata->equipitem = nullptr;
	
}

extern long EXITING;

//! \brief Recreates player mesh from scratch
static void applyTweak(EquipmentSlot equip, TweakType tw, std::string_view selection) {
	
	Entity * item = entities.get(player.equiped[equip]);
	if(!item) {
		return;
	}
	
	Entity * io = entities.player();
	
	arx_assert(item->tweakerinfo != nullptr);
	
	const IO_TWEAKER_INFO & tweak = *item->tweakerinfo;
	
	if(!tweak.filename.empty()) {
		res::path mesh = "graph/obj3d/interactive/npc/human_base/tweaks" / tweak.filename;
		EERIE_MESH_TWEAK_Do(io, tw, mesh);
	}
	
	if(tweak.skintochange.empty() || tweak.skinchangeto.empty()) {
		return;
	}
	
	res::path file = "graph/obj3d/textures" / tweak.skinchangeto;
	MaterialId newMaterial = addMaterial(*io->obj, TextureContainer::Load(file, TextureContainer::Level));
	
	VertexSelectionId sel;
	for(VertexSelectionId i : io->obj->selections.handles()) {
		if(io->obj->selections[i].name == selection) {
			sel = i;
			break;
		}
	}
	if(!sel) {
		return;
	}
	
	MaterialId oldMaterial;
	for(MaterialId material : io->obj->materials.handles()) {
		if(io->obj->materials[material] &&
		   io->obj->materials[material]->m_texName.filename() == tweak.skintochange) {
			oldMaterial = material;
		}
	}
	if(!oldMaterial) {
		return;
	}
	
	for(EERIE_FACE & face : io->obj->facelist) {
		if(IsInSelection(io->obj, face.vid[0], sel) &&
		   IsInSelection(io->obj, face.vid[1], sel) &&
		   IsInSelection(io->obj, face.vid[2], sel)) {
			if(face.material == oldMaterial) {
				face.material = newMaterial;
			}
		}
	}
	
}

void ARX_EQUIPMENT_RecreatePlayerMesh() {
	
	if(EXITING) {
		return;
	}
	
	arx_assert(entities.player());
	Entity * io = entities.player();
	
	if(io->obj != hero) {
		delete io->obj;
	}
	
	io->obj = loadObject("graph/obj3d/interactive/npc/human_base/human_base.teo", false).release();
	
	applyTweak(EQUIP_SLOT_HELMET, TWEAK_HEAD, "head");
	applyTweak(EQUIP_SLOT_ARMOR, TWEAK_TORSO, "chest");
	applyTweak(EQUIP_SLOT_LEGGINGS, TWEAK_LEGS, "leggings");
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment); toequip && toequip->obj) {
			if(toequip->type_flags & (OBJECT_TYPE_DAGGER | OBJECT_TYPE_1H | OBJECT_TYPE_2H | OBJECT_TYPE_BOW)) {
				if(player.Interface & INTER_COMBATMODE) {
					ARX_EQUIPMENT_AttachPlayerWeaponToHand();
				} else {
					linkEntities(*entities.player(), "weapon_attach", *toequip, "primary_attach");
				}
			} else if((toequip->type_flags & OBJECT_TYPE_SHIELD) && entities.get(player.equiped[EQUIP_SLOT_SHIELD])) {
				linkEntities(*entities.player(), "shield_attach", *toequip, "shield_attach");
			}
		}
	}
	
	ARX_PLAYER_Restore_Skin();
	HERO_SHOW_1ST = -1;
	ARX_INTERACTIVE_Show_Hide_1st(entities.player(), !EXTERNALVIEW);
	ARX_INTERACTIVE_HideGore(entities.player(), false);
	
	EERIE_Object_Precompute_Fast_Access(hero);
	EERIE_Object_Precompute_Fast_Access(entities.player()->obj);
	
	ARX_INTERACTIVE_RemoveGoreOnIO(entities.player());
	
}

void ARX_EQUIPMENT_UnEquipAllPlayer() {
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * item = entities.get(equipment)) {
			ARX_EQUIPMENT_UnEquip(entities.player(), item);
		}
	}
	
	ARX_PLAYER_ComputePlayerFullStats();
}

bool isEquippedByPlayer(const Entity * item) {
	
	arx_assert(entities.player());
	
	if(!item) {
		return false;
	}
	
	return std::find(player.equiped.begin(), player.equiped.end(), item->index()) != player.equiped.end();
}

// flags & 1 == destroyed !
void ARX_EQUIPMENT_UnEquip(Entity * target, Entity * tounequip, long flags) {
	
	if(target != entities.player() || !tounequip) {
		return;
	}
	
	for(EntityHandle & equipment : player.equiped) {
		if(equipment == tounequip->index()) {
			unlinkEntity(*tounequip);
			equipment = EntityHandle();
			arx_assert(!isEquippedByPlayer(tounequip));
			target->bbox2D.min.x = 9999;
			target->bbox2D.max.x = -9999;
			if(!flags) {
				if(!g_draggedEntity) {
					ARX_SOUND_PlayInterface(g_snd.INVSTD);
					setDraggedEntity(tounequip);
				} else {
					giveToPlayer(tounequip);
				}
			}
			SendIOScriptEvent(tounequip, entities.player(), SM_EQUIPOUT);
			SendIOScriptEvent(entities.player(), tounequip, SM_EQUIPOUT);
		}
	}
	
	if(tounequip->type_flags & (OBJECT_TYPE_HELMET | OBJECT_TYPE_ARMOR | OBJECT_TYPE_LEGGINGS)) {
		ARX_EQUIPMENT_RecreatePlayerMesh();
	}
	
}

void ARX_EQUIPMENT_AttachPlayerWeaponToHand() {
	
	arx_assert(entities.player());
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment); toequip && toequip->obj) {
			if(toequip->type_flags & (OBJECT_TYPE_DAGGER | OBJECT_TYPE_1H | OBJECT_TYPE_2H | OBJECT_TYPE_BOW)) {
				linkEntities(*entities.player(), "primary_attach", *toequip, "primary_attach");
				return;
			}
		}
	}
	
}

void ARX_EQUIPMENT_AttachPlayerWeaponToBack() {
	
	arx_assert(entities.player());
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment)) {
			if((toequip->type_flags & OBJECT_TYPE_DAGGER) || (toequip->type_flags & OBJECT_TYPE_1H)
			   || (toequip->type_flags & OBJECT_TYPE_2H) || (toequip->type_flags & OBJECT_TYPE_BOW)) {
				std::string_view attach = (toequip->type_flags & OBJECT_TYPE_BOW) ? "test" : "primary_attach";
				linkEntities(*entities.player(), "weapon_attach", *toequip, attach);
				return;
			}
		}
	}
	
}

WeaponType ARX_EQUIPMENT_GetPlayerWeaponType() {
	
	if(Entity * toequip = entities.get(player.equiped[EQUIP_SLOT_WEAPON])) {
		if(toequip->type_flags & OBJECT_TYPE_DAGGER) {
			return WEAPON_DAGGER;
		}
		if(toequip->type_flags & OBJECT_TYPE_1H) {
			return WEAPON_1H;
		}
		if(toequip->type_flags & OBJECT_TYPE_2H) {
			return WEAPON_2H;
		}
		if(toequip->type_flags & OBJECT_TYPE_BOW) {
			return WEAPON_BOW;
		}
	}
	
	return WEAPON_BARE;
}

void ARX_EQUIPMENT_LaunchPlayerUnReadyWeapon() {
	arx_assert(entities.player());
	arx_assert(arrowobj);
	
	Entity * io = entities.player();
	
	ANIM_HANDLE * anim;
	WeaponType type = ARX_EQUIPMENT_GetPlayerWeaponType();
	
	switch(type) {
		case WEAPON_DAGGER:
			anim = io->anims[ANIM_DAGGER_UNREADY_PART_1];
			break;
		case WEAPON_1H:
			anim = io->anims[ANIM_1H_UNREADY_PART_1];
			break;
		case WEAPON_2H:
			anim = io->anims[ANIM_2H_UNREADY_PART_1];
			break;
		case WEAPON_BOW:
			anim = io->anims[ANIM_MISSILE_UNREADY_PART_1];
			EERIE_LINKEDOBJ_UnLinkObjectFromObject(io->obj, arrowobj.get());
			break;
		default:
			anim = io->anims[ANIM_BARE_UNREADY];
			break;
	}
	
	changeAnimation(io, 1, anim);
}

Entity * getWeapon(Entity & entity) noexcept {
	if(&entity == entities.player()) {
		return entities.get(player.equiped[EQUIP_SLOT_WEAPON]);
	} else if(entity.ioflags & IO_NPC) {
		return entity._npcdata->weapon;
	}
	return nullptr;
}

std::string_view getWeaponMaterial(Entity & entity) noexcept {
	if(Entity * weapon = getWeapon(entity); weapon && !weapon->weaponmaterial.empty()) {
		return weapon->weaponmaterial;
	}
	if((entity.ioflags & IO_NPC) &&!entity.weaponmaterial.empty()) {
		return entity.weaponmaterial;
	}
	return "bare";
}

DamageType getDamageTypeFromWeaponMaterial(std::string_view material) noexcept {
	if(material == "claw") {
		return DAMAGE_TYPE_ORGANIC;
	} else if(material == "axe" || material == "sword" || material == "dagger") {
		return DAMAGE_TYPE_METAL;
	} else if(material == "club") {
		return DAMAGE_TYPE_WOOD;
	} else {
		return DAMAGE_TYPE_GENERIC;
	}
}

float ARX_EQUIPMENT_ComputeDamages(Entity * io_source, Entity * io_target, float ratioaim, Vec3f * position) {
	
	SendIOScriptEvent(io_source, io_target, SM_AGGRESSION);
	
	if(!io_source || !io_target) {
		return 0.f;
	}
	
	std::string_view wmat = getWeaponMaterial(*io_source);
	DamageType type = getDamageTypeFromWeaponMaterial(wmat);
	
	if(!(io_target->ioflags & IO_NPC)) {
		if(io_target->ioflags & IO_FIX) {
			float damages = 1.f;
			if(io_source == entities.player()) {
				damages = player.m_miscFull.damages;
			} else if(io_source->ioflags & IO_NPC) {
				damages = io_source->_npcdata->damages;
			}
			damageProp(*io_target, damages, io_source, nullptr, type);
		}
		return 0.f;
	}
	
	float attack, ac, damages;
	float backstab = 1.f;
	
	std::string_view amat = "flesh";
	
	bool critical = false;
	
	if(io_source == entities.player()) {
		
		attack = player.m_miscFull.damages;
		
		if(Random::getf(0.f, 100.f) <= player.m_miscFull.criticalHit
		   && SendIOScriptEvent(io_source, io_source, SM_CRITICAL) != REFUSE) {
			critical = true;
		}
		
		damages = attack * ratioaim;
		
		if(io_target->_npcdata->npcflags & NPCFLAG_BACKSTAB) {
			if(Random::getf(0.f, 100.f) <= player.m_skillFull.stealth * 0.5f) {
				if(SendIOScriptEvent(io_source, io_source, SM_BACKSTAB) != REFUSE) {
					backstab = 1.5f;
				}
			}
		}
		
	} else {
		
		if(!(io_source->ioflags & IO_NPC)) {
			// no NPC source...
			return 0.f;
		}
		
		attack = io_source->_npcdata->tohit;
		
		damages = io_source->_npcdata->damages * ratioaim * Random::getf(0.5f, 1.0f);
		
		Spell * spell = spells.getSpellOnTarget(io_source->index(), SPELL_CURSE);
		if(spell) {
			damages *= (1 - spell->m_level * 0.05f);
		}
		
		if(Random::getf(0.f, 100) <= io_source->_npcdata->critical
		   && SendIOScriptEvent(io_source, io_source, SM_CRITICAL) != REFUSE) {
			critical = true;
		}
		
		if(Random::getf(0.f, 100.f) <= io_source->_npcdata->backstab_skill) {
			if(SendIOScriptEvent(io_source, io_source, SM_BACKSTAB) != REFUSE) {
				backstab = 1.5f;
			}
		}
		
	}
	
	float absorb;
	
	if(io_target == entities.player()) {
		ac = player.m_miscFull.armorClass;
		absorb = player.m_skillFull.defense * 0.5f;
	} else {
		ac = ARX_INTERACTIVE_GetArmorClass(io_target);
		absorb = io_target->_npcdata->absorb;
		
		Spell * spell = spells.getSpellOnTarget(io_target->index(), SPELL_CURSE);
		if(spell) {
			float modif = (1 - spell->m_level * 0.05f);
			ac *= modif;
			absorb *= modif;
		}
	}
	
	if(!io_target->armormaterial.empty()) {
		amat = io_target->armormaterial;
	}
	
	if(io_target == entities.player()) {
		if(Entity * armor = entities.get(player.equiped[EQUIP_SLOT_ARMOR])) {
			if(!armor->armormaterial.empty()) {
				amat = armor->armormaterial;
			}
		}
	}
	
	float dmgs = damages * backstab;
	dmgs -= dmgs * absorb * 0.01f;
	
	Vec3f pos = io_target->pos;
	float power = std::min(1.f, dmgs * 0.05f) * 0.1f + 0.9f;
	
	ARX_SOUND_PlayCollision(amat, wmat, power, 1.f, pos, io_source);
	
	float chance = 100.f - (ac - attack);
	if(Random::getf(0.f, 100.f) > chance) {
		return 0.f;
	}
	
	ARX_SOUND_PlayCollision("flesh", wmat, power, 1.f, pos, io_source);
	
	if(dmgs > 0.f) {
		
		if(critical) {
			dmgs *= 1.5f;
		}
		
		if(io_target == entities.player()) {
			
			// TODO should this be player.pos - player.baseOffset() = player.basePosition()?
			Vec3f ppos = io_source->pos - (player.pos + player.baseOffset());
			ppos = glm::normalize(ppos);
			
			// Push the player
			PUSH_PLAYER_FORCE += ppos * -dmgs * Vec3f(1.0f / 11, 1.0f / 30, 1.0f / 11);
			
			damagePlayer(dmgs, 0, io_source);
			ARX_DAMAGES_DamagePlayerEquipment(dmgs);
			
		} else {
			
			Vec3f ppos = io_source->pos - io_target->pos;
			ppos = glm::normalize(ppos);
			
			// Push the NPC
			io_target->forcedmove += ppos * -dmgs;
			
			Vec3f * targetPosition = position ? position : &io_target->pos;
			damageNpc(*io_target, dmgs, io_source, nullptr, type, targetPosition);
		}
	}
	
	return dmgs;
}

static float ARX_EQUIPMENT_GetSpecialValue(Entity * io, long val) {
	
	if(!io || !(io->ioflags & IO_ITEM) || !io->_itemdata->equipitem) {
		return -1;
	}
	
	for(long i = IO_EQUIPITEM_ELEMENT_SPECIAL_1; i <= IO_EQUIPITEM_ELEMENT_SPECIAL_4; i++) {
		if(io->_itemdata->equipitem->elements[i].special == val) {
			return io->_itemdata->equipitem->elements[i].value;
		}
	}
	
	return -1;
}

// flags & 1 = blood spawn only
bool ARX_EQUIPMENT_Strike_Check(Entity * io_source, Entity * io_weapon, float ratioaim, long flags, EntityHandle targ) {
	
	ARX_PROFILE_FUNC();
	
	arx_assert(io_source);
	arx_assert(io_weapon);
	
	if(!io_weapon->obj) {
		return false;
	}
	
	bool ret = false;
	
	EXCEPTIONS_LIST_Pos = 0;
	
	float drain_life = ARX_EQUIPMENT_GetSpecialValue(io_weapon, IO_SPECIAL_ELEM_DRAIN_LIFE);
	float paralyse = ARX_EQUIPMENT_GetSpecialValue(io_weapon, IO_SPECIAL_ELEM_PARALYZE);
	
	for(const EERIE_ACTIONLIST & action : io_weapon->obj->actionlist) {
		
		float rad = GetHitValue(action.name);
		if(rad == -1.f) {
			continue;
		}
		
		Sphere sphere;
		sphere.origin = io_weapon->obj->vertexWorldPositions[action.idx].v;
		sphere.radius = rad;
		
		if(io_source != entities.player()) {
			sphere.radius += 15.f;
		}
		
		std::vector<Entity *> sphereContent;
		if(CheckEverythingInSphere(sphere, io_source, entities.get(targ), sphereContent)) {
			for(Entity * target : sphereContent) {
				arx_assert(target);
				arx_assert(target->obj);
				{
					
					bool HIT_SPARK = false;
					EXCEPTIONS_LIST[EXCEPTIONS_LIST_Pos] = target->index();
					EXCEPTIONS_LIST_Pos++;
					if(EXCEPTIONS_LIST_Pos >= MAX_IN_SPHERE) {
						EXCEPTIONS_LIST_Pos--;
					}
					
					VertexId hitpoint;
					float curdist = 999999.f;
					for(const EERIE_FACE & face : target->obj->facelist) {
						if(face.facetype & POLY_HIDE) {
							continue;
						}
						float d = glm::distance(sphere.origin, target->obj->vertexWorldPositions[face.vid[0]].v);
						if(d < curdist) {
							arx_assume(face.vid[0]);
							hitpoint = face.vid[0];
							curdist = d;
						}
					}
					if(!hitpoint) {
						continue;
					}
					
					Color color = (target->ioflags & IO_NPC) ? target->_npcdata->blood_color : Color::white;
					Vec3f pos = target->obj->vertexWorldPositions[hitpoint].v;
					
					float dmgs = 0.f;
					if(!(flags & 1)) {
						
						dmgs = ARX_EQUIPMENT_ComputeDamages(io_source, target, ratioaim, &pos);
						
						if(target->ioflags & IO_NPC) {
							ret = true;
							target->spark_n_blood = 0;
							target->_npcdata->SPLAT_TOT_NB = 0;
							
							if(drain_life > 0.f) {
								float life_gain = std::min(dmgs, drain_life);
								life_gain = std::min(life_gain, target->_npcdata->lifePool.current);
								life_gain = std::max(life_gain, 0.f);
								if(io_source->ioflags & IO_NPC) {
									healCharacter(*io_source, life_gain);
								}
							}
							
							if(paralyse > 0.f) {
								GameDuration ptime = std::chrono::duration<float, std::milli>(std::min(dmgs * 1000.f, paralyse));
								ARX_SPELLS_Launch(SPELL_PARALYSE,
								                  *io_weapon,
								                  SPELLCAST_FLAG_NOMANA | SPELLCAST_FLAG_NOCHECKCANCAST,
								                  5,
								                  target,
								                  ptime);
							}
						}
						
						if(io_source == entities.player()) {
							ARX_DAMAGES_DurabilityCheck(io_weapon, g_framedelay * 0.006f);
						}
					}
					
					if((target->ioflags & IO_NPC) && (dmgs > 0.f || target->spark_n_blood == SP_BLOODY)) {
						target->spark_n_blood = SP_BLOODY;
						
						if(!(flags & 1)) {
							ARX_PARTICLES_Spawn_Splat(pos, dmgs, color);
							
							float power = (dmgs * 0.025f) + 0.7f;
							
							Sphere sp;
							sp.origin = pos + glm::normalize(toXZ(pos - io_source->pos)) * 30.f;
							sp.radius = 3.5f * power * 20;
							
							if(CheckAnythingInSphere(sp, entities.player(), CAS_NO_NPC_COL)) {
								Sphere splatSphere;
								splatSphere.origin = sp.origin;
								splatSphere.radius = 30.f;
								PolyBoomAddSplat(splatSphere, Color3f(color), 1);
							}
						}
						
						ARX_PARTICLES_Spawn_Blood2(pos, dmgs, color, target);
					} else if(!(target->ioflags & IO_NPC) && dmgs > 0.f) {
						if(target->ioflags & IO_ITEM) {
							ParticleSparkSpawnContinous(pos, Random::getu(0, 3));
						} else {
							ParticleSparkSpawnContinous(pos, Random::getu(0, 30));
						}
						
						spawnAudibleSound(pos, *io_source);
						
						if(io_source == entities.player()) {
							HIT_SPARK = true;
						}
					} else if(target->ioflags & IO_NPC) {
						unsigned int nb;
						
						if(target->spark_n_blood == SP_SPARKING) {
							nb = Random::getu(0, 3);
						} else {
							nb = 30;
						}
						
						if(target->ioflags & IO_ITEM) {
							nb = 1;
						}
						
						ParticleSparkSpawnContinous(pos, nb);
						spawnAudibleSound(pos, *io_source);
						target->spark_n_blood = SP_SPARKING;
						
						if(!(target->ioflags & IO_NPC)) {
							HIT_SPARK = true;
						}
					} else if((target->ioflags & IO_FIX) || (target->ioflags & IO_ITEM)) {
						unsigned int nb;
						
						if(target->spark_n_blood == SP_SPARKING) {
							nb = Random::getu(0, 3);
						} else {
							nb = 30;
						}
						
						if(target->ioflags & IO_ITEM) {
							nb = 1;
						}
						
						ParticleSparkSpawnContinous(pos, nb);
						spawnAudibleSound(pos, *io_source);
						target->spark_n_blood = SP_SPARKING;
						
						if(!(target->ioflags & IO_NPC)) {
							HIT_SPARK = true;
						}
						
					}
					
					if(HIT_SPARK) {
						if(!io_source->isHit) {
							ARX_DAMAGES_DurabilityCheck(io_weapon, 1.f);
							io_source->isHit = true;
							
							std::string_view weapon_material = "metal";
							if(!io_weapon->weaponmaterial.empty()) {
								weapon_material = io_weapon->weaponmaterial;
							}
							
							if(target->material != MATERIAL_NONE) {
								const char * matStr = ARX_MATERIAL_GetNameById(target->material);
								ARX_SOUND_PlayCollision(weapon_material, matStr, 1.f, 1.f, sphere.origin, nullptr);
							}
						}
					}
				}
			}
		}
		
		const EERIEPOLY * ep = CheckBackgroundInSphere(sphere);
		if(ep) {
			if(io_source == entities.player()) {
				if(!io_source->isHit) {
					ARX_DAMAGES_DurabilityCheck(io_weapon, 1.f);
					io_source->isHit = true;
					
					std::string_view weapon_material = "metal";
					if(!io_weapon->weaponmaterial.empty()) {
						weapon_material = io_weapon->weaponmaterial;
					}
					
					std::string_view bkg_material = "earth";
					if(ep && ep->tex && !ep->tex->m_texName.empty()) {
						bkg_material = GetMaterialString(ep->tex->m_texName);
					}
					
					ARX_SOUND_PlayCollision(weapon_material, bkg_material, 1.f, 1.f, sphere.origin, io_source);
				}
			}
			
			ParticleSparkSpawnContinous(sphere.origin, Random::getu(0, 10));
			spawnAudibleSound(sphere.origin, *io_source);
		}
	}
	
	return ret;
}

void ARX_EQUIPMENT_LaunchPlayerReadyWeapon() {
	
	arx_assert(entities.player());
	Entity * entity = entities.player();
	
	ANIM_HANDLE * anim = nullptr;
	switch(ARX_EQUIPMENT_GetPlayerWeaponType()) {
		case WEAPON_DAGGER: {
			anim = entity->anims[ANIM_DAGGER_READY_PART_1];
			break;
		}
		case WEAPON_1H: {
			anim = entity->anims[ANIM_1H_READY_PART_1];
			break;
		}
		case WEAPON_2H: {
			if(!entities.get(player.equiped[EQUIP_SLOT_SHIELD])) {
				anim = entity->anims[ANIM_2H_READY_PART_1];
			}
			break;
		}
		case WEAPON_BOW: {
			if(!entities.get(player.equiped[EQUIP_SLOT_SHIELD])) {
				anim = entity->anims[ANIM_MISSILE_READY_PART_1];
			}
			break;
		}
		default: {
			anim = entity->anims[ANIM_BARE_READY];
			break;
		}
	}
	
	changeAnimation(entity, 1, anim);
	
}

void ARX_EQUIPMENT_UnEquipPlayerWeapon() {
	
	if(Entity * weapon = entities.get(player.equiped[EQUIP_SLOT_WEAPON])) {
		Entity * pioOldDragInter = g_draggedEntity;
		g_draggedEntity = weapon;
		ARX_SOUND_PlayInterface(g_snd.INVSTD);
		ARX_EQUIPMENT_UnEquip(entities.player(), weapon);
		g_draggedEntity = pioOldDragInter;
	}
	
	player.equiped[EQUIP_SLOT_WEAPON] = EntityHandle();
	
}

bool bRing = false;

void ARX_EQUIPMENT_Equip(Entity * target, Entity * toequip) {
	
	if(!target || !toequip || target != entities.player()) {
		return;
	}
	
	toequip->setOwner(nullptr);
	toequip->show = SHOW_FLAG_ON_PLAYER;
	if(toequip == g_draggedEntity) {
		setDraggedEntity(nullptr);
	}
	
	if(toequip->type_flags & (OBJECT_TYPE_DAGGER | OBJECT_TYPE_1H | OBJECT_TYPE_2H | OBJECT_TYPE_BOW)) {
		
		if(Entity * weapon = entities.get(player.equiped[EQUIP_SLOT_WEAPON])) {
			ARX_EQUIPMENT_UnEquip(target, weapon);
		}
		player.equiped[EQUIP_SLOT_WEAPON] = toequip->index();
		std::string_view attach = (toequip->type_flags & OBJECT_TYPE_BOW) ? "test" : "primary_attach";
		linkEntities(*target, "weapon_attach", *toequip, attach);
		
		if(toequip->type_flags & (OBJECT_TYPE_2H | OBJECT_TYPE_BOW)) {
			if(Entity * shield = entities.get(player.equiped[EQUIP_SLOT_SHIELD])) {
				ARX_EQUIPMENT_UnEquip(target, shield);
			}
		}
		
	} else if(toequip->type_flags & OBJECT_TYPE_SHIELD) {
		
		if(Entity * shield = entities.get(player.equiped[EQUIP_SLOT_SHIELD])) {
			ARX_EQUIPMENT_UnEquip(target, shield);
		}
		player.equiped[EQUIP_SLOT_SHIELD] = toequip->index();
		linkEntities(*target, "shield_attach", *toequip, "shield_attach");
		
		if(Entity * weapon = entities.get(player.equiped[EQUIP_SLOT_WEAPON])) {
			if(weapon->type_flags & (OBJECT_TYPE_2H | OBJECT_TYPE_BOW)) {
				ARX_EQUIPMENT_UnEquip(target, weapon);
			}
		}
		
	} else if(toequip->type_flags & OBJECT_TYPE_RING) {
		
		// check first, if not already equiped
		if(entities.get(player.equiped[EQUIP_SLOT_RING_LEFT]) != toequip
		   && entities.get(player.equiped[EQUIP_SLOT_RING_RIGHT]) != toequip) {
			long willequip = -1;
			if(player.equiped[EQUIP_SLOT_RING_LEFT] == EntityHandle()) {
				willequip = EQUIP_SLOT_RING_LEFT;
			}
			if(player.equiped[EQUIP_SLOT_RING_RIGHT] == EntityHandle()) {
				willequip = EQUIP_SLOT_RING_RIGHT;
			}
			if(willequip == -1) {
				if(bRing) {
					if(Entity * ring = entities.get(player.equiped[EQUIP_SLOT_RING_RIGHT])) {
						ARX_EQUIPMENT_UnEquip(target, ring);
					}
					willequip = EQUIP_SLOT_RING_RIGHT;
				} else {
					if(Entity * ring = entities.get(player.equiped[EQUIP_SLOT_RING_LEFT])) {
						ARX_EQUIPMENT_UnEquip(target, ring);
					}
					willequip = EQUIP_SLOT_RING_LEFT;
				}
				bRing = !bRing;
			}
			player.equiped[willequip] = toequip->index();
		}
		
	} else if(toequip->type_flags & OBJECT_TYPE_ARMOR) {
		
		if(Entity * armor = entities.get(player.equiped[EQUIP_SLOT_ARMOR])) {
			ARX_EQUIPMENT_UnEquip(target, armor);
		}
		player.equiped[EQUIP_SLOT_ARMOR] = toequip->index();
		
	} else if(toequip->type_flags & OBJECT_TYPE_LEGGINGS) {
		
		if(Entity * leggings = entities.get(player.equiped[EQUIP_SLOT_LEGGINGS])) {
			ARX_EQUIPMENT_UnEquip(target, leggings);
		}
		player.equiped[EQUIP_SLOT_LEGGINGS] = toequip->index();
		
	} else if(toequip->type_flags & OBJECT_TYPE_HELMET) {
		
		if(Entity * helmet = entities.get(player.equiped[EQUIP_SLOT_HELMET])) {
			ARX_EQUIPMENT_UnEquip(target, helmet);
		}
		player.equiped[EQUIP_SLOT_HELMET] = toequip->index();
		
	}
	
	if(toequip->type_flags & (OBJECT_TYPE_HELMET | OBJECT_TYPE_ARMOR | OBJECT_TYPE_LEGGINGS)) {
		ARX_EQUIPMENT_RecreatePlayerMesh();
	}
	
	ARX_PLAYER_ComputePlayerFullStats();
	
}

bool ARX_EQUIPMENT_SetObjectType(Entity & io, std::string_view temp, bool set) {
	
	ItemType flag = ARX_EQUIPMENT_GetObjectTypeFlag(temp);
	
	if(set) {
		io.type_flags |= flag;
	} else {
		io.type_flags &= ~flag;
	}
	
	return (flag != 0);
}

//! \brief Initializes Equipment infos
void ARX_EQUIPMENT_Init()
{
	equipinfo[IO_EQUIPITEM_ELEMENT_STRENGTH].name = "strength";
	equipinfo[IO_EQUIPITEM_ELEMENT_DEXTERITY].name = "dexterity";
	equipinfo[IO_EQUIPITEM_ELEMENT_CONSTITUTION].name = "constitution";
	equipinfo[IO_EQUIPITEM_ELEMENT_MIND].name = "intelligence";
	equipinfo[IO_EQUIPITEM_ELEMENT_Stealth].name = "stealth";
	equipinfo[IO_EQUIPITEM_ELEMENT_Mecanism].name = "mecanism";
	equipinfo[IO_EQUIPITEM_ELEMENT_Intuition].name = "intuition";
	equipinfo[IO_EQUIPITEM_ELEMENT_Etheral_Link].name = "etheral_link";
	equipinfo[IO_EQUIPITEM_ELEMENT_Object_Knowledge].name = "object_knowledge";
	equipinfo[IO_EQUIPITEM_ELEMENT_Casting].name = "casting";
	equipinfo[IO_EQUIPITEM_ELEMENT_Projectile].name = "projectile";
	equipinfo[IO_EQUIPITEM_ELEMENT_Close_Combat].name = "close_combat";
	equipinfo[IO_EQUIPITEM_ELEMENT_Defense].name = "defense";
	equipinfo[IO_EQUIPITEM_ELEMENT_Armor_Class].name = "armor_class";
	equipinfo[IO_EQUIPITEM_ELEMENT_Resist_Magic].name = "resist_magic";
	equipinfo[IO_EQUIPITEM_ELEMENT_Resist_Poison].name = "resist_poison";
	equipinfo[IO_EQUIPITEM_ELEMENT_Critical_Hit].name = "critical_hit";
	equipinfo[IO_EQUIPITEM_ELEMENT_Damages].name = "damages";
	equipinfo[IO_EQUIPITEM_ELEMENT_Duration].name = "duration";
	equipinfo[IO_EQUIPITEM_ELEMENT_AimTime].name = "aim_time";
	equipinfo[IO_EQUIPITEM_ELEMENT_Identify_Value].name = "identify_value";
	equipinfo[IO_EQUIPITEM_ELEMENT_Life].name = "life";
	equipinfo[IO_EQUIPITEM_ELEMENT_Mana].name = "mana";
	equipinfo[IO_EQUIPITEM_ELEMENT_MaxLife].name = "maxlife";
	equipinfo[IO_EQUIPITEM_ELEMENT_MaxMana].name = "maxmana";
	equipinfo[IO_EQUIPITEM_ELEMENT_SPECIAL_1].name = "special1";
	equipinfo[IO_EQUIPITEM_ELEMENT_SPECIAL_2].name = "special2";
	equipinfo[IO_EQUIPITEM_ELEMENT_SPECIAL_3].name = "special3";
	equipinfo[IO_EQUIPITEM_ELEMENT_SPECIAL_4].name = "special4";
}

//! \brief Removes All special equipement properties
void ARX_EQUIPMENT_Remove_All_Special(Entity * io)
{
	if(!io || !(io->ioflags & IO_ITEM)) {
		return;
	}
	
	io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_SPECIAL_1].special = IO_SPECIAL_ELEM_NONE;
	io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_SPECIAL_2].special = IO_SPECIAL_ELEM_NONE;
	io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_SPECIAL_3].special = IO_SPECIAL_ELEM_NONE;
	io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_SPECIAL_4].special = IO_SPECIAL_ELEM_NONE;
}

float getEquipmentBaseModifier(EquipmentModifierType modifier, bool getRelative) {
	
	float sum = 0;
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment)) {
			if((toequip->ioflags & IO_ITEM) && toequip->_itemdata->equipitem) {
				IO_EQUIPITEM_ELEMENT * elem = &toequip->_itemdata->equipitem->elements[modifier];
				bool isRelative = elem->flags.has(IO_ELEMENT_FLAG_PERCENT);
				if(isRelative == getRelative) {
					sum += elem->value;
				}
			}
		}
	}
	
	if(getRelative) {
		// Convert from percent to ratio
		sum *= 0.01f;
	}
	
	return sum;
}

float getEquipmentModifier(EquipmentModifierType modifier, float baseval) {
	float modabs = getEquipmentBaseModifier(modifier, false);
	float modrel = getEquipmentBaseModifier(modifier, true);
	return modabs + modrel * std::max(0.f, baseval + modabs);
}

void ARX_EQUIPMENT_SetEquip(Entity * io, bool special,
                            std::string_view modifierName, float val,
                            EquipmentModifierFlags flags) {
	
	if(io == nullptr) {
		return;
	}
	
	if(!(io->ioflags & IO_ITEM)) {
		return;
	}
	
	if(!io->_itemdata->equipitem) {
		io->_itemdata->equipitem = new IO_EQUIPITEM;
		io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_Duration].value = 10;
		io->_itemdata->equipitem->elements[IO_EQUIPITEM_ELEMENT_AimTime].value = 10;
	}
	
	if(special) {
		for(long i = IO_EQUIPITEM_ELEMENT_SPECIAL_1; i <= IO_EQUIPITEM_ELEMENT_SPECIAL_4; i++) {
			if(io->_itemdata->equipitem->elements[i].special == IO_SPECIAL_ELEM_NONE) {
				if(modifierName == "paralyse") {
					io->_itemdata->equipitem->elements[i].special = IO_SPECIAL_ELEM_PARALYZE;
				} else if(modifierName == "drainlife") {
					io->_itemdata->equipitem->elements[i].special = IO_SPECIAL_ELEM_DRAIN_LIFE;
				}
				
				io->_itemdata->equipitem->elements[i].value = val;
				io->_itemdata->equipitem->elements[i].flags = flags;
				return;
			}
		}
		return;
	}
	
	for(long i = 0; i < IO_EQUIPITEM_ELEMENT_Number; i++) {
		if(modifierName == equipinfo[i].name) {
			io->_itemdata->equipitem->elements[i].value = val;
			io->_itemdata->equipitem->elements[i].special = IO_SPECIAL_ELEM_NONE;
			io->_itemdata->equipitem->elements[i].flags = flags;
			return;
		}
	}
}

void ARX_EQUIPMENT_IdentifyAll() {
	
	arx_assert(entities.player());
	
	for(EntityHandle equipment : player.equiped) {
		if(Entity * toequip = entities.get(equipment)) {
			ARX_INVENTORY_IdentifyIO(toequip);
		}
	}
	
}

float GetHitValue(std::string_view name) {
	
	if(boost::starts_with(name, "hit_")) {
		// Get the number after the first 4 characters in the string
		try {
			return float(util::parseInt(name.substr(4)));
		} catch(...) { /* ignore */ }
	}
	
	return -1;
}
