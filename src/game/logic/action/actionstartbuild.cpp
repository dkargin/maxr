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

#include "actionstartbuild.h"

#include "game/data/model.h"
#include "game/logic/jobs/startbuildjob.h"


cActionStartBuild::cActionStartBuild(const cVehicle& vehicle, sID buildingTypeID, int buildSpeed, const cPosition& buildPosition) :
	cAction(eActiontype::ACTION_START_BUILD),
	vehicleID(vehicle.getId()),
	buildingTypeID(buildingTypeID),
	buildSpeed(buildSpeed),
	buildPosition(buildPosition),
	buildPath(false)
{}

//------------------------------------------------------------------------------
cActionStartBuild::cActionStartBuild(const cVehicle& vehicle, sID buildingTypeID, int buildSpeed, const cPosition& buildPosition, const cPosition& pathEndPosition) :
	cAction(eActiontype::ACTION_START_BUILD),
	vehicleID(vehicle.getId()),
	buildingTypeID(buildingTypeID),
	buildSpeed(buildSpeed),
	buildPosition(buildPosition),
	buildPath(true),
	pathEndPosition(pathEndPosition)
{}


//------------------------------------------------------------------------------
cActionStartBuild::cActionStartBuild(cBinaryArchiveOut& archive) :
	cAction(eActiontype::ACTION_START_BUILD)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionStartBuild::execute(cModel& model) const
{
	//Note: this function handles incoming data from network. Make every possible sanity check!

	cMap& map = *model.getMap();

	cVehicle* vehicle = model.getVehicleFromID(vehicleID);
	if (vehicle == nullptr) return;
	if (vehicle->getOwner()->getId() != playerNr) return;

	if (!model.getUnitsData()->isValidId(buildingTypeID)) return;
	if (!buildingTypeID.isABuilding()) return;
	if (!map.isValidPosition(buildPosition)) return;
	if (buildPath && !map.isValidPosition(pathEndPosition)) return;
	if (buildSpeed > 2 || buildSpeed < 0) return;
	
	if (vehicle->isUnitBuildingABuilding() || vehicle->BuildPath) return;
	if (vehicle->isDisabled()) return;

	const cStaticUnitData& data = model.getUnitsData()->getStaticUnitData(buildingTypeID);

	if (vehicle->getStaticUnitData().canBuild != data.buildAs) return;

	std::array<int, 3> turboBuildRounds;
	std::array<int, 3> turboBuildCosts;
	int buildcost = vehicle->getOwner()->getUnitDataCurrentVersion(buildingTypeID)->getBuildCost();
	vehicle->calcTurboBuild(turboBuildRounds, turboBuildCosts, buildcost);

	if (turboBuildCosts[buildSpeed] > vehicle->getStoredResources() ||
		turboBuildRounds[buildSpeed] <= 0)
	{
		//TODO: notify GUI -> not enough material
		// activePlayer->addSavedReport (std::make_unique<cSavedReportSimple> (eSavedReportType::ProducingError));
		return;
	}

	cPosition oldPosition = vehicle->getPosition();
	if (data.isBig)
	{
// 		model.sideStepStealthUnit(buildPosition, *vehicle, buildPosition);
// 		model.sideStepStealthUnit(buildPosition + cPosition(1, 0), *vehicle, buildPosition);
// 		model.sideStepStealthUnit(buildPosition + cPosition(0, 1), *vehicle, buildPosition);
// 		model.sideStepStealthUnit(buildPosition + cPosition(1, 1), *vehicle, buildPosition);

		if (!(map.possiblePlaceBuilding(data, buildPosition, vehicle) &&
			map.possiblePlaceBuilding(data, buildPosition + cPosition(1, 0), vehicle) &&
			map.possiblePlaceBuilding(data, buildPosition + cPosition(0, 1), vehicle) &&
			map.possiblePlaceBuilding(data, buildPosition + cPosition(1, 1), vehicle)))
		{
			//TODO: notify GUI -> blocked position
			return;
		}
		vehicle->buildBigSavedPosition = vehicle->getPosition();

		// set vehicle to build position
		map.moveVehicleBig(*vehicle, buildPosition);
		vehicle->getOwner()->doScan();
	}
	else
	{
		if (buildPosition != vehicle->getPosition()) return;

		if (!map.possiblePlaceBuilding(data, buildPosition, vehicle))
		{
			//TODO: notify GUI -> blocked position
			return;
		}
	}

	vehicle->setBuildingType(buildingTypeID);
	vehicle->bandPosition = pathEndPosition;

	vehicle->setBuildCosts(turboBuildCosts[buildSpeed]);
	vehicle->setBuildTurns(turboBuildRounds[buildSpeed]);
	vehicle->setBuildCostsStart(vehicle->getBuildCosts());
	vehicle->setBuildTurnsStart(vehicle->getBuildTurns());

	vehicle->setBuildingABuilding(true);
	vehicle->BuildPath = buildPath;

	model.addJob (new cStartBuildJob (*vehicle, oldPosition, data.isBig));

	if (vehicle->getMoveJob()) vehicle->getMoveJob()->stop();
}
