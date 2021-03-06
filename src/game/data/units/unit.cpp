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

#include "unit.h"

#include "game/logic/attackjob.h"
#include "game/logic/client.h"
#include "game/logic/clientevents.h"
#include "game/data/map/map.h"
#include "game/data/player/player.h"
#include "game/data/map/mapview.h"

#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"

#include "utility/drawing.h"
#include "utility/box.h"
#include "utility/crc.h"
#include "utility/listhelpers.h"

using namespace std;

bool cStaticUnitData::setGraphics(const std::string& layer, const cRenderablePtr& sprite)
{
	if(layer == "main" || layer == "image")
	{
		image = sprite;
		// Dirty hack to override colorkey for unit
		if(hasPlayerColor && sprite)
		{
			//image->setColorKey(0xffffff);
		}
	}
	else if(layer == "shadow")
	{
		shadow = sprite;
		if(shadow)
		{
			// Hardcoding it untill all units has a proper XML setting for it
			shadow->setAlphaKey(50);
		}
	}
	else if(layer == "underlay")
		underlay = sprite;
	else
		return false;
	return true;
}

/*static*/ void cStaticUnitData::renderFactionShadowSprite(const cRenderablePtr& image, const cRenderablePtr& shadow, cRenderContext& context, const sRenderOps& ops)
{
	sRenderOps local_ops = ops;

	cRenderContext tmpContext = context;

	SDL_Rect tmpRect = context.dstRect;
	tmpRect.x = 0;
	tmpRect.y = 0;
	SDL_Surface* tmpSurface = GraphicsData.gfx_tmp.get();
	tmpContext.setTarget(tmpSurface, tmpRect);

	if(ops.shadow && shadow)
		shadow->render(context);

	// blit the players color and building graphic
	if (ops.owner)
	{
		SDL_Rect rc = tmpRect;
		SDL_BlitSurface(ops.owner->getColor().getTexture(), nullptr, tmpSurface, &rc);
		tmpContext.overrideColorkey = true;
		tmpContext.colorkey = 0xffffff;
	}
	else
	{
		SDL_Rect rc = tmpRect;
		SDL_FillRect(tmpSurface, &rc, 0xff00ff);
	}

	if(image)
		image->render(tmpContext);

	int alpha = ops.alpha;
	SDL_SetSurfaceAlphaMod (tmpSurface, alpha);
	SDL_Rect rc = context.dstRect;
	SDL_SetColorKey(tmpSurface, SDL_TRUE, 0xFF00FF);
	SDL_BlitSurface (tmpSurface, &tmpRect, context.surface, &rc);
}

void cStaticUnitData::render(cRenderContext& context, const sRenderOps& ops) const
{
	sRenderOps local_ops = ops;

	cRenderContext tmpContext = context;

	SDL_Rect tmpRect = context.dstRect;
	tmpRect.x = 0;
	tmpRect.y = 0;
	SDL_Surface* tmpSurface = GraphicsData.gfx_tmp.get();
	tmpContext.setTarget(tmpSurface, tmpRect);

	if(ops.shadow && shadow)
		shadow->render(context);

	// blit the players color and building graphic
	if (hasPlayerColor && ops.owner)
	{
		SDL_Rect rc = tmpRect;
		SDL_BlitSurface(ops.owner->getColor().getTexture(), nullptr, tmpSurface, &rc);
		tmpContext.overrideColorkey = true;
		tmpContext.colorkey = 0xffffff;
	}
	else
	{
		SDL_Rect rc = tmpRect;
		//ColorRaw color = SDL_MapRGB(tmpSurface->format, ops.background.r, ops.background.g, ops.background.b);
		SDL_FillRect(tmpSurface, &rc, 0xff00ff);
	}

	if(image)
		image->render(tmpContext);

	int alpha = ops.alpha;
	SDL_SetSurfaceAlphaMod (tmpSurface, alpha);
	SDL_Rect rc = context.dstRect;
	SDL_SetColorKey(tmpSurface, SDL_TRUE, 0xFF00FF);
	SDL_BlitSurface (tmpSurface, &tmpRect, context.surface, &rc);

	//cSpriteTool::saveImage(tmpSurface, &tmpRect, name + ".cache.png");
}

//------------------------------------------------------------------------------
cUnit::cUnit (const cDynamicUnitData* unitData, cStaticUnitDataPtr _staticData, cPlayer* owner, unsigned int ID)
	: iID (ID)
	, dir (0)
	, job (nullptr)
	, alphaEffectValue (0)
	, owner (owner)
	, position (0, 0)
	, turnsDisabled (0)
	, sentryActive (false)
	, manualFireActive (false)
	, attacking (false)
	, beeingAttacked (false)
	, beenAttacked (false)
	, staticData(_staticData)
	, storageResCur(0)
{
	if (unitData != nullptr)
		data = *unitData;

	data.setMaximumCurrentValues();

	//isBig = (staticData && staticData->isBig);
	//cellSize = (staticData && staticData->isBig) ? 6 : 3;
	cellSize = (staticData && staticData->cellSize ) ? staticData->cellSize : 1;

	disabledChanged.connect ([&]() { statusChanged(); });
	sentryChanged.connect ([&]() { statusChanged(); });
	manualFireChanged.connect ([&]() { statusChanged(); });
	attackingChanged.connect ([&]() { statusChanged(); });
	beeingAttackedChanged.connect ([&]() { statusChanged(); });

	layingMinesChanged.connect ([&]() { statusChanged(); });
	clearingMinesChanged.connect ([&]() { statusChanged(); });
	buildingChanged.connect ([&]() { statusChanged(); });
	clearingChanged.connect ([&]() { statusChanged(); });
	workingChanged.connect ([&]() { statusChanged(); });
	movingChanged.connect ([&]() { statusChanged(); });
}

//------------------------------------------------------------------------------
cUnit::~cUnit()
{
	destroyed();
}

//------------------------------------------------------------------------------
cPlayer* cUnit::getOwner() const
{
	return owner;
}

//------------------------------------------------------------------------------
void cUnit::setOwner (cPlayer* owner_)
{
	std::swap (owner, owner_);
	if (owner != owner_) ownerChanged();
}

//--------------------------------------------------------------------------
void cUnit::storeVehicle(cVehicle& vehicle, cMap& map)
{
	map.deleteVehicle(vehicle);
	vehicle.getOwner()->removeFromScan(vehicle);

	if (vehicle.isSentryActive())
	{
		vehicle.getOwner()->deleteSentry(vehicle);
	}

	if (vehicle.getMoveJob()) vehicle.getMoveJob()->stop();
	vehicle.setManualFireActive(false);

	vehicle.setLoaded(true);
	vehicle.setIsBeeinAttacked(false);

	storedUnits.push_back(&vehicle);
	storedUnitsChanged();
}

//------------------------------------------------------------------------------
void cUnit::exitVehicleTo(cVehicle& vehicle, const cPosition& position, cMap& map)
{
	Remove(storedUnits, &vehicle);
	storedUnitsChanged();
	vehicle.setLoaded(false);

	vehicle.setPosition(position);
	map.addVehicle(vehicle, position);

	vehicle.getOwner()->addToScan(vehicle);
}

//------------------------------------------------------------------------------
void cUnit::setDetectedByPlayer(const cPlayer* player)
{
	int playerId = player->getId();

	if (!Contains(detectedByPlayerList, playerId))
	{
		detectedByPlayerList.push_back(playerId);
		player->detectedStealthUnit(*this);
	}

	if (!Contains(detectedInThisTurnByPlayerList, playerId))
		detectedInThisTurnByPlayerList.push_back(playerId);

}

//------------------------------------------------------------------------------
void cUnit::resetDetectedByPlayer(const cPlayer* player)
{
	if (Contains(detectedByPlayerList, player->getId()))
	{
		Remove(detectedByPlayerList, player->getId());
		if (!isAVehicle() || !static_cast<const cVehicle*>(this)->isUnitLoaded())
		{
			player->stealthUnitDissappeared(*this);
		}
	}
	Remove(detectedInThisTurnByPlayerList, player->getId());
}

//------------------------------------------------------------------------------
bool cUnit::isDetectedByPlayer(const cPlayer* player) const
{
	return Contains(detectedByPlayerList, player->getId());
}

//------------------------------------------------------------------------------
bool cUnit::isDetectedByAnyPlayer() const
{
	return detectedByPlayerList.size() > 0;
}

//------------------------------------------------------------------------------
void cUnit::clearDetectedInThisTurnPlayerList()
{
	detectedInThisTurnByPlayerList.clear();
}

//------------------------------------------------------------------------------
const cPosition& cUnit::getPosition() const
{
	return position;
}

//------------------------------------------------------------------------------
void cUnit::setPosition (cPosition position_)
{
	std::swap (position, position_);
	if (position != position_) positionChanged();
}

//------------------------------------------------------------------------------
std::vector<cPosition> cUnit::getAdjacentPositions() const
{
	return generateOuterBorder(position, cellSize);
}

//------------------------------------------------------------------------------
/** returns the remaining hitpoints after an attack */
//------------------------------------------------------------------------------
int cUnit::calcHealth (int damage) const
{
	damage -= data.getArmor();

	// minimum damage is 1
	damage = std::max (1, damage);

	const int hp = data.getHitpoints() - damage;
	return std::max (0, hp);
}

//------------------------------------------------------------------------------
/** Checks if the target is in range */
//------------------------------------------------------------------------------
bool cUnit::isInWeaponRange (const cPosition& position) const
{
	const auto distanceSquared = (position - this->position).l2NormSquared();

	return distanceSquared <= Square (data.getRange());
}

cVector2 cUnit::getCenter() const
{
	float x = (float)position.x() + 0.5*cellSize;
	float y = (float)position.y() + 0.5*cellSize;
	return cVector2(x,y);
}
//------------------------------------------------------------------------------
bool cUnit::isNextTo (const cPosition& position, const cStaticUnitData& data) const
{
	return isNextTo(position, data.cellSize, data.cellSize);
}

//------------------------------------------------------------------------------
bool cUnit::isNextTo (const cPosition& pos, int w, int h) const
{
	int left = std::min(position.x(), pos.x());
	int top = std::min(position.y(), pos.y());
	int right = std::max(position.x() + cellSize, pos.x() + w);
	int bottom = std::max(position.y() + cellSize, pos.y() + h);

	return (right - left) == cellSize + w ||
			(bottom - top) == cellSize + h;
}

bool cUnit::isNextTo (const cUnit& other) const
{
	int left = std::min(position.x(), other.position.x());
	int top = std::min(position.y(), other.position.y());
	int right = std::max(position.x() + cellSize, other.position.x() + other.cellSize);
	int bottom = std::max(position.y() + cellSize, other.position.y() + other.cellSize);

	return (right - left) == cellSize + other.cellSize ||
			(bottom - top) == cellSize + other.cellSize;
}

//------------------------------------------------------------------------------
bool cUnit::isAbove (const cPosition& position) const
{
	return getArea().withinOrTouches (position);
}

//------------------------------------------------------------------------------

int cUnit::getCellSize() const
{
	return cellSize;
}

void cUnit::setCellSize(int cs)
{
	std::swap(cellSize, cs);
	if (cellSize != cs)
		unitSizeChanged();
}

uint32_t cUnit::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(data, crc);
	crc = calcCheckSum(iID, crc);
	crc = calcCheckSum(dir, crc);
	for (const auto& u : storedUnits)
		crc = calcCheckSum(u, crc);
	for (const auto& p : seenByPlayerList)
		crc = calcCheckSum(p, crc);
	for (const auto& p : detectedByPlayerList)
		crc = calcCheckSum(p, crc);

	crc = calcCheckSum(cellSize, crc);

	for (const auto& p : detectedInThisTurnByPlayerList)
		crc = calcCheckSum(p, crc);

	crc = calcCheckSum(owner, crc);
	crc = calcCheckSum(position, crc);
	crc = calcCheckSum(customName, crc);
	crc = calcCheckSum(turnsDisabled, crc);
	crc = calcCheckSum(sentryActive, crc);
	crc = calcCheckSum(manualFireActive, crc);
	crc = calcCheckSum(attacking, crc);
	crc = calcCheckSum(beeingAttacked, crc);
	crc = calcCheckSum(beenAttacked, crc);
	crc = calcCheckSum(storageResCur, crc);

	return crc;
}

bool cUnit::hasStaticFlag(UnitFlag flag) const
{
	return getStaticUnitData().hasFlag(flag);
}

//------------------------------------------------------------------------------
cBox<cPosition> cUnit::getArea() const
{
	return cBox<cPosition> (position, position + cellSize);
}

// http://rosettacode.org/wiki/Roman_numerals/Encode#C.2B.2B
static std::string to_roman (unsigned int value)
{
	struct romandata_t { unsigned int value; char const* numeral; };
	const struct romandata_t romandata[] =
	{
		//{1000, "M"}, {900, "CM"},
		//{500, "D"}, {400, "CD"},
		{100, "C"}, { 90, "XC"},
		{ 50, "L"}, { 40, "XL"},
		{ 10, "X"}, { 9, "IX"},
		{ 5, "V"}, { 4, "IV"},
		{ 1, "I"},
		{ 0, nullptr} // end marker
	};

	std::string result;
	for (const romandata_t* current = romandata; current->value > 0; ++current)
	{
		while (value >= current->value)
		{
			result += current->numeral;
			value -= current->value;
		}
	}
	return result;
}

//------------------------------------------------------------------------------
std::string cUnit::getName() const
{
	if (!customName.empty())
		return customName;
	else
		return staticData->getName();
}

//------------------------------------------------------------------------------
/** generates the name for the unit depending on the versionnumber */
//------------------------------------------------------------------------------
string cUnit::getNamePrefix() const
{
	string rome = "MK ";
	// +1, because the numbers in the name start at 1, not at 0
	unsigned int nr = data.getVersion() + 1;

	return "MK " + to_roman (nr);
}

//------------------------------------------------------------------------------
/** Returns the name of the vehicle how it should be displayed */
//------------------------------------------------------------------------------
string cUnit::getDisplayName() const
{
	return getNamePrefix() + " " + getName();
}

//------------------------------------------------------------------------------
/** changes the name of the unit and indicates it as "not default" */
//------------------------------------------------------------------------------
void cUnit::changeName (const string& newName)
{
	customName = newName;
	renamed();
}

//------------------------------------------------------------------------------
/** rotates the unit to the given direction */
//------------------------------------------------------------------------------
void cUnit::rotateTo (int newDir)
{
	if (newDir < 0 || newDir >= 8 || newDir == dir)
		return;

	int t = dir;
	int dest = 0;

	for (int i = 0; i < 8; ++i)
	{
		if (t == newDir)
		{
			dest = i;
			break;
		}
		++t;

		if (t > 7)
			t = 0;
	}

	if (dest < 4)
		++dir;
	else
		--dir;

	if (dir < 0)
		dir += 8;
	else
	{
		if (dir > 7)
			dir -= 8;
	}
}

//------------------------------------------------------------------------------
/** Checks, if the unit can attack an object at the given coordinates*/
//------------------------------------------------------------------------------
bool cUnit::canAttackObjectAt (const cPosition& position, const cMapView& map, bool forceAttack, bool checkRange) const
{
	if (staticData->canAttack == TERRAIN_NONE) return false;
	if (data.getShots() <= 0) return false;
	if (data.getAmmo() <= 0) return false;
	if (attacking) return false;
	if (isUnitMoving()) return false;
	if (isBeeingAttacked()) return false;
	if (isAVehicle() && static_cast<const cVehicle*> (this)->isUnitLoaded()) return false;
	if (map.isValidPosition (position) == false) return false;
	if (checkRange && isInWeaponRange (position) == false) return false;

	if (staticData->muzzleType == cStaticUnitData::MUZZLE_TYPE_TORPEDO && map.isWaterOrCoast(position) == false)
		return false;

	const cUnit* target = cAttackJob::selectTarget(position, staticData->canAttack, map, owner);

	if (target && target->iID == iID)  // a unit cannot fire on itself
		return false;

	if (!owner->canSeeAt (position) && !forceAttack)
		return false;

	if (forceAttack)
		return true;

	if (target == nullptr)
		return false;

	// do not fire on e.g. platforms, connectors etc.
	// see ticket #253 on bug tracker
	if (target->isABuilding() && isAVehicle() && staticData->factorAir == 0 && map.possiblePlace (*static_cast<const cVehicle*> (this), position))
		return false;

	if (target->owner == owner)
		return false;

	return true;
}

//------------------------------------------------------------------------------
void cUnit::upgradeToCurrentVersion()
{
	if (owner == nullptr) return;
	const cDynamicUnitData* upgradeVersion = owner->getUnitDataCurrentVersion (data.getId());
	if (upgradeVersion == nullptr) return;

	data.setVersion (upgradeVersion->getVersion());

	// keep difference between max and current hitpoints
	int missingHitpoints = data.getHitpointsMax() - data.getHitpoints();
	data.setHitpoints (upgradeVersion->getHitpointsMax() - missingHitpoints);

	data.setHitpointsMax (upgradeVersion->getHitpointsMax());

	// don't change the current ammo-amount!
	data.setAmmoMax (upgradeVersion->getAmmoMax());

	// don't change the current speed-amount!
	data.setSpeedMax (upgradeVersion->getSpeedMax());

	data.setArmor (upgradeVersion->getArmor());
	data.setScan (upgradeVersion->getScan());
	data.setRange (upgradeVersion->getRange());
	// don't change the current shot-amount!
	data.setShotsMax (upgradeVersion->getShotsMax());
	data.setDamage (upgradeVersion->getDamage());
	data.setBuildCost(upgradeVersion->getBuildCost());
}

//------------------------------------------------------------------------------
void cUnit::setDisabledTurns (int turns)
{
	std::swap (turnsDisabled, turns);
	if (turns != turnsDisabled) disabledChanged();
}

//------------------------------------------------------------------------------
void cUnit::setSentryActive (bool value)
{
	std::swap (sentryActive, value);
	if (value != sentryActive) sentryChanged();
}

//------------------------------------------------------------------------------
void cUnit::setManualFireActive (bool value)
{
	std::swap (manualFireActive, value);
	if (value != manualFireActive) manualFireChanged();
}

//------------------------------------------------------------------------------
void cUnit::setAttacking (bool value)
{
	std::swap (attacking, value);
	if (value != attacking) attackingChanged();
}

//------------------------------------------------------------------------------
void cUnit::setIsBeeinAttacked (bool value)
{
	std::swap (beeingAttacked, value);
	if (value != beeingAttacked) beeingAttackedChanged();
}

//------------------------------------------------------------------------------
void cUnit::setHasBeenAttacked (bool value)
{
	std::swap (beenAttacked, value);
	if (value != beenAttacked) beenAttackedChanged();
}

//------------------------------------------------------------------------------
int cUnit::getDisabledTurns() const
{
	return turnsDisabled;
}

//------------------------------------------------------------------------------
bool cUnit::isSentryActive() const
{
	return sentryActive;
}

//------------------------------------------------------------------------------
bool cUnit::isManualFireActive() const
{
	return manualFireActive;
}

//------------------------------------------------------------------------------
bool cUnit::isAttacking() const
{
	return attacking;
}

//------------------------------------------------------------------------------
bool cUnit::isBeeingAttacked() const
{
	return beeingAttacked;
}

//------------------------------------------------------------------------------
bool cUnit::hasBeenAttacked() const
{
	return beenAttacked;
}
//------------------------------------------------------------------------------
int cUnit::getStoredResources() const
{
	return storageResCur;
}

//------------------------------------------------------------------------------
void cUnit::setStoredResources(int value)
{
	value = std::max(std::min(value, staticData->storageResMax), 0);
	std::swap(storageResCur, value);
	if (storageResCur != value) storedResourcesChanged();
}

//------------------------------------------------------------------------------
bool cUnit::isStealthOnCurrentTerrain(const cMapField& field, const sTerrain& terrain) const
{
	if (staticData->isStealthOn & AREA_EXP_MINE)
	{
		return true;
	}
	else if (staticData->factorAir > 0 &&
		isAVehicle() &&
		static_cast<const cVehicle*>(this)->getFlightHeight() > 0)
	{
		return (staticData->isStealthOn & TERRAIN_AIR) != 0;
	}
	else if ((field.hasBridgeOrPlattform() && staticData->factorGround > 0) ||
			(!terrain.coast && !terrain.water))
	{
		return (staticData->isStealthOn & TERRAIN_GROUND) != 0;
	}
	else if (terrain.coast)
	{
		return (staticData->isStealthOn & TERRAIN_COAST) != 0;
	}
	else if (terrain.water)
	{
		return (staticData->isStealthOn & TERRAIN_SEA) != 0;
	}

	return false;
}

//------------------------------------------------------------------------------
void cUnit::detectThisUnit(const cMap& map, const std::vector<std::shared_ptr<cPlayer>>& playerList)
{
	if (staticData->isStealthOn == TERRAIN_NONE) return;

	for (const auto& player : playerList)
	{
		if (checkDetectedByPlayer(*player, map))
		{
			setDetectedByPlayer(player.get());
		}
	}
}

//------------------------------------------------------------------------------
void cUnit::detectOtherUnits(const cMap& map) const
{
	if (staticData->canDetectStealthOn == TERRAIN_NONE) return;

	const int minx = std::max(getPosition().x() - data.getScan(), 0);
	const int maxx = std::min(getPosition().x() + data.getScan(), map.getSize().x() - 1);
	const int miny = std::max(getPosition().y() - data.getScan(), 0);
	const int maxy = std::min(getPosition().y() + data.getScan(), map.getSize().y() - 1);

	for (int x = minx; x <= maxx; ++x)
	{
		for (int y = miny; y <= maxy; ++y)
		{
			const cPosition checkAtPos(x, y);
			int scanSquared = data.getScan()*data.getScan();
			if ((getPosition() - checkAtPos).l2NormSquared() > scanSquared) continue;

			const auto& vehicles = map.getField(checkAtPos).getVehicles();
			for (const auto& vehicle : vehicles)
			{
				if (vehicle->checkDetectedByPlayer(*owner, map))
				{
					vehicle->setDetectedByPlayer(owner);

				}
			}
			const auto& buildings = map.getField(checkAtPos).getBuildings();
			for (const auto& building : buildings)
			{
				if (building->checkDetectedByPlayer(*owner, map))
				{
					building->setDetectedByPlayer(owner);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
const cStaticUnitData& cUnit::getStaticUnitData() const
{
	return *staticData;
}

//------------------------------------------------------------------------------
bool cUnit::checkDetectedByPlayer(const cPlayer& player, const cMap& map) const
{
	//big stealth units not implemented yet
	if (cellSize > 1)
		return false;

	if (&player == owner)
		return false;

	if (staticData->isStealthOn == TERRAIN_NONE)
		return false;

	if (isAVehicle() && static_cast<const cVehicle*>(this)->isUnitLoaded())
		return false;

	// get current terrain
	bool isOnWater = map.isWater(position);
	bool isOnCoast = map.isCoast(position);

	if (staticData->factorGround > 0 && map.getField(position).hasBridgeOrPlattform())
	{
		isOnWater = false;
		isOnCoast = false;
	}

	// check stealth status
	if (!isStealthOnCurrentTerrain(map.getField(position), map.staticMap->getTerrain(position)) && player.canSeeAnyAreaUnder(*this))
	{
		return true;
	}
	else if ((staticData->isStealthOn & TERRAIN_GROUND) && player.hasLandDetection(position) && !isOnCoast && !isOnWater)
	{
		return true;
	}
	else if ((staticData->isStealthOn & TERRAIN_SEA) && player.hasSeaDetection(position) && isOnWater)
	{
		return true;
	}
	else if ((staticData->isStealthOn & TERRAIN_COAST) && player.hasLandDetection(position) && isOnCoast && staticData->factorGround > 0)
	{
		return true;
	}
	else if ((staticData->isStealthOn & TERRAIN_COAST) && player.hasSeaDetection(position) && isOnCoast && staticData->factorSea > 0)
	{
		return true;
	}
	else if ((staticData->isStealthOn & AREA_EXP_MINE) && player.hasMineDetection(position))
	{
		return true;
	}
	//TODO: isStealthOn & TERRAIN_AIR

	return false;
}

std::vector<cPosition> generateBorder(const cPosition& corner, int size)
{
	if (size < 1)
		size = 1;
	std::vector<cPosition> result(size*4-4);
	int dx = 1;
	int dy = 1;
	bool hor = true;
	int x = corner.x();
	int y = corner.y();

	int step = size-1;

	//2 -> 1

	for(cPosition& pos: result)
	{
		pos.x() = x;
		pos.y() = y;

		step--;

		if(hor)
		{
			x += dx;
			if (step == 0)
			{
				dx = -dx;
				hor = false;
				step = size-1;
			}
		}
		else
		{
			y += dy;
			if (step == 0)
			{
				dy = -dy;
				hor = true;
				step = size-1;
			}
		}
	}

	return std::move(result);
}

std::vector<cPosition> generateOuterBorder(const cPosition& corner, int size)
{
	return generateBorder(corner-1, size+2);
}


// Generate array of adjacent cells
// Direct adjacency is checked: left, right, upper and bottom sides are concidered adjacent
// Corner sides are skipped
std::vector<cAdjPosition> generateAdjacentBorder(const cPosition& corner, int size)
{
	if (size < 1)
			size = 1;

	std::vector<cAdjPosition> result(size*4);
	for(int i = 0; i < size; i++)
	{
		// Left side
		result[i] = std::make_pair(corner.relative(-1,i), AdjLeft);
		// Top side
		result[i+size] = std::make_pair(corner.relative(i,-1), AdjTop);
		// Right side
		result[i+size*2] = std::make_pair(corner.relative(size, size-i-1), AdjRight);
		// Bottom side
		result[i+size*3] = std::make_pair(corner.relative(size-i-1, size), AdjBottom);
	}

	return std::move(result);
}
