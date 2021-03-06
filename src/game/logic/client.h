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

#ifndef game_logic_clientH
#define game_logic_clientH

#include <memory>

#include "game/logic/gametimer.h"
#include "connectionmanager.h"
#include "utility/thread/concurrentqueue.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"
#include "netmessage2.h"
#include "game/data/model.h"

class cBuilding;
class cCasualtiesTracker;
class cAttackJob;
class cClientMoveJob;
class cAutoMJob;
class cFx;
class cFxContainer;
class cJob;
class cMap;
class cNetMessage;
class cPlayer;
class cStaticMap;
class cPlayerBasicData;
class cGameSettings;
class cPosition;
class cTurnCounter;
class cTurnTimeClock;
class cTurnTimeDeadline;
class cGameGuiState;
class cSubBase;
class cSavegame;

Uint32 TimerCallback (Uint32 interval, void* arg);


class cClient : public INetMessageReceiver
{
	friend class cDebugOutputWidget;
	friend class cPlayer;
public:
	cClient (std::shared_ptr<cConnectionManager> connectionManager);
	~cClient();

	const cModel& getModel() const { return model; };

	const cPlayer& getActivePlayer() const { return *activePlayer; }
	void setActivePlayer(cPlayer* player) { activePlayer = player; }

	void setUnitsData(std::shared_ptr<const cUnitsData> unitsData);
	void setGameSettings(const cGameSettings& gameSettings);
	void setMap(std::shared_ptr<cStaticMap> staticMap);
	void setPlayers(const std::vector<cPlayerBasicData>& splayers, size_t activePlayerNr);

	unsigned int getNetMessageQueueSize() const { return static_cast<unsigned int>(eventQueue.safe_size()); };
	virtual void pushMessage(std::unique_ptr<cNetMessage2> message) MAXR_OVERRIDE_FUNCTION;

	//
	void enableFreezeMode (eFreezeMode mode);
	void disableFreezeMode (eFreezeMode mode);
	const cFreezeModes& getFreezeModes () const;
	const std::map<int, ePlayerConnectionState>& getPlayerConnectionStates() const;
	//

	void addAutoMoveJob (std::weak_ptr<cAutoMJob> autoMoveJob);

	/**
	* sends the netMessage to the server.
	*@author Eiko
	*@param message The netMessage to be send.
	*/
	void sendNetMessage (std::unique_ptr<cNetMessage> message) const;
	/**
	* sends a serialized copy of the netmessage to the server.
	*/
	void sendNetMessage(cNetMessage2& message) const;
	void sendNetMessage(cNetMessage2&& message) const;

	/**
	* gets the vehicle with the ID
	*@author alzi alias DoctorDeath
	*@param iID The ID of the vehicle
	*/
	cVehicle* getVehicleFromID (unsigned int id) const;
	cBuilding* getBuildingFromID (unsigned int id) const;
	cUnit* getUnitFromID (unsigned int id) const;

	/**
	* handles move and attack jobs
	* this function should be called in all menu loops
	*/
	void doGameActions();

	void handleNetMessages();

	/**
	* processes everything that is need for this netMessage
	*@author alzi alias DoctorDeath
	*@param message The netMessage to be handled.
	*@return 0 for success
	*/
	int handleNetMessage (cNetMessage& message);

	void addFx (std::shared_ptr<cFx> fx, bool playSound = true);


	void deletePlayer (cPlayer& player);

	const std::shared_ptr<cCasualtiesTracker>& getCasualtiesTracker() { return casualtiesTracker; }
	std::shared_ptr<const cCasualtiesTracker> getCasualtiesTracker() const { return casualtiesTracker; }

	const std::shared_ptr<cGameTimerClient>& getGameTimer() const { return gameTimer; }

	void loadModel(int saveGameNumber);


	mutable cSignal<void (int fromPlayerNr, std::unique_ptr<cSavedReport>& report, int toPlayerNr)> reportMessageReceived;
	mutable cSignal<void (int savingID)> guiSaveInfoRequested;
	mutable cSignal<void (const cNetMessageGUISaveInfo& guiInfo)> guiSaveInfoReceived;
	mutable cSignal<void ()> freezeModeChanged;
	mutable cSignal<void ()> connectionToServerLost;


	//TODO: move signals to model
	mutable cSignal<void (const cUnit&)> unitHasStolenSuccessfully;
	mutable cSignal<void (const cUnit&)> unitHasDisabledSuccessfully;
	mutable cSignal<void (const cUnit&)> unitStealDisableFailed;

	mutable cSignal<void (const cUnit&)> unitSuppliedWithAmmo;
	mutable cSignal<void (const cUnit&)> unitRepaired;

	mutable cSignal<void (const cUnit&)> unitDisabled;
	mutable cSignal<void (const cUnit&)> unitStolen; //TODO: was in addUnit()

	void run();
private:

	void handleAutoMoveJobs();

	/**
	* gets the subbase with the id
	*@author alzi alias DoctorDeath
	*@param iID Id of the subbase
	*/
	cSubBase* getSubBaseFromID (int iID);

	void HandleNetMessage_GAME_EV_DEL_BUILDING (cNetMessage& message);
	void HandleNetMessage_GAME_EV_DEL_VEHICLE (cNetMessage& message);
	void HandleNetMessage_GAME_EV_UNIT_DATA (cNetMessage& message);
	void HandleNetMessage_GAME_EV_SPECIFIC_UNIT_DATA (cNetMessage& message);
	void HandleNetMessage_GAME_EV_RESOURCES (cNetMessage& message);
	void HandleNetMessage_GAME_EV_MARK_LOG (cNetMessage& message);
	void HandleNetMessage_GAME_EV_SUPPLY (cNetMessage& message);
	void HandleNetMessage_GAME_EV_ADD_RUBBLE (cNetMessage& message);
	void HandleNetMessage_GAME_EV_CLEAR_ANSWER (cNetMessage& message);
	void HandleNetMessage_GAME_EV_STOP_CLEARING (cNetMessage& message);
	void HandleNetMessage_GAME_EV_NOFOG (cNetMessage& message);
	void HandleNetMessage_GAME_EV_DEFEATED (cNetMessage& message);
	void HandleNetMessage_GAME_EV_DEL_PLAYER (cNetMessage& message);
	void HandleNetMessage_GAME_EV_UNIT_UPGRADE_VALUES (cNetMessage& message);
	void HandleNetMessage_GAME_EV_CREDITS_CHANGED (cNetMessage& message);
	void HandleNetMessage_GAME_EV_UPGRADED_BUILDINGS (cNetMessage& message);
	void HandleNetMessage_GAME_EV_UPGRADED_VEHICLES (cNetMessage& message);
	void HandleNetMessage_GAME_EV_RESEARCH_SETTINGS (cNetMessage& message);
	void HandleNetMessage_GAME_EV_RESEARCH_LEVEL (cNetMessage& message);
	void HandleNetMessage_GAME_EV_FINISHED_RESEARCH_AREAS (cNetMessage& message);
	void HandleNetMessage_GAME_EV_REFRESH_RESEARCH_COUNT (cNetMessage& message);
	void HandleNetMessage_GAME_EV_SET_AUTOMOVE (cNetMessage& message);
	void HandleNetMessage_GAME_EV_COMMANDO_ANSWER (cNetMessage& message);
	void HandleNetMessage_GAME_EV_SCORE (cNetMessage& message);
	void HandleNetMessage_GAME_EV_NUM_ECOS (cNetMessage& message);
	void HandleNetMessage_GAME_EV_UNIT_SCORE (cNetMessage& message);
	void HandleNetMessage_GAME_EV_REVEAL_MAP (cNetMessage& message);
private:
	cModel model;

	cSignalConnectionManager signalConnectionManager;

	cServer* server;
	std::shared_ptr<cConnectionManager> connectionManager;

	cConcurrentQueue<std::unique_ptr<cNetMessage>> eventQueue;
	cConcurrentQueue<std::unique_ptr<cNetMessage2>> eventQueue2;

	std::shared_ptr<cGameTimerClient> gameTimer;

	/** the active Player */
	cPlayer* activePlayer;

	std::shared_ptr<cCasualtiesTracker> casualtiesTracker; //TODO: move to cModel

	cFreezeModes freezeModes;
	std::map<int, ePlayerConnectionState> playerConnectionStates;

	std::list<std::weak_ptr<cAutoMJob>> autoMoveJobs; //TODO: move to cModel
};

#endif // game_logic_clientH
