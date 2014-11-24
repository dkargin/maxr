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
#ifndef menueventsH
#define menueventsH

#include <string>
#include <vector>

#include "game/logic/landingpositionstate.h"
#include "network.h"

class cTCP;
class cStaticMap;
class cPlayerBasicData;
class cGameSettings;
class cPosition;

enum eMenuMessages
{
	MU_MSG_CHAT = FIRST_MENU_MESSAGE,	// simple text message
	MU_MSG_REQ_IDENTIFIKATION,	// host requests a identification of this player
	MU_MSG_PLAYER_NUMBER,		// host assigns a new player number to a player
	MU_MSG_IDENTIFIKATION,		// player send his identification
	MU_MSG_PLAYERLIST,			// a list with all players and their data
	MU_MSG_OPTINS,				// all options selected by the host
	// Map down/up-load
	MU_MSG_START_MAP_DOWNLOAD,    // the host start a map upload to the client
	MU_MSG_MAP_DOWNLOAD_DATA,     // the host sends map data to the client
	MU_MSG_CANCELED_MAP_DOWNLOAD, // the host canceled the map upload to the client
	MU_MSG_FINISHED_MAP_DOWNLOAD, // the host finished uploading the map
	MU_MSG_REQUEST_MAP,           // a player wants to download a map from the server
	// Game Preparation
	MU_MSG_GO,                  // host wants to start the game/preparation
	MU_MSG_LANDING_STATE,       // informs a client about the state of the landing position selection he is currently in
	MU_MSG_LANDING_POSITION,	// landing position during landing position selection
	MU_MSG_ALL_LANDED,          // all players have selected there landing points and clients can start game
	MU_MSG_IN_LANDING_POSITION_SELECTION_STATUS,
	MU_MSG_PLAYER_HAS_SELECTED_LANDING_POSITION
};

void sendMenuChatMessage (cTCP& network, const std::string& chatMsg, const cPlayerBasicData* player = NULL, int fromPlayerNr = -1, bool translationText = false);

void sendRequestIdentification (cTCP& network, const cPlayerBasicData& player);

void sendPlayerNumber (cTCP& network, const cPlayerBasicData& player);

void sendPlayerList (cTCP& network, const std::vector<std::shared_ptr<cPlayerBasicData>>& players);

void sendGameData(cTCP& network, const cStaticMap* map, const cGameSettings* settings, const std::vector<cPlayerBasicData>& savePlayers, const std::string& saveGameName, const cPlayerBasicData* player = NULL);

void sendIdentification (cTCP& network, const cPlayerBasicData& player);

void sendGameIdentification (cTCP& network, const cPlayerBasicData& player, int socket);

void sendRequestMap (cTCP& network, const std::string& mapName, int playerNr);

void sendGo (cTCP& network);

void sendLandingState (cTCP& network, eLandingPositionState state, const cPlayerBasicData& player);

void sendAllLanded (cTCP& network);

void sendLandingPosition (cTCP& network, const cPosition& position, const cPlayerBasicData& player);

void sendInLandingPositionSelectionStatus (cTCP& network, const cPlayerBasicData& player, bool isIn, const cPlayerBasicData* receiver);

void sendPlayerHasSelectedLandingPosition (cTCP& network, const cPlayerBasicData& player, const cPlayerBasicData* receiver);

#endif // menueventsH
