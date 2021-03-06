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

#include "model.h"

#include "player/player.h"
#include "map/map.h"
#include "units/vehicle.h"
#include "units/building.h"
#include "game/logic/movejob.h"
#include "utility/crc.h"
#include "game/logic/pathcalculator.h"
#include "utility/listhelpers.h"
#include "game/logic/turncounter.h"
#include "game/logic/turntimeclock.h"
#include "game/logic/fxeffects.h"
#include "game/logic/jobs/destroyjob.h"
#include "map/mapview.h"

//------------------------------------------------------------------------------
cModel::cModel() :
	nextUnitId(0),
	gameSettings(std::make_shared<cGameSettings>()),
	unitsData(std::make_shared<cUnitsData>()),
	turnCounter (std::make_shared<cTurnCounter> (1)),
	gameTime(0),
	gameId(0),
	turnEndState(TURN_ACTIVE),
	activeTurnPlayer(nullptr),
	turnEndDeadline(0),
	turnLimitDeadline(0)
{
	turnTimeClock = std::make_shared<cTurnTimeClock>(*this);

	gameSettings->turnEndDeadlineChanged.connect([this]()
	{
		if (turnEndDeadline)
		{
			turnTimeClock->changeDeadline(turnEndDeadline, gameSettings->getTurnEndDeadline());
		}
	});

	gameSettings->turnEndDeadlineActiveChanged.connect([this]()
	{
		if (!gameSettings->isTurnEndDeadlineActive() && turnEndDeadline)
		{
			turnTimeClock->removeDeadline(turnEndDeadline);
			turnEndDeadline = 0;
		}
	});

	gameSettings->turnLimitChanged.connect([this]()
	{
		if (turnLimitDeadline)
		{
			turnTimeClock->changeDeadline(turnLimitDeadline, gameSettings->getTurnLimit());
		}
	});

	gameSettings->turnLimitActiveChanged.connect([this]()
	{
		if (!gameSettings->isTurnLimitActive() && turnLimitDeadline)
		{
			turnTimeClock->removeDeadline(turnLimitDeadline);
			turnLimitDeadline = 0;
		}
		else if (gameSettings->isTurnLimitActive() && !turnLimitDeadline)
		{
			turnLimitDeadline = turnTimeClock->startNewDeadlineFrom(turnTimeClock->getStartGameTime(), gameSettings->getTurnLimit());
		}
	});
};

//------------------------------------------------------------------------------
cModel::~cModel()
{
	if (map != nullptr)
	{
		map->reset();
	}
	for (auto attackjob : attackJobs)
	{
		delete attackjob;
	}
	for (auto movejob : moveJobs)
	{
		delete movejob;
	}
}

//------------------------------------------------------------------------------
void cModel::initGameId()
{
	if (gameId == 0)
	{
		while ((gameId = randomGenerator.get()) == 0);
	}
}

//------------------------------------------------------------------------------
void cModel::advanceGameTime()
{
	gameTime++;
	gameTimeChanged();

	runMoveJobs();
	runAttackJobs();
	effectsList.run();
	handleTurnEnd();
	helperJobs.run(*this);
}

//------------------------------------------------------------------------------
unsigned int cModel::getGameTime() const
{
	return gameTime;
}

//------------------------------------------------------------------------------
uint32_t cModel::getChecksum() const
{
	uint32_t crc = 0;
	//crc = calcCheckSum(gameTime, crc);
	crc = calcCheckSum(gameId, crc);
	crc = calcCheckSum(*gameSettings, crc);
	crc = calcCheckSum(*map, crc);
	for (const auto& player : playerList)
		crc = calcCheckSum(*player, crc);
	for (const auto& building : neutralBuildings)
		crc = calcCheckSum(*building, crc);
	crc = calcCheckSum(nextUnitId, crc);
	crc = calcCheckSum(*unitsData, crc);
	for (const auto& movejob : moveJobs)
		crc = calcCheckSum(*movejob, crc);
	for (const auto& attackJob : attackJobs)
		crc = calcCheckSum(*attackJob, crc);
	crc = calcCheckSum(*turnCounter, crc);
	crc = calcCheckSum(turnEndState, crc);
	crc = calcCheckSum(activeTurnPlayer->getId(), crc);
	crc = calcCheckSum(turnEndDeadline, crc);
	crc = calcCheckSum(turnLimitDeadline, crc);
	crc = calcCheckSum(*turnTimeClock, crc);
	crc = calcCheckSum(helperJobs, crc);

	return crc;
}
//------------------------------------------------------------------------------
void cModel::setGameSettings(const cGameSettings& gameSettings_)
{
	*gameSettings = gameSettings_;

	if (gameSettings->isTurnLimitActive())
	{
		turnLimitDeadline = turnTimeClock->startNewDeadlineFromNow(gameSettings->getTurnLimit());
	}
}
//------------------------------------------------------------------------------
void cModel::setMap(std::shared_ptr<cStaticMap> map_)
{
	map = std::make_shared<cMap>(map_);
	for (auto player : playerList)
		player->initMaps(*map);
}
//------------------------------------------------------------------------------
cPlayer* cModel::getPlayer(int playerNr)
{
	for (auto player : playerList)
	{
		if (player->getId() == playerNr)
			return player.get();
	}
	return nullptr;
}
//------------------------------------------------------------------------------
const cPlayer* cModel::getPlayer(int playerNr) const
{
	//TODO: remove code duplication
	for (auto player : playerList)
	{
		if (player->getId() == playerNr)
			return player.get();
	}
	return nullptr;
}
//------------------------------------------------------------------------------
const cPlayer* cModel::getPlayer(std::string player) const
{
	// first try to find player by number
	const int playerNr = atoi(player.c_str());
	if (playerNr != 0 || player[0] == '0')
	{
		return getPlayer(playerNr);
	}

	// try to find player by name
	for (auto p : playerList)
	{
		if (p->getName() == player)
			return p.get();
	}
	return nullptr;
}
//------------------------------------------------------------------------------
void cModel::setPlayerList(const std::vector<cPlayerBasicData>& splayers)
{
	assert(playerList.size() == 0);

	for (auto playerInfo : splayers)
	{
		auto player = std::make_shared<cPlayer>(playerInfo, *unitsData);
		if (map) player->initMaps(*map);
		playerList.push_back(player);

	}
	activeTurnPlayer = playerList[0].get();;
}

//------------------------------------------------------------------------------
const cPlayer* cModel::getActiveTurnPlayer() const
{
	return activeTurnPlayer;
}

//------------------------------------------------------------------------------
cVehicle& cModel::addVehicle(const cPosition& position, const sID& id, cPlayer* player)
{
	// TODO: This one is risky. Should do some checks
	const auto data = unitsData->getVehicle(id);
	// generate the vehicle:
	cVehicle& addedVehicle = player->addNewVehicle(position,data, nextUnitId);
	nextUnitId++;

	// place the vehicle:
	map->addVehicle(addedVehicle, position);
	player->addToScan(addedVehicle);

	// TODO: Make this action inside surveyior
	// scan with surveyor:
	if (addedVehicle.getStaticUnitData().hasFlag(UnitFlag::CanSurvey))
	{
		addedVehicle.doSurvey();
	}

	//TODO: take off animation?
	if (addedVehicle.canLand(*map))
	{
		addedVehicle.setFlightHeight(0);
	}
	else
	{
		addedVehicle.setFlightHeight(64);
	}

	addedVehicle.detectOtherUnits(*map);

	return addedVehicle;
}

//------------------------------------------------------------------------------
cBuilding& cModel::addBuilding(const cPosition& position, const sID& id, cPlayer* player)
{
	// TODO: This one is risky. Should do some checks
	auto data = unitsData->getBuilding(id);
	// Generate the building:
	cBuilding& addedBuilding = player->addNewBuilding(position, data, nextUnitId);
	nextUnitId++;

	addedBuilding.initMineRessourceProd(*map);

	//cBuilding* buildingToBeDeleted = map->getField(position).getTopBuilding();

	map->addBuilding(addedBuilding, position);
	player->addToScan(addedBuilding);

	// integrate the building to the base:
	player->base.addBuilding(&addedBuilding, *map);

	// if this is a top building, delete connectors, mines and roads
	if (addedBuilding.getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_GROUND)
	{
		for(cPosition& bigPosition: generateBorder(position, addedBuilding.getCellSize()))
		{
			auto buildings = &map->getField(bigPosition).getBuildings();
			// Iterate all over the border of the building
			for (size_t i = 0; i != buildings->size(); ++i)
			{
				if ((*buildings)[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit((*buildings)[i]);
					--i;
				}
			}
		}
#ifdef FUCK_THIS
		/// Old code for removing OVERBUILD_TYPE_YESNREMOVE objects
		if (addedBuilding.getIsBig())
		{
			auto bigPosition = position;
			auto buildings = &map->getField(bigPosition).getBuildings();
			// Iterate all over the border of the building
			for (size_t i = 0; i != buildings->size(); ++i)
			{
				if ((*buildings)[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit((*buildings)[i]);
					--i;
				}
			}
			bigPosition.x()++;
			buildings = &map->getField(bigPosition).getBuildings();
			for (size_t i = 0; i != buildings->size(); ++i)
			{
				if ((*buildings)[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit((*buildings)[i]);
					--i;
				}
			}
			bigPosition.y()++;
			buildings = &map->getField(bigPosition).getBuildings();
			for (size_t i = 0; i != buildings->size(); ++i)
			{
				if ((*buildings)[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit((*buildings)[i]);
					--i;
				}
			}
			bigPosition.x()--;
			buildings = &map->getField(bigPosition).getBuildings();
			for (size_t i = 0; i != buildings->size(); ++i)
			{
				if ((*buildings)[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit((*buildings)[i]);
					--i;
				}
			}
		}
		else
		{
			deleteUnit(buildingToBeDeleted);

			const auto& buildings = map->getField(position).getBuildings();
			for (size_t i = 0; i != buildings.size(); ++i)
			{
				if (buildings[i]->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE)
				{
					deleteUnit(buildings[i]);
					--i;
				}
			}
		}
#endif
	}

	if (addedBuilding.getStaticUnitData().canMineMaxRes > 0)
	{
		addedBuilding.startWork();
	}
	
	addedBuilding.detectOtherUnits(*map);

	return addedBuilding;
}

//------------------------------------------------------------------------------
void cModel::destroyUnit(cUnit& unit)
{
	addJob(new cDestroyJob(unit, *this));
}

//------------------------------------------------------------------------------
void cModel::addRubble(const cPosition& position, int value, int size)
{
	value = std::max(1, value);
	bool big = size > 1;

	// We should split rubble to several tiles
	// if the building is partly at the coast and above the water

	if (map->isWaterOrCoast(position))
	{
		if (big)
		{
			addRubble(position + cPosition(1, 0), value / 4, 1);
			addRubble(position + cPosition(0, 1), value / 4, 1);
			addRubble(position + cPosition(1, 1), value / 4, 1);
		}
		return;
	}

	if (big && map->isWaterOrCoast(position + cPosition(1, 0)))
	{
		addRubble(position, value / 4, 1);
		addRubble(position + cPosition(0, 1), value / 4, 1);
		addRubble(position + cPosition(1, 1), value / 4, 1);
		return;
	}

	if (big && map->isWaterOrCoast(position + cPosition(0, 1)))
	{
		addRubble(position, value / 4, 1);
		addRubble(position + cPosition(1, 0), value / 4, 1);
		addRubble(position + cPosition(1, 1), value / 4, 1);
		return;
	}

	if (big && map->isWaterOrCoast(position + cPosition(1, 1)))
	{
		addRubble(position, value / 4, 1);
		addRubble(position + cPosition(1, 0), value / 4, 1);
		addRubble(position + cPosition(0, 1), value / 4, 1);
		return;
	}

	std::shared_ptr<cBuilding> rubble;

	sBuildingDataPtr data;

#ifdef FIX_RUBBLE
	if (big)
	{
		data = std::dynamic_pointer_cast<sBuildingUIData>(unitsData->getRubbleBigData()->shared_from_this());
	}
	else
	{
		data = std::dynamic_pointer_cast<sBuildingUIData>(unitsData->getRubbleBigData()->shared_from_this());
	}
#endif
	if(data)
	{
		rubble = std::make_shared<cBuilding>(data, nullptr, nullptr, nextUnitId);

		nextUnitId++;

		rubble->setPosition(position);

		rubble->setRubbleValue(value, randomGenerator);

		map->addBuilding(*rubble, position);

		neutralBuildings.insert(std::move(rubble));
	}
}

//------------------------------------------------------------------------------
void cModel::deleteUnit(cUnit* unit)
{
	if (unit == 0)
		return;

	Log.write(" cModel: delete unit, id: " + iToStr(unit->getId()) + " @" + iToStr(getGameTime()), cLog::eLOG_TYPE_NET_DEBUG);

	if (unit->isABuilding() && static_cast<cBuilding*>(unit)->isRubble())
	{
		deleteRubble(static_cast<cBuilding*> (unit));
		return;
	}

	cPlayer* owner = unit->getOwner();

	//TODO
	//if (unit->getOwner() && casualtiesTracker != nullptr && ((unit->isABuilding() && unit->data.buildCosts <= 2) == false))
	//	casualtiesTracker->logCasualty(unit->data.ID, unit->getOwner()->getNr());

	std::shared_ptr<cUnit> owningPtr; // keep owning ptr to make sure that unit instance will outlive the following method.
	if (unit->isABuilding())
	{
		cBuilding* building = static_cast<cBuilding*> (unit);
		owningPtr = owner->removeUnit(*building);
	}
	else
	{
		cVehicle* vehicle = static_cast<cVehicle*> (unit);
		owningPtr = owner->removeUnit(*vehicle);
	}

	helperJobs.onRemoveUnit(unit);

	// detach from move job
	if (unit->isAVehicle())
	{
		cVehicle* vehicle = static_cast<cVehicle*> (unit);
		if (vehicle->getMoveJob())
		{
			assert(vehicle->getMoveJob()->getVehicle() == vehicle);
			vehicle->getMoveJob()->removeVehicle();
		}
	}
	if (unit->isAttacking())
	{
		for (auto attackJob : attackJobs)
		{
			attackJob->onRemoveUnit(*unit);
		}
	}

	// lose eco points
	if (unit->isABuilding() && static_cast<cBuilding*> (unit)->points != 0)
	{
		//unit->getOwner()->setScore(unit->getOwner()->getScore(turnClock->getTurn()) - static_cast<cBuilding*> (unit)->points, turnClock->getTurn());
		owner->countEcoSpheres();
	}

	if (unit->isABuilding())
		map->deleteBuilding(*static_cast<cBuilding*> (unit));
	else
		map->deleteVehicle(*static_cast<cVehicle*> (unit));

	if (unit->isABuilding() && static_cast<cBuilding*> (unit)->subBase != nullptr)
		owner->base.deleteBuilding(static_cast<cBuilding*> (unit), *map);

	if (owner != nullptr)
	{
		owner->deleteSentry(*unit);
		owner->removeFromScan(*unit);
	}
}
//------------------------------------------------------------------------------
void cModel::deleteRubble(cBuilding* rubble)
{
	assert(rubble->isRubble());

	map->deleteBuilding(*rubble);

	auto iter = neutralBuildings.find(*rubble);
	assert(iter != neutralBuildings.end());

	if (iter != neutralBuildings.end())
	{
		neutralBuildings.erase(iter);
	}
}

//------------------------------------------------------------------------------
cMoveJob* cModel::addMoveJob(cVehicle& vehicle, const cPosition& destination)
{
	cMapView mapView(map, nullptr);
	cPathCalculator pc(vehicle, mapView, destination, false);
	auto path = pc.calcPath();
	if (path.empty())
	{
		return nullptr;
	}

	return addMoveJob(vehicle, path);
}

//------------------------------------------------------------------------------
cMoveJob* cModel::addMoveJob(cVehicle& vehicle, const std::forward_list<cPosition>& path)
{
	cMoveJob* currentMoveJob = vehicle.getMoveJob();
	if (currentMoveJob)
	{
		if (currentMoveJob->isActive())
		{
			// cannot add movejob while the unit is already moving
			return nullptr;
		}
		else
		{
			// a waiting movejob can be replaced by new one
			currentMoveJob->stop();
			currentMoveJob->removeVehicle();
		}
	}

	cMoveJob* moveJob = new cMoveJob(path, vehicle, *this);
	vehicle.setMoveJob(moveJob);

	moveJobs.push_back(moveJob);

	return moveJob;
}

//------------------------------------------------------------------------------
std::vector<const cPlayer*> cModel::resumeMoveJobs(const cPlayer* player /*= nullptr*/)
{
	std::vector<const cPlayer*> players;
	for (const auto& moveJob : moveJobs)
	{
		if ((player && moveJob->getVehicle()->getOwner() != player) || !moveJob->getVehicle())
		{
			continue;
		}
		if (moveJob->isWaiting() && moveJob->getVehicle() && moveJob->getVehicle()->data.getSpeed() > 0)
		{
			moveJob->resume();
			players.push_back(moveJob->getVehicle()->getOwner());
		}
	}
	RemoveDuplicates(players);

	return players;
}

//------------------------------------------------------------------------------
void cModel::addAttackJob(cUnit& aggressor, const cPosition& targetPosition)
{
	cAttackJob* attackJob = new cAttackJob(aggressor, targetPosition, *this);
	attackJobs.push_back(attackJob);
}

//------------------------------------------------------------------------------
void cModel::handlePlayerFinishedTurn(cPlayer& player)
{
	player.setHasFinishedTurn(true);

	if (gameSettings->getGameType() != eGameSettingsGameType::Turns && gameSettings->isTurnEndDeadlineActive() && !turnEndDeadline)
	{
		turnEndDeadline = turnTimeClock->startNewDeadlineFromNow(gameSettings->getTurnEndDeadline());
	}

	playerFinishedTurn(player);
}

//------------------------------------------------------------------------------
void cModel::addFx(std::shared_ptr<cFx> fx)
{
	effectsList.push_back(fx);
	addedEffect(fx);
}

//------------------------------------------------------------------------------
void cModel::addJob(cJob* job)
{
	helperJobs.addJob(*job);
}

//------------------------------------------------------------------------------
std::shared_ptr<const cTurnTimeClock> cModel::getTurnTimeClock() const
{
	return turnTimeClock;
}

//------------------------------------------------------------------------------
std::shared_ptr<const cTurnCounter> cModel::getTurnCounter() const
{
	return turnCounter;
}

//------------------------------------------------------------------------------
cUnit* cModel::getUnitFromID(unsigned int id) const
{
	cUnit* result = getVehicleFromID(id);
	if (result == nullptr)
		result = getBuildingFromID(id);
	return result;
}

//------------------------------------------------------------------------------
cVehicle* cModel::getVehicleFromID(unsigned int id) const
{
	for (size_t i = 0; i != playerList.size(); ++i)
	{
		auto unit = playerList[i]->getVehicleFromId(id);
		if (unit) return unit;
	}
	return 0;
}

//------------------------------------------------------------------------------
cBuilding* cModel::getBuildingFromID(unsigned int id) const
{
	for (size_t i = 0; i != playerList.size(); ++i)
	{
		auto unit = playerList[i]->getBuildingFromId(id);
		if (unit) return unit;
	}

	auto iter = neutralBuildings.find(id);
	return iter == neutralBuildings.end() ? nullptr : iter->get();
}

//------------------------------------------------------------------------------
void cModel::refreshMapPointer()
{
	map->reset();
	for (const auto& player : playerList)
	{
		for (const auto& vehicle : player->getVehicles())
		{
			if (!vehicle->isUnitLoaded())
				map->addVehicle(*vehicle, vehicle->getPosition());
		}
		for (const auto& building : player->getBuildings())
		{
			map->addBuilding(*building, building->getPosition());
		}
	}
	for (const auto& building : neutralBuildings)
	{
		map->addBuilding(*building, building->getPosition());
	}
}

//------------------------------------------------------------------------------
void cModel::runMoveJobs()
{
	const int max = moveJobs.size();
	for (int i = 0; i < max; i++)
	{
		auto moveJob = moveJobs[i];
		moveJob->run(*this); //this can add new items to 'moveJobs'
		if (moveJob->isFinished())
		{
			cVehicle* vehicle = moveJob->getVehicle();
			if (vehicle != nullptr && vehicle->getMoveJob() == moveJob)
			{
				vehicle->setMoveJob(nullptr);
			}
			delete moveJob;
			moveJobs[i] = nullptr;
		}
	}
	Remove(moveJobs, nullptr);
}

//------------------------------------------------------------------------------
void cModel::runAttackJobs()
{
	auto attackJobsTemp = attackJobs;
	for (auto attackJob : attackJobsTemp)
	{
		attackJob->run(*this); //this can add new items to 'attackjobs'
		if (attackJob->finished())
		{
			delete attackJob;
			attackJobs.erase(std::find(attackJobs.begin(), attackJobs.end(), attackJob));
		}
	}
}

//------------------------------------------------------------------------------
void cModel::handleTurnEnd()
{
	switch (turnEndState)
	{
	case TURN_ACTIVE:
		{
			bool turnFinished = true;
			if (gameSettings->getGameType() == eGameSettingsGameType::Turns)
			{
				if (!activeTurnPlayer->getHasFinishedTurn())
				{
					turnFinished = false;
				}
			}
			else
			{
				for (const auto& player : playerList)
				{
					if (!player->getHasFinishedTurn() && !player->isDefeated)
					{
						turnFinished = false;
					}
				}
			}
			if (turnFinished || turnTimeClock->hasReachedAnyDeadline())
			{
				turnEnded();

				const auto player = gameSettings->getGameType() == eGameSettingsGameType::Turns ? activeTurnPlayer : nullptr;
				const auto resumedMJobOwners = resumeMoveJobs(player);
				for (const auto& player : resumedMJobOwners)
				{
					player->turnEndMovementsStarted();
				}

				turnEndState = EXECUTE_REMAINING_MOVEMENTS;
			}
		}
		break;
	case EXECUTE_REMAINING_MOVEMENTS:
		{
			bool activeMoveJob = false;
			for (const auto& moveJob : moveJobs)
			{
				if (moveJob->isActive())
				{
					activeMoveJob = true;
				}
			}
			if (!activeMoveJob)
			{
				turnEndState = EXECUTE_TURN_START;
			}
		}
		break;
	case EXECUTE_TURN_START:
		{
			if (gameSettings->getGameType() == eGameSettingsGameType::Turns)
			{
				// select next player
				//TODO: skip defeated player?
				auto nextPlayerIter = std::find_if(playerList.begin(), playerList.end(), [this](const std::shared_ptr<cPlayer>& player) {return player.get() == activeTurnPlayer; });
				assert(nextPlayerIter != playerList.end());
				++nextPlayerIter;
				if (nextPlayerIter == playerList.end())
				{
					activeTurnPlayer = playerList.front().get();
				}
				else
				{
					activeTurnPlayer = nextPlayerIter->get();
				}

				if (activeTurnPlayer == playerList.front().get())
				{
					//TODO: checkDefeats();
					turnCounter->increaseTurn();
				}

				if (turnCounter->getTurn() > 1)
				{
					// don't execute turn start action in turn 1, because model is already completely initialized for turn 1
					activeTurnPlayer->makeTurnStart(*this);
				}
			}
			else
			{
				turnCounter->increaseTurn();

				for (auto& player : playerList)
				{
					player->makeTurnStart(*this);
				}

			}

			turnTimeClock->restartFromNow();
			turnTimeClock->clearAllDeadlines();
			turnEndDeadline = 0;
			turnLimitDeadline = 0;
			if (gameSettings->isTurnLimitActive())
			{
				turnLimitDeadline = turnTimeClock->startNewDeadlineFromNow(gameSettings->getTurnLimit());
			}

			turnEndState = TURN_ACTIVE;
			newTurnStarted();
		}
		break;
	}
}

//------------------------------------------------------------------------------
void cModel::setUnitsData(std::shared_ptr<cUnitsData> unitsData_)
{
	unitsData = unitsData_;
}

//------------------------------------------------------------------------------
void cModel::sideStepStealthUnit(const cPosition& position, const cVehicle& vehicle, const cPosition& bigOffset)
{
	sideStepStealthUnit(position, vehicle.getStaticUnitData(), vehicle.getOwner(), bigOffset);
}

//------------------------------------------------------------------------------
void cModel::sideStepStealthUnit(const cPosition& position, const cStaticUnitData& vehicleData, cPlayer* vehicleOwner, const cPosition& bigOffset)
{
	if (map->possiblePlaceVehicle(vehicleData, position, nullptr))
	{
		return;
	}

	if (vehicleData.factorAir > 0) return;

	// first look for an undetected stealth unit
	cVehicle* stealthVehicle = map->getField(position).getVehicle();
	if (!stealthVehicle) return;
	if (stealthVehicle->getOwner() == vehicleOwner) return;
	if (stealthVehicle->getStaticUnitData().isStealthOn == TERRAIN_NONE) return;
	if (stealthVehicle->isDetectedByPlayer(vehicleOwner)) return;

	if (stealthVehicle->isUnitMoving())
	{
		stealthVehicle->setDetectedByPlayer(vehicleOwner);
		return;
	}

	// found a stealth unit. Try to find a place where the unit can move
	bool placeFound = false;
	int minCosts = 99;
	cPosition bestPosition;
	const int minx = std::max(position.x() - 1, 0);
	const int maxx = std::min(position.x() + 1, map->getSize().x() - 1);
	const int miny = std::max(position.y() - 1, 0);
	const int maxy = std::min(position.y() + 1, map->getSize().y() - 1);
	for (int x = minx; x <= maxx; ++x)
	{
		for (int y = miny; y <= maxy; ++y)
		{
			const cPosition currentPosition(x, y);
			if (currentPosition == position) continue;

			// when a bigOffet was passed,
			// for example a constructor needs space for a big building
			// so not all directions are allowed for the side stepping
			if (bigOffset != -1)
			{
				if (currentPosition == bigOffset ||
					currentPosition == bigOffset + cPosition(1, 0) ||
					currentPosition == bigOffset + cPosition(0, 1) ||
					currentPosition == bigOffset + cPosition(1, 1)) continue;
			}

			// check whether this field is a possible destination
			if (!map->possiblePlace(*stealthVehicle, currentPosition, false)) continue;

			// check costs of the move
			int costs = cPathCalculator::calcNextCost(position, currentPosition, stealthVehicle, map.get());
			if (costs > stealthVehicle->data.getSpeed()) continue;

			// check whether the vehicle would be detected
			// on the destination field
			bool detectOnDest = false;
			if (stealthVehicle->getStaticUnitData().isStealthOn & TERRAIN_GROUND)
			{
				for (size_t i = 0; i != playerList.size(); ++i)
				{
					if (playerList[i].get() == stealthVehicle->getOwner()) continue;
					if (playerList[i]->hasLandDetection(currentPosition)) detectOnDest = true;
				}
				if (map->isWater(currentPosition)) detectOnDest = true;
			}
			if (stealthVehicle->getStaticUnitData().isStealthOn & TERRAIN_SEA)
			{
				for (size_t i = 0; i != playerList.size(); ++i)
				{
					if (playerList[i].get() == stealthVehicle->getOwner()) continue;
					if (playerList[i]->hasSeaDetection(currentPosition)) detectOnDest = true;
				}
				if (!map->isWater(currentPosition)) detectOnDest = true;

				if (stealthVehicle->getStaticUnitData().factorGround > 0)
				{
					if (map->getField(currentPosition).hasBridgeOrPlattform())
					{
						detectOnDest = true;
					}
				}
			}
			if (detectOnDest) continue;

			// take the move with the lowest costs.
			// Decide randomly, when costs are equal
			if (costs < minCosts || (costs == minCosts && randomGenerator.get(2)))
			{
				// this is a good candidate for a destination
				minCosts = costs;
				bestPosition = currentPosition;
				placeFound = true;
			}
		}
	}

	if (placeFound)
	{
		std::forward_list<cPosition> path;
		path.push_front(bestPosition);
		addMoveJob (*stealthVehicle, path);
		return;
	}

	// sidestepping failed. Uncover the vehicle.
	stealthVehicle->setDetectedByPlayer(vehicleOwner);
}
