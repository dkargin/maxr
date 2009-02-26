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

#include "movejobs.h"
#include "player.h"
#include "serverevents.h"
#include "clientevents.h"
#include "server.h"
#include "client.h"
#include "vehicles.h"
#include "math.h"

cPathCalculator::cPathCalculator( int ScrX, int ScrY, int DestX, int DestY, cMap *Map, cVehicle *Vehicle )
{
	this->DestX = DestX;
	this->DestY = DestY;
	this->ScrX = ScrX;
	this->ScrY = ScrY;
	this->Map = Map;
	this->Vehicle = Vehicle;
	bPlane = Vehicle->data.can_drive == DRIVE_AIR;
	bShip = Vehicle->data.can_drive == DRIVE_SEA;

	Waypoints = NULL;
	MemBlocks = NULL;
	nodesHeap = NULL;
	openList = NULL;
	closedList = NULL;

	blocknum = 0;
	blocksize = 0;
	heapCount = 0;
	calcPath ();
}

cPathCalculator::~cPathCalculator()
{
	if ( MemBlocks != NULL )
	{
		for ( int i = 0; i < blocknum; i++ )
		{
			delete MemBlocks[i];
		}
		free ( MemBlocks );
	}
	if ( nodesHeap != NULL ) delete [] nodesHeap;
	if ( openList != NULL ) delete [] openList;
	if ( closedList != NULL ) delete [] closedList;
}

void cPathCalculator::calcPath ()
{
	// generate open and closed list
	nodesHeap = new sPathNode*[Map->size*Map->size+1];
	openList = new sPathNode*[Map->size*Map->size+1];
	closedList = new sPathNode*[Map->size*Map->size+1];
	fill <sPathNode**, sPathNode*> ( nodesHeap, &nodesHeap[Map->size*Map->size+1], NULL );
	fill <sPathNode**, sPathNode*> ( openList, &openList[Map->size*Map->size+1], NULL );
	fill <sPathNode**, sPathNode*> ( closedList, &closedList[Map->size*Map->size+1], NULL );

	// generate startnode
	sPathNode *StartNode = allocNode ();
	StartNode->x = ScrX;
	StartNode->y = ScrY;
	StartNode->costG = 0;
	StartNode->costH = heuristicCost ( ScrX, ScrY );
	StartNode->costF = StartNode->costG+StartNode->costH;

	StartNode->prev = NULL;
	openList[ScrX+ScrY*Map->size] = StartNode;
	insertToHeap ( StartNode, false );

	while ( heapCount > 0 )
	{
		// get the node with the lowest F value
		sPathNode *CurrentNode = nodesHeap[1];

		// move it from the open to the closed list
		openList[CurrentNode->x+CurrentNode->y*Map->size] = NULL;
		closedList[CurrentNode->x+CurrentNode->y*Map->size] = CurrentNode;
		deleteFirstFromHeap();

		// generate waypoints when destination has been reached
		if ( CurrentNode->x == DestX && CurrentNode->y == DestY )
		{
			sWaypoint *NextWaypoint;
			Waypoints = new sWaypoint;

			sPathNode *NextNode = CurrentNode;
			NextNode->next = NULL;
			do
			{
				NextNode->prev->next = NextNode;
				NextNode = NextNode->prev;
			}
			while ( NextNode->prev != NULL );


			NextWaypoint = Waypoints;
			NextWaypoint->X = NextNode->x;
			NextWaypoint->Y = NextNode->y;
			NextWaypoint->Costs = 0;
			do
			{
				NextNode = NextNode->next;

				NextWaypoint->next = new sWaypoint;
				NextWaypoint->next->X = NextNode->x;
				NextWaypoint->next->Y = NextNode->y;
				NextWaypoint->next->Costs = calcNextCost ( NextNode->prev->x, NextNode->prev->y, NextWaypoint->next->X, NextWaypoint->next->Y );
				NextWaypoint = NextWaypoint->next;
			}
			while ( NextNode->next != NULL );

			NextWaypoint->next = NULL;

			return;
		}

		// expand node
		expandNodes ( CurrentNode );
	}

	// there is no path to the destination field
	Waypoints = NULL;
}

void cPathCalculator::expandNodes ( sPathNode *ParentNode )
{
	// add all nearby nodes
	for ( int y = ParentNode->y-1; y <= ParentNode->y+1; y++ )
	{
		if ( y < 0 || y >= Map->size ) continue;

		for ( int x = ParentNode->x-1; x <= ParentNode->x+1; x++ )
		{
			if ( x < 0 || x >= Map->size ) continue;
			if ( x == ParentNode->x && y == ParentNode->y ) continue;

			if ( !Map->possiblePlaceVehicle( Vehicle->data, x, y, Vehicle->owner) ) continue;
			if ( closedList[x+y*Map->size] != NULL ) continue;

			if ( openList[x+y*Map->size] == NULL )
			{
				// generate new node
				sPathNode *NewNode = allocNode ();
				NewNode->x = x;
				NewNode->y = y;
				NewNode->costG = calcNextCost( ParentNode->x, ParentNode->y, x, y ) + ParentNode->costG;
				NewNode->costH = heuristicCost ( x, y );
				NewNode->costF = NewNode->costG+NewNode->costH;
				NewNode->prev = ParentNode;
				openList[x+y*Map->size] = NewNode;
				insertToHeap ( NewNode, false );
			}
			else
			{
				// modify existing node
				int costG, costH, costF;
				costG = calcNextCost( ParentNode->x, ParentNode->y, x,y ) + ParentNode->costG;
				costH = heuristicCost ( x, y );
				costF = costG+costH;
				if ( costF < openList[x+y*Map->size]->costF )
				{
					openList[x+y*Map->size]->costG = costG;
					openList[x+y*Map->size]->costH = costH;
					openList[x+y*Map->size]->costF = costF;
					openList[x+y*Map->size]->prev = ParentNode;
					insertToHeap ( openList[x+y*Map->size], true );
				}
			}
		}
	}
}

sPathNode *cPathCalculator::allocNode()
{
	// alloced new memory block if necessary
	if ( blocksize <= 0 )
	{
		MemBlocks = (sPathNode**) realloc ( MemBlocks, (blocknum+1)*sizeof(sPathNode*) );
		MemBlocks[blocknum] = new sPathNode[10];
		blocksize = MEM_BLOCK_SIZE;
		blocknum++;
	}
	blocksize--;
	return &MemBlocks[blocknum-1][blocksize];
}

void cPathCalculator::insertToHeap( sPathNode *Node, bool exists )
{
	int i;
	if ( exists )
	{
		// get the number of the existing node
		for ( int j = 1; j <= heapCount; j++ )
		{
			if ( nodesHeap[j] == Node )
			{
				i = j;
				break;
			}
		}
	}
	else
	{
		// add the new node in the end
		heapCount++;
		nodesHeap[heapCount] = Node;
		i = heapCount;
	}
	// resort the nodes
	while ( i > 1 )
	{
		if ( Node->costF < nodesHeap[i/2]->costF )
		{
			sPathNode *TempNode = nodesHeap[i/2];
			nodesHeap[i/2] = Node;
			nodesHeap[i] = TempNode;
			i = i/2;
		}
		else break;
	}
}

void cPathCalculator::deleteFirstFromHeap()
{
	// overwrite the first node by the last one
	nodesHeap[1] = nodesHeap[heapCount];
	nodesHeap[heapCount] = NULL;
	heapCount--;
	int v = 1, u;
	while ( true )
	{
		u = v;
		if ( 2*u+1 <= heapCount )	// both children in the heap exists
		{
			if ( nodesHeap[u]->costF >= nodesHeap[u*2]->costF ) v = 2*u;
			if ( nodesHeap[v]->costF >= nodesHeap[u*2+1]->costF ) v = 2*u+1;
		}
		else if ( 2*u <= heapCount )	// only one children exists
		{
			if ( nodesHeap[u]->costF >= nodesHeap[u*2]->costF ) v = 2*u;
		}
		// do the resort
		if ( u != v )
		{
			sPathNode *TempNode = nodesHeap[u];
			nodesHeap[u] = nodesHeap[v];
			nodesHeap[v] = TempNode;
		}
		else break;
	}
}

int cPathCalculator::heuristicCost ( int srcX, int srcY )
{
	// calculate the minimal costs with the theorem of pythagoras
	int deltaX = DestX - srcX;
	int deltaY = DestY - srcY;

	return Round ( sqrt ( (double)deltaX*deltaX + deltaY*deltaY ) );
}

int cPathCalculator::calcNextCost( int srcX, int srcY, int destX, int destY )
{
	int costs, offset;
	// costs of planes and ships can't be modified
	if ( bPlane || bShip )
	{
		if ( srcX != destX && srcY != destY ) return (int)(4*1.5);
		else return 4;
	}
	costs = 4;
	offset = destX+destY*Map->size;
	cBuilding* building = Map->fields[offset].getBaseBuilding();
	// moving on water will cost more
	if ( Map->IsWater ( offset ) && !building ) costs = 12;
	// moving on a road is cheaper
	else if ( building && building->data.is_road ) costs = 2;
	// for surveyers moving can't be more expensive then 4
	if ( Vehicle->data.can_survey && costs > 4 ) costs = 4;

	// mutliplicate with the factor 1.5 for diagonal movements
	if ( srcX != destX && srcY != destY ) costs = (int)(costs*1.5);
	return costs;
}

cServerMoveJob::cServerMoveJob ( int iSrcOff, int iDestOff, bool bPlane, cVehicle *Vehicle )
{
	if ( !Server ) return;

	Map = Server->Map;
	this->Vehicle = Vehicle;
	ScrX = iSrcOff%Map->size;
	ScrY = iSrcOff/Map->size;
	DestX = iDestOff%Map->size;
	DestY = iDestOff/Map->size;
	this->bPlane = bPlane;
	bShip = Vehicle->data.can_drive == DRIVE_SEA;
	bFinished = false;
	bEndForNow = false;
	iSavedSpeed = 0;
	Waypoints = NULL;
	iReservedOff = -1;

	// unset sentry status when moving vehicle
	if ( Vehicle->bSentryStatus )
	{
		Vehicle->owner->deleteSentryVehicle ( Vehicle );
		Vehicle->bSentryStatus = false;
	}
	sendUnitData ( Vehicle, Vehicle->owner->Nr );
	for ( unsigned int i = 0; i < Vehicle->SeenByPlayerList.Size(); i++ )
	{
		sendUnitData ( Vehicle, Vehicle->SeenByPlayerList[i]->Nr );
	}

	if ( Vehicle->ServerMoveJob )
	{
		iSavedSpeed = Vehicle->ServerMoveJob->iSavedSpeed;
		Vehicle->ServerMoveJob->release();
		Vehicle->moving = false;
		Vehicle->MoveJobActive = false;
	}
	Vehicle->ServerMoveJob = this;
}

cServerMoveJob::~cServerMoveJob()
{
	if ( iReservedOff >= 0 && iReservedOff <= Map->size*Map->size )
	{
		if ( !bPlane && Map->fields[iReservedOff].reserved ) Map->fields[iReservedOff].reserved = false;
		if ( bPlane && Map->fields[iReservedOff].air_reserved ) Map->fields[iReservedOff].air_reserved = false;
	}
	sWaypoint *NextWaypoint;
	while ( Waypoints )
	{
		NextWaypoint = Waypoints->next;
		delete Waypoints;
		Waypoints = NextWaypoint;
	}
	Waypoints = NULL;
}

bool cServerMoveJob::generateFromMessage ( cNetMessage *message )
{
	if ( message->iType != GAME_EV_MOVE_JOB_CLIENT ) return false;
	int iCount = 0;
	int iWaypointOff;
	int iReceivedCount = message->popInt16();

	Log.write(" Server: Received MoveJob: VehicleID: " + iToStr( Vehicle->iID ) + ", SrcX: " + iToStr( ScrX ) + ", SrcY: " + iToStr( ScrY ) + ", DestX: " + iToStr( DestX ) + ", DestY: " + iToStr( DestY ) + ", WaypointCount: " + iToStr( iReceivedCount ), cLog::eLOG_TYPE_NET_DEBUG);
	
	// Add the waypoints
	sWaypoint *Waypoint = new sWaypoint;
	Waypoints = Waypoint;
	while ( iCount < iReceivedCount )
	{
		iWaypointOff = message->popInt32();
		Waypoint->X = iWaypointOff%Map->size;
		Waypoint->Y = iWaypointOff/Map->size;
		Waypoint->Costs = message->popInt16();
		Waypoint->next = NULL;

		iCount++;

		if ( iCount < iReceivedCount )
		{
			Waypoint->next = new sWaypoint;
			Waypoint = Waypoint->next;
		}
	}
	calcNextDir ();

	//check whether the vehicle has to be hided
	//we check here the next waypoint of the vehicle, so other payers will not see in which direction the vehicle was driving
	if ( Waypoints && Waypoints->next && Vehicle->data.speed && Server->Map->possiblePlace(Vehicle, Waypoints->next->X, Waypoints->next->Y) )
	{
		int offset = Waypoints->next->X + Waypoints->next->Y * Server->Map->size;
		for ( unsigned int i = 0; i < Vehicle->DetectedByPlayerList.Size(); i++ )
		{
			cPlayer* player = Vehicle->DetectedByPlayerList[i];
			if ( Vehicle->data.is_stealth_land && ( !player->DetectLandMap[offset] && !Server->Map->IsWater(offset) ))
			{
				Vehicle->resetDetectedByPlayer( player );
				i--;
			}
			else if ( Vehicle->data.is_stealth_sea && ( !player->DetectSeaMap[offset] && Server->Map->IsWater(offset) ))
			{
				Vehicle->resetDetectedByPlayer( player );
				i--;
			}
		}

		Server->checkPlayerUnits();
	}

	return true;
}

bool cServerMoveJob::calcPath()
{
	if ( ScrX == DestX && ScrY == DestY ) return false;

	cPathCalculator PathCalculator( ScrX, ScrY, DestX, DestY, Map, Vehicle );
	Waypoints = PathCalculator.Waypoints;
	if ( Waypoints )
	{
		calcNextDir();
		return true;
	}
	return false;
}

void cServerMoveJob::release()
{
	bEndForNow = false;
	bFinished = true;
	Log.write ( " Server: Released old movejob", cLog::eLOG_TYPE_NET_DEBUG );
	for ( unsigned int i = 0; i < Server->ActiveMJobs.Size(); i++ )
	{
		if ( this == Server->ActiveMJobs[i] ) return;
	}
	Server->addActiveMoveJob ( this );
	Log.write ( " Server: Added released movejob to avtive ones", cLog::eLOG_TYPE_NET_DEBUG );
}

bool cServerMoveJob::checkMove()
{
	bool bInSentryRange;
	if ( !Vehicle || !Waypoints || !Waypoints->next )
	{
		bFinished = true;
		return false;
	}

	bInSentryRange = Vehicle->InSentryRange();
	if ( !Server->Map->possiblePlace( Vehicle, Waypoints->next->X, Waypoints->next->Y) || bInSentryRange )
	{
		Log.write( " Server: Next point is blocked: ID: " + iToStr ( Vehicle->iID ) + ", X: " + iToStr ( Waypoints->next->X ) + ", Y: " + iToStr ( Waypoints->next->Y ), LOG_TYPE_NET_DEBUG );
		// if the next point would be the last, finish the job here
		if ( Waypoints->next->X == DestX && Waypoints->next->Y == DestY )
		{
			bFinished = true;
		}
		// else delete the movejob and inform the client that he has to find a new path
		else
		{
			for ( unsigned int i = 0; i < Vehicle->SeenByPlayerList.Size(); i++ )
			{
				sendNextMove( Vehicle->iID, Vehicle->PosX+Vehicle->PosY*Map->size, MJOB_BLOCKED, Vehicle->SeenByPlayerList[i]->Nr );
			}
			sendNextMove ( Vehicle->iID, Vehicle->PosX+Vehicle->PosY*Map->size, MJOB_BLOCKED, Vehicle->owner->Nr );
		}
		return false;
	}

	// not enough waypoints for this move
	if ( Vehicle->data.speed < Waypoints->next->Costs )
	{
		Log.write( " Server: Vehicle has not enough waypoints for the next move -> EndForNow: ID: " + iToStr ( Vehicle->iID ) + ", X: " + iToStr ( Waypoints->next->X ) + ", Y: " + iToStr ( Waypoints->next->Y ), LOG_TYPE_NET_DEBUG );
		iSavedSpeed += Vehicle->data.speed;
		Vehicle->data.speed = 0;
		bEndForNow = true;
		return true;
	}

	Vehicle->MoveJobActive = true;
	Vehicle->moving = true;

	// reserv the next field
	if ( !bPlane )
	{
		Map->fields[Waypoints->next->X+Waypoints->next->Y*Map->size].reserved = true;
		iReservedOff = Waypoints->next->X+Waypoints->next->Y*Map->size;
	}
	else
	{
		Map->fields[Waypoints->next->X+Waypoints->next->Y*Map->size].air_reserved = true;
		iReservedOff = Waypoints->next->X+Waypoints->next->Y*Map->size;
	}

	// send move command to all players who can see the unit
	for ( unsigned int i = 0; i < Vehicle->SeenByPlayerList.Size(); i++ )
	{
		sendNextMove( Vehicle->iID, Vehicle->PosX+Vehicle->PosY*Map->size, MJOB_OK, Vehicle->SeenByPlayerList[i]->Nr );
	}
	sendNextMove ( Vehicle->iID, Vehicle->PosX+Vehicle->PosY*Map->size, MJOB_OK, Vehicle->owner->Nr );
	return true;
}

void cServerMoveJob::moveVehicle()
{
	int iSpeed;
	if ( !Vehicle ) return;
	if ( Vehicle->data.is_human )
	{
		iSpeed = MOVE_SPEED/2;
	}
	else if ( !(Vehicle->data.can_drive == DRIVE_AIR) && !(Vehicle->data.can_drive == DRIVE_SEA) )
	{
		iSpeed = MOVE_SPEED;
		cBuilding* building = Map->fields[Waypoints->next->X+Waypoints->next->Y*Map->size].getBaseBuilding();
		if ( Waypoints && Waypoints->next && building && building->data.is_road ) iSpeed *= 2;
	}
	else if ( Vehicle->data.can_drive == DRIVE_AIR ) iSpeed = MOVE_SPEED*2;
	else iSpeed = MOVE_SPEED;

	switch ( iNextDir )
	{
		case 0:
			Vehicle->OffY -= iSpeed;
			break;
		case 1:
			Vehicle->OffY -= iSpeed;
			Vehicle->OffX += iSpeed;
			break;
		case 2:
			Vehicle->OffX += iSpeed;
			break;
		case 3:
			Vehicle->OffX += iSpeed;
			Vehicle->OffY += iSpeed;
			break;
		case 4:
			Vehicle->OffY += iSpeed;
			break;
		case 5:
			Vehicle->OffX -= iSpeed;
			Vehicle->OffY += iSpeed;
			break;
		case 6:
			Vehicle->OffX -= iSpeed;
			break;
		case 7:
			Vehicle->OffX -= iSpeed;
			Vehicle->OffY -= iSpeed;
			break;
	}


	// check whether the point has been reached:
	if ( Vehicle->OffX >= 64 || Vehicle->OffY >= 64 || Vehicle->OffX <= -64 || Vehicle->OffY <= -64 )
	{
		Log.write(" Server: Vehicle reached the next field: ID: " + iToStr ( Vehicle->iID )+ ", X: " + iToStr ( Waypoints->next->X ) + ", Y: " + iToStr ( Waypoints->next->Y ), cLog::eLOG_TYPE_NET_DEBUG);
		sWaypoint *Waypoint;
		Waypoint = Waypoints->next;
		delete Waypoints;
		Waypoints = Waypoint;

		if ( Vehicle->data.can_drive == DRIVE_AIR )
		{
			Map->fields[Waypoints->X+Waypoints->Y*Map->size].air_reserved = false;
			iReservedOff = -1;
		}
		else
		{
			Map->fields[Waypoints->X+Waypoints->Y*Map->size].reserved = false;
			iReservedOff = -1;
		}

		Map->moveVehicle( Vehicle, Waypoints->X, Waypoints->Y );

		Vehicle->OffX = 0;
		Vehicle->OffY = 0;
		
		if ( Waypoints->next == NULL )
		{
			bFinished = true;
		}

		Vehicle->data.speed += iSavedSpeed;
		iSavedSpeed = 0;
		Vehicle->DecSpeed ( Waypoints->Costs );

		// check for results of the move

		// make mines explode if necessary
		cBuilding* mine = Map->fields[Vehicle->PosX+Vehicle->PosY*Map->size].getMine();
		if ( Vehicle->data.can_drive != DRIVE_AIR && mine && mine->owner != Vehicle->owner )
		{
			Server->AJobs.Add( new cServerAttackJob( mine, Vehicle->PosX+Vehicle->PosY*Map->size ));
			bEndForNow = true;
		}

		// search for resources if necessary
		if ( Vehicle->data.can_survey )
		{
			sendVehicleResources( Vehicle, Map );
			Vehicle->doSurvey();
		}

		//hide vehicle
		while ( Vehicle->DetectedByPlayerList.Size() ) Vehicle->resetDetectedByPlayer( Vehicle->DetectedByPlayerList[0]);

		//handle detection
		Vehicle->makeDetection();

		// let other units fire on this one
		Vehicle->InSentryRange();

		// lay/clear mines if necessary
		if ( Vehicle->data.can_lay_mines )
		{
			bool bResult = false;
			if ( Vehicle->LayMines ) bResult = Vehicle->layMine();
			else if ( Vehicle->ClearMines ) bResult = Vehicle->clearMine();
			if ( bResult )
			{
				// send new unit values
				sendUnitData( Vehicle, Vehicle->owner->Nr );
				for ( unsigned int i = 0; i < Vehicle->SeenByPlayerList.Size(); i++ )
				{
					sendUnitData(Vehicle, Vehicle->SeenByPlayerList[i]->Nr );
				}
			}
		}

		Vehicle->owner->DoScan();

		Vehicle->moving = false;
		calcNextDir();

		//workaround for repairing/reloading
		cBuilding* b = (*Server->Map)[Vehicle->PosX+Vehicle->PosY*Server->Map->size].getBuildings();
		if ( Vehicle->data.can_drive == DRIVE_AIR && b && b->owner == Vehicle->owner && b->data.is_pad )
		{
			Vehicle->FlightHigh = 0;
		}
		else
		{
			Vehicle->FlightHigh = 64;
		}
	}
}

void cServerMoveJob::calcNextDir()
{
	if ( !Waypoints || !Waypoints->next ) return;
#define TESTXY_CND(a,b) if( Waypoints->X a Waypoints->next->X && Waypoints->Y b Waypoints->next->Y )
	TESTXY_CND ( ==,> ) iNextDir = 0;
	else TESTXY_CND ( <,> ) iNextDir = 1;
	else TESTXY_CND ( <,== ) iNextDir = 2;
	else TESTXY_CND ( <,< ) iNextDir = 3;
	else TESTXY_CND ( ==,< ) iNextDir = 4;
	else TESTXY_CND ( >,< ) iNextDir = 5;
	else TESTXY_CND ( >,== ) iNextDir = 6;
	else TESTXY_CND ( >,> ) iNextDir = 7;
}

cClientMoveJob::cClientMoveJob ( int iSrcOff, int iDestOff, bool bPlane, cVehicle *Vehicle )
{
	if ( !Client ) return;

	Map = Client->Map;
	this->Vehicle = Vehicle;
	ScrX = iSrcOff%Map->size;
	ScrY = iSrcOff/Map->size;
	DestX = iDestOff%Map->size;
	DestY = iDestOff/Map->size;
	this->bPlane = bPlane;
	bShip = Vehicle->data.can_drive == DRIVE_SEA;
	bFinished = false;
	bEndForNow = false;
	iSavedSpeed = 0;
	Waypoints = NULL;

	/*if ( Vehicle->PosX != ScrX || Vehicle->PosY != ScrY )
	{
		Log.write(" Client: Vehicle with id " + iToStr ( Vehicle->iID ) + " is at wrong position (" + iToStr (Vehicle->PosX) + "x" + iToStr(Vehicle->PosY) + ") for movejob from " +  iToStr (ScrX) + "x" + iToStr (ScrY) + " to " + iToStr (DestX) + "x" + iToStr (DestY) + "resetting to right position", cLog::eLOG_TYPE_NET_WARNING);
		// set vehicle to correct position
		if  ( !bPlane && Vehicle == Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].vehicle ) Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].vehicle = NULL;
		else if ( bPlane && Vehicle == Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].plane ) Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].plane = NULL;
		Vehicle->PosX = ScrX;
		Vehicle->PosY = ScrY;
		if ( !bPlane ) Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].vehicle = Vehicle;
		else Map->GO[Vehicle->PosX+Vehicle->PosY*Map->size].plane = Vehicle;
	}*/

	if ( Vehicle->ClientMoveJob )
	{
		Vehicle->ClientMoveJob->release();
		Vehicle->moving = false;
		Vehicle->MoveJobActive = false;
	}
	Vehicle->ClientMoveJob = this;
}

cClientMoveJob::~cClientMoveJob()
{
	sWaypoint *NextWaypoint;
	while ( Waypoints )
	{
		NextWaypoint = Waypoints->next;
		delete Waypoints;
		Waypoints = NextWaypoint;
	}
	Waypoints = NULL;
}

bool cClientMoveJob::generateFromMessage( cNetMessage *message )
{
	if ( message->iType != GAME_EV_MOVE_JOB_SERVER ) return false;
	int iCount = 0;
	int iWaypointOff;
	int iReceivedCount = message->popInt16();

	Log.write(" Client: Received MoveJob: VehicleID: " + iToStr( Vehicle->iID ) + ", SrcX: " + iToStr( ScrX ) + ", SrcY: " + iToStr( ScrY ) + ", DestX: " + iToStr( DestX ) + ", DestY: " + iToStr( DestY ) + ", WaypointCount: " + iToStr( iReceivedCount ), cLog::eLOG_TYPE_NET_DEBUG);

	// Add the waypoints
	sWaypoint *Waypoint = new sWaypoint;
	Waypoints = Waypoint;
	while ( iCount < iReceivedCount )
	{
		iWaypointOff = message->popInt32();
		Waypoint->X = iWaypointOff%Map->size;
		Waypoint->Y = iWaypointOff/Map->size;
		Waypoint->Costs = message->popInt16();
		Waypoint->next = NULL;

		iCount++;

		if ( iCount < iReceivedCount )
		{
			Waypoint->next = new sWaypoint;
			Waypoint = Waypoint->next;
		}
	}
	calcNextDir ();
	return true;
}

bool cClientMoveJob::calcPath()
{
	if ( ScrX == DestX && ScrY == DestY ) return false;

	cPathCalculator PathCalculator( ScrX, ScrY, DestX, DestY, Map, Vehicle );
	Waypoints = PathCalculator.Waypoints;
	if ( Waypoints )
	{
		calcNextDir();
		return true;
	}
	return false;
}

void cClientMoveJob::release()
{
	bEndForNow = false;
	bFinished = true;
	Log.write ( " Client: Released old movejob", cLog::eLOG_TYPE_NET_DEBUG );
	for (unsigned int i = 0; i < Client->ActiveMJobs.Size(); i++)
	{
		if ( this == Client->ActiveMJobs[i] ) return;
	}
	Client->addActiveMoveJob ( this );
	Log.write ( " Client: Added released movejob to avtive ones", cLog::eLOG_TYPE_NET_DEBUG );
}

void cClientMoveJob::handleNextMove( int iNextDestX, int iNextDestY, int iType )
{
	// set vehicle to server position
	if ( Vehicle->PosX != iNextDestX || Vehicle->PosY != iNextDestY )
	{
		// the client is faster then the server and has already
		// reached the last field or the next will be the last,
		// then stop the vehicle
		if ( Waypoints == NULL || Waypoints->next == NULL )
		{
			Log.write ( " Client: Client has already reached the last field", cLog::eLOG_TYPE_NET_DEBUG );
			bFinished = true;
			Vehicle->OffX = Vehicle->OffY = 0;
			return;
		}
		else
		{
			// server is one field faster then client
			if ( iNextDestX == Waypoints->next->X && iNextDestY == Waypoints->next->Y )
			{
				Log.write ( " Client: Server is one field faster then client", cLog::eLOG_TYPE_NET_DEBUG );
				doEndMoveVehicle();
			}
			else
			{
				// check whether the destination field is one of the next in the waypointlist
				// if not it must have been one that has been deleted already
				bool bServerIsFaster = false;
				sWaypoint *Waypoint = Waypoints->next->next;
				while ( Waypoint )
				{
					if ( Waypoint->X == iNextDestX && Waypoint->Y == iNextDestY )
					{
						bServerIsFaster = true;
						break;
					}
					Waypoint = Waypoint->next;
				}
				// the server is more then one field faster
				if ( bServerIsFaster )
				{
					Log.write ( " Client: Server is more then one field faster", cLog::eLOG_TYPE_NET_DEBUG );

					Map->moveVehicle( Vehicle, iNextDestX, iNextDestY );
					Vehicle->OffX = Vehicle->OffY = 0;

					Waypoint = Waypoints;
					while ( Waypoint )
					{
						if ( Waypoint->next->X != iNextDestX && Waypoint->next->Y != iNextDestY )
						{
							Waypoints = Waypoint->next;
							delete Waypoint;
							Waypoint = Waypoints;
						}
						else
						{
							Waypoints = Waypoint;
							break;
						}
					}
				}
				// the client is faster
				else
				{
					Log.write ( " Client: Client is faster (one or more fields) deactivating movejob; Vehicle-ID: " + iToStr ( Vehicle->iID ), cLog::eLOG_TYPE_NET_DEBUG );
					// just stop the vehicle and wait for the next commando of the server
					for ( unsigned int i = 0; i < Client->ActiveMJobs.Size(); i++ )
					{
						if ( Client->ActiveMJobs[i] == this ) Client->ActiveMJobs.Delete ( i );
					}
					Vehicle->MoveJobActive = false;
					bEndForNow = true;
					return;
				}
			}
		}
	}
	switch ( iType )
	{
	case MJOB_OK:
		{
			Vehicle->moving = true;
			if ( !Vehicle->MoveJobActive ) Vehicle->StartMoveSound();
			if ( bEndForNow && !Vehicle->MoveJobActive )
			{
				bEndForNow = false;
				Client->addActiveMoveJob ( Vehicle->ClientMoveJob );
				Log.write ( " Client: reactivated movejob; Vehicle-ID: " + iToStr ( Vehicle->iID ), cLog::eLOG_TYPE_NET_DEBUG );
			}
			Vehicle->MoveJobActive = true;
			Log.write(" Client: The movejob is ok: DestX: " + iToStr ( Waypoints->next->X ) + ", DestY: " + iToStr ( Waypoints->next->Y ), cLog::eLOG_TYPE_NET_DEBUG);
		}
		break;
	case MJOB_STOP:
		{
			Log.write(" Client: The movejob will end for now", cLog::eLOG_TYPE_NET_DEBUG);
			bSuspended = true;
			bEndForNow = true;
		}
		break;
	case MJOB_FINISHED:
		{
			Log.write(" Client: The movejob is finished", cLog::eLOG_TYPE_NET_DEBUG);
			release ();
		}
		break;
	case MJOB_BLOCKED:
		{
			Log.write(" Client: Movejob is finished becouse the next field is blocked: DestX: " + iToStr ( Waypoints->next->X ) + ", DestY: " + iToStr ( Waypoints->next->Y ), cLog::eLOG_TYPE_NET_DEBUG);
			bFinished = true;
			// TODO: Calc a new path and start the new job
		}
		break;
	}
}

void cClientMoveJob::moveVehicle()
{
	if ( Vehicle == NULL || Vehicle->ClientMoveJob != this ) return;

	// do not move the vehicle, if the movejob hasn't got any more waypoints
	if ( Waypoints == NULL || Waypoints->next == NULL ) return;

	int iSpeed;
	if ( Vehicle->data.is_human )
	{
		Vehicle->WalkFrame++;
		if ( Vehicle->WalkFrame >= 13 ) Vehicle->WalkFrame = 1;
		iSpeed = MOVE_SPEED/2;
	}
	else if ( !(Vehicle->data.can_drive == DRIVE_AIR) && !(Vehicle->data.can_drive == DRIVE_SEA) )
	{
		iSpeed = MOVE_SPEED;
		cBuilding* building = Map->fields[Waypoints->next->X+Waypoints->next->Y*Map->size].getBaseBuilding();
		if ( Waypoints && Waypoints->next && building && building->data.is_road ) iSpeed *= 2;
	}
	else if ( Vehicle->data.can_drive == DRIVE_AIR ) iSpeed = MOVE_SPEED*2;
	else iSpeed = MOVE_SPEED;

	// Ggf Tracks malen:
	if ( SettingsData.bMakeTracks && Vehicle->data.make_tracks && !Map->IsWater ( Vehicle->PosX+Vehicle->PosY*Map->size,false ) &&!
	        ( Waypoints && Waypoints->next && Map->terrain[Map->Kacheln[Waypoints->next->X+Waypoints->next->Y*Map->size]].water ) &&
	        ( Vehicle->owner == Client->ActivePlayer || Client->ActivePlayer->ScanMap[Vehicle->PosX+Vehicle->PosY*Map->size] ) )
	{
		if ( !Vehicle->OffX && !Vehicle->OffY )
		{
			switch ( Vehicle->dir )
			{
				case 0:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64-10,0 );
					break;
				case 4:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64+10,0 );
					break;
				case 2:
					Client->addFX ( fxTracks,Vehicle->PosX*64+10,Vehicle->PosY*64,2 );
					break;
				case 6:
					Client->addFX ( fxTracks,Vehicle->PosX*64-10,Vehicle->PosY*64,2 );
					break;
				case 1:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64,1 );
					break;
				case 5:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64,1 );
					break;
				case 3:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64,3 );
					break;
				case 7:
					Client->addFX ( fxTracks,Vehicle->PosX*64,Vehicle->PosY*64,3 );
					break;
			}
		}
		else if ( abs ( Vehicle->OffX ) == iSpeed*2 || abs ( Vehicle->OffY ) == iSpeed*2 )
		{
			switch ( Vehicle->dir )
			{
				case 1:
					Client->addFX ( fxTracks,Vehicle->PosX*64+26,Vehicle->PosY*64-26,1 );
					break;
				case 5:
					Client->addFX ( fxTracks,Vehicle->PosX*64-26,Vehicle->PosY*64+26,1 );
					break;
				case 3:
					Client->addFX ( fxTracks,Vehicle->PosX*64+26,Vehicle->PosY*64+26,3 );
					break;
				case 7:
					Client->addFX ( fxTracks,Vehicle->PosX*64-26,Vehicle->PosY*64-26,3 );
					break;
			}
		}
	}

	switch ( iNextDir )
	{
		case 0:
			Vehicle->OffY -= iSpeed;
			break;
		case 1:
			Vehicle->OffY -= iSpeed;
			Vehicle->OffX += iSpeed;
			break;
		case 2:
			Vehicle->OffX += iSpeed;
			break;
		case 3:
			Vehicle->OffX += iSpeed;
			Vehicle->OffY += iSpeed;
			break;
		case 4:
			Vehicle->OffY += iSpeed;
			break;
		case 5:
			Vehicle->OffX -= iSpeed;
			Vehicle->OffY += iSpeed;
			break;
		case 6:
			Vehicle->OffX -= iSpeed;
			break;
		case 7:
			Vehicle->OffX -= iSpeed;
			Vehicle->OffY -= iSpeed;
			break;
	}

	// check whether the point has been reached:
	if ( Vehicle->OffX >= 64 || Vehicle->OffY >= 64 || Vehicle->OffX <= -64 || Vehicle->OffY <= -64 )
	{
		doEndMoveVehicle ();
	}
}

void cClientMoveJob::doEndMoveVehicle ()
{
	if ( Vehicle == NULL || Vehicle->ClientMoveJob != this ) return;

	if ( Waypoints->next == NULL )
	{
		// this is just to avoid errors, this should normaly never happen.
		bFinished = true;
		return;
	}

	Vehicle->data.speed += iSavedSpeed;
	iSavedSpeed = 0;
	Vehicle->DecSpeed ( Waypoints->next->Costs );
	Vehicle->moving = false;
	Vehicle->WalkFrame = 0;

	sWaypoint *Waypoint;
	Waypoint = Waypoints->next;
	delete Waypoints;
	Waypoints = Waypoint;

	Vehicle->moving = false;

	cMapField& field = Map->fields[Waypoints->X+Waypoints->Y*Map->size];
	if ( !bPlane )
	{
		if ( field.getVehicles() != NULL || field.getTopBuilding() != NULL && !field.getTopBuilding()->data.is_connector )
		{
			if ( field.getVehicles() ) Log.write ( " Client: Next waypoint for vehicle with ID \"" + iToStr( Vehicle->iID )  + "\" is blocked by an other vehicle with ID\"" + iToStr( field.getVehicles()->iID ) + "\"", cLog::eLOG_TYPE_NET_ERROR );
			else Log.write ( " Client: Next waypoint for vehicle with ID \"" + iToStr( Vehicle->iID )  + "\" is blocked by an other building with ID\"" + iToStr( field.getTopBuilding()->iID ) + "\"", cLog::eLOG_TYPE_NET_ERROR );
			bFinished = true;
		}
		else
		{
			Map->moveVehicle( Vehicle, Waypoints->X, Waypoints->Y );
			Log.write(" Client: Vehicle reached the next field: ID: " + iToStr ( Vehicle->iID )+ ", X: " + iToStr ( Waypoints->X ) + ", Y: " + iToStr ( Waypoints->Y ), cLog::eLOG_TYPE_NET_DEBUG);
		}
	}
	else
	{
		if ( field.getPlanes() != NULL )
		{
			Log.write ( " Client: Next waypoint for plane with ID \"" + iToStr( Vehicle->iID )  + "\" is blocked by an other plane with ID\"" + iToStr( field.getPlanes()->iID ) + "\"", cLog::eLOG_TYPE_NET_ERROR );
			bFinished = true;
		}
		else
		{
			Map->moveVehicle( Vehicle, Waypoints->X, Waypoints->Y );
			Log.write(" Client: Vehicle reached the next field: ID: " + iToStr ( Vehicle->iID )+ ", X: " + iToStr ( Waypoints->X ) + ", Y: " + iToStr ( Waypoints->Y ), cLog::eLOG_TYPE_NET_DEBUG);
		}
	}
	Vehicle->OffX = 0;
	Vehicle->OffY = 0;

	Vehicle->owner->DoScan();
	
	Client->bFlagDrawMMap = true; 
	Client->mouseMoveCallback( true ); 

	calcNextDir();
}

void cClientMoveJob::calcNextDir ()
{
	if ( !Waypoints || !Waypoints->next ) return;
#define TESTXY_CND(a,b) if( Waypoints->X a Waypoints->next->X && Waypoints->Y b Waypoints->next->Y )
	TESTXY_CND ( ==,> ) iNextDir = 0;
	else TESTXY_CND ( <,> ) iNextDir = 1;
	else TESTXY_CND ( <,== ) iNextDir = 2;
	else TESTXY_CND ( <,< ) iNextDir = 3;
	else TESTXY_CND ( ==,< ) iNextDir = 4;
	else TESTXY_CND ( >,< ) iNextDir = 5;
	else TESTXY_CND ( >,== ) iNextDir = 6;
	else TESTXY_CND ( >,> ) iNextDir = 7;
}

void cClientMoveJob::drawArrow ( SDL_Rect Dest, SDL_Rect *LastDest, bool bSpezial )
{
	int iIndex = 0;
#define TESTXY_DP(a,b) if( Dest.x a LastDest->x && Dest.y b LastDest->y )
	TESTXY_DP ( >,< ) iIndex = 0;
	else TESTXY_DP ( ==,< ) iIndex = 1;
	else TESTXY_DP ( <,< ) iIndex = 2;
	else TESTXY_DP ( >,== ) iIndex = 3;
	else TESTXY_DP ( <,== ) iIndex = 4;
	else TESTXY_DP ( >,> ) iIndex = 5;
	else TESTXY_DP ( ==,> ) iIndex = 6;
	else TESTXY_DP ( <,> ) iIndex = 7;
	if ( bSpezial )
	{
		SDL_BlitSurface ( OtherData.WayPointPfeileSpecial[iIndex][64-Client->Hud.Zoom], NULL, buffer, &Dest );
	}
	else
	{
		SDL_BlitSurface ( OtherData.WayPointPfeile[iIndex][64-Client->Hud.Zoom], NULL, buffer, &Dest );
	}
}
