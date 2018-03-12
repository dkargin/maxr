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
#include <cmath>

#include "building.h"

#include "game/logic/client.h"
#include "game/logic/clientevents.h"
#include "utility/listhelpers.h"
#include "game/logic/fxeffects.h"
#include "main.h"
#include "netmessage.h"
#include "utility/pcx.h"
#include "game/data/player/player.h"
#include "game/logic/server.h"
#include "game/logic/serverevents.h"
#include "settings.h"
#include "game/logic/upgradecalculator.h"
#include "game/data/units/vehicle.h"
#include "video.h"
#include "utility/unifonts.h"
#include "game/data/report/savedreportsimple.h"
#include "game/data/report/special/savedreportresourcechanged.h"
#include "utility/random.h"

#include "ui/sound/soundmanager.h"
#include "ui/sound/effects/soundeffectvoice.h"
#include "utility/crc.h"
#include "utility/drawing.h"
#include "game/data/map/mapview.h"
#include "game/data/map/mapfieldview.h"

using namespace std;

bool cBuildingData::setGraphics(const std::string& layer, const cRenderablePtr& sprite)
{
	if(layer == "effect")
		this->effect = sprite;
	else
		return cStaticUnitData::setGraphics(layer, sprite);
	return true;
}

void cBuildingData::render(cRenderContext& context, const sRenderOps& ops) const
{
	cStaticUnitData::render(context, ops);
}

//--------------------------------------------------------------------------
cBuildListItem::cBuildListItem()
{
	remainingMetal = 0;
}

//--------------------------------------------------------------------------
cBuildListItem::cBuildListItem (sID type_, int remainingMetal_) :
	type (type_),
	remainingMetal (remainingMetal_)
{}

//--------------------------------------------------------------------------
cBuildListItem::cBuildListItem (const cBuildListItem& other) :
	type (other.type),
	remainingMetal (other.remainingMetal)
{}

//--------------------------------------------------------------------------
cBuildListItem::cBuildListItem (cBuildListItem&& other) :
	type (std::move (other.type)),
	remainingMetal (std::move (other.remainingMetal))
{}

//--------------------------------------------------------------------------
cBuildListItem& cBuildListItem::operator= (const cBuildListItem& other)
{
	type = other.type;
	remainingMetal = other.remainingMetal;
	return *this;
}

//--------------------------------------------------------------------------
cBuildListItem& cBuildListItem::operator= (cBuildListItem && other)
{
	type = std::move (other.type);
	remainingMetal = std::move (other.remainingMetal);
	return *this;
}

//--------------------------------------------------------------------------
const sID& cBuildListItem::getType() const
{
	return type;
}

//--------------------------------------------------------------------------
void cBuildListItem::setType (const sID& type_)
{
	auto oldType = type;
	type = type_;
	if (type != oldType) typeChanged();
}

//--------------------------------------------------------------------------
int cBuildListItem::getRemainingMetal() const
{
	return remainingMetal;
}

//--------------------------------------------------------------------------
void cBuildListItem::setRemainingMetal (int value)
{
	std::swap (remainingMetal, value);
	if (value != remainingMetal) remainingMetalChanged();
}

//--------------------------------------------------------------------------
uint32_t cBuildListItem::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(type, crc);
	crc = calcCheckSum(remainingMetal, crc);

	return crc;
}

//--------------------------------------------------------------------------
// cBuilding Implementation
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
cBuilding::cBuilding (sBuildingDataPtr data, const cDynamicUnitData* ddata, cPlayer* owner, unsigned int ID) :
	cUnit(ddata, data, owner, ID),
	buildingData(data),
	isWorking (false),
	wasWorking(false),
	metalPerRound(0),
	effectAlpha(0)
{
	setSentryActive (staticData && staticData->canAttack != TERRAIN_NONE);

	rubbleTyp = 0;
	rubbleValue = 0;
	researchArea = cResearch::kAttackResearch;
	points = 0;

	repeatBuild = false;

	maxMetalProd = 0;
	maxGoldProd = 0;
	maxOilProd = 0;
	metalProd = 0;
	goldProd = 0;
	oilProd = 0;

	subBase = nullptr;
	buildSpeed = 0;

	if (cellSize > 1)
	{
		DamageFXPointX  = random (cellSize*64) + 32;
		DamageFXPointY  = random (cellSize*64) + 32;
		DamageFXPointX2 = random (cellSize*64) + 32;
		DamageFXPointY2 = random (cellSize*64) + 32;
	}
	else
	{
		DamageFXPointX = random (64 - 24);
		DamageFXPointY = random (64 - 24);
		DamageFXPointX2 = 0;
		DamageFXPointY2 = 0;
	}

	refreshData();

	buildListChanged.connect ([&]() { statusChanged(); });
	buildListFirstItemDataChanged.connect ([&]() { statusChanged(); });
	researchAreaChanged.connect ([&]() { statusChanged(); });
	metalPerRoundChanged.connect([&]() { statusChanged(); });
	ownerChanged.connect ([&]() { registerOwnerEvents(); });

	registerOwnerEvents();
}

//--------------------------------------------------------------------------
cBuilding::~cBuilding()
{
}

//----------------------------------------------------
/** Returns a string with the current state */
//----------------------------------------------------
string cBuilding::getStatusStr (const cPlayer* whoWantsToKnow, const cUnitsData& unitsData) const
{
	auto font = cUnicodeFont::font.get();
	if (isDisabled())
	{
		string sText;
		sText = lngPack.i18n ("Text~Comp~Disabled") + " (";
		sText += iToStr (getDisabledTurns()) + ")";
		return sText;
	}
	if (isUnitWorking() || (factoryHasJustFinishedBuilding() && isDisabled() == false))
	{
		// Factory:
		if (!staticData->canBuild.empty() && !buildList.empty() && getOwner() == whoWantsToKnow)
		{
			const cBuildListItem& buildListItem = buildList[0];
			const string& unitName = unitsData.getUnit(buildListItem.getType())->getName();
			string sText;

			if (buildListItem.getRemainingMetal() > 0)
			{
				int iRound;

				iRound = (int) ceilf (buildListItem.getRemainingMetal() / (float)getMetalPerRound());
				sText = lngPack.i18n ("Text~Comp~Producing") + lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitName + " (";
				sText += iToStr (iRound) + ")";

				if (font->getTextWide (sText, FONT_LATIN_SMALL_WHITE) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing") + ":\n";
					sText += unitName + " (";
					sText += iToStr (iRound) + ")";
				}

				return sText;
			}
			else //new unit is rdy + which kind of unit
			{
				sText = lngPack.i18n ("Text~Comp~Producing_Fin");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += unitName;

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing_Fin");
					sText += ":\n";
					sText += unitName;
				}
				return sText;
			}
		}

		// Research Center
		if (staticData->hasFlag(UnitFlag::CanResearch) && getOwner() == whoWantsToKnow)
		{
			string sText = lngPack.i18n ("Text~Comp~Working") + "\n";
			for (int area = 0; area < cResearch::kNrResearchAreas; area++)
			{
				if (getOwner()->getResearchCentersWorkingOnArea ((cResearch::ResearchArea)area) > 0)
				{
					switch (area)
					{
						case cResearch::kAttackResearch: sText += lngPack.i18n ("Text~Others~Attack"); break;
						case cResearch::kShotsResearch: sText += lngPack.i18n ("Text~Others~Shots_7"); break;
						case cResearch::kRangeResearch: sText += lngPack.i18n ("Text~Others~Range"); break;
						case cResearch::kArmorResearch: sText += lngPack.i18n ("Text~Others~Armor_7"); break;
						case cResearch::kHitpointsResearch: sText += lngPack.i18n ("Text~Others~Hitpoints_7"); break;
						case cResearch::kSpeedResearch: sText += lngPack.i18n ("Text~Others~Speed"); break;
						case cResearch::kScanResearch: sText += lngPack.i18n ("Text~Others~Scan"); break;
						case cResearch::kCostResearch: sText += lngPack.i18n ("Text~Others~Costs"); break;
					}
					sText += lngPack.i18n ("Text~Punctuation~Colon") + iToStr (getOwner()->getResearchState().getRemainingTurns (area, getOwner()->getResearchCentersWorkingOnArea ((cResearch::ResearchArea)area))) + "\n";
				}
			}
			return sText;
		}

		// Goldraffinerie:
		if (staticData->convertsGold && getOwner() == whoWantsToKnow)
		{
			string sText;
			sText = lngPack.i18n ("Text~Comp~Working") + "\n";
			sText += lngPack.i18n ("Text~Title~Credits") + lngPack.i18n ("Text~Punctuation~Colon");
			sText += iToStr (getOwner()->getCredits());
			return sText;
		}
		return lngPack.i18n ("Text~Comp~Working");
	}

	if (isAttacking())
		return lngPack.i18n ("Text~Comp~AttackingStatusStr");
	else if (isBeeingAttacked())
		return lngPack.i18n ("Text~Comp~IsBeeingAttacked");
	else if (isSentryActive())
		return lngPack.i18n ("Text~Comp~Sentry");
	else if (isManualFireActive())
		return lngPack.i18n ("Text~Comp~ReactionFireOff");

	//GoldRaf idle + gold-amount
	if (staticData->convertsGold && getOwner() == whoWantsToKnow && !isUnitWorking())
	{
		string sText;
		sText = lngPack.i18n("Text~Comp~Waits") + "\n";
		sText += lngPack.i18n("Text~Title~Credits") + lngPack.i18n("Text~Punctuation~Colon");
		sText += iToStr(getOwner()->getCredits());
		return sText;
	}

	//Research centre idle + projects
	// Research Center
	if (staticData->hasFlag(UnitFlag::CanResearch) && getOwner() == whoWantsToKnow && !isUnitWorking())
	{
		string sText = lngPack.i18n("Text~Comp~Waits") + "\n";
		for (int area = 0; area < cResearch::kNrResearchAreas; area++)
		{
			if (getOwner()->getResearchCentersWorkingOnArea((cResearch::ResearchArea)area) > 0)
			{
				switch (area)
				{
				case cResearch::kAttackResearch: sText += lngPack.i18n("Text~Others~Attack"); break;
				case cResearch::kShotsResearch: sText += lngPack.i18n("Text~Others~Shots_7"); break;
				case cResearch::kRangeResearch: sText += lngPack.i18n("Text~Others~Range"); break;
				case cResearch::kArmorResearch: sText += lngPack.i18n("Text~Others~Armor_7"); break;
				case cResearch::kHitpointsResearch: sText += lngPack.i18n("Text~Others~Hitpoints_7"); break;
				case cResearch::kSpeedResearch: sText += lngPack.i18n("Text~Others~Speed"); break;
				case cResearch::kScanResearch: sText += lngPack.i18n("Text~Others~Scan"); break;
				case cResearch::kCostResearch: sText += lngPack.i18n("Text~Others~Costs"); break;
				}
				sText += lngPack.i18n("Text~Punctuation~Colon") + iToStr(getOwner()->getResearchState().getRemainingTurns(area, getOwner()->getResearchCentersWorkingOnArea((cResearch::ResearchArea)area))) + "\n";
			}
		}
		return sText;
	}


	return lngPack.i18n ("Text~Comp~Waits");
}


//--------------------------------------------------------------------------
void cBuilding::makeReport (cSoundManager& soundManager) const
{
	if (hasStaticFlag(UnitFlag::CanResearch) && isUnitWorking() && getOwner() && getOwner()->isCurrentTurnResearchAreaFinished (getResearchArea()))
	{
		soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, VoiceData.VOIResearchComplete));
	}
}

//--------------------------------------------------------------------------
/** Refreshs all data to the maximum values */
//--------------------------------------------------------------------------
bool cBuilding::refreshData()
{
	// NOTE: according to MAX 1.04 units get their shots/movepoints back even if they are disabled

	if (data.getShots() < data.getShotsMax())
	{
		data.setShots (std::min (this->data.getShotsMax(), this->data.getAmmo()));
		return true;
	}
	return false;
}

void cBuilding::render_rubble (cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const
{
	assert (isRubble());

	if (cellSize > 1)
	{
		if(ops.shadow && GraphicsData.big_rubble_shadow)
			GraphicsData.big_rubble_shadow->render(context);

		if(GraphicsData.big_rubble)
			GraphicsData.big_rubble->render(context);
	}
	else
	{
		if(ops.shadow && GraphicsData.small_rubble_shadow)
			GraphicsData.small_rubble_shadow->render(context);

		if(GraphicsData.small_rubble)
			GraphicsData.small_rubble->render(context);
	}
}

//------------------------------------------------------------------------------
void cBuilding::connectFirstBuildListItem()
{
	buildListFirstItemSignalConnectionManager.disconnectAll();
	if (!buildList.empty())
	{
		buildListFirstItemSignalConnectionManager.connect(buildList[0].remainingMetalChanged, [this]() { buildListFirstItemDataChanged(); });
		buildListFirstItemSignalConnectionManager.connect(buildList[0].typeChanged, [this]() { buildListFirstItemDataChanged(); });
	}
}

//------------------------------------------------------------------------------
void cBuilding::render (unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, const cStaticUnitData::sRenderOps& ops) const
{
	cRenderable::sContext context;
	context.surface = surface;
	context.dstRect = dest;
	context.cache = true;
	context.channels["clan"] = this->getOwner()->getClan()+1;
	context.channels["animation"] = animationTime;

	// check, if it is dirt:
	if (isRubble())
	{
		// Rubble graphics can use direction to pick a proper rubble sprite variant
		context.channels["direction"] = dir;
		render_rubble (context, ops);
		return;
	}

	if(ops.underlay && buildingData->underlay)
		buildingData->underlay->render(context);

	// draw the connector slots:
	if ((subBase && !alphaEffectValue) || hasStaticFlag(UnitFlag::ConnectsToBase))
	{
		drawConnectors (context, ops);
	}

	// draw the building
	buildingData->render(context, ops);

	if(isUnitWorking() && buildingData->effect)
	{
		context.channels["alpha"] = effectAlpha;
		buildingData->effect->render(context);
	}
}

//--------------------------------------------------------------------------
void cBuilding::updateNeighbours (const cMap& map)
{
	auto adjacent = generateAdjacentBorder(this->getPosition(), this->getCellSize());
		// find all neighbouring subbases
	for(const cAdjPosition& adjPos : adjacent)
	{
		getOwner()->base.checkNeighbour (adjPos.first, *this, map);
	}
	CheckNeighbours (map);
}

//--------------------------------------------------------------------------
/**
 * Checks, if there are neighbours
 * It was necessary to show pretty connections between connectors and buildings
 * I guess it should be moved to connector logic
 */

//--------------------------------------------------------------------------
void cBuilding::CheckNeighbours (const cMap& map)
{
	int size = this->cellSize;
	// Size of processed tile template
	int templateSize = size+2;
	std::vector<int> tiles(templateSize*templateSize, 0);

	cPosition adjCorner = getPosition()-cPosition(1,1);
	auto outer = generateOuterBorder(getPosition(), size);
	auto inner = generateBorder(getPosition(), size);

	auto index = [templateSize, adjCorner](const cPosition& pos)
	{
		cPosition delta = pos - adjCorner;
		return delta.x() + delta.y()*templateSize;
	};

	for(const cPosition& pos: outer)
	{
		// We are expecting connector to be at the top of the building
		const cBuilding* b = map.getField(pos).getTopBuilding();
		bool val = b && b->getOwner() == getOwner() &&
				(b->hasStaticFlag(UnitFlag::ConnectsToBase) || b->hasStaticFlag(UnitFlag::IsConnector));

		if(val)
		{
			tiles[index(pos)] = 1;
		}
	}

	int connectMode = 0;
	if(hasStaticFlag(UnitFlag::IsConnector))
		connectMode = 1;
	else if(hasStaticFlag(UnitFlag::ConnectsToBase))
		connectMode = 2;

	for(const cPosition& pos: inner)
	{
		if(!map.isValidPosition(pos))
			continue;
		tiles[index(pos)] = connectMode;
	}

	connectorTiles.clear();

	for(const cPosition& pos: inner)
	{
		if(!map.isValidPosition(pos))
			continue;

		int pick = -1;

		bool left = tiles[index(pos.relative(-1, 0))] == 1;
		bool right = tiles[index(pos.relative(1, 0))] == 1;
		bool up = tiles[index(pos.relative(0, -1))] == 1;
		bool down = tiles[index(pos.relative(0, 1))] == 1;
		int center = tiles[index(pos)];
		//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15|
		//|   | | |   |   |   | | |   | | | | |   |   | | | | | | |   | | |
		//| o | o | o-| o |-o | o |-o-|-o | o-| o-|-o |-o |-o-| o-|-o-|-o-|
		//|   |   |   | | |   | | |   |   |   | | | | | | |   | | | | | | |
		// TODO: Hide this rules into XML generic tile description
		// Concrete walls, roads and platforms can be implemented using tiling rules in the future
		if(center > 0)
		{
			if(left && right && up && down)
				pick = 15;
			else if(left && right && down)
				pick = 14;
			else if(up && right && down)
				pick = 13;
			else if(up && left && right)
				pick = 12;
			else if(up && down && left)
				pick = 11;
			else if(left && down)
				pick = 10;
			else if(right && down)
				pick = 9;
			else if(up && right)
				pick = 8;
			else if(up && left)
				pick = 7;
			else if(left && right)
				pick = 6;
			else if(up && down)
				pick = 5;
			else if(left)
				pick = 4;
			else if(down)
				pick = 3;
			else if(right)
				pick = 2;
			else if(up)
				pick = 1;
			else if(center == 1)
				pick = 0;
			else
				pick = -1;
		}

		if(pick >= 0)
		{
			connectorTiles.push_back(std::make_pair(pos-getPosition(), pick));
		}
	}
}

//--------------------------------------------------------------------------
/** Draws the connectors at the building: */
//--------------------------------------------------------------------------
void cBuilding::drawConnectors (cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const
{
	cRenderContext tmpContext = context;

	int tileSize = context.dstRect.w / this->cellSize;

	for(auto tile: this->connectorTiles)
	{
		tmpContext.dstRect = context.dstRect;
		tmpContext.dstRect.w = tileSize;
		tmpContext.dstRect.h = tileSize;
		cPosition pos = tile.first;
		if(tile.second < 0)
			continue;
		tmpContext.channels["connector"] = tile.second;
		tmpContext.dstRect.x += tileSize*pos.x();
		tmpContext.dstRect.y += tileSize*pos.y();

		if(ops.shadow && GraphicsData.connector_shadow)
			GraphicsData.connector_shadow->render(tmpContext);
		if(GraphicsData.connector)
			GraphicsData.connector->render(tmpContext);
	}
}

//--------------------------------------------------------------------------
/** starts the building for the server thread */
//--------------------------------------------------------------------------
void cBuilding::startWork ()
{
	if (isUnitWorking())
	{
		return;
	}

	if (isDisabled())
	{
		//TODO: is report needed?
		//sendSavedReport (server, cSavedReportSimple (eSavedReportType::BuildingDisabled), getOwner());
		return;
	}

	if (subBase && !subBase->startBuilding(this))
		return;

	// research building
	if (hasStaticFlag(UnitFlag::CanResearch))
	{
		getOwner()->startAResearch (researchArea);
	}

	if (hasStaticFlag(UnitFlag::CanScore))
	{
		//sendNumEcos (server, *getOwner());
	}
}

//--------------------------------------------------------------------------
/** Stops the building's working */
//--------------------------------------------------------------------------
void cBuilding::stopWork (bool forced)
{
	if (!isUnitWorking()) return;

	if (subBase && !subBase->stopBuilding(this, forced))
		return;

	if (hasStaticFlag(UnitFlag::CanResearch))
	{
		getOwner()->stopAResearch (researchArea);
	}

	if (hasStaticFlag(UnitFlag::CanScore))
	{
		//sendNumEcos (server, *getOwner());
	}
}

bool cBuilding::canTransferTo(const cPosition& position, const cMapView& map) const
{
	const auto& field = map.getField(position);

	const cUnit* unit = field.getVehicle();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	unit = field.getTopBuilding();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	return false;
}

//------------------------------------------------------------
bool cBuilding::canTransferTo (const cUnit& unit) const
{
	if (unit.getOwner() != getOwner())
		return false;

	if (&unit == this)
		return false;


	if (unit.isAVehicle())
	{
		const cVehicle* v = static_cast<const cVehicle*>(&unit);

		if (v->getStaticUnitData().storeResType != staticData->storeResType)
			return false;

		if (v->isUnitBuildingABuilding() || v->isUnitClearing())
			return false;

		if(subBase)
		{
			for (const auto b : subBase->getBuildings())
			{
				if (b->isNextTo (*v))
					return true;
			}
		}
		return false;
	}
	else if (unit.isABuilding())
	{
		const cBuilding* b = static_cast<const cBuilding*>(&unit);
		if (b->subBase != subBase)
			return false;

		if (staticData->storeResType != b->getStaticUnitData().storeResType)
			return false;

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------
bool cBuilding::canExitTo(const cPosition& position, const cMap& map, const cStaticUnitData& vehicleData) const
{
	if (!map.possiblePlaceVehicle(vehicleData, position, getOwner())) return false;
	if (!isNextTo(position, vehicleData))
		return false;

	return true;
}

//--------------------------------------------------------------------------
bool cBuilding::canExitTo(const cPosition& position, const cMapView& map, const cStaticUnitData& vehicleData) const
{
	if (!map.possiblePlaceVehicle(vehicleData, position)) return false;
	if (!isNextTo(position, vehicleData))
		return false;

	return true;
}

//--------------------------------------------------------------------------
bool cBuilding::canLoad (const cPosition& position, const cMapView& map, bool checkPosition) const
{
	if (map.isValidPosition (position) == false) return false;

	if (canLoad (map.getField (position).getPlane(), checkPosition)) return true;
	else return canLoad (map.getField (position).getVehicle(), checkPosition);
}

//--------------------------------------------------------------------------
/** returns, if the vehicle can be loaded from its position: */
//--------------------------------------------------------------------------
bool cBuilding::canLoad (const cVehicle* vehicle, bool checkPosition) const
{
	if (!vehicle) return false;

	if (vehicle->isUnitLoaded()) return false;

	if (storedUnits.size() == staticData->storageUnitsMax) return false;

	if (checkPosition && !isNextTo(*vehicle)) return false;

	if (!Contains(staticData->storeUnitsTypes, vehicle->getStaticUnitData().isStorageType)) return false;

	if (vehicle->isUnitMoving() || vehicle->isAttacking()) return false;

	if (vehicle->getOwner() != getOwner() || vehicle->isUnitBuildingABuilding() || vehicle->isUnitClearing()) return false;

	if (vehicle->isBeeingAttacked()) return false;

	return true;
}

//-------------------------------------------------------------------------------
// Draws big symbols for the info menu:
//-------------------------------------------------------------------------------
void cBuilding::DrawSymbolBig (eSymbolsBig sym, int x, int y, int maxx, int value, int orgvalue, SDL_Surface* sf)
{
	SDL_Rect src = {0, 0, 0, 0};

	switch (sym)
	{
		case SBSpeed:
			src.x = 0;
			src.y = 109;
			src.w = 11;
			src.h = 12;
			break;

		case SBHits:
			src.x = 11;
			src.y = 109;
			src.w = 7;
			src.h = 11;
			break;

		case SBAmmo:
			src.x = 18;
			src.y = 109;
			src.w = 9;
			src.h = 14;
			break;

		case SBAttack:
			src.x = 27;
			src.y = 109;
			src.w = 10;
			src.h = 14;
			break;

		case SBShots:
			src.x = 37;
			src.y = 109;
			src.w = 15;
			src.h = 7;
			break;

		case SBRange:
			src.x = 52;
			src.y = 109;
			src.w = 13;
			src.h = 13;
			break;

		case SBArmor:
			src.x = 65;
			src.y = 109;
			src.w = 11;
			src.h = 14;
			break;

		case SBScan:
			src.x = 76;
			src.y = 109;
			src.w = 13;
			src.h = 13;
			break;

		case SBMetal:
			src.x = 89;
			src.y = 109;
			src.w = 12;
			src.h = 15;
			break;

		case SBOil:
			src.x = 101;
			src.y = 109;
			src.w = 11;
			src.h = 12;
			break;

		case SBGold:
			src.x = 112;
			src.y = 109;
			src.w = 13;
			src.h = 10;
			break;

		case SBEnergy:
			src.x = 125;
			src.y = 109;
			src.w = 13;
			src.h = 17;
			break;

		case SBHuman:
			src.x = 138;
			src.y = 109;
			src.w = 12;
			src.h = 16;
			break;
	}

	maxx -= src.w;

	if (orgvalue < value)
	{
		maxx -= src.w + 3;
	}

	int offx = src.w;

	while (offx * value >= maxx)
	{
		offx--;

		if (offx < 4)
		{
			value /= 2;
			orgvalue /= 2;
			offx = src.w;
		}
	}

	SDL_Rect dest;
	dest.x = x;
	dest.y = y;

	Uint32 color = SDL_MapRGB (sf->format, 0xFC, 0, 0);
	for (int i = 0; i < value; i++)
	{
		if (i == orgvalue)
		{
			SDL_Rect mark;
			dest.x += src.w + 3;
			mark.x = dest.x - src.w / 2;
			mark.y = dest.y;
			mark.w = 1;
			mark.h = src.h;
			SDL_FillRect (sf, &mark, color);
		}

		SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get(), &src, sf, &dest);

		dest.x += offx;
	}
}

//-------------------------------------------------------------------------------
/** checks the resources that are available under the mining station */
//--------------------------------------------------------------------------

void cBuilding::initMineRessourceProd (const cMap& map)
{
	if (!staticData->canMineMaxRes) return;

	auto position = getPosition();

	maxMetalProd = 0;
	maxGoldProd = 0;
	maxOilProd = 0;

	cPosition pos = position;
	int size = this->getCellSize();
	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++)
		{
			const sResources* res = &map.getResource (pos.relative(x,y));
			switch (res->typ)
			{
				case eResourceType::Metal: maxMetalProd += res->value; break;
				case eResourceType::Gold:  maxGoldProd  += res->value; break;
				case eResourceType::Oil:   maxOilProd   += res->value; break;
			}
		}
	}

	maxMetalProd = min (maxMetalProd, staticData->canMineMaxRes);
	maxGoldProd  = min (maxGoldProd, staticData->canMineMaxRes);
	maxOilProd   = min (maxOilProd, staticData->canMineMaxRes);

	// set default mine allocation
	int freeProductionCapacity = staticData->canMineMaxRes;
	metalProd = maxMetalProd;
	freeProductionCapacity -= metalProd;
	goldProd = min(maxGoldProd, freeProductionCapacity);
	freeProductionCapacity -= goldProd;
	oilProd = min(maxOilProd, freeProductionCapacity);

}

//--------------------------------------------------------------------------
/** calculates the costs and the duration of the 3 buildspeeds
 * for the vehicle with the given base costs
 * iRemainingMetal is only needed for recalculating costs of vehicles
 * in the Buildqueue and is set per default to -1 */
//--------------------------------------------------------------------------
void cBuilding::calcTurboBuild (std::array<int, 3>& turboBuildRounds, std::array<int, 3>& turboBuildCosts, int vehicleCosts, int remainingMetal) const
{
	// first calc costs for a new Vehical

	// 1x
	turboBuildCosts[0] = vehicleCosts;

	// 2x
	int a = turboBuildCosts[0];
	turboBuildCosts[1] = turboBuildCosts[0];

	while (a >= 2 * staticData->needsMetal)
	{
		turboBuildCosts[1] += 2 * staticData->needsMetal;
		a -= 2 * staticData->needsMetal;
	}

	// 4x
	turboBuildCosts[2] = turboBuildCosts[1];
	a = turboBuildCosts[1];

	while (a >= 15)
	{
		turboBuildCosts[2] += (12 * staticData->needsMetal - min(a, 8 * staticData->needsMetal));
		a -= 8 * staticData->needsMetal;
	}

	// now this is a litle bit tricky ...
	// trying to calculate a plausible value,
	// if we are changing the speed of an already started build-job
	if (remainingMetal >= 0)
	{
		float WorkedRounds;

		switch (buildSpeed)  // BuildSpeed here is the previous build speed
		{
			case 0:
				WorkedRounds = (turboBuildCosts[0] - remainingMetal) / (1.f * staticData->needsMetal);
				turboBuildCosts[0] -= (int) (1     *  1 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[1] -= (int) (0.5f  *  4 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[2] -= (int) (0.25f * 12 * staticData->needsMetal * WorkedRounds);
				break;

			case 1:
				WorkedRounds = (turboBuildCosts[1] - remainingMetal) / (float)(4 * staticData->needsMetal);
				turboBuildCosts[0] -= (int) (2    *  1 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[1] -= (int) (1    *  4 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[2] -= (int) (0.5f * 12 * staticData->needsMetal * WorkedRounds);
				break;

			case 2:
				WorkedRounds = (turboBuildCosts[2] - remainingMetal) / (float)(12 * staticData->needsMetal);
				turboBuildCosts[0] -= (int) (4 *  1 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[1] -= (int) (2 *  4 * staticData->needsMetal * WorkedRounds);
				turboBuildCosts[2] -= (int) (1 * 12 * staticData->needsMetal * WorkedRounds);
				break;
		}
	}

	// calc needed turns
	turboBuildRounds[0] = (int) ceilf (turboBuildCosts[0] / (1.f * staticData->needsMetal));

	if (staticData->maxBuildFactor > 1)
	{
		turboBuildRounds[1] = (int) ceilf (turboBuildCosts[1] / (4.f * staticData->needsMetal));
		turboBuildRounds[2] = (int) ceilf (turboBuildCosts[2] / (12.f * staticData->needsMetal));
	}
	else
	{
		turboBuildRounds[1] = 0;
		turboBuildRounds[2] = 0;
	}
}

//--------------------------------------------------------------------------
/*
bool cBuilding::isDetectedByPlayer (const cPlayer* player) const
{
	return Contains (detectedByPlayerList, player);
}

//--------------------------------------------------------------------------
void cBuilding::setDetectedByPlayer (cPlayer* player, bool addToDetectedInThisTurnList)
{
	if (!isDetectedByPlayer (player))
		detectedByPlayerList.push_back (player);
}

//--------------------------------------------------------------------------
void cBuilding::resetDetectedByPlayer (const cPlayer* player)
{
	Remove (detectedByPlayerList, player);
}

//--------------------------------------------------------------------------
void cBuilding::makeDetection (cServer& server)
{
	// check whether the building has been detected by others
	if (staticData->isStealthOn == TERRAIN_NONE) return;

	if (staticData->isStealthOn & AREA_EXP_MINE)
	{
		auto& playerList = server.playerList;
		for (unsigned int i = 0; i < playerList.size(); i++)
		{
			auto& player = *playerList[i];
			if (&player == getOwner()) continue;
			if (player.hasMineDetection (getPosition()))
			{
				setDetectedByPlayer (&player);
			}
		}
	}
}
*/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- methods, that already have been extracted as part of cUnit refactoring
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool cBuilding::factoryHasJustFinishedBuilding() const
{
	return (!buildList.empty() && isUnitWorking() == false && buildList[0].getRemainingMetal() <= 0);
}

//-----------------------------------------------------------------------------
void cBuilding::executeUpdateBuildingCommmand (const cClient& client, bool updateAllOfSameType) const
{
	sendUpgradeBuilding (client, *this, updateAllOfSameType);
}

//-----------------------------------------------------------------------------
bool cBuilding::buildingCanBeStarted() const
{
	return (staticData->hasFlag(UnitFlag::CanWork) && isUnitWorking() == false
		&& (!buildList.empty() || staticData->canBuild.empty()));
}

//-----------------------------------------------------------------------------
bool cBuilding::buildingCanBeUpgraded() const
{
	const cDynamicUnitData& upgraded = *getOwner()->getUnitDataCurrentVersion (data.getId());
	return (data.getVersion() != upgraded.getVersion() && subBase && subBase->getMetalStored() >= 2);
}

//-----------------------------------------------------------------------------
bool cBuilding::isBuildListEmpty() const
{
	return buildList.empty();
}

//-----------------------------------------------------------------------------
size_t cBuilding::getBuildListSize() const
{
	return buildList.size();
}

//-----------------------------------------------------------------------------
const cBuildListItem& cBuilding::getBuildListItem (size_t index) const
{
	return buildList[index];
}

//-----------------------------------------------------------------------------
cBuildListItem& cBuilding::getBuildListItem (size_t index)
{
	return buildList[index];
}

//-----------------------------------------------------------------------------
void cBuilding::setBuildList (std::vector<cBuildListItem> buildList_)
{
	buildList = std::move (buildList_);

	connectFirstBuildListItem();

	buildListChanged();
}

//-----------------------------------------------------------------------------
void cBuilding::addBuildListItem (cBuildListItem item)
{
	buildList.push_back (std::move (item));

	connectFirstBuildListItem();

	buildListChanged();
}

//-----------------------------------------------------------------------------
void cBuilding::removeBuildListItem (size_t index)
{
	buildList.erase (buildList.begin() + index);

	connectFirstBuildListItem();

	buildListChanged();
}

//-----------------------------------------------------------------------------
int cBuilding::getBuildSpeed() const
{
	return buildSpeed;
}

//-----------------------------------------------------------------------------
int cBuilding::getMetalPerRound() const
{
	if (buildList.size() > 0)
		return min(metalPerRound, buildList[0].getRemainingMetal());
	else
		return 0;
}

//-----------------------------------------------------------------------------
bool cBuilding::getRepeatBuild() const
{
	return repeatBuild;
}

//-----------------------------------------------------------------------------
void cBuilding::setWorking (bool value)
{
	std::swap (isWorking, value);
	if (value != isWorking) workingChanged();
}

//-----------------------------------------------------------------------------
void cBuilding::setBuildSpeed(int value)
{
	std::swap(buildSpeed, value);
	if(value != buildSpeed) buildSpeedChanged();
}

//-----------------------------------------------------------------------------
void cBuilding::setMetalPerRound(int value)
{
	std::swap(metalPerRound, value);
	if(value != metalPerRound) metalPerRoundChanged();
}

//-----------------------------------------------------------------------------
void cBuilding::setRepeatBuild(bool value)
{
	std::swap(repeatBuild, value);
	if(value != repeatBuild) repeatBuildChanged();
}

int cBuilding::getMaxProd(eResourceType type) const
{
	switch (type)
	{
	case eResourceType::Metal:
		return maxMetalProd;
	case eResourceType::Gold:
		return maxGoldProd;
	case eResourceType::Oil:
		return maxOilProd;
	default:
		return 0;
	}
}

//-----------------------------------------------------------------------------
void cBuilding::setResearchArea (cResearch::ResearchArea area)
{
	std::swap (researchArea, area);
	if (researchArea != area) researchAreaChanged();
}

//-----------------------------------------------------------------------------
cResearch::ResearchArea cBuilding::getResearchArea() const
{
	return researchArea;
}

//------------------------------------------------------------------------------
void cBuilding::setRubbleValue(int value, cCrossPlattformRandom& randomGenerator)
{
	rubbleValue = value;
	// 2 types for large, and 5 types for small rubble.
	rubbleTyp = randomGenerator.get(10);
}

//------------------------------------------------------------------------------
int cBuilding::getRubbleValue() const
{
	return rubbleValue;
}

//------------------------------------------------------------------------------
uint32_t cBuilding::getChecksum(uint32_t crc) const
{
	crc = cUnit::getChecksum(crc);
	crc = calcCheckSum(rubbleTyp, crc);
	crc = calcCheckSum(rubbleValue, crc);
	/*
	crc = calcCheckSum(BaseN, crc);
	crc = calcCheckSum(BaseE, crc);
	crc = calcCheckSum(BaseS, crc);
	crc = calcCheckSum(BaseW, crc);
	crc = calcCheckSum(BaseBN, crc);
	crc = calcCheckSum(BaseBE, crc);
	crc = calcCheckSum(BaseBS, crc);
	crc = calcCheckSum(BaseBW, crc);
	*/
	crc = calcCheckSum(metalProd, crc);
	crc = calcCheckSum(oilProd, crc);
	crc = calcCheckSum(goldProd, crc);
	crc = calcCheckSum(wasWorking, crc);
	crc = calcCheckSum(points, crc);
	crc = calcCheckSum(isWorking, crc);
	crc = calcCheckSum(buildSpeed, crc);
	crc = calcCheckSum(metalPerRound, crc);
	crc = calcCheckSum(repeatBuild, crc);
	crc = calcCheckSum(maxMetalProd, crc);
	crc = calcCheckSum(maxOilProd, crc);
	crc = calcCheckSum(maxGoldProd, crc);
	crc = calcCheckSum(researchArea, crc);
	crc = calcCheckSum(buildList, crc);

	return crc;
}

//-----------------------------------------------------------------------------
void cBuilding::registerOwnerEvents()
{
	ownerSignalConnectionManager.disconnectAll();

	if (getOwner() == nullptr || staticData == nullptr) return;

	if (staticData->convertsGold)
	{
		ownerSignalConnectionManager.connect (getOwner()->creditsChanged, [&]() { statusChanged(); });
	}

	if (staticData->hasFlag(UnitFlag::CanResearch))
	{
		ownerSignalConnectionManager.connect (getOwner()->researchCentersWorkingOnAreaChanged, [&] (cResearch::ResearchArea) { statusChanged(); });
		ownerSignalConnectionManager.connect (getOwner()->getResearchState().neededResearchPointsChanged, [&] (cResearch::ResearchArea) { statusChanged(); });
		ownerSignalConnectionManager.connect (getOwner()->getResearchState().currentResearchPointsChanged, [&] (cResearch::ResearchArea) { statusChanged(); });
	}
}
