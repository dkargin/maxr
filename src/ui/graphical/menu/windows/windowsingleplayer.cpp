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

#include <functional>

#include "ui/graphical/menu/windows/windowsingleplayer.h"
#include "game/data/gamesettings.h"
#include "ui/graphical/menu/dialogs/dialogok.h"
#include "ui/graphical/menu/windows/windowgamesettings/windowgamesettings.h"
#include "ui/graphical/menu/windows/windowmapselection/windowmapselection.h"
#include "ui/graphical/menu/windows/windowclanselection/windowclanselection.h"
#include "ui/graphical/menu/windows/windowlandingunitselection/windowlandingunitselection.h"
#include "ui/graphical/menu/windows/windowlandingpositionselection/windowlandingpositionselection.h"
#include "ui/graphical/menu/windows/windowload/windowload.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/application.h"
#include "ui/graphical/game/gamegui.h"
#include "game/startup/local/singleplayer/localsingleplayergamenew.h"
#include "game/startup/local/singleplayer/localsingleplayergamesaved.h"
#include "utility/log.h"
#include "main.h"
#include "network.h"
#include "game/data/player/player.h"
#include "game/data/units/landingunit.h"
#include "settings.h"
#include "game/logic/client.h"
#include "game/logic/server.h"
#include "game/logic/server2.h"

#include "game/logic/clientevents.h"

//------------------------------------------------------------------------------
cWindowSinglePlayer::cWindowSinglePlayer() :
	cWindowMain (lngPack.i18n ("Text~Others~Single_Player"))
{
	using namespace std::placeholders;

	auto newGameButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~Game_New")));
	signalConnectionManager.connect (newGameButton->clicked, std::bind (&cWindowSinglePlayer::newGameClicked, this));

	auto loadGameButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (390, 190 + buttonSpace), ePushButtonType::StandardBig, lngPack.i18n ("Text~Others~Game_Load")));
	signalConnectionManager.connect (loadGameButton->clicked, std::bind (&cWindowSinglePlayer::loadGameClicked, this));

	auto backButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (415, 190 + buttonSpace * 6), ePushButtonType::StandardSmall, lngPack.i18n ("Text~Others~Back")));
	signalConnectionManager.connect (backButton->clicked, std::bind (&cWindowSinglePlayer::backClicked, this));
}

//------------------------------------------------------------------------------
cWindowSinglePlayer::~cWindowSinglePlayer()
{}

void sLandingConfig::loadUnitsData(const cUnitsData &unitsData) const
{
    for(auto& item: this->baseLayout)
    {
        item.data = unitsData.getUnit(item.ID);
    }
}

//------------------------------------------------------------------------------
void cWindowSinglePlayer::newGameClicked()
{
    if (!getActiveApplication())
        return;

    application = getActiveApplication();

    cLocalSingleplayerGameNew* newGame = new cLocalSingleplayerGameNew();
    game.reset(newGame);
	//initialize copy of unitsData that will be used in game
    newGame->setUnitsData(std::make_shared<const cUnitsData>(UnitsDataGlobal));
    newGame->setClanData(std::make_shared<const cClanData>(ClanDataGlobal));

    windowGameSettings.reset(new cWindowGameSettings());
	windowGameSettings->applySettings (cGameSettings());

    windowGameSettings->done.connect ([this]()
    {
        game->setGameSettings (std::make_shared<cGameSettings> (windowGameSettings->getGameSettings()));
        stateSelectMap();
	});

    application->show (windowGameSettings);
}

//------------------------------------------------------------------------------
void cWindowSinglePlayer::loadGameClicked()
{
    if (!getActiveApplication())
        return;

    application = getActiveApplication();

    windowLoad.reset(new cWindowLoad());
    windowLoad->load.connect ([this] (const cSaveGameInfo& saveInfo)
	{
        auto game = std::make_shared<cLocalSingleplayerGameSaved>(saveInfo.number);

		try
		{
			game->start(*application);
		}
		catch (std::runtime_error e)
		{
			Log.write("Could not start saved game.", cLog::eLOG_TYPE_ERROR);
			Log.write(e.what(), cLog::eLOG_TYPE_ERROR);
			application->show(std::make_shared<cDialogOk>(lngPack.i18n("Text~Error_Messages~ERROR_Save_Loading")));
			return;
		}

		windowLoad->close();
	});

    application->show (windowLoad);
}

void cWindowSinglePlayer::stateSelectMap()
{
    windowMapSelection.reset(new cWindowMapSelection());
    windowMapSelection->done.connect ([this]()
    {
        auto gameSettings = game->getGameSettings();
        auto staticMap = std::make_shared<cStaticMap>();

        if (!windowMapSelection->loadSelectedMap (*staticMap))
        {
            // TODO: error dialog: could not load selected map!
            return;
        }

        game->setStaticMap (staticMap);

        if (gameSettings->getClansEnabled())
        {
            this->stateSelectClan();
        }
        else
        {
            this->stateSetupUnits();
        }
    });

    application->show(windowMapSelection);
}

void cWindowSinglePlayer::stateSelectClan()
{
    windowClanSelection.reset(new cWindowClanSelection(game->getUnitsData(), game->getClanData()));

    signalConnectionManager.connect (windowClanSelection->canceled, [this]() { windowClanSelection->close(); });
    windowClanSelection->done.connect ([this]()
    {
        auto gameSettings = game->getGameSettings();
        int clan = windowClanSelection->getSelectedClan();
        game->setPlayerClan (clan);

        stateSetupUnits();
    });

    application->show(windowClanSelection);
}

void cWindowSinglePlayer::stateSetupUnits()
{
    auto gameSettings = game->getGameSettings();
    auto config = game->getLandingConfig();
    createInitial(*config, *gameSettings, *game->getUnitsData());

    windowLandingUnitSelection.reset(
            new cWindowLandingUnitSelection(
                    cPlayerColor(),
                    windowClanSelection->getSelectedClan(),
                    *config,
                    gameSettings->getStartCredits(),
                    game->getUnitsData()));

    signalConnectionManager.connect (windowLandingUnitSelection->canceled,
                                     [this]() { windowLandingUnitSelection->close(); });
    windowLandingUnitSelection->done.connect ([this]()
    {
        windowLandingUnitSelection->updateConfig(game->getLandingConfig());
        stateSelectLanding();
    });

    application->show (windowLandingUnitSelection);
}

void cWindowSinglePlayer::stateSelectLanding()
{
    auto gameSettings = game->getGameSettings();
    bool fixedBridgeHead = gameSettings->getBridgeheadType() == eGameSettingsBridgeheadType::Definite;
    //auto landingUnits = windowLandingUnitSelection->getLandingUnits();
    auto unitsdata = game->getUnitsData();
    // Make sure our base layout has proper pointers to static unit data
    game->getLandingConfig()->loadUnitsData(*unitsdata);

    windowLandingPositionSelection.reset(new cWindowLandingPositionSelection(
                    game->getStaticMap(),
                    fixedBridgeHead,
                    game->getLandingConfig(),
                    unitsdata, false));

    signalConnectionManager.connect (windowLandingPositionSelection->canceled,
                                     [this]() { windowLandingPositionSelection->close(); });

    windowLandingPositionSelection->selectedPosition.connect([this] (cPosition landingPosition)
    {
        game->getLandingConfig()->landingPosition = landingPosition;
        game->start (*application);

        windowLandingPositionSelection->close();
        windowLandingUnitSelection->close();
        if(windowClanSelection)
            windowClanSelection->close();
        windowMapSelection->close();
        windowGameSettings->close();
    });
    application->show (windowLandingPositionSelection);
}

//------------------------------------------------------------------------------
void cWindowSinglePlayer::backClicked()
{
	close();
}
