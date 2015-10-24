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

#include <memory>

#include "game/logic/action.h"

#include "main.h"
#include "netmessage2.h"

std::unique_ptr<cNetMessage2> cNetMessage2::createFromBuffer(const std::vector<unsigned char>& serialMessage)
{
	cBinaryArchiveOut archive(serialMessage);

	eNetMessageType type;
	archive >> type;

	int playerNr;
	archive >> playerNr;

	std::unique_ptr<cNetMessage2> message;
	switch (type)
	{
	case eNetMessageType::ACTION:
		message = cAction::createFromBuffer(archive); break;
	case eNetMessageType::GAMETIME_SYNC_SERVER:
		message = std::make_unique<cNetMessageSyncServer>(archive); break;
	case eNetMessageType::GAMETIME_SYNC_CLIENT:
		message = std::make_unique<cNetMessageSyncClient>(archive); break;
	case eNetMessageType::RANDOM_SEED:
		message = std::make_unique<cNetMessageRandomSeed>(archive); break;
	//case eNetMessageType::PLAYERSTATE:
	//	message = std::make_unique<cNe>(archive); break;
	case eNetMessageType::CHAT:
		message = std::make_unique<cNetMessageChat>(archive); break;
	case eNetMessageType::GUI_SAVE_INFO:
		message = std::make_unique<cNetMessageGUISaveInfo>(archive); break;
	case eNetMessageType::REQUEST_GUI_SAVE_INFO:
		message = std::make_unique<cNetMessageRequestGUISaveInfo>(archive); break;
		
	default:
		//TODO: Do not throw. Or catch errors on handling netmessages.
		throw std::runtime_error("Unknown net message type " + iToStr(static_cast<int>(type)));
		break;
	}

	message->playerNr = playerNr;

	return message;
}

std::unique_ptr<cNetMessage2> cNetMessage2::clone() const
{
	std::vector<unsigned char> serialMessage;
	cBinaryArchiveIn archiveIn(serialMessage);
	archiveIn << *this;

	return cNetMessage2::createFromBuffer(serialMessage);
}
