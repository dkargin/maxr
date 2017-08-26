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

#include "startbuildjob.h"

#include "game/data/units/vehicle.h"
#include "game/logic/movejob.h"
#include "game/data/model.h"


cStartBuildJob::cStartBuildJob (cVehicle& vehicle_, const cPosition& org_, bool big_) :
	cJob (vehicle_),
	org (org_),
	big (big_)
{
	vehicle_.setMovementOffset (cPosition (vehicle_.getPosition().x() < org.x() ? 64 : 0, vehicle_.getPosition().y() < org.y() ? 64 : 0));
}

//------------------------------------------------------------------------------
void cStartBuildJob::run (cModel& model)
{
	assert(unit->isAVehicle());
	cVehicle* vehicle = static_cast<cVehicle*>(unit);

	if (!vehicle->isUnitBuildingABuilding() && !vehicle->isUnitClearing())
	{
		//cancel the job, if the vehicle is not building or clearing!
		finished = true;
		vehicle->setMovementOffset (cPosition (0, 0));
	}

	if (big)
	{
		int deltaX = (vehicle->getPosition().x() < org.x() ? -1 : 1) * MOVE_SPEED;
		int deltaY = (vehicle->getPosition().y() < org.y() ? -1 : 1) * MOVE_SPEED;
		int dir = 0;
		if (deltaX > 0 && deltaY > 0) dir = 3;
		if (deltaX > 0 && deltaY < 0) dir = 1;
		if (deltaX < 0 && deltaY > 0) dir = 5;
		if (deltaX < 0 && deltaY < 0) dir = 7;

		if (vehicle->getMovementOffset().x() == 32)
		{
			if (model.getGameTime() % 10 != 0) return;
			vehicle->rotateTo (0);
			if (vehicle->dir == 0)
			{
				finished = true;
				vehicle->setMovementOffset (cPosition (0, 0));
			}
		}
		else if (vehicle->dir == dir)
		{
			cPosition newOffset (vehicle->getMovementOffset());
			newOffset.x() += deltaX;
			newOffset.y() += deltaY;
			vehicle->setMovementOffset (newOffset);

			if ((vehicle->getMovementOffset().x() > 32 && deltaX > 0) || (vehicle->getMovementOffset().y() < 32 && deltaX < 0))
			{
				vehicle->setMovementOffset (cPosition (32, 32));
			}
		}
		else
		{
			if (model.getGameTime() % 10 != 0) return;
			vehicle->rotateTo (dir);
		}
	}
	else
	{
		if (model.getGameTime() % 10 != 0) return;
		vehicle->rotateTo (0);
		if (vehicle->dir == 0)
		{
			finished = true;
		}
	}
}

//------------------------------------------------------------------------------
eJobType cStartBuildJob::getType() const
{
		return eJobType::START_BUILD;
}

//------------------------------------------------------------------------------
uint32_t cStartBuildJob::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(getType(), crc);
	crc = calcCheckSum(unit ? unit->getId() : 0, crc);
	crc = calcCheckSum(org, crc);
	crc = calcCheckSum(big, crc);

	return crc;
}
