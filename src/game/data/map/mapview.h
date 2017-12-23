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

#ifndef game_data_map_mapviewH
#define game_data_map_mapviewH

#include <memory>

#include "utility/signal/signalconnectionmanager.h"
#include "utility/signal/signal.h"

class cMap;
class cPlayer;
class cPosition;
class cUnit;
class cVehicle;
class cMapFieldView;
class cStaticUnitData;
struct sResources;

/**
* This class represents a players view of the map. Access to all not visible information of the
* map is filtered. When nullptr is passed as player, the view can see everything.
*/
class cMapView
{
public:
	cMapView(std::shared_ptr<const cMap> map, std::shared_ptr<const cPlayer> player);

	bool isValidPosition(const cPosition& position) const;
	bool isPositionVisible(const cPosition& position) const;
	bool isWaterOrCoast(const cPosition& position) const;
	bool isWater(const cPosition& position) const;
	bool isCoast(const cPosition& position) const;
	bool isBlocked(const cPosition& position) const;

	bool canSeeUnit(const cUnit& unit) const;
	
	cPosition getSize() const;
	int getOffset(const cPosition& position) const;

	const cMapFieldView getField(const cPosition& position) const;
	const sResources& getResource(const cPosition& position) const;

	bool possiblePlace (const cVehicle& vehicle, const cPosition& position, bool ignoreMovingVehicles = false) const;
	bool possiblePlaceVehicle (const cStaticUnitData& vehicleData, const cPosition& position) const;
	bool possiblePlaceBuilding (const cStaticUnitData& buildingData, const cPosition& position, const cVehicle* vehicle = nullptr) const;

	/** 
	* Triggered when a unit appeared in sight (Scan area of player changed, unit moved into scan area,
	* stealth unit detected, unit unloaded or new unit exited factory).
	*/
	mutable cSignal<void (const cUnit& unit)> unitAppeared;
	/**
	* Triggered when a unit disappeared from the view (Scan area of player changed, unit moved out of scan area,
	* unit loaded or unit destroyed).
	*/
	mutable cSignal<void (const cUnit& unit)> unitDissappeared;
	mutable cSignal<void (const cUnit& unit, const cPosition& oldPosition)> unitMoved;
	mutable cSignal<void ()> scanAreaChanged;
	
private:
	cMapView(const cMapView& other) MAXR_DELETE_FUNCTION;
	cMapView& operator= (const cMapView& other) MAXR_DELETE_FUNCTION;

	std::shared_ptr<const cMap> map;
	std::shared_ptr<const cPlayer> player; // may be null

	cSignalConnectionManager connectionManager;
};


#endif