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
#ifndef serverH
#define serverH
#include "defines.h"
#include "main.h"
#include "map.h"
#include "player.h"

/**
* Callback funktion for the serverthread
*@author alzi alias DoctorDeath
*/
int CallbackRunServerThread( void *arg );

/**
* Server class which takes all calculations for the game and has the data of all players
*@author alzi alias DoctorDeath
*/
class cServer
{
	/** a list with all events for the server */
	cList<SDL_Event*> *EventQueue;
	/** the thread the server runs in */
	SDL_Thread *ServerThread;
	/** mutex for the eventqueue */
	SDL_mutex *QueueMutex;
	/** true if the server should exit and end his thread */
	bool bExit;



	/** the map */
	cMap *Map;
	/** current movejob */
	cMJobs *mjobs;
	/** List with all active movejobs */
	cList<cMJobs*> *ActiveMJobs;
	/** List with all attackjobs */
	cList<cAJobs*> *AJobs;
	/** List with all players */
	cList<cPlayer*> *PlayerList;
	/** true if this is a hotseat game */
	bool bHotSeat;
	/** number of active player in hotseat */
	int iHotSeatPlayer;
	/** true if the game should be played in turns */
	bool bPlayTurns;
	/** number of active player in multiplayer game */
	int iActiveTurnPlayerNr;
	/** name of the savegame to load or to save */
	string sSaveLoadFile;
	/** ndex number of the savegame to load or to save */
	int iSaveLoadNumber;

	/**
	* gets the next event of the eventqueue and stores it to 'event'. If the queue is empty 'event' will be set to NULL.
	*@author alzi alias DoctorDeath
	*@return 1 for success, 0 if the eventqueue is empty
	*/
	int pollEvent( SDL_Event *event );
	/**
	* processes everything that is need for this event.
	*@author alzi alias DoctorDeath
	*@param event The SDL_Event to be handled.
	*@return 0 for success
	*/
	int HandleEvent( SDL_Event *event );

	/**
	* checks whether the field is free for landing
	*@author alzi alias DoctorDeath
	*@param iX The X coordinate to check.
	*@param iY The Y coordinate to check.
	*@return true if it is free, else false.
	*/
	bool freeForLanding ( int iX, int iY );
	/**
	* lands the vehicle at a free position in the radius
	*@author alzi alias DoctorDeath
	*@param iX The X coordinate to land.
	*@param iY The Y coordinate to land.
	*@param iWidth Width of the field.
	*@param iHeight iHeight of the field.
	*@param Vehicle Vehicle to land.
	*@param Player Player whose vehicle should be land.
	*@return NULL if the vehicle could not be landed, else a pointer to the vehicle.
	*/
	cVehicle *landVehicle ( int iX, int iY, int iWidth, int iHeight, sVehicle *Vehicle, cPlayer *Player );
	/**
	* adds the unit to the map and player.
	*@author alzi alias DoctorDeath
	*@param iPosX The X were the unit should be added.
	*@param iPosY The Y were the unit should be added.
	*@param Vehicle Vehicle which should be added.
	*@param Building Building which should be added.
	*@param Player Player whose vehicle should be added.
	*@param bInit true if this is a initialisation call.
	*/
	void addUnit( int iPosX, int iPosY, sVehicle *Vehicle, cPlayer *Player, bool bInit = false );
	void addUnit( int iPosX, int iPosY, sBuilding *Building, cPlayer *Player, bool bInit = false );
	/**
	* deletes the building
	*@author alzi alias DoctorDeath
	*@param Building Building which should be deleted.
	*/
	void deleteBuilding ( cBuilding *Building );

public:
	/**
	* initialises the server class
	*@author alzi alias DoctorDeath
	*/
	void init( cMap *map, cList<cPlayer*> *PlayerList );
	/**
	* kills the server class
	*@author alzi alias DoctorDeath
	*/
	void kill();
	/**
	* pushes an event to the eventqueue of the server. This is threadsafe.
	*@author alzi alias DoctorDeath
	*@param event The SDL_Event to be pushed.
	*@return 0 for success
	*/
	int pushEvent( SDL_Event *event );
	/**
	* sends the event to the client on which the player with 'iPlayerNum' is playing
	*@author alzi alias DoctorDeath
	*@param event The SDL_Event to be send.
	*@param iLenght Lenght of data in the event to be send. This should not be longer then PACKAGE_LENGHT -2!
	*@param iPlayerNum Number of player who should receive this event.
	*/
	void sendEvent( SDL_Event *event, int iLenght, int iPlayerNum );

	/**
	* runs the server. Should only be called by the ServerThread!
	*@author alzi alias DoctorDeath
	*/
	void run();

	/**
	* lands all units at the given position
	*@author alzi alias DoctorDeath
	*@param iX The X coordinate to land.
	*@param iY The Y coordinate to land.
	*@param Player The Player who wants to land.
	*@param List List with all units to land.
	*@param bFixed true if the bridgehead is fixed.
	*/
	void makeLanding( int iX, int iY, cPlayer *Player, cList<sLanding*> *List, bool bFixed );
} EX *Server;

#endif serverH
