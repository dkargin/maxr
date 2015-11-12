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

#include "game/data/units/unit.h"

#include "game/logic/attackjob.h"
#include "game/logic/client.h"
#include "game/logic/clientevents.h"
#include "game/data/map/map.h"
#include "game/data/player/player.h"

#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"

#include "utility/position.h"
#include "utility/box.h"

using namespace std;

//------------------------------------------------------------------------------
cUnit::cUnit (const cDynamicUnitData* unitData, const cStaticUnitData* staticData, cPlayer* owner, unsigned int ID)
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
	, markedAsDone (false)
	, beenAttacked (false)
	, staticData(staticData)
	, storageResCur(0)
{
	if (unitData != nullptr)
		data = *unitData;

	data.setMaximumCurrentValues();

	isBig = (staticData && staticData->isBig);

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
	std::vector<cPosition> adjacentPositions;

	if (isBig)
	{
		adjacentPositions.push_back (position + cPosition (-1, -1));
		adjacentPositions.push_back (position + cPosition (0, -1));
		adjacentPositions.push_back (position + cPosition (1, -1));
		adjacentPositions.push_back (position + cPosition (2, -1));
		adjacentPositions.push_back (position + cPosition (-1, 0));
		adjacentPositions.push_back (position + cPosition (2, 0));
		adjacentPositions.push_back (position + cPosition (-1, 1));
		adjacentPositions.push_back (position + cPosition (2, 1));
		adjacentPositions.push_back (position + cPosition (-1, 2));
		adjacentPositions.push_back (position + cPosition (0, 2));
		adjacentPositions.push_back (position + cPosition (1, 2));
		adjacentPositions.push_back (position + cPosition (2, 2));
	}
	else
	{
		adjacentPositions.push_back (position + cPosition (-1, -1));
		adjacentPositions.push_back (position + cPosition (0, -1));
		adjacentPositions.push_back (position + cPosition (1, -1));
		adjacentPositions.push_back (position + cPosition (-1, 0));
		adjacentPositions.push_back (position + cPosition (1, 0));
		adjacentPositions.push_back (position + cPosition (-1, 1));
		adjacentPositions.push_back (position + cPosition (0, 1));
		adjacentPositions.push_back (position + cPosition (1, 1));
	}
	return adjacentPositions;
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
bool cUnit::isInRange (const cPosition& position) const
{
	const auto distanceSquared = (position - this->position).l2NormSquared();

	return distanceSquared <= Square (data.getRange());
}

//------------------------------------------------------------------------------
bool cUnit::isNextTo (const cPosition& position) const
{
	if (position.x() + 1 < this->position.x() || position.y() + 1 < this->position.y())
		return false;

	const int size = isBig ? 2 : 1;

	if (position.x() - size > this->position.x() || position.y() - size > this->position.y())
		return false;
	return true;
}

//------------------------------------------------------------------------------
bool cUnit::isAbove (const cPosition& position) const
{
	return getArea().withinOrTouches (position);
}

//------------------------------------------------------------------------------
bool cUnit::getIsBig() const
{
	return isBig;
}

//------------------------------------------------------------------------------
void cUnit::setIsBig(bool value)
{
	std::swap(isBig, value);
	if (isBig != value) isBigChanged();
}

//------------------------------------------------------------------------------
cBox<cPosition> cUnit::getArea() const
{
	return cBox<cPosition> (position, position + (isBig ? cPosition (1, 1) : cPosition (0, 0)));
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
bool cUnit::canAttackObjectAt (const cPosition& position, const cMap& map, bool forceAttack, bool checkRange) const
{
	if (staticData->canAttack == false) return false;
	if (data.getShots() <= 0) return false;
	if (data.getAmmo() <= 0) return false;
	if (attacking) return false;
	if (isBeeingAttacked()) return false;
	if (isAVehicle() && static_cast<const cVehicle*> (this)->isUnitLoaded()) return false;
	if (map.isValidPosition (position) == false) return false;
	if (checkRange && isInRange (position) == false) return false;

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

	// TODO: check behaviour in original
	if (data.getHitpoints() == data.getHitpointsMax())
		data.setHitpoints (upgradeVersion->getHitpointsMax());
	data.setHitpointsMax (upgradeVersion->getHitpointsMax());

	// don't change the current ammo-amount!
	data.setAmmoMax (upgradeVersion->getAmmoMax());

	data.setSpeedMax (upgradeVersion->getSpeedMax());

	data.setArmor (upgradeVersion->getArmor());
	data.setScan (upgradeVersion->getScan());
	data.setRange (upgradeVersion->getRange());
	// TODO: check behaviour in original
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
void cUnit::setMarkedAsDone (bool value)
{
	std::swap (markedAsDone, value);
	if (value != markedAsDone) markedAsDoneChanged();
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
bool cUnit::isMarkedAsDone() const
{
	return markedAsDone;
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
const cStaticUnitData& cUnit::getStaticUnitData() const
{
	return *staticData;
}
