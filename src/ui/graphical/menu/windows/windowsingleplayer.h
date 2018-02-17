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

#ifndef ui_graphical_menu_windows_windowsingleplayerH
#define ui_graphical_menu_windows_windowsingleplayerH

#include "ui/graphical/menu/windows/windowmain.h"
#include "utility/signal/signalconnectionmanager.h"

// We will cycle through this windows
class cWindowGameSettings;
class cWindowMapSelection;
class cWindowClanSelection;
class cWindowLandingUnitSelection;
class cWindowLandingPositionSelection;

class cWindowLoad;

class cLocalSingleplayerGame;
class cLocalSingleplayerGameNew;

// Wizard for setting up single player game
class cWindowSinglePlayer : public cWindowMain
{
public:
	cWindowSinglePlayer();
	~cWindowSinglePlayer();

    /* Dialog flow:
     * 0. Set game settings, like turn duration or resources
     * 1. Select map, done by cWindowMapSelection
     * 2. Select clan (optionally), by cWindowClanSelection
     * 3. Select landing units, by cWindowLandingUnitSelection
     * 4. Select starting position, by cWindowLandingPositionSelection
     */
    enum class State
    {
        SetupGame,
        SelectMap,
        SelectClan,
        SetupUnits,
        SelectLanding,
    };

private:
	cSignalConnectionManager signalConnectionManager;

	void newGameClicked();
	void loadGameClicked();
	void backClicked();

    void stateSelectMap();
    void stateSelectClan();
    void stateSetupUnits();
    void stateSelectLanding();

    std::shared_ptr<cWindowGameSettings> windowGameSettings;
    std::shared_ptr<cWindowMapSelection> windowMapSelection;
    std::shared_ptr<cWindowClanSelection> windowClanSelection;
    std::shared_ptr<cWindowLandingUnitSelection> windowLandingUnitSelection;
    std::shared_ptr<cWindowLandingPositionSelection> windowLandingPositionSelection;

    std::shared_ptr<cWindowLoad> windowLoad;

    // Contains full settings for a new game
    // This state is shared between several wizard panes
    std::shared_ptr<cLocalSingleplayerGameNew> game;

    cApplication* application;
};

#endif // ui_graphical_menu_windows_windowsingleplayerH
