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

#ifndef ui_graphical_menu_windows_windowgamesettings_gamesettingsH
#define ui_graphical_menu_windows_windowgamesettings_gamesettingsH

#include <string>
#include <chrono>

#include "utility/signal/signal.h"
#include "utility/serialization/serialization.h"

class cNetMessage;

enum class eGameSettingsResourceAmount
{
	Limited,
	Normal,
	High,
	TooMuch
};

enum class eGameSettingsResourceDensity
{
	Sparse,
	Normal,
	Dense,
	TooMuch
};

enum class eGameSettingsBridgeheadType
{
	Mobile,
	Definite,
};

enum class eGameSettingsGameType
{
	Simultaneous,
	Turns,
	HotSeat
};

enum class eGameSettingsVictoryCondition
{
	Turns,
	Points,
	Death
};

std::string gameSettingsResourceAmountToString (eGameSettingsResourceAmount amount, bool translated = false);
eGameSettingsResourceAmount gameSettingsResourceAmountFromString (const std::string& string);

std::string gameSettingsResourceDensityToString (eGameSettingsResourceDensity density, bool translated = false);
eGameSettingsResourceDensity gameSettingsResourceDensityFromString (const std::string& string);

std::string gameSettingsBridgeheadTypeToString (eGameSettingsBridgeheadType type, bool translated = false);
eGameSettingsBridgeheadType gameSettingsBridgeheadTypeFromString (const std::string& string);

std::string gameSettingsGameTypeToString (eGameSettingsGameType type, bool translated = false);
eGameSettingsGameType gameSettingsGameTypeString (const std::string& string);

std::string gameSettingsVictoryConditionToString (eGameSettingsVictoryCondition condition, bool translated = false);
eGameSettingsVictoryCondition gameSettingsVictoryConditionFromString (const std::string& string);

class cGameSettings
{
public:
	static const int defaultCreditsNone    = 0;
	static const int defaultCreditsLow     = 50;
	static const int defaultCreditsLimited = 100;
	static const int defaultCreditsNormal  = 150;
	static const int defaultCreditsHigh    = 200;
	static const int defaultCreditsMore    = 250;

	static const int defaultVictoryTurnsOption0 = 100;
	static const int defaultVictoryTurnsOption1 = 200;
	static const int defaultVictoryTurnsOption2 = 400;

	static const int defaultVictoryPointsOption0 = 100;
	static const int defaultVictoryPointsOption1 = 200;
	static const int defaultVictoryPointsOption2 = 400;

	static const std::chrono::seconds defaultTurnLimitOption0;
	static const std::chrono::seconds defaultTurnLimitOption1;
	static const std::chrono::seconds defaultTurnLimitOption2;
	static const std::chrono::seconds defaultTurnLimitOption3;
	static const std::chrono::seconds defaultTurnLimitOption4;
	static const std::chrono::seconds defaultTurnLimitOption5;

	static const std::chrono::seconds defaultEndTurnDeadlineOption0;
	static const std::chrono::seconds defaultEndTurnDeadlineOption1;
	static const std::chrono::seconds defaultEndTurnDeadlineOption2;
	static const std::chrono::seconds defaultEndTurnDeadlineOption3;
	static const std::chrono::seconds defaultEndTurnDeadlineOption4;
	static const std::chrono::seconds defaultEndTurnDeadlineOption5;

	cGameSettings();
	cGameSettings (const cGameSettings& other);

	cGameSettings& operator= (const cGameSettings& other);

	eGameSettingsResourceAmount getMetalAmount() const;
	void setMetalAmount (eGameSettingsResourceAmount value);

	eGameSettingsResourceAmount getOilAmount() const;
	void setOilAmount (eGameSettingsResourceAmount value);

	eGameSettingsResourceAmount getGoldAmount() const;
	void setGoldAmount (eGameSettingsResourceAmount value);

	eGameSettingsResourceDensity getResourceDensity() const;
	void setResourceDensity (eGameSettingsResourceDensity value);

	eGameSettingsBridgeheadType getBridgeheadType() const;
	void setBridgeheadType (eGameSettingsBridgeheadType value);

	eGameSettingsGameType getGameType() const;
	void setGameType (eGameSettingsGameType value);

	bool getClansEnabled() const;
	void setClansEnabled (bool value);

	unsigned int getStartCredits() const;
	void setStartCredits (unsigned int value);

	eGameSettingsVictoryCondition getVictoryCondition() const;
	void setVictoryCondition (eGameSettingsVictoryCondition value);

	unsigned int getVictoryTurns() const;
	void setVictoryTurns (unsigned int value);

	unsigned int getVictoryPoints() const;
	void setVictoryPoints (unsigned int value);

	const std::chrono::seconds& getTurnEndDeadline() const;
	void setTurnEndDeadline (const std::chrono::seconds& value);

	bool isTurnEndDeadlineActive() const;
	void setTurnEndDeadlineActive (bool value);

	const std::chrono::seconds& getTurnLimit() const;
	void setTurnLimit (const std::chrono::seconds& value);

	bool isTurnLimitActive() const;
	void setTurnLimitActive (bool value);

	uint32_t getChecksum(uint32_t crc) const;

	mutable cSignal<void ()> metalAmountChanged;
	mutable cSignal<void ()> oilAmountChanged;
	mutable cSignal<void ()> goldAmountChanged;

	mutable cSignal<void ()> resourceDensityChanged;

	mutable cSignal<void ()> bridgeheadTypeChanged;

	mutable cSignal<void ()> gameTypeChanged;

	mutable cSignal<void ()> clansEnabledChanged;

	mutable cSignal<void ()> startCreditsChanged;

	mutable cSignal<void ()> victoryConditionTypeChanged;
	mutable cSignal<void ()> victoryTurnsChanged;
	mutable cSignal<void ()> victoryPointsChanged;

	mutable cSignal<void ()> turnEndDeadlineChanged;
	mutable cSignal<void ()> turnEndDeadlineActiveChanged;

	mutable cSignal<void ()> turnLimitChanged;
	mutable cSignal<void ()> turnLimitActiveChanged;

	template<typename T>
	void serialize(T& archive)
	{
		archive & NVP(metalAmount);
		archive & NVP(oilAmount);
		archive & NVP(goldAmount);
		archive & NVP(resourceDensity);
		archive & NVP(bridgeheadType);
		archive & NVP(gameType);
		archive & NVP(clansEnabled);
		archive & NVP(startCredits);
		archive & NVP(victoryConditionType);
		archive & NVP(victoryTurns);
		archive & NVP(victoryPoints);
		archive & NVP(turnEndDeadline);
		archive & NVP(turnEndDeadlineActive);
		archive & NVP(turnLimit);
		archive & NVP(turnLimitActive);
	}

private:
	eGameSettingsResourceAmount metalAmount;
	eGameSettingsResourceAmount oilAmount;
	eGameSettingsResourceAmount goldAmount;

	eGameSettingsResourceDensity resourceDensity;

	eGameSettingsBridgeheadType bridgeheadType;

	eGameSettingsGameType gameType;

	bool clansEnabled;

	unsigned int startCredits;

	eGameSettingsVictoryCondition victoryConditionType;
	unsigned int victoryTurns;
	unsigned int victoryPoints;

	std::chrono::seconds turnEndDeadline;
	bool turnEndDeadlineActive;

	std::chrono::seconds turnLimit;
	bool turnLimitActive;
};

#endif // ui_graphical_menu_windows_windowgamesettings_gamesettingsH
