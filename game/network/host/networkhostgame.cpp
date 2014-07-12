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

#include "game/network/host/networkhostgame.h"
#include "client.h"
#include "server.h"
#include "savegame.h"
#include "loaddata.h"

//------------------------------------------------------------------------------
cNetworkHostGame::~cNetworkHostGame ()
{
	if (server)
	{
		server->stop ();
		reloadUnitValues ();
	}
}

//------------------------------------------------------------------------------
void cNetworkHostGame::run ()
{
	if (localClient) localClient->getGameTimer ()->run ();
}

//------------------------------------------------------------------------------
void cNetworkHostGame::save (int saveNumber, const std::string& saveName)
{
	if (!server) throw std::runtime_error ("Game not started!"); // should never happen (hence a translation is not necessary).

	cSavegame savegame (saveNumber);
	savegame.save (*server, saveName);
	server->makeAdditionalSaveRequest (saveNumber);
}

//------------------------------------------------------------------------------
void cNetworkHostGame::setNetwork (std::shared_ptr<cTCP> network_)
{
	network = network_;
}
