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

#include "unitdetails.h"
#include "../label.h"
#include "../../../../video.h"
#include "../../../../upgradecalculator.h"
#include "../../../../player.h"

//------------------------------------------------------------------------------
cUnitDetails::cUnitDetails (const cPosition& position) :
	cWidget (position),
	playerOriginalData (nullptr),
	playerCurrentData (nullptr),
	unitObjectCurrentData (nullptr),
	upgrades (nullptr)
{
	for (size_t i = 0; i < maxRows; ++i)
	{
		amountLabels[i] = addChild (std::make_unique<cLabel> (cBox<cPosition> (position + cPosition (5, 3 + rowHeight * i), position + cPosition (5 + 21, 3 + rowHeight * i + 10)), "", FONT_LATIN_NORMAL, eAlignmentType::Right));
		nameLabels[i] = addChild (std::make_unique<cLabel> (cBox<cPosition> (position + cPosition (30, 3 + rowHeight * i), position + cPosition (30 + 55, 3 + rowHeight * i + 10)), "", FONT_LATIN_NORMAL, eAlignmentType::Left));
	}

	const cPosition size (250, 170);
    surface = AutoSurface (SDL_CreateRGBSurface (0, size.x (), size.y (), Video.getColDepth (), 0, 0, 0, 0));

	SDL_FillRect (surface.get (), NULL, 0xFF00FF);
	SDL_SetColorKey (surface.get (), SDL_TRUE, 0xFF00FF);

	resize (size);
}

//------------------------------------------------------------------------------
void cUnitDetails::draw ()
{
	if (surface != nullptr)
	{
		reset ();

		SDL_Rect position = getArea ().toSdlRect ();
		SDL_BlitSurface (surface.get (), NULL, cVideo::buffer, &position);
	}

	cWidget::draw ();
}

//------------------------------------------------------------------------------
const sID* cUnitDetails::getCurrentUnitId ()
{
	if (playerOriginalData != nullptr)
	{
		return &unitId;
	}
	return nullptr;
}

//------------------------------------------------------------------------------
void cUnitDetails::setUnit (const sID& unitId_, const cPlayer& owner, const sUnitData* unitObjectCurrentData_, const cUnitUpgrade* upgrades_)
{
	unitId = unitId_;

	playerOriginalData = unitId.getUnitDataOriginalVersion (&owner);
	playerCurrentData = owner.getUnitDataCurrentVersion (unitId);

	if (unitObjectCurrentData_ == nullptr)
	{
		unitObjectCurrentData = playerCurrentData;
	}
	else
	{
		unitObjectCurrentData = unitObjectCurrentData_;
	}

	upgrades = upgrades_;
}

//------------------------------------------------------------------------------
void cUnitDetails::setUpgrades (const cUnitUpgrade* upgrades_)
{
	upgrades = upgrades_;
}

//------------------------------------------------------------------------------
void cUnitDetails::drawRow (size_t index, eUnitDataSymbolType symbolType, int amount, const std::string& name, int value1, int value2)
{
	if (index >= maxRows) return;

	if (index != 0)
	{
		SDL_Rect dest = {0, rowHeight * index - 3, surface->w, 1};
		SDL_FillRect (surface.get (), &dest, 0xFFFC0000);
	}

	amountLabels[index]->show ();
	nameLabels[index]->show ();

	amountLabels[index]->setText (iToStr(amount));
	nameLabels[index]->setText (name);
	drawBigSymbols (symbolType, cPosition (95, rowHeight * index), value1, value2);
}

//------------------------------------------------------------------------------
void cUnitDetails::reset ()
{
	if (unitObjectCurrentData == nullptr || playerCurrentData == nullptr || playerOriginalData == nullptr) return;

	const sUnitUpgrade* upgrade = nullptr;

	size_t rowIndex = 0;

	SDL_FillRect (surface.get (), NULL, 0xFF00FF);
	SDL_SetColorKey (surface.get (), SDL_TRUE, 0xFF00FF);

	if (unitObjectCurrentData->canAttack)
	{
		// Damage:
		upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_DAMAGE) : nullptr;
		drawRow (rowIndex++, eUnitDataSymbolType::Attack, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->damage, lngPack.i18n ("Text~Others~Attack_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->damage, playerOriginalData->damage);

		if (!unitObjectCurrentData->explodesOnContact)
		{
			// Shots:
			upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_SHOTS) : nullptr;
			drawRow (rowIndex++, eUnitDataSymbolType::Shots, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->shotsMax, lngPack.i18n ("Text~Others~Shots_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->shotsMax, playerOriginalData->shotsMax);

			// Range:
			upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_RANGE) : nullptr;
			drawRow (rowIndex++, eUnitDataSymbolType::Range, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->range, lngPack.i18n ("Text~Others~Range_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->range, playerOriginalData->range);

			// Ammo:
			upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_AMMO) : nullptr;
			drawRow (rowIndex++, eUnitDataSymbolType::Ammo, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->ammoMax, lngPack.i18n ("Text~Others~Ammo_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->ammoMax, playerOriginalData->ammoMax);
		}
	}

	sUnitData::eStorageResType transport;
	if (unitId.isAVehicle ()) transport = unitObjectCurrentData->storeResType;
	else transport = unitObjectCurrentData->storeResType;

	if (transport != sUnitData::STORE_RES_NONE)
	{
		eUnitDataSymbolType symbolType;
		switch (transport)
		{
		case sUnitData::STORE_RES_METAL:
			symbolType = eUnitDataSymbolType::Metal;
			break;
		case sUnitData::STORE_RES_OIL:
			symbolType = eUnitDataSymbolType::Oil;
			break;
		case sUnitData::STORE_RES_GOLD:
			symbolType = eUnitDataSymbolType::Gold;
			break;
		}
		drawRow (rowIndex++, symbolType, unitObjectCurrentData->storageResMax, lngPack.i18n ("Text~Others~Cargo"), unitObjectCurrentData->storageResMax, playerOriginalData->storageResMax);
	}

	if (unitObjectCurrentData->produceEnergy)
	{
		// Energy production:
		drawRow (rowIndex++, eUnitDataSymbolType::Energy, unitObjectCurrentData->produceEnergy, lngPack.i18n ("Text~Others~Produce_7"), unitObjectCurrentData->produceEnergy, playerOriginalData->produceEnergy);

		// Oil consumption:
		drawRow (rowIndex++, eUnitDataSymbolType::Oil, unitObjectCurrentData->needsOil, lngPack.i18n ("Text~Others~Usage_7"), unitObjectCurrentData->needsOil, playerOriginalData->needsOil);
	}

	if (unitObjectCurrentData->produceHumans)
	{
		// Human production:
		drawRow (rowIndex++, eUnitDataSymbolType::Human, unitObjectCurrentData->produceHumans, lngPack.i18n ("Text~Others~Produce_7"), unitObjectCurrentData->produceHumans, playerOriginalData->produceHumans);
	}

	// Armor:
	upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_ARMOR) : nullptr;
	drawRow (rowIndex++, eUnitDataSymbolType::Armor, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->armor, lngPack.i18n ("Text~Others~Armor_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->armor, playerOriginalData->armor);

	// Hit points:
	upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_HITS) : nullptr;
	drawRow (rowIndex++, eUnitDataSymbolType::Hits, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->hitpointsMax, lngPack.i18n ("Text~Others~Hitpoints_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->hitpointsMax, playerOriginalData->hitpointsMax);

	// Scan:
	if (unitObjectCurrentData->scan)
	{
		upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_SCAN) : nullptr;
		drawRow (rowIndex++, eUnitDataSymbolType::Scan, upgrade ? upgrade->getCurValue () : unitObjectCurrentData->scan, lngPack.i18n ("Text~Others~Scan_7"), upgrade ? upgrade->getCurValue () : unitObjectCurrentData->scan, playerOriginalData->scan);
	}

	// Speed:
	if (unitObjectCurrentData->speedMax)
	{
		upgrade = upgrades ? upgrades->getUpgrade (sUnitUpgrade::UPGRADE_TYPE_SPEED) : nullptr;
		drawRow (rowIndex++, eUnitDataSymbolType::Speed, (upgrade ? upgrade->getCurValue () : unitObjectCurrentData->speedMax) / 4, lngPack.i18n ("Text~Others~Speed_7"), (upgrade ? upgrade->getCurValue () : unitObjectCurrentData->speedMax) / 4, playerOriginalData->speedMax / 4);
	}

	// energy consumption:
	if (unitObjectCurrentData->needsEnergy)
	{
		drawRow (rowIndex++, eUnitDataSymbolType::Energy, unitObjectCurrentData->needsEnergy, lngPack.i18n ("Text~Others~Usage_7"), unitObjectCurrentData->needsEnergy, playerOriginalData->needsEnergy);
	}

	// humans needed:
	if (unitObjectCurrentData->needsHumans)
	{
		drawRow (rowIndex++, eUnitDataSymbolType::Human, unitObjectCurrentData->needsHumans, lngPack.i18n ("Text~Others~Usage_7"), unitObjectCurrentData->needsHumans, playerOriginalData->needsHumans);
	}

	// raw material consumption:
	if (unitObjectCurrentData->needsMetal)
	{
		drawRow (rowIndex++, eUnitDataSymbolType::Metal, unitObjectCurrentData->needsMetal, lngPack.i18n ("Text~Others~Usage_7"), unitObjectCurrentData->needsMetal, playerOriginalData->needsMetal);
	}

	// gold consumption:
	if (unitObjectCurrentData->convertsGold)
	{
		drawRow (rowIndex++, eUnitDataSymbolType::Gold, unitObjectCurrentData->convertsGold, lngPack.i18n ("Text~Others~Usage_7"), unitObjectCurrentData->convertsGold, playerOriginalData->convertsGold);
	}

	// Costs:
	// Do not use unit data but currentVersion data
	// since cost doesn't change unit version
	drawRow (rowIndex++, eUnitDataSymbolType::Metal, playerCurrentData->buildCosts, lngPack.i18n ("Text~Others~Costs"), playerCurrentData->buildCosts, playerOriginalData->buildCosts);

	while (rowIndex < maxRows)
	{
		amountLabels[rowIndex]->hide ();
		nameLabels[rowIndex]->hide ();
		++rowIndex;
	}
}

//------------------------------------------------------------------------------
void cUnitDetails::drawBigSymbols (eUnitDataSymbolType symbolType, const cPosition& position, int value1, int value2)
{
	int maxX = 160;
	auto src = getBigSymbolPosition (symbolType);
	const auto srcSize = src.getSize();
	maxX -= srcSize.x ();

	if (value2 != value1) maxX -= srcSize.x() + 3;
	if (value2 < value1) std::swap (value1, value2);
	int offX = srcSize.x ();
	while (offX * value2 > maxX)
	{
		--offX;
		if (offX < 4)
		{
			value1 /= 2;
			value2 /= 2;
			offX = srcSize.x ();
		}
	}
	SDL_Rect dest = {position.x (), position.y () + (rowHeight-4 - srcSize.y ()) / 2, 0, 0};

	for (int i = 0; i != value1; ++i)
	{
		auto srcRect = src.toSdlRect ();
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get (), &srcRect, surface.get (), &dest);

		dest.x += offX;
	}
	if (value1 == value2) return;

	dest.x += srcSize.x () + 3;
	SDL_Rect mark = {Sint16 (dest.x - srcSize.x () / 2), dest.y, 1, srcSize.y()};

	SDL_FillRect (surface.get (), &mark, 0xFFFC0000);

	if (symbolType == eUnitDataSymbolType::Metal)
	{
		src = getBigSymbolPosition (eUnitDataSymbolType::MetalEmpty);
	}
	for (int i = value1; i != value2; ++i)
	{
		auto srcRect = src.toSdlRect ();
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get (), &srcRect, surface.get (), &dest);

		dest.x += offX;
	}
}

//------------------------------------------------------------------------------
cBox<cPosition> cUnitDetails::getBigSymbolPosition (eUnitDataSymbolType symbolType)
{
	cPosition position (0, 109);
	cPosition size (1, 1);

	switch (symbolType)
	{
	case eUnitDataSymbolType::Speed:
		position.x () = 0;
		size.x () = 11;
		size.y () = 12;
		break;
	case eUnitDataSymbolType::Hits:
		position.x () = 11;
		size.x () = 7;
		size.y () = 11;
		break;
	case eUnitDataSymbolType::Ammo:
		position.x () = 18;
		size.x () = 9;
		size.y () = 14;
		break;
	case eUnitDataSymbolType::Attack:
		position.x () = 27;
		size.x () = 10;
		size.y () = 14;
		break;
	case eUnitDataSymbolType::Shots:
		position.x () = 37;
		size.x () = 15;
		size.y () = 7;
		break;
	case eUnitDataSymbolType::Range:
		position.x () = 52;
		size.x () = 13;
		size.y () = 13;
		break;
	case eUnitDataSymbolType::Armor:
		position.x () = 65;
		size.x () = 11;
		size.y () = 14;
		break;
	case eUnitDataSymbolType::Scan:
		position.x () = 76;
		size.x () = 13;
		size.y () = 13;
		break;
	case eUnitDataSymbolType::Metal:
		position.x () = 89;
		size.x () = 12;
		size.y () = 15;
		break;
	case eUnitDataSymbolType::MetalEmpty:
		position.x () = 175;
		size.x () = 12;
		size.y () = 15;
		break;
	case eUnitDataSymbolType::Oil:
		position.x () = 101;
		size.x () = 11;
		size.y () = 12;
		break;
	case eUnitDataSymbolType::Gold:
		position.x () = 112;
		size.x () = 13;
		size.y () = 10;
		break;
	case eUnitDataSymbolType::Energy:
		position.x () = 125;
		size.x () = 13;
		size.y () = 17;
		break;
	case eUnitDataSymbolType::Human:
		position.x () = 138;
		size.x () = 12;
		size.y () = 16;
		break;
	case eUnitDataSymbolType::TransportTank:
	case eUnitDataSymbolType::TransportAir:
		break;
	}

	return cBox<cPosition> (position, position + size - 1);
}
