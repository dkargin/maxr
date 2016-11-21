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

#ifndef game_logic_casualtiestrackerH
#define game_logic_casualtiestrackerH

#include "main.h"
#include "utility/signal/signal.h"
#include <vector>

//-------------------------------------------------------------------------------
class cCasualtiesTracker
{
public:
	cCasualtiesTracker() {}


	void logCasualty (sID unitType, int playerNr);
	int getCasualtiesOfUnitType (sID unitType, int playerNr) const;

	std::vector<sID> getUnitTypesWithLosses() const;

	mutable cSignal<void (const sID&, int)> casualtyChanged;
	mutable cSignal<void ()> casualtiesChanged;
	//-------------------------------------------------------------------------------
private:
	struct Casualty
	{
		sID unitID;
		int numberOfLosses;
	};
	struct CasualtiesOfPlayer
	{
		std::vector<Casualty> casualties;
		int playerNr;
	};
	mutable std::vector<CasualtiesOfPlayer> casualtiesPerPlayer;

	std::vector<Casualty>& getCasualtiesOfPlayer (int playerNr) const;
	void setCasualty (sID unitType, int numberOfLosses, int playerNr);
};


#endif // game_logic_casualtiestrackerH
