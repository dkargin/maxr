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

#include "ui/graphical/game/control/mousemode/mousemodesteal.h"
#include "ui/graphical/game/control/mouseaction/mouseactionsteal.h"
#include "ui/graphical/game/unitselection.h"
#include "game/data/map/map.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/building.h"
#include "input/mouse/mouse.h"
#include "input/mouse/cursor/mousecursorsimple.h"
#include "input/mouse/cursor/mousecursoramount.h"

//------------------------------------------------------------------------------
cMouseModeSteal::cMouseModeSteal (const cMap* map_, const cUnitSelection& unitSelection_, const cPlayer* player_) :
	cMouseMode (map_, unitSelection_, player_)
{
	establishUnitSelectionConnections();
}

//------------------------------------------------------------------------------
eMouseModeType cMouseModeSteal::getType() const
{
	return eMouseModeType::Steal;
}

//------------------------------------------------------------------------------
void cMouseModeSteal::setCursor (cMouse& mouse, const cPosition& mapPosition) const
{
	if (canExecuteAction (mapPosition))
	{
		const auto selectedVehicle = unitSelection.getSelectedVehicle();

		if (selectedVehicle && map)
		{
			const auto& field = map->getField (mapPosition);
			const cUnit* unit = field.getVehicle();

			mouse.setCursor (std::make_unique<cMouseCursorAmount> (eMouseCursorAmountType::Steal, selectedVehicle->calcCommandoChance (unit, true)));
		}
		else mouse.setCursor (std::make_unique<cMouseCursorAmount> (eMouseCursorAmountType::Steal));
	}
	else
	{
		mouse.setCursor (std::make_unique<cMouseCursorSimple> (eMouseCursorSimpleType::No));
	}
}

//------------------------------------------------------------------------------
std::unique_ptr<cMouseAction> cMouseModeSteal::getMouseAction (const cPosition& mapPosition) const
{
	if (canExecuteAction (mapPosition))
	{
		return std::make_unique<cMouseActionSteal> ();
	}
	else return nullptr;
}

//------------------------------------------------------------------------------
bool cMouseModeSteal::canExecuteAction (const cPosition& mapPosition) const
{
	if (!map) return false;

	const auto selectedVehicle = unitSelection.getSelectedVehicle();

	return selectedVehicle && selectedVehicle->canDoCommandoAction (mapPosition, *map, true);
}

//------------------------------------------------------------------------------
void cMouseModeSteal::establishUnitSelectionConnections()
{
	const auto selectedUnit = unitSelection.getSelectedUnit();

	if (selectedUnit)
	{
		selectedUnitSignalConnectionManager.connect (selectedUnit->data.shotsChanged, [this]() { needRefresh(); });
		selectedUnitSignalConnectionManager.connect (selectedUnit->positionChanged, [this]() { needRefresh(); });
	}
}

//------------------------------------------------------------------------------
void cMouseModeSteal::establishMapFieldConnections (const cMapField& field)
{
	mapFieldSignalConnectionManager.connect (field.unitsChanged, [this, &field]() { updateFieldUnitConnections (field); needRefresh(); });

	updateFieldUnitConnections (field);
}

//------------------------------------------------------------------------------
void cMouseModeSteal::updateFieldUnitConnections (const cMapField& field)
{
	mapFieldUnitsSignalConnectionManager.disconnectAll();

	auto plane = field.getPlane();
	if (plane)
	{
		mapFieldUnitsSignalConnectionManager.connect (plane->flightHeightChanged, [this]() { needRefresh(); });
		mapFieldUnitsSignalConnectionManager.connect (plane->disabledChanged, [this]() { needRefresh(); });
		mapFieldUnitsSignalConnectionManager.connect (plane->data.storedUnitsChanged, [this]() { needRefresh(); });
	}
	auto vehicle = field.getVehicle();
	if (vehicle)
	{
		mapFieldUnitsSignalConnectionManager.connect (vehicle->data.storedUnitsChanged, [this]() { needRefresh(); });
		mapFieldUnitsSignalConnectionManager.connect (vehicle->disabledChanged, [this]() { needRefresh(); });
	}
	auto building = field.getBuilding();
	if (building)
	{
		mapFieldUnitsSignalConnectionManager.connect (building->data.storedUnitsChanged, [this]() { needRefresh(); });
		mapFieldUnitsSignalConnectionManager.connect (building->disabledChanged, [this]() { needRefresh(); });
	}
}
