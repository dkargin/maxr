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

#include <set>

#include "actioninitnewgame.h"
#include "game/data/model.h"
#include "game/data/gamesettings.h"
#include "game/data/map/map.h"
#include "utility/log.h"
#include "game/data/player/player.h"
#include "utility/listhelpers.h"
#include "game/data/player/clans.h"
#include "utility/string/toString.h"

//------------------------------------------------------------------------------
cActionInitNewGame::cActionInitNewGame() :
	cAction(eActiontype::ACTION_INIT_NEW_GAME)
{}

//------------------------------------------------------------------------------
cActionInitNewGame::cActionInitNewGame(cBinaryArchiveOut& archive)
	: cAction(eActiontype::ACTION_INIT_NEW_GAME)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionInitNewGame::execute(cModel& model) const
{
	//Note: this funktion handels incoming data from network. Make every possible sanity check!

	model.initGameId();

	cPlayer& player = *model.getPlayer(playerNr);

	const cUnitsData& unitsdata = *model.getUnitsData();
	landingConfig.loadUnitsData(unitsdata);

	player.removeAllUnits();
	Log.write(" GameId: " + toString(model.getGameId()), cLog::eLOG_TYPE_NET_DEBUG);

	// init clan
	if (model.getGameSettings()->getClansEnabled())
	{
		if (clan < 0 || static_cast<size_t>(clan) >= unitsdata.getNrOfClans())
		{
			Log.write(" Landing failed. Invalid clan number.", cLog::eLOG_TYPE_NET_ERROR);
			return;
		}
		player.setClan(clan, unitsdata);
	}
	else
	{
		player.setClan(-1, unitsdata);
	}

	// init landing position
	if (!model.getMap()->isValidPosition(landingConfig.landingPosition))
	{
		Log.write(" Received invalid landing position", cLog::eLOG_TYPE_NET_ERROR);
		return;
	}

	cPosition updatedLandingPosition = landingConfig.landingPosition;
	if (model.getGameSettings()->getBridgeheadType() == eGameSettingsBridgeheadType::Definite)
	{
		// Find place for mine if bridgehead is fixed
		if (!findPositionForLayout(updatedLandingPosition, landingConfig.baseLayout, *model.getMap()->staticMap))
		{
			Log.write("couldn't place player start mine: " + player.getName(), cLog::eLOG_TYPE_NET_ERROR);
			return;
		}
	}

	player.setLandingPos(updatedLandingPosition);

	// place new ressources
	model.getMap()->placeRessources(model);

	// apply upgrades
	int credits = model.getGameSettings()->getStartCredits();
	for (const auto& upgrade : landingConfig.unitUpgrades)
	{
		const sID& unitId = upgrade.first;
		const cUnitUpgrade& upgradeValues = upgrade.second;

		if (!unitsdata.isValidId(unitId))
		{
			Log.write(" Apply upgrades failed. Unknown sID: " + unitId.getText(), cLog::eLOG_TYPE_NET_ERROR);
			return;
		}
		int costs = upgradeValues.calcTotalCosts(unitsdata.getDynamicData(unitId, player.getClan()), *player.getUnitDataCurrentVersion(unitId), player.getResearchState());
		if (costs <= 0)
		{
			Log.write(" Apply upgrades failed. Couldn't calculate costs.", cLog::eLOG_TYPE_NET_ERROR);
			return;
		}
		credits -= costs;
		if (credits <= 0)
		{
			Log.write(" Apply upgrade failed. Used more than the available credits.", cLog::eLOG_TYPE_NET_ERROR);
			return;
		}
		upgradeValues.updateUnitData(*player.getUnitDataCurrentVersion(unitId));
	}

	// Recalculation of credits. Do we need it right here in such a way?
	for (const auto& landing : landingConfig.landingUnits)
	{

		if (!unitsdata.isValidId(landing.unitID))
		{
			Log.write(" Landing failed. Unknown sID: " + landing.unitID.getText(), cLog::eLOG_TYPE_NET_ERROR);
			return;
		}

		#ifdef FUCK_THIS
		auto it = std::find_if(initialLandingUnits.begin(), initialLandingUnits.end(),
							   [landing](std::pair<sID, int> unit){ return unit.first == landing.unitID; });

		if (it != initialLandingUnits.end())
		{
			// landing unit is one of the initial landing units, that the player gets for free
			// so don't pay for the unit and the default cargo
			credits -= (landing.cargo - it->second) / 5;
			initialLandingUnits.erase(it);
		}
		else
		{
			credits -= landing.cargo / 5;
			credits -= unitsdata.getDynamicUnitData(landing.unitID, clan).getBuildCost();
		}
		#endif
	}

	if (credits < 0)
	{
		Log.write(" Landing failed. Used more than the available credits", cLog::eLOG_TYPE_ERROR);
		return;
	}

	makeLanding(player, landingConfig, model);

	//transfer remaining credits to player
	player.setCredits(credits);
}


struct sPositionUtils
{
	// Comparator
	bool operator()(const cPosition& a, const cPosition& b) const
	{
		if (a.x() == b.x())
			return a.y() < b.y();
		return a.x() < b.x();
	}
};

class cPositionSet : public std::set<cPosition, sPositionUtils>
{
public:
	typedef std::set<cPosition, sPositionUtils> set_type;

	// Check if object with specified size fits to this position
	bool fits(const cPosition& pos, int size) const
	{
		for(int y = pos.y(); y < pos.y() + size; y++)
		{
			for(int x = pos.x(); x < pos.x() + size; x++)
			{
				if(this->count(cPosition(x,y)))
					return false;
			}
		}
		return true;
	}

	// Add positions that occupied by object with specified size
	void insert(const cPosition& pos, int size)
	{
		for(int y = pos.y(); y < pos.y() + size; y++)
			for(int x = pos.x(); x < pos.x() + size; x++)
				this->set_type::insert(cPosition(x, y));
	}
};

bool cActionInitNewGame::isValidLandingPosition(cPosition position, const cStaticMap& map, bool fixedBridgeHead, const sLandingConfig& config, const cUnitsData& unitsData)
{
	cPositionSet blockedPositions;

	if (fixedBridgeHead)
	{
		bool found = findPositionForLayout(position, config.baseLayout, map);

		if (!found)
			return false;

		for(const auto& item: config.baseLayout)
		{
			blockedPositions.insert(item.pos + position, item.data->cellSize);
		}
	}

	int maxRaduis = 6;
	float maxRadiusMult = 3.0; 	// 1.5

	for (const auto& unit : config.landingUnits)
	{
		const cStaticUnitData& unitData = *unitsData.getUnit(unit.unitID);
		bool placed = false;
		int landingRadius = 1;

		while (!placed)
		{
			landingRadius += 1;

			// prevent, that units are placed far away from the starting position
			if (landingRadius > maxRadiusMult*sqrt(config.landingUnits.size()) && landingRadius > maxRaduis)
				return false;

			for (int offY = -landingRadius; (offY < landingRadius) && !placed; ++offY)
			{
				for (int offX = -landingRadius; (offX < landingRadius) && !placed; ++offX)
				{
					const cPosition place = position + cPosition(offX, offY);
					bool fitsMap = map.possiblePlace(unitData, place);

					bool fitsBlocks = blockedPositions.fits(place, unitData.cellSize);
					if(fitsMap && fitsBlocks)
					{
						blockedPositions.insert(place, unitData.cellSize);
						placed = true;
					}
				}
			}
		}
	}

	return true;
}

//------------------------------------------------------------------------------
void cActionInitNewGame::makeLanding(cPlayer& player, const sLandingConfig& landingConfig, cModel& model) const
{
	cMap& map = *model.getMap();
	cPosition landingPosition = player.getLandingPos();

	if (model.getGameSettings()->getBridgeheadType() == eGameSettingsBridgeheadType::Definite)
	{
		// place buildings:
		for(const auto& item: landingConfig.baseLayout)
		{
			if(item.data)
				model.addBuilding(landingPosition + item.pos, item.data->ID, &player);
		}

		// Explore area under created buildings
		if(landingConfig.exploreLanding >= 0)
		{
			for(const auto& item: landingConfig.baseLayout)
			{
				cPosition pos = landingPosition + item.pos;
				cPosition min = pos - landingConfig.exploreLanding;
				cPosition max = pos + item.data->cellSize + landingConfig.exploreLanding;

				for(int y = min.y(); y < max.y(); y++)
					for(int x = min.x(); x < max.x(); x++)
					{
						cPosition toSurvey(x,y);
						if(map.isValidPosition(toSurvey))
						{
							player.exploreResource(toSurvey);
						}
					}
			}
		}
	}

	for (const sLandingUnit& landing: landingConfig.landingUnits)
	{
		cVehicle* vehicle = nullptr;
		int radius = 1;

		if (!model.getUnitsData()->isValidId(landing.unitID))
		{
			Log.write(" Landing of unit failed. Unknown sID: " + landing.unitID.getText(), cLog::eLOG_TYPE_NET_ERROR);
			continue;
		}

		while (!vehicle)
		{
			vehicle = landVehicle(landingPosition, radius, landing.unitID, player, model);

			if(vehicle)
			{
				std::stringstream ss;
				cPosition pos = vehicle->getPosition() - landingPosition;
				ss << "Landed vehicle " << landing.unitID.getText() << " to (" << pos.x() << ";" << pos.y() << ")";
				Log.write(ss.str());
			}
			radius += 1;
		}

		if(vehicle == nullptr)
		{
			Log.write(" Landing of unit failed. Invalid location", cLog::eLOG_TYPE_NET_ERROR);
		}

		if (landing.cargo && vehicle)
		{
			if (vehicle->getStaticUnitData().storeResType != eResourceType::Gold)
				vehicle->setStoredResources(landing.cargo);
		}
	}
}
//------------------------------------------------------------------------------
cVehicle* cActionInitNewGame::landVehicle(const cPosition& landingPosition, int radius, const sID& id, cPlayer& player, cModel& model) const
{
	const cMap& map = *model.getMap();
	const cStaticUnitData& unitData = *model.getUnitsData()->getUnit(id);

	for (int offY = -radius; offY < radius; ++offY)
	{
		for (int offX = -radius; offX < radius; ++offX)
		{
			cPosition pos = landingPosition.relative(offX, offY);
			if (!map.possiblePlaceVehicle(unitData, pos, &player))
				continue;

			return &model.addVehicle(pos, id, &player);
		}
	}
	return nullptr;
}

bool cActionInitNewGame::findPositionForLayout(cPosition& position,
											   const std::vector<cBaseLayoutItem>& layout,
											   const cStaticMap& map, int maxRadius)
{
	int radius = 0;

	while (true)
	{
		for (int offY = -radius; offY <= radius; ++offY)
		{
			for (int offX = -radius; offX <= radius; ++offX)
			{
				const cPosition place = position.relative(offX, offY);

				bool allFit = true;
				for(const auto& item: layout)
				{
					assert(item.data != nullptr);
					if(!map.possiblePlace(*item.data, place+item.pos))
					{
						allFit = false;
						break;
					}
				}

				if ( allFit == true )
				{
					position = place;
					return true;
				}
			}
		}

		radius += 1;

		if (radius > maxRadius)
		{
			return false;
		}
	}
}
