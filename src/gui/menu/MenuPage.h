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

#ifndef ARX_GUI_MENU_MENUPAGE_H
#define ARX_GUI_MENU_MENUPAGE_H

#include <memory>

#include "gui/widget/Widget.h"
#include "gui/widget/WidgetContainer.h"
#include "math/Rectangle.h"
#include "math/Vector.h"

class MenuPage {
	
public:
	
	MenuPage(const MenuPage &) = delete;
	MenuPage & operator=(const MenuPage &) = delete;
	
	explicit MenuPage(MENUSTATE id);
	virtual ~MenuPage() = default;
	void update(Vec2f pos);
	void render();
	void drawDebug();
	virtual void focus();
	virtual void init() = 0;
	void activate(Widget * widget);
	void unfocus();
	
	float m_rowSpacing;
	
	MENUSTATE id() const { return m_id; }
	
	void setSize(const Vec2f & size) { m_content = m_rect = Rectf(size.x, size.y); }
	
	auto children() { return m_children.widgets(); }
	
protected:
	
	enum Anchor {
		TopLeft,
		TopCenter,
		TopRight,
		BottomLeft,
		BottomCenter,
		BottomRight
	};
	
	void addCorner(std::unique_ptr<Widget> widget, Anchor anchor);
	
	void addCenter(std::unique_ptr<Widget> widget, bool centerX = true);
	
	void addBackButton(MENUSTATE page);
	
	void reserveTop();
	void reserveBottom();
	
	Vec2f buttonSize(float x, float y) const;
	Vec2f checkboxSize() const;
	Vec2f sliderSize() const;
	
	Rectf m_rect;
	
private:
	
	Rectf m_content;
	
	const MENUSTATE m_id;
	
	bool m_initialized;
	Widget * m_selected;
	Widget * m_focused;
	bool m_disableShortcuts;
	WidgetContainer m_children;
	
};

#endif // ARX_GUI_MENU_MENUPAGE_H
