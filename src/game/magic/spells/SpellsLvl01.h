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

#ifndef ARX_GAME_MAGIC_SPELLS_SPELLSLVL01_H
#define ARX_GAME_MAGIC_SPELLS_SPELLSLVL01_H

#include <memory>

#include "game/magic/Spell.h"

#include "graphics/effects/MagicMissile.h"
#include "platform/Platform.h"

class MagicSightSpell final : public Spell {
	
public:
	
	bool CanLaunch() override;
	void Launch() override;
	void End() override;
	void Update() override;
	
};

class MagicMissileSpell final : public Spell {
	
public:
	
	MagicMissileSpell();
	
	void Launch() override;
	void End() override;
	void Update() override;
	
private:
	
	bool m_mrCheat;
	audio::SourcedSample snd_loop;
	std::vector<LightHandle> m_lights;
	std::vector<std::unique_ptr<CMagicMissile>> m_missiles;
	
};

class IgnitSpell final : public Spell {
	
public:
	
	IgnitSpell();
	
	void Launch() override;
	void End() override;
	void Update() override;
	
private:
	
	Vec3f m_srcPos;
	
	struct T_LINKLIGHTTOFX {
		LightHandle m_effectLight;
		size_t m_targetLight;
	};
	
	std::vector<T_LINKLIGHTTOFX> m_lights;
	
};

class DouseSpell final : public Spell {
	
public:
	
	void Launch() override;
	void End() override;
	void Update() override;
	
private:
	
	std::vector<size_t> m_lights;
	
};

class ActivatePortalSpell final : public Spell {
	
public:
	
	void Launch() override;
	
};

#endif // ARX_GAME_MAGIC_SPELLS_SPELLSLVL01_H
