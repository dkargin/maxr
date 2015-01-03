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

#ifndef game_startup_network_host_networkhostgamesavedH
#define game_startup_network_host_networkhostgamesavedH

#include <memory>
#include <vector>

#include "game/startup/network/host/networkhostgame.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

class cApplication;
class cPlayerBasicData;

class cNetworkHostGameSaved : public cNetworkHostGame
{
public:
	void start (cApplication& application);

	void setSaveGameNumber (int saveGameNumber);

	void setPlayers (std::vector<cPlayerBasicData> players, const cPlayerBasicData& localPlayer);

	const std::vector<cPlayerBasicData>& getPlayers();
	const cPlayerBasicData& getLocalPlayer();
private:
	cSignalConnectionManager signalConnectionManager;

	size_t localPlayerIndex;
	std::vector<cPlayerBasicData> players;

	int saveGameNumber;
};

#endif // game_startup_network_host_networkhostgamesavedH
