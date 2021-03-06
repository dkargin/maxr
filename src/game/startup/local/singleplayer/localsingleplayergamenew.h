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

#ifndef game_startup_local_singleplayer_localsingleplayergamenewH
#define game_startup_local_singleplayer_localsingleplayergamenewH

#include <memory>
#include <vector>
#include <utility>

#include "game/startup/local/singleplayer/localsingleplayergame.h"
#include "maxrconfig.h"
#include "utility/signal/signalconnectionmanager.h"
#include "utility/position.h"

class cApplication;
class cStaticMap;
class cGameSettings;
class cPlayerBasicData;
class cPlayer;
class cPosition;
class cUnitUpgrade;

struct sLandingConfig;
struct sID;

class cLocalSingleplayerGameNew : public cLocalSingleplayerGame
{
public:
	cLocalSingleplayerGameNew();

	void start (cApplication& application);

	void setGameSettings (std::shared_ptr<cGameSettings> gameSettings);
    std::shared_ptr<cGameSettings> getGameSettings();

	void setStaticMap (std::shared_ptr<cStaticMap> staticMap);
    std::shared_ptr<cStaticMap> getStaticMap();

    void setPlayerClan(int clan);
    int getPlayerClan() const;
    std::shared_ptr<sLandingConfig> getLandingConfig();

	cPlayerBasicData createPlayer();
private:
	cSignalConnectionManager signalConnectionManager;

	std::shared_ptr<cStaticMap> staticMap;
	std::shared_ptr<cGameSettings> gameSettings;

    std::shared_ptr<sLandingConfig> landingConfig;
    int clan = -1;
};

#endif // game_startup_local_singleplayer_localsingleplayergamenewH
