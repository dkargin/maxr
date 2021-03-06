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

#include "game/data/player/clans.h"
#include "game/data/units/unitdata.h"
#include "game/data/gamesettings.h"

using namespace std;


//--------------------------------------------------
void cClanUnitStat::addModification (const string& area, int value)
{
	modifications[area] = value;
}

//--------------------------------------------------
bool cClanUnitStat::hasModification (const string& key) const
{
	return modifications.find (key) != modifications.end();
}

//--------------------------------------------------
int cClanUnitStat::getModificationValue (const string& key) const
{
	map<string, int>::const_iterator it = modifications.find (key);
	if (it != modifications.end())
		return it->second;
	return 0;
}

//--------------------------------------------------
static string GetModificatorString (int original, int modified)
{
	const int diff = modified - original;
	if (diff > 0)
		return " +" + iToStr (diff);
	else if (diff < 0)
		return " -" + iToStr (-diff);
	else // diff == 0
		return " =" + iToStr (modified);
}

//--------------------------------------------------
string cClanUnitStat::getClanStatsDescription(const cUnitsData& originalData) const
{
	const cDynamicUnitData* data = &originalData.getDynamicData(unitId);

	if (data == nullptr) return "Unknown";

	string result = originalData.getUnit(unitId)->getName() + lngPack.i18n ("Text~Punctuation~Colon");
	const char* const commaSep = ", ";
	const char* sep = "";

	struct
	{
		const char* type;
		const char* textToTranslate;
		int originalValue;
	} t[] =
	{
		// ToDo / Fixme if #756 fixed, use the non "_7" version of the text files
		{"Damage", "Text~Others~Attack_7", data->getDamage()},
		{"Range", "Text~Others~Range", data->getRange()},
		{"Armor", "Text~Others~Armor_7", data->getArmor()},
		{"Hitpoints", "Text~Others~Hitpoints_7", data->getHitpointsMax()},
		{"Scan", "Text~Others~Scan_7", data->getScan()},
		{"Speed", "Text~Others~Speed_7", data->getSpeedMax() / 4},
	};

	for (int i = 0; i != sizeof (t) / sizeof (*t); ++i)
	{
		if (hasModification (t[i].type) == false) continue;
		result += sep;
		result += lngPack.i18n (t[i].textToTranslate);
		result += GetModificatorString (t[i].originalValue, getModificationValue (t[i].type));
		sep = commaSep;
	}
	if (hasModification ("Built_Costs"))
	{
		result += sep;
		int nrTurns = getModificationValue ("Built_Costs");
		if (!originalData.getUnit(data->getId())->hasFlag(UnitFlag::IsHuman))
			nrTurns /= unitId.isAVehicle() == 0 ? 2 : 3;

		result += iToStr (nrTurns) + " " + lngPack.i18n ("Text~Comp~Turns");
	}
	return result;
}

//--------------------------------------------------
cClanUnitStat* cClan::getUnitStat (sID id) const
{
	for (const auto& stat : stats)
		if (stat->getUnitId() == id)
			return stat.get();
	return nullptr;
}

//--------------------------------------------------
cClanUnitStat* cClan::getUnitStat (unsigned int index) const
{
	if (index < stats.size())
		return stats[index].get();
	return nullptr;
}

//--------------------------------------------------
cClanUnitStat* cClan::addUnitStat (sID id)
{
	stats.push_back (std::make_unique<cClanUnitStat> (id));
	return stats.back().get();
}

//---------------------------------------------------
cClan::cClan(const cClan& other) :
	num(other.num),
	description(other.description),
	name(other.name)
{
	for (const auto& stat : other.stats)
	{
		stats.push_back(std::make_unique<cClanUnitStat>(*stat));
	}

	landingUnits = other.landingUnits;
	baseLayout = other.baseLayout;
}

//--------------------------------------------------
void cClan::setDescription (const string& newDescription)
{
	description = newDescription;
}

//---------------------------------------------------
const std::string cClan::getDescription() const
{
	std::string translatedDescription = lngPack.getClanDescription(num);
	if (!translatedDescription.empty())
		return translatedDescription;

	return description;
}

//--------------------------------------------------
void cClan::setName (const string& newName)
{
	name = newName;
}

//--------------------------------------------------
const std::string cClan::getName() const
{
	std::string translatedName = lngPack.getClanName(num);
	if (!translatedName.empty())
		return translatedName;

	return name;
}

//--------------------------------------------------
vector<string> cClan::getClanStatsDescription(const cUnitsData& originalData) const
{
	vector<string> result;
	for (int i = 0; i != getNrUnitStats(); ++i)
	{
		cClanUnitStat* stat = getUnitStat (i);
		result.push_back (stat->getClanStatsDescription(originalData));
	}
	return result;
}

void cClan::addEmbarkUnit(const sLandingUnit& item)
{
	landingUnits.push_back(item);
}

void cClan::addEmbarkBuilding(const cBaseLayoutItem& item)
{
	baseLayout.push_back(item);
}

//------------------------------------------------------------------------------
void cClan::createLanding(sLandingConfig& config, const cGameSettings& gameSettings, const cUnitsData& unitsData)
{
	if (gameSettings.getBridgeheadType() == eGameSettingsBridgeheadType::Mobile)
		return;

	if(config.state != 0)
		return;

	//Add initial buildings
	for(const auto& item: this->baseLayout)
	{
		config.baseLayout.push_back(item);
	}

	// Add initial vehicles.
	// This vehicles can depend in starting credits
	const int startCredits = gameSettings.getStartCredits();
	for(const auto& item: landingUnits)
	{
		if(startCredits >= item.minCredits)
			config.landingUnits.push_back(item);
	}

	config.state = 1;
}

// Reset all clan data
void cClan::resetAll()
{
	resetEmbark();
	resetStats();
}

// Reset embarcation
void cClan::resetEmbark()
{
	landingUnits.clear();;
	baseLayout.clear();
}

// Reset unit upgrades
void cClan::resetStats()
{
	stats.clear();
}


//---------------------------------------------------
cClanData::cClanData(const cClanData& other)
{
	for (const auto& clan : other.clans)
	{
		clans.push_back(std::make_unique<cClan>(*clan));
	}
}

//--------------------------------------------------
cClan* cClanData::makeClan(const char* name)
{
	assert(name && strlen(name) > 0);

	// Trying to find existing clan
	for (const auto& clan : clans)
	{
		if(clan->getName() == name)
			return clan.get();
	}

	// No clan is found. Creating a new one
	cClan* clan = new cClan((int) clans.size());
	clan->setName(name);
	clans.push_back(std::unique_ptr<cClan>(clan));
	return clan;
}

//--------------------------------------------------
cClan* cClanData::getClan (unsigned int num) const
{
	if (num < clans.size())
		return clans[num].get();
	return nullptr;
}
