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

#include "actionstartmove.h"
#include "game/data/units/vehicle.h"
#include "game/data/model.h"
#include "utility/string/toString.h"
#include "../pathcalculator.h"


//------------------------------------------------------------------------------
cActionStartMove::cActionStartMove(cBinaryArchiveOut& archive) :
	cAction(eActiontype::ACTION_START_MOVE)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
cActionStartMove::cActionStartMove(const cVehicle& vehicle, const std::forward_list<cPosition>& path) :
	cAction(eActiontype::ACTION_START_MOVE),
	path(path),
	unitId(vehicle.getId())
{}

//------------------------------------------------------------------------------
void cActionStartMove::execute(cModel& model) const
{
	//Note: this function handles incoming data from network. Make every possible sanity check!

	cVehicle* vehicle = model.getVehicleFromID(unitId);
	if (vehicle == nullptr)
	{
		Log.write(" Can't find vehicle with id " + toString(unitId), cLog::eLOG_TYPE_NET_WARNING);
		return;
	}
	
	if (vehicle->getOwner()->getId() != playerNr) return;
	
	cPosition lastWaypoint = lastWaypoint = vehicle->getPosition();
	for (auto& waypoint : path)
	{
		// one step at a time please...
		if ((waypoint - lastWaypoint).l2NormSquared() > 2)
		{
			return;
		}
		lastWaypoint = waypoint;
	}

	// TODO: is this check really needed?
	if (vehicle->isBeeingAttacked())
	{
		Log.write(" Cannot move a vehicle currently under attack", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	if (vehicle->isAttacking())
	{
		Log.write(" Cannot move a vehicle currently attacking", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	if (vehicle->isUnitBuildingABuilding() || vehicle->BuildPath)
	{
		Log.write(" Cannot move a vehicle currently building", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	if (vehicle->isUnitClearing())
	{
		Log.write(" Cannot move a vehicle currently building", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	if (vehicle->isDisabled())
	{
		Log.write(" Cannot move a vehicle currently disabled", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	if (vehicle->isUnitMoving())
	{
		Log.write(" Cannot move a vehicle already moving", cLog::eLOG_TYPE_NET_DEBUG);
		return;
	}
	


	// everything is ok. add the movejob

	// unset sentry status when moving a vehicle
	if (vehicle->isSentryActive())
	{
		vehicle->getOwner()->deleteSentry(*vehicle);
	}

	model.addMoveJob(*vehicle, path);
}
