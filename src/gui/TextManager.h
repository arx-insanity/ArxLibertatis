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

#ifndef ARX_GUI_TEXTMANAGER_H
#define ARX_GUI_TEXTMANAGER_H

#include <string>
#include <vector>

#include "core/TimeTypes.h"
#include "graphics/Color.h"
#include "math/Types.h"

class Font;

class TextManager {
	
public:
	
	bool AddText(Font * font, std::string && text, const Rect & bounds,
	             Color color = Color::white, PlatformDuration displayTime = 0,
	             PlatformDuration scrollDelay = 0, float scrollSpeed = 0.f, int maxLines = 0);
	
	void Update(PlatformDuration delta);
	void Render();
	void Clear();
	bool Empty() const;
	
private:
	
	struct ManagedText {
		
		std::string text;
		Font * font = nullptr;
		PlatformDuration scrollDelay;
		PlatformDuration displayTime;
		Rectf bounds;
		Rect clipRect;
		float scrollPosition = 0.f;
		float scrollSpeed = 0.f;
		Color color;
		
	};
	
	std::vector<ManagedText> m_entries;
	
};

#endif // ARX_GUI_TEXTMANAGER_H
