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

#include "ui/graphical/game/widgets/mouseaction/mouseactionsupplyammo.h"
#include "ui/graphical/game/widgets/gamemapwidget.h"
#include "ui/graphical/game/unitselection.h"
#include "game/data/map/map.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/building.h"
#include "input/mouse/mouse.h"
#include "input/mouse/cursor/mousecursorsimple.h"

//------------------------------------------------------------------------------
bool cMouseActionSupplyAmmo::executeLeftClick (cGameMapWidget& gameMapWidget, const cMap& map, const cPosition& mapPosition, cUnitSelection& unitSelection, bool changeAllowed) const
{
	const auto selectedVehicle = unitSelection.getSelectedVehicle ();

	if (!selectedVehicle) return false;

	const auto& field = map.getField (mapPosition);

	const auto overVehicle = field.getVehicle ();
	const auto overPlane = field.getPlane ();
	const auto overBuilding = field.getBuilding ();

	if (overVehicle)
	{
		gameMapWidget.triggeredSupplyAmmo (*selectedVehicle, *overVehicle);
	}
	else if (overPlane && overPlane->getFlightHeight () == 0)
	{
		gameMapWidget.triggeredSupplyAmmo (*selectedVehicle, *overPlane);
	}
	else if (overBuilding)
	{
		gameMapWidget.triggeredSupplyAmmo (*selectedVehicle, *overBuilding);
	}
	else
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
bool cMouseActionSupplyAmmo::doesChangeState () const
{
	return true;
}

//------------------------------------------------------------------------------
bool cMouseActionSupplyAmmo::isSingleAction () const
{
	return false;
}
