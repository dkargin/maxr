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

#include "ui/graphical/menu/widgets/special/unitlistviewitembuy.h"
#include "ui/graphical/menu/widgets/image.h"
#include "ui/graphical/menu/widgets/label.h"
#include "game/data/player/player.h"

//------------------------------------------------------------------------------
cUnitListViewItemBuy::cUnitListViewItemBuy (unsigned int width, const sID& unitId, const cPlayer& owner, const cUnitsData& unitsData) :
	cUnitListViewItem (width, unitId, owner, unitsData)
{
	auto costLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (cPosition (width - 15, 0), cPosition (width, unitImage->getEndPosition().y())), iToStr (owner.getUnitDataCurrentVersion (unitId)->getBuildCost()), FONT_LATIN_SMALL_YELLOW, toEnumFlag (eAlignmentType::Right) | eAlignmentType::CenterVerical));
	costLabel->setConsumeClick (false);

	nameLabel->resize (nameLabel->getSize() - cPosition (15, 0));
}

//------------------------------------------------------------------------------
void cUnitListViewItemBuy::markAsInsufficient()
{
	nameLabel->setFont (FONT_LATIN_SMALL_RED);
}

//------------------------------------------------------------------------------
void cUnitListViewItemBuy::unmarkAsInsufficient()
{
	nameLabel->setFont (FONT_LATIN_SMALL_WHITE);
}
