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

#include "game/startup/local/hotseat/localhotseatgamenew.h"
#include "game/data/gamesettings.h"
#include "ui/graphical/application.h"
#include "game/logic/client.h"
#include "game/logic/server.h"
#include "game/data/player/player.h"
#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/landingunit.h"
#include "game/logic/clientevents.h"

//------------------------------------------------------------------------------
cLocalHotSeatGameNew::cLocalHotSeatGameNew()
{}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::start (cApplication& application)
{
	assert (gameSettings != nullptr);

	server->setMap (staticMap);

	server->setGameSettings (*gameSettings);

	clients.resize (playersData.size());

	std::vector<cPlayerBasicData> players;
	for (size_t i = 0; i < playersData.size(); ++i)
	{
		clients[i]->setGameSettings (*gameSettings);
	}

	server->start();

	gameGuiController = std::make_unique<cGameGuiController> (application, staticMap);

	for (size_t i = 0; i < playersData.size(); ++i)
	{
		clients[i]->setPlayers (players, i);

		auto& clientPlayer = clients[i]->getActivePlayer();

		cGameGuiState gameGuiState;
        gameGuiState.setMapPosition (playersData[i].config->landingPosition);
		gameGuiController->addPlayerGameGuiState (clientPlayer.getId(), gameGuiState);
	}

	//server->startTurnTimers();

	auto activePlayer = server->getActiveTurnPlayer();
	assert (activePlayer != nullptr);

	gameGuiController->setClients (clients, activePlayer->getId());

	gameGuiController->start();

//	terminate = false;

	application.addRunnable (shared_from_this());
}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::setGameSettings (std::shared_ptr<cGameSettings> gameSettings_)
{
	gameSettings = gameSettings_;
}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::setStaticMap (std::shared_ptr<cStaticMap> staticMap_)
{
	staticMap = staticMap_;
}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::setPlayers (const std::vector<cPlayerBasicData>& players)
{
	playersData.clear();
	playersData.resize (players.size());
	for (size_t i = 0; i < players.size(); ++i)
	{
		playersData[i].basicData = players[i];
        playersData[i].config.reset(new sLandingConfig());
	}
}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::setPlayerClan (size_t playerIndex, int clan)
{
    playersData[playerIndex].clan = clan;
}

//------------------------------------------------------------------------------
void cLocalHotSeatGameNew::setLandingPosition (size_t playerIndex, const cPosition& landingPosition_)
{
    playersData[playerIndex].config->landingPosition = landingPosition_;
}

//------------------------------------------------------------------------------
const std::shared_ptr<cStaticMap>& cLocalHotSeatGameNew::getStaticMap()
{
	return staticMap;
}

//------------------------------------------------------------------------------
const std::shared_ptr<cGameSettings>& cLocalHotSeatGameNew::getGameSettings()
{
	return gameSettings;
}

//------------------------------------------------------------------------------
const std::vector<sLandingUnit>& cLocalHotSeatGameNew::getLandingUnits(size_t playerIndex)
{
    return playersData[playerIndex].config->landingUnits;
}

std::shared_ptr<sLandingConfig> cLocalHotSeatGameNew::getLandingConfig(size_t playerIndex)
{
    return playersData[playerIndex].config;
}

//------------------------------------------------------------------------------
size_t cLocalHotSeatGameNew::getPlayerCount() const
{
	return playersData.size();
}

//------------------------------------------------------------------------------
const cPlayerBasicData& cLocalHotSeatGameNew::getPlayer (size_t playerIndex) const
{
	return playersData[playerIndex].basicData;
}

//------------------------------------------------------------------------------
int cLocalHotSeatGameNew::getPlayerClan (size_t playerIndex) const
{
    return playersData[playerIndex].clan;
}
