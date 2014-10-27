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

#ifndef ui_graphical_game_widgets_unitdetailshudH
#define ui_graphical_game_widgets_unitdetailshudH

#include <array>

#include "ui/graphical/widget.h"

#include "ui/graphical/menu/widgets/special/unitdatasymboltype.h"

class cLabel;
class cUnit;
class cPlayer;
class cGameSettings;

class cUnitDetailsHud : public cWidget
{
public:
	explicit cUnitDetailsHud (const cBox<cPosition>& area, bool drawLines = false);

	virtual void draw (SDL_Surface& destination, const cBox<cPosition>& clipRect) MAXR_OVERRIDE_FUNCTION;;

	void setUnit (const cUnit* unit);

	void setPlayer (const cPlayer* player);

	void setGameSettings (std::shared_ptr<const cGameSettings> gameSettings);

	// TODO: find nice place for these functions
	static void drawSmallSymbols (SDL_Surface* destination, int rowHeight, eUnitDataSymbolType symbolType, const cPosition& position, int value1, int value2);
	static cBox<cPosition> getSmallSymbolPosition (eUnitDataSymbolType symbolType);
private:
	AutoSurface surface;

	void reset ();

	void drawRow (size_t index, eUnitDataSymbolType symbolType, int amount, int maximalAmount, const std::string& name);

	static const size_t maxRows = 4;
	static const int rowHeight = 12;

	std::array<cLabel*, maxRows> amountLabels;
	std::array<cLabel*, maxRows> nameLabels;

	bool drawLines;

	const cUnit* unit;
	const cPlayer* player;
	std::shared_ptr<const cGameSettings> gameSettings;
};

#endif // ui_graphical_game_widgets_unitdetailshudH
