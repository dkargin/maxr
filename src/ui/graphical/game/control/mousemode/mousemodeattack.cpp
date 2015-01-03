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

#include "ui/graphical/game/control/mousemode/mousemodeattack.h"
#include "ui/graphical/game/control/mouseaction/mouseactionattack.h"
#include "ui/graphical/game/unitselection.h"
#include "game/data/map/map.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/building.h"
#include "input/mouse/mouse.h"
#include "input/mouse/cursor/mousecursorsimple.h"
#include "input/mouse/cursor/mousecursorattack.h"

//------------------------------------------------------------------------------
cMouseModeAttack::cMouseModeAttack (const cMap* map_, const cUnitSelection& unitSelection_, const cPlayer* player_) :
	cMouseMode (map_, unitSelection_, player_)
{
	establishUnitSelectionConnections();
}

//------------------------------------------------------------------------------
eMouseModeType cMouseModeAttack::getType() const
{
	return eMouseModeType::Attack;
}

//------------------------------------------------------------------------------
void cMouseModeAttack::setCursor (cMouse& mouse, const cPosition& mapPosition) const
{
	if (canExecuteAction (mapPosition))
	{
		const auto selectedUnit = unitSelection.getSelectedUnit();

		if (selectedUnit != nullptr && map != nullptr)
		{
			mouse.setCursor (std::make_unique<cMouseCursorAttack> (*selectedUnit, mapPosition, *map));
		}
		else
		{
			mouse.setCursor (std::make_unique<cMouseCursorAttack> ());
		}
	}
	else
	{
		mouse.setCursor (std::make_unique<cMouseCursorSimple> (eMouseCursorSimpleType::No));
	}
}

//------------------------------------------------------------------------------
std::unique_ptr<cMouseAction> cMouseModeAttack::getMouseAction (const cPosition& mapPosition) const
{
	if (canExecuteAction (mapPosition))
	{
		return std::make_unique<cMouseActionAttack> ();
	}
	else return nullptr;
}

//------------------------------------------------------------------------------
bool cMouseModeAttack::canExecuteAction (const cPosition& mapPosition) const
{
	if (!map) return false;

	const auto selectedVehicle = unitSelection.getSelectedVehicle();
	const auto selectedBuilding = unitSelection.getSelectedBuilding();

	return (selectedVehicle && (selectedVehicle->data.muzzleType != sUnitData::MUZZLE_TYPE_TORPEDO || map->isWaterOrCoast (mapPosition))) ||
		   (selectedBuilding && selectedBuilding->isInRange (mapPosition));
}

//------------------------------------------------------------------------------
void cMouseModeAttack::establishUnitSelectionConnections()
{
	const auto selectedUnit = unitSelection.getSelectedUnit();

	if (selectedUnit)
	{
		selectedUnitSignalConnectionManager.connect (selectedUnit->data.rangeChanged, [this]() { needRefresh(); });
		selectedUnitSignalConnectionManager.connect (selectedUnit->data.damageChanged, [this]() { needRefresh(); });
		selectedUnitSignalConnectionManager.connect (selectedUnit->disabledChanged, [this]() { needRefresh(); });
		selectedUnitSignalConnectionManager.connect (selectedUnit->positionChanged, [this]() { needRefresh(); });
	}
}

//------------------------------------------------------------------------------
void cMouseModeAttack::establishMapFieldConnections (const cMapField& field)
{
	mapFieldSignalConnectionManager.connect (field.unitsChanged, [this, &field]() { updateFieldUnitConnections (field); needRefresh(); });

	updateFieldUnitConnections (field);
}

//------------------------------------------------------------------------------
void cMouseModeAttack::updateFieldUnitConnections (const cMapField& field)
{
	mapFieldUnitsSignalConnectionManager.disconnectAll();

	auto plane = field.getPlane();
	if (plane)
	{
		mapFieldUnitsSignalConnectionManager.connect (plane->flightHeightChanged, [this]() { needRefresh(); });
		mapFieldUnitsSignalConnectionManager.connect (plane->data.hitpointsChanged, [this]() { needRefresh(); });
	}
	auto vehicle = field.getVehicle();
	if (vehicle)
	{
		mapFieldUnitsSignalConnectionManager.connect (vehicle->data.hitpointsChanged, [this]() { needRefresh(); });
	}
	auto building = field.getBuilding();
	if (building)
	{
		mapFieldUnitsSignalConnectionManager.connect (building->data.hitpointsChanged, [this]() { needRefresh(); });
	}
}
