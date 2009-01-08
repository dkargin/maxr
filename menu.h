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
#ifndef menuH
#define menuH
#include "defines.h"
#include "main.h"
#include <SDL.h>
#include "map.h"
#include "buildings.h"
#include "player.h"
#include "network.h"
#include "savegame.h"

// Globales //////////////////////////////////////////////////////////////////
EX string SaveLoadFile;	// Name of the savegame to load or to save
EX int SaveLoadNumber;	// Index number of the savegame to load or to save

struct sColor
{
	unsigned char cBlue, cGreen, cRed;
};

// Strukturen ////////////////////////////////////////////////////////////////
struct sSaveFile
{
	string filename;
	string gamename;
	string type;
	string time;
	int number;
};

struct sPlayer{
  int what[4];
  string clan[4];
};

// Strukturen ////////////////////////////////////////////////////////////////
struct sPlayerHS{
  int what[8];
  int iColor[8];
  string name[8];
  string clan[8];
};

enum ePlayer
{
	PLAYER_N,
	PLAYER_H,
	PLAYER_AI
};

enum eLandingState
{
	LANDING_STATE_UNKNOWN,		//initial state
	LANDING_POSITION_OK,		//there are no other players near the position
	LANDING_POSITION_WARNING,	//there are players within the warning distance
	LANDING_POSITION_TOO_CLOSE,	//the position is too close to another player
	LANDING_POSITION_CONFIRMED	//warnings about nearby players will be ignored, because the player has confirmed his position
};

// Struktur f�r die Optionen:
struct sOptions{
  int metal,oil,gold,dichte;
  int credits;
  bool FixedBridgeHead;
  bool AlienTech;
  bool PlayRounds;
};

// Struktur f�r die Upgrades:
struct sHUp{
  SDL_Surface *sf;
  bool vehicle;
  sID UnitID;
  int costs;
  sUpgrades upgrades[8];
};

// Struktur f�r die Landung:
struct sLanding{
  SDL_Surface *sf;
  sID UnitID;
  int costs;
  int cargo;
};

/**
* struct for handling the client landing positions
*/
struct sClientLandData
{
	sClientLandData();
	int iLandX, iLandY;
	eLandingState landingState;
	int iLastLandX, iLastLandY;
};


enum MESSAGE_TYPES
{
	MU_MSG_CHAT = FIRST_MENU_MESSAGE,	// simple text message
	MU_MSG_NEW_PLAYER,			// a new player has connected
	MU_MSG_REQ_IDENTIFIKATION,	// host requests a identifacation of this player
	MU_MSG_IDENTIFIKATION,		// player send his idenetification
	MU_MSG_DEL_PLAYER,			// a player should be deleted
	MU_MSG_PLAYERLIST,			// a list with all players and their data
	MU_MSG_OPTINS,				// all options selected by the host
	MU_MSG_GO,					// host wants to start the game
	MU_MSG_LANDING_VEHICLES,	// the list of purcased vehicles
	MU_MSG_RESOURCES,			// the resources on the map
	MU_MSG_UPGRADES,			// data of upgraded units
	GAME_EV_REQ_IDENT,			// a server of a running game requests an identification
	GAME_EV_OK_RECONNECT,		// a server gives his ok to the reconnect
	//messages for the landing selectiong menu:
	MU_MSG_LANDING_COORDS,		// the selected landing coords of a client
	MU_MSG_RESELECT_LANDING,	// informs a client that the player has to reselect the landing site
	MU_MSG_ALL_LANDED,			// all players have selcted there landing points and clients can start game
};

class cMultiPlayerMenu
{
public:

	cMultiPlayerMenu(bool bHost);
	~cMultiPlayerMenu();

	void runNetworkMenu();
	void sendMessage( cNetMessage *Message, int iPlayer = -1 );
	void HandleMessages();

private:
	SDL_Surface *sfTmp;
	bool bRefresh;
	bool bOptions;
	bool bStartSelecting;
	string sIP;
	string savegameString;
	string sMap;
	int iFocus;
	int iPort;
	cMap *Map;
	sOptions Options;
	bool bExit;

	cList<cPlayer*> PlayerList;
	bool *ReadyList;
	int iNextPlayerNr;
	cPlayer *ActualPlayer;
	cList<string> ChatLog;
	sClientLandData* clientLandingCoordsList;  /**the landing coords of all clients*/
	cList<sLanding>* clientLandingVehicleList; /**the list of purcased vehicles of all clients*/
	cSavegame *Savegame;

	void addChatLog( string sMsg );
	void showChatLog();
	void displayGameSettings();
	void displayPlayerList();
	
	void sendIdentification();
	void sendPlayerList( cList<cPlayer*> *SendPlayerList = NULL );
	void sendOptions();
	void sendLandingVehicles( const cList<sLanding>& c );
	void sendUpgrades();

	int testAllReady();

	void runNewGame ();
	int runSavedGame ();

public:
	cList<cNetMessage*> MessageList;
	cList<cNetMessage*> landingSelectionMessageList;
	bool bHost;
} EX *MultiPlayerMenu;

class cSelectLandingMenu
{
public:
	cSelectLandingMenu( cMap* map, sClientLandData* clientLandData, int iClients, int iLocalClient = 0 );
	void run();
private:
	int iLandedClients;
	int iClients;
	int iLocalClient;
	bool bAllLanded;
	cMap* map;
	sClientLandData c;
	sClientLandData* clientLandData;

	void drawHud();
	void drawMap();
	void handleMessages();
	void selectLandingSite();
	/**
	* runs a statemachine, that controlls the landing state of the given player
	* @return the new landing state of the player
	*/
	eLandingState checkLandingState(int playerNr );

	void sendLandingCoords( sClientLandData& c );
};

// Prototypen ////////////////////////////////////////////////////////////////
void RunMainMenu(void);
/**
 *
 * @param
 */
void ExitMenu(void);
/**
 *
 * @param
 */
void RunSPMenu(void);
/**
 *
 * @param
 * @return
 */
string RunPlanetSelect(void);
/**
 *
 * @param init
 * @return
 */
sOptions RunOptionsMenu(sOptions *init);

/**
 * @author beko
 * @param
 * @return
 */
sPlayerHS runPlayerSelectionHotSeat(void);

/**
 *
 * @param
 * @return
 */
sPlayer runPlayerSelection(void);
/**
 *
 * @param str
 * @param x
 * @param y
 * @param checked
 * @param surface Source Surface for proper background drawing
 * @param center
 */
void placeSelectableText(std::string sText,int x,int y,bool checked, SDL_Surface *surface, bool center=true);
/**
 *
 * @param player
 * @param LandingList
 */
void RunHangar(cPlayer *player, cList<sLanding> *LandingList);
/**
 *
 * @param x
 * @param y
 * @return
 */
int GetKachelBig(int x,int y);
/**
 *
 * @param
 */
void RunMPMenu(void);
/**
 *
 * @param sf
 * @return
 */
int GetColorNr(SDL_Surface *sf);
/**
 *
 * @param
 */
void HeatTheSeat(void);

/**
 *
 * @param players
 */
void showPlayerStatesHotSeat(sPlayerHS players);

/**
 *
 * @param players
 */
void ShowPlayerStates(sPlayer players);
/**
 *
 * @param list
 * @param selected
 * @param offset
 * @param surface Source Surface for proper background drawing
 */
void ShowLandingList(cList<sLanding> *list,int selected,int offset, SDL_Surface *surface);
/**
 *
 * @param bSave Should you can load savegames in this menu?
 * @return
 */
int ShowDateiMenu( bool bSave );
void loadFiles ( cList<string> *filesList, cList<sSaveFile*> &savesList, int offset );
void displayFiles ( cList<sSaveFile*> &savesList, int offset, int selected, bool bSave, bool bCursor, bool bFirstSelect, SDL_Rect rDialog );
/**
 *
 * @param sFileName
 * @param sTime
 * @param sSavegameName
 * @param sMode
 */
void loadMenudatasFromSave ( string sFileName, string *sTime, string *sSavegameName, string *sMode );

#endif
