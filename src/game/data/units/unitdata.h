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

#ifndef game_data_units_unitdataH
#define game_data_units_unitdataH

#include <string>
#include <utility>
#include <vector>

#include "game/data/resourcetype.h"

#include "utility/signal/signal.h"
#include "utility/serialization/serialization.h"
#include "utility/drawing.h"
#include "sound.h"

#include "unit.h"
#include "vehicle.h"
#include "building.h"

class cClanData;

// Database for all the units
class cUnitsData
{
public:
	typedef sID UID;
	typedef std::map<UID, cDynamicUnitData> DynamicUnitDataStorage;

	cUnitsData();

	// direct pointer on some of the building graphics
	SDL_Surface* ptr_connector;
	SDL_Surface* ptr_connector_org;
	SDL_Surface* ptr_connector_shw;
	SDL_Surface* ptr_connector_shw_org;

	void initializeIDData();
	void initializeClanUnitData(const cClanData& clanData);

	// Generate or obtain dynamic unit data for specified id
	cDynamicUnitData& getDynamicData(const std::string& id);
	// Generate or obtain static unit data for specified id
	cStaticUnitDataPtr getUnit(const std::string& id);

	// Get vehicle data for specified id
	// Can return empty pointer if no object is found
	std::shared_ptr<cVehicleData> getVehicle(const UID& id) const;

	// Get vehicle data for specified id
	// Can return empty pointer if no object is found
	std::shared_ptr<cBuildingData> getBuilding(const UID& id) const;

	// Get vehicle data for specified id
	// Will create a new object, if there is no such one already
	std::shared_ptr<cVehicleData> makeVehicle(const UID& id);

	// Get vehicle data for specified id
	// Will create a new object, if there is no such one already
	std::shared_ptr<cBuildingData> makeBuilding(const UID& id);

	// Get vehicle data for specified id
	// Can return empty pointer if no object is found
	cStaticUnitDataPtr getUnit(const UID& id) const;

	bool isValidId(const UID& id) const;

	size_t getNrOfClans() const;

	// clan = -1: without clans
	const cDynamicUnitData& getDynamicData(const sID& id, int clan = -1) const;
	cDynamicUnitData& getDynamicData(const sID& id, int clan = -1);
	// clan = -1: without clans
	const DynamicUnitDataStorage& getAllDynamicData(int clan = -1) const;

	std::vector<cStaticUnitDataPtr> getAllUnits() const;
	std::vector<cStaticUnitDataPtr> getUnitsOfType(UnitType type) const;
	std::vector<std::shared_ptr<cVehicleData>> getAllVehicles() const;
	std::vector<std::shared_ptr<cBuildingData>> getAllBuildings() const;

	uint32_t getChecksum(uint32_t crc) const;

	template <typename T> void save(T& archive)
	{
		try
		{
			auto buildings = getAllBuildings();
			auto vehicles = getAllVehicles();

			int vehicleNum = vehicles.size();
			archive & NVP(vehicleNum);
			for (auto vehicle : vehicles)
			{
				archive & serialization::makeNvp("vehicleID", vehicle->ID);
				archive & serialization::makeNvp("vehicle", *vehicle);
			}

			int buildingNum = buildings.size();

			archive & NVP(buildingNum);
			for (auto building : buildings)
			{
				archive & serialization::makeNvp("buildingID", building->ID);
				archive & serialization::makeNvp("building", *building);
			}
		}
		catch(std::runtime_error& e)
		{
			printf("Error saving cUnitData: %s\n", e.what());
		}

		archive & NVP(dynamicUnitData);
		archive & NVP(clanDynamicUnitData);
	}

	template <typename T> void load(T& archive)
	{
		//staticUnitData.clear();
		dynamicUnitData.clear();
		clanDynamicUnitData.clear();
		crcValid = false;
		try
		{
			int vehicleNum = 0;
			archive & NVP(vehicleNum);
			for(int i = 0; i < vehicleNum; i++)
			{
				sID vehicleID;
				archive & NVP(vehicleID);
				auto& vehicle = *makeVehicle(vehicleID);
				archive & NVP(vehicle);
			}

			int buildingNum = 0;
			archive & NVP(buildingNum);
			for(int i = 0; i < buildingNum; i++)
			{
				sID buildingID;
				archive & NVP(buildingID);
				auto& building = *makeBuilding(buildingID);
				archive & NVP(building);
			}
		}catch(std::runtime_error& e) {
			printf("Error loading cUnitData: %s\n", e.what());
		}
		archive & NVP(dynamicUnitData);
		archive & NVP(clanDynamicUnitData);
	}

	SERIALIZATION_SPLIT_MEMBER()
private:
	sID specialIDLandMine;
	sID specialIDSeaMine;
	sID specialIDConnector;

	bool registerBuilding(const std::shared_ptr<cBuildingData>& building);
	bool registerVehicle(const std::shared_ptr<cVehicleData>& building);

	// the dynamic unit data. Contains the modified versions for the clans
	// 'Clan' should be replaced in this context on 'Player'
	// 'Clan' stuff is relevant during embarcation stage and calculation of final unit stats.
	// After player has embarked - no need to keep 'clan' stuff
	std::vector<DynamicUnitDataStorage> clanDynamicUnitData;

	//cStaticUnitData rubbleSmall;
	//cStaticUnitData rubbleBig;

	// the static unit data
	std::map<UID, cStaticUnitDataPtr> staticUnitData;
	// the dynamic unit data. Standard version without clan modifications
	DynamicUnitDataStorage dynamicUnitData;

	// unitdata does not change during the game.
	// So caching the checksum saves a lot cpu ressources.
	mutable uint32_t crcCache;
	mutable bool crcValid;
};

#endif // game_data_units_unitdataH
