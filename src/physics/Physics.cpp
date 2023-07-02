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

#include "physics/Physics.h"

#include <stddef.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "graphics/Raycast.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/data/Mesh.h"

#include "math/Vector.h"

#include "core/GameTime.h"

#include "game/NPC.h"
#include "game/EntityManager.h"
#include "game/magic/spells/SpellsLvl06.h"

#include "physics/Collisions.h"

#include "scene/GameSound.h"
#include "scene/Interactive.h"
#include "scene/Tiles.h"


// Creation of the physics box... quite cabalistic and extensive func...
// Need to put a (really) smarter algorithm in there...
void EERIE_PHYSICS_BOX_Create(EERIE_3DOBJ * obj) {
	
	if(!obj) {
		return;
	}
	
	obj->pbox.reset();
	
	if(obj->vertexlist.empty()) {
		return;
	}
	
	std::unique_ptr<PHYSICS_BOX_DATA> pbox = std::make_unique<PHYSICS_BOX_DATA>();
	
	pbox->stopcount = 0;
	
	Vec3f cubmin = Vec3f(std::numeric_limits<float>::max());
	Vec3f cubmax = Vec3f(-std::numeric_limits<float>::max());
	
	const EERIE_VERTEX & origin = obj->vertexlist[obj->origin];
	for(const EERIE_VERTEX & vertex : obj->vertexlist) {
		if(&vertex != &origin) {
			cubmin = glm::min(cubmin, vertex.v);
			cubmax = glm::max(cubmax, vertex.v);
		}
	}
	
	pbox->vert[0].pos = cubmin + (cubmax - cubmin) * .5f;
	pbox->vert[13].pos = pbox->vert[0].pos;
	pbox->vert[13].pos.y = cubmin.y;
	pbox->vert[14].pos = pbox->vert[0].pos;
	pbox->vert[14].pos.y = cubmax.y;
	
	for(size_t k = 1; k < pbox->vert.size() - 2; k++) {
		pbox->vert[k].pos.x = pbox->vert[0].pos.x;
		pbox->vert[k].pos.z = pbox->vert[0].pos.z;
		if(k < 5) {
			pbox->vert[k].pos.y = cubmin.y;
		} else if(k < 9) {
			pbox->vert[k].pos.y = pbox->vert[0].pos.y;
		} else {
			pbox->vert[k].pos.y = cubmax.y;
		}
	}
	
	float diff = cubmax.y - cubmin.y;
	if(diff < 12.f) {
		
		cubmax.y += 8.f;
		cubmin.y -= 8.f;
		
		for(size_t k = 1; k < pbox->vert.size() - 2; k++) {
			pbox->vert[k].pos.x = pbox->vert[0].pos.x;
			pbox->vert[k].pos.z = pbox->vert[0].pos.z;
			if(k < 5) {
				pbox->vert[k].pos.y = cubmin.y;
			} else if(k < 9) {
				pbox->vert[k].pos.y = pbox->vert[0].pos.y;
			} else {
				pbox->vert[k].pos.y = cubmax.y;
			}
		}
		
		pbox->vert[14].pos.y = cubmax.y;
		pbox->vert[13].pos.y = cubmin.y;
		float RATI = diff * (1.0f / 8);
		
		for(const EERIE_VERTEX & vertex : obj->vertexlist) {
			
			if(&vertex == &origin) {
				continue;
			}
			
			size_t SEC = 1;
			pbox->vert[SEC].pos.x = std::min(pbox->vert[SEC].pos.x, vertex.v.x);
			pbox->vert[SEC].pos.z = std::min(pbox->vert[SEC].pos.z, vertex.v.z);
			pbox->vert[SEC + 1].pos.x = std::min(pbox->vert[SEC + 1].pos.x, vertex.v.x);
			pbox->vert[SEC + 1].pos.z = std::max(pbox->vert[SEC + 1].pos.z, vertex.v.z);
			pbox->vert[SEC + 2].pos.x = std::max(pbox->vert[SEC + 2].pos.x, vertex.v.x);
			pbox->vert[SEC + 2].pos.z = std::max(pbox->vert[SEC + 2].pos.z, vertex.v.z);
			pbox->vert[SEC + 3].pos.x = std::max(pbox->vert[SEC + 3].pos.x, vertex.v.x);
			pbox->vert[SEC + 3].pos.z = std::min(pbox->vert[SEC + 3].pos.z, vertex.v.z);
			
			SEC = 5;
			pbox->vert[SEC].pos.x = std::min(pbox->vert[SEC].pos.x, vertex.v.x - RATI);
			pbox->vert[SEC].pos.z = std::min(pbox->vert[SEC].pos.z, vertex.v.z - RATI);
			pbox->vert[SEC + 1].pos.x = std::min(pbox->vert[SEC + 1].pos.x, vertex.v.x - RATI);
			pbox->vert[SEC + 1].pos.z = std::max(pbox->vert[SEC + 1].pos.z, vertex.v.z + RATI);
			pbox->vert[SEC + 2].pos.x = std::max(pbox->vert[SEC + 2].pos.x, vertex.v.x + RATI);
			pbox->vert[SEC + 2].pos.z = std::max(pbox->vert[SEC + 2].pos.z, vertex.v.z + RATI);
			pbox->vert[SEC + 3].pos.x = std::max(pbox->vert[SEC + 3].pos.x, vertex.v.x + RATI);
			pbox->vert[SEC + 3].pos.z = std::min(pbox->vert[SEC + 3].pos.z, vertex.v.z - RATI);
			
			SEC = 9;
			pbox->vert[SEC].pos.x = std::min(pbox->vert[SEC].pos.x, vertex.v.x);
			pbox->vert[SEC].pos.z = std::min(pbox->vert[SEC].pos.z, vertex.v.z);
			pbox->vert[SEC + 1].pos.x = std::min(pbox->vert[SEC + 1].pos.x, vertex.v.x);
			pbox->vert[SEC + 1].pos.z = std::max(pbox->vert[SEC + 1].pos.z, vertex.v.z);
			pbox->vert[SEC + 2].pos.x = std::max(pbox->vert[SEC + 2].pos.x, vertex.v.x);
			pbox->vert[SEC + 2].pos.z = std::max(pbox->vert[SEC + 2].pos.z, vertex.v.z);
			pbox->vert[SEC + 3].pos.x = std::max(pbox->vert[SEC + 3].pos.x, vertex.v.x);
			pbox->vert[SEC  + 3].pos.z = std::min(pbox->vert[SEC + 3].pos.z, vertex.v.z);
			
		}
		
	} else {
		
		float cut = (cubmax.y - cubmin.y) * (1.0f / 3);
		float ysec2 = cubmin.y + cut * 2.f;
		float ysec1 = cubmin.y + cut;
		
		for(const EERIE_VERTEX & vertex : obj->vertexlist) {
			
			if(&vertex == &origin) {
				continue;
			}
			
			size_t SEC;
			if(vertex.v.y < ysec1) {
				SEC = 1;
			} else if(vertex.v.y < ysec2) {
				SEC = 5;
			} else {
				SEC = 9;
			}
			
			pbox->vert[SEC].pos.x = std::min(pbox->vert[SEC].pos.x, vertex.v.x);
			pbox->vert[SEC].pos.z = std::min(pbox->vert[SEC].pos.z, vertex.v.z);
			pbox->vert[SEC + 1].pos.x = std::min(pbox->vert[SEC + 1].pos.x, vertex.v.x);
			pbox->vert[SEC + 1].pos.z = std::max(pbox->vert[SEC + 1].pos.z, vertex.v.z);
			pbox->vert[SEC + 2].pos.x = std::max(pbox->vert[SEC + 2].pos.x, vertex.v.x);
			pbox->vert[SEC + 2].pos.z = std::max(pbox->vert[SEC + 2].pos.z, vertex.v.z);
			pbox->vert[SEC + 3].pos.x = std::max(pbox->vert[SEC + 3].pos.x, vertex.v.x);
			pbox->vert[SEC + 3].pos.z = std::min(pbox->vert[SEC + 3].pos.z, vertex.v.z);
			
		}
		
	}
	
	for(size_t k = 0; k < 4; k++) {
		if(glm::abs(pbox->vert[5 + k].pos.x - pbox->vert[0].pos.x) < 2.f) {
			pbox->vert[5 + k].pos.x = (pbox->vert[1 + k].pos.x + pbox->vert[9 + k].pos.x) * 0.5f;
		}
		if(glm::abs(pbox->vert[5 + k].pos.z - pbox->vert[0].pos.z) < 2.f) {
			pbox->vert[5 + k].pos.z = (pbox->vert[1 + k].pos.z + pbox->vert[9 + k].pos.z) * 0.5f;
		}
	}
	
	pbox->radius = 0.f;
	
	bool first = true;
	for(PhysicsParticle & particle : pbox->vert) {
		
		if(glm::distance(particle.pos, pbox->vert[0].pos) > 20.f) {
			particle.pos.x = (particle.pos.x - pbox->vert[0].pos.x) * 0.5f + pbox->vert[0].pos.x;
			particle.pos.z = (particle.pos.z - pbox->vert[0].pos.z) * 0.5f + pbox->vert[0].pos.z;
		}
		
		particle.initpos = particle.pos;
		
		if(first) {
			first = false;
		} else {
			float d = glm::distance(pbox->vert[0].pos, particle.pos);
			pbox->radius = std::max(pbox->radius, d);
		}
		
	}
	
	float surface = 0.f;
	for(const EERIE_FACE & face : obj->facelist) {
		const Vec3f & p0 = obj->vertexlist[face.vid[0]].v;
		const Vec3f & p1 = obj->vertexlist[face.vid[1]].v;
		const Vec3f & p2 = obj->vertexlist[face.vid[2]].v;
		surface += glm::distance((p0 + p1) * .5f, p2) * glm::distance(p0, p1) * .5f;
	}
	pbox->surface = surface;
	
	obj->pbox = std::move(pbox);
}

// Used to launch an object into the physical world...
void EERIE_PHYSICS_BOX_Launch(EERIE_3DOBJ * obj, const Vec3f & pos,
                              const Anglef & angle, const Vec3f & vect) {
	
	arx_assert(obj);
	arx_assert(obj->pbox);
	
	float ratio = obj->pbox->surface * (1.0f / 10000);
	ratio = glm::clamp(ratio, 0.f, 0.8f);
	ratio = 1.f - (ratio * (1.0f / 4));
	
	for(PhysicsParticle & particle : obj->pbox->vert) {
		
		particle.pos = particle.initpos;
		particle.pos = VRotateY(particle.pos, MAKEANGLE(270.f - angle.getYaw()));
		particle.pos = VRotateX(particle.pos, -angle.getPitch());
		particle.pos = VRotateZ(particle.pos, angle.getRoll());
		particle.pos += pos;
		
		particle.force = Vec3f(0.f);
		particle.velocity = vect * (250.f * ratio);
		particle.mass = 0.4f + ratio * 0.1f;
		
	}
	
	obj->pbox->active = 1;
	obj->pbox->stopcount = 0;
	obj->pbox->storedtiming = 0;
	
}


static const float VELOCITY_THRESHOLD = 400.f;

template <size_t N>
static void ComputeForces(std::array<PhysicsParticle, N> & particles) {
	
	const Vec3f PHYSICS_Gravity(0.f, 65.f, 0.f);
	const float PHYSICS_Damping = 0.5f;
	
	float lastmass = 1.f;
	float div = 1.f;
	
	for(PhysicsParticle & particle : particles) {
		
		// Reset Force
		particle.force = Vec3f(0.f);
		
		// Apply Gravity
		if(particle.mass > 0.f) {
			// need to be precomputed...
			if(lastmass != particle.mass) {
				div = 1.f / particle.mass;
				lastmass = particle.mass;
			}
			particle.force += (PHYSICS_Gravity * div);
		}
		
		// Apply Damping
		particle.force += particle.velocity * -PHYSICS_Damping;
		
	}
	
}

//! Calculate new Positions and Velocities given a deltatime
//! \param DeltaTime that has passed since last iteration
template <size_t N>
static void RK4Integrate(std::array<PhysicsParticle, N> & particles, float DeltaTime) {
	
	float halfDeltaT, sixthDeltaT;
	halfDeltaT = DeltaTime * .5f; // some time values i will need
	sixthDeltaT = (1.0f / 6);
	
	std::array<std::array<PhysicsParticle, N>, 5> m_TempSys;
	
	for(size_t i = 0; i < 4; i++) {
		
		arx_assert(particles.size() == m_TempSys[i + 1].size());
		m_TempSys[i + 1] = particles;
		
		if(i == 3) {
			halfDeltaT = DeltaTime;
		}
		
		for(size_t j = 0; j < particles.size(); j++) {
			
			const PhysicsParticle & source = particles[j];
			PhysicsParticle & accum1 = m_TempSys[i + 1][j];
			PhysicsParticle & target = m_TempSys[0][j];
			
			accum1.force = source.force * (source.mass * halfDeltaT);
			accum1.velocity = source.velocity * halfDeltaT;
			
			// determine the new velocity for the particle over 1/2 time
			target.velocity = source.velocity + accum1.force;
			target.mass = source.mass;
			
			// set the new position
			target.pos = source.pos + accum1.velocity;
			
		}
		
		ComputeForces(m_TempSys[0]); // compute the new forces
	}
	
	for(size_t j = 0; j < particles.size(); j++) {
		
		PhysicsParticle & particle = particles[j];
		
		const PhysicsParticle & accum1 = m_TempSys[1][j];
		const PhysicsParticle & accum2 = m_TempSys[2][j];
		const PhysicsParticle & accum3 = m_TempSys[3][j];
		const PhysicsParticle & accum4 = m_TempSys[4][j];
		
		Vec3f dv = accum1.force + ((accum2.force + accum3.force) * 2.f) + accum4.force;
		Vec3f dp = accum1.velocity + ((accum2.velocity + accum3.velocity) * 2.f) + accum4.velocity;
		
		particle.velocity += (dv * sixthDeltaT);
		particle.pos += (dp * sixthDeltaT * 1.2f); // TODO what is this 1.2 factor doing here ?
		
	}
	
}

static bool IsObjectInField(const PHYSICS_BOX_DATA & pbox) {
	
	for(const Spell & spell : spells.ofType(SPELL_CREATE_FIELD)) {
		
		if(Entity * field = entities.get(static_cast<const CreateFieldSpell &>(spell).m_entity)) {
			for(const PhysicsParticle & particle : pbox.vert) {
				Cylinder cylinder = Cylinder(particle.pos + Vec3f(0.f, 17.5f, 0.f), 35.f, -35.f);
				if(isCylinderCollidingWithPlatform(cylinder, *field)) {
					return true;
				}
			}
		}
		
	}
	
	return false;
}

static Material polyTypeToCollisionMaterial(const EERIEPOLY & ep) {
	if(ep.type & POLY_METAL) {
		return MATERIAL_METAL;
	}
	if(ep.type & POLY_WOOD) {
		return MATERIAL_WOOD;
	}
	if(ep.type & POLY_STONE) {
		return MATERIAL_STONE;
	}
	if(ep.type & POLY_GRAVEL) {
		return MATERIAL_GRAVEL;
	}
	if(ep.type & POLY_WATER) {
		return MATERIAL_WATER;
	}
	if(ep.type & POLY_EARTH) {
		return MATERIAL_EARTH;
	}
	return MATERIAL_STONE;
}

static bool ARX_INTERACTIVE_CheckFULLCollision(const PHYSICS_BOX_DATA & pbox, Entity & source) {
	
	for(const auto & entry : treatio) {
		
		if(entry.show != SHOW_FLAG_IN_SCENE || (entry.ioflags & IO_NO_COLLISIONS) || !entry.io) {
			continue;
		}
		
		Entity & entity = *entry.io;
		if(entity == source || !entity.obj || &entity == entities.player()
		   || entity.index() == source.no_collide
		   || (entity.ioflags & (IO_CAMERA | IO_MARKER | IO_ITEM))
		   || entity.usepath
		   || ((entity.ioflags & IO_NPC) && (source.ioflags & IO_NO_NPC_COLLIDE))
		   || !closerThan(entity.pos, pbox.vert[0].pos, 600.f)
		   || !In3DBBoxTolerance(pbox.vert[0].pos, entity.bbox3D, pbox.radius)) {
			continue;
		}
		
		if((entity.ioflags & IO_NPC) && entity._npcdata->lifePool.current > 0.f) {
			for(const PhysicsParticle & vertex : pbox.vert) {
				if(PointInCylinder(entity.physics.cyl, vertex.pos)) {
					return true;
				}
			}
			continue;
		}
		
		if(!(entity.ioflags & IO_FIX)) {
			continue;
		}
		
		if((entity.gameFlags & GFLAG_PLATFORM)) {
			for(const PhysicsParticle & particle : pbox.vert) {
				if((entity.bbox3D.max.y <= particle.pos.y + 30.f || entity.bbox3D.min.y >= particle.pos.y) &&
				   platformCollides(entity, Sphere(particle.pos, 30.f))) {
					return true;
				}
			}
		}
		
		size_t step = 6;
		const size_t nbv = entity.obj->vertexlist.size();
		Sphere sp;
		sp.radius = 28.f;
		if(nbv < 500) {
			step = 1;
			sp.radius = 36.f;
		} else if(nbv < 900) {
			step = 2;
		} else if(nbv < 1500) {
			step = 4;
		}
		
		for(size_t i = 1; i < nbv; i += step) {
			if(VertexId(i) != entity.obj->origin) {
				sp.origin = entity.obj->vertexWorldPositions[VertexId(i)].v;
				for(const PhysicsParticle & vertex : pbox.vert) {
					if(sp.contains(vertex.pos)) {
						if((entity.gameFlags & GFLAG_DOOR)) {
							GameDuration elapsed = g_gameTime.now() - entity.collide_door_time;
							if(elapsed > 500ms) {
								entity.collide_door_time = g_gameTime.now();
								SendIOScriptEvent(&source, &entity, SM_COLLIDE_DOOR);
								entity.collide_door_time = g_gameTime.now();
								SendIOScriptEvent(&entity, &source, SM_COLLIDE_DOOR);
							}
						}
						return true;
					}
				}
			}
		}
		
	}
	
	return false;
}


static void ARX_TEMPORARY_TrySound(Entity & source, Material collisionMaterial, float volume) {
	
	GameInstant now = g_gameTime.now();
	
	if(now > source.soundtime) {
		source.soundcount++;
		if(source.soundcount < 5) {
			Material material = EEIsUnderWater(source.pos) ? MATERIAL_WATER : source.material;
			ARX_SOUND_PlayCollision(material, collisionMaterial, std::min(volume, 1.f), 1.f, source.pos, &source);
			source.soundtime = now + 100ms;
		}
	}
	
}

bool EERIE_PHYSICS_BOX_IsValidPosition(const Vec3f & pos) {
	
	auto tile = g_tiles->getTile(pos);
	if(!tile) {
		// Position is outside the world grid
		return false;
	}
	
	if(tile.intersectingPolygons().empty()) {
		// Position is in an empty tile
		return false;
	}
	
	if(pos.y > tile.maxY()) {
		// Position is below the lowest part of the tile
		return false;
	}
	
	return true;
}

static void ARX_EERIE_PHYSICS_BOX_Compute(PHYSICS_BOX_DATA & pbox, float framediff, Entity & source) {

	Vec3f oldpos[32];
	
	for(size_t i = 0; i < pbox.vert.size(); i++) {
		PhysicsParticle * pv = &pbox.vert[i];
		oldpos[i] = pv->pos;
		pv->velocity.x = glm::clamp(pv->velocity.x, -VELOCITY_THRESHOLD, VELOCITY_THRESHOLD);
		pv->velocity.y = glm::clamp(pv->velocity.y, -VELOCITY_THRESHOLD, VELOCITY_THRESHOLD);
		pv->velocity.z = glm::clamp(pv->velocity.z, -VELOCITY_THRESHOLD, VELOCITY_THRESHOLD);
	}
	
	RK4Integrate(pbox.vert, framediff);
	
	EERIEPOLY * collisionPoly = nullptr;
	for(size_t i = 0; i < pbox.vert.size(); i++) {
		const Vec3f start = oldpos[i];
		const Vec3f end = pbox.vert[i].pos;
		RaycastResult ray = raycastScene(start, end, POLY_NOCOL | POLY_WATER | POLY_TRANS);
		if(ray.hit) {
			collisionPoly = ray.hit;
			break;
		}
	}
	
	if(!collisionPoly && !ARX_INTERACTIVE_CheckFULLCollision(pbox, source) && !IsObjectInField(pbox)) {
		pbox.stopcount = std::max(pbox.stopcount - 2, 0);
		return;
	}
	
	Material collisionMat = MATERIAL_STONE;
	if(collisionPoly) {
		collisionMat = polyTypeToCollisionMaterial(*collisionPoly);
	}
	Vec3f velocity = pbox.vert[0].velocity;
	float power = (glm::abs(velocity.x) + glm::abs(velocity.y) + glm::abs(velocity.z)) * .01f;
	ARX_TEMPORARY_TrySound(source, collisionMat, 0.4f + power);
	
	if(!collisionPoly) {
		for(size_t k = 0; k < pbox.vert.size(); k++) {
			PhysicsParticle * pv = &pbox.vert[k];
			pv->velocity *= Vec3f(-0.3f, -0.4f, -0.3f);
			pv->pos = oldpos[k];
		}
	} else {
		for(size_t k = 0; k < pbox.vert.size(); k++) {
			PhysicsParticle * pv = &pbox.vert[k];
			float t = glm::dot(collisionPoly->norm, pv->velocity);
			pv->velocity -= collisionPoly->norm * (2.f * t);
			pv->velocity *= Vec3f(0.3f, 0.4f, 0.3f);
			pv->pos = oldpos[k];
		}
	}
	
	pbox.stopcount += 1;
	
}

void ARX_PHYSICS_BOX_ApplyModel(PHYSICS_BOX_DATA & pbox, float framediff, float rubber, Entity & source) {
	
	if(pbox.active == 2) {
		return;
	}
	
	if(framediff == 0.f) {
		return;
	}
	
	float timing = pbox.storedtiming + framediff * rubber * 0.0055f;
	float t_threshold = 0.18f;
	
	if(timing < t_threshold) {
		pbox.storedtiming = timing;
		return;
	}
	
	while(timing >= t_threshold) {
		ComputeForces(pbox.vert);
		ARX_EERIE_PHYSICS_BOX_Compute(pbox, std::min(0.11f, timing * 10), source);
		timing -= t_threshold;
	}
	
	pbox.storedtiming = timing;
	
	if(pbox.stopcount < 16) {
		return;
	}
	
	pbox.active = 2;
	pbox.stopcount = 0;
	
	source.soundcount = 0;
	source.soundtime = g_gameTime.now() + 2s;
}
