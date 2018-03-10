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
#include <cmath>

#include "game/data/player/player.h"

#include "game/data/units/building.h"
#include "game/logic/client.h"
#include "utility/listhelpers.h"
#include "netmessage.h"
#include "game/logic/server.h"
#include "game/logic/serverevents.h"
#include "game/data/units/vehicle.h"
#include "game/data/report/savedreport.h"
#include "game/logic/turncounter.h"
#include "utility/crc.h"

using namespace std;

//------------------------------------------------------------------------------
// Implementation cPlayer class
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
cPlayer::cPlayer (const cPlayerBasicData& splayer_, const cUnitsData& unitsData) :
	splayer (splayer_),
	numEcos (0),
	clan (-1),
	hasFinishedTurn (false),
	isRemovedFromGame (false),
	base(*this)
{
	// get the default (no clan) unit data
	dynamicUnitsData = unitsData.getDynamicUnitsData(-1);

	researchCentersWorkingTotal = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = 0;
	credits = 0;

	isDefeated = false;

	splayer.nameChanged.connect ([this]() { nameChanged(); });
	splayer.colorChanged.connect ([this]() { colorChanged(); });
}

//------------------------------------------------------------------------------
cPlayer::~cPlayer()
{}

//------------------------------------------------------------------------------
void cPlayer::setClan (int newClan, const cUnitsData& unitsData)
{
	if (newClan < -1)
		return;
	if (newClan > 0 && static_cast<size_t>(newClan) >= unitsData.getNrOfClans())
		return;

	clan = newClan;

	dynamicUnitsData = unitsData.getDynamicUnitsData(clan);
}

//------------------------------------------------------------------------------
int cPlayer::getCredits() const
{
	return credits;
}

//------------------------------------------------------------------------------
void cPlayer::setCredits (int credits_)
{
	std::swap (credits, credits_);
	if (credits != credits_) creditsChanged();
}

//------------------------------------------------------------------------------
cDynamicUnitData* cPlayer::getUnitDataCurrentVersion (const sID& id)
{
	const cPlayer* constMe = this;
	return const_cast<cDynamicUnitData*> (constMe->getUnitDataCurrentVersion (id));
}

//------------------------------------------------------------------------------
const cDynamicUnitData* cPlayer::getUnitDataCurrentVersion(const sID& id) const
{
	for (const auto &data : dynamicUnitsData)
	{
		if (data.getId() == id) return &data;
	}
	return nullptr;
}

//------------------------------------------------------------------------------
/** initialize the maps */
//------------------------------------------------------------------------------
void cPlayer::initMaps (cMap& map)
{
	mapSize = map.getSize();
	const int size = mapSize.x() * mapSize.y();
	
	resourceMap.resize (size, 0);
	sentriesMapAir.resize (mapSize);
	sentriesMapGround.resize (mapSize);

	scanMap.resize (mapSize);
	detectLandMap.resize (mapSize);
	detectSeaMap.resize (mapSize);
	detectMinesMap.resize (mapSize);
}

const cPosition& cPlayer::getMapSize() const
{
	return mapSize;
}

//------------------------------------------------------------------------------
void cPlayer::addToScan(const cUnit& unit)
{
	assert(!unit.isDisabled());

	const int size = unit.getIsBig() ? 2 : 1;

	scanMap.add(unit.data.getScan(), unit.getPosition(), size);
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_GROUND)
	{
		detectLandMap.add(unit.data.getScan(), unit.getPosition(), size);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_SEA)
	{
		detectSeaMap.add(unit.data.getScan(), unit.getPosition(), size);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & AREA_EXP_MINE)
	{
		// mines can only be detected on directly adjacent fields
		detectMinesMap.add(1, unit.getPosition(), size, true);
	}
}

//------------------------------------------------------------------------------
void cPlayer::updateScan(const cUnit& unit, const cPosition& newPosition, bool newIsBig)
{
	assert(!unit.isDisabled());

	const int oldSize = unit.getIsBig() ? 2 : 1;
	const int newSize = newIsBig ? 2 : 1;
	scanMap.update(unit.data.getScan(), unit.getPosition(), newPosition, oldSize, newSize);
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_GROUND)
	{
		detectLandMap.update(unit.data.getScan(), unit.getPosition(), newPosition, oldSize, newSize);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_SEA)
	{
		detectSeaMap.update(unit.data.getScan(), unit.getPosition(), newPosition, oldSize, newSize);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & AREA_EXP_MINE)
	{
		// mines can only be detected on directly adjacent fields
		detectMinesMap.update(1, unit.getPosition(), newPosition, oldSize, newSize, true);
	}
}

//------------------------------------------------------------------------------
void cPlayer::updateScan(const cUnit& unit, int newScanRange)
{
	assert(!unit.isDisabled());

	const int size = unit.getIsBig() ? 2 : 1;
	scanMap.update(unit.data.getScan(), newScanRange, unit.getPosition(), size);
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_GROUND)
	{
		detectLandMap.update(unit.data.getScan(), newScanRange, unit.getPosition(), size);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_SEA)
	{
		detectSeaMap.update(unit.data.getScan(), newScanRange, unit.getPosition(), size);
	}
	// mine detection does not change
}

//------------------------------------------------------------------------------
void cPlayer::removeFromScan(const cUnit& unit)
{
	const int size = unit.getIsBig() ? 2 : 1;

	scanMap.remove(unit.data.getScan(), unit.getPosition(), size);
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_GROUND)
	{
		detectLandMap.remove(unit.data.getScan(), unit.getPosition(), size);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & TERRAIN_SEA)
	{
		detectSeaMap.remove(unit.data.getScan(), unit.getPosition(), size);
	}
	if (unit.getStaticUnitData().canDetectStealthOn & AREA_EXP_MINE)
	{
		// mines can only be detected on directly adjacent fields
		detectMinesMap.remove(1, unit.getPosition(), size, true);
	}
}

//------------------------------------------------------------------------------
const cRangeMap& cPlayer::getScanMap() const
{
	return scanMap;
}

//------------------------------------------------------------------------------
cVehicle& cPlayer::addNewVehicle (const cPosition& position, const cStaticUnitData& unitData, unsigned int uid)
{
	auto vehicle = std::make_shared<cVehicle> (unitData, *getUnitDataCurrentVersion(unitData.ID), this, uid);
	vehicle->setPosition (position);

	auto result = vehicles.insert (std::move (vehicle));
	assert (result.second);

	return * (*result.first);
}

//------------------------------------------------------------------------------
cBuilding& cPlayer::addNewBuilding (const cPosition& position, const cStaticUnitData& unitData, unsigned int uid)
{
	auto building = std::make_shared<cBuilding>(&unitData, getUnitDataCurrentVersion(unitData.ID), this, uid);
	building->setPosition (position);

	auto result = buildings.insert (std::move (building));
	assert (result.second);
	return * (*result.first);
}

//------------------------------------------------------------------------------
void cPlayer::addUnit (std::shared_ptr<cVehicle> vehicle)
{
	vehicles.insert (std::move (vehicle));
}

//------------------------------------------------------------------------------
void cPlayer::addUnit (std::shared_ptr<cBuilding> building)
{
	buildings.insert (std::move (building));
}

//------------------------------------------------------------------------------
std::shared_ptr<cBuilding> cPlayer::removeUnit (const cBuilding& building)
{
	auto iter = buildings.find (building);
	if (iter == buildings.end()) return nullptr;

	auto removed = *iter;
	buildings.erase (iter);
	return removed;
}

//------------------------------------------------------------------------------
std::shared_ptr<cVehicle> cPlayer::removeUnit (const cVehicle& vehicle)
{
	auto iter = vehicles.find (vehicle);
	if (iter == vehicles.end()) return nullptr;

	auto removed = *iter;
	vehicles.erase (iter);
	return removed;
}

//------------------------------------------------------------------------------
void cPlayer::removeAllUnits()
{
	vehicles.clear();
	buildings.clear();
}

//------------------------------------------------------------------------------
cVehicle* cPlayer::getVehicleFromId (unsigned int id) const
{
	auto iter =  vehicles.find (id);
	return iter == vehicles.end() ? nullptr : (*iter).get();
}

//------------------------------------------------------------------------------
cBuilding* cPlayer::getBuildingFromId (unsigned int id) const
{
	auto iter = buildings.find (id);
	return iter == buildings.end() ? nullptr : (*iter).get();
}

//------------------------------------------------------------------------------
const cFlatSet<std::shared_ptr<cVehicle>, sUnitLess<cVehicle>>& cPlayer::getVehicles() const
{
	return vehicles;
}

//------------------------------------------------------------------------------
const cFlatSet<std::shared_ptr<cBuilding>, sUnitLess<cBuilding>>& cPlayer::getBuildings() const
{
	return buildings;
}

//------------------------------------------------------------------------------
void cPlayer::addSentry (cUnit& u)
{
	if (u.isSentryActive()) return;

	u.setSentryActive (true);

	const int size = u.getIsBig() ? 2 : 1;
	if (u.getStaticUnitData().canAttack & TERRAIN_AIR)
	{
		sentriesMapAir.add(u.data.getRange(), u.getPosition(), size);
	}
	if ((u.getStaticUnitData().canAttack & TERRAIN_GROUND) || (u.getStaticUnitData().canAttack & TERRAIN_SEA))
	{
		sentriesMapGround.add(u.data.getRange(), u.getPosition(), size);
	}
}

//------------------------------------------------------------------------------
void cPlayer::deleteSentry (cUnit& u)
{
	if (!u.isSentryActive()) return;

	u.setSentryActive (false);

	const int size = u.getIsBig() ? 2 : 1;
	if (u.getStaticUnitData().canAttack & TERRAIN_AIR)
	{
		sentriesMapGround.remove(u.data.getRange(), u.getPosition(), size);
	}
	else if ((u.getStaticUnitData().canAttack & TERRAIN_GROUND) || (u.getStaticUnitData().canAttack & TERRAIN_SEA))
	{
		sentriesMapAir.remove(u.data.getRange(), u.getPosition(), size);
	}
}

//------------------------------------------------------------------------------
void cPlayer::refreshSentryMaps()
{
	//1 save original maps
	const auto sentriesMapAirCopy = sentriesMapAir.getMap();
	const auto sentriesMapGroundCopy = sentriesMapGround.getMap();

	//2 add all units (yes they are on the scan maps twice now)
	for (const auto& unit : vehicles)
	{
		if (unit->isSentryActive())
		{
			addSentry(*unit);
		}
	}

	for (const auto& unit : buildings)
	{
		if (unit->isSentryActive())
		{
			addSentry(*unit);
		}
	}

	//3 subtract values from the saved maps
	sentriesMapAir.subtract(sentriesMapAirCopy);
	sentriesMapGround.subtract(sentriesMapGroundCopy);
}

//------------------------------------------------------------------------------
void cPlayer::refreshScanMaps()
{
	// simply clear the maps and re-add all units would lead to a lot of
	// false triggered events (like unit detected, etc.).
	// so we use a special method, that only triggers signals on fields that
	// really changed during the refresh.

	//1 save original maps
	const auto scanMapCopy = scanMap.getMap();
	const auto detectLandMapCopy = detectLandMap.getMap();
	const auto detectSeaMapCopy = detectSeaMap.getMap();
	const auto detectMinesMapCopy = detectMinesMap.getMap();
	
	//2 add all units (yes they are on the scan maps twice now)
	for (const auto& vehicle : vehicles)
	{
		if (vehicle->isUnitLoaded()) continue;
		addToScan(*vehicle);
	}

	for (const auto& building : buildings)
	{
		addToScan(*building);
	}

	//3 subtract values from the saved maps
	scanMap.subtract(scanMapCopy);
	detectLandMap.subtract(detectLandMapCopy);
	detectSeaMap.subtract(detectSeaMapCopy);
	detectMinesMap.subtract(detectMinesMapCopy);
}

//------------------------------------------------------------------------------
void cPlayer::revealResource()
{
	resourceMap.fill(1);
}

//------------------------------------------------------------------------------
bool cPlayer::canSeeAnyAreaUnder (const cUnit& unit) const
{
	if (canSeeAt (unit.getPosition())) return true;
	if (!unit.getIsBig()) return false;

	return canSeeAt (unit.getPosition() + cPosition (0, 1)) ||
		   canSeeAt (unit.getPosition() + cPosition (1, 1)) ||
		   canSeeAt (unit.getPosition() + cPosition (1, 0));
}

//------------------------------------------------------------------------------
bool cPlayer::canSeeUnit(const cUnit& unit, const cMap& map) const
{
	const auto& position = unit.getPosition();
	return canSeeUnit(unit, map.getField(position), map.staticMap->getTerrain(position));
}

//------------------------------------------------------------------------------
bool cPlayer::canSeeUnit(const cUnit& unit, const cMapField& field, const sTerrain& terrain) const
{
	if (unit.isAVehicle() && static_cast<const cVehicle*>(&unit)->isUnitLoaded()) return false;

	if (unit.getOwner() == this) return true;

	if (!canSeeAnyAreaUnder(unit)) return false;
	
	if (!unit.isStealthOnCurrentTerrain(field, terrain)) return true;

	return unit.isDetectedByPlayer(this);
}

//--------------------------------------------------------------------------
std::string cPlayer::resourceMapToString() const
{
	std::string str;
	str.reserve(resourceMap.size() + 1);
	for (size_t i = 0; i != resourceMap.size(); ++i)
	{
		str += getHexValue(resourceMap[i]);
	}
	return str;
}

//--------------------------------------------------------------------------
void cPlayer::setResourceMapFromString(const std::string& str)
{
	for (size_t i = 0; i != resourceMap.size(); ++i)
	{
		resourceMap.set(i, getByteValue(str, 2*i));
	}
}

//------------------------------------------------------------------------------
bool cPlayer::hasUnits() const
{
	return !vehicles.empty() || !buildings.empty();
}

//------------------------------------------------------------------------------
/** Starts a research center. */
//------------------------------------------------------------------------------
void cPlayer::startAResearch (cResearch::ResearchArea researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		++researchCentersWorkingTotal;
		++researchCentersWorkingOnArea[researchArea];

		researchCentersWorkingOnAreaChanged (researchArea);
		researchCentersWorkingTotalChanged();
	}
}

//------------------------------------------------------------------------------
/** Stops a research center. */
//------------------------------------------------------------------------------
void cPlayer::stopAResearch (cResearch::ResearchArea researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		--researchCentersWorkingTotal;
		if (researchCentersWorkingOnArea[researchArea] > 0)
		{
			--researchCentersWorkingOnArea[researchArea];
			researchCentersWorkingOnAreaChanged (researchArea);
		}
		researchCentersWorkingTotalChanged();
	}
}

//------------------------------------------------------------------------------
/** At turn end update the research level */
//------------------------------------------------------------------------------
void cPlayer::doResearch (const cUnitsData& unitsData)
{
	bool researchFinished = false;
	std::vector<int> areasReachingNextLevel;
	currentTurnResearchAreasFinished.clear();
	for (int area = 0; area < cResearch::kNrResearchAreas; ++area)
	{
		if (researchCentersWorkingOnArea[area] > 0 &&
			researchState.doResearch (researchCentersWorkingOnArea[area], area))
		{
			// next level reached
			areasReachingNextLevel.push_back (area);
			currentTurnResearchAreasFinished.push_back (area);
			researchFinished = true;
		}
	}
	if (researchFinished)
	{
		upgradeUnitTypes (areasReachingNextLevel, unitsData);
	}
}

void cPlayer::accumulateScore (cServer& server)
{
	const int now = server.getTurnClock()->getTurn();
	int deltaScore = 0;

	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& bp = *i;
		if (bp->getStaticUnitData().canScore && bp->isUnitWorking())
		{
			bp->points++;
			deltaScore++;

			sendUnitScore (server, *bp);
		}
	}
	setScore (getScore (now) + deltaScore, now);
	sendScore (server, *this, now);
}

void cPlayer::countEcoSpheres()
{
	numEcos = 0;

	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& bp = *i;
		if (bp->getStaticUnitData().canScore && bp->isUnitWorking())
			++numEcos;
	}
}

void cPlayer::setScore (int s, int turn)
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
	{
		pointsHistory.resize(t, pointsHistory.back());
	}
	pointsHistory[t - 1] = s;
}

int cPlayer::getScore (int turn) const
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
	{
		return pointsHistory.empty() ? 0 : pointsHistory.back();
	}
	return pointsHistory[t - 1];
}

int cPlayer::getScore() const
{
	return pointsHistory.back();
}

//------------------------------------------------------------------------------
void cPlayer::upgradeUnitTypes (const std::vector<int>& areasReachingNextLevel, const cUnitsData& originalUnitsData)
{
	for (auto& unitData : dynamicUnitsData)
	{
		const cDynamicUnitData& originalData = originalUnitsData.getDynamicUnitData(unitData.getId(), getClan());
		bool incrementVersion = false;
		for (auto researchArea : areasReachingNextLevel)
		{
			if (unitData.getId().isABuilding() && researchArea == cResearch::kSpeedResearch) continue;

			const int newResearchLevel = researchState.getCurResearchLevel (researchArea);
			int startValue = 0;
			switch (researchArea)
			{
				case cResearch::kAttackResearch: startValue = originalData.getDamage(); break;
				case cResearch::kShotsResearch: startValue = originalData.getShotsMax(); break;
				case cResearch::kRangeResearch: startValue = originalData.getRange(); break;
				case cResearch::kArmorResearch: startValue = originalData.getArmor(); break;
				case cResearch::kHitpointsResearch: startValue = originalData.getHitpointsMax(); break;
				case cResearch::kScanResearch: startValue = originalData.getScan(); break;
				case cResearch::kSpeedResearch: startValue = originalData.getSpeedMax(); break;
				case cResearch::kCostResearch: startValue = originalData.getBuildCost(); break;
			}

			cUpgradeCalculator::UnitTypes unitType = cUpgradeCalculator::kStandardUnit;
			if (unitData.getId().isABuilding()) unitType = cUpgradeCalculator::kBuilding;
			if (originalUnitsData.getStaticUnitData(unitData.getId()).isHuman) unitType = cUpgradeCalculator::kInfantry;

			int oldResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel - 10,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1, unitType);
			int newResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1, unitType);

			if (oldResearchBonus != newResearchBonus)
			{
				switch (researchArea)
				{
					case cResearch::kAttackResearch: unitData.setDamage (unitData.getDamage() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kShotsResearch: unitData.setShotsMax (unitData.getShotsMax() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kRangeResearch: unitData.setRange (unitData.getRange() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kArmorResearch: unitData.setArmor (unitData.getArmor() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kHitpointsResearch: unitData.setHitpointsMax (unitData.getHitpointsMax() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kScanResearch: unitData.setScan (unitData.getScan() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kSpeedResearch: unitData.setSpeedMax (unitData.getSpeedMax() + newResearchBonus - oldResearchBonus); break;
					case cResearch::kCostResearch: unitData.setBuildCost( unitData.getBuildCost() + newResearchBonus - oldResearchBonus); break;
				}
				if (researchArea != cResearch::kCostResearch)   // don't increment the version, if the only change are the costs
					incrementVersion = true;
			}
		}
		if (incrementVersion)
			unitData.setVersion (unitData.getVersion() + 1);
	}
}

//------------------------------------------------------------------------------
void cPlayer::refreshResearchCentersWorkingOnArea()
{
	int oldResearchCentersWorkingOnArea[cResearch::kNrResearchAreas];

	int newResearchCount = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
	{
		oldResearchCentersWorkingOnArea[i] = researchCentersWorkingOnArea[i];
		researchCentersWorkingOnArea[i] = 0;
	}

	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& building = *i;
		if (building->getStaticUnitData().canResearch && building->isUnitWorking())
		{
			researchCentersWorkingOnArea[building->getResearchArea()] += 1;
			newResearchCount++;
		}
	}
	std::swap (researchCentersWorkingTotal, newResearchCount);

	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
	{
		if (oldResearchCentersWorkingOnArea[i] != researchCentersWorkingOnArea[i])
		{
			researchCentersWorkingOnAreaChanged ((cResearch::ResearchArea)i);
		}
	}
	if (researchCentersWorkingTotal != newResearchCount) researchCentersWorkingTotalChanged();
}

//------------------------------------------------------------------------------
void cPlayer::refreshBase(const cMap& map)
{
	base.reset();
	for (auto& building : buildings)
	{
		base.addBuilding(building.get(), map);
	}
}

//------------------------------------------------------------------------------
void cPlayer::makeTurnStart(cModel& model)
{
	setHasFinishedTurn(false);
	resetTurnReportData();

	base.checkTurnEnd();

	base.makeTurnStart();

	// reload all buildings
	for (auto& building : buildings)
	{
		if (building->isDisabled())
		{
			building->setDisabledTurns(building->getDisabledTurns() - 1);
			if (building->isDisabled() == false && building->wasWorking)
			{
				building->startWork();
				building->wasWorking = false;
			}
		}
		building->refreshData();
	}

	// reload all vehicles
	for (auto& vehicle : vehicles)
	{
		if (vehicle->isDisabled())
		{
			vehicle->setDisabledTurns(vehicle->getDisabledTurns() - 1);
			if (!vehicle->isDisabled())
			{
				addToScan(*vehicle);
			}
		}
		vehicle->refreshData();
		vehicle->proceedBuilding(model);
		//vehicle->proceedClearing(model);
	}

	//just to prevent, that an error in scanmap updates have an permanent impact
	refreshScanMaps();
	refreshSentryMaps();

	// allow stealth vehicles to enter stealth mode, when they move in the new turn
	for (auto& vehicle : vehicles)
	{
		vehicle->clearDetectedInThisTurnPlayerList();
	}

	// do research:
	//TODO: research
	//doResearch(*model.getUnitsData());

	// eco-spheres:
	//TODO:
	//accumulateScore(*this);

	// Gun'em down:
	for (auto& vehicle : vehicles)
	{
		vehicle->inSentryRange(model);
	}
}

//------------------------------------------------------------------------------
uint32_t cPlayer::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(splayer, crc);
	crc = calcCheckSum(dynamicUnitsData, crc);
	crc = calcCheckSum(base, crc);
	for (const auto& v : vehicles)
		crc = calcCheckSum(*v, crc);
	for (const auto& b : buildings)
		crc = calcCheckSum(*b, crc);
	crc = calcCheckSum(landingPos, crc);
	crc = calcCheckSum(mapSize, crc);
	crc = calcCheckSum(scanMap, crc);
	crc = calcCheckSum(resourceMap, crc);
	crc = calcCheckSum(sentriesMapAir, crc);
	crc = calcCheckSum(sentriesMapGround, crc);
	crc = calcCheckSum(detectLandMap, crc);
	crc = calcCheckSum(detectSeaMap, crc);
	crc = calcCheckSum(detectMinesMap, crc);
	crc = calcCheckSum(pointsHistory, crc);
	crc = calcCheckSum(isDefeated, crc);
	crc = calcCheckSum(numEcos, crc);
	crc = calcCheckSum(clan, crc);
	crc = calcCheckSum(credits, crc);
	crc = calcCheckSum(currentTurnResearchAreasFinished, crc);
	crc = calcCheckSum(hasFinishedTurn, crc);
	crc = calcCheckSum(isRemovedFromGame, crc);
	crc = calcCheckSum(researchState, crc);
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		crc = calcCheckSum(researchCentersWorkingOnArea[i], crc);
	crc = calcCheckSum(researchCentersWorkingTotal, crc);

	return crc;
}

//------------------------------------------------------------------------------
bool cPlayer::mayHaveOffensiveUnit() const
{
	for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
	{
		const auto& vehicle = *i;
		if (vehicle->getStaticUnitData().canAttack || !vehicle->getStaticUnitData().canBuild.empty()) return true;
	}
	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& building = *i;
		if (building->getStaticUnitData().canAttack || !building->getStaticUnitData().canBuild.empty()) return true;
	}
	return false;
}

//------------------------------------------------------------------------------
void cPlayer::addTurnReportUnit (const sID& unitTypeId)
{
	auto iter = std::find_if (currentTurnUnitReports.begin(), currentTurnUnitReports.end(), [unitTypeId] (const sTurnstartReport & entry) { return entry.type == unitTypeId; });
	if (iter != currentTurnUnitReports.end())
	{
		++iter->count;
	}
	else
	{
		sTurnstartReport entry;
		entry.type = unitTypeId;
		entry.count = 1;
		currentTurnUnitReports.push_back (entry);
	}
}

//------------------------------------------------------------------------------
void cPlayer::resetTurnReportData()
{
	currentTurnUnitReports.clear();
}

//------------------------------------------------------------------------------
const std::vector<sTurnstartReport>& cPlayer::getCurrentTurnUnitReports() const
{
	return currentTurnUnitReports;
}

//------------------------------------------------------------------------------
const std::vector<int>& cPlayer::getCurrentTurnResearchAreasFinished() const
{
	return currentTurnResearchAreasFinished;
}

//------------------------------------------------------------------------------
void cPlayer::setCurrentTurnResearchAreasFinished (std::vector<int> areas)
{
	currentTurnResearchAreasFinished = std::move (areas);
}

//------------------------------------------------------------------------------
bool cPlayer::isCurrentTurnResearchAreaFinished (cResearch::ResearchArea area) const
{
	return std::find (currentTurnResearchAreasFinished.begin(), currentTurnResearchAreasFinished.end(), area) != currentTurnResearchAreasFinished.end();
}

//------------------------------------------------------------------------------
const cResearch& cPlayer::getResearchState() const
{
	return researchState;
}

//------------------------------------------------------------------------------
cResearch& cPlayer::getResearchState()
{
	return researchState;
}

//------------------------------------------------------------------------------
int cPlayer::getResearchCentersWorkingTotal() const
{
	return researchCentersWorkingTotal;
}

//------------------------------------------------------------------------------
int cPlayer::getResearchCentersWorkingOnArea (cResearch::ResearchArea area) const
{
	return researchCentersWorkingOnArea[area];
}

//------------------------------------------------------------------------------
bool cPlayer::canSeeAt (const cPosition& position) const
{
	return scanMap.get(position);
}

//------------------------------------------------------------------------------
bool cPlayer::getHasFinishedTurn() const
{
	return hasFinishedTurn;
}

//------------------------------------------------------------------------------
void cPlayer::setHasFinishedTurn (bool value)
{
	std::swap (hasFinishedTurn, value);
	if (hasFinishedTurn != value) hasFinishedTurnChanged();
}

//------------------------------------------------------------------------------
bool cPlayer::getIsRemovedFromGame() const
{
	return isRemovedFromGame;
}

//------------------------------------------------------------------------------
void cPlayer::setIsRemovedFromGame (bool value)
{
	std::swap (isRemovedFromGame, value);
	if (isRemovedFromGame != value) isRemovedFromGameChanged();
}
