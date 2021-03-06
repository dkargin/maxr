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

#include "game/startup/local/singleplayer/localsingleplayergamenew.h"
#include "game/data/gamesettings.h"
#include "ui/graphical/application.h"
#include "game/logic/client.h"
#include "game/logic/server.h"
#include "game/logic/server2.h"
#include "game/data/player/player.h"
#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/data/units/landingunit.h"
#include "game/logic/clientevents.h" //TODO: remove
#include "game/logic/action/actioninitnewgame.h"

//------------------------------------------------------------------------------
cLocalSingleplayerGameNew::cLocalSingleplayerGameNew()
{
    landingConfig.reset(new sLandingConfig());
}

//------------------------------------------------------------------------------
void cLocalSingleplayerGameNew::start (cApplication& application)
{
	assert (gameSettings != nullptr);
	auto connectionManager = std::make_shared<cConnectionManager>();

	server = std::make_unique<cServer2>(connectionManager);
	client = std::make_shared<cClient>(connectionManager);

	client->setMap (staticMap);
	server->setMap (staticMap);

	server->setUnitsData(unitsData); //TODO: don't copy unitsData here, but move cGame::unitsData?
	client->setUnitsData(unitsData);

	client->setGameSettings (*gameSettings);
	server->setGameSettings (*gameSettings);

	auto player = createPlayer();
	std::vector<cPlayerBasicData> players;
	players.push_back (player);
	client->setPlayers (players, 0);
	server->setPlayers(players);

	connectionManager->setLocalServer(server.get());
	connectionManager->setLocalClient(client.get(), 0);

	server->start();

	cActionInitNewGame action;
	action.clan = clan;
    action.landingConfig = *landingConfig;
    client->sendNetMessage(action);

	gameGuiController = std::make_unique<cGameGuiController> (application, staticMap);

	gameGuiController->setSingleClient (client);
	gameGuiController->setServer(server.get());

	cGameGuiState playerGameGuiState;
    playerGameGuiState.setMapPosition (landingConfig->landingPosition);
	gameGuiController->addPlayerGameGuiState (0, std::move (playerGameGuiState));

	gameGuiController->start();

	resetTerminating();

	application.addRunnable (shared_from_this());

	signalConnectionManager.connect(gameGuiController->terminated, [&]() { exit(); });
}

//------------------------------------------------------------------------------
void cLocalSingleplayerGameNew::setGameSettings (std::shared_ptr<cGameSettings> gameSettings_)
{
	gameSettings = gameSettings_;
}

//------------------------------------------------------------------------------
std::shared_ptr<cGameSettings> cLocalSingleplayerGameNew::getGameSettings()
{
    return gameSettings;
}

//------------------------------------------------------------------------------
void cLocalSingleplayerGameNew::setStaticMap (std::shared_ptr<cStaticMap> staticMap_)
{
	staticMap = staticMap_;
}

//------------------------------------------------------------------------------
std::shared_ptr<cStaticMap> cLocalSingleplayerGameNew::getStaticMap ()
{
    return staticMap;
}

//------------------------------------------------------------------------------
void cLocalSingleplayerGameNew::setPlayerClan (int clan)
{
    this->clan = clan;
}

int cLocalSingleplayerGameNew::getPlayerClan() const
{
    return this->clan;
}

std::shared_ptr<sLandingConfig> cLocalSingleplayerGameNew::getLandingConfig()
{
    return landingConfig;
}

//------------------------------------------------------------------------------
cPlayerBasicData cLocalSingleplayerGameNew::createPlayer()
{
	cPlayerBasicData player (cSettings::getInstance().getPlayerName(), cPlayerColor(), 0);

	return player;
}
