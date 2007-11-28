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
#include "automjobs.h"
#include "engine.h"
#include "vehicles.h"



//static variables
cEngine *cAutoMJob::engine = NULL;
cAutoMJob **cAutoMJob::autoMJobs = NULL;
int cAutoMJob::iCount = 0;

//static functions of cAutoMJob

//static funktion, that initializes the cAutoMJob-Class
void cAutoMJob::init(cEngine* engine)
{
	cAutoMJob::engine = engine;
	iCount = 0;
}

//static function that calls DoAutoMove for all active auto move jobs
//this function is periodically called by the engine
void cAutoMJob::handleAutoMoveJobs()
{
	int i;
	for ( i = 0; i < iCount; i++)
	{
		autoMJobs[i]->DoAutoMove();
	}
}

//functions of cAutoMJobs

//construktor for cAutoMJob
cAutoMJob::cAutoMJob(cVehicle *vehicle)
{
	 autoMJobs = (cAutoMJob **) realloc(autoMJobs, (iCount + 1) * sizeof(this));
	 autoMJobs[iCount] = this;
	 iNumber = iCount;
	 iCount++;
	 this->vehicle = vehicle;
	 OPX = vehicle->PosX;
	 OPY = vehicle->PosY;
	 n = iNumber % WAIT_FRAMES; //this is just to prevent, that posibly all surveyors try to calc their next move in the same frame
}

//destruktor for cAutoMJob
cAutoMJob::~cAutoMJob()
{
	int i;
	for (i = iNumber; i < iCount - 1; i++)
	{
		autoMJobs[i] = autoMJobs[i + 1];
		autoMJobs[i]->iNumber = i;
	}
	iCount--;
	autoMJobs = (cAutoMJob **) realloc(autoMJobs, iCount * sizeof(this));
}

//performs the auto move of a vehicle and adds new mjobs to the engine, if nessesary
void cAutoMJob::DoAutoMove()
{
	//TODO: check if surveyor was moved by the player, and set new operation point
	if (vehicle->mjob == NULL || vehicle->mjob->finished )
	{
		
		if (n > WAIT_FRAMES)
		{
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
		if (vehicle->mjob->Suspended && vehicle->data.speed)
		{
			engine->AddActiveMoveJob(vehicle->mjob);
			n = iNumber % WAIT_FRAMES; //prevent, that all surveyors try to calc their next move in the same frame
		}
	}

}

//think about the next move:
//the AI looks at all fields next to the surveyor
//and calculates an factor for each field
//the surveyor will move to the field with the highest value
void cAutoMJob::PlanNextMove()
{

	//TODO: survey an partly detected area with ressources completly, like in the org MAX
	int x, y;
	int bestX, bestY;
	float tempFactor;
	float maxFactor = FIELD_BLOCKED;

	for ( x = vehicle->PosX - 1; x <= vehicle->PosX + 1; x ++)
	{	
		for (y = vehicle->PosY - 1; y <= vehicle->PosY + 1; y++)
		{
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
		engine->AddMoveJob(vehicle->PosX + vehicle->PosY * engine->map->size, bestX + bestY * engine->map->size, false, false);
	}	
	else //no fields to survey next to the surveyor
	{
		PlanLongMove();
	}
}


//calculates an "importance-factor" for a given field
float cAutoMJob::CalcFactor(int PosX, int PosY)
{
	if ( !FieldIsFree(PosX, PosY) ) return FIELD_BLOCKED;

	//calculate some values, on which the "impotance-factor" may depend
	//TODO: don't use the deltas

	//the number of fields which would be surveyed by this move
	float NrSurvFields = 0; 
	int x, y;

	for ( x = PosX - 1; x <= PosX + 1; x ++)
	{	
		for (y = PosY - 1; y <= PosY + 1; y++)
		{
			if ( x == PosX && y == PosY ) continue;
			if ( x < 0 || y < 0 || x >= engine->map->size || y >= engine->map->size ) continue;

			if ( vehicle->owner->ResourceMap[x + y * engine->map->size] == 0)
			{
				NrSurvFields++;
			}
		}
	}
	if (vehicle->PosX != PosX && vehicle->PosY != PosY) //diagonal move
	{
		NrSurvFields /= 2;
	}
	
	//the distances to the OP
	
	float oldDistanceOP = sqrt( (float) (vehicle->PosX - OPX) * (vehicle->PosX - OPX) + (vehicle->PosY - OPY) * (vehicle->PosY - OPY) );
	float newDistanceOP = sqrt( (float) (PosX - OPX) * (PosX - OPX) + (PosY - OPY) * (PosY - OPY) );
	float deltaDistanceOP = oldDistanceOP - newDistanceOP;

	//distances to other surveyors
	
	int i;
	float oldDistancesSurv = 0;
	float newDistancesSurv = 0;
	float deltaDistancesSury;
	float temp;
	for ( i = 0; i < iCount ; i++)
	{
		if ( i == iNumber ) continue;
		
		temp = sqrt( pow( (float) PosX - autoMJobs[i]->vehicle->PosX , 2) + pow( (float) PosY - autoMJobs[i]->vehicle->PosY , 2) );
		newDistancesSurv += pow( temp, EXP);

		temp = sqrt( pow( (float) vehicle->PosX - autoMJobs[i]->vehicle->PosX , 2) + pow( (float) vehicle->PosY - autoMJobs[i]->vehicle->PosY , 2) );
		oldDistancesSurv += pow( temp, EXP);
	}
	deltaDistancesSury = oldDistancesSurv - newDistancesSurv;


	//and now the magic formula, which calculates the "importance-factor"
	
	
	if (NrSurvFields == 0) return FIELD_BLOCKED;
	float factor = A * NrSurvFields + B * deltaDistanceOP + C * deltaDistancesSury;
	
	if (factor < FIELD_BLOCKED)
	{
		factor = FIELD_BLOCKED;
	}

	return  factor;

}

//checks if the destination field is free
bool cAutoMJob::FieldIsFree(int PosX, int PosY)
{
	if ( PosX < 0 || PosY < 0 || PosX >= engine->map->size || PosY >= engine->map->size ) return false; //check map borders
	
	int terrainNr=engine->map->Kacheln[PosX + PosY * engine->map->size];
	if ( TerrainData.terrain[terrainNr].blocked ) return false; //check terrain
	
	sGameObjects objects = engine->map->GO[PosX + PosY * engine->map->size];
	if ( objects.reserviert || objects.vehicle || objects.top ) return false; //check if there is another unit on the field
	//FIXME: an field with a connector is returned as false

	//TODO: check for enemy mines

	return true;
}

//searches the map for a location where the surveyor can resume
void cAutoMJob::PlanLongMove()
{
	int x, y;
	int bestX, bestY;
	float destinationOP, destinationSurv;
	float tempValue;
	float minValue = 0;

	for ( x = 0; x < engine->map->size; x++ )
	{
		for ( y = 0; y < engine->map->size; y++ )
		{
			if ( !FieldIsFree( x, y) ) continue;
			if ( vehicle->owner->ResourceMap[x + y * engine->map->size] == 1 ) continue;

			destinationOP = sqrt( (float) (x - OPX) * (x - OPX) + (y - OPY) * (y - OPY) );
			destinationSurv = sqrt( (float) (x - vehicle->PosX) * (x - vehicle->PosX) + (y - vehicle->PosY) * (y - vehicle->PosY) );
			tempValue = D * destinationOP + E * destinationSurv;

			if ( (tempValue < minValue) || (minValue == 0) )
			{
				minValue = tempValue;
				bestX = x;
				bestY = y;
			}
			//TODO: check, if a path can be found to the coordinates
		}
	}
	if ( minValue != 0 )
	{
		engine->AddMoveJob( vehicle->PosX + vehicle->PosY * engine->map->size , bestX + bestY * engine->map->size, false, false);
	}
	else
	{
		//TODO: disable automove
	}
}