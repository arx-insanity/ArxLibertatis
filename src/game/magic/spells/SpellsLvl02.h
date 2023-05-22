/*
 * Copyright 2014-2022 Arx Libertatis Team (see the AUTHORS file)
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

#ifndef ARX_GAME_MAGIC_SPELLS_SPELLSLVL02_H
#define ARX_GAME_MAGIC_SPELLS_SPELLSLVL02_H

#include "game/magic/Spell.h"
#include "graphics/effects/Cabal.h"
#include "graphics/particle/ParticleSystem.h"
#include "platform/Platform.h"

class HealSpell final : public Spell {
	
public:
	
	HealSpell();
	
	bool CanLaunch() override;
	void Launch() override;
	void End() override;
	void Update() override;
	
private:
	
	Vec3f m_pos;
	LightHandle m_light;
	ParticleSystem m_particles;
	
};

class DetectTrapSpell final : public Spell {
	
public:
	
	void Launch() override;
	void End() override;
	void Update() override;
	
};

class ArmorSpell final : public Spell {
	
public:
	
	void Launch() override;
	void End() override;
	void Update() override;
	
	Vec3f getPosition() const override;
	
};

class LowerArmorSpell final : public Spell {
	
public:
	
	LowerArmorSpell();
	
	void Launch() override;
	void End() override;
	void Update() override;
	
	Vec3f getPosition() const override;
	
private:
	
	bool m_haloCreated;
	
};

class HarmSpell final : public Spell {
	
public:
	
	void Launch() override;
	void End() override;
	void Update() override;
	
private:
	
	DamageHandle m_damage;
	CabalFx m_cabal;
	
};

#endif // ARX_GAME_MAGIC_SPELLS_SPELLSLVL02_H
