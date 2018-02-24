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

#ifndef game_data_units_unitdataH
#define game_data_units_unitdataH

#include <string>
#include <utility>
#include <vector>

#include "game/data/resourcetype.h"

#include "utility/signal/signal.h"
#include "utility/serialization/serialization.h"
#include "utility/drawing.h"
#include "sound.h"

class cClanData;

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

class cBuildingData;
class cVehicleData;

// Database for all the units
class cUnitsData
{
public:
    typedef sID UID;
    typedef std::map<UID, cDynamicUnitData> DynamicUnitDataStorage;

	cUnitsData();

    // TODO: Should get rid of this
    cBuildingData* rubbleBig;
    cBuildingData* rubbleSmall;

    // direct pointer on some of the building graphics
    SDL_Surface* ptr_small_beton;
    SDL_Surface* ptr_small_beton_org;
    SDL_Surface* ptr_connector;
    SDL_Surface* ptr_connector_org;
    SDL_Surface* ptr_connector_shw;
    SDL_Surface* ptr_connector_shw_org;


	void initializeIDData();
	void initializeClanUnitData(const cClanData& clanData);

    // Generate or obtain dynamic unit data for specified id
    cDynamicUnitData& getDynamicUnitData(const std::string& id);
    // Generate or obtain static unit data for specified id
    cStaticUnitDataPtr getUnit(const std::string& id);

    // Get vehicle data for specified id
    // Can return empty pointer if no object is found
    std::shared_ptr<cVehicleData> getVehicle(const UID& id) const;

    // Get vehicle data for specified id
    // Can return empty pointer if no object is found
    std::shared_ptr<cBuildingData> getBuilding(const UID& id) const;

    // Get vehicle data for specified id
    // Can return empty pointer if no object is found
    cStaticUnitDataPtr getUnit(const UID& id) const;

    bool isValidId(const UID& id) const;

	size_t getNrOfClans() const;

	// clan = -1: without clans
	const cDynamicUnitData& getDynamicUnitData(const sID& id, int clan = -1) const;
	// clan = -1: without clans
    const DynamicUnitDataStorage& getDynamicUnitsData(int clan = -1) const;

    std::vector<cStaticUnitDataPtr> getStaticUnitsData() const;

    std::vector<cStaticUnitDataPtr> getUnitsData(UnitType type) const;

	uint32_t getChecksum(uint32_t crc) const;

    template <typename T> void serialize(T& archive);

    friend class cUnitsDataMeta;
private:
	sID constructorID;
	sID engineerID;
	sID surveyorID;
	sID specialIDLandMine;
	sID specialIDSeaMine;
	sID specialIDMine;
	sID specialIDSmallGen;
	sID specialIDConnector;
	sID specialIDSmallBeton;

	// the dynamic unit data. Contains the modified versions for the clans
    // 'Clan' should be replaced in this context on 'Player'
    // 'Clan' stuff is relevant during embarcation stage and calculation of final unit stats.
    // After player has embarked - no need to keep 'clan' stuff
    std::vector<DynamicUnitDataStorage> clanDynamicUnitData;

    //cStaticUnitData rubbleSmall;
    //cStaticUnitData rubbleBig;

    // the static unit data
    std::map<UID, cStaticUnitDataPtr> staticUnitData;
    // the dynamic unit data. Standard version without clan modifications
    DynamicUnitDataStorage dynamicUnitData;

	// unitdata does not change during the game. 
	// So caching the checksum saves a lot cpu ressources.
	mutable uint32_t crcCache;
	mutable bool crcValid;
};

// This is mega-ugly, but alows to hide serialization from the header
class cTextArchiveIn;
class cBinaryArchiveIn;
class cBinaryArchiveOut;
class cXmlArchiveOut;
class cXmlArchiveIn;

class cUnitsDataMeta
{
public:
    static void serialize(cTextArchiveIn& archive, cUnitsData &ud);
    static void serialize(cBinaryArchiveIn& archive, cUnitsData &ud);
    static void serialize(cBinaryArchiveOut& archive, cUnitsData &ud);
    static void serialize(cXmlArchiveOut& archive, cUnitsData &ud);
    static void serialize(cXmlArchiveIn& archive, cUnitsData &ud);

    template<class T> static void serialize_impl(T& archive, cUnitsData& ud);
};

template<class T> void cUnitsData::serialize(T& archive)
{
    cUnitsDataMeta::serialize(archive, *this);
}

#endif // game_data_units_unitdataH
