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

#include "ui/graphical/menu/windows/windownetworklobbyclient/windownetworklobbyclient.h"
#include "ui/graphical/menu/windows/windowgamesettings/gamesettings.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "main.h"
#include "network.h"
#include "utility/log.h"
#include "game/data/player/player.h"
#include "netmessage.h"
#include "menuevents.h"

//------------------------------------------------------------------------------
cWindowNetworkLobbyClient::cWindowNetworkLobbyClient () :
	cWindowNetworkLobby (lngPack.i18n ("Text~Others~TCPIP_Client"), false)
{
	auto connectButton = addChild (std::make_unique<cPushButton> (getPosition () + cPosition (470, 200), ePushButtonType::StandardSmall, lngPack.i18n ("Text~Title~Connect")));
	signalConnectionManager.connect (connectButton->clicked, std::bind (&cWindowNetworkLobbyClient::handleConnectClicked, this));
}

//------------------------------------------------------------------------------
void cWindowNetworkLobbyClient::handleConnectClicked ()
{
	triggeredConnect ();
}

//------------------------------------------------------------------------------
void cWindowNetworkLobbyClient::setSaveGame(const std::vector<cPlayerBasicData>& players, std::string saveGameName_)
{
	saveGamePlayers = players;
	saveGameName = saveGameName_;

	updateSettingsText();
}
