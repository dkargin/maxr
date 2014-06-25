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

#ifndef gui_game_gameguiH
#define gui_game_gameguiH

#include <array>

#include "../window.h"
#include "../../utility/signal/signal.h"
#include "../../utility/signal/signalconnectionmanager.h"
#include "../../sound.h"
#include "../../upgradecalculator.h"
#include "unitselection.h"

class cHud;
class cGameMapWidget;
class cMiniMapWidget;
class cGameMessageListView;
class cChatBox;
class cLabel;
class cStaticMap;
class cMap;
class cPlayer;
class cUnit;
class cVehicle;
class cBuilding;
class cAnimationTimer;
class cClient;
class cHudPanels;
class cSavedReport;
class cCasualtiesTracker;
class cTurnClock;
class cGameSettings;
class cSoundManager;
struct sID;
struct sBuildList;

class cGameGui : public cWindow
{
public:
	explicit cGameGui (std::shared_ptr<const cStaticMap> staticMap);

	void setDynamicMap (std::shared_ptr<const cMap> dynamicMap);
	void setPlayer (std::shared_ptr<const cPlayer> player);
	void setPlayers (std::vector<std::shared_ptr<const cPlayer>> players);
	void setCasualtiesTracker (std::shared_ptr<const cCasualtiesTracker> casualties);
	void setTurnClock (std::shared_ptr<const cTurnClock> turnClock);
	void setGameSettings (std::shared_ptr<const cGameSettings> gameSettings);

	void connectToClient (cClient& client);
	void disconnectCurrentClient ();

	void centerAt (const cPosition& position);

	void setInfoTexts (const std::string& primiaryText, const std::string& additionalText);

	virtual bool handleMouseMoved (cApplication& application, cMouse& mouse, const cPosition& offset) MAXR_OVERRIDE_FUNCTION;
	virtual bool handleMouseWheelMoved (cApplication& application, cMouse& mouse, const cPosition& amount) MAXR_OVERRIDE_FUNCTION;

	virtual void handleActivated (cApplication& application, bool firstTime) MAXR_OVERRIDE_FUNCTION;
	virtual void handleDeactivated (cApplication& application, bool removed) MAXR_OVERRIDE_FUNCTION;

	virtual bool wantsCentered () const MAXR_OVERRIDE_FUNCTION;

	cSignal<void (int saveNumber, const std::string& name)> triggeredSave;
protected:

	virtual std::unique_ptr<cMouseCursor> getDefaultCursor () const MAXR_OVERRIDE_FUNCTION;
private:
	cSignalConnectionManager signalConnectionManager;
	cSignalConnectionManager clientSignalConnectionManager;
	cSignalConnectionManager playerSignalConnectionManager;
	cSignalConnectionManager dynamicMapSignalConnectionManager;
	cSignalConnectionManager moveJobSignalConnectionManager;
	cSignalConnectionManager buildPositionSelectionConnectionManager;
	cSignalConnectionManager panelSignalConnectionManager;

	std::shared_ptr<cAnimationTimer> animationTimer;
	std::shared_ptr<cSoundManager> soundManager;

	std::shared_ptr<const cStaticMap> staticMap;
	std::shared_ptr<const cMap> dynamicMap;
	std::shared_ptr<const cPlayer> player;
	std::shared_ptr<const cCasualtiesTracker> casualtiesTracker;
	std::shared_ptr<const cTurnClock> turnClock;
	std::shared_ptr<const cGameSettings> gameSettings;
	std::vector<std::shared_ptr<const cPlayer>> players;

	cHud* hud;
	cHudPanels* hudPanels;
	cGameMapWidget* gameMap;
	cMiniMapWidget* miniMap;
	cGameMessageListView* messageList;
    cChatBox* chatBox;

	cLabel* primiaryInfoLabel;
	cLabel* additionalInfoLabel;

	cPosition mouseScrollDirection;
	std::pair<bool, cPosition> savedReportPosition;

	bool openPanelOnActivation;

	void startOpenPanel ();
	void startClosePanel ();

	void exit ();

	void resetMiniMapViewWindow ();

	void showFilesWindow ();
	void showPreferencesDialog ();
	void showReportsWindow ();

    void toggleChatBox ();

	void showUnitHelpWindow (const cUnit& unit);
	void showUnitTransferDialog (const cUnit& sourceUnit, const cUnit& destinationUnit);
	void showBuildBuildingsWindow (const cVehicle& vehicle);
	void showBuildVehiclesWindow (const cBuilding& building);
	void showResourceDistributionDialog (const cUnit& unit);
	void showResearchDialog (const cUnit& unit);
	void showUpgradesWindow (const cUnit& unit);
	void showStorageWindow (const cUnit& unit);
	void showSelfDestroyDialog (const cUnit& unit);

	void updateHudCoordinates (const cPosition& tilePosition);
	void updateHudUnitName (const cPosition& tilePosition);

	void updateSelectedUnitIdleSound ();
	void updateSelectedUnitMoveSound ();

    void startSelectedUnitSound (const cSoundChunk& sound);
	void stopSelectedUnitSound ();

	void connectMoveJob (const cVehicle& vehicle);

	void handleChatCommand (const std::string& command);

	void initShortcuts ();

	void handleReport (const cSavedReport& report);

	void handleResolutionChange ();

	cSignal<void (const cUnit&, const cUnit&, int, int)> transferTriggered;
	cSignal<void (const cVehicle&, const cPosition&, const sID&, int)> buildBuildingTriggered;
	cSignal<void (const cVehicle&, const cPosition&, const sID&, int)> buildBuildingPathTriggered;
	cSignal<void (const cBuilding&, const std::vector<sBuildList>&, int, bool)> buildVehiclesTriggered;
	cSignal<void (const cUnit& unit, size_t index, const cPosition& position)> activateAtTriggered;
	cSignal<void (const cUnit&, const cUnit&)> reloadTriggered;
	cSignal<void (const cUnit&, const cUnit&)> repairTriggered;
	cSignal<void (const cUnit&, size_t index)> upgradeTriggered;
	cSignal<void (const cUnit&)> upgradeAllTriggered;
	cSignal<void (const cBuilding&, int, int, int)> changeResourceDistributionTriggered;
	cSignal<void (const std::array<int, cResearch::kNrResearchAreas>&)> changeResearchSettingsTriggered;
	cSignal<void (const std::vector<std::pair<sID, cUnitUpgrade>>&)> takeUnitUpgradesTriggered;
	cSignal<void (const std::string&)> chatCommandTriggered;
	cSignal<void (const cUnit&)> selfDestructionTriggered;
};

#endif // gui_game_gameguiH
