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

/* Author: Paul Grathwohl */

#include "game/logic/casualtiestracker.h"

#include "game/data/units/unitdata.h"
#include "utility/log.h"
#include "netmessage.h"
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

//--------------------------------------------------------------------------
void cCasualtiesTracker::logCasualty (sID unitType, int playerNr)
{
	setCasualty (unitType, getCasualtiesOfUnitType (unitType, playerNr) + 1, playerNr);

	casualtiesChanged();
}

//--------------------------------------------------------------------------
void cCasualtiesTracker::setCasualty (sID unitType, int numberOfLosses, int playerNr)
{
	auto signalCaller = makeScopedOperation ([ = ]() { casualtyChanged (unitType, playerNr); });

	vector<Casualty>& casualties = getCasualtiesOfPlayer (playerNr);

	for (size_t i = 0; i != casualties.size(); ++i)
	{
		if (unitType == casualties[i].unitID)
		{
			casualties[i].numberOfLosses = numberOfLosses;
			return;
		}
	}
	Casualty newCasualtyEntry;
	newCasualtyEntry.numberOfLosses = numberOfLosses;
	newCasualtyEntry.unitID = unitType;
	for (size_t i = 0; i != casualties.size(); ++i)
	{
		if (unitType.less_vehicleFirst (casualties[i].unitID))
		{
			vector<Casualty>::iterator it = casualties.begin();
			casualties.insert (it + i, newCasualtyEntry);
			return;
		}
	}
	casualties.push_back (newCasualtyEntry);
}

//--------------------------------------------------------------------------
int cCasualtiesTracker::getCasualtiesOfUnitType (sID unitType, int playerNr) const
{
	const vector<Casualty>& casualties = getCasualtiesOfPlayer (playerNr);
	for (unsigned int i = 0; i < casualties.size(); i++)
	{
		if (unitType == casualties[i].unitID)
			return casualties[i].numberOfLosses;
	}
	return 0;
}

//--------------------------------------------------------------------------
vector<sID> cCasualtiesTracker::getUnitTypesWithLosses() const
{
	vector<sID> result;

	for (size_t i = 0; i != casualtiesPerPlayer.size(); ++i)
	{
		const vector<Casualty>& casualties = casualtiesPerPlayer[i].casualties;
		for (size_t entryIdx = 0; entryIdx != casualties.size(); ++entryIdx)
		{
			const Casualty& casualty = casualties[entryIdx];
			bool containedInResult = false;
			for (size_t j = 0; j != result.size(); ++j)
			{
				if (result[j] == casualty.unitID)
				{
					containedInResult = true;
					break;
				}
			}
			if (containedInResult == true) continue;

			bool inserted = false;
			for (size_t j = 0; j != result.size(); ++j)
			{
				const sID unitID = casualty.unitID;

				// buildings should be inserted first
				if (unitID.less_buildingFirst (result[j]))
				{
					result.insert (result.begin() + j, casualty.unitID);
					inserted = true;
					break;
				}
			}
			if (inserted == false)
				result.push_back (casualty.unitID);
		}
	}
	return result;
}

//--------------------------------------------------------------------------
vector<cCasualtiesTracker::Casualty>& cCasualtiesTracker::getCasualtiesOfPlayer (int playerNr) const
{
	for (unsigned int i = 0; i < casualtiesPerPlayer.size(); i++)
	{
		if (casualtiesPerPlayer[i].playerNr == playerNr)
			return casualtiesPerPlayer[i].casualties;
	}

	CasualtiesOfPlayer newCasualtiesOfPlayer;
	newCasualtiesOfPlayer.playerNr = playerNr;
	for (unsigned int i = 0; i < casualtiesPerPlayer.size(); i++)
	{
		if (playerNr < casualtiesPerPlayer[i].playerNr)
		{
			vector<CasualtiesOfPlayer>::iterator it = casualtiesPerPlayer.begin();
			casualtiesPerPlayer.insert (it + i, newCasualtiesOfPlayer);
			return casualtiesPerPlayer[i].casualties;
		}
	}
	casualtiesPerPlayer.push_back (newCasualtiesOfPlayer);
	return casualtiesPerPlayer.back().casualties;
}
