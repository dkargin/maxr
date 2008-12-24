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
#ifndef buildingsH
#define buildingsH
#include "defines.h"
#include "sound.h"
#include "main.h"
#include <SDL.h>
#include "base.h"
#include "dialog.h"
#include "map.h"

// Define zum Updaten:
#define UpdateBuilding(from,to) if((from).hit_points==(from).max_hit_points){(from).hit_points=(to).max_hit_points;}(from).version=(to).version;(from).max_hit_points=(to).max_hit_points;(from).armor=(to).armor;(from).scan=(to).scan;(from).range=(to).range;(from).max_shots=(to).max_shots;(from).damage=(to).damage;(from).max_ammo=(to).max_ammo;(from).iBuilt_Costs=(to).iBuilt_Costs;

// Struktur f�r Upgrades /////////////////////////////////////////////////////
struct sUpgrades{
  bool active;
  int NextPrice;
  int Purchased;
  int *value;
  int StartValue;
  string name;
};

// Struktur f�r die Bilder und Sounds:
struct sBuilding{
  SDL_Surface *img,*img_org; // Surface des Buildings
  SDL_Surface *shw,*shw_org; // Surfaces des Schattens
  SDL_Surface *eff,*eff_org; // Surfaces des Effektes
  SDL_Surface *video;  // Video
  sUnitData data;  // Grunddaten des Buildings
  char id[4];          // ID dieses Elements
  int nr;              // Nr dieses Elements
  SDL_Surface *info;   // Infobild
  char *text;          // Infotext

  // Die Sounds:
  struct Mix_Chunk *Start;
  struct Mix_Chunk *Running;
  struct Mix_Chunk *Stop;
  struct Mix_Chunk *Attack;
};

class cPlayer;
class cBase;
struct sUpgradeStruct;

// Enum f�r die Symbole
#ifndef D_eSymbols
#define D_eSymbols
enum eSymbols {SSpeed,SHits,SAmmo,SMetal,SEnergy,SShots,SOil,SGold,STrans,SHuman,SAir};
enum eSymbolsBig {SBSpeed,SBHits,SBAmmo,SBAttack,SBShots,SBRange,SBArmor,SBScan,SBMetal,SBOil,SBGold,SBEnergy,SBHuman};
#endif

struct sBuildStruct
{
public:
	sBuildStruct(SDL_Surface* const sf_, int const id_, int const iRemainingMetal_ = -1) :
		sf(sf_),
		id(id_),
		iRemainingMetal(iRemainingMetal_)
	{}

	SDL_Surface *sf;
	int id;
	int iRemainingMetal;
};

// Struktur f�r die Bauliste:
struct sBuildList{
  struct sVehicle *typ;
  int metall_remaining;
};

enum ResourceKind
{
	TYPE_METAL = 0,
	TYPE_OIL   = 1,
	TYPE_GOLD  = 2
};

struct sMineValues
{
	int& GetProd(ResourceKind);

	int GetMaxProd(ResourceKind) const;

	int iMetalProd, iOilProd, iGoldProd;
	int iMaxMetalProd, iMaxOilProd, iMaxGoldProd;

	int iBuildingID;
};

// Die Buulding Klasse ///////////////////////////////////////////////////////
class cBuilding{
public:
	cBuilding(sBuilding *b,cPlayer *Owner,cBase *Base);
	~cBuilding(void);

	/** the identification number of this unit */
	unsigned int iID;
	/** a list were the numbers of all players who can see this building are stored in */
	cList<cPlayer*> SeenByPlayerList;
	/** a list were the numbers of all players who have deteced this vehicle are stored in */
	cList<cPlayer*> DetectedByPlayerList;
	int PosX,PosY;   // Position auf der Karte
	sBuilding *typ;  // Typ des Buildings
	bool selected;   // Gibt an, ob das Building ausgew�hlt ist
	string name; // Name des Buildings
	cPlayer *owner;  // Eigent�mer des Buildings
	sUnitData data;    // Daten des Buildings
	cBuilding *next,*prev; // Zeiger f�r die Verkettung
	bool MenuActive; // Gibt an, ob das Men� grad aktiv ist
	bool AttackMode; // Gibt an, ob der AttackMode grad aktiv ist
	bool bIsBeeingAttacked; /** true when an attack on this building is running */
	int RubbleTyp;     // Typ des Drecks
	int RubbleValue;   // Wert des Drecks
	int StartUp;     // Z�hler f�r die Startupannimation
	cBase *base;     // Die Basis des Geb�udes
	bool BaseN,BaseE,BaseS,BaseW; // Gibt an, ob das Geb�ude in einer Richting verbunden ist
	bool BaseBN,BaseBE,BaseBS,BaseBW; // Gibt an, ob das Geb�ude in einer Richting verbunden ist (zus�tzlich f�r gro�e Geb�ude)
	struct sSubBase *SubBase;     // Die SubBase dieses Geb�udes
	int EffectAlpha; // Alphawert f�r den Effekt
	bool EffectInc;  // Gibt an, ob der Effect rauf, oder runter gez�hlt wird
	bool IsWorking;  // Gibt an, ob das Geb�ude grade arbeitet
	bool bSentryStatus;		/** true if the building is on sentry */
	bool Transfer;   // Gibt an, ob ein Transfer statfinden soll
	int MetalProd,OilProd,GoldProd; // Produktion des geb�udes
	int MaxMetalProd,MaxOilProd,MaxGoldProd; // Maximal m�gliche Produktion
	int dir;         // Frame des Geb�udes
	bool Attacking;  // Gibt an, ob das Building gerade angreift
	cList<sBuildList*> *BuildList; // Die Bauliste der Fabrik
	int BuildSpeed;  // Die baugeschwindigkeit der Fabrik
	int MetalPerRound; //Die Menge an Metal, die die Fabrik bei momentaner Baugeschwindigkeit pro Runde maximal verbaut
	bool RepeatBuild; // Gibt an, ob der Bau wiederholt werden soll
	bool LoadActive; // Gibt an, ob ein Vehicle geladen werden soll
	cList<cVehicle*> StoredVehicles; // Liste mit geladenen Vehicles
	int VehicleToActivate; // Nummer des Vehicles, dass aktiviert werden soll
	bool ActivatingVehicle; // Gibt an, ob ein Vehicle aktiviert werden soll
	int DamageFXPointX,DamageFXPointY,DamageFXPointX2,DamageFXPointY2; // Die Punkte, an denen Rauch bei besch�digung aufsteigen wird
	int Disabled;    // Gibt an, f�r wie lange diese Einheit disabled ist
	bool IsLocked;   // Gibt an, ob dieses Building in irgend einer Log-Liste ist

	void Draw(SDL_Rect *dest);
	void Select(void);
	void Deselct(void);
	void ShowDetails(void);
	void GenerateName(void);
	int PlayStram(void);
	std::string GetStatusStr(void);
	void DrawSymbol(eSymbols sym,int x,int y,int maxx,int value,int maxvalue,SDL_Surface *sf);
	void DrawNumber(int x,int y,int value,int maxvalue,SDL_Surface *sf);
	/**
	* refreshes the shots of this building
	*@author alzi alias DoctorDeath
	*@return 1 if there has been refreshed something else 0.
	*/
	int refreshData();
	void ShowHelp(void);
	void DrawSymbolBig(eSymbolsBig sym,int x,int y,int maxx,int value,int orgvalue,SDL_Surface *sf);
	void Center(void);
	void DrawMunBar(void) const;
	void DrawHelthBar(void) const;
	void drawStatus() const;
	int GetScreenPosX(void) const;
	int GetScreenPosY(void) const;
	int CalcHelth(int damage);
	void DrawMenu(void);
	int GetMenuPointAnz(void);
	SDL_Rect GetMenuSize(void);
	bool MouseOverMenu(int mx,int my);
	void SelfDestructionMenu(void);
	void ShowBigDetails(void);
	void updateNeighbours( cMap *map );
	void CheckNeighbours( cMap *map );
	void DrawConnectors(SDL_Rect dest);
	void ServerStartWork();
	void ClientStartWork();
	void ServerStopWork(bool override);
	void ClientStopWork();
	bool CanTransferTo(struct sGameObjects *go);
	void ShowTransfer(sGameObjects *target);
	void DrawTransBar(int len);
	void MakeTransBar(int *trans,int MaxTarget,int Target);
	void CheckRessourceProd(void);
	void showMineManager();
	void doMineInc(ResourceKind, cList<sMineValues*>& Mines);
	void doMineDec(ResourceKind, cList<sMineValues*>& Mines);
	void calcMineFree ( cList<sMineValues*> *Mines, int *iFreeM, int *iFreeO, int *iFreeG );
	void MakeMineBars(int iTempSBMetalProd, int iTempSBOilProd, int iTempSBGoldProd, int MaxM,int MaxO,int MaxG,int *FreeM,int *FreeO,int *FreeG);
	void DrawMineBar(int typ,int value,int max_value,int offy,bool number,int fixed);
	bool IsInRange(int off);
	/*
	* checks if the unit can attack the offset
	* when override is false, the funktion only returns true, when there is an enemy unit
	* ATTENTION: must not be called with override == false from the server thread!
	*/
	bool CanAttackObject(int off,bool override=false);
	void DrawAttackCursor( int offset );
	void RotateTo(int Dir);
	void ShowBuildMenu(void);
	void ShowBuildList(cList<sBuildStruct*>& list, int selected, int offset, bool showInfo);
	void DrawBuildButtons(int speed);
	void ShowToBuildList(cList<sBuildStruct*>& list, int selected, int offset, bool showInfo);
	void CalcTurboBuild(int *iTurboBuildRounds, int *iTurboBuildCosts, int iVehicleCosts, int iRemainingMetal = -1);
	void DrawExitPoints(sVehicle *typ);
	bool canExitTo ( const int x, const int y, const cMap* map, const sVehicle *typ ) const;
	bool canLoad( int offset, cMap *Map );
	bool canLoad( cVehicle *Vehicle );
	void storeVehicle( cVehicle *Vehicle, cMap *Map );
	void ShowStorage(void);
	void DrawStored(int off);
	void ShowStorageMetalBar(void);
	void exitVehicleTo( cVehicle *Vehicle, int offset, cMap *Map );
	void MakeStorageButtonsAlle(bool *AlleAufladenEnabled,bool *AlleReparierenEnabled,bool *AlleUpgradenEnabled);
	void ShowResearch(void);
	void ShowResearchSchieber(void);
	void MakeResearchSchieber(int x,int y);
	void ShowUpgrade(void);
	void ShowUpgradeList(cList<sUpgradeStruct*>& list, int selected, int offset, bool beschreibung);
	void ShowGoldBar(int StartCredits);
	void MakeUpgradeSliderVehicle(sUpgrades *u,int nr);
	void MakeUpgradeSliderBuilding(sUpgrades *u,int nr);
	void CreateUpgradeList(cList<sUpgradeStruct*>& selection, cList<sUpgradeStruct*>& images, int* selected, int* offset);
	void MakeUpgradeSubButtons(void);
	void SendUpdateStored(int index);
	/**
	* returns whether this player has detected this unit or not
	*@author alzi alias DoctorDeath
	*@param player player for which the stauts sould be checked
	*@return true if the player has detected the unit
	*/
	bool isDetectedByPlayer( cPlayer* player );
	/**
	* adds a player to the DetecedByPlayerList
	*/
	void resetDetectedByPlayer( cPlayer* player );
	/**
	* removes a player from the detectedByPlayerList
	*/
	void setDetectedByPlayer( cPlayer* player );
	/**
	* - checks whether the building has been detected by an other unit
	* the detection maps have to be up to date, when calling this funktion
	* this function has to be called on the server everytime a building is added
	*/
	void makeDetection();
	/**
	* checks whether the coordinates are next to the building
	*/
	bool isNextTo( int x, int y) const;
};

#endif
