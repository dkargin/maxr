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

#ifndef game_data_units_vehicleH
#define game_data_units_vehicleH

#include <vector>
#include <array>

#include <SDL.h>

#include "defines.h"
#include "maxrconfig.h"
#include "game/data/units/unit.h"
#include "sound.h"

class cAutoMJob;
class cBuilding;
class cClientMoveJob;
class cMap;
class cStaticMap;
class cMapField;
class cPlayer;
class cServer;
class cClient;
class cServerMoveJob;
class cApplication;
class cSoundManager;
class cMoveJob;

//-----------------------------------------------------------------------------
// Enum for the symbols
//-----------------------------------------------------------------------------
#ifndef D_eSymbols
#define D_eSymbols

//-----------------------------------------------------------------------------
enum eSymbols
{
	SSpeed,
	SHits,
	SAmmo,
	SMetal,
	SEnergy,
	SShots,
	SOil,
	SGold,
	STrans,
	SHuman,
	SAir
};

//-----------------------------------------------------------------------------
enum eSymbolsBig
{
	SBSpeed,
	SBHits,
	SBAmmo,
	SBAttack,
	SBShots,
	SBRange,
	SBArmor,
	SBScan,
	SBMetal,
	SBOil,
	SBGold,
	SBEnergy,
	SBHuman
};

#endif

//-----------------------------------------------------------------------------
// Struct for the pictures and sounds
//-----------------------------------------------------------------------------
struct cVehicleData : public cStaticUnitData
{
	bool animationMovement = false;
	bool makeTracks = false;

	std::shared_ptr<cRenderable> build;
	std::shared_ptr<cRenderable> build_shadow;
	std::shared_ptr<cRenderable> clear;
	std::shared_ptr<cRenderable> clear_shadow;

	bool setGraphics(const std::string& layer, const std::shared_ptr<cRenderable>& sprite) override;

	void render(cRenderContext& context, const sRenderOps& ops) const;

	virtual UnitType getType() const
	{
		return UnitType::Vehicle;
	}

	AutoSurface storage;        // image of the vehicle in storage
	std::string FLCFile;        // FLC-Video

	cVehicleData();
};

typedef std::shared_ptr<cVehicleData> sVehicleDataPtr;

//-----------------------------------------------------------------------------
/** Class for a vehicle-unit of a player */
//-----------------------------------------------------------------------------
class cVehicle : public cUnit
{
	//-----------------------------------------------------------------------------
public:
	cVehicle (sVehicleDataPtr staticData,  const cDynamicUnitData& data, cPlayer* Owner, unsigned int ID);
	virtual ~cVehicle();

	virtual bool isAVehicle() const { return true; }
	virtual bool isABuilding() const { return false; }

	virtual const cPosition& getMovementOffset() const MAXR_OVERRIDE_FUNCTION { return tileMovementOffset; }
	void setMovementOffset (const cPosition& newOffset) { tileMovementOffset = newOffset; }

	sVehicleDataPtr getVehicleData() const;

	sVehicleDataPtr vehicleData;

	mutable int ditherX, ditherY;
	mutable int bigBetonAlpha;
	bool hasAutoMoveJob; // this is just a status information for the server, so that he can write the information to the saves
	cPosition bandPosition; // X,Y Position für das Band
	cPosition buildBigSavedPosition; // last position before building has started
	bool BuildPath;   // Gibt an, ob ein Pfad gebaut werden soll
	int DamageFXPointX, DamageFXPointY; // Die Punkte, an denen Rauch bei beschädigung aufsteigen wird
	unsigned int WalkFrame; // Frame der Geh-Annimation

	/**
	* refreshes speedCur and shotsCur and continues building or clearing
	*@author alzi alias DoctorDeath
	*@return true if there has been refreshed something else false.
	*/
	bool refreshData();
	void proceedBuilding (cModel& model);
	void continuePathBuilding(cModel& model);
	bool proceedClearing(cServer& server);

	virtual std::string getStatusStr (const cPlayer* player, const cUnitsData& unitsData) const MAXR_OVERRIDE_FUNCTION;
	void DecSpeed (int value);
	void doSurvey ();
	virtual void makeReport (cSoundManager& soundManager) const MAXR_OVERRIDE_FUNCTION;
	virtual bool canTransferTo (const cPosition& position, const cMapView& map) const MAXR_OVERRIDE_FUNCTION;
	virtual bool canTransferTo (const cUnit& position) const MAXR_OVERRIDE_FUNCTION;
	bool inSentryRange (cModel& model);
	virtual bool canExitTo (const cPosition& position, const cMap& map, const cStaticUnitData& unitData) const MAXR_OVERRIDE_FUNCTION;
	virtual bool canExitTo(const cPosition& position, const cMapView& map, const cStaticUnitData& unitData) const MAXR_OVERRIDE_FUNCTION;
	bool canLoad(const cPosition& position, const cMapView& map, bool checkPosition = true) const;
	virtual bool canLoad(const cVehicle* Vehicle, bool checkPosition = true) const MAXR_OVERRIDE_FUNCTION;
#define SUPPLY_TYPE_REARM 0
#define SUPPLY_TYPE_REPAIR 1
	/// supplyType: one of SUPPLY_TYPE_REARM and SUPPLY_TYPE_REPAIR
	bool canSupply (const cMapView& map, const cPosition& position, int supplyType) const;
	/// supplyType: one of SUPPLY_TYPE_REARM and SUPPLY_TYPE_REPAIR
	bool canSupply (const cUnit* unit, int supplyType) const;
	void calcTurboBuild (std::array<int, 3>& turboBuildTurns, std::array<int, 3>& turboBuildCosts, int buildCosts) const;
	/**
	* lays a mine at the current position of the unit.
	*/
	void layMine (cModel& model);
	/**
	* clear a mine at the current position of the unit.
	*/
	void clearMine (cModel& model);
	/**
	* checks whether the commando action can be performed or not
	*@author alzi alias DoctorDeath
	*/
	bool canDoCommandoAction (const cPosition& position, const cMapView& map, bool steal) const;
	bool canDoCommandoAction (const cUnit* unit, bool steal) const;
	/**
	* calculates the chance for disabling or stealing the target unit
	*@author alzi alias DoctorDeath
	*/
	int calcCommandoChance (const cUnit* destUnit, bool steal) const;
	int calcCommandoTurns (const cUnit* destUnit) const;
	/**
	* returns whether this player has detected this unit or not
	*@author alzi alias DoctorDeath
	*@param iPlayerNum number of player for which the status should be checked
	*@return true if the player has detected the unit
	*/
	bool isDetectedByPlayer (const cPlayer* player) const;
	/**
	* removes a player from the detectedByPlayerList
	*/
	void resetDetectedByPlayer (cServer& server, cPlayer* player);
	/**
	* adds a player to the DetecedByPlayerList
	*/
	virtual void setDetectedByPlayer (cPlayer* player, bool addToDetectedInThisTurnList = true) MAXR_OVERRIDE_FUNCTION;
	/**
	* - detects stealth units in the scan range of the vehicle
	* - checks whether the vehicle has been detected by an other unit
	* the detection maps have to be up to date,
	* when calling this function,
	* this function has to be called on the server
	* every time a unit was moved, builded, unloaded...
	*/
	void makeDetection (cServer& server);

	/** After a movement the detection state of a unit might be reset,
	 * if it was not detected in _this_ turn. */
	void tryResetOfDetectionStateAfterMove (cServer& server);
	/** Resets the list of players, that detected this unit in this turn
	 * (is called at turn end). */
	void clearDetectedInThisTurnPlayerList();
	bool wasDetectedInThisTurnByPlayer (const cPlayer* player) const;

	static void blitWithPreScale (SDL_Surface* org_src, SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dest, SDL_Rect* destrect, float factor, int frames = 1);

	void executeAutoMoveJobCommand (cClient& client);

	/**
	* Is this a plane and is there a landing platform beneath it,
	* that can be used to land on?
	* @author: eiko
	*/
	bool canLand (const cMap& map) const;


	/**
	* draws the main image of the vehicle onto the passed surface
	*/
	void render (const cMapView* map, unsigned long long animationTime, const cPlayer* activePlayer, SDL_Surface* surface, const SDL_Rect& dest, const cStaticUnitData::sRenderOps& ops) const;

	bool isUnitLoaded() const { return loaded; }

	virtual bool isUnitMoving() const { return moving; } //test if the vehicle is moving right now. Having a waiting movejob doesn't count a moving
	virtual bool isAutoMoveJobActive() const { return autoMoveJob != nullptr; }
	virtual bool isUnitClearing() const { return isClearing; }
	virtual bool isUnitLayingMines() const { return layMines; }
	virtual bool isUnitClearingMines() const { return clearMines; }
	virtual bool isUnitBuildingABuilding() const { return isBuilding; }
	virtual bool canBeStoppedViaUnitMenu() const;

	void setMoving (bool value);
	void setLoaded (bool value);
	void setClearing (bool value);
	void setBuildingABuilding (bool value);
	void setLayMines (bool value);
	void setClearMines (bool value);

	int getClearingTurns() const;
	void setClearingTurns (int value);

	float getCommandoRank() const;
	void setCommandoRank (float value);

	const sID& getBuildingType() const;
	void setBuildingType (const sID& id);
	int getBuildCosts() const;
	void setBuildCosts (int value);
	int getBuildTurns() const;
	void setBuildTurns (int value);
	int getBuildCostsStart() const;
	void setBuildCostsStart (int value);
	int getBuildTurnsStart() const;
	void setBuildTurnsStart (int value);

	int getFlightHeight() const;
	void setFlightHeight (int value);

	cMoveJob* getMoveJob();
	const cMoveJob* getMoveJob() const;
	void setMoveJob (cMoveJob* moveJob);

	cAutoMJob* getAutoMoveJob();
	const cAutoMJob* getAutoMoveJob() const;
	void startAutoMoveJob (cClient& client);
	void stopAutoMoveJob();

	/**
	* return the unit which contains this vehicle
	*/
	cBuilding* getContainerBuilding();
	cVehicle* getContainerVehicle();

	virtual uint32_t getChecksum(uint32_t crc) const;

	mutable cSignal<void ()> clearingTurnsChanged;
	mutable cSignal<void ()> buildingTurnsChanged;
	mutable cSignal<void ()> buildingCostsChanged;
	mutable cSignal<void ()> buildingTypeChanged;
	mutable cSignal<void ()> commandoRankChanged;
	mutable cSignal<void ()> flightHeightChanged;

	mutable cSignal<void ()> moveJobChanged;
	mutable cSignal<void ()> autoMoveJobChanged;
	mutable cSignal<void ()> moveJobBlocked;

	template <typename T>
	void serialize(T& archive)
	{
		cUnit::serializeThis (archive); //serialize cUnit members

		archive & NVP(hasAutoMoveJob);
		archive & NVP(bandPosition);
		archive & NVP(buildBigSavedPosition);
		archive & NVP(BuildPath);
		archive & NVP(WalkFrame);

		archive & NVP(detectedInThisTurnByPlayerList);
		archive & NVP(tileMovementOffset);
		archive & NVP(loaded);
		archive & NVP(moving);
		archive & NVP(isBuilding);
		archive & NVP(buildingTyp);
		archive & NVP(buildCosts);
		archive & NVP(buildTurns);
		archive & NVP(buildTurnsStart);
		archive & NVP(buildCostsStart);
		archive & NVP(isClearing);
		archive & NVP(clearingTurns);
		archive & NVP(layMines);
		archive & NVP(clearMines);
		archive & NVP(flightHeight);
		archive & NVP(commandoRank);

		if (!archive.isWriter)
		{
			vehicleData = std::dynamic_pointer_cast<cVehicleData>(staticData);
		}
	}
private:

	void render_BuildingOrBigClearing (const cMapView& map, unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, bool drawShadow) const;
	void render_smallClearing (unsigned long long animationTime, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor, bool drawShadow) const;
	//void render_shadow (const cMapView& map, SDL_Surface* surface, const SDL_Rect& dest, float zoomFactor) const;

	//---- sentry and reaction fire helpers ------------------------------------
	/**
	 * Is called after a unit moved one field;
	 * it allows opponent units to react to that movement and
	 * fire on the moving vehicle, if they can.
	 * An opponent unit only fires as reaction to the movement,
	 * if the moving unit is an "offense" for that opponent
	 * (i.e. it could attack a unit/building of the opponent).
	 * @author: pagra
	 */
	bool provokeReactionFire (cModel& model);
	bool doesPlayerWantToFireOnThisVehicleAsReactionFire (const cModel& model, const cPlayer* player) const;
	bool makeAttackOnThis (cModel& model, cUnit* opponentUnit, const std::string& reasonForLog) const;
	bool makeSentryAttack (cModel& model, cUnit* unit) const;
	bool isOtherUnitOffendedByThis (const cModel& model, const cUnit& otherUnit) const;
	bool doReactionFire (cModel& model, cPlayer* player) const;
	bool doReactionFireForUnit (cModel& model, cUnit* opponentUnit) const;

	/// helper method that returns a list of all players,
	/// that can detect this unit
	std::vector<cPlayer*> calcDetectedByPlayer (cServer& server) const;
	/// list of players, that detected this vehicle in this turn
	std::vector<cPlayer*> detectedInThisTurnByPlayerList;

	cPosition tileMovementOffset;  // offset within tile during movement

	bool moving;
	cMoveJob* moveJob;

	std::shared_ptr<cAutoMJob> autoMoveJob; //the auto move AI of the vehicle

	bool loaded;

	bool isBuilding;
	sID buildingTyp;
	int buildCosts;
	int buildTurns;
	int buildTurnsStart;
	int buildCostsStart;

	bool isClearing;
	int clearingTurns;

	bool layMines;
	bool clearMines;

	int flightHeight;

	float commandoRank;
};

#endif // game_data_units_vehicleH
