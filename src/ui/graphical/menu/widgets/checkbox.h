/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef ui_graphical_menu_widgets_checkboxH
#define ui_graphical_menu_widgets_checkboxH

#include "ui/graphical/menu/widgets/clickablewidget.h"

#include "utility/autosurface.h"
#include "unifonts.h"
#include "sound.h"
#include "utility/signal/signal.h"

enum class eCheckBoxType
{
	TextOnly,
	Round,
	Angular,

	Standard,

	Tank,
	Plane,
	Ship,
	Building,
	Tnt,

	HudIndex_00,
	HudIndex_01,
	HudIndex_02,
	HudIndex_10,
	HudIndex_11,
	HudIndex_12,
	HudIndex_20,
	HudIndex_21,
	HudIndex_22,

	HudChat,
	HudLock,

	HudTnt,
	Hud2x,
	HudPlayers,

	UnitContextMenu,

	ArrowDownSmall
};

enum eCheckBoxTextAnchor
{
	Left,
	Right
};

class cCheckBox : public cClickableWidget
{
public:
	explicit cCheckBox (const cPosition& position, eCheckBoxType type = eCheckBoxType::Standard, bool centered = false, cSoundChunk* clickSound = &SoundData.SNDObjectMenu);
	cCheckBox (const cPosition& position, const std::string& text, eUnicodeFontType fontType = FONT_LATIN_NORMAL, eCheckBoxTextAnchor textAnchor = eCheckBoxTextAnchor::Right, eCheckBoxType type = eCheckBoxType::Standard, bool centered = false, cSoundChunk* clickSound = &SoundData.SNDObjectMenu);

	void setChecked (bool checked);
	bool isChecked() const;

	void toggle();

	void lock();
	void unlock();

	cSignal<void ()> toggled;

	virtual void draw (SDL_Surface& destination, const cBox<cPosition>& clipRect) MAXR_OVERRIDE_FUNCTION;

	virtual bool handleMousePressed (cApplication& application, cMouse& mouse, eMouseButtonType button) MAXR_OVERRIDE_FUNCTION;
	virtual bool handleMouseReleased (cApplication& application, cMouse& mouse, eMouseButtonType button) MAXR_OVERRIDE_FUNCTION;
protected:
	virtual void setPressed (bool pressed) MAXR_OVERRIDE_FUNCTION;

	virtual bool handleClicked (cApplication& application, cMouse& mouse, eMouseButtonType button) MAXR_OVERRIDE_FUNCTION;
private:
	AutoSurface surface;

	eCheckBoxType type;

	std::string text;
	eUnicodeFontType fontType;
	eCheckBoxTextAnchor textAnchor;
	int textLimitWidth;

	cSoundChunk* clickSound;

	bool checked;

	bool isLocked;


	void renewSurface();
};

#endif // ui_graphical_menu_widgets_checkboxH
