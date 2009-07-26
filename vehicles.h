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
#ifndef vehiclesH
#define vehiclesH
#include "defines.h"
#include <SDL.h>
#include "main.h"
#include "sound.h"
#include "automjobs.h"
#include "input.h"

class cPlayer;
class cAutoMJob;
class cMap;
class cServerMoveJob;
class cClientMoveJob;
class cEndMoveAction;

//-----------------------------------------------------------------------------
// Enum for the symbols
//-----------------------------------------------------------------------------
#ifndef D_eSymbols
#define D_eSymbols

//-----------------------------------------------------------------------------
enum eSymbols {
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
enum eSymbolsBig {
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
struct sVehicle{
  SDL_Surface *img[8],*img_org[8]; // 8 Surfaces des Vehicles
  SDL_Surface *shw[8],*shw_org[8]; // 8 Surfaces des Schattens
  SDL_Surface *build,*build_org;         // Surfaces beim Bauen
  SDL_Surface *build_shw,*build_shw_org; // Surfaces beim Bauen (Schatten)
  SDL_Surface *clear_small,*clear_small_org;         // Surfaces beim Clearen (die groﬂe wird in build geladen)
  SDL_Surface *clear_small_shw,*clear_small_shw_org; // Surfaces beim Clearen (Schatten) (die groﬂe wird in build geladen)
  SDL_Surface *overlay,*overlay_org;     // Overlays
  SDL_Surface *storage; // Bild des Vehicles im Lager
  char *FLCFile;       // FLC-Video
  sUnitData data;   // Grunddaten des Vehicles
  char id[4];          // ID dieses Elements
  int nr;              // Nr dieses Elements
  SDL_Surface *info;   // Infobild

  // Die Sounds:
  struct Mix_Chunk *Wait;
  struct Mix_Chunk *WaitWater;
  struct Mix_Chunk *Start;
  struct Mix_Chunk *StartWater;
  struct Mix_Chunk *Stop;
  struct Mix_Chunk *StopWater;
  struct Mix_Chunk *Drive;
  struct Mix_Chunk *DriveWater;
  struct Mix_Chunk *Attack;

  void scaleSurfaces( float factor );
};


//-----------------------------------------------------------------------------
/** Class for a vehicle-unit of a player */
//-----------------------------------------------------------------------------
class cVehicle
{
//-----------------------------------------------------------------------------
public:
	cVehicle(sVehicle *v,cPlayer *Owner);
	~cVehicle();

	/** the identification number of this unit */
	unsigned int iID;
	/** a list were the numbers of all players who can see this vehicle are stored in */
	cList<cPlayer*> SeenByPlayerList;
	/** a list were the numbers of all players who have deteced this vehicle are stored in */
	cList<cPlayer*> DetectedByPlayerList;
	int PosX,PosY;   // Position auf der Karte
	int OffX,OffY;   // Offset w�hrend der Bewegung
	sVehicle *typ;   // Typ des Vehicles
	int dir;         // aktuelle Drehrichtung
	bool selected;   // Gibt an, ob das Fahrzeug ausgew�hlt ist
	bool groupSelected;
	string name; // Name des Vehicles
	cPlayer *owner;  // Eigent�mer des Vehicles
	cServerMoveJob *ServerMoveJob;
	cClientMoveJob *ClientMoveJob;
	cAutoMJob *autoMJob; //the auto move AI of the vehicle
	bool hasAutoMoveJob; // this is just a status information for the server, so that he can write the information to the saves
	bool moving;     // Gibt an, ob sich das Vehicle grade bewegt
	bool MoveJobActive; // Gibt an, ob der MoveJob gerade ausgef�hrt wird
	bool MenuActive; // Gibt an, ob das Men� aktiv ist
	bool AttackMode; // Gibt an, ob der Attack-Modus aktiv ist
	bool Attacking;  // Gibt an, ob das Fahrzeug gerade angreift
	bool bIsBeeingAttacked; /** true when an attack on this vehicle is running */
	int ditherX,ditherY; // Dithering f�r Flugzeuge
	bool IsBuilding;  // Gibt an ob was gebaut wird
	sID BuildingTyp;  // Gibt an, was gebaut wird
	int BuildCosts;   // Die verbleibenden Baukosten
	int BuildRounds;  // Die verbleibenden Baurunden
	int BuildRoundsStart; // Startwert der Baurunden (f�rs Pfadbauen)
	int BuildCostsStart;  // Startwert der Baukosten (f�rs Pfadbauen)
	bool PlaceBand;   // Gibt an, ob grad ein Band platziert wird
	int BandX,BandY;  // X,Y Position f�r das Band
	int BuildBigSavedPos; // Letzte Position vor dem Baubeginn
	bool BuildPath;   // Gibt an, ob ein Pfad gebaut werden soll
	bool IsClearing;  // Gibt an, ob einn Feld ger�umt wird
	int ClearingRounds; // Gibt an, wie lange ein Feld noch ger�umt wird
	unsigned int BigBetonAlpha; // AlphaWert des gro�en Betons
	bool bSentryStatus;		/** true if the vehicle is on sentry */
	bool Transfer;    // Gibt an, ob gerade ein Transfer statfinden soll
	int StartUp;      // Z�hler f�r die Startupannimation
	int FlightHigh;   // Die Flugh�he des Flugzeugs
	bool LoadActive; // Gibt an, ob ein Vehicle geladen werden soll
	cList<cVehicle*> StoredVehicles; // Liste mit geladenen Vehicles
	int VehicleToActivate; // Nummer des Vehicles, dass aktiviert werden soll
	bool ActivatingVehicle; // Gibt an, ob ein Vehicle aktiviert werden soll
	bool MuniActive;   // Gibt an, ob grad Munition aufgeladen werden soll
	bool RepairActive; // Gibt an, ob grad repariert werden soll
	bool LayMines;    // Gibt an, ob Minen gelegt werden sollen
	bool ClearMines;  // Gibt an, ob Minen ger�umt werden sollen
	bool Loaded;      // Gibt an, ob das Vehicle geladen wurde
	int DamageFXPointX,DamageFXPointY; // Die Punkte, an denen Rauch bei besch�digung aufsteigen wird
	int WalkFrame;    // Frame der Geh-Annimation
	float CommandoRank; // Rang des Commandos
	bool StealActive,DisableActive; // Legt fest, ob gestohlen, oder sabotiert werden soll
	int Disabled;     // Gibt an, f�r wie lange diese Einheit disabled ist
	bool IsLocked;    // Gibt an, ob dieses Vehicle in irgend einer Log-Liste ist
	cList<cEndMoveAction*> passiveEndMoveActions;

	cVehicle *next,*prev; // Verkettungselemente
	sUnitData data;    // Daten des Vehicles

	/**
	* Draws the vehicle to the screen buffer.
	* Takes the main image from the cache or calls cVehicle::render()
	*/
	void draw(SDL_Rect screenPosition );
	void Select();
	void Deselct();
	void ShowDetails();
	void GenerateName();
	/**
	* refreshes speedCur and shotsCur and continues building or clearing
	*@author alzi alias DoctorDeath
	*@return 1 if there has been refreshed something else 0.
	*/
	int refreshData();
	void DrawSymbol(eSymbols sym,int x,int y,int maxx,int value,int maxvalue,SDL_Surface *sf);
	void DrawNumber(int x,int y,int value,int maxvalue,SDL_Surface *sf);
	void DrawSymbolBig(eSymbolsBig sym,int x,int y,int maxx,int value,int orgvalue,SDL_Surface *sf);
	int GetScreenPosX() const;
	int GetScreenPosY() const;
	void DrawPath();
	void RotateTo(int Dir);
	std::string getStatusStr();
	int playStream();
	void StartMoveSound();
	void DrawMenu( sMouseState *mouseState = NULL );
	int GetMenuPointAnz();
	SDL_Rect GetMenuSize();
	bool MouseOverMenu(int mx,int my);
	void DecSpeed(int value);
	void DrawMunBar() const;
	void drawHealthBar() const;
	void drawStatus() const;
	void Center();
	/*
	* checks if the unit can attack the offset
	* when override is false, the funktion only returns true, when there is an enemy unit
	* ATTENTION: must not be called with override == false from the server thread!
	*/
	bool CanAttackObject(int off, cMap *Map, bool override=false, bool checkRange = true);
	bool IsInRange(int off, cMap *Map);
	void DrawAttackCursor( int offset );
	int CalcHelth(int damage);
	void FindNextband();
	void doSurvey();
	void MakeReport();
	bool CanTransferTo( class cMapField *OverUnitField );
	bool InSentryRange();
	void DrawExitPoints(sVehicle*) const;
	bool canExitTo ( const int x, const int y, const cMap* map, const sVehicle *typ ) const;
	bool canLoad( int off, cMap *Map, bool checkPosition = true );
	bool canLoad( cVehicle *Vehicle, bool checkPosition = true );
	void storeVehicle( cVehicle *Vehicle, cMap *Map );
	void exitVehicleTo( cVehicle *Vehicle, int offset, cMap *Map );
#define SUPPLY_TYPE_REARM	0
#define SUPPLY_TYPE_REPAIR	1
	bool canSupply( int iOff, int iType );
	bool canSupply( cVehicle *Vehicle, int iType );
	bool canSupply( cBuilding *Building, int iType );
	/** Upgrades the unit data of this vehicle to the current, upgraded version of the player. */
	void upgradeToCurrentVersion();
	void calcTurboBuild(int* const iTurboBuildRounds, int* const iTurboBuildCosts, int iBuild_Costs );
	/**
	* lays a mine. Should only be called by the server!
	*@author alzi alias DoctorDeath
	*@return true if a mine has been layed
	*/
	bool layMine();
	/**
	* clears a field from a mine. Should only be called by the server!
	*@author alzi alias DoctorDeath
	*@return true if there was a mine to be cleared
	*/
	bool clearMine();
	/**
	* checks whether the commando action can be performed or not
	*@author alzi alias DoctorDeath
	*/
	bool canDoCommandoAction( int x, int y, cMap *map, bool steal );
	/**
	* draws the commando curser for stealing or disabling with the calculated chance
	*@author alzi alias DoctorDeath
	*/
	void drawCommandoCursor( int off, bool steal );
	/**
	* calculates the chance for disabling or stealing the target unit
	*@author alzi alias DoctorDeath
	*/
	int calcCommandoChance( cVehicle *destVehicle, cBuilding *destBuilding, bool steal );
	int calcCommandoTurns( cVehicle *destVehicle, cBuilding *destBuilding );
	void DeleteStored();
	/**
	* returns whether this player has detected this unit or not
	*@author alzi alias DoctorDeath
	*@param iPlayerNum number of player for which the stauts sould be checked
	*@return true if the player has detected the unit
	*/
	bool isDetectedByPlayer( const cPlayer* player );
	/**
	* adds a player to the DetecedByPlayerList
	*/
	void resetDetectedByPlayer( cPlayer* player );
	/**
	* removes a player from the detectedByPlayerList
	*/
	void setDetectedByPlayer( cPlayer* player );
	/**
	* - detects stealth units in the scan range of the vehicle
	* - checks whether the vehicle has been detected by an other unit
	* the detection maps have to be up to date, when calling this funktion
	* this function has to be called on the server everytime a unit was moved, builded, unloaded...
	*/
	void makeDetection();
	/**
	* checks whether the offset is next to the vehicle
	*/
	bool isNextTo( int x, int y) const;
	void blitWithPreScale ( SDL_Surface *org_src, SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dest, SDL_Rect *destrect, float factor, int frames = 1 );
private:
	/**
	* draws the main image of the vehicle onto the passed surface
	*/
	void render( SDL_Surface* surface, const SDL_Rect& dest );
};

#endif
