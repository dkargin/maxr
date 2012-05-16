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
#include "math.h"
#include "client.h"
#include "clientevents.h"
#include "automjobs.h"
#include "vehicles.h"
#include "movejobs.h"
#include "player.h"

using namespace std;

static cList<cAutoMJob*> autoMJobs;

//static functions of cAutoMJob

//static function that calls DoAutoMove for all active auto move jobs
//this function is periodically called by the engine
void cAutoMJob::handleAutoMoveJobs()
{
	for (size_t i = 0; i < autoMJobs.Size(); ++i)
	{
		autoMJobs[i]->DoAutoMove();
		if ( autoMJobs[i]->finished )
		{
			delete autoMJobs[i];
		}
	}
}

//functions of cAutoMJobs

//construktor for cAutoMJob
cAutoMJob::cAutoMJob(cVehicle *vehicle)
{
	 iNumber = (int)autoMJobs.Size();
	 autoMJobs.Add(this);
	 this->vehicle = vehicle;
	 finished = false;
	 OPX = vehicle->PosX;
	 OPY = vehicle->PosY;
	 playerMJob = false;
	 lastDestX = vehicle->PosX;
	 lastDestY = vehicle->PosY;
	 n = iNumber % WAIT_FRAMES; //this is just to prevent, that posibly all surveyors try to calc their next move in the same frame
	 sendSetAutoStatus ( vehicle->iID, true );
}

//destruktor for cAutoMJob
cAutoMJob::~cAutoMJob()
{
	if (!playerMJob )
	{
		sendWantStopMove( vehicle->iID );
	}
	sendSetAutoStatus ( vehicle->iID, false );
	for (unsigned int i = iNumber; i < autoMJobs.Size() - 1; i++)
	{
		autoMJobs[i] = autoMJobs[i + 1];
		autoMJobs[i]->iNumber = (int)i;
	}
	autoMJobs.PopBack();

	vehicle->autoMJob = NULL;
}

//performs the auto move of a vehicle and adds new mjobs to the engine, if necessary
void cAutoMJob::DoAutoMove()
{
	if ( vehicle->isBeeingAttacked ) return;
	if ( Client->bWaitForOthers ) return;
	if ( vehicle->owner != Client->getActivePlayer() ) return;

	if ( vehicle->ClientMoveJob == NULL || vehicle->ClientMoveJob->bFinished )
	{
			if (n > WAIT_FRAMES)
			{
				changeOP();
				PlanNextMove();
				n = 0;
			}
			else
			{
				n++;
			}
	}
	else
	{
		if ( vehicle->ClientMoveJob && (vehicle->ClientMoveJob->DestX != lastDestX || vehicle->ClientMoveJob->DestY != lastDestY)  )
		{
			playerMJob = true;
		}
		if ( vehicle->ClientMoveJob->bSuspended && vehicle->data.speedCur )
		{
			Client->addMoveJob( vehicle, vehicle->ClientMoveJob->DestX, vehicle->ClientMoveJob->DestY );
			n = iNumber % WAIT_FRAMES; //prevent, that all surveyors try to calc their next move in the same frame
		}
	}

}

//think about the next move:
//the AI looks at all fields next to the surveyor
//and calculates a factor for each field
//the surveyor will move to the field with the highest value
void cAutoMJob::PlanNextMove()
{
	//TODO: completely survey a partly explored area with resources, like in the org MAX
	//TODO: if no resources found in the immediate area of the surveyor, plan longer path immediately,
	//      like in the org MAX. This will also decrease the number of events generated by planning every step
	int x, y;
	int bestX = -1, bestY = -1;
	float tempFactor;
	float maxFactor = FIELD_BLOCKED;

	for ( x = vehicle->PosX - 1; x <= vehicle->PosX + 1; x ++)
	{
		for (y = vehicle->PosY - 1; y <= vehicle->PosY + 1; y++)
		{
			// skip the surveyor's current position
			if ( x == vehicle->PosX && y == vehicle->PosY ) continue;

			tempFactor = CalcFactor( x, y);
			if ( tempFactor > maxFactor )
			{
				maxFactor = tempFactor;
				bestX = x;
				bestY = y;
			}
		}
	}

	if ( maxFactor != FIELD_BLOCKED )
	{
		Client->addMoveJob( vehicle, bestX, bestY );
		lastDestX = bestX;
		lastDestY = bestY;
	}
	else //no fields to survey next to the surveyor
	{
		PlanLongMove();
	}
}


//calculates an "importance-factor" for a given field
float cAutoMJob::CalcFactor(int PosX, int PosY)
{
	if ( !Client->getMap()->possiblePlace( vehicle, PosX, PosY, true )) return (float)FIELD_BLOCKED;

	//calculate some values, on which the "importance-factor" may depend

	// calculate the number of fields which would be surveyed by this move
	float NrSurvFields = 0;
	int x, y;
	for ( x = PosX - 1; x <= PosX + 1; x ++)
	{
		// check for map borders
		if ( x < 0 || x >= Client->getMap()->size ) continue;
		for (y = PosY - 1; y <= PosY + 1; y++)
		{
			// check for map borders
			if ( y < 0 || y >= Client->getMap()->size ) continue;

			int iPos = x + y * Client->getMap()->size;

			// int terrainNr = Client->getMap()->Kacheln[x + y * Client->getMap()->size]; !the line where this variable is needed was commented out earlier!
			if ( vehicle->owner->ResourceMap[iPos] == 0 )//&& !Client->getMap()->terrain[terrainNr].blocked )
			{
				NrSurvFields++;
			}
		}
	}
	if (vehicle->PosX != PosX && vehicle->PosY != PosY) //diagonal move
	{
		NrSurvFields /= 2;
	}

	// calculate the number of fields which has already revealed resources
	float NrResFound = 0;
	for ( x = PosX - 1; x <= PosX + 1; x ++)
	{
		// check for map borders
		if ( x < 0 || x >= Client->getMap()->size ) continue;
		for (y = PosY - 1; y <= PosY + 1; y++)
		{
			// check for map borders
			if ( y < 0 || y >= Client->getMap()->size ) continue;

			int iPos = x + y * Client->getMap()->size;

			// check if the surveyor already found some resources in this new direction or not
			if ( vehicle->owner->ResourceMap[iPos] != 0 && Client->getMap()->Resources[iPos].typ != 0 )
			{
				NrResFound++;
			}

		}
	}

	//the distance to the OP
	float newDistanceOP = sqrt( pow((float) PosX - OPX , 2) + pow((float) PosY - OPY , 2) );

	//the distance to other surveyors
	float newDistancesSurv = 0;
	float temp;
	for (size_t i = 0; i < autoMJobs.Size(); ++i)
	{
		if ( i == iNumber ) continue;
		if (autoMJobs[i]->vehicle->owner != vehicle->owner) continue;
		temp = sqrt( pow( (float) PosX - autoMJobs[i]->vehicle->PosX , (float)2.0) + pow( (float) PosY - autoMJobs[i]->vehicle->PosY , (float)2.0) );
		newDistancesSurv += pow( (float)temp, (float)EXP);
	}

	//and now calc the "importance-factor"

	if (NrSurvFields == 0) return (float)FIELD_BLOCKED;

	float factor = (float)(A * NrSurvFields + G * NrResFound - B * newDistanceOP - C * newDistancesSurv);

	if (factor < FIELD_BLOCKED)
	{
		factor = FIELD_BLOCKED;
	}

	return  factor;

}

//searches the map for a location where the surveyor can resume
void cAutoMJob::PlanLongMove()
{
	int x, y;
	int bestX = -1, bestY = -1;
	float distanceOP, distanceSurv;
	float factor;
	float minValue = 0;

	for ( x = 0; x < Client->getMap()->size; x++ )
	{
		for ( y = 0; y < Client->getMap()->size; y++ )
		{
			// if field is not passable/walkable or if it's already has been explored, continue
			if ( !Client->getMap()->possiblePlace( vehicle, x, y ) || vehicle->owner->ResourceMap[x + y * Client->getMap()->size] == 1 ) continue;

			// calculate the distance to other surveyors
			float distancesSurv = 0;
			float temp;
			for (size_t i = 0; i < autoMJobs.Size(); ++i)
			{
				// skip our selves and other Players' surveyors
				if (i == iNumber || autoMJobs[i]->vehicle->owner != vehicle->owner) continue;
				temp = sqrt( pow( (float) x - autoMJobs[i]->vehicle->PosX , (float)2.0) + pow( (float) y - autoMJobs[i]->vehicle->PosY , (float)2.0) );
				distancesSurv += pow( (float)temp, (float)EXP2);
			}

			distanceOP = sqrt( pow( (float) x - OPX , 2) + pow( (float) y - OPY , 2) );
			distanceSurv = sqrt( pow( (float) x - vehicle->PosX , 2) + pow( (float) y - vehicle->PosY , 2) );
			//TODO: take into account the length of the path to the coordinates too (I seen a case, when a surveyor took 7 additional senseless steps just to avoid or by-pass an impassable rocky terrain)
			factor = (float)(D * distanceOP + E * distanceSurv + F * distancesSurv);

			if ( (factor < minValue) || (minValue == 0) )
			{
				minValue = factor;
				bestX = x;
				bestY = y;
			}
			//TODO: check, if a path can be found to the coordinates
		}
	}
	if ( minValue != 0 )
	{
		if ( Client->addMoveJob( vehicle, bestX, bestY ) )
		{
			lastDestX = bestX;
			lastDestY = bestY;
		}
		else
		{
			string message = "Surveyor AI: I'm totally confused. Don't know what to do...";
			Client->getActivePlayer()->addSavedReport ( Client->addCoords( message, vehicle->PosX, vehicle->PosY ), sSavedReportMessage::REPORT_TYPE_UNIT, vehicle->data.ID, vehicle->PosX, vehicle->PosY );
			finished = true;
		}
	}
	else
	{
		string message = "Surveyor AI: My life is so senseless. I've nothing to do...";
		Client->getActivePlayer()->addSavedReport ( Client->addCoords( message, vehicle->PosX, vehicle->PosY ), sSavedReportMessage::REPORT_TYPE_UNIT, vehicle->data.ID, vehicle->PosX, vehicle->PosY );
		finished = true;
	}
}

//places the OP nearer to the surveyor, if the distance between surv. and OP exceeds MAX_DISTANCE_OP
//places the OP to the actual position, if the surveyor was send there by the player
void cAutoMJob::changeOP()
{
	if ( playerMJob )
	{
		OPX = vehicle->PosX;
		OPY = vehicle->PosY;
		playerMJob = false;
	}
	else
	{
		float distanceOP = sqrt( pow( (float) vehicle->PosX - OPX , 2) + pow( (float) vehicle->PosY - OPY , 2) );
		if ( distanceOP > MAX_DISTANCE_OP )
		{
			OPX = (int) (vehicle->PosX + ( OPX - vehicle->PosX ) * (float) DISTANCE_NEW_OP / MAX_DISTANCE_OP);
			OPY = (int) (vehicle->PosY + ( OPY - vehicle->PosY ) * (float) DISTANCE_NEW_OP / MAX_DISTANCE_OP);
		}
	}
}
