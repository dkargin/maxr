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
#include "engine.h"
#include "game.h"
#include "sound.h"
#include "fonts.h"
#include "mouse.h"

// Funktionen der Engine Klasse //////////////////////////////////////////////
cEngine::cEngine ( cMap *Map,cFSTcpIp *fstcpip )
{
	map=Map;
	mjobs=NULL;
	ActiveMJobs=new TList;
	AJobs=new TList;
	this->fstcpip=fstcpip;
	EndeCount=0;
	RundenendeActionsReport=0;
	if ( fstcpip&&fstcpip->bServer )
	{
		SyncNo=0;
	}
	else
	{
		SyncNo=-1;
	}
	PingList=NULL;
	LogFile=NULL;
	LogHistory=NULL;
}

cEngine::~cEngine ( void )
{
	// Alle Listen und Objekte l�schen:
	while ( mjobs )
	{
		cMJobs *next;
		next=mjobs->next;
		delete mjobs;
		mjobs=next;
	}
	delete ActiveMJobs;
	delete AJobs;
	if ( PingList )
	{
		while ( PingList->Count )
		{
			// delete (sPing*)(PingList->Items[0]);
			// PingList->Delete(0);
		}
		delete PingList;
	}
	StopLog();
}

// L�sst die Engine laufen:
void cEngine::Run ( void )
{
	int i;
	// Network
	if(fstcpip)
	{
		// Look for new messages
		if ( fstcpip->iStatus == STAT_CONNECTED && fstcpip->bReceiveThreadFinished )
		{
			SDL_WaitThread ( fstcpip->FSTcpIpReceiveThread, NULL ); // free the last memory allocated by the thread. If not done so, SDL_CreateThread will hang after about 1010 successfully created threads
			fstcpip->FSTcpIpReceiveThread = SDL_CreateThread ( Receive,NULL );
		}
		// Handle incomming messages
		HandleGameMessages();
	}

	// Diese Aktionen nur Zeitgebunden ausf�hren:
	if ( !timer0 ) return;

	// Alle Move-Jobs bearbeiten:
	for ( i=0;i<ActiveMJobs->Count;i++ )
	{
		bool WasMoving,BuildAtTarget;
		cMJobs *job;
		cVehicle *v;
		job=ActiveMJobs->MJobsItems[i];
		v=job->vehicle;
		if ( v )
		{
			WasMoving=v->MoveJobActive;
		}
		else
		{
			WasMoving=false;
		}
		BuildAtTarget=job->BuildAtTarget;
		// Pr�fen, ob der Job erledigt ist:
		if ( job->finished||job->EndForNow )
		{
			job->Suspended=true;
			// Den erledigten Job l�schen:
			cMJobs *ptr,*last;
			if ( job->EndForNow&&v )
			{
				if( fstcpip && fstcpip->bServer )
				{
					string sMessage;
					sMessage = iToStr( v->PosX + v->PosY * map->size ) + "#" + iToStr( job->DestX + job->DestY * map->size ) + "#";
					if ( job->plane ) sMessage += "1";
					else sMessage += "0";
					fstcpip->FSTcpIpSend ( MSG_END_MOVE_FOR_NOW,sMessage.c_str() );
					v->MoveJobActive = false;
				}
				else if( !fstcpip )
				{
					v->MoveJobActive = false;
				}
				ActiveMJobs->DeleteMJobs ( i );
			}
			else
			{
				if ( v&&v->mjob==job )
				{
					if( fstcpip && fstcpip->bServer )
					{
						string sMessage;
						sMessage = iToStr( v->PosX + v->PosY * map->size ) + "#";
						fstcpip->FSTcpIpSend ( MSG_END_MOVE,sMessage.c_str() );
						v->MoveJobActive = false;
					}
					else if( !fstcpip )
					{
						v->MoveJobActive = false;
					}
					v->mjob=NULL;
				}
				ActiveMJobs->DeleteMJobs ( i );
				ptr=mjobs;
				last=NULL;
				while ( ptr )
				{
					if ( ptr==job )
					{
						if ( !last )
						{
							mjobs=ptr->next;
						}
						else
						{
							last->next=ptr->next;
						}
						delete ptr;
						break;
					}
					last=ptr;
					ptr=ptr->next;
				}
			}
			i--;
			// Pr�fen, ob ein Sound zu spielen ist:
			if ( v==game->SelectedVehicle&&WasMoving )
			{
				StopFXLoop ( game->ObjectStream );
				if ( map->IsWater ( v->PosX+v->PosY*map->size ) &&v->data.can_drive!=DRIVE_AIR )
				{
					PlayFX ( v->typ->StopWater );
				}
				else
				{
					PlayFX ( v->typ->Stop );
				}
				game->ObjectStream=v->PlayStram();
			}
			// Pr�fen, ob gebaut werden soll:
			if ( BuildAtTarget )
			{
				v->IsBuilding=true;
				v->BuildRounds=v->BuildRoundsStart;
				v->BuildCosts=v->BuildCostsStart;
				if ( game->SelectedVehicle==v )
				{
					// Den Building Sound machen:
					StopFXLoop ( game->ObjectStream );
					game->ObjectStream=v->PlayStram();
				}
			}
			continue;
		}
		// Pr�fen, ob das Vehicle gedreht werden muss:
		if ( job->next_dir!=job->vehicle->dir&&job->vehicle->data.speed )
		{
			job->vehicle->rotating=true;
			if ( timer1 )
			{
				job->vehicle->RotateTo ( job->next_dir );
			}
			continue;
		}
		else
		{
			job->vehicle->rotating=false;
		}
		// Pr�fen, ob sich das Vehicle grade nicht bewegt:
		if ( !job->vehicle->moving )
		{
			job->StartMove();
			if ( job->finished ) continue;
			if ( !job->vehicle->moving ) continue;
		}
		// Das Vehicle bewegen:
		job->DoTheMove();
		game->fDrawMap=true;
	}
	// Alle Attack-Jobs bearbeiten:
	for ( i=0;i<AJobs->Count;i++ )
	{
		bool destroyed;
		cAJobs *aj;
		aj=AJobs->AJobsItems[i];
		// Pr�fen, ob das Vehicle gedreht werden muss:
		if ( aj->vehicle )
		{
			if ( aj->FireDir!=aj->vehicle->dir )
			{
				aj->vehicle->rotating=true;
				aj->vehicle->RotateTo ( aj->FireDir );
				continue;
			}
			else
			{
				aj->vehicle->rotating=false;
			}
		}
		else
		{
			if ( aj->FireDir!=aj->building->dir&&! ( aj->building&&aj->building->data.is_expl_mine ) )
			{
				aj->building->RotateTo ( aj->FireDir );
				continue;
			}
		}
		// Die M�ndungsfeuere Animation abspielen:
		if ( !aj->MuzzlePlayed )
		{
			aj->PlayMuzzle();
			continue;
		}
		destroyed=aj->MakeImpact();
		if ( !aj->MineDetonation )
		{
			aj->MakeClusters();
			if ( aj->vehicle ) aj->vehicle->Attacking=false;else aj->building->Attacking=false;
		}

		if ( aj->Wache&&!destroyed )
		{
			if ( aj->ScrBuilding )
			{
				cBuilding *b;
				b=map->GO[aj->scr].top;
				if ( b&&b->data.shots )
				{
					AddAttackJob ( aj->scr,aj->dest,false,aj->ScrAir,aj->DestAir,aj->ScrBuilding,true );
				}
				else
				{
					cVehicle *v;
					if ( aj->DestAir )
					{
						v=map->GO[aj->dest].plane;
					}
					else
					{
						v=map->GO[aj->dest].vehicle;
					}
					if ( v ) v->InWachRange();
				}
			}
			else if ( aj->ScrAir )
			{
				cVehicle *v;
				v=map->GO[aj->scr].plane;
				if ( v&&v->data.shots )
				{
					AddAttackJob ( aj->scr,aj->dest,false,aj->ScrAir,aj->DestAir,aj->ScrBuilding,true );
				}
				else
				{
					if ( aj->DestAir )
					{
						v=map->GO[aj->dest].plane;
					}
					else
					{
						v=map->GO[aj->dest].vehicle;
					}
					if ( v ) v->InWachRange();
				}
			}
			else
			{
				cVehicle *v;
				v=map->GO[aj->scr].vehicle;
				if ( v&&v->data.shots )
				{
					AddAttackJob ( aj->scr,aj->dest,false,aj->ScrAir,aj->DestAir,aj->ScrBuilding,true );
				}
				else
				{
					if ( aj->DestAir )
					{
						v=map->GO[aj->dest].plane;
					}
					else
					{
						v=map->GO[aj->dest].vehicle;
					}
					if ( v ) v->InWachRange();
				}
			}

		}

		delete aj;
		AJobs->DeleteAJobs ( i );
		i--;
	}
}

// �ndert den Namen eines Vehicles:
void cEngine::ChangeVehicleName ( int posx,int posy,string name,bool override,bool plane )
{
	cVehicle *v;
	if ( plane )
	{
		v=map->GO[posx+posy*map->size].plane;
	}
	else
	{
		v=map->GO[posx+posy*map->size].vehicle;
	}
	if ( v )
	{
		v->name=name;
	}
}

// �ndert den Namen eines Buildings:
void cEngine::ChangeBuildingName ( int posx,int posy,string name,bool override,bool base )
{
	cBuilding *b;
	if ( base )
	{
		b=map->GO[posx+posy*map->size].base;
	}
	else
	{
		b=map->GO[posx+posy*map->size].top;
	}
	if ( b )
	{
		b->name=name;
	}
}

// F�gt einen Bewegungs-Job ein (und gibt eine Referenz zur�ck). Mit ClientMove
// wird ein Client-MoveJob erstellt:
cMJobs *cEngine::AddMoveJob ( int ScrOff,int DestOff,bool ClientMove,bool plane,bool suspended )
{
	cMJobs *job;
	string sMessage;
	if( fstcpip && !fstcpip->bServer && !ClientMove)
	{
		// Client:
		sMessage = iToStr(ScrOff) + "#" + iToStr(DestOff) + "#";
		if ( plane ) sMessage += "1";
		else sMessage += "0";
		fstcpip->FSTcpIpSend( MSG_ADD_MOVEJOB, sMessage.c_str() );
		return NULL;
	}
	else
	{
		// Server/SP:
		job=new cMJobs ( map,ScrOff,DestOff,plane );
		job->ClientMove=ClientMove;
		job->next=mjobs;
		mjobs=job;

		if ( !job->finished )
		{
			if ( suspended )
			{
				job->Suspended=true;
			}
			else
			{
				AddActiveMoveJob ( job );
			}
			if ( job->vehicle->InWachRange() )
			{
				job->finished=true;
				return job;
			}
		}
		else
		{
			return job;
		}

		// Ggf den Bauvorgang abschlie�en:
		if ( job->vehicle->IsBuilding&&!job->finished )
		{
			job->vehicle->IsBuilding=false;
			job->vehicle->BuildOverride=false;
			{
				if ( job->vehicle->data.can_build==BUILD_BIG )
				{
					if ( job->vehicle->PosX!=job->vehicle->BandX||job->vehicle->PosY!=job->vehicle->BandY ) map->GO[job->vehicle->BandX+job->vehicle->BandY*map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX+1||job->vehicle->PosY!=job->vehicle->BandY ) map->GO[job->vehicle->BandX+1+job->vehicle->BandY*map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX+1||job->vehicle->PosY!=job->vehicle->BandY+1 ) map->GO[job->vehicle->BandX+1+ ( job->vehicle->BandY+1 ) *map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX||job->vehicle->PosY!=job->vehicle->BandY+1 ) map->GO[job->vehicle->BandX+ ( job->vehicle->BandY+1 ) *map->size].vehicle=NULL;
					// Das Geb�ude erstellen:
					AddBuilding ( job->vehicle->BandX,job->vehicle->BandY,UnitsData.building+job->vehicle->BuildingTyp,job->vehicle->owner );
				}
				else
				{
					// Das Geb�ude erstellen:
					AddBuilding ( job->vehicle->PosX,job->vehicle->PosY,UnitsData.building+job->vehicle->BuildingTyp,job->vehicle->owner );
				}
			}
		}
		else
			// Oder ggf den Clearvorgang abschlie�en:
			if ( job->vehicle->IsClearing&&!job->finished )
			{
				job->vehicle->IsClearing=false;
				job->vehicle->BuildOverride=false;
				if ( job->vehicle->ClearBig )
				{
					if ( job->vehicle->PosX!=job->vehicle->BandX||job->vehicle->PosY!=job->vehicle->BandY ) map->GO[job->vehicle->BandX+job->vehicle->BandY*map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX+1||job->vehicle->PosY!=job->vehicle->BandY ) map->GO[job->vehicle->BandX+1+job->vehicle->BandY*map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX+1||job->vehicle->PosY!=job->vehicle->BandY+1 ) map->GO[job->vehicle->BandX+1+ ( job->vehicle->BandY+1 ) *map->size].vehicle=NULL;
					if ( job->vehicle->PosX!=job->vehicle->BandX||job->vehicle->PosY!=job->vehicle->BandY+1 ) map->GO[job->vehicle->BandX+ ( job->vehicle->BandY+1 ) *map->size].vehicle=NULL;
				}
				// Den Dirt l�schen:
				job->vehicle->data.cargo+=map->GO[job->vehicle->PosX+job->vehicle->PosY*map->size].base->DirtValue;
				if ( job->vehicle->data.cargo>job->vehicle->data.max_cargo ) job->vehicle->data.cargo=job->vehicle->data.max_cargo;
				if ( job->vehicle==game->SelectedVehicle ) job->vehicle->ShowDetails();
				game->DeleteDirt ( map->GO[job->vehicle->PosX+job->vehicle->PosY*map->size].base );
			}

		// Die Move-Message absetzen:
		if( fstcpip && fstcpip->bServer && job->waypoints && job->waypoints->next )
		{
			sMessage = iToStr(job->waypoints->X+job->waypoints->Y*map->size) + "#" + iToStr(job->waypoints->next->X+job->waypoints->next->Y*map->size) + "#";
			if ( plane ) sMessage += "1";
			else sMessage += "0";
			fstcpip->FSTcpIpSend( MSG_MOVE_TO, sMessage.c_str() );
		}

		if ( job&&ClientMove&&job->vehicle->BuildPath&&job->vehicle->data.can_build==BUILD_SMALL&& ( job->vehicle->BandX!=job->vehicle->PosX||job->vehicle->BandY!=job->vehicle->PosY ) &&!job->finished&&job->vehicle->data.cargo>=job->vehicle->BuildCosts*job->vehicle->BuildRoundsStart )
		{
			job->BuildAtTarget=true;
		}
		return job;
	}
}

// F�gt einen Movejob in die Liste der aktiven Jobs ein:
void cEngine::AddActiveMoveJob ( cMJobs *job )
{
	ActiveMJobs->AddMJobs ( job );
	job->Suspended=false;
}

// Reserviert ein Feld in der GO-Map:
void cEngine::Reservieren ( int x,int y,bool plane )
{
	if ( !plane )
	{
		map->GO[x+y*map->size].reserviert=true;
	}
	else
	{
		map->GO[x+y*map->size].air_reserviert=true;
	}
}

// Setzt ein Vehicle in der GO-Map um:
void cEngine::MoveVehicle ( int FromX,int FromY,int ToX,int ToY,bool override,bool plane )
{
	cVehicle *v;
	if ( !override&& ( ( !plane&& ( map->GO[ToX+ToY*map->size].vehicle||!map->GO[ToX+ToY*map->size].reserviert ) ) || ( plane&& ( map->GO[ToX+ToY*map->size].plane||!map->GO[ToX+ToY*map->size].air_reserviert ) ) ) ) return;
	if ( !plane )
	{
		v=map->GO[FromX+FromY*map->size].vehicle;
	}
	else
	{
		v=map->GO[FromX+FromY*map->size].plane;
	}
	if ( !v ) return;
	if ( !plane )
	{
		map->GO[FromX+FromY*map->size].vehicle=NULL;
		map->GO[ToX+ToY*map->size].vehicle=v;
		map->GO[ToX+ToY*map->size].reserviert=false;
	}
	else
	{
		map->GO[FromX+FromY*map->size].plane=NULL;
		map->GO[ToX+ToY*map->size].plane=v;
		map->GO[ToX+ToY*map->size].air_reserviert=false;
	}
	v->PosX=ToX;
	v->PosY=ToY;
	// Client only:
	if( fstcpip && !fstcpip->bServer && ( v->OffX || v->OffY ) )
	{
		if( v->mjob )
		{
			v->mjob->finished = true;
			v->data.speed += v->mjob->SavedSpeed;
			v->mjob->SavedSpeed = 0;
			if( v->mjob->waypoints && v->mjob->waypoints->next )
			{
				v->DecSpeed( v->mjob->waypoints->next->Costs );
			}
			v->mjob = NULL;      
		}
		v->moving = false;
		v->rotating = false;
		v->WalkFrame = 0;
		if( game->SelectedVehicle == v ) v->ShowDetails();
		v->owner->DoScan();
		MouseMoveCallback( true );
	}
	v->OffX=0;
	v->OffY=0;

	// Ggf Minen zur Detonation bringen:
	if ( !v->data.can_detect_mines&&v->data.can_drive!=DRIVE_AIR&&map->GO[v->PosX+v->PosY*map->size].base&&map->GO[v->PosX+v->PosY*map->size].base->data.is_expl_mine&&map->GO[v->PosX+v->PosY*map->size].base->owner!=v->owner )
	{
		map->GO[v->PosX+v->PosY*map->size].base->Detonate();
		v->moving=false;
		v->WalkFrame=0;
		if ( v->mjob )
		{
			v->mjob->finished=true;
			v->mjob=NULL;
		}
		v->MoveJobActive=false;
	}

	// Server only:
	if( fstcpip && fstcpip->bServer ){
		if ( v->mjob&&v->mjob->waypoints&&v->mjob->waypoints->next&&!v->mjob->Suspended )
		{
			// Move the vehicle and make the next move:
			v->mjob->StartMove();
			if ( v->mjob )
			{
				string sMessage;
				sMessage = iToStr( FromX ) + "#" + iToStr( FromY ) + "#" + iToStr( ToX ) + "#" + iToStr( ToY ) + "#"; 
				if ( plane ) sMessage += "1";
				else sMessage += "0";
				fstcpip->FSTcpIpSend( MSG_MOVE_VEHICLE,sMessage.c_str() );

				if ( !v->mjob->finished )
				{
					sMessage = iToStr( v->mjob->waypoints->X + v->mjob->waypoints->Y * map->size ) + "#" + iToStr( v->mjob->waypoints->next->X + v->mjob->waypoints->next->Y * map->size ) + "#"; 
					if ( plane ) sMessage += "1";
					else sMessage += "0";
					fstcpip->FSTcpIpSend( MSG_MOVE_TO,sMessage.c_str() );
				}
			}
		}
		else
		{
			// Just move the vehicle:
			unsigned char msg[36];
			msg[0]='#';
			msg[1]=36;
			msg[2]=MSG_MOVE_VEHICLE;
			( ( int* ) ( msg+3 ) ) [0]=FromX;
			( ( int* ) ( msg+3 ) ) [1]=FromY;
			( ( int* ) ( msg+3 ) ) [2]=ToX;
			( ( int* ) ( msg+3 ) ) [3]=ToY;
			msg[35]=v->data.can_drive==DRIVE_AIR;
		}
	}
	// Ggf nach Rohstoffen suchen:
	if ( v->data.can_survey )
	{
		v->DoSurvey();
	}
	// Ggf beschie�en lassen:
	v->InWachRange();
	// Ggf Minen suchen:
	if ( v->data.can_detect_mines&&v->owner==game->ActivePlayer )
	{
		v->DetectMines();
	}
	// Ggf Minen legen/r�umen:
	if ( v->data.can_lay_mines&&v->owner==game->ActivePlayer )
	{
		if ( v->LayMines )
		{
			v->LayMine();
			if ( v->data.cargo<=0 )
			{
				v->LayMines=false;
			}
		}
		else if ( v->ClearMines )
		{
			v->ClearMine();
			if ( v->data.cargo>=v->data.max_cargo )
			{
				v->ClearMines=false;
			}
		}
	}
	// Ggf Meldung machen:
	if ( v->owner!=game->ActivePlayer&&game->ActivePlayer->ScanMap[ToX+ToY*map->size]&&!game->ActivePlayer->ScanMap[FromX+FromY*map->size]&&v->detected )
	{
		char str[50];
		sprintf ( str,"%s %s",v->name.c_str(), lngPack.Translate( "Text~Comp~Detected").c_str() );
		game->AddCoords ( str,v->PosX,v->PosY );
		if ( random ( 2,0 ) ==0 ) PlayVoice ( VoiceData.VOIDetected1 );
		else PlayVoice ( VoiceData.VOIDetected2 );
	}
	return;
}

// F�gt ein Vehicle ein:
void cEngine::AddVehicle ( int posx,int posy,sVehicle *v,cPlayer *p,bool init,bool engine_call )
{
	cVehicle *n;
	// Das Fahrzeug erzeugen:
	n=p->AddVehicle ( posx,posy,v );
	// Das Fahrzeug platzieren:
	if ( n->data.can_drive!=DRIVE_AIR )
	{
		int off=posx+map->size*posy;
		if ( map->GO[off].vehicle==NULL ) map->GO[off].vehicle=n;
	}
	else
	{
		int off=posx+map->size*posy;
		if ( map->GO[off].plane==NULL ) map->GO[off].plane=n;
	}
	// Startup:
	if ( !init ) n->StartUp=10;
	// Ggf mit dem Gutachter scannen:
	if ( n->data.can_survey )
	{
		n->DoSurvey();
	}
	if ( !init ) n->InWachRange();
}

// F�gt ein Building ein:
void cEngine::AddBuilding ( int posx,int posy,sBuilding *b,cPlayer *p,bool init )
{
	cBuilding *n;
	int off;
	// Das Building erzeugen:
	n = p->AddBuilding ( posx,posy,b );
	// Das Building platzieren:
	off=posx+map->size*posy;
	if ( n->data.is_base )
	{
		if(map->GO[off].base)
		{
			map->GO[off].subbase = map->GO[off].base;
			map->GO[off].base = n;
		}
		else
		{
			map->GO[off].base = n;
		}
	}
	else
	{
#define DELETE_OBJ(a,b) if(a){if(a->prev){a->prev->next=a->next;if(a->next)a->next->prev=a->prev;}else{a->owner->b=a->next;if(a->next)a->next->prev=NULL;}if(a->base)a->base->DeleteBuilding(a);delete a;}
		if ( n->data.is_big )
		{
			DELETE_OBJ ( map->GO[off].top,BuildingList )
			map->GO[off].top=n;
			if ( map->GO[off].base&&(map->GO[off].base->data.is_road || map->GO[off].base->data.is_expl_mine) )
			{
				DELETE_OBJ ( map->GO[off].base,BuildingList )
				map->GO[off].base = NULL;
			}
			off++;
			DELETE_OBJ ( map->GO[off].top,BuildingList )
			map->GO[off].top=n;
			if ( map->GO[off].base&&(map->GO[off].base->data.is_road || map->GO[off].base->data.is_expl_mine) )
			{
				DELETE_OBJ ( map->GO[off].base,BuildingList )
				map->GO[off].base=NULL;
			}
			off+=map->size;
			DELETE_OBJ ( map->GO[off].top,BuildingList )
			map->GO[off].top=n;
			if ( map->GO[off].base&&(map->GO[off].base->data.is_road || map->GO[off].base->data.is_expl_mine) )
			{
				DELETE_OBJ ( map->GO[off].base,BuildingList )
				map->GO[off].base=NULL;
			}
			off--;
			DELETE_OBJ ( map->GO[off].top,BuildingList )
			map->GO[off].top=n;
			if ( map->GO[off].base&&(map->GO[off].base->data.is_road || map->GO[off].base->data.is_expl_mine) )
			{
				DELETE_OBJ ( map->GO[off].base,BuildingList )
				map->GO[off].base=NULL;
			}
		}
		else
		{
			DELETE_OBJ ( map->GO[off].top,BuildingList )
			map->GO[off].top=n;
			if ( !n->data.is_connector&&map->GO[off].base&&(map->GO[off].base->data.is_road || map->GO[off].base->data.is_expl_mine) )
			{
				DELETE_OBJ ( map->GO[off].base,BuildingList )
				map->GO[off].base=NULL;
			}
		}
#undef DELETE_OBJ
	}
	if ( !init ) n->StartUp=10;
	// Das Geb�ude in die Basis integrieren:
	p->base->AddBuilding ( n );

}

// Wird aufgerufen, wenn ein Objekt zerst�rt werden soll:
void cEngine::DestroyObject ( int off,bool air )
{
	cVehicle *vehicle=NULL;
	cBuilding *building=NULL;
	// Das Objekt zerst�ren:
	if ( air ) vehicle=map->GO[off].plane;
	else
	{
		vehicle=map->GO[off].vehicle;
		if ( map->GO[off].top ) building=map->GO[off].top;
		else building=map->GO[off].base;
	}
	if ( vehicle )
	{
		if ( game->SelectedVehicle&&game->SelectedVehicle==vehicle )
		{
			vehicle->Deselct();
			game->SelectedVehicle=NULL;
		}
		if ( vehicle->mjob )
		{
			vehicle->mjob->finished=true;
			vehicle->mjob->vehicle=NULL;
		}
		if ( vehicle->prev )
		{
			cVehicle *v;
			v=vehicle->prev;
			v->next=vehicle->next;
			if ( v->next ) v->next->prev=v;
		}
		else
		{
			vehicle->owner->VehicleList=vehicle->next;
			if ( vehicle->next ) vehicle->next->prev=NULL;
		}
		// Annimation abspielen:
		{
			int nr;
			nr=random ( 3,0 );
			if ( nr==0 )
			{
				game->AddFX ( fxExploSmall0,vehicle->PosX*64+32,vehicle->PosY*64+32,0 );
			}
			else if ( nr==1 )
			{
				game->AddFX ( fxExploSmall1,vehicle->PosX*64+32,vehicle->PosY*64+32,0 );
			}
			else
			{
				game->AddFX ( fxExploSmall2,vehicle->PosX*64+32,vehicle->PosY*64+32,0 );
			}
		}
		// fahrzeug l�schen:
		if ( air ) map->GO[off].plane=NULL;
		else
		{
			if ( vehicle->IsBuilding&&vehicle->data.can_build==BUILD_BIG )
			{
				map->GO[vehicle->BandX+vehicle->BandY*map->size].vehicle=NULL;
				map->GO[vehicle->BandX+1+vehicle->BandY*map->size].vehicle=NULL;
				map->GO[vehicle->BandX+1+ ( vehicle->BandY+1 ) *map->size].vehicle=NULL;
				map->GO[vehicle->BandX+ ( vehicle->BandY+1 ) *map->size].vehicle=NULL;
			}
			else
			{
				map->GO[off].vehicle=NULL;
			}

			// Ggf Mine l�schen:
			if ( map->GO[off].base&&map->GO[off].base->owner )
			{
				cBuilding *building=map->GO[off].base;
				if ( building==game->SelectedBuilding ) building->Deselct();
				if ( building->prev )
				{
					cBuilding *pb;
					pb=building->prev;
					pb->next=building->next;
					if ( pb->next ) pb->next->prev=pb;
				}
				else
				{
					building->owner->BuildingList=building->next;
					if ( building->next ) building->next->prev=NULL;
				}
				map->GO[off].base=NULL;
				delete building;
			}

			// �berreste erstellen:
			if ( vehicle->data.is_human )
			{
				// Leiche erstellen:
				game->AddFX ( fxCorpse,vehicle->PosX*64,vehicle->PosY*64,0 );
			}
			else
			{
				// Dirt erstellen:
				if ( !map->GO[off].base ) game->AddDirt ( vehicle->PosX,vehicle->PosY,vehicle->data.costs/2,false );
			}
		}
		vehicle->owner->DoScan();
		delete vehicle;
		MouseMoveCallback ( true );
	}
	else if ( building&&building->owner )
	{
		if ( game->SelectedBuilding&&game->SelectedBuilding==building )
		{
			building->Deselct();
			game->SelectedBuilding=NULL;
		}
		if ( building->prev )
		{
			cBuilding *b;
			b=building->prev;
			b->next=building->next;
			if ( b->next ) b->next->prev=b;
		}
		else
		{
			building->owner->BuildingList=building->next;
			if ( building->next ) building->next->prev=NULL;
		}
		// Die Basis neu berechnen:
		if ( building->owner->base )
		{
			building->owner->base->DeleteBuilding ( building );
		}
		// Annimation abspielen:
		if ( !building->data.is_big )
		{
			int nr;
			nr=random ( 3,0 );
			if ( nr==0 )
			{
				game->AddFX ( fxExploSmall0,building->PosX*64+32,building->PosY*64+32,0 );
			}
			else if ( nr==1 )
			{
				game->AddFX ( fxExploSmall1,building->PosX*64+32,building->PosY*64+32,0 );
			}
			else
			{
				game->AddFX ( fxExploSmall2,building->PosX*64+32,building->PosY*64+32,0 );
			}
		}
		else
		{
			int nr;
			nr=random ( 4,0 );
			if ( nr==0 )
			{
				game->AddFX ( fxExploBig0,building->PosX*64+64,building->PosY*64+64,0 );
			}
			else if ( nr==1 )
			{
				game->AddFX ( fxExploBig1,building->PosX*64+64,building->PosY*64+64,0 );
			}
			else if ( nr==2 )
			{
				game->AddFX ( fxExploBig2,building->PosX*64+64,building->PosY*64+64,0 );
			}
			else
			{
				game->AddFX ( fxExploBig2,building->PosX*64+64,building->PosY*64+64,0 );
			}
		}
		// Building l�schen:
		if ( map->GO[off].top )
		{
			if ( building->data.is_big )
			{
				map->GO[off].top=NULL;
				map->GO[off+1].top=NULL;
				map->GO[off+1+map->size].top=NULL;
				map->GO[off+map->size].top=NULL;
				// Dirt erstellen:
				if ( !map->GO[off].base ) game->AddDirt ( building->PosX,building->PosY,building->data.costs/2,true );
			}
			else
			{
				map->GO[off].top=NULL;
				// Dirt erstellen:
				if ( !map->GO[off].base ) game->AddDirt ( building->PosX,building->PosY,building->data.costs/2,false );
			}
		}
		else
		{
			if(map->GO[off].subbase)
			{
				map->GO[off].base = map->GO[off].subbase;
				map->GO[off].subbase = NULL;
			}
			else
			{
				map->GO[off].base=NULL;
			}
			// Dirt erstellen:
			if ( !map->GO[off].base ) game->AddDirt ( building->PosX,building->PosY,building->data.costs/2,false );
		}
		building->owner->DoScan();
		delete building;
		MouseMoveCallback ( true );
	}
	game->fDrawMMap=true;
}

// Pr�ft, ob Jemand besiegt wurde:
void cEngine::CheckDefeat ( void )
{
	cPlayer *p;
	int i;
	string sTmpString;

	for ( i=0;i<game->PlayerList->Count;i++ )
	{
		p=game->PlayerList->PlayerItems[i];
		if ( p->IsDefeated() )
		{
			sTmpString = lngPack.Translate( "Text~Game_MP~Title_Player") + " ";
			sTmpString += p->name + " ";
			sTmpString += lngPack.Translate( "Text~Comp~Defeated") + "!";
			game->AddMessage ( (char *)sTmpString.c_str() );

			if ( game->HotSeat )
			{
				if ( p==game->ActivePlayer )
				{
					game->HotSeatPlayer++;
					if ( game->HotSeatPlayer>=game->PlayerList->Count ) game->HotSeatPlayer=0;
					game->ActivePlayer->HotHud=* ( game->hud );
					game->ActivePlayer=game->PlayerList->PlayerItems[game->HotSeatPlayer];

					delete p;
					game->PlayerList->DeletePlayer ( i );
					i--;
				}
				else
				{
					delete p;
					game->PlayerList->DeletePlayer ( i );
					i--;
				}
			}
		}
	}
}

// F�gt einen Reporteintrag in die entsprechende Liste ein:
void cEngine::AddReport ( string name,bool vehicle )
{
	struct sReport *r;
	int i;
	if ( vehicle )
	{
		for ( i=0;i<game->ActivePlayer->ReportVehicles->Count;i++ )
		{
			r=game->ActivePlayer->ReportVehicles->ReportItems[i];
			if ( !r->name.compare ( name ) )
			{
				r->anz++;
				return;
			}
		}
		r=new sReport;
		r->name=name;
		r->anz=1;
		game->ActivePlayer->ReportVehicles->AddReport ( r );
	}
	else
	{
		for ( i=0;i<game->ActivePlayer->ReportBuildings->Count;i++ )
		{
			r=game->ActivePlayer->ReportBuildings->ReportItems[i];
			if ( !r->name.compare ( name ) )
			{
				r->anz++;
				return;
			}
		}
		r=new sReport;
		r->name=name;
		r->anz=1;
		game->ActivePlayer->ReportBuildings->AddReport ( r );
	}
}

// Zeigt einen Report zum Rundenbeginn an:
void cEngine::MakeRundenstartReport ( void )
{
	struct sReport *r;
	string Report;
	string stmp;
	char sztmp[32];
	char str[100];
	int anz;
	sprintf ( str,"%s %d",lngPack.Translate( "Text~Comp~Turn_Start").c_str(), game->Runde );
	game->AddMessage ( str );

	anz=0;
	Report="";
	while ( game->ActivePlayer->ReportBuildings->Count )
	{
		r=game->ActivePlayer->ReportBuildings->ReportItems[0];
		if ( anz ) Report+=", ";
		anz+=r->anz;
		sprintf ( sztmp,"%d",r->anz ); stmp = sztmp; stmp += " "; stmp += r->name;
		Report += r->anz>1?stmp:r->name;
		delete r;
		game->ActivePlayer->ReportBuildings->DeleteReport ( 0 );
	}
	while ( game->ActivePlayer->ReportVehicles->Count )
	{
		r=game->ActivePlayer->ReportVehicles->ReportItems[0];
		if ( anz ) Report+=", ";
		anz+=r->anz;
		sprintf ( sztmp,"%d",r->anz ); stmp = sztmp; stmp += " "; stmp += r->name;
		Report+=r->anz>1?stmp:r->name;
		delete r;
		game->ActivePlayer->ReportVehicles->DeleteReport ( 0 );
	}

	if ( anz==0 )
	{
		if ( !game->ActivePlayer->ReportForschungFinished ) PlayVoice ( VoiceData.VOIStartNone );
		game->ActivePlayer->ReportForschungFinished=false;
		return;
	}
	if ( anz==1 )
	{
		Report+=" "+lngPack.Translate( "Text~Comp~Finished") +".";
		if ( !game->ActivePlayer->ReportForschungFinished ) PlayVoice ( VoiceData.VOIStartOne );
	}
	else
	{
		Report+=" "+lngPack.Translate( "Text~Comp~Finished2") +".";
		if ( !game->ActivePlayer->ReportForschungFinished ) PlayVoice ( VoiceData.VOIStartMore );
	}
	game->ActivePlayer->ReportForschungFinished=false;
	game->AddMessage ( ( char * ) Report.c_str() );
}

// Bereitet das Logging vor:
void cEngine::StartLog ( void )
{
	if ( LogFile ) SDL_RWclose ( LogFile );
	LogFile=SDL_RWFromFile ( "engine.log","w" );
	if ( LogHistory )
	{
		while ( LogHistory->Count )
		{
//      delete LogHistory->Items[0];
			LogHistory->Delete ( 0 );
		}
		delete LogHistory;
	}
	LogHistory=new TList;
}

// Beendet das Logging vor:
void cEngine::StopLog ( void )
{
	if ( !LogFile ) return;
	SDL_RWclose ( LogFile );
	LogFile=NULL;
	if ( LogHistory )
	{
		while ( LogHistory->Count )
		{
//      delete (AnsiString*)(LogHistory->Items[0]);
			LogHistory->Delete ( 0 );
		}
		delete LogHistory;
	}
	LogHistory=NULL;
}

// Loggt eine Nachricht:
void cEngine::LogMessage ( string msg )
{
	if ( !LogFile ) return;

	if ( game->ShowLog )
	{
		string str;
		if ( LogHistory->Count>=54 )
		{
//      delete LogHistory->Items[0];
			LogHistory->Delete ( 0 );
		}
		LogHistory->Add ( str );
		str=msg;
	}

	SDL_RWwrite ( LogFile,msg.c_str(),1, ( int ) msg.length() );
	SDL_RWwrite ( LogFile,"\n",1,1 );

	SDL_FreeRW ( LogFile );
}

// �ndert den Namen eines Spielers:
void cEngine::ChangePlayerName ( string name )
{
	/*unsigned char msg[200];
	int len;
	if(!fstcpip)return;
	len=name.Length()+8;
	msg[0]='#';
	msg[1]=len;
	msg[2]=MSG_CHANGE_PLAYER_NAME;
	((int*)(msg+3))[0]=game->ActivePlayer->Nr;
	strcpy(msg+7,name.c_str());
	fstcpip->Send(msg,len);*/
}

// Wird aufgerufen, wenn ein Spieler Ende dr�ckt:
void cEngine::EndePressed ( int PlayerNr )
{
	EndeCount++;
}

// Pr�ft das Rundenende:
void cEngine::CheckEnde ( void )
{
	// SP:
	if ( EndeCount ) Rundenende();
}

// F�hrt alle Rundenendenaktionen durch:
void cEngine::Rundenende ( void )
{
	int i;
	game->WantToEnd=false;
	EndeCount=0;
	if ( !game->PlayRounds )
	{
		game->hud->Ende=false;
		game->hud->EndeButton ( false );
	}
	game->Runde++;
	game->hud->ShowRunde();

	// Alle Buildings wieder aufladen:
	for ( i=0;i<game->PlayerList->Count;i++ )
	{
		bool ShieldChaned;
		cBuilding *b;
		cPlayer *p;
		p=game->PlayerList->PlayerItems[i];

		ShieldChaned=false;
		b=p->BuildingList;
		while ( b )
		{
			if ( b->Disabled )
			{
				b->Disabled--;
				if ( b->Disabled )
				{
					b=b->next;
					continue;
				}
			}
			if ( b->data.can_attack&&!game->HotSeat&&!game->PlayRounds ) b->RefreshData();
			if ( b->IsWorking&&b->data.max_shield&&b->data.shield<b->data.max_shield )
			{
				b->data.shield+=10;
				if ( b->data.shield>b->data.max_shield ) b->data.shield=b->data.max_shield;
				ShieldChaned=true;
			}
			b=b->next;
		}
		if ( ShieldChaned )
		{
			p->CalcShields();
		}
	}

	// Alle Vehicles wieder aufladen:
	for ( i=0;i<game->PlayerList->Count;i++ )
	{
		cVehicle *v;
		cPlayer *p;
		p=game->PlayerList->PlayerItems[i];

		v=p->VehicleList;
		while ( v )
		{
			if ( v->detection_override ) v->detection_override=false;
			if ( v->Disabled )
			{
				v->Disabled--;
				if ( v->Disabled )
				{
					v=v->next;
					continue;
				}
			}

			if ( !game->HotSeat&&!game->PlayRounds ) v->RefreshData();
			if ( v->mjob ) v->mjob->EndForNow=false;

			v=v->next;
		}
	}
	// Gun'em down:
	{
		for ( i=0;i<game->PlayerList->Count;i++ )
		{
			cVehicle *v;
			cPlayer *p;
			p=game->PlayerList->PlayerItems[i];

			v=p->VehicleList;
			while ( v )
			{
				v->InWachRange();
				v=v->next;
			}
		}
	}

	// Rohstoffe produzieren:
	game->ActivePlayer->base->Rundenende();

	// Forschen:
	game->ActivePlayer->DoResearch();

	if ( game->SelectedVehicle )
	{
		game->SelectedVehicle->ShowDetails();
	}
	else if ( game->SelectedBuilding )
	{
		game->SelectedBuilding->ShowDetails();
	}
	// M�ll einsammeln:
	CollectTrash();

	// Ggf Autosave machen:
	if ( SettingsData.bAutoSave )
	{
		game->MakeAutosave();
	}

	CheckDefeat();

	// Meldung zum Rundenstart:
	MakeRundenstartReport();
}

// F�hrt alle Aktionen am Rundenende aus und gibt true zur�ck, wenn es was
// zu tun gab:
bool cEngine::DoEndActions ( void )
{
	cVehicle *v;
	bool todo=false;
	v=game->ActivePlayer->VehicleList;
	while ( v )
	{
		{
			// SP/Server:
			if ( v->mjob&&v->data.speed )
			{
				v->mjob->CalcNextDir();
				AddActiveMoveJob ( v->mjob );
				todo=true;
			}
		}
		v=v->next;
	}
	return todo;
}

// Pr�ft ob sich noch Fahrzeuge bewegen:
bool cEngine::CheckVehiclesMoving ( bool WantToEnd )
{
	return ActiveMJobs->Count>0;
}

// Sammelt den gesammten M�ll ein:
void cEngine::CollectTrash ( void )
{
	cMJobs *j;
	int i;
	// Alle Movejobs einsammeln:
	while ( mjobs&&mjobs->finished )
	{
		j=mjobs->next;
		for ( i=0;i<ActiveMJobs->Count;i++ )
		{
			if ( ActiveMJobs->MJobsItems[i] == mjobs )
			{
				ActiveMJobs->DeleteMJobs ( i );
				break;
			}
		}
		if ( mjobs->vehicle )
		{
			try
			{
				mjobs->vehicle->mjob=NULL;
			}
			catch ( ... )
			{}}
		delete mjobs;
		mjobs=j;
	}
	j=mjobs;
	while ( j&&j->next )
	{
		if ( j->next->finished )
		{
			cMJobs *n;
			n=j->next->next;
			if ( j->next->vehicle&&j->next->vehicle->mjob==j->next ) j->next->vehicle->mjob=NULL;
			delete j->next;
			j->next=n;
		}
		else
		{
			j=j->next;
		}
	}
}

// F�hrt einen Angriff aus:
void cEngine::AddAttackJob ( int ScrOff,int DestOff,bool override,bool ScrAir,bool DestAir,bool ScrBuilding,bool Wache )
{
	if ( ScrOff==DestOff )
	{
		if ( !ScrBuilding||map->GO[ScrOff].base==NULL||map->GO[ScrOff].base->owner==NULL||!map->GO[ScrOff].base->data.is_expl_mine ) return;
	}
	cAJobs *aj;
	aj=new cAJobs ( map,ScrOff,DestOff,ScrAir,DestAir,ScrBuilding,Wache );
	AJobs->AddAJobs ( aj );
}

// Empf�ngt eine Nachricht aus dem Netzwerk:
void cEngine::HandleGameMessages()
{
	cNetMessage *msg;
	string sMsgString;
	for ( int i=0;i<fstcpip->NetMessageList->iCount;i++ )
	{
		msg = (cNetMessage *) fstcpip->NetMessageList->Items[i];
		sMsgString = ( char * ) msg->msg;
		switch( msg->typ )
		{
			// Chatmessages:
			case MSG_CHAT:
			{
				game->AddMessage( sMsgString );
				PlayFX( SoundData.SNDChat );
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Add movejob:
			case MSG_ADD_MOVEJOB:
			{
				cMJobs *job;
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				job = AddMoveJob( atoi( Strings->Items[0].c_str() ),atoi( Strings->Items[1].c_str() ),false,atoi( Strings->Items[2].c_str() ) );
				// Check if path is barred:
				if(job->finished)
				{
					fstcpip->FSTcpIpSend(MSG_NO_PATH,"");
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Move vehicle:
			case MSG_MOVE_VEHICLE:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				MoveVehicle ( atoi( Strings->Items[0].c_str() ),atoi( Strings->Items[1].c_str() ),atoi( Strings->Items[2].c_str() ),atoi( Strings->Items[3].c_str() ),true,atoi( Strings->Items[4].c_str() ) );
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Move vehicle for a field:
			case MSG_MOVE_TO:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				AddMoveJob( atoi( Strings->Items[0].c_str() ),atoi( Strings->Items[1].c_str() ),true,atoi( Strings->Items[2].c_str() ));
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Path is barred:
			case MSG_NO_PATH:
			{
				if( random( 1,0 ) )
				{
					PlayVoice(VoiceData.VOINoPath1);
				}
				else
				{
					PlayVoice(VoiceData.VOINoPath2);
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// End of a movejobs:
			case MSG_END_MOVE:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				cVehicle *v;
				if( atoi ( Strings->Items[1].c_str() ) == 0 )
				{
					v=map->GO[atoi ( Strings->Items[0].c_str() )].vehicle;
				}
				else
				{
					v=map->GO[atoi ( Strings->Items[0].c_str() )].plane;        
				}
				if( v )
				{
					v->MoveJobActive=false;
					if( v == game->SelectedVehicle )
					{
						StopFXLoop( game->ObjectStream );
						if( map->IsWater( v->PosX + v->PosY * map->size) && v->data.can_drive != DRIVE_AIR )
						{
							PlayFX( v->typ->StopWater );
						}
						else
						{
							PlayFX( v->typ->Stop );
						}
						game->ObjectStream = v->PlayStram();
					}
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// �ndert den Namen eines Vehicled:
			case MSG_CHANGE_VEH_NAME:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ende eines Movejobs f�r diese Runde:
			case MSG_END_MOVE_FOR_NOW:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				cVehicle *v;
				if( atoi( Strings->Items[2].c_str() ) == 0 )
				{
					v = map->GO[atoi( Strings->Items[0].c_str() ) ].vehicle;
				}
				else
				{
					v = map->GO[atoi( Strings->Items[0].c_str() ) ].plane;
				}
				if( !v ) break;
				if( v->owner == game->ActivePlayer )
				{
					AddMoveJob(v->PosX + v->PosY * map->size,atoi( Strings->Items[1].c_str() ),true,atoi( Strings->Items[2].c_str() ) );
				}
				v->MoveJobActive = false;
				if( v == game->SelectedVehicle )
				{
					StopFXLoop( game->ObjectStream );
					if( map->IsWater( v->PosX + v->PosY * map->size ) && v->data.can_drive != DRIVE_AIR )
					{
						PlayFX( v->typ->StopWater );
					}
					else
					{
						PlayFX( v->typ->Stop );
					}
					game->ObjectStream = v->PlayStram();
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// �ndert den Namen eines Spielers:
			case MSG_CHANGE_PLAYER_NAME:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Benachrichtigung �ber ein gedr�cktes Ende:
			case MSG_ENDE_PRESSED:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Benachrichtigung �ber den Abbruch eines MJobs:
			case MSG_MJOB_STOP:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				cVehicle *v;
				if( atoi( Strings->Items[1].c_str() ) == 0)
				{
					v = map->GO[atoi( Strings->Items[0].c_str() )].vehicle;
				}else{
					v = map->GO[atoi( Strings->Items[0].c_str() )].plane;        
				}
				if( v && v->mjob )
				{
					v->mjob->finished = true;
					v->mjob = NULL;
					v->MoveJobActive = false;
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ein neuer Attackjob:
			case MSG_ADD_ATTACKJOB:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ein Objekt zerst�ren:
			case MSG_DESTROY_OBJECT:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Execute a movejob:
			case MSG_ERLEDIGEN:
			{
				TList *Strings;
				Strings = SplitMessage ( sMsgString );
				cVehicle *v;
				if( atoi( Strings->Items[1].c_str() ) == 0)
				{
					v=map->GO[atoi( Strings->Items[0].c_str() )].vehicle;
				}else
				{
					v=map->GO[atoi( Strings->Items[0].c_str() )].plane;        
				}
				if( v && v->mjob )
				{
					v->mjob->CalcNextDir();
					AddActiveMoveJob(v->mjob);
					if( fstcpip->bServer && v->mjob->waypoints && v->mjob->waypoints->next )
					{
						string sMessage;
						sMessage = iToStr( v->mjob->waypoints->X + v->mjob->waypoints->Y * map->size ) + "#" + iToStr( v->mjob->waypoints->next->X + v->mjob->waypoints->next->Y * map->size ) + "#";
						if ( v->mjob->plane ) sMessage += "1";
						else sMessage += "0";
						fstcpip->FSTcpIpSend( MSG_MOVE_TO, sMessage.c_str() );
					}
				}
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Meldet, dass Speed gesaved wurde:
			case MSG_SAVED_SPEED:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// �ndert den Namen eines Buildings:
			case MSG_CHANGE_BUI_NAME:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Startet einen Bauvorgang eines Geb�udes:
			case MSG_START_BUILD:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Stopt den Bauvorgang/R�umen eines Geb�udes:
			case MSG_STOP_BUILD:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// F�gt ein neues Geb�ude ein:
			case MSG_ADD_BUILDING:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Die Konstruktion eines gro�en Geb�udes starten:
			case MSG_START_BUILD_BIG:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Den Constructor umsetzen:
			case MSG_RESET_CONSTRUCTOR:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Startet das R�umen eines Feldes:
			case MSG_START_CLEAR:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Vehicle einladen:
			case MSG_STORE_VEHICLE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Vehicle ausladen:
			case MSG_ACTIVATE_VEHICLE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ein Building starten:
			case MSG_START_WORK:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ein Building stoppen:
			case MSG_STOP_WORK:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ein Vehicle einf�gen:
			case MSG_ADD_VEHICLE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Etwas reparieren:
			case MSG_REPAIR:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Etwas aufladen:
			case MSG_RELOAD:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Den Wachstatus von etwas �ndern:
			case MSG_WACHE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// R�umt eine Mine:
			case MSG_CLEAR_MINE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Upgrade eines Spielers:
			case MSG_UPGRADE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Furschung abgeschlossen:
			case MSG_RESEARCH:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Geb�ude verbessern:
			case MSG_UPDATE_BUILDING:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Einen Fehler eines Commandos melden:
			case MSG_COMMANDO_MISTAKE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Eine Commandooperation durchf�hren:
			case MSG_COMMANDO_SUCCESS:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Aufforderung zur Synchronisation:
			case MSG_START_SYNC:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Sync Player:
			case MSG_SYNC_PLAYER:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Sync Vehicle:
			case MSG_SYNC_VEHICLE:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Sync Building:
			case MSG_SYNC_BUILDING:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Updated ein gestoredtes Vehicle:
			case MSG_UPDATE_STORED:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Bericht �ber Abschluss der RundenendeActions:
			case MSG_REPORT_R_E_A:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Ping:
			case MSG_PING:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Pong:
			case MSG_PONG:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Niederlage des Host:
			case MSG_HOST_DEFEAT:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// Niederlage eines Spielers:
			case MSG_PLAYER_DEFEAT:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			// N�chster Spieler im Runden-Spiel-Modus:
			case MSG_PLAY_ROUNDS_NEXT:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
			default:
			{
				fstcpip->NetMessageList->Delete ( i );
				break;
			}
		}
	}
}

// Sends a chat-message:
void cEngine::SendChatMessage(const char *str){
	if(fstcpip){
		string sChatMessage = str;
		if(sChatMessage.length() > 255)
		{
			sChatMessage.erase( 255 );
		}
		fstcpip->FSTcpIpSend(MSG_CHAT, sChatMessage.c_str());
	}
	game->AddMessage(str);
	PlayFX(SoundData.SNDChat);
}

TList* cEngine::SplitMessage ( string sMsg )
{
	TList *Strings;
	Strings = new TList;
	int npos=0;
	for ( int i=0; npos != string::npos; i++ )
	{
		Strings->Items[i] = sMsg.substr ( npos, ( sMsg.find ( "#",npos )-npos ) );
		npos = ( int ) sMsg.find ( "#",npos );
		if ( npos != string::npos )
			npos++;
	}
	return Strings;
}