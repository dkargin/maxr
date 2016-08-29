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

#include "action.h"
#include "game/data/model.h"
#include "game/data/gamesettings.h"
#include "game/data/map/map.h"
#include "utility/log.h"
#include "game/data/player/player.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/building.h"
#include "utility/string/toString.h"

std::unique_ptr<cAction> cAction::createFromBuffer(cBinaryArchiveOut& archive)
{
	eActiontype type;
	archive >> type;

	switch (type)
	{
	case eActiontype::ACTION_INIT_NEW_GAME:
		return std::make_unique<cActionInitNewGame>(archive);
	default:
		//TODO: to throw or not to throw...
		Log.write("Unknown action type " + iToStr(static_cast<int>(type)), cLog::eLOG_TYPE_NET_ERROR);
		return nullptr;
	}
}

//------------------------------------------------------------------------------
std::string enumToString(cAction::eActiontype value)
{
	switch (value)
	{
	case cAction::eActiontype::ACTION_INIT_NEW_GAME:
		return "ACTION_INIT_NEW_GAME";
		break;
	default:
		assert(false);
		return toString(static_cast<int>(value));
		break;
	}
}

//------------------------------------------------------------------------------
cActionInitNewGame::cActionInitNewGame() : 
	cAction(eActiontype::ACTION_INIT_NEW_GAME), 
	clan(-1)
{};

//------------------------------------------------------------------------------
cActionInitNewGame::cActionInitNewGame(cBinaryArchiveOut& archive)
	: cAction(eActiontype::ACTION_INIT_NEW_GAME)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionInitNewGame::execute(cModel& model) const
{
	//TODO: clan
	//TODO: upgrades
	//TODO: copy credits
	//TODO: delete all units
	//TODO: Fehlerprüfung der empfangenen Nachricht
	cPlayer& player = *model.getPlayer(playerNr);
	player.setLandingPos(landingPosition);
	makeLanding(landingPosition, player, landingUnits, model);
	model.getMap()->placeRessources(model);
}

//------------------------------------------------------------------------------
void cActionInitNewGame::makeLanding(const cPosition& landingPosition, cPlayer& player, const std::vector<sLandingUnit>& landingUnits, cModel& model) const
{
	cMap& map = *model.getMap();

	// Find place for mine if bridgehead is fixed
	if (model.getGameSettings()->getBridgeheadType() == eGameSettingsBridgeheadType::Definite)
	{ //
		if (map.possiblePlaceBuilding(model.getUnitsData()->getSmallGeneratorData(), landingPosition + cPosition(-1, 0)) &&
			map.possiblePlaceBuilding(model.getUnitsData()->getMineData(), landingPosition + cPosition(0, -1)) &&
			map.possiblePlaceBuilding(model.getUnitsData()->getMineData(), landingPosition + cPosition(1, -1)) &&
			map.possiblePlaceBuilding(model.getUnitsData()->getMineData(), landingPosition + cPosition(1, 0)) &&
			map.possiblePlaceBuilding(model.getUnitsData()->getMineData(), landingPosition + cPosition(0, 0)))
		{
			// place buildings:
			model.addBuilding(landingPosition + cPosition(-1, 0), model.getUnitsData()->specialIDSmallGen, &player, true);
			model.addBuilding(landingPosition + cPosition(0, -1), model.getUnitsData()->specialIDMine, &player, true);
		}
		else
		{
			Log.write("couldn't place player start mine: " + player.getName(), cLog::eLOG_TYPE_ERROR);
		}
	}

	int iWidth = 2;
	int iHeight = 2;
	for (size_t i = 0; i != landingUnits.size(); ++i)
	{
		const sLandingUnit& landing = landingUnits[i];
		cVehicle* vehicle = landVehicle(landingPosition, iWidth, iHeight, landing.unitID, player, model);
		while (!vehicle)
		{
			iWidth += 2;
			iHeight += 2;
			vehicle = landVehicle(landingPosition, iWidth, iHeight, landing.unitID, player, model);
		}
		if (landing.cargo && vehicle)
		{
			vehicle->setStoredResources(landing.cargo);
		}
	}
}
//------------------------------------------------------------------------------
cVehicle* cActionInitNewGame::landVehicle(const cPosition& landingPosition, int iWidth, int iHeight, const sID& id, cPlayer& player, cModel& model) const
{
	for (int offY = -iHeight / 2; offY < iHeight / 2; ++offY)
	{
		for (int offX = -iWidth / 2; offX < iWidth / 2; ++offX)
		{
			if (!model.getMap()->possiblePlaceVehicle(model.getUnitsData()->getStaticUnitData(id), landingPosition + cPosition(offX, offY), &player)) continue;

			return &model.addVehicle(landingPosition + cPosition(offX, offY), id, &player, true);
		}
	}
	return nullptr;
}
