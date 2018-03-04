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

#include "unit.h" // sID
#include "game/logic/upgradecalculator.h"
#include "utility/position.h"


struct sLandingUnit
{
	sID unitID;
	unsigned cargo = 0;
	cPosition position;

	int minCredits = 0;

	// If unit is provided by default settings, and can not be removed from unit selection
	bool isDefault = false;

	static sLandingUnit make(sID id, unsigned cargo, bool isDefault = false)
	{
		sLandingUnit unit;
		unit.unitID = id;
		unit.cargo = cargo;
		unit.position = cPosition(0,0);
		unit.isDefault = isDefault;
		return unit;
	}

	template<typename T>
	void serialize(T& archive)
	{
		archive & unitID;
		archive & cargo;
		archive & position;
		archive & minCredits;
		archive & isDefault;
	}
};


struct cBaseLayoutItem
{
	cPosition pos;
	sID ID;

	// This mutable one is bad
	mutable cStaticUnitDataPtr data = nullptr;


	template<typename T>
	void serialize(T& archive)
	{
		archive & pos;
		archive & ID;
	}
};

// Contains the data for embark
struct sLandingConfig
{
	// Landing state
	int state = 0;
	// Units that player have picked
	std::vector<sLandingUnit> landingUnits;
	// Upgrades that player has picked
	// TODO: should it be here?
	std::vector<std::pair<sID, cUnitUpgrade>> unitUpgrades;
	cPosition landingPosition;

	// Initial layout of the base. Should be filled in XML
	std::vector<cBaseLayoutItem> baseLayout;

	// Size of landing area to be automatically surveyed
	// Set to -1 to disable
	int exploreLanding = 1;

	void loadUnitsData(const cUnitsData& unitsData) const;

	template<typename T>
	void serialize(T& archive)
	{
		archive & landingUnits;
		archive & landingPosition;
		archive & unitUpgrades;
		archive & baseLayout;
		archive & exploreLanding;
	}
};

class cGameSettings;

#endif // game_data_units_landingunitH
