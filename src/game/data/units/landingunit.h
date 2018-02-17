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

#ifndef game_data_units_landingunitH
#define game_data_units_landingunitH

#include <list>
#include <vector>

#include "main.h" // sID
#include "game/logic/upgradecalculator.h"
#include "utility/position.h"


struct sLandingUnit
{
	sID unitID;
	unsigned cargo;
	cPosition position;

    static sLandingUnit make(sID id, unsigned cargo)
    {
        sLandingUnit unit;
        unit.unitID = id;
        unit.cargo = cargo;
        unit.position = cPosition(0,0);
        return unit;
    }

	template<typename T>
	void serialize(T& archive)
	{
		archive & unitID;
		archive & cargo;
		archive & position;
	}
};


struct cLayoutItem
{
	cPosition pos;
	const cStaticUnitData* data;

	template<typename T>
	void serialize(T& archive)
	{
		archive & pos;
		archive & data->ID;
	}
};

// Contains the data for embark
struct sLandingConfig
{
    // Landing state
    int state = 0;
	// Units that player have picked
	std::vector<sLandingUnit> landingUnits;
	// Selected clan
    int clan = -1;
    // Upgrades that player has picked
    // TODO: should it be here?
	std::vector<std::pair<sID, cUnitUpgrade>> unitUpgrades;
	cPosition landingPosition;

	// Initial layout of the base. Should be filled in XML
    std::list<cLayoutItem> baseLayout;

	template<typename T>
	void serialize(T& archive)
	{
		archive & landingUnits;
		archive & landingPosition;
		archive & unitUpgrades;
		//archive & baseLayout;
	}
};

class cGameSettings;
// Creates initial landing config
void createInitial(sLandingConfig& config, int clan, const cGameSettings& gameSettings, const cUnitsData& unitsData);

#endif // game_data_units_landingunitH
