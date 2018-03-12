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

#include "actionfinishbuild.h"

#include "game/data/model.h"

//------------------------------------------------------------------------------
cActionFinishBuild::cActionFinishBuild(const cUnit& unit, const cPosition& escapePosition) :
	cAction(eActiontype::ACTION_FINISH_BUILD),
	unitId(unit.getId()),
	escapePosition(escapePosition)
{};

//------------------------------------------------------------------------------
cActionFinishBuild::cActionFinishBuild(cBinaryArchiveOut& archive)
	: cAction(eActiontype::ACTION_FINISH_BUILD)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionFinishBuild::execute(cModel& model) const
{
	//Note: this function handles incoming data from network. Make every possible sanity check!

	cUnit* unit = model.getUnitFromID(unitId);
	if (unit == nullptr) return;
	if (!unit->getOwner()) return;
	if (unit->getOwner()->getId() != playerNr) return;

	if (unit->isAVehicle())
	{
		finishABuilding(model, *static_cast<cVehicle*>(unit));
	}
	else
	{
		finishAVehicle(model, *static_cast<cBuilding*>(unit));
	}
	return;
}

//------------------------------------------------------------------------------
void cActionFinishBuild::finishABuilding(cModel &model, cVehicle& vehicle) const
{
	auto map = model.getMap();

	int size = 1;
	if (!vehicle.isUnitBuildingABuilding() || vehicle.getBuildTurns() > 0)
		return;
	if (!map->isValidPosition(escapePosition))
		return;
	//if (!vehicle.isNextTo(escapePosition, 1, 1))
	//    return;
	model.sideStepStealthUnit(escapePosition, vehicle);

	if (!map->possiblePlace(vehicle, escapePosition, false))
		return;

	model.addBuilding(vehicle.getPosition(), vehicle.getBuildingType(), vehicle.getOwner());

	// end building
	vehicle.setBuildingABuilding(false);
	vehicle.BuildPath = false;

	// set the vehicle to the border
	if (vehicle.getCellSize() > 1)
	{
		int x = vehicle.getPosition().x();
		int y = vehicle.getPosition().y();

		if (escapePosition.x() > vehicle.getPosition().x())
			x++;
		if (escapePosition.y() > vehicle.getPosition().y())
			y++;

		if(cPlayer* owner = vehicle.getOwner())
			owner->updateScan(vehicle, cPosition(x, y), vehicle.getCellSize());
		map->moveVehicle(vehicle, cPosition(x, y));
	}

	// drive away from the building lot
	model.addMoveJob(vehicle, escapePosition);
}

//------------------------------------------------------------------------------
void cActionFinishBuild::finishAVehicle(cModel &model, cBuilding& building) const
{
	auto map = model.getMap();

	// TODO: Fix it better
	int size = 1;
	if (!map->isValidPosition(escapePosition))
		return;

	if (!building.isNextTo(escapePosition, size, size))
		return;

	if (building.isBuildListEmpty())
		return;
	cBuildListItem& buildingListItem = building.getBuildListItem(0);
	if (buildingListItem.getRemainingMetal() > 0)
		return;

	const cStaticUnitData& unitData = *model.getUnitsData()->getUnit(buildingListItem.getType());

	model.sideStepStealthUnit(escapePosition, unitData, building.getOwner());
	if (!map->possiblePlaceVehicle(unitData, escapePosition, building.getOwner()))
		return;

	model.addVehicle (escapePosition, buildingListItem.getType(), building.getOwner());

	// start new buildjob
	if (building.getRepeatBuild())
	{
		buildingListItem.setRemainingMetal(-1);
		building.addBuildListItem(buildingListItem);
	}
	building.removeBuildListItem(0);

	if (!building.isBuildListEmpty())
	{
		cBuildListItem& buildingListItem = building.getBuildListItem(0);
		if (buildingListItem.getRemainingMetal() == -1)
		{
			std::array<int, 3> turboBuildRounds;
			std::array<int, 3> turboBuildCosts;
			building.calcTurboBuild(turboBuildRounds, turboBuildCosts, building.getOwner()->getUnitDataCurrentVersion(buildingListItem.getType())->getBuildCost());
			buildingListItem.setRemainingMetal(turboBuildCosts[building.getBuildSpeed()]);
		}
		building.startWork();
	}
}
