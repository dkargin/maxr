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

#include <algorithm>
#include <cctype>
#include <cassert>

#include "ui/graphical/menu/windows/windowgamesettings/gamesettings.h"
#include "utility/tounderlyingtype.h"
#include "utility/string/iequals.h"
#include "main.h"

//------------------------------------------------------------------------------
std::string gameSettingsResourceAmountToString (eGameSettingsResourceAmount amount, bool translated)
{
	if (translated)
	{
		switch (amount)
		{
		case eGameSettingsResourceAmount::Limited:
			return lngPack.i18n ("Text~Option~Limited");
		case eGameSettingsResourceAmount::Normal:
			return lngPack.i18n ("Text~Option~Normal");
		case eGameSettingsResourceAmount::High:
			return lngPack.i18n ("Text~Option~High");
		case eGameSettingsResourceAmount::TooMuch:
			return lngPack.i18n ("Text~Option~TooMuch");
		}
	}
	else
	{
		switch (amount)
		{
		case eGameSettingsResourceAmount::Limited:
			return "limited";
		case eGameSettingsResourceAmount::Normal:
			return "normal";
		case eGameSettingsResourceAmount::High:
			return "high";
		case eGameSettingsResourceAmount::TooMuch:
			return "toomuch";
		}
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
eGameSettingsResourceAmount gameSettingsResourceAmountFromString (const std::string& string)
{
	if (iequals(string, "limited")) return eGameSettingsResourceAmount::Limited;
	else if (iequals (string, "normal")) return eGameSettingsResourceAmount::Normal;
	else if (iequals (string, "high")) return eGameSettingsResourceAmount::High;
	else if (iequals (string, "toomuch")) return eGameSettingsResourceAmount::TooMuch;
	else throw std::runtime_error ("Invalid resource amount string '" + string + "'");
}

//------------------------------------------------------------------------------
std::string gameSettingsResourceDensityToString (eGameSettingsResourceDensity density, bool translated)
{
	if (translated)
	{
		switch (density)
		{
		case eGameSettingsResourceDensity::Sparse:
			return lngPack.i18n ("Text~Option~Sparse");
		case eGameSettingsResourceDensity::Normal:
			return lngPack.i18n ("Text~Option~Normal");
		case eGameSettingsResourceDensity::Dense:
			return lngPack.i18n ("Text~Option~Dense");
		case eGameSettingsResourceDensity::TooMuch:
			return lngPack.i18n ("Text~Option~TooMuch");
		}
	}
	else
	{
		switch (density)
		{
		case eGameSettingsResourceDensity::Sparse:
			return "sparse";
		case eGameSettingsResourceDensity::Normal:
			return "normal";
		case eGameSettingsResourceDensity::Dense:
			return "dense";
		case eGameSettingsResourceDensity::TooMuch:
			return "toomuch";
		}
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
eGameSettingsResourceDensity gameSettingsResourceDensityFromString (const std::string& string)
{
	if (iequals (string, "sparse")) return eGameSettingsResourceDensity::Sparse;
	else if (iequals (string, "normal")) return eGameSettingsResourceDensity::Normal;
	else if (iequals (string, "dense")) return eGameSettingsResourceDensity::Dense;
	else if (iequals (string, "toomuch")) return eGameSettingsResourceDensity::TooMuch;
	else throw std::runtime_error ("Invalid resource density string '" + string + "'");
}

//------------------------------------------------------------------------------
std::string gameSettingsBridgeheadTypeToString (eGameSettingsBridgeheadType type, bool translated)
{
	if (translated)
	{
		switch (type)
		{
		case eGameSettingsBridgeheadType::Definite:
			return lngPack.i18n ("Text~Option~Definite");
		case eGameSettingsBridgeheadType::Mobile:
			return lngPack.i18n ("Text~Option~Mobile");
		}
	}
	else
	{
		switch (type)
		{
		case eGameSettingsBridgeheadType::Definite:
			return "definite";
		case eGameSettingsBridgeheadType::Mobile:
			return "mobile";
		}
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
eGameSettingsBridgeheadType gameSettingsBridgeheadTypeFromString (const std::string& string)
{
	if (iequals (string, "definite")) return eGameSettingsBridgeheadType::Definite;
	else if (iequals (string, "mobile")) return eGameSettingsBridgeheadType::Mobile;
	else throw std::runtime_error ("Invalid bridgehead type string '" + string + "'");
}

//------------------------------------------------------------------------------
std::string gameSettingsGameTypeToString (eGameSettingsGameType type, bool translated)
{
	if (translated)
	{
		switch (type)
		{
		case eGameSettingsGameType::Simultaneous:
			return lngPack.i18n ("Text~Option~Type_Simu");
		case eGameSettingsGameType::Turns:
			return lngPack.i18n ("Text~Option~Type_Turns");
		case eGameSettingsGameType::HotSeat:
			return "Hot Seat"; // TODO: translation?!
		}
	}
	else
	{
		switch (type)
		{
		case eGameSettingsGameType::Simultaneous:
			return "simultaneous";
		case eGameSettingsGameType::Turns:
			return "turns";
		case eGameSettingsGameType::HotSeat:
			return "hotseat";
		}
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
eGameSettingsGameType gameSettingsGameTypeString (const std::string& string)
{
	if (iequals (string, "simultaneous")) return eGameSettingsGameType::Simultaneous;
	else if (iequals (string, "turns")) return eGameSettingsGameType::Turns;
	else if (iequals (string, "hotseat")) return eGameSettingsGameType::HotSeat;
	else throw std::runtime_error ("Invalid game type string '" + string + "'");
}

//------------------------------------------------------------------------------
std::string gameSettingsVictoryConditionToString (eGameSettingsVictoryCondition condition, bool translated)
{
	if (translated)
	{
		switch (condition)
		{
		case eGameSettingsVictoryCondition::Turns:
			return lngPack.i18n ("Text~Comp~Turns");;
		case eGameSettingsVictoryCondition::Points:
			return lngPack.i18n ("Text~Comp~Points");;
		case eGameSettingsVictoryCondition::Death:
			return lngPack.i18n ("Text~Comp~NoLimit");
		}
	}
	else
	{
		switch (condition)
		{
		case eGameSettingsVictoryCondition::Turns:
			return "turns";
		case eGameSettingsVictoryCondition::Points:
			return "points";
		case eGameSettingsVictoryCondition::Death:
			return "death";
		}
	}
	assert (false);
	return "";
}

//------------------------------------------------------------------------------
eGameSettingsVictoryCondition gameSettingsVictoryConditionFromString (const std::string& string)
{
	if (iequals (string, "turns")) return eGameSettingsVictoryCondition::Turns;
	else if (iequals (string, "points")) return eGameSettingsVictoryCondition::Points;
	else if (iequals (string, "death")) return eGameSettingsVictoryCondition::Death;
	else throw std::runtime_error ("Invalid victory condition string '" + string + "'");
}

//------------------------------------------------------------------------------
cGameSettings::cGameSettings () :
	metalAmount (eGameSettingsResourceAmount::Normal),
	oilAmount (eGameSettingsResourceAmount::Normal),
	goldAmount (eGameSettingsResourceAmount::Normal),
	resourceDensity (eGameSettingsResourceDensity::Normal),
	bridgeheadType (eGameSettingsBridgeheadType::Definite),
	gameType (eGameSettingsGameType::Simultaneous),
	clansEnabled (true),
	startCredits (defaultCreditsNormal),
	victoryConditionType (eGameSettingsVictoryCondition::Death),
	victoryTurns (400),
	vectoryPoints (400),
	turnEndDeadline (90),
	turnEndDeadlineActive (true),
	turnLimit (5 * 60),
	turnLimitActive (false)
{}

//------------------------------------------------------------------------------
cGameSettings::cGameSettings (const cGameSettings& other) :
	metalAmount (other.metalAmount),
	oilAmount (other.oilAmount),
	goldAmount (other.goldAmount),
	resourceDensity (other.resourceDensity),
	bridgeheadType (other.bridgeheadType),
	gameType (other.gameType),
	clansEnabled (other.clansEnabled),
	startCredits (other.startCredits),
	victoryConditionType (other.victoryConditionType),
	victoryTurns (other.victoryTurns),
	vectoryPoints (other.vectoryPoints),
	turnEndDeadline (other.turnEndDeadline),
	turnEndDeadlineActive (other.turnEndDeadlineActive),
	turnLimit (other.turnLimit),
	turnLimitActive (other.turnLimitActive)
{}

//------------------------------------------------------------------------------
cGameSettings& cGameSettings::operator=(const cGameSettings& other)
{
	metalAmount = other.metalAmount;
	oilAmount = other.oilAmount;
	goldAmount = other.goldAmount;
	resourceDensity = other.resourceDensity;
	bridgeheadType = other.bridgeheadType;
	gameType = other.gameType;
	clansEnabled = other.clansEnabled;
	startCredits = other.startCredits;
	victoryConditionType = other.victoryConditionType;
	victoryTurns = other.victoryTurns;
	vectoryPoints = other.vectoryPoints;
	turnEndDeadline = other.turnEndDeadline;
	turnEndDeadlineActive = other.turnEndDeadlineActive;
	turnLimit = other.turnLimit;
	turnLimitActive = other.turnLimitActive;

	return *this;
}

//------------------------------------------------------------------------------
eGameSettingsResourceAmount cGameSettings::getMetalAmount () const
{
	return metalAmount;
}

//------------------------------------------------------------------------------
void cGameSettings::setMetalAmount (eGameSettingsResourceAmount value)
{
	std::swap(metalAmount, value);
	if (metalAmount != value) metalAmountChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsResourceAmount cGameSettings::getOilAmount () const
{
	return oilAmount;
}

//------------------------------------------------------------------------------
void cGameSettings::setOilAmount (eGameSettingsResourceAmount value)
{
	std::swap (oilAmount, value);
	if (oilAmount != value) oilAmountChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsResourceAmount cGameSettings::getGoldAmount () const
{
	return goldAmount;
}

//------------------------------------------------------------------------------
void cGameSettings::setGoldAmount (eGameSettingsResourceAmount value)
{
	std::swap (goldAmount, value);
	if (goldAmount != value) goldAmountChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsResourceDensity cGameSettings::getResourceDensity () const
{
	return resourceDensity;
}

//------------------------------------------------------------------------------
void cGameSettings::setResourceDensity (eGameSettingsResourceDensity value)
{
	std::swap (resourceDensity, value);
	if (resourceDensity != value) resourceDensityChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsBridgeheadType cGameSettings::getBridgeheadType () const
{
	return bridgeheadType;
}

//------------------------------------------------------------------------------
void cGameSettings::setBridgeheadType (eGameSettingsBridgeheadType value)
{
	std::swap (bridgeheadType, value);
	if (bridgeheadType != value) bridgeheadTypeChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsGameType cGameSettings::getGameType () const
{
	return gameType;
}

//------------------------------------------------------------------------------
void cGameSettings::setGameType (eGameSettingsGameType value)
{
	std::swap (gameType, value);
	if (gameType != value) gameTypeChanged ();
}

//------------------------------------------------------------------------------
bool cGameSettings::getClansEnabled () const
{
	return clansEnabled;
}

//------------------------------------------------------------------------------
void cGameSettings::setClansEnabled (bool value)
{
	std::swap (clansEnabled, value);
	if (clansEnabled != value) clansEnabledChanged ();
}

//------------------------------------------------------------------------------
unsigned int cGameSettings::getStartCredits () const
{
	return startCredits;
}

//------------------------------------------------------------------------------
void cGameSettings::setStartCredits (unsigned int value)
{
	std::swap (startCredits, value);
	if (startCredits != value) startCreditsChanged ();
}

//------------------------------------------------------------------------------
eGameSettingsVictoryCondition cGameSettings::getVictoryCondition () const
{
	return victoryConditionType;
}

//------------------------------------------------------------------------------
void cGameSettings::setVictoryCondition (eGameSettingsVictoryCondition value)
{
	std::swap (victoryConditionType, value);
	if (victoryConditionType != value) victoryConditionTypeChanged ();
}

//------------------------------------------------------------------------------
unsigned int cGameSettings::getVictoryTurns () const
{
	return victoryTurns;
}

//------------------------------------------------------------------------------
void cGameSettings::setVictoryTurns (unsigned int value)
{
	std::swap (victoryTurns, value);
	if (victoryTurns != value) victoryTurnsChanged ();
}

//------------------------------------------------------------------------------
unsigned int cGameSettings::getVictoryPoints () const
{
	return vectoryPoints;
}

//------------------------------------------------------------------------------
void cGameSettings::setVictoryPoints (unsigned int value)
{
	std::swap (vectoryPoints, value);
	if (vectoryPoints != value) victoryPointsChanged ();
}

//------------------------------------------------------------------------------
const std::chrono::seconds& cGameSettings::getTurnEndDeadline () const
{
	return turnEndDeadline;
}

//------------------------------------------------------------------------------
void cGameSettings::setTurnEndDeadline (const std::chrono::seconds& value)
{
	const auto oldValue = turnEndDeadline;
	turnEndDeadline = value;
	if (oldValue != turnEndDeadline) turnEndDeadlineChanged ();
}

//------------------------------------------------------------------------------
bool cGameSettings::isTurnEndDeadlineActive () const
{
	return turnEndDeadlineActive;
}

//------------------------------------------------------------------------------
void cGameSettings::setTurnEndDeadlineActive (bool value)
{
	std::swap (turnEndDeadlineActive, value);
	if (turnEndDeadlineActive != value) turnEndDeadlineActiveChanged ();
}

//------------------------------------------------------------------------------
const std::chrono::seconds& cGameSettings::getTurnLimit () const
{
	return turnLimit;
}

//------------------------------------------------------------------------------
void cGameSettings::setTurnLimit (const std::chrono::seconds& value)
{
	const auto oldValue = turnLimit;
	turnLimit = value;
	if (oldValue != turnLimit) turnLimitChanged ();
}

//------------------------------------------------------------------------------
bool cGameSettings::isTurnLimitActive () const
{
	return turnLimitActive;
}

//------------------------------------------------------------------------------
void cGameSettings::setTurnLimitActive (bool value)
{
	std::swap (turnLimitActive, value);
	if (turnLimitActive != value) turnLimitActiveChanged ();
}

//------------------------------------------------------------------------------
void cGameSettings::pushInto (cNetMessage& message) const
{
	// FIXME: implement
	//assert (false);
}

//------------------------------------------------------------------------------
void cGameSettings::popFrom (cNetMessage& message)
{
	// FIXME: implement
	//assert (false);
}
