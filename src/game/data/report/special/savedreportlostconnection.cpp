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

#include "game/data/report/special/savedreportlostconnection.h"
#include "netmessage.h"
#include "game/data/player/player.h"
#include "main.h"

//------------------------------------------------------------------------------
cSavedReportLostConnection::cSavedReportLostConnection (const cPlayer& player) :
	playerName (player.getName())
{}

//------------------------------------------------------------------------------
cSavedReportLostConnection::cSavedReportLostConnection (cNetMessage& message)
{
	playerName = message.popString();
}

//------------------------------------------------------------------------------
cSavedReportLostConnection::cSavedReportLostConnection (const tinyxml2::XMLElement& element)
{
	playerName = element.Attribute ("playerName");
}

//------------------------------------------------------------------------------
void cSavedReportLostConnection::pushInto (cNetMessage& message) const
{
	message.pushString (playerName);

	cSavedReport::pushInto (message);
}

//------------------------------------------------------------------------------
void cSavedReportLostConnection::pushInto (tinyxml2::XMLElement& element) const
{
	element.SetAttribute ("playerName", playerName.c_str());

	cSavedReport::pushInto (element);
}

//------------------------------------------------------------------------------
eSavedReportType cSavedReportLostConnection::getType() const
{
	return eSavedReportType::LostConnection;
}

//------------------------------------------------------------------------------
std::string cSavedReportLostConnection::getMessage() const
{
	return lngPack.i18n ("Text~Multiplayer~Lost_Connection", playerName);
}

//------------------------------------------------------------------------------
bool cSavedReportLostConnection::isAlert() const
{
	return false;
}