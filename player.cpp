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
#include <math.h>

#include "player.h"

#include "buildings.h"
#include "client.h"
#include "clist.h"
#include "hud.h"
#include "server.h"
#include "serverevents.h"
#include "vehicles.h"

using namespace std;

//-----------------------------------------------------------------------
// Implementation cPlayer class
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
cPlayer::cPlayer (const string& Name, SDL_Surface* Color, int nr, int iSocketNum) :
	name (Name),
	color (Color),
	Nr (nr),
	numEcos (0),
	lastDeletedUnit (0),
	clan (-1)
{
	// copy the vehicle stats
	VehicleData.reserve (UnitsData.getNrVehicles());
	for (unsigned int i = 0; i < UnitsData.getNrVehicles(); i++)
		VehicleData.push_back (UnitsData.getVehicle (i).data); // get the default (no clan) vehicle data

	// copy the building stats
	BuildingData.reserve (UnitsData.getNrBuildings());
	for (unsigned int i = 0; i < UnitsData.getNrBuildings(); i++)
		BuildingData.push_back (UnitsData.getBuilding (i).data); // get the default (no clan) building data

	VehicleList = NULL;
	BuildingList = NULL;

	ResearchCount = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = 0;
	Credits = 0;

	this->iSocketNum = iSocketNum;
	isDefeated = false;
	isRemovedFromGame = false;
	bFinishedTurn = false;

	savedHud = new sHudStateContainer;
	researchFinished = false;
}

//-----------------------------------------------------------------------
cPlayer::cPlayer (const cPlayer& Player)
{
	name = Player.name;
	color = Player.color;
	Nr = Player.Nr;
	iSocketNum = Player.iSocketNum;
	clan = Player.clan;
	pointsHistory = Player.pointsHistory;
	numEcos = Player.numEcos;
	lastDeletedUnit = Player.lastDeletedUnit;

	// copy vehicle and building data
	VehicleData = Player.VehicleData;
	BuildingData = Player.BuildingData;

	// Don't copy ScanMap, ResourceMap, DetectLandMap, DetectSeaMap,
	//  DetectMinesMap, SentriesMapAir, SentriesMapGround
	// there should be empty.

	VehicleList = NULL;
	BuildingList = NULL;

	Credits = Player.Credits;
	ResearchCount = Player.ResearchCount;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = Player.researchCentersWorkingOnArea[i];
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
	{
		researchLevel.setCurResearchLevel (Player.researchLevel.getCurResearchLevel (i), i);
		researchLevel.setCurResearchPoints (Player.researchLevel.getCurResearchPoints (i), i);
	}
	reportResearchAreasFinished = Player.reportResearchAreasFinished;

	isDefeated = false;
	bFinishedTurn = Player.bFinishedTurn;

	savedHud = new sHudStateContainer;

	researchFinished = Player.researchFinished;
}

//-----------------------------------------------------------------------
cPlayer::~cPlayer()
{
	// Erst alle geladenen Vehicles lˆschen:

	for (cVehicle* ptr = VehicleList; ptr; ptr = ptr->next)
	{
		if (ptr->storedUnits.size())
		{
			ptr->deleteStoredUnits();
		}
	}
	// Jetzt alle Vehicles lˆschen:
	while (VehicleList)
	{
		cVehicle* ptr = VehicleList->next;
		VehicleList->sentryActive = false;
		delete VehicleList;
		VehicleList = ptr;
	}
	while (BuildingList)
	{
		cBuilding* ptr = BuildingList->next;
		BuildingList->sentryActive = false;

		// Stored Vehicles are already deleted; just clear the list
		BuildingList->storedUnits.clear();

		delete BuildingList;
		BuildingList = ptr;
	}

	for (size_t i = 0; i != ReportVehicles.size(); ++i)
	{
		delete ReportVehicles[i];
	}
	ReportVehicles.clear();
	for (size_t i = 0; i != ReportBuildings.size(); ++i)
	{
		delete ReportBuildings[i];
	}
	ReportBuildings.clear();

	delete savedHud;
}

//--------------------------------------------------------------------------
void cPlayer::setClan (int newClan)
{
	if (newClan == clan || newClan < -1 || 7 < newClan)
		return;

	clan = newClan;

	for (unsigned int i = 0; i < UnitsData.getNrVehicles(); i++)
		VehicleData[i] = UnitsData.getVehicle (i, clan).data;

	for (unsigned int i = 0; i < UnitsData.getNrBuildings(); i++)
		BuildingData[i] = UnitsData.getBuilding (i, clan).data;
}

//--------------------------------------------------------------------------
/** Adds the vehicle to the list of the player */
//--------------------------------------------------------------------------
cVehicle* cPlayer::AddVehicle (int posx, int posy, const sVehicle* v, unsigned int ID)
{
	cVehicle* n = new cVehicle (v, this, ID);
	n->PosX = posx;
	n->PosY = posy;

	addUnitToList (n);

	drawSpecialCircle (n->PosX, n->PosY, n->data.scan, ScanMap, mapSize);
	if (n->data.canDetectStealthOn & TERRAIN_GROUND) drawSpecialCircle (n->PosX, n->PosY, n->data.scan, DetectLandMap, mapSize);
	if (n->data.canDetectStealthOn & TERRAIN_SEA) drawSpecialCircle (n->PosX, n->PosY, n->data.scan, DetectSeaMap, mapSize);
	if (n->data.canDetectStealthOn & AREA_EXP_MINE)
	{
		const int minx = std::max (n->PosX - 1, 0);
		const int maxx = std::min (n->PosX + 1, mapSize - 1);
		const int miny = std::max (n->PosY - 1, 0);
		const int maxy = std::min (n->PosY + 1, mapSize - 1);
		for (int x = minx; x <= maxx; ++x)
			for (int y = miny; y <= maxy; ++y)
				DetectMinesMap[x + mapSize * y] = 1;
	}
	return n;
}

//--------------------------------------------------------------------------
/** initialize the maps */
//--------------------------------------------------------------------------
void cPlayer::InitMaps (int MapSizeX, cMap* map)
{
	mapSize = MapSizeX;
	const int size = MapSizeX * MapSizeX;
	// Scanner-Map:
	ScanMap.clear();
	ScanMap.resize (size, 0);
	// Ressource-Map
	ResourceMap.clear();
	ResourceMap.resize (size, 0);

	base.map = map;
	// Sentry-Map:
	SentriesMapAir.clear();
	SentriesMapAir.resize (size, 0);
	SentriesMapGround.clear();
	SentriesMapGround.resize (size, 0);

	// Detect-Maps:
	DetectLandMap.clear();
	DetectLandMap.resize (size, 0);
	DetectSeaMap.clear();
	DetectSeaMap.resize (size, 0);
	DetectMinesMap.clear();
	DetectMinesMap.resize (size, 0);
}

template <typename T>
T* getPreviousUnitById(T* root, unsigned int id)
{
	if (root == 0 || id < root->iID) return 0;
	T* it = root;
	for (; it->next; it = it->next)
	{
		if (it->next->iID < id)
			return it;
	}
	return it;
}

void cPlayer::addUnitToList (cUnit* addedUnit)
{
	//units in the linked list are sorted in increasing order of IDs

	//find unit before the added unit
	if (addedUnit->isBuilding())
	{
		cBuilding* addedBuilding = static_cast<cBuilding*>(addedUnit);
		cBuilding* prev = getPreviousUnitById (BuildingList, addedUnit->iID);
		insert_after_in_intrusivelist (BuildingList, prev, *addedBuilding);
	}
	else
	{
		cVehicle* addedVehicle = static_cast<cVehicle*>(addedUnit);
		cVehicle* prev = getPreviousUnitById (VehicleList, addedUnit->iID);
		insert_after_in_intrusivelist (VehicleList, prev, *addedVehicle);
	}
}

//--------------------------------------------------------------------------
/** Adds the building to the list of the player */
//--------------------------------------------------------------------------
cBuilding* cPlayer::addBuilding (int posx, int posy, const sBuilding* b, unsigned int ID)
{
	cBuilding* Building = new cBuilding (b, this, ID);

	Building->PosX = posx;
	Building->PosY = posy;

	addUnitToList (Building);

	if (Building->data.scan)
	{
		if (Building->data.isBig) drawSpecialCircleBig (Building->PosX, Building->PosY, Building->data.scan, ScanMap, mapSize);
		else drawSpecialCircle (Building->PosX, Building->PosY, Building->data.scan, ScanMap, mapSize);
	}
	return Building;
}

//--------------------------------------------------------------------------
void cPlayer::addSentry (cUnit* u)
{
	u->sentryActive = true;
	if (u->data.canAttack & TERRAIN_AIR)
	{
		drawSpecialCircle (u->PosX, u->PosY, u->data.range, SentriesMapAir, mapSize);
	}
	if ((u->data.canAttack & TERRAIN_GROUND) || (u->data.canAttack & TERRAIN_SEA))
	{
		drawSpecialCircle (u->PosX, u->PosY, u->data.range, SentriesMapGround, mapSize);
	}
}

//--------------------------------------------------------------------------
void cPlayer::deleteSentry (cUnit* u)
{
	u->sentryActive = false;
	if (u->data.canAttack & TERRAIN_AIR)
	{
		refreshSentryAir();
	}
	else if ((u->data.canAttack & TERRAIN_GROUND) || (u->data.canAttack & TERRAIN_SEA))
	{
		refreshSentryGround();
	}
}

//--------------------------------------------------------------------------
void cPlayer::refreshSentryAir()
{
	std::fill (SentriesMapAir.begin(), SentriesMapAir.end(), 0);

	for (const cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		if (unit->sentryActive && unit->data.canAttack & TERRAIN_AIR)
		{
			drawSpecialCircle (unit->PosX, unit->PosY, unit->data.range, SentriesMapAir, mapSize);
		}
	}

	for (const cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		if (unit->sentryActive && unit->data.canAttack & TERRAIN_AIR)
		{
			drawSpecialCircle (unit->PosX, unit->PosY, unit->data.range, SentriesMapAir, mapSize);
		}
	}
}

//--------------------------------------------------------------------------
void cPlayer::refreshSentryGround()
{
	std::fill (SentriesMapGround.begin(), SentriesMapGround.end(), 0);

	for (const cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		if (unit->sentryActive && ( (unit->data.canAttack & TERRAIN_GROUND) || (unit->data.canAttack & TERRAIN_SEA)))
		{
			drawSpecialCircle (unit->PosX, unit->PosY, unit->data.range, SentriesMapGround, mapSize);
		}
	}

	for (const cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		if (unit->sentryActive && ( (unit->data.canAttack & TERRAIN_GROUND) || (unit->data.canAttack & TERRAIN_SEA)))
		{
			drawSpecialCircle (unit->PosX, unit->PosY, unit->data.range, SentriesMapGround, mapSize);
		}
	}
}

//--------------------------------------------------------------------------
/** Does a scan for all units of the player */
//--------------------------------------------------------------------------
void cPlayer::DoScan()
{
	if (isDefeated) return;
	std::fill (ScanMap.begin(), ScanMap.end(), 0);
	std::fill (DetectLandMap.begin(), DetectLandMap.end(), 0);
	std::fill (DetectSeaMap.begin(), DetectSeaMap.end(), 0);
	std::fill (DetectMinesMap.begin(), DetectMinesMap.end(), 0);

	// iterate the vehicle list
	for (const cVehicle* vp = VehicleList; vp; vp = vp->next)
	{
		if (vp->Loaded) continue;

		if (vp->turnsDisabled)
			ScanMap[vp->PosX + vp->PosY * mapSize] = 1;
		else
		{
			if (vp->data.isBig)
				drawSpecialCircleBig (vp->PosX, vp->PosY, vp->data.scan, ScanMap, mapSize);
			else
				drawSpecialCircle (vp->PosX, vp->PosY, vp->data.scan, ScanMap, mapSize);

			//detection maps
			if (vp->data.canDetectStealthOn & TERRAIN_GROUND) drawSpecialCircle (vp->PosX, vp->PosY, vp->data.scan, DetectLandMap, mapSize);
			else if (vp->data.canDetectStealthOn & TERRAIN_SEA) drawSpecialCircle (vp->PosX, vp->PosY, vp->data.scan, DetectSeaMap, mapSize);
			if (vp->data.canDetectStealthOn & AREA_EXP_MINE)
			{
				const int minx = std::max (vp->PosX - 1, 0);
				const int maxx = std::min (vp->PosX + 1, mapSize - 1);
				const int miny = std::max (vp->PosY - 1, 0);
				const int maxy = std::min (vp->PosY + 1, mapSize - 1);
				for (int x = minx; x <= maxx; ++x)
				{
					for (int y = miny; y <= maxy; ++y)
					{
						DetectMinesMap[x + mapSize * y] = 1;
					}
				}
			}
		}
	}

	// iterate the building list
	for (const cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->turnsDisabled)
			ScanMap[bp->PosX + bp->PosY * mapSize] = 1;
		else if (bp->data.scan)
		{
			if (bp->data.isBig)
				drawSpecialCircleBig (bp->PosX, bp->PosY, bp->data.scan, ScanMap, mapSize);
			else
				drawSpecialCircle (bp->PosX, bp->PosY, bp->data.scan, ScanMap, mapSize);
		}
	}
}

void cPlayer::revealMap()
{
	std::fill (ScanMap.begin(), ScanMap.end(), 1);
}

void cPlayer::revealResource()
{
	std::fill (ResourceMap.begin(), ResourceMap.end(), 1);
}

cVehicle* cPlayer::getNextVehicle (cVehicle* start)
{
	start = (start == NULL) ? VehicleList : start->next;
	for (cVehicle* it = start; it; it = it->next)
	{
		if (!it->isMarkedAsDone && (!it->IsBuilding || it->BuildRounds == 0)
			&& !it->IsClearing && !it->sentryActive && !it->Loaded
			&& (it->data.speedCur || it->data.shotsCur))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getNextBuilding (cBuilding* start)
{
	start = (start == NULL) ? BuildingList : start->next;
	for (cBuilding* it = start; it; it = it->next)
	{
		if (!it->isMarkedAsDone && !it->IsWorking && !it->sentryActive
			&& (!it->data.canBuild.empty() || it->data.shotsCur
				|| it->data.canMineMaxRes > 0 || it->data.convertsGold > 0
				|| it->data.canResearch))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getNextMiningStation (cBuilding* start)
{
	start = (start == NULL) ? BuildingList : start->next;
	for (cBuilding* it = start; it; it = it->next)
	{
		if (it->data.canMineMaxRes > 0)
			return it;
	}
	return NULL;
}

//--------------------------------------------------------------------------
/** Returns the next unit that can still fire/shoot */
//--------------------------------------------------------------------------
cUnit* cPlayer::getNextUnit(cUnit* start)
{
	if (start == NULL || start->owner != this)
	{
		cVehicle* nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
		cBuilding* nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
	}
	else if (start->isVehicle())
	{
		cVehicle* nextVehicle = getNextVehicle (static_cast<cVehicle*> (start));
		if (nextVehicle) return nextVehicle;
		cBuilding* nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
		nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
	}
	else
	{
		assert (start->isBuilding());
		cBuilding* building = static_cast<cBuilding*>(start);
		cBuilding* nextBuilding = getNextBuilding (building);
		if (nextBuilding) return nextBuilding;
		cVehicle* nextVehicle = getNextVehicle (NULL);
		if (nextVehicle) return nextVehicle;
		nextBuilding = getNextBuilding (NULL);
		if (nextBuilding) return nextBuilding;
	}
	// finally, return the more recent built Mining station.
	// since list order is by increasing age, take the first in list.
	return getNextMiningStation (NULL);
}

cVehicle* cPlayer::getPrevVehicle (cVehicle* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (VehicleList) : start->prev;
	for (cVehicle* it = start; it; it = it->prev)
	{
		if (!it->isMarkedAsDone && (!it->IsBuilding || it->BuildRounds == 0)
			&& !it->IsClearing && !it->sentryActive && !it->Loaded
			&& (it->data.speedCur || it->data.shotsCur))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getPrevBuilding (cBuilding* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (BuildingList) : start->prev;
	for (cBuilding* it = start; it; it = it->prev)
	{
		if (!it->isMarkedAsDone && !it->IsWorking && !it->sentryActive
			&& (!it->data.canBuild.empty() || it->data.shotsCur
				|| it->data.canMineMaxRes > 0 || it->data.convertsGold > 0
				|| it->data.canResearch))
			return it;
	}
	return NULL;
}

cBuilding* cPlayer::getPrevMiningStation (cBuilding* start)
{
	start = (start == NULL) ? get_last_of_intrusivelist (BuildingList) : start->prev;
	for (cBuilding* it = start; it; it = it->prev)
	{
		if (it->data.canMineMaxRes > 0)
			return it;
	}
	return NULL;
}

//--------------------------------------------------------------------------
/** Returns the previous vehicle, that can still move / shoot */
//--------------------------------------------------------------------------
cUnit* cPlayer::getPrevUnit (cUnit* start)
{
	if (start == NULL || start->owner != this)
	{
		cVehicle* prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
		cBuilding* prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
	}
	else if (start->isVehicle())
	{
		cVehicle* prevVehicle = getPrevVehicle (static_cast<cVehicle*> (start));
		if (prevVehicle) return prevVehicle;
		cBuilding* prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
		prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
	}
	else
	{
		assert (start->isBuilding());
		cBuilding* building = static_cast<cBuilding*>(start);
		cBuilding* prevBuilding = getPrevBuilding (building);
		if (prevBuilding) return prevBuilding;
		cVehicle* prevVehicle = getPrevVehicle (NULL);
		if (prevVehicle) return prevVehicle;
		prevBuilding = getPrevBuilding (NULL);
		if (prevBuilding) return prevBuilding;
	}
	// finally, return the more recent built Mining station.
	// since list order is by increasing age, take the first in list.
	return getNextMiningStation (NULL);
}

//--------------------------------------------------------------
/** Starts a research center. */
//--------------------------------------------------------------
void cPlayer::startAResearch (int researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		ResearchCount++;
		researchCentersWorkingOnArea[researchArea] += 1;
	}
}

//--------------------------------------------------------------
/** Stops a research center. */
//--------------------------------------------------------------
void cPlayer::stopAResearch (int researchArea)
{
	if (0 <= researchArea && researchArea <= cResearch::kNrResearchAreas)
	{
		ResearchCount--;
		if (researchCentersWorkingOnArea[researchArea] > 0)
			researchCentersWorkingOnArea[researchArea] -= 1;
	}
}

//--------------------------------------------------------------
/** At turnend update the research level */
//--------------------------------------------------------------
void cPlayer::doResearch (cServer& server)
{
	bool researchFinished = false;
	std::vector<sUnitData*> upgradedUnitDatas;
	std::vector<int> areasReachingNextLevel;
	reportResearchAreasFinished.clear();
	for (int area = 0; area < cResearch::kNrResearchAreas; area++)
	{
		if (researchCentersWorkingOnArea[area] > 0 &&
			researchLevel.doResearch (researchCentersWorkingOnArea[area], area))
		{
			// next level reached
			areasReachingNextLevel.push_back (area);
			reportResearchAreasFinished.push_back (area);
			researchFinished = true;
		}
	}
	if (researchFinished)
	{
		upgradeUnitTypes (areasReachingNextLevel, upgradedUnitDatas);

		for (unsigned int i = 0; i < upgradedUnitDatas.size(); i++)
			sendUnitUpgrades (server, *upgradedUnitDatas[i], Nr);
	}
	sendResearchLevel (server, researchLevel, Nr);
}

void cPlayer::accumulateScore (cServer& server)
{
	const int now = server.getTurn();
	int deltaScore = 0;

	for (cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->typ->data.canScore && bp->IsWorking)
		{
			bp->points++;
			deltaScore++;

			sendUnitScore (server, *bp);
		}
	}
	setScore (getScore (now) + deltaScore, now);
	sendScore (server, *this, now);
}

void cPlayer::CountEcoSpheres()
{
	numEcos = 0;

	for (const cBuilding* bp = BuildingList; bp; bp = bp->next)
	{
		if (bp->typ->data.canScore && bp->IsWorking)
			numEcos ++;
	}
}

void cPlayer::setScore (int s, int turn)
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
		pointsHistory.resize (t);
	pointsHistory[t - 1] = s;
}

void cPlayer::clearDone()
{
	for (cVehicle* unit = VehicleList; unit; unit = unit->next)
	{
		unit->isMarkedAsDone = false;
	}

	for (cBuilding* unit = BuildingList; unit; unit = unit->next)
	{
		unit->isMarkedAsDone = false;
	}
}

int cPlayer::getScore (int turn) const
{
	// turn begins at 1.
	unsigned int t = turn;

	if (pointsHistory.size() < t)
	{
		int score = pointsHistory.empty() ?
					0 : pointsHistory[pointsHistory.size() - 1];
		pointsHistory.resize (t);
		pointsHistory[t - 1] = score;
	}
	return pointsHistory[t - 1];
}

//--------------------------------------------------------------
void cPlayer::upgradeUnitTypes (const std::vector<int>& areasReachingNextLevel, std::vector<sUnitData*>& resultUpgradedUnitDatas)
{
	for (unsigned int i = 0; i < UnitsData.getNrVehicles(); i++)
	{
		const sUnitData& originalData = UnitsData.getVehicle (i, getClan()).data;
		bool incrementVersion = false;
		for (unsigned int areaCounter = 0; areaCounter < areasReachingNextLevel.size(); areaCounter++)
		{
			const int researchArea = areasReachingNextLevel[areaCounter];
			const int newResearchLevel = researchLevel.getCurResearchLevel (researchArea);
			int startValue = 0;
			switch (researchArea)
			{
				case cResearch::kAttackResearch: startValue = originalData.damage; break;
				case cResearch::kShotsResearch: startValue = originalData.shotsMax; break;
				case cResearch::kRangeResearch: startValue = originalData.range; break;
				case cResearch::kArmorResearch: startValue = originalData.armor; break;
				case cResearch::kHitpointsResearch: startValue = originalData.hitpointsMax; break;
				case cResearch::kScanResearch: startValue = originalData.scan; break;
				case cResearch::kSpeedResearch: startValue = originalData.speedMax; break;
				case cResearch::kCostResearch: startValue = originalData.buildCosts; break;
			}
			int oldResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel - 10,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   originalData.isHuman ? cUpgradeCalculator::kInfantry : cUpgradeCalculator::kStandardUnit);
			int newResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   originalData.isHuman ? cUpgradeCalculator::kInfantry : cUpgradeCalculator::kStandardUnit);
			if (oldResearchBonus != newResearchBonus)
			{
				switch (researchArea)
				{
					case cResearch::kAttackResearch: VehicleData[i].damage += newResearchBonus - oldResearchBonus; break;
					case cResearch::kShotsResearch: VehicleData[i].shotsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kRangeResearch: VehicleData[i].range += newResearchBonus - oldResearchBonus; break;
					case cResearch::kArmorResearch: VehicleData[i].armor += newResearchBonus - oldResearchBonus; break;
					case cResearch::kHitpointsResearch: VehicleData[i].hitpointsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kScanResearch: VehicleData[i].scan += newResearchBonus - oldResearchBonus; break;
					case cResearch::kSpeedResearch: VehicleData[i].speedMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kCostResearch: VehicleData[i].buildCosts += newResearchBonus - oldResearchBonus; break;
				}
				if (researchArea != cResearch::kCostResearch)   // don't increment the version, if the only change are the costs
					incrementVersion = true;
				if (!Contains (resultUpgradedUnitDatas, &(VehicleData[i])))
					resultUpgradedUnitDatas.push_back (&(VehicleData[i]));
			}
		}
		if (incrementVersion)
			VehicleData[i].version += 1;
	}

	for (unsigned int i = 0; i < UnitsData.getNrBuildings(); i++)
	{
		const sUnitData& originalData = UnitsData.getBuilding (i, getClan()).data;
		bool incrementVersion = false;
		for (unsigned int areaCounter = 0; areaCounter < areasReachingNextLevel.size(); areaCounter++)
		{
			const int researchArea = areasReachingNextLevel[areaCounter];
			const int newResearchLevel = researchLevel.getCurResearchLevel (researchArea);

			int startValue = 0;
			switch (researchArea)
			{
				case cResearch::kAttackResearch: startValue = originalData.damage; break;
				case cResearch::kShotsResearch: startValue = originalData.shotsMax; break;
				case cResearch::kRangeResearch: startValue = originalData.range; break;
				case cResearch::kArmorResearch: startValue = originalData.armor; break;
				case cResearch::kHitpointsResearch: startValue = originalData.hitpointsMax; break;
				case cResearch::kScanResearch: startValue = originalData.scan; break;
				case cResearch::kCostResearch: startValue = originalData.buildCosts; break;
			}
			int oldResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel - 10,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   cUpgradeCalculator::kBuilding);
			int newResearchBonus = cUpgradeCalculator::instance().calcChangeByResearch (startValue, newResearchLevel,
								   researchArea == cResearch::kCostResearch ? cUpgradeCalculator::kCost : -1,
								   cUpgradeCalculator::kBuilding);
			if (oldResearchBonus != newResearchBonus)
			{
				switch (researchArea)
				{
					case cResearch::kAttackResearch: BuildingData[i].damage += newResearchBonus - oldResearchBonus; break;
					case cResearch::kShotsResearch: BuildingData[i].shotsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kRangeResearch: BuildingData[i].range += newResearchBonus - oldResearchBonus; break;
					case cResearch::kArmorResearch: BuildingData[i].armor += newResearchBonus - oldResearchBonus; break;
					case cResearch::kHitpointsResearch: BuildingData[i].hitpointsMax += newResearchBonus - oldResearchBonus; break;
					case cResearch::kScanResearch: BuildingData[i].scan += newResearchBonus - oldResearchBonus; break;
					case cResearch::kCostResearch: BuildingData[i].buildCosts += newResearchBonus - oldResearchBonus; break;
				}
				if (researchArea != cResearch::kCostResearch)   // don't increment the version, if the only change are the costs
					incrementVersion = true;
				if (!Contains (resultUpgradedUnitDatas, &(BuildingData[i])))
					resultUpgradedUnitDatas.push_back (&(BuildingData[i]));
			}
		}
		if (incrementVersion)
			BuildingData[i].version += 1;
	}
}

//------------------------------------------------------------
void cPlayer::refreshResearchCentersWorkingOnArea()
{
	int newResearchCount = 0;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		researchCentersWorkingOnArea[i] = 0;

	for (const cBuilding* curBuilding = BuildingList; curBuilding; curBuilding = curBuilding->next)
	{
		if (curBuilding->data.canResearch && curBuilding->IsWorking)
		{
			researchCentersWorkingOnArea[curBuilding->researchArea] += 1;
			newResearchCount++;
		}
	}
	ResearchCount = newResearchCount;
}

//--------------------------------------------------------------------------
void cPlayer::DeleteLock (cUnit& unit)
{
	std::vector<cUnit*>::iterator it = std::find (LockList.begin(), LockList.end(), &unit);
	if (it != LockList.end()) LockList.erase (it);
}

//--------------------------------------------------------------------------
/** Toggles the lock state of a unit under the mouse
 * (when locked it's range and scan is displayed, although the unit is not selected).
*/
//--------------------------------------------------------------------------
void cPlayer::ToggelLock (cMapField* OverUnitField)
{
	cUnit* unit = NULL;
	if (OverUnitField->getBaseBuilding() && OverUnitField->getBaseBuilding()->owner != this)
	{
		unit = OverUnitField->getBaseBuilding();
	}
	else if (OverUnitField->getTopBuilding() && OverUnitField->getTopBuilding()->owner != this)
	{
		unit = OverUnitField->getTopBuilding();
	}
	if (OverUnitField->getVehicles() && OverUnitField->getVehicles()->owner != this)
	{
		unit = OverUnitField->getVehicles();
	}
	if (OverUnitField->getPlanes() && OverUnitField->getPlanes()->owner != this)
	{
		unit = OverUnitField->getPlanes();
	}

	if (unit == NULL) return;
	unit->IsLocked = !unit->IsLocked;
	std::vector<cUnit*>::iterator it = std::find(LockList.begin(), LockList.end(), unit);
	if (it == LockList.end()) LockList.push_back (unit);
	else LockList.erase (it);
}

//--------------------------------------------------------------------------
/** Draws all entries, that are in the lock list. */
//--------------------------------------------------------------------------
void cPlayer::DrawLockList (cGameGUI& gameGUI)
{
	if (!gameGUI.lockChecked()) return;
	const int tileSize = gameGUI.getTileSize();
	const cMap& map = *gameGUI.getClient()->getMap();
	for (unsigned int i = 0; i < LockList.size(); i++)
	{
		cUnit* unit = LockList[i];
		const int off = unit->PosX + unit->PosY * map.size;
		if (!ScanMap[off])
		{
			unit->IsLocked = false;
			LockList.erase (LockList.begin() + i);
			i--;
			continue;
		}
		const SDL_Rect screenPos = {Sint16 (unit->getScreenPosX (gameGUI)), Sint16 (unit->getScreenPosY (gameGUI)), 0, 0};

		if (gameGUI.scanChecked())
		{
			if (unit->data.isBig)
				drawCircle (screenPos.x + tileSize, screenPos.y + tileSize, unit->data.scan * tileSize, SCAN_COLOR, buffer);
			else
				drawCircle (screenPos.x + tileSize / 2, screenPos.y + tileSize / 2, unit->data.scan * tileSize, SCAN_COLOR, buffer);
		}
		if (gameGUI.rangeChecked() && (unit->data.canAttack & TERRAIN_GROUND))
			drawCircle (screenPos.x + tileSize / 2, screenPos.y + tileSize / 2,
						unit->data.range * tileSize + 1, RANGE_GROUND_COLOR, buffer);
		if (gameGUI.rangeChecked() && (unit->data.canAttack & TERRAIN_AIR))
			drawCircle (screenPos.x + tileSize / 2, screenPos.y + tileSize / 2,
						unit->data.range * tileSize + 2, RANGE_AIR_COLOR, buffer);
		if (gameGUI.ammoChecked() && unit->data.canAttack)
			unit->drawMunBar (gameGUI, screenPos);
		if (gameGUI.hitsChecked())
			unit->drawHealthBar (gameGUI, screenPos);
	}
}

//--------------------------------------------------------------------------
void cPlayer::drawSpecialCircle (int iX, int iY, int iRadius, std::vector<char>& map, int mapsize)
{
	const float PI_ON_180 = 0.017453f;
	const float PI_ON_4 = PI_ON_180 * 45;
	if (iRadius <= 0) return;

	iRadius *= 10;
	const float step = (PI_ON_180 * 90 - acos (1.0 / iRadius)) / 2;

	for (float angle = 0; angle <= PI_ON_4; angle += step)
	{
		int rx = (int) (cos (angle) * iRadius);
		int ry = (int) (sin (angle) * iRadius);
		rx /= 10;
		ry /= 10;

		int x1 = rx + iX;
		int x2 = -rx + iX;
		for (int k = x2; k <= x1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize) break;
			if (iY + ry >= 0 && iY + ry < mapsize)
				map[k + (iY + ry) * mapsize] |= 1;
			if (iY - ry >= 0 && iY - ry < mapsize)
				map[k + (iY - ry) * mapsize] |= 1;
		}

		x1 = ry + iX;
		x2 = -ry + iX;
		for (int k = x2; k <= x1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize) break;
			if (iY + rx >= 0 && iY + rx < mapsize)
				map[k + (iY + rx) *mapsize] |= 1;
			if (iY - rx >= 0 && iY - rx < mapsize)
				map[k + (iY - rx) *mapsize] |= 1;
		}
	}
}

//--------------------------------------------------------------------------
void cPlayer::drawSpecialCircleBig (int iX, int iY, int iRadius, std::vector<char>& map, int mapsize)
{
	const float PI_ON_180 = 0.017453f;
	const float PI_ON_4 = PI_ON_180 * 45;
	if (iRadius <= 0) return;

	--iRadius;
	iRadius *= 10;
	const float step = (PI_ON_180 * 90 - acos (1.0 / iRadius)) / 2;
	for (float angle = 0; angle <= PI_ON_4; angle += step)
	{
		int rx = (int) (cos (angle) * iRadius);
		int ry = (int) (sin (angle) * iRadius);
		rx /= 10;
		ry /= 10;

		int x1 = rx + iX;
		int x2 = -rx + iX;
		for (int k = x2; k <= x1 + 1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize) break;
			if (iY + ry >= 0 && iY + ry < mapsize)
				map[k + (iY + ry) *mapsize] |= 1;
			if (iY - ry >= 0 && iY - ry < mapsize)
				map[k + (iY - ry) *mapsize] |= 1;

			if (iY + ry + 1 >= 0 && iY + ry + 1 < mapsize)
				map[k + (iY + ry + 1) *mapsize] |= 1;
			if (iY - ry + 1 >= 0 && iY - ry + 1 < mapsize)
				map[k + (iY - ry + 1) *mapsize] |= 1;
		}

		x1 = ry + iX;
		x2 = -ry + iX;
		for (int k = x2; k <= x1 + 1; k++)
		{
			if (k < 0) continue;
			if (k >= mapsize) break;
			if (iY + rx >= 0 && iY + rx < mapsize)
				map[k + (iY + rx) *mapsize] |= 1;
			if (iY - rx >= 0 && iY - rx < mapsize)
				map[k + (iY - rx) *mapsize] |= 1;

			if (iY + rx + 1 >= 0 && iY + rx + 1 < mapsize)
				map[k + (iY + rx + 1) *mapsize] |= 1;
			if (iY - rx + 1 >= 0 && iY - rx + 1 < mapsize)
				map[k + (iY - rx + 1) *mapsize] |= 1;
		}
	}
}

//--------------------------------------------------------------------------
void cPlayer::addSavedReport (const string& message, sSavedReportMessage::eReportTypes type, sID unitID, int xPos, int yPos, int colorNr)
{
	sSavedReportMessage savedReport;
	savedReport.message = message;
	savedReport.type = type;
	savedReport.xPos = xPos;
	savedReport.yPos = yPos;
	savedReport.unitID = unitID;
	savedReport.colorNr = colorNr;

	savedReportsList.push_back (savedReport);
}
