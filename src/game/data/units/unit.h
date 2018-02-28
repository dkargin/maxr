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

#ifndef game_data_units_unitH
#define game_data_units_unitH

#include <string>
#include "utility/signal/signal.h"
#include "utility/position.h"
#include "utility/box.h"
#include "utility/drawing.h"
#include "game/data/resourcetype.h"
#include "sound.h"

class cJob;
class cMap;
class cMapView;
class cMapField;
class cPlayer;
class cSoundManager;
class cUnitsData;
struct sTerrain;

//-----------------------------------------------------------------------------
struct sID
{
	sID() : firstPart(0), secondPart(0) {}
	sID(int first, int second) : firstPart(first), secondPart(second) {}

	std::string getText() const;
	void generate(const std::string& text);

	bool isAVehicle() const { return firstPart == 0; }
	bool isABuilding() const { return firstPart == 1; }

	/** Get the basic version of a unit.
	* @param Owner If Owner is given, his clan will be taken
	*        into consideration for modifications of the unit's values.
	* @return the sUnitData of the owner without upgrades
	*         (but with the owner's clan modifications) */
	//const sUnitData* getUnitDataOriginalVersion (const cPlayer* Owner = nullptr) const;

	bool operator== (const sID& ID) const;
	bool operator!= (const sID& rhs) const { return !(*this == rhs); }
	bool operator< (const sID& rhs) const { return less_vehicleFirst(rhs); }
	bool less_vehicleFirst(const sID& ID) const;
	bool less_buildingFirst(const sID& ID) const;

	uint32_t getChecksum(uint32_t crc) const;

	template<typename T>
	void serialize(T& archive)
	{
		archive & NVP(firstPart);
		archive & NVP(secondPart);
	}

public:
	int firstPart;
	int secondPart;
};

/**
 * Generic unit flags.
 * They are belonging to static data,
 * and not supposed to be changed in realtime
 */
enum class UnitFlag : int
{
	ConnectsToBase,
	CanClearArea,
	CanDriveAndFire,
	CanBuildPath,
	CanBuildRepeat,
	CanBeCaptured,
	CanBeDisabled,
	CanCapture,
	CanDisable,
	CanRepair,
	CanRearm,
	CanResearch,
	CanPlaceMines,
	CanSurvey,
	DoesSelfRepair,
	CanSelfDestroy,
	CanScore,
	CanBeLandedOn,
	CanWork,
	ExplodesOnContact,
	IsHuman,
	HasAnimationMovement,		//< Legacy from old resource system
	UnitFlagMax,
};

enum class UnitType
{
	Vehicle,
	Building,
};

// class for vehicle properties, that are constant and equal for all instances of a unit type
class cStaticUnitData : public std::enable_shared_from_this<cStaticUnitData>
{
public:
	cStaticUnitData();
	virtual ~cStaticUnitData();

	std::string getName() const;
	std::string getDescripton() const;
	void setName(std::string name_){ name = name_; }
	void setDescription(std::string text) { description = text; }

	uint32_t getChecksum(uint32_t crc) const;

	// Set graphics layer
	virtual bool setGraphics(const std::string& layer, const cSpritePtr& sprite);

	// Main
	sID ID;

	// Attack
	enum eMuzzleType
	{
		MUZZLE_TYPE_NONE,
		MUZZLE_TYPE_BIG,
		MUZZLE_TYPE_ROCKET,
		MUZZLE_TYPE_SMALL,
		MUZZLE_TYPE_MED,
		MUZZLE_TYPE_MED_LONG,
		MUZZLE_TYPE_ROCKET_CLUSTER,
		MUZZLE_TYPE_TORPEDO,
		MUZZLE_TYPE_SNIPER
	};
	eMuzzleType muzzleType;

	char canAttack;
public:
	/**
	 * Local UI data. Not synchronized between the players
	 */
	// Sprite to be used
	AutoSurface info;
	// Additional overlay, like radar dish
	cSpritePtr overlay;
	// Directed sprites
	std::array<cSpritePtr, 8> directed_image;
	// Directed shadow sprites
	std::array<cSpritePtr, 8> directed_shadow;

	// Non-directed sprite
	cSpritePtr image;
	cSpritePtr shadow;
	// Image for 'dead' state
	cSpritePtr corpse_image;

	// Sprite that will be rendered below the unit, on the ground
	cSpritePtr underlay;

	// Some common flags
	bool hasDamageEffect = false;
	bool buildUpGraphic = false;
	bool powerOnGraphic = false;
	bool hasPlayerColor = false;
	bool hasCorpse = false;
	bool customGraphics = false;
	int hasFrames = 0;

	// Die Sounds:
	cSoundChunk Wait;
	cSoundChunk WaitWater;
	cSoundChunk Start;
	cSoundChunk StartWater;
	cSoundChunk Stop;
	cSoundChunk StopWater;
	cSoundChunk Drive;
	cSoundChunk DriveWater;
	cSoundChunk Attack;
	cSoundChunk Running;

	std::string canBuild;
	std::string buildAs;

	int maxBuildFactor;

	float factorGround;
	float factorSea;
	float factorAir;
	float factorCoast;

	// Abilities
	float modifiesSpeed;
	int convertsGold;

	// Container for unit flags
	uint64_t flags = 0;

	// Check if unit has specified flag
	bool hasFlag(UnitFlag flag) const
	{
		return flags & (1 << int(flag));
	}

	// Enables specified flag
	// @return if flag value was changed
	bool enableFlag(UnitFlag flag)
	{
		if (hasFlag(flag))
			return false;
		flags |= (1 << int(flag));
		return true;
	}

	// Disables specified flag
	bool disableFlag(UnitFlag flag)
	{
		if (!hasFlag(flag))
			return false;
		flags &= ~(1 << int(flag));
		return true;
	}

	// Set flag to value
	bool setFlag(UnitFlag flag, bool value)
	{
		return (value) ? enableFlag(flag) : disableFlag(flag);
	}

	virtual UnitType getType() const = 0;

	int canMineMaxRes;
	int needsMetal;
	int needsOil;
	int needsEnergy;
	int needsHumans;
	int produceEnergy;
	int produceHumans;

	char isStealthOn;
	char canDetectStealthOn;

	enum eSurfacePosition
	{
		SURFACE_POS_BENEATH_SEA,
		SURFACE_POS_ABOVE_SEA,
		SURFACE_POS_BASE,
		SURFACE_POS_ABOVE_BASE,
		SURFACE_POS_GROUND,
		SURFACE_POS_ABOVE
	};
	eSurfacePosition surfacePosition;

	enum eOverbuildType
	{
		OVERBUILD_TYPE_NO,
		OVERBUILD_TYPE_YES,
		OVERBUILD_TYPE_YESNREMOVE
	};
	eOverbuildType canBeOverbuild;

	int cellSize;
	// Storage
	int storageResMax;
	eResourceType storeResType;

	int storageUnitsMax;
	enum eStorageUnitsImageType
	{
		STORE_UNIT_IMG_NONE,
		STORE_UNIT_IMG_TANK,
		STORE_UNIT_IMG_PLANE,
		STORE_UNIT_IMG_SHIP,
		STORE_UNIT_IMG_HUMAN
	};
	eStorageUnitsImageType storeUnitsImageType;
	std::vector<std::string> storeUnitsTypes;
	std::string isStorageType;

	template<typename T>
	void serialize(T& archive)
	{
		archive & NVP(ID);
		archive & NVP(muzzleType);
		archive & NVP(canAttack);
		archive & NVP(canBuild);
		archive & NVP(buildAs);
		archive & NVP(maxBuildFactor);
		archive & NVP(factorGround);
		archive & NVP(factorSea);
		archive & NVP(factorAir);
		archive & NVP(factorCoast);
		archive & NVP(flags);
		archive & NVP(modifiesSpeed);
		archive & NVP(convertsGold);
		archive & NVP(canMineMaxRes);
		archive & NVP(needsMetal);
		archive & NVP(needsOil);
		archive & NVP(needsEnergy);
		archive & NVP(needsHumans);
		archive & NVP(produceEnergy);
		archive & NVP(produceHumans);
		archive & NVP(isStealthOn);
		archive & NVP(canDetectStealthOn);
		archive & NVP(surfacePosition);
		archive & NVP(canBeOverbuild);
		archive & NVP(cellSize);
		archive & NVP(storageResMax);
		archive & NVP(storeResType);
		archive & NVP(storageUnitsMax);
		archive & NVP(storeUnitsImageType);
		archive & NVP(storeUnitsTypes);
		archive & NVP(isStorageType);
		archive & NVP(description);
		archive & NVP(name);
	}

	// Graphics part
private:
	std::string description; //untranslated data from unit xml. Will be used, when translation for the unit is not available
	std::string name;        //untranslated data from unit xml. Will be used, when translation for the unit is not available
};

typedef std::shared_ptr<cStaticUnitData> cStaticUnitDataPtr;

//class for vehicle properties, that are individual for each instance of a unit
class cDynamicUnitData
{
public:
	cDynamicUnitData();
	cDynamicUnitData(const cDynamicUnitData& other);
	cDynamicUnitData& operator= (const cDynamicUnitData& other);

	void setMaximumCurrentValues();

	sID getId() const;
	void setId(const sID& value);

	int getBuildCost() const;
	void setBuildCost(int value);

	int getVersion() const;
	void setVersion(int value);

	int getSpeed() const;
	void setSpeed(int value);

	int getSpeedMax() const;
	void setSpeedMax(int value);

	int getHitpoints() const;
	void setHitpoints(int value);

	int getHitpointsMax() const;
	void setHitpointsMax(int value);

	int getScan() const;
	void setScan(int value);

	int getRange() const;
	void setRange(int value);

	int getShots() const;
	void setShots(int value);

	int getShotsMax() const;
	void setShotsMax(int value);

	int getAmmo() const;
	void setAmmo(int value);

	int getAmmoMax() const;
	void setAmmoMax(int value);

	int getDamage() const;
	void setDamage(int value);

	int getArmor() const;
	void setArmor(int value);

	uint32_t getChecksum(uint32_t crc) const;

	mutable cSignal<void()> buildCostsChanged;
	mutable cSignal<void()> versionChanged;
	mutable cSignal<void()> speedChanged;
	mutable cSignal<void()> speedMaxChanged;
	mutable cSignal<void()> hitpointsChanged;
	mutable cSignal<void()> hitpointsMaxChanged;
	mutable cSignal<void()> shotsChanged;
	mutable cSignal<void()> shotsMaxChanged;
	mutable cSignal<void()> ammoChanged;
	mutable cSignal<void()> ammoMaxChanged;
	mutable cSignal<void()> scanChanged;
	mutable cSignal<void()> rangeChanged;
	mutable cSignal<void()> damageChanged;
	mutable cSignal<void()> armorChanged;

	template <typename T>
	void serialize(T& archive)
	{
		archive & NVP(id);
		archive & NVP(buildCosts);
		archive & NVP(version);
		archive & NVP(speedCur);
		archive & NVP(speedMax);
		archive & NVP(hitpointsCur);
		archive & NVP(hitpointsMax);
		archive & NVP(shotsCur);
		archive & NVP(shotsMax);
		archive & NVP(ammoCur);
		archive & NVP(ammoMax);
		archive & NVP(range);
		archive & NVP(scan);
		archive & NVP(damage);
		archive & NVP(armor);

		if (!archive.isWriter)
			crcValid = false;
	}
private:
	// Main
	sID id;
	// Production
	int buildCosts;
	int version;
	int speedCur;
	int speedMax;

	int hitpointsCur;
	int hitpointsMax;
	int shotsCur;
	int shotsMax;
	int ammoCur;
	int ammoMax;

	int range;
	int scan;

	int damage;
	int armor;

	mutable uint32_t crcCache;
	mutable bool crcValid;
};

class cVehicle;

//-----------------------------------------------------------------------------
class cUnit
{
protected:
	cUnit(const cDynamicUnitData* unitData, cStaticUnitDataPtr staticData, cPlayer* owner, unsigned int ID);
public:
	virtual ~cUnit();

	unsigned int getId() const { return iID; }

	virtual bool isAVehicle() const = 0;
	virtual bool isABuilding() const = 0;

	cPlayer* getOwner() const;
	void setOwner (cPlayer* owner);

	virtual bool canTransferTo (const cPosition& position, const cMapView& map) const = 0;
	virtual bool canTransferTo (const cUnit& unit) const = 0;
	virtual bool canExitTo (const cPosition& position, const cMapView& map, const cStaticUnitData& unitData) const = 0;
	virtual bool canExitTo (const cPosition& position, const cMap& map, const cStaticUnitData& unitData) const = 0;
	virtual std::string getStatusStr (const cPlayer* whoWantsToKnow, const cUnitsData& unitsData) const = 0;

	virtual void makeReport (cSoundManager& soundManager) const = 0;

	virtual const cPosition& getMovementOffset() const { static const cPosition dummy (0, 0); return dummy; }

	virtual void setDetectedByPlayer (cPlayer* player, bool addToDetectedInThisTurnList = true) = 0;

	const cPosition& getPosition() const;
	void setPosition (cPosition position);

	cBox<cPosition> getArea() const;

	std::vector<cPosition> getAdjacentPositions() const;

	// Get geometric center of the unit
	cVector2 getCenter() const;

	int calcHealth (int damage) const;
	bool isInWeaponRange (const cPosition& position) const;
	/// checks whether the coordinates are next to the unit
	bool isNextTo (const cPosition& position, const cStaticUnitData& data) const;
	bool isNextTo (const cPosition& position, int w, int h) const;
	bool isNextTo (const cUnit& other) const;

	bool isDisabled() const { return turnsDisabled > 0; }
	bool isAbove (const cPosition& position) const;


	std::string getName() const;
	std::string getNamePrefix() const;
	std::string getDisplayName() const;
	void changeName (const std::string& newName);

	void rotateTo (int newDir);

	bool hasStaticFlag(UnitFlag flag) const;

	/** checks if the unit can attack something at the offset.
	 *  when forceAttack is false, the function only returns true,
	 *  if there is an enemy unit
	 */
	bool canAttackObjectAt (const cPosition& position, const cMapView& map, bool forceAttack = false, bool checkRange = true) const;

	/** Upgrades the unit data of this unit to the current,
	 * upgraded version of the player.
	 */
	void upgradeToCurrentVersion();
	/** checks if the unit has stealth abilities on its current position */
	bool isStealthOnCurrentTerrain(const cMapField& field, const sTerrain& terrain) const;

	void setDisabledTurns (int turns);
	void setSentryActive (bool value);
	void setManualFireActive (bool value);
	void setAttacking (bool value);
	void setIsBeeinAttacked (bool value);
	void setHasBeenAttacked (bool value);

	int getDisabledTurns() const;
	bool isSentryActive() const;
	bool isManualFireActive() const;
	bool isAttacking() const;
	bool isBeeingAttacked() const;
	bool hasBeenAttacked() const;

	int getStoredResources() const;
	void setStoredResources(int value);

	//protected:
	virtual bool isUnitMoving() const { return false; } //test if the vehicle is moving right now. Having a waiting movejob doesn't count as moving
	virtual bool isAutoMoveJobActive() const { return false; }
	virtual bool isUnitWorking() const { return false; }
	virtual bool isUnitClearing() const { return false; }
	virtual bool isUnitLayingMines() const { return false; }
	virtual bool isUnitClearingMines() const { return false; }
	virtual bool isUnitBuildingABuilding() const { return false; }
	virtual bool factoryHasJustFinishedBuilding() const { return false; }
	virtual bool buildingCanBeStarted() const { return false; }
	virtual bool buildingCanBeUpgraded() const { return false; }
	virtual bool canBeStoppedViaUnitMenu() const = 0;

	int getCellSize() const;
	void setCellSize(int cs);

	virtual uint32_t getChecksum(uint32_t crc) const;

	// Important NOTE: This signal will be triggered when the destructor of the unit gets called.
	//                 This means when the signal is triggered it can not be guaranteed that all
	//                 of the objects attributes are still valid (especially the ones of derived classes).
	//                 Therefor you should not access the unit from a function that connects to this signal!
	mutable cSignal<void ()> destroyed;

	mutable cSignal<void ()> ownerChanged;

	mutable cSignal<void ()> positionChanged;

	mutable cSignal<void ()> renamed;
	mutable cSignal<void ()> statusChanged;

	mutable cSignal<void ()> disabledChanged;
	mutable cSignal<void ()> sentryChanged;
	mutable cSignal<void ()> manualFireChanged;
	mutable cSignal<void ()> attackingChanged;
	mutable cSignal<void ()> beeingAttackedChanged;
	mutable cSignal<void ()> beenAttackedChanged;
	mutable cSignal<void ()> movingChanged;

	mutable cSignal<void ()> storedUnitsChanged; //the unit has loaded or unloaded another unit
	mutable cSignal<void ()> stored;            //this unit has been loaded by another unit
	mutable cSignal<void ()> activated;         //this unit has been unloaded by another unit

	mutable cSignal<void ()> layingMinesChanged;
	mutable cSignal<void ()> clearingMinesChanged;
	mutable cSignal<void ()> buildingChanged;
	mutable cSignal<void ()> clearingChanged;
	mutable cSignal<void ()> workingChanged;
	mutable cSignal<void ()> storedResourcesChanged;
	mutable cSignal<void ()> unitSizeChanged;

	template<typename T>
	void serializeThis(T& archive)
	{
		archive & NVP(data);
		//archive & NVP(iID);  //const member. needs to be deserialized before calling constructor
		archive & NVP(dir);
		archive & NVP(storedUnits);
		archive & NVP(detectedByPlayerList);
		archive & NVP(owner);
		archive & NVP(position);
		archive & NVP(customName);
		archive & NVP(turnsDisabled);
		archive & NVP(sentryActive);
		archive & NVP(manualFireActive);
		archive & NVP(attacking);
		archive & NVP(beeingAttacked);
		archive & NVP(beenAttacked);
		archive & NVP(cellSize);
		archive & NVP(storageResCur);

		if (!archive.isWriter && data.getId() != sID(0, 0))
		{
			//restore pointer to static unit data
			archive.getPointerLoader()->get(data.getId(), staticData);
		}
		//TODO: detection?
	}

public: // TODO: make protected/private and make getters/setters
	const cStaticUnitData& getStaticUnitData() const;
	cDynamicUnitData data;		// basic data of the unit
	const unsigned int iID;		// the identification number of this unit
	int dir;					// ?Frame of the unit/current direction the unit is facing?

	std::vector<cVehicle*> storedUnits;		// list with the vehicles, that are stored in this unit

	std::vector<cPlayer*> seenByPlayerList; // a list of all players who can see this unit //TODO: remove
	std::vector<cPlayer*> detectedByPlayerList;		// detection state of stealth units. Use cPlayer::canSeeUnit() to check
													// if the unit is actually visible at the moment

	// little jobs, running on the vehicle.
	// e.g. rotating to a specific direction
	cJob* job;

	mutable int alphaEffectValue;

	//-----------------------------------------------------------------------------
protected:
	cStaticUnitDataPtr staticData;
	// Unit size in cells
	int cellSize;

	//-----------------------------------------------------------------------------
private:
	cPlayer* owner;
	cPosition position;

	std::string customName; //stores the name of the unit, when the player enters an own name for the unit. Otherwise the string is empty.

	int turnsDisabled;  ///< the number of turns this unit will be disabled, 0 if the unit is active
	bool sentryActive; ///< is the unit on sentry?
	bool manualFireActive; ///< if active, then the unit only fires by manual control and not as reaction fire
	bool attacking;  ///< is the unit currently attacking?
	bool beeingAttacked; ///< true when an attack on this unit is running
	bool beenAttacked; //the unit was attacked in this turn
	int storageResCur; //amount of stored ressources

};

template<typename T>
struct sUnitLess
{
	//static_assert(std::is_base_of<cUnit, T>::value, "Invalid template parameter. Has to be of a derived class of cUnit!");

	bool operator() (const std::shared_ptr<T>& left, const std::shared_ptr<T>& right) const
	{
		return left->iID < right->iID;
	}
	bool operator() (const std::shared_ptr<T>& left, const T& right) const
	{
		return left->iID < right.iID;
	}
	bool operator() (const T& left, const std::shared_ptr<T>& right) const
	{
		return left.iID < right->iID;
	}
	bool operator() (const T& left, const T& right) const
	{
		return left.iID < right.iID;
	}
	bool operator() (unsigned int left, const T& right) const
	{
		return left < right.iID;
	}
	bool operator() (const T& left, unsigned int right) const
	{
		return left.iID < right;
	}
	bool operator() (unsigned int left, const std::shared_ptr<T>& right) const
	{
		return left < right->iID;
	}
	bool operator() (const std::shared_ptr<T>& left, unsigned int right) const
	{
		return left->iID < right;
	}
};


// Generates array of coordinates belonging to internal border of an object with specified size
// @param corner - coordinate of left topmost corner of the object
// @param size - size of the object
std::vector<cPosition> generateBorder(const cPosition& corner, int size);

// Generates array of coordinates belonging to outer border of an object with specified size
// @param corner - coordinate of left topmost corner of the object
// @param size - size of the object
std::vector<cPosition> generateOuterBorder(const cPosition& corner, int size);

enum AdjSide
{
	AdjLeft = 1,
	AdjTop = 2,
	AdjRight = 4,
	AdjBottom = 8,
};

typedef std::pair<cPosition, int> cAdjPosition;

// Generates array of coordinates that are adjacent to specified object
// @param corner - coordinate of left topmost corner of the object
// @param size - size of the object
std::vector<cAdjPosition> generateAdjacentBorder(const cPosition& corner, int size);

#endif // game_data_units_unitH
