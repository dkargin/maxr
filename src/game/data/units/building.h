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

#ifndef game_data_units_buildingH
#define game_data_units_buildingH

#include <vector>
#include <array>

#include <SDL.h>

#include "defines.h"
#include "maxrconfig.h"
#include "game/data/units/unit.h"
#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"
#include "game/logic/upgradecalculator.h" // cResearch::ResearchArea

class cBase;
class cPlayer;
class cVehicle;
class cMap;
class cMapField;
class cServer;
class cClient;
class cSubBase;
class cCrossPlattformRandom;

//--------------------------------------------------------------------------
/** struct for the images and sounds */
//--------------------------------------------------------------------------
struct cBuildingData : public cStaticUnitData
{
	bool hasBetonUnderground;

	AutoSurface video;  // video

	// Surfaces of the effects
	std::shared_ptr<cRenderable> effect;

	bool setGraphics(const std::string& layer, const std::shared_ptr<cRenderable>& sprite) override;

	virtual UnitType getType() const
	{
		return UnitType::Building;
	}

	virtual void render(cRenderContext& context, const sRenderOps& ops) const override;
};

typedef std::shared_ptr<cBuildingData> sBuildingDataPtr;

// enum for the upgrade symbols
#ifndef D_eSymbols
#define D_eSymbols
enum eSymbols {SSpeed, SHits, SAmmo, SMetal, SEnergy, SShots, SOil, SGold, STrans, SHuman, SAir};
enum eSymbolsBig {SBSpeed, SBHits, SBAmmo, SBAttack, SBShots, SBRange, SBArmor, SBScan, SBMetal, SBOil, SBGold, SBEnergy, SBHuman};
#endif

//--------------------------------------------------------------------------
/** struct for the building order list */
//--------------------------------------------------------------------------
class cBuildListItem
{
public:
	cBuildListItem();
	cBuildListItem (sID type, int remainingMetal);
	cBuildListItem (const cBuildListItem& other);
	cBuildListItem (cBuildListItem&& other);

	cBuildListItem& operator= (const cBuildListItem& other);
	cBuildListItem& operator= (cBuildListItem && other);

	const sID& getType() const;
	void setType (const sID& type);

	int getRemainingMetal() const;
	void setRemainingMetal (int value);

	uint32_t getChecksum(uint32_t crc) const;

	cSignal<void ()> typeChanged;
	cSignal<void ()> remainingMetalChanged;

	template<typename T>
	void serialize(T& archive)
	{
		archive & NVP(type);
		archive & NVP(remainingMetal);
	}
private:
	sID type;
	int remainingMetal;
};

//--------------------------------------------------------------------------
enum ResourceKind
{
	TYPE_METAL = 0,
	TYPE_OIL   = 1,
	TYPE_GOLD  = 2
};

//--------------------------------------------------------------------------
/** Class cBuilding for one building. */
//--------------------------------------------------------------------------
class cBuilding : public cUnit
{
public:
	friend class cDebugOutputWidget;

	cBuilding(sBuildingDataPtr data, const cDynamicUnitData* ddata, cPlayer* Owner, unsigned int ID);
	virtual ~cBuilding();

	virtual bool isAVehicle() const { return false; }
	virtual bool isABuilding() const { return true; }
	bool isRubble() const { return rubbleValue > 0; }

	sBuildingDataPtr buildingData;
	mutable int effectAlpha; // alpha value for the effect

	cSubBase* subBase;     // the subbase to which this building belongs
	int metalProd, oilProd, goldProd;          // production settings (from mine allocation menu)

	int DamageFXPointX, DamageFXPointY, DamageFXPointX2, DamageFXPointY2; // the points, where smoke will be generated when the building is damaged
	/** true if the building was has been working before it was disabled */
	bool wasWorking;
	int points;     // accumulated eco-sphere points

	int playStream();
	virtual std::string getStatusStr (const cPlayer* player, const cUnitsData& unitsData) const MAXR_OVERRIDE_FUNCTION;

	virtual void makeReport (cSoundManager& soundManager) const MAXR_OVERRIDE_FUNCTION;

	/**
	* refreshes the shotsCur of this building
	*@author alzi alias DoctorDeath
	*@return 1 if there has been refreshed something, else 0.
	*/
	bool refreshData();
	void DrawSymbolBig (eSymbolsBig sym, int x, int y, int maxx, int value, int orgvalue, SDL_Surface* sf);
	void updateNeighbours (const cMap& map);
	void CheckNeighbours (const cMap& Map);

	void startWork ();
	void stopWork (bool forced = false);

	/** check whether a transfer to a unit on the field is possible */
	virtual bool canTransferTo (const cPosition& position, const cMapView& map) const MAXR_OVERRIDE_FUNCTION;
	virtual bool canTransferTo (const cUnit& unit) const MAXR_OVERRIDE_FUNCTION;
	void initMineRessourceProd (const cMap& map);
	void calcTurboBuild (std::array<int, 3>& turboBuildRounds, std::array<int, 3>& turboBuildCosts, int vehicleCosts, int remainingMetal = -1) const;
	virtual bool canExitTo (const cPosition& position, const cMap& map, const cStaticUnitData& unitData) const MAXR_OVERRIDE_FUNCTION;
	virtual bool canExitTo (const cPosition& position, const cMapView& map, const cStaticUnitData& vehicleData) const MAXR_OVERRIDE_FUNCTION;
	bool canLoad(const cPosition& position, const cMapView& map, bool checkPosition = true) const;
	virtual bool canLoad (const cVehicle* Vehicle, bool checkPosition = true) const MAXR_OVERRIDE_FUNCTION;

	/**
	* returns whether this player has detected this unit or not
	*@author alzi alias DoctorDeath
	*@param player player for which the status should be checked
	*@return true if the player has detected the unit
	*/
	bool isDetectedByPlayer (const cPlayer* player) const;
	/**
	* removes a player from the detectedByPlayerList
	*/
	void resetDetectedByPlayer (const cPlayer* player);
	/**
	* adds a player to the DetecedByPlayerList
	*/
	virtual void setDetectedByPlayer (cPlayer* player, bool addToDetectedInThisTurnList = true) MAXR_OVERRIDE_FUNCTION;
	/**
	* - checks whether the building has been detected by an other unit
	* the detection maps have to be up to date, when calling this function
	* this function has to be called on the server
	* every time a building is added
	*/
	void makeDetection (cServer& server);

	/**
	* draws the main image of the building onto the given surface
	*/
	void render (unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, const cStaticUnitData::sRenderOps& ops) const;

	//void render_simple(SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, unsigned long long animationTime = 0, int alpha = 254) const;
	//static void render_simple (SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, const cBuildingData& uiData, const cPlayer* owner, int frameNr = 0, int alpha = 254);

	void executeUpdateBuildingCommmand (const cClient& client, bool updateAllOfSameType) const;

	virtual bool isUnitWorking() const { return isWorking; }
	virtual bool factoryHasJustFinishedBuilding() const;
	virtual bool buildingCanBeStarted() const;
	virtual bool buildingCanBeUpgraded() const;
	virtual bool canBeStoppedViaUnitMenu() const { return isUnitWorking(); }

	bool isBuildListEmpty() const;
	size_t getBuildListSize() const;
	const cBuildListItem& getBuildListItem (size_t index) const;
	cBuildListItem& getBuildListItem (size_t index);
	void setBuildList (std::vector<cBuildListItem> buildList);
	void addBuildListItem (cBuildListItem item);
	void removeBuildListItem (size_t index);

	int getBuildSpeed() const;
	int getMetalPerRound() const;
	bool getRepeatBuild() const;

	void setWorking (bool value);
	void setBuildSpeed(int value);
	void setMetalPerRound(int value);
	void setRepeatBuild(bool value);

	int getMaxProd(eResourceType type) const;

	void setResearchArea (cResearch::ResearchArea area);
	cResearch::ResearchArea getResearchArea() const;

	void setRubbleValue(int value, cCrossPlattformRandom& randomGenerator);
	int getRubbleValue() const;

	virtual uint32_t getChecksum(uint32_t crc) const;

	cSignal<void ()> buildListChanged;
	cSignal<void ()> buildListFirstItemDataChanged;
	cSignal<void ()> researchAreaChanged;

	cSignal<void()> buildSpeedChanged;
	cSignal<void()> metalPerRoundChanged;
	cSignal<void()> repeatBuildChanged;

	template <typename T>
	void serialize(T& archive)
	{
		cUnit::serializeThis(archive); //serialize cUnit members

		archive & NVP(rubbleTyp);
		archive & NVP(rubbleValue);
		archive & NVP(maxMetalProd);
		archive & NVP(maxOilProd);
		archive & NVP(maxGoldProd);
		archive & NVP(metalProd);
		archive & NVP(oilProd);
		archive & NVP(goldProd);
		archive & NVP(buildSpeed);
		archive & NVP(metalPerRound);
		archive & NVP(repeatBuild);
		archive & NVP(wasWorking);
		archive & NVP(points);
		archive & NVP(isWorking);
		archive & NVP(researchArea);
		archive & NVP(buildList);

		if (!archive.isWriter)
		{
			/*
			if (isRubble())
			{
				if (cellSize > 1)
				{
					uiData = UnitsUiData.rubbleBig;
					staticData = archive.getPointerLoader()->getBigRubbleData();
				}
				else
				{
					uiData = UnitsUiData.rubbleSmall;
					staticData = archive.getPointerLoader()->getSmallRubbleData();
				}
			}
			else
			{
			}*/

			buildingData = std::dynamic_pointer_cast<cBuildingData>(staticData);
			registerOwnerEvents();
			connectFirstBuildListItem();
		}
	}

private:
	cSignalConnectionManager buildListFirstItemSignalConnectionManager;
	cSignalConnectionManager ownerSignalConnectionManager;

	/**
	* draws the connectors onto the given surface
	*/
	void drawConnectors(cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const;

	void render_rubble(cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const;
	void connectFirstBuildListItem();

	bool isWorking;  // is the building currently working?

	int buildSpeed;
	int metalPerRound;
	bool repeatBuild;

	int maxMetalProd, maxOilProd, maxGoldProd; // the maximum possible production of the building (ressources under the building)

	int rubbleTyp;     // type of the rubble graphic (when unit is rubble)
	int rubbleValue;   // number of resources in the rubble field

	// Picked connector sprite indexes for the building
	std::vector<std::pair<cPosition, int>> connectorTiles;

	cResearch::ResearchArea researchArea; ///< if the building can research, this is the area the building last researched or is researching

	std::vector<cBuildListItem> buildList; // list with the units to be build by this factory

	void registerOwnerEvents();
};

#endif // game_data_units_buildingH
