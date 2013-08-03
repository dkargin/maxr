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

#include "menuevents.h"

#include "buildings.h"
#include "client.h"
#include "log.h"
#include "mapdownload.h"
#include "netmessage.h"
#include "player.h"
#include "serverevents.h"
#include "vehicles.h"

using namespace std;

void sendMenuChatMessage (cTCP& network, const string& chatMsg, const sMenuPlayer* player, int fromPlayerNr, bool translationText)
{
	cNetMessage* message = new cNetMessage (MU_MSG_CHAT);
	message->pushString (chatMsg);
	message->pushBool (translationText);
	cMenu::sendMessage (network, message, player, fromPlayerNr);
}

void sendRequestIdentification (cTCP& network, const sMenuPlayer& player)
{
	cNetMessage* message = new cNetMessage (MU_MSG_REQ_IDENTIFIKATION);
	message->pushInt16 (player.getNr());
	message->pushString (string (PACKAGE_VERSION) + " " + PACKAGE_REV);
	cMenu::sendMessage (network, message, &player);
}

void sendPlayerList (cTCP& network, const std::vector<sMenuPlayer*>& players)
{
	cNetMessage* message = new cNetMessage (MU_MSG_PLAYERLIST);

	for (int i = (int) players.size() - 1; i >= 0; i--)
	{
		const sMenuPlayer& player = *players[i];
		message->pushInt16 (player.getNr());
		message->pushBool (player.isReady());
		message->pushInt16 (player.getColorIndex());
		message->pushString (player.getName());
	}
	message->pushInt16 ( (int) players.size());
	cMenu::sendMessage (network, message);
}

void sendGameData (cTCP& network, const cStaticMap* map, const sSettings* settings, const string& saveGameString, const sMenuPlayer* player)
{
	cNetMessage* message = new cNetMessage (MU_MSG_OPTINS);

	message->pushString (saveGameString);

	if (map)
	{
		const std::string mapName = map->getName();
		message->pushInt32 (MapDownload::calculateCheckSum (mapName));
		message->pushString (mapName);
	}
	message->pushBool (map != NULL);

	if (settings)
	{
		message->pushInt16 (settings->iTurnDeadline);
		message->pushChar (settings->gameType);
		message->pushChar (settings->clans);
		message->pushChar (settings->alienTech);
		message->pushChar (settings->bridgeHead);
		message->pushInt16 (settings->credits);
		message->pushChar (settings->resFrequency);
		message->pushChar (settings->gold);
		message->pushChar (settings->oil);
		message->pushChar (settings->metal);
		message->pushChar (settings->victoryType);
		message->pushInt16 (settings->duration);
	}
	message->pushBool (settings != NULL);

	cMenu::sendMessage (network, message, player);
}

void sendGo (cTCP& network)
{
	cNetMessage* message = new cNetMessage (MU_MSG_GO);
	cMenu::sendMessage (network, message);
}

void sendIdentification (cTCP& network, const sMenuPlayer& player)
{
	cNetMessage* message = new cNetMessage (MU_MSG_IDENTIFIKATION);
	message->pushString (string (PACKAGE_VERSION) + " " + PACKAGE_REV);
	message->pushBool (player.isReady());
	message->pushString (player.getName());
	message->pushInt16 (player.getColorIndex());
	message->pushInt16 (player.getNr());
	cMenu::sendMessage (network, message);
}

void sendClan (cTCP& network, int clanNr, int ownerNr)
{
	cNetMessage* message = new cNetMessage (MU_MSG_CLAN);

	message->pushInt16 (clanNr);
	message->pushInt16 (ownerNr);

	// the host has not to send the message over tcpip and
	// we can handle the message directly
	cMenu::sendMessage (network, message);
}

void sendLandingUnits (cTCP& network, const std::vector<sLandingUnit>& landingList, int ownerNr)
{
	cNetMessage* message = new cNetMessage (MU_MSG_LANDING_VEHICLES);

	for (size_t i = 0; i != landingList.size(); ++i)
	{
		message->pushID (landingList[i].unitID);
		message->pushInt16 (landingList[i].cargo);
	}
	message->pushInt16 ( (int) landingList.size());
	message->pushInt16 (ownerNr);

	// the host has not to send the message over tcpip
	// and we can handle the message directly
	cMenu::sendMessage (network, message);
}

void sendUnitUpgrades (cTCP* network, const cPlayer& player, cMenu* activeMenu)
{
	cNetMessage* message = NULL;
	int count = 0;

	// send vehicles
	for (unsigned int i = 0; i < UnitsData.getNrVehicles(); ++i)
	{
		const sUnitData& playerData = player.VehicleData[i];
		const sUnitData& originalData = UnitsData.getVehicle (i, player.getClan());
		if (playerData.damage == originalData.damage &&
			playerData.shotsMax == originalData.shotsMax &&
			playerData.range == originalData.range &&
			playerData.ammoMax == originalData.ammoMax &&
			playerData.armor == originalData.armor &&
			playerData.hitpointsMax == originalData.hitpointsMax &&
			playerData.scan == originalData.scan &&
			playerData.speedMax == originalData.speedMax)
		{
			continue;
		}
		if (message == NULL)
		{
			message = new cNetMessage (MU_MSG_UPGRADES);
		}
		message->pushInt16 (playerData.speedMax);
		message->pushInt16 (playerData.scan);
		message->pushInt16 (playerData.hitpointsMax);
		message->pushInt16 (playerData.armor);
		message->pushInt16 (playerData.ammoMax);
		message->pushInt16 (playerData.range);
		message->pushInt16 (playerData.shotsMax);
		message->pushInt16 (playerData.damage);
		message->pushID (playerData.ID);
		message->pushBool (true);  // true for vehicles

		count++;

		if (message->iLength + 38 > PACKAGE_LENGTH)
		{
			message->pushInt16 (count);
			message->pushInt16 (player.getNr());
			if (activeMenu)
			{
				activeMenu->handleNetMessage (message);
				delete message;
			}
			else if (network) cMenu::sendMessage (*network, message);
			message = NULL;
			count = 0;
		}
	}
	if (message != NULL)
	{
		message->pushInt16 (count);
		message->pushInt16 (player.getNr());
		if (activeMenu)
		{
			activeMenu->handleNetMessage (message);
			delete message;
		}
		else if (network) cMenu::sendMessage (*network, message);
		message = NULL;
		count = 0;
	}

	// send buildings
	for (unsigned int i = 0; i < UnitsData.getNrBuildings(); ++i)
	{
		const sUnitData& playerData = player.BuildingData[i];
		const sUnitData& originalData = UnitsData.getBuilding (i, player.getClan());
		if (playerData.damage == originalData.damage &&
			playerData.shotsMax == originalData.shotsMax &&
			playerData.range == originalData.range &&
			playerData.ammoMax == originalData.ammoMax &&
			playerData.armor == originalData.armor &&
			playerData.hitpointsMax == originalData.hitpointsMax &&
			playerData.scan == originalData.scan)
		{
			continue;
		}
		if (message == NULL)
		{
			message = new cNetMessage (MU_MSG_UPGRADES);
		}
		message->pushInt16 (playerData.scan);
		message->pushInt16 (playerData.hitpointsMax);
		message->pushInt16 (playerData.armor);
		message->pushInt16 (playerData.ammoMax);
		message->pushInt16 (playerData.range);
		message->pushInt16 (playerData.shotsMax);
		message->pushInt16 (playerData.damage);
		message->pushID (playerData.ID);
		message->pushBool (false);  // false for buildings

		count++;

		if (message->iLength + 34 > PACKAGE_LENGTH)
		{
			message->pushInt16 (count);
			message->pushInt16 (player.getNr());
			if (activeMenu)
			{
				activeMenu->handleNetMessage (message);
				delete message;
			}
			else if (network) cMenu::sendMessage (*network, message);
			message = NULL;
			count = 0;
		}
	}
	if (message != NULL)
	{
		message->pushInt16 (count);
		message->pushInt16 (player.getNr());
		if (activeMenu)
		{
			activeMenu->handleNetMessage (message);
			delete message;
		}
		else if (network) cMenu::sendMessage (*network, message);
	}
}

void sendLandingCoords (cTCP& network, const sClientLandData& c, int ownerNr, cMenu* activeMenu)
{
	Log.write ("Client: sending landing coords", cLog::eLOG_TYPE_NET_DEBUG);
	cNetMessage* message = new cNetMessage (MU_MSG_LANDING_COORDS);
	message->pushInt16 (c.iLandY);
	message->pushInt16 (c.iLandX);
	message->pushChar (ownerNr);

	if (activeMenu)
	{
		activeMenu->handleNetMessage (message);
		delete message;
	}
	else cMenu::sendMessage (network, message);
}

void sendReselectLanding (cTCP& network, eLandingState state, const sMenuPlayer* player, cMenu* activeMenu)
{
	cNetMessage* message = new cNetMessage (MU_MSG_RESELECT_LANDING);
	message->pushChar (state);

	if (!DEDICATED_SERVER && player->getNr() == 0 && activeMenu)
	{
		activeMenu->handleNetMessage (message);
		delete message;
	}
	else cMenu::sendMessage (network, message, player);
}

void sendAllLanded (cTCP& network, cMenu* activeMenu)
{
	cNetMessage* message = new cNetMessage (MU_MSG_ALL_LANDED);
	cMenu::sendMessage (network, message);
	if (activeMenu)
	{
		message = new cNetMessage (MU_MSG_ALL_LANDED);
		activeMenu->handleNetMessage (message);
		delete message;
	}
}

void sendGameIdentification (cTCP& network, const sMenuPlayer& player, int socket)
{
	cNetMessage* message = new cNetMessage (GAME_EV_IDENTIFICATION);
	message->pushInt16 (socket);
	message->pushString (player.getName());
	cMenu::sendMessage (network, message);
}

void sendReconnectionSuccess (cTCP& network, int playerNr)
{
	cNetMessage* message = new cNetMessage (GAME_EV_RECON_SUCESS);
	message->pushInt16 (playerNr);
	cMenu::sendMessage (network, message);
}

void sendRequestMap (cTCP& network, const string& mapName, int playerNr)
{
	cNetMessage* msg = new cNetMessage (MU_MSG_REQUEST_MAP);
	msg->pushString (mapName);
	msg->pushInt16 (playerNr);
	cMenu::sendMessage (network, msg);
}

static bool UnitUpgradesHaveBeenPurchased (const sUnitUpgrade upgrades[8])
{
	for (int i = 0; i != 8; ++i)
	{
		if (upgrades[i].purchased)
		{
			return true;
		}
	}
	return false;
}

static int FindUpgradeValue (const sUnitUpgrade upgrades[8], sUnitUpgrade::eUpgradeTypes upgradeType, int defaultValue)
{
	for (int i = 0; i != 8; ++i)
	{
		if (upgrades[i].active && upgrades[i].type == upgradeType)
			return upgrades[i].curValue;
	}
	return defaultValue; // the specified upgrade was not found...
}

void sendTakenUpgrades (const cClient& client, sUnitUpgrade (*unitUpgrades) [8])
{
	const cPlayer* player = client.getActivePlayer();
	cNetMessage* msg = NULL;
	int iCount = 0;

	for (unsigned int i = 0; i < UnitsData.getNrVehicles() + UnitsData.getNrBuildings(); ++i)
	{
		const sUnitUpgrade* curUpgrades = unitUpgrades[i];

		if (!UnitUpgradesHaveBeenPurchased (curUpgrades)) continue;

		if (msg == NULL)
		{
			msg = new cNetMessage (GAME_EV_WANT_BUY_UPGRADES);
			iCount = 0;
		}

		const sUnitData* currentVersion;
		if (i < UnitsData.getNrVehicles()) currentVersion = &player->VehicleData[i];
		else currentVersion = &player->BuildingData[i - UnitsData.getNrVehicles()];

		if (i < UnitsData.getNrVehicles()) msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_SPEED, currentVersion->speedMax));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_SCAN, currentVersion->scan));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_HITS, currentVersion->hitpointsMax));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_ARMOR, currentVersion->armor));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_AMMO, currentVersion->ammoMax));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_RANGE, currentVersion->range));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_SHOTS, currentVersion->shotsMax));
		msg->pushInt16 (FindUpgradeValue (curUpgrades, sUnitUpgrade::UPGRADE_TYPE_DAMAGE, currentVersion->damage));
		msg->pushID (currentVersion->ID);

		iCount++; // msg contains one more upgrade struct

		// the msg would be too long,
		// if another upgrade would be written into it.
		// So send it and put the next upgrades in a new message.
		if (msg->iLength + 38 > PACKAGE_LENGTH)
		{
			msg->pushInt16 (iCount);
			msg->pushInt16 (player->getNr());
			client.sendNetMessage (msg);
			msg = NULL;
		}
	}

	if (msg != NULL)
	{
		msg->pushInt16 (iCount);
		msg->pushInt16 (player->getNr());
		client.sendNetMessage (msg);
	}
}
