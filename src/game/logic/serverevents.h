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

#ifndef game_logic_servereventsH
#define game_logic_servereventsH

#include <string>
#include "defines.h"
#include "main.h" // for sID
#include "game/logic/movejobs.h"
#include "game/logic/upgradecalculator.h" // cResearch::ResearchArea
#include "network.h"

class cBuilding;
class cHud;
class cMap;
class cNetMessage;
class cResearch;
class cUnit;
class cSavedReport;
class cGameGuiState;
struct sSubBase;

enum SERVER_EVENT_TYPES
{
	// Types between FIRST_SERVER_MESSAGE and FIRST_CLIENT_MESSAGE are for the server
	GAME_EV_CHAT_CLIENT = FIRST_SERVER_MESSAGE,		// a chat message from client to server
	GAME_EV_WANT_TO_END_TURN,		// a client wants to end the turn
	GAME_EV_WANT_START_WORK,		// a client wants to start a building
	GAME_EV_WANT_STOP_WORK,			// a client wants to stop a building
	GAME_EV_MOVE_JOB_CLIENT,		// a message with all waypoints
	GAME_EV_WANT_STOP_MOVE,			// a client wants to stop a moving vehicle
	GAME_EV_MOVEJOB_RESUME,			// a client wants a paused movejob to be resumed
	GAME_EV_WANT_ATTACK,			// a client wants to attack an other unit
	GAME_EV_MINELAYERSTATUS,		// a minelayer changes his laying status
	GAME_EV_WANT_BUILD,				// a vehicle wants to start building a building
	GAME_EV_END_BUILDING,			// a vehicle has finished building and will leave the building lot now
	GAME_EV_WANT_STOP_BUILDING,		// a vehicle wants to stop building
	GAME_EV_WANT_TRANSFER,			// information about a resource transfer
	GAME_EV_WANT_BUILDLIST,			// a building wants his buildlist to be verified by the server and start work
	GAME_EV_WANT_EXIT_FIN_VEH,		// a client wants to exit a finished vehicle out of a building
	GAME_EV_CHANGE_RESOURCES,		// a client wants to change his resource production
	GAME_EV_WANT_CHANGE_MANUAL_FIRE,// a client wants to change the manual fire status of a unit
	GAME_EV_WANT_CHANGE_SENTRY,		// a client wants to change the sentry status of a unit
	GAME_EV_WANT_MARK_LOG,			// marks a position in the log file
	GAME_EV_WANT_SUPPLY,			// a client wants to rearm or repair a unit
	GAME_EV_WANT_VEHICLE_UPGRADE,	// a client wants to upgrade a vehicle in a building to the newest version
	GAME_EV_WANT_START_CLEAR,		// a bulldowzer wants to start clearing the field under his position
	GAME_EV_WANT_STOP_CLEAR,		// a bulldowzer wants to stop the clearing
	GAME_EV_ABORT_WAITING,			// the player wants to abort waiting for the reconnect of a disconnected player
	GAME_EV_IDENTIFICATION,			// a message with the name of the player who wants to reconnect
	GAME_EV_RECON_SUCCESS,			// a client has reconnected successfully and is ready to receive his game data
	GAME_EV_WANT_LOAD,				// a client wants to load a unit into another
	GAME_EV_WANT_EXIT,				// a client wants to exit a stored unit
	GAME_EV_REQUEST_RESYNC,			// requests the server to resync a client
	GAME_EV_WANT_BUY_UPGRADES,		// a client wants to buy gold upgrades for units
	GAME_EV_WANT_BUILDING_UPGRADE,	// a client wants to upgrade one or more buildings to the newest version
	GAME_EV_WANT_RESEARCH_CHANGE,	// a client wants to change the research assignments of his research centers
	GAME_EV_AUTOMOVE_STATUS,		// a unit has been set to automoving
	GAME_EV_SAVE_HUD_INFO,			// the current hud settings
	GAME_EV_SAVE_REPORT_INFO,		// a saved report
	GAME_EV_FIN_SEND_SAVE_INFO,		//
	GAME_EV_WANT_COM_ACTION,		// an infiltrator wants to steal or disable another unit
	GAME_EV_WANT_SELFDESTROY,
	GAME_EV_WANT_CHANGE_UNIT_NAME,	// the player wants to change the name of an unit
	GAME_EV_END_MOVE_ACTION,		// specifies an action, which will be executed at the end of a movejob

	GAME_EV_WANT_KICK_PLAYER,
	GAME_EV_REQ_RECON_IDENT,        // a server of a running game requests an identification of a player who wants to reconnect
	GAME_EV_RECONNECT_ANSWER,       // a server returns an answer for the reconnect

	// Preparation room
	MU_MSG_CLAN,					// a player sends his clan
	MU_MSG_LANDING_VEHICLES,		// the list of purchased vehicles
	MU_MSG_UPGRADES,				// data of upgraded units
	MU_MSG_LANDING_COORDS,			// the selected landing coords of a client
	MU_MSG_READY_TO_START,			// the client is ready and wants the server to start the game

	// DEDICATED_SERVER
	GAME_EV_WANT_DISCONNECT,		// the player wants to disconnect (but later reconnect to the dedicated server)
	GAME_EV_REQUEST_CASUALTIES_REPORT, // a client wants to have the current casualties data
	NET_GAME_TIME_CLIENT,			//reports the current gametime of the client to server
};

/**
* Sends an event to a player that a new unit has to be added
*@author alzi alias DoctorDeath
*@param position The position of the unit
*@param bVehicle True if the unit is an vehicle
*@param iUnitNum The typ number of the unit
*@param player The player who should receive this event and get the new unit
*@param bInit True if this is called by game initialisation
*/
void sendAddUnit (cServer& server, const cPosition& position, int iID, bool bVehicle, sID UnitID, const cPlayer& player, bool bInit, bool bAddToMap = true);
/**
* Sends an event to a player that a unit has to be deleted
*@param unit unit that has to be deleted
*@param receiver The player who should receive this event.
*              Null for all player who can see the unit
*/
void sendDeleteUnit (cServer& server, const cUnit& unit, const cPlayer* receiver);
void sendDeleteUnitMessage (cServer& server, const cUnit& unit, const cPlayer& receiver);
/**
* adds a rubble object to the client
*/
void sendAddRubble (cServer& server, const cBuilding& building, const cPlayer& receiver);
/**
* Sends an event to a player that he has detected
* an enemy unit and should add it
*@author alzi alias DoctorDeath
*@param unit The unit that should be added by the player
*@param player The player who should receive this event
*/
void sendAddEnemyUnit (cServer& server, const cUnit& unit, const cPlayer& receiver);

void sendMakeTurnEnd (cServer& server, const cPlayer* receiver = nullptr);

void sendTurnFinished (cServer& server, const cPlayer& playerWhoEndedTurn, const cPlayer* nextPlayer, const cPlayer* receiver = nullptr);

void sendTurnStartTime (cServer& server, unsigned int gameTime);

void sendTurnEndDeadlineStartTime (cServer& server, unsigned int gameTime);

/**
 * Sends the data values of this unit to the client
 *
 * @param unit The unit from which the data should be taken
 * @param player The player who should receive this message
 */
void sendUnitData (cServer& server, const cUnit& unit, const cPlayer& receiver);
/**
 * Sends the unit data to the owner of the unit (if there is one) and to
 * all players that can see the unit.
 *
 * @param unit The unit from which the data should be taken
 */
void sendUnitData (cServer& server, const cUnit& unit);

void sendSpecificUnitData (cServer& server, const cVehicle& Vehicle);

/**
* sends all necessary information to all clients to start the building
*@ author Eiko
*/
void sendDoStartWork (cServer& server, const cBuilding& building);

/**
* sends all necessary information to all clients to stop the building
*@ author Eiko
*/
void sendDoStopWork (cServer& server, const cBuilding& building);

/**
* sends information about the move to the next field of a client
*@author alzi alias DoctorDeath
*/
void sendNextMove (cServer& server, const cVehicle& vehicle, int iType, int iSavedSpeed = -1);

/**
* sends all waypoints of a movejob to a client.
* If the movejob is already running,
* the sourceoffset will be changed to the actual position of the vehicle
*@author alzi alias DoctorDeath
*/
void sendMoveJobServer (cServer& server, const cServerMoveJob& MoveJob, const cPlayer& receiver);
/**
* sends the resourcedata of new scaned fields around the unit to a client
*@author alzi alias DoctorDeath
*/
void sendVehicleResources (cServer& server, const cVehicle& vehicle);
void sendResources (cServer& server, const cPlayer& player);
/**
* sends an answer to a client wheter and how he has to build.
*@author alzi alias DoctorDeath
*/
void sendBuildAnswer (cServer& server, bool bOK, const cVehicle& vehicle);
/**
* sends that a vehicle has to stop building
*@author alzi alias DoctorDeath
*/
void sendStopBuild (cServer& server, int iVehicleID, const cPosition& newPosition, const cPlayer& receiver);
/**
* send the values if a subbase.
*@author alzi alias DoctorDeath
*@param subbase the subbase which values should be send
*/
void sendSubbaseValues (cServer& server, const sSubBase& subbase, const cPlayer& receiver);
/**
* sends a list with all units which are wanted to be produced by the building
*@author alzi alias DoctorDeath
*@param building the building which buildlist should be send
*/
void sendBuildList (cServer& server, const cBuilding& building);
/**
* send the new values of resource production of a building
*@author alzi alias DoctorDeath
*@param building the building which producevalues should be send
*/
void sendProduceValues (cServer& server, const cBuilding& building);
/**
* sends that a unit has to be rearmed or repaired
*@author alzi alias DoctorDeath
*@param iDestID the ID of the destination unit
*@param bDestVehicle true if the destination unit is a vehicle
*@param iValue the new ammo or hitpoint value to be set
*@param iType SUPPLY_TYPE_REARM or SUPPLY_TYPE_REPAIR
*@param receiver The player, who will receive the message
*/
void sendSupply (cServer& server, int iDestID, bool bDestVehicle, int iValue, int iType, const cPlayer& receiver);
/**
* informs the owner of the vehicle whether the vehicle has been detected
* by another player.
* this is used by the client for correct drawing of the unit
*/
void sendDetectionState (cServer& server, const cVehicle& vehicle);

/**
* sends whether and how the unit has to clean the field
*@author alzi alias DoctorDeath
*/
void sendClearAnswer (cServer& server, int answertype, const cVehicle& vehicle, int turns, const cPosition& bigPosition, const cPlayer* player);
/**
* sends that a unit has to stop clearing
*@author alzi alias DoctorDeath
*/
void sendStopClear (cServer& server, const cVehicle& vehicle, const cPosition& bigPosition, const cPlayer& player);
/**
* sends that the player has to set his hole ScanMap to 1
*@author alzi alias DoctorDeath
*/
void sendNoFog (cServer& server, const cPlayer& receiver);
/**
* sends that a player has been defeated
*@author alzi alias DoctorDeath
*/
void sendDefeated (cServer& server, const cPlayer& player, const cPlayer* receiver = nullptr);
/**
* sends that a client has to wait until he will be defrezzed
*@param waitForPlayer tells the client, for which other player he is waiting
*/
void sendFreeze (cServer& server, eFreezeMode mode, int waitForPlayer);
/**
* sends that the client can abort waiting
*/
void sendUnfreeze (cServer& server, eFreezeMode mode);
/**
* sends that a client has to wait for another player to end his turn
*@author alzi alias DoctorDeath
*/
void sendWaitFor (cServer& server, const cPlayer& player, const cPlayer* receiver);
/**
* sends that a player has to be deleted
*@author alzi alias DoctorDeath
*/
void sendDeletePlayer (cServer& server, const cPlayer& player, const cPlayer* receiver);
/**
* the server wants to get an identification of the new connected player
*@author alzi alias DoctorDeath
*/
void sendRequestIdentification (cTCP& network, int iSocket);
/**
* the server gives its ok to the reconnection
*@author alzi alias DoctorDeath
*/
void sendReconnectAnswer (cServer& server, int socketNumber);
void sendReconnectAnswer (cServer& server, int socketNumber, const cPlayer& player);

void sendTurn (cServer& server, int turn, const cPlayer& receiver);
void sendGameGuiState (cServer& server, const cGameGuiState& state, const cPlayer& player);
void sendStoreVehicle (cServer& server, int unitid, bool vehicle, int storedunitid, const cPlayer& receiver);
void sendActivateVehicle (cServer& server, int unitid, bool vehicle, int activatunitid, const cPosition& position, const cPlayer& receiver);
void sendDeleteEverything (cServer& server, const cPlayer& receiver);
void sendUnitUpgrades (cServer& server, const sUnitData& Data, const cPlayer& receiver);
void sendCredits (cServer& server, int newCredits, const cPlayer& receiver);
void sendUpgradeBuildings (cServer& server, const std::vector<cBuilding*>& upgradedBuildings, int totalCosts, const cPlayer& receiver);
void sendUpgradeVehicles (cServer& server, const std::vector<cVehicle*>& upgradedVehicles, int totalCosts, unsigned int storingBuildingID, const cPlayer& receiver);
void sendResearchSettings (cServer& server, const std::vector<cBuilding*>& researchCentersToChangeArea, const std::vector<cResearch::ResearchArea>& newAreasForResearchCenters, const cPlayer& receiver);
void sendResearchLevel (cServer& server, const cResearch& researchLevel, const cPlayer& receiver);
void sendFinishedResearchAreas (cServer& server, const std::vector<int>& areas, const cPlayer& receiver);
void sendRefreshResearchCount (cServer& server, const cPlayer& receiver);
void sendClansToClients (cServer& server, const std::vector<std::unique_ptr<cPlayer>>& playerList);
void sendGameTime (cServer& server, const cPlayer& receiver, int gameTime);
void sendSetAutomoving (cServer& server, const cVehicle& vehicle);
/**
* sends the result of a infiltrating action to the client
*@author alzi alias DoctorDeath
*/
void sendCommandoAnswer (cServer& server, bool success, bool steal, const cVehicle& srcUnit, const cPlayer& receiver);
void sendRequestSaveInfo (cServer& server, const int saveingID);
void sendSavedReport (cServer& server, const cSavedReport& savedReport, const cPlayer* receiver);

void sendCasualtiesReport (cServer& server, const cPlayer* receiver);

void sendScore (cServer& server, const cPlayer& subject, int turn, const cPlayer* receiver = nullptr);
void sendNumEcos (cServer& server, cPlayer& subject, const cPlayer* receiver = nullptr);
void sendUnitScore (cServer& server, const cBuilding&);


void sendSelfDestroy (cServer& server, const cBuilding& building);

void sendEndMoveActionToClient (cServer& server, const cVehicle& vehicle, int destID, eEndMoveActionType type);

void sendRevealMap (cServer& server, const cPlayer& receiver);

#endif // game_logic_servereventsH
