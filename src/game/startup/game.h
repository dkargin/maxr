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

#ifndef game_startup_gameH
#define game_startup_gameH

#include <memory>
#include "utility/runnable.h"
#include "maxrconfig.h"
#include "utility/signal/signal.h"

class cUnitsData;
class cClanData;

/**
 * cGame clas
 * @brief Base class for game instance. Keeps database of units and clans
 * There are child classes for each game setup: local, hotseat, host, client
 */
class cGame : public cRunnable, public std::enable_shared_from_this<cGame>
{
public:
	cGame() :
		terminate (false)
	{}

	virtual ~cGame() {}

	virtual bool wantsToTerminate() const MAXR_OVERRIDE_FUNCTION;

	void exit();

	void resetTerminating();

	void setUnitsData(std::shared_ptr<const cUnitsData> unitsData_);
	std::shared_ptr<const cUnitsData> getUnitsData() const;
	void setClanData(std::shared_ptr<const cClanData> clanData_);
	std::shared_ptr<const cClanData> getClanData() const;

	mutable cSignal<void()> terminated;
protected:
	std::shared_ptr<const cUnitsData> unitsData;
	std::shared_ptr<const cClanData> clanData;
private:
	bool terminate;
};

#endif // game_startup_gameH
