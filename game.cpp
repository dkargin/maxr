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
#include "game.h"
#include "main.h"
#include "mouse.h"
#include "fonts.h"
#include "keyinp.h"
#include "menu.h"
#include "keys.h"
#include "dialog.h"
#include "log.h"
#include "files.h"
#include <sstream>

// Funktionen der Game-Klasse ////////////////////////////////////////////////
cGame::cGame ( cFSTcpIp *fstcpip, cMap *map )
{
	this->map=map;
	PlayerCheat="";
	TimerID=SDL_AddTimer ( 50,Timer,NULL );
	TimerTime=0;
	Frame=0;
	SelectedVehicle=NULL;
	SelectedBuilding=NULL;
	ObjectStream=-1;
	OverObject=NULL;
	BlinkColor=0xFFFFFF;
	FLC=NULL;
	FLCname="";
	video=NULL;
	hud=new cHud;
	engine=new cEngine ( map, fstcpip );
	HelpActive=false;
	ChangeObjectName=false;
	ChatInput=false;
	messages=new TList;
	DebugFPS=false;
	DebugCom=false;
	DebugBase=false;
	DebugWache=false;
	DebugFX=false;
	DebugTrace=false;
	DebugLog=false;
	ShowLog=false;
	Defeated=false;
	PlayRounds=false;
	ActiveRoundPlayerNr=0;
	MsgCoordsX=-1;
	MsgCoordsY=-1;
	HotSeatPlayer=0;
	//Auskommentiert bei Main�berarbeitung
	//for(DebugIndex=0;DebugIndex<DB_COM_BUFFER;DebugIndex++){
	//  DebugComSend[DebugIndex]=0;
	//  DebugComRead[DebugIndex]=0;
	//}
	//TODO:
	ComAvgSend=0;
	ComAvgRead=0;
	Runde=1;
	WantToEnd=false;
	FXList=new TList;
	FXListBottom=new TList;
	UpShowTank=true;
	UpShowPlane=true;
	UpShowShip=true;
	UpShowBuild=true;
	UpShowTNT=false;
	AlienTech=false;
	HotSeat=false;
	End=false;

	SetWind ( random ( 360,0 ) );
}

cGame::~cGame ( void )
{
	hud->Zoom=64;
	hud->ScaleSurfaces();
	SDL_RemoveTimer ( TimerID );
	StopFXLoop ( ObjectStream );
	while ( messages->Count )
	{
		sMessage *msg;
		msg = messages->MessageItems[0];
		free ( msg->msg );
		free ( msg );
		messages->DeleteMessage ( 0 );
	}
	if ( FLC ) FLI_Close ( FLC );
	delete hud;
	delete engine;
	delete messages;
	while ( FXList->Count )
	{
		sFX *ptr;
		ptr=FXList->FXItems[0];
		if ( ptr->typ==fxRocket )
		{
			sFXRocketInfos *p2;
			p2= ( sFXRocketInfos* ) ( ptr->param );
			delete p2;
		}
		else if ( ptr->typ==fxDarkSmoke )
		{
			sFXDarkSmoke *p2;
			p2= ( sFXDarkSmoke* ) ( ptr->param );
			delete p2;
		}
		delete ptr;
		FXList->DeleteFX ( 0 );
	}
	delete FXList;
	while ( FXListBottom->Count )
	{
		sFX *ptr;
		ptr=FXListBottom->FXItems[0];
		if ( ptr->typ==fxTorpedo )
		{
			sFXRocketInfos *p2;
			p2= ( sFXRocketInfos* ) ( ptr->param );
			delete p2;
		}
		else if ( ptr->typ==fxTracks )
		{
			sFXTracks *p2;
			p2= ( sFXTracks* ) ( ptr->param );
			delete p2;
		}
		delete ptr;
		FXListBottom->DeleteFX ( 0 );
	}
	delete FXListBottom;
	/*while(DirtList){
	  cBuilding *ptr;
	  ptr=DirtList->next;
	  delete DirtList;
	  DirtList=ptr;
	}*/
}

// Init //////////////////////////////////////////////////////////////////////
// Initialisiert das Spiel:
// Player - Liste der Spieler
void cGame::Init ( TList *Player,int APNo )
{
	PlayerList=Player;
	fDrawHud=true;
	fDrawMap=true;
	fDraw=true;
	fDrawMMap=true;
	ActivePlayer=PlayerList->PlayerItems[APNo];
}

// Run ///////////////////////////////////////////////////////////////////////
// Startet das Spiel:
void cGame::Run ( void )
{
	int LastMouseX,LastMouseY,i,DebugOff;
	bool Startup=true;

	mouse->Show();
	mouse->SetCursor ( CHand );
	mouse->MoveCallback=true;
	hud->DoAllHud();

//  if(!engine->fstcpip)
	PlayRounds=false;
	if ( PlayRounds )
	{
		if ( ActiveRoundPlayerNr!=ActivePlayer->Nr )
		{
			hud->Ende=true;
			hud->EndeButton ( true );
		}
		else if ( !hud->Ende )
		{
			hud->EndeButton ( false );
		}
	}
	ActivePlayer->DoScan();

	while ( 1 )
	{
		// Niederlage pr�fen:
		if ( Defeated ) break;
		// Den User bearbeiten:
		if ( CheckUser() ==-1 )
		{
			MakePanel ( false );
			break;
		}
		// Ende durch laden/speichern Men�
		if ( End )
		{
			MakePanel ( false );
			break;
		}
		// Die Map malen:
		if ( fDrawMap )
		{
			DrawMap();
		}
		// Die FX-Effekte anzeigen:
		if ( fDrawMap )
		{
			DisplayFX();
		}
		// Ggf das Objekt deselektieren:
		if ( SelectedVehicle&&SelectedVehicle->owner!=ActivePlayer&&!ActivePlayer->ScanMap[SelectedVehicle->PosX+SelectedVehicle->PosY*map->size] )
		{
			SelectedVehicle->Deselct();
			SelectedVehicle=NULL;
		}
		if ( SelectedBuilding&&SelectedBuilding->owner!=ActivePlayer&&!ActivePlayer->ScanMap[SelectedBuilding->PosX+SelectedBuilding->PosY*map->size] )
		{
			SelectedBuilding->Deselct();
			SelectedBuilding=NULL;
		}
		// Die Objekt-Kreise malen:
		if ( SelectedVehicle )
		{
			int spx,spy;
			spx=SelectedVehicle->GetScreenPosX();
			spy=SelectedVehicle->GetScreenPosY();
			if ( hud->Scan )
			{
				DrawCircle ( spx+hud->Zoom/2,
				             spy+hud->Zoom/2,
				             SelectedVehicle->data.scan*hud->Zoom,SCAN_COLOR,buffer );
			}
			if ( hud->Reichweite&& ( SelectedVehicle->data.can_attack==ATTACK_LAND||SelectedVehicle->data.can_attack==ATTACK_SUB_LAND||SelectedVehicle->data.can_attack==ATTACK_AIRnLAND ) )
			{
				DrawCircle ( spx+hud->Zoom/2,
				             spy+hud->Zoom/2,
				             SelectedVehicle->data.range*hud->Zoom+1,RANGE_GROUND_COLOR,buffer );
			}
			if ( hud->Reichweite&&SelectedVehicle->data.can_attack==ATTACK_AIR )
			{
				DrawCircle ( spx+hud->Zoom/2,
				             spy+hud->Zoom/2,
				             SelectedVehicle->data.range*hud->Zoom+2,RANGE_AIR_COLOR,buffer );
			}
			if ( hud->Munition&&SelectedVehicle->data.can_attack )
			{
				SelectedVehicle->DrawMunBar();
			}
			if ( hud->Treffer )
			{
				SelectedVehicle->DrawHelthBar();
			}
			if ( ( ( SelectedVehicle->IsBuilding&&SelectedVehicle->BuildRounds==0 ) || ( SelectedVehicle->IsClearing&&SelectedVehicle->ClearingRounds==0 ) ) &&SelectedVehicle->owner==ActivePlayer )
			{
				if ( SelectedVehicle->data.can_build==BUILD_BIG||SelectedVehicle->ClearBig )
				{
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+1+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+2+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom*2,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+2+ ( SelectedVehicle->PosY ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom*2,spy );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY+1 ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy+hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+2+ ( SelectedVehicle->PosY+1 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom*2,spy+hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY+2 ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy+hud->Zoom*2 );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+ ( SelectedVehicle->PosY+2 ) *map->size ) ) DrawExitPoint ( spx,spy+hud->Zoom*2 );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+1+ ( SelectedVehicle->PosY+2 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom,spy+hud->Zoom*2 );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+2+ ( SelectedVehicle->PosY+2 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom*2,spy+hud->Zoom*2 );
				}
				else
				{
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+1+ ( SelectedVehicle->PosY-1 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom,spy-hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+1+ ( SelectedVehicle->PosY ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom,spy );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX-1+ ( SelectedVehicle->PosY+1 ) *map->size ) ) DrawExitPoint ( spx-hud->Zoom,spy+hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+ ( SelectedVehicle->PosY+1 ) *map->size ) ) DrawExitPoint ( spx,spy+hud->Zoom );
					if ( SelectedVehicle->CanDrive ( SelectedVehicle->PosX+1+ ( SelectedVehicle->PosY+1 ) *map->size ) ) DrawExitPoint ( spx+hud->Zoom,spy+hud->Zoom );
				}
			}
			if ( SelectedVehicle->PlaceBand )
			{
				if ( SelectedVehicle->data.can_build==BUILD_BIG )
				{
					SDL_Rect dest;
					dest.x=180- ( ( int ) ( ( hud->OffX ) / ( 64.0/hud->Zoom ) ) ) +hud->Zoom*SelectedVehicle->BandX;
					dest.y=18- ( ( int ) ( ( hud->OffY ) / ( 64.0/hud->Zoom ) ) ) +hud->Zoom*SelectedVehicle->BandY;
					dest.h=dest.w=GraphicsData.gfx_band_big->h;
					SDL_BlitSurface ( GraphicsData.gfx_band_big,NULL,buffer,&dest );
				}
				else
				{
					SDL_Rect dest;
					int x,y;
					mouse->GetKachel ( &x,&y );
					if ( x==SelectedVehicle->PosX||y==SelectedVehicle->PosY )
					{
						dest.x=180- ( ( int ) ( ( hud->OffX ) / ( 64.0/hud->Zoom ) ) ) +hud->Zoom*x;
						dest.y=18- ( ( int ) ( ( hud->OffY ) / ( 64.0/hud->Zoom ) ) ) +hud->Zoom*y;
						dest.h=dest.w=GraphicsData.gfx_band_small->h;
						SDL_BlitSurface ( GraphicsData.gfx_band_small,NULL,buffer,&dest );
						SelectedVehicle->BandX=x;
						SelectedVehicle->BandY=y;
						SelectedVehicle->BuildPath=true;
					}
					else
					{
						SelectedVehicle->BandX=SelectedVehicle->PosX;
						SelectedVehicle->BandY=SelectedVehicle->PosY;
					}
				}
			}
			if ( SelectedVehicle->ActivatingVehicle&&SelectedVehicle->owner==ActivePlayer )
			{
				SelectedVehicle->DrawExitPoints ( SelectedVehicle->StoredVehicles->VehicleItems[SelectedVehicle->VehicleToActivate]->typ );
			}
		}
		else if ( SelectedBuilding )
		{
			int spx,spy;
			spx=SelectedBuilding->GetScreenPosX();
			spy=SelectedBuilding->GetScreenPosY();
			if ( hud->Scan )
			{
				if ( SelectedBuilding->data.is_big )
				{
					DrawCircle ( spx+hud->Zoom,
					             spy+hud->Zoom,
					             SelectedBuilding->data.scan*hud->Zoom,SCAN_COLOR,buffer );
				}
				else
				{
					DrawCircle ( spx+hud->Zoom/2,
					             spy+hud->Zoom/2,
					             SelectedBuilding->data.scan*hud->Zoom,SCAN_COLOR,buffer );
				}
			}
			if ( hud->Reichweite&& ( SelectedBuilding->data.can_attack==ATTACK_LAND||SelectedBuilding->data.can_attack==ATTACK_SUB_LAND ) &&!SelectedBuilding->data.is_expl_mine )
			{
				DrawCircle ( spx+hud->Zoom/2,
				             spy+hud->Zoom/2,
				             SelectedBuilding->data.range*hud->Zoom+2,RANGE_GROUND_COLOR,buffer );
			}
			if ( hud->Reichweite&&SelectedBuilding->data.can_attack==ATTACK_AIR )
			{
				DrawCircle ( spx+hud->Zoom/2,
				             spy+hud->Zoom/2,
				             SelectedBuilding->data.range*hud->Zoom+2,RANGE_AIR_COLOR,buffer );
			}
			if ( hud->Reichweite&&SelectedBuilding->data.max_shield )
			{
				if ( SelectedBuilding->data.is_big )
				{
					DrawCircle ( spx+hud->Zoom,
					             spy+hud->Zoom,
					             SelectedBuilding->data.range*hud->Zoom+3,RANGE_SHIELD_COLOR,buffer );
				}
				else
				{
					DrawCircle ( spx+hud->Zoom/2,
					             spy+hud->Zoom/2,
					             SelectedBuilding->data.range*hud->Zoom+3,RANGE_SHIELD_COLOR,buffer );
				}
			}
			if ( hud->Munition&&SelectedBuilding->data.can_attack&&!SelectedBuilding->data.is_expl_mine )
			{
				SelectedBuilding->DrawMunBar();
			}
			if ( hud->Treffer )
			{
				SelectedBuilding->DrawHelthBar();
			}
			if ( SelectedBuilding->BuildList && SelectedBuilding->BuildList->Count && !SelectedBuilding->IsWorking && SelectedBuilding->BuildList->BuildListItems[0]->metall_remaining <= 0 && SelectedBuilding->owner == ActivePlayer )
			{
				SelectedBuilding->DrawExitPoints ( SelectedBuilding->BuildList->BuildListItems[0]->typ );
			}
			if ( SelectedBuilding->ActivatingVehicle&&SelectedBuilding->owner==ActivePlayer )
			{
				SelectedBuilding->DrawExitPoints ( SelectedBuilding->StoredVehicles->VehicleItems[SelectedBuilding->VehicleToActivate]->typ );
			}
		}
		ActivePlayer->DrawLockList ( hud );
		// Die Minimap malen:
		if ( fDrawMMap )
		{
			fDrawMMap=false;
			DrawMiniMap();
			fDrawHud=true;
		}
		// Debugausgaben machen:
		DebugOff=30;
		if ( DebugFPS )
		{
			// Frames pro Sekunde:
			char DebugStr[100];
			int hour, min, sec;
			unsigned int time;
			cycles++;
			if ( fDrawMap )
			{
				hour=SDL_GetTicks() / ( 60*60*1000 );
				min=SDL_GetTicks() / ( 60*1000 )-hour*60;
				sec=SDL_GetTicks() /1000-min*60-hour*60*60;
				time = SDL_GetTicks()-FPSstart;
				if ( !time ) time++;
				sprintf ( DebugStr,"fps: %.2f",frames*1.0/ ( time/1000.0 ) );
				fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
				sprintf ( DebugStr,"cps: %.2f",cycles*1.0/ ( time/1000.0 ) );
				fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
				sprintf ( DebugStr,"frames: %d",frames );
				fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
				sprintf ( DebugStr,"cycles: %d",cycles );
				fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
				sprintf ( DebugStr,"time: %d:%d:%d",hour,min,sec );
				fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
			}
		}
		/*if(DebugCom&&engine->fstcpip&&fDrawMap){
		  // Com-Infos:
		  unsigned short hour,min,sec,msec;
		  char DebugStr[100];
		  unsigned int time;
		  sprintf(DebugStr,"connections: %d",engine->fstcpip->GetConnectionCount());
		  fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;
		  sprintf(DebugStr,"lags: %d",engine->fstcpip->GetLag());
		  fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;
		  sprintf(DebugStr,"retrys: %d",engine->fstcpip->GetRetrys());
		  fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;
		  sprintf(DebugStr,"resends: %d",engine->fstcpip->GetResends());
		  fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;
		  (Comstart.CurrentTime()-Comstart).DecodeTime(&hour,&min,&sec,&msec);
		  time=(((int)hour*24+min)*60+sec)*1000+msec;
		  if(time>1000){
		    Comstart=Comstart.CurrentTime();
		    AddDebugComGraph(engine->fstcpip->GetTX(),engine->fstcpip->GetRX());
		    engine->fstcpip->ResetStats();
		  }
		  ShowDebugComGraph(DebugOff);DebugOff+=60;
		}*/
		if ( DebugBase&&fDrawMap )
		{
			char DebugStr[100];
			sprintf ( DebugStr,"subbases: %d",ActivePlayer->base->SubBases->Count );
			fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
		}
		if ( DebugWache&&fDrawMap )
		{
			char DebugStr[100];
			sprintf ( DebugStr,"w-air: %d",ActivePlayer->WachpostenAir->Count );
			fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
			sprintf ( DebugStr,"w-ground: %d",ActivePlayer->WachpostenGround->Count );
			fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
		}
		if ( DebugFX&&fDrawMap )
		{
			char DebugStr[100];
			sprintf ( DebugStr,"fx-count: %d",FXList->Count+FXListBottom->Count );
			fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
			sprintf ( DebugStr,"wind-dir: %d", ( int ) ( WindDir*57.29577 ) );
			fonts->OutTextSmall ( DebugStr,550,DebugOff,ClWhite,buffer );DebugOff+=10;
		}
		/*if(engine->PingList){
		  unsigned short hour,min,sec,msec;
		  char DebugStr[100];
		  unsigned int time;
		  int i,missing;
		  (engine->PingStart.CurrentTime()-engine->PingStart).DecodeTime(&hour,&min,&sec,&msec);
		  time=(((int)hour*24+min)*60+sec)*1000+msec;

		  sprintf(DebugStr,"ping: %d",5000-time);
		  fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;

		  missing=0;
		  for(i=0;i<engine->PingList->Count;i++){
		    sPing *p;
		    p=(sPing*)(engine->PingList->Items[i]);

		    missing+=PING_COUNT-p->rx_count;
		    sprintf(DebugStr,"ping %d: %d/%d",i,p->rx_count,PING_COUNT);
		    fonts->OutTextSmall(DebugStr,550,DebugOff,ClWhite,buffer);DebugOff+=10;
		  }

		  if(missing==0){
		    bool wrong=false;
		    int k;

		    for(i=0;i<engine->PingList->Count;i++){
		      sPing *p;
		      p=(sPing*)(engine->PingList->Items[i]);
		      for(k=0;k<p->rx_count;k++){
		        if(p->rx[k]!=k){
		          wrong=true;
		          break;
		        }
		      }
		      if(wrong)break;
		    }

		    if(wrong){
		      AddMessage("Ping complete, but wron order!");
		    }else{
		      AddMessage("Ping ok!");
		    }

		    while(engine->PingList->Count){
		      delete (sPing*)(engine->PingList->Items[0]);
		      engine->PingList->Delete(0);
		    }
		    delete engine->PingList;
		    engine->PingList=NULL;
		  }else if(time>5000){
		    sprintf(DebugStr,"TIMEOUT! missing: %d",missing);
		    AddMessage(DebugStr);
		    while(engine->PingList->Count){
		      delete (sPing*)(engine->PingList->Items[0]);
		      engine->PingList->Delete(0);
		    }
		    delete engine->PingList;
		    engine->PingList=NULL;
		  }
		}*/
		if ( DebugTrace&&fDrawMap )
		{
			Trace();
		}
		if ( DebugLog&&ShowLog&&fDrawMap )
		{
			for ( i=0;i<engine->LogHistory->Count;i++ )
			{
				string str;
				str=engine->LogHistory->Items[i];
				fonts->OutTextSmall ( ( char* ) str.c_str(),184,20+i*8,ClWhite,buffer );
			}
		}
		// Pr�fen, ob das Hud neu gemalt werden muss:
		if ( fDrawHud||fDrawMap )
		{
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			mouse->GetBack ( buffer );
			fDraw=true;
		}
		// Das Video malen:
		if ( fDraw||fDrawHud )
		{
			DrawFLC();
		}
		// Ggf das Objektmen� anzeigen:
		if ( fDrawMap&&SelectedVehicle&&SelectedVehicle->MenuActive )
		{
			SelectedVehicle->DrawMenu();
		}
		if ( fDrawMap&&SelectedBuilding&&SelectedBuilding->MenuActive )
		{
			SelectedBuilding->DrawMenu();
		}
		// Ggf die Chateingabe anzeigen:
		if ( ChatInput&&fDrawMap )
		{
			string OutTxt = ">"; OutTxt += InputStr.c_str();
			if ( Frame%2 )
			{
				fonts->OutText ( ( char * ) OutTxt.c_str(),185,440,buffer );
			}
			else
			{
				OutTxt += "_";
				fonts->OutText ( ( char * ) OutTxt.c_str(),185,440,buffer );
			}
		}
		// Die Messages anzeigen:
		if ( fDrawMap )
		{
			HandleMessages();
		}
		// Rundenende pr�fen:
		if ( WantToEnd&&!engine->CheckVehiclesMoving ( true ) )
		{
			WantToEnd=false;
			if ( HotSeat )
			{
				if ( MakeHotSeatEnde() )
				{
					engine->EndePressed ( ActivePlayer->Nr );
					hud->Ende=true;
				}
			}
			else
			{
				engine->EndePressed ( ActivePlayer->Nr );
			}
		}
		engine->CheckEnde();
		// Die Maus malen:
		if ( fDraw )
		{
			LastMouseX=mouse->x;LastMouseY=mouse->y;
			mouse->draw ( false,buffer );
		}
		else if ( mouse->x!=LastMouseX||mouse->y!=LastMouseY )
		{
			LastMouseX=mouse->x;LastMouseY=mouse->y;
			mouse->draw ( true,screen );
		}
		// Den Buffer anzeigen:
		if ( fDraw )
		{
			frames++;
			if ( Startup )
			{
				MakePanel ( true );
				Startup=false;
			}
			SHOW_SCREEN
			fDraw=false;
			fDrawHud=false;
			fDrawMap=false;
		}
		else if ( !SettingsData.bFastMode )
		{
			SDL_Delay ( 1 ); // Es gibt nichts zu tun.
		}
		// Die Engine laufen lassen:
		engine->Run();
		// Alle Timer Handeln:
		HandleTimer();
		if ( timer1 )
		{
			Frame++;
			fDrawMap=true;
			RotateBlinkColor();
			if ( FLC!=NULL&&hud->PlayFLC )
			{
				FLI_NextFrame ( FLC );
			}
		}
		// Den Wind �ndern:
		if ( timer2&&SettingsData.bDamageEffects )
		{
			static int NextChange=25,NextDirChange=25,dir=90,change=3;
			if ( NextChange==0 )
			{
				NextChange=10+random ( 20,0 );
				dir+=change;
				SetWind ( dir );
				if ( dir>=360 ) dir-=360;
				else if ( dir<0 ) dir+=360;

				if ( NextDirChange==0 )
				{
					NextDirChange=10+random ( 25,0 );
					change=random ( 11,0 )-5;
				}
				else NextDirChange--;

			}
			else NextChange--;
		}
	}
	mouse->MoveCallback=false;
}

// CheckUser /////////////////////////////////////////////////////////////////
// Pr�ft alle Eingaben vom User und gibt -1 zur�ck zum Beenden:
int cGame::CheckUser ( void )
{
	static int LastMouseButton=0,MouseButton;
	static bool LastReturn=false;
	Uint8 *keystate;
	// Events holen:
	SDL_PumpEvents();

	// Tasten pr�fen:
	keystate=SDL_GetKeyState ( NULL );
	if ( ChangeObjectName )
	{
		DoKeyInp ( keystate );
		if ( InputEnter )
		{
			ChangeObjectName=false;
			if ( SelectedVehicle )
			{
				engine->ChangeVehicleName ( SelectedVehicle->PosX,SelectedVehicle->PosY,InputStr,false,SelectedVehicle->data.can_drive==DRIVE_AIR );
			}
			else if ( SelectedBuilding )
			{
				engine->ChangeBuildingName ( SelectedBuilding->PosX,SelectedBuilding->PosY,InputStr,false,SelectedBuilding->data.is_base );
			}
		}
		else
		{
			if ( fonts->GetTextLenSmall ( ( char * ) InputStr.c_str() ) >=128 )
			{
				InputStr.erase ( InputStr.length()-1 );
			}
		}
	}
	else if ( ChatInput )
	{
		DoKeyInp ( keystate );
		if ( InputEnter )
		{
			ChatInput=false;
			if ( !InputStr.empty() &&!DoCommand ( ( char * ) InputStr.c_str() ) )
			{
				// engine->SendChatMessage((ActivePlayer->name+": "+InputStr).c_str());
			}
		}
		else
		{
			string OutTxt = ">"; OutTxt += InputStr; OutTxt += "_";
			if ( fonts->GetTextLen ( ( char * ) OutTxt.c_str() ) >=440 )
			{
				InputStr.erase ( InputStr.length()-1 );
			}
		}
	}
	else
	{
		if ( keystate[KeysList.KeyExit]&&ShowYesNo ( "Wirklich beenden?\n\nAlle nicht gespeicherten Spieldaten gehen dabei verloren." ) )
		{
			game->DrawMap ( false );
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			return -1;
		}
		if ( keystate[KeysList.KeyJumpToAction]&&MsgCoordsX!=-1 )
		{
			hud->OffX=MsgCoordsX*64- ( ( int ) ( ( ( float ) 224/hud->Zoom ) *64 ) ) +32;
			hud->OffY=MsgCoordsY*64- ( ( int ) ( ( ( float ) 224/hud->Zoom ) *64 ) ) +32;
			fDrawMap=true;
			hud->DoScroll ( 0 );
			MsgCoordsX=-1;
		}
		if ( keystate[KeysList.KeyEndTurn]&&!LastReturn&&!hud->Ende )
		{
			hud->EndeButton ( true );
			hud->MakeMeMyEnd();
			LastReturn=true;
		}
		else if ( !keystate[KeysList.KeyEndTurn] ) LastReturn=false;
		if ( keystate[KeysList.KeyChat]&&!keystate[SDLK_RALT]&&!keystate[SDLK_LALT] )
		{
			ChatInput=true;
			InputStr="";
		}
		if ( keystate[KeysList.KeyScroll8a]||keystate[KeysList.KeyScroll8b] ) hud->DoScroll ( 8 );
		if ( keystate[KeysList.KeyScroll2a]||keystate[KeysList.KeyScroll2b] ) hud->DoScroll ( 2 );
		if ( keystate[KeysList.KeyScroll6a]||keystate[KeysList.KeyScroll6b] ) hud->DoScroll ( 6 );
		if ( keystate[KeysList.KeyScroll4a]||keystate[KeysList.KeyScroll4b] ) hud->DoScroll ( 4 );
		if ( keystate[KeysList.KeyScroll7] ) hud->DoScroll ( 7 );
		if ( keystate[KeysList.KeyScroll9] ) hud->DoScroll ( 9 );
		if ( keystate[KeysList.KeyScroll1] ) hud->DoScroll ( 1 );
		if ( keystate[KeysList.KeyScroll3] ) hud->DoScroll ( 3 );
		if ( keystate[KeysList.KeyZoomIna]||keystate[KeysList.KeyZoomInb] ) hud->SetZoom ( hud->Zoom+1 );
		if ( keystate[KeysList.KeyZoomOuta]||keystate[KeysList.KeyZoomOutb] ) hud->SetZoom ( hud->Zoom-1 );

		{
			static SDLKey last_key=SDLK_UNKNOWN;
			if ( keystate[KeysList.KeyFog] ) {if ( last_key!=KeysList.KeyFog ) {hud->SwitchNebel ( !hud->Nebel );last_key=KeysList.KeyFog;}}
			else if ( keystate[KeysList.KeyGrid] ) {if ( last_key!=KeysList.KeyGrid ) {hud->SwitchGitter ( !hud->Gitter );last_key=KeysList.KeyGrid;}}
			else if ( keystate[KeysList.KeyScan] ) {if ( last_key!=KeysList.KeyScan ) {hud->SwitchScan ( !hud->Scan );last_key=KeysList.KeyScan;}}
			else if ( keystate[KeysList.KeyRange] ) {if ( last_key!=KeysList.KeyRange ) {hud->SwitchReichweite ( !hud->Reichweite );last_key=KeysList.KeyRange;}}
			else if ( keystate[KeysList.KeyAmmo] ) {if ( last_key!=KeysList.KeyAmmo ) {hud->SwitchMunition ( !hud->Munition );last_key=KeysList.KeyAmmo;}}
			else if ( keystate[KeysList.KeyHitpoints] ) {if ( last_key!=KeysList.KeyHitpoints ) {hud->SwitchTreffer ( !hud->Treffer );last_key=KeysList.KeyHitpoints;}}
			else if ( keystate[KeysList.KeyColors] ) {if ( last_key!=KeysList.KeyColors ) {hud->SwitchFarben ( !hud->Farben );last_key=KeysList.KeyColors;}}
			else if ( keystate[KeysList.KeyStatus] ) {if ( last_key!=KeysList.KeyStatus ) {hud->SwitchStatus ( !hud->Status );last_key=KeysList.KeyStatus;}}
			else if ( keystate[KeysList.KeySurvey] ) {if ( last_key!=KeysList.KeySurvey ) {hud->SwitchStudie ( !hud->Studie );last_key=KeysList.KeySurvey;}}
			else last_key=SDLK_UNKNOWN;
		}
	}

	// Mauskoords holen:
	mouse->GetPos();
	// Mausbuttons handeln:
	MouseButton=mouse->GetMouseButton();
	if ( MouseStyle==OldSchool&&MouseButton==4&&LastMouseButton!=4&&OverObject )
	{
		if ( OverObject->vehicle ) OverObject->vehicle->ShowHelp();
		else if ( OverObject->plane ) OverObject->plane->ShowHelp();
		else if ( OverObject->base ) OverObject->base->ShowHelp();
		else if ( OverObject->top ) OverObject->top->ShowHelp();
		MouseButton=0;
	}
	if ( MouseStyle==OldSchool&&MouseButton==5 ) MouseButton=4;
	if ( MouseButton==4&&LastMouseButton!=4 )
	{
		if ( HelpActive )
		{
			HelpActive=false;
		}
		else
		{
			if ( OverObject&& (
			            ( SelectedVehicle&& ( OverObject->vehicle==SelectedVehicle||OverObject->plane==SelectedVehicle ) ) ||
			            ( SelectedBuilding&& ( OverObject->base==SelectedBuilding||OverObject->top==SelectedBuilding ) ) ) )
			{
				int next=-1;

				if ( SelectedVehicle )
				{
					if ( OverObject->plane==SelectedVehicle )
					{
						if ( OverObject->vehicle ) next='v';
						else if ( OverObject->top ) next='t';
						else if ( OverObject->base&&OverObject->base->owner ) next='b';
					}
					else
					{
						if ( OverObject->top ) next='t';
						else if ( OverObject->base&&OverObject->base->owner ) next='b';
						else if ( OverObject->plane ) next='p';
					}

					SelectedVehicle->Deselct();
					SelectedVehicle=NULL;
					ChangeObjectName=false;
					StopFXLoop ( ObjectStream );
				}
				else if ( SelectedBuilding )
				{
					if ( OverObject->top==SelectedBuilding )
					{
						if ( OverObject->base&&OverObject->base->owner ) next='b';
						else if ( OverObject->plane ) next='p';
						else if ( OverObject->vehicle ) next='v';
					}
					else
					{
						if ( OverObject->plane ) next='p';
						else if ( OverObject->vehicle ) next='v';
						else if ( OverObject->top ) next='t';
					}

					SelectedBuilding->Deselct();
					SelectedBuilding=NULL;
					ChangeObjectName=false;
					StopFXLoop ( ObjectStream );
				}
				switch ( next )
				{
					case 't':
						SelectedBuilding=OverObject->top;
						SelectedBuilding->Select();
						ObjectStream=SelectedBuilding->PlayStram();
						break;
					case 'b':
						SelectedBuilding=OverObject->base;
						SelectedBuilding->Select();
						ObjectStream=SelectedBuilding->PlayStram();
						break;
					case 'v':
						SelectedVehicle=OverObject->vehicle;
						SelectedVehicle->Select();
						ObjectStream=SelectedVehicle->PlayStram();
						break;
					case 'p':
						SelectedVehicle=OverObject->plane;
						SelectedVehicle->Select();
						ObjectStream=SelectedVehicle->PlayStram();
						break;
				}
			}
			else if ( SelectedVehicle!=NULL )
			{
				SelectedVehicle->Deselct();
				SelectedVehicle=NULL;
				ChangeObjectName=false;
				StopFXLoop ( ObjectStream );
			}
			else if ( SelectedBuilding!=NULL )
			{
				SelectedBuilding->Deselct();
				SelectedBuilding=NULL;
				ChangeObjectName=false;
				StopFXLoop ( ObjectStream );
			}
		}
	}
	if ( MouseButton&&!LastMouseButton&&MouseButton!=4 )
	{
		if ( OverObject&&hud->Lock ) ActivePlayer->ToggelLock ( OverObject );
		if ( SelectedVehicle&&mouse->cur==GraphicsData.gfx_Ctransf )
		{
			SelectedVehicle->ShowTransfer ( map->GO+mouse->GetKachelOff() );
		}
		else if ( SelectedBuilding&&mouse->cur==GraphicsData.gfx_Ctransf )
		{
			SelectedBuilding->ShowTransfer ( map->GO+mouse->GetKachelOff() );
		}
		else if ( SelectedVehicle&&SelectedVehicle->PlaceBand&&mouse->cur==GraphicsData.gfx_Cband )
		{
			SelectedVehicle->PlaceBand=false;
			SelectedVehicle->IsBuilding=true;
			if ( SelectedVehicle->data.can_build==BUILD_BIG )
			{
				// Den Building Sound machen:
				StopFXLoop ( ObjectStream );
				ObjectStream=SelectedVehicle->PlayStram();
				map->GO[SelectedVehicle->BandX+SelectedVehicle->BandY*map->size].vehicle=SelectedVehicle;
				map->GO[SelectedVehicle->BandX+1+SelectedVehicle->BandY*map->size].vehicle=SelectedVehicle;
				map->GO[SelectedVehicle->BandX+ ( SelectedVehicle->BandY+1 ) *map->size].vehicle=SelectedVehicle;
				map->GO[SelectedVehicle->BandX+1+ ( SelectedVehicle->BandY+1 ) *map->size].vehicle=SelectedVehicle;
				SelectedVehicle->PosX=SelectedVehicle->BandX;
				SelectedVehicle->PosY=SelectedVehicle->BandY;
			}
			else if ( SelectedVehicle->PosX!=SelectedVehicle->BandX||SelectedVehicle->PosY!=SelectedVehicle->BandY )
			{
				// Den Building Sound machen:
				StopFXLoop ( ObjectStream );
				ObjectStream=SelectedVehicle->PlayStram();
			}
			else
			{
				SelectedVehicle->IsBuilding=false;
			}
		}
		else if ( mouse->cur==GraphicsData.gfx_Cactivate&&SelectedBuilding&&SelectedBuilding->ActivatingVehicle )
		{
			SelectedBuilding->ExitVehicleTo ( SelectedBuilding->VehicleToActivate,mouse->GetKachelOff(),false );
			PlayFX ( SoundData.SNDActivate );
			MouseMoveCallback ( true );
		}
		else if ( mouse->cur==GraphicsData.gfx_Cactivate&&SelectedVehicle&&SelectedVehicle->ActivatingVehicle )
		{
			SelectedVehicle->ExitVehicleTo ( SelectedVehicle->VehicleToActivate,mouse->GetKachelOff(),false );
			PlayFX ( SoundData.SNDActivate );
			MouseMoveCallback ( true );
		}
		else if ( mouse->cur==GraphicsData.gfx_Cactivate&&SelectedBuilding&&SelectedBuilding->BuildList&&SelectedBuilding->BuildList->Count )
		{
			sBuildList *ptr;
			int x,y;
			mouse->GetKachel ( &x,&y );
			ptr=SelectedBuilding->BuildList->BuildListItems[0];
			engine->AddVehicle ( x,y,ptr->typ,ActivePlayer,false );
			if ( SelectedBuilding->RepeatBuild )
			{
				SelectedBuilding->BuildList->DeleteBuildList ( 0 );
				ptr->metall_remaining=ptr->typ->data.costs;
				SelectedBuilding->BuildList->AddBuildList ( ptr );
				SelectedBuilding->StartWork();
			}
			else
			{
				delete ptr;
				SelectedBuilding->BuildList->DeleteBuildList ( 0 );
				if ( SelectedBuilding->BuildList->Count )
				{
					SelectedBuilding->StartWork();
				}
			}
			MouseMoveCallback ( true );
		}
		else if ( mouse->cur==GraphicsData.gfx_Cload&&SelectedBuilding&&SelectedBuilding->LoadActive )
		{
			int off;
			off=mouse->GetKachelOff();
			PlayFX ( SoundData.SNDLoad );
			SelectedBuilding->StoreVehicle ( off );
		}
		else if ( mouse->cur==GraphicsData.gfx_Cload&&SelectedVehicle&&SelectedVehicle->LoadActive )
		{
			int off;
			off=mouse->GetKachelOff();
			PlayFX ( SoundData.SNDLoad );
			SelectedVehicle->StoreVehicle ( off );
		}
		else if ( mouse->cur==GraphicsData.gfx_Cmuni&&SelectedVehicle&&SelectedVehicle->MuniActive )
		{
			cBuilding *b=NULL;
			cVehicle *v=NULL;
			int off;
			off=mouse->GetKachelOff();
			PlayFX ( SoundData.SNDReload );
			SelectedVehicle->data.cargo-=2;
			v=map->GO[off].vehicle;
			if ( !v&&map->GO[off].plane&&map->GO[off].plane->FlightHigh==0 ) v=map->GO[off].plane;
			if ( v )
			{
				v->data.ammo=v->data.max_ammo;
			}
			else
			{
				b=map->GO[off].top;
				if ( b )
				{
					b->data.ammo=b->data.max_ammo;
				}
			}
			SelectedVehicle->ShowDetails();
		}
		else if ( mouse->cur==GraphicsData.gfx_Crepair&&SelectedVehicle&&SelectedVehicle->RepairActive )
		{
			cBuilding *b=NULL;
			cVehicle *v=NULL;
			int off;
			off=mouse->GetKachelOff();
			PlayFX ( SoundData.SNDRepair );
			SelectedVehicle->data.cargo-=2;
			v=map->GO[off].vehicle;
			if ( !v&&map->GO[off].plane&&map->GO[off].plane->FlightHigh==0 ) v=map->GO[off].plane;
			if ( v )
			{
				v->data.hit_points=v->data.max_hit_points;
			}
			else
			{
				b=map->GO[off].top;
				if ( b )
				{
					b->data.hit_points=b->data.max_hit_points;
				}
			}
			SelectedVehicle->ShowDetails();
		}
		else if ( mouse->cur==GraphicsData.gfx_Cmove&&SelectedVehicle&&!SelectedVehicle->moving&&!SelectedVehicle->rotating&&!hud->Ende&&!SelectedVehicle->Attacking )
		{
			if ( SelectedVehicle->data.can_drive!=DRIVE_AIR )
			{
				if ( ( SelectedVehicle->IsBuilding&&SelectedVehicle->data.can_build==BUILD_BIG ) || ( SelectedVehicle->IsClearing&&SelectedVehicle->ClearBig ) )
				{
					// Das Vehicle an den Rand setzen:
					bool changed=false;
					int x,y;
					mouse->GetKachel ( &x,&y );
					if ( abs ( x-SelectedVehicle->PosX ) <=1&&abs ( y-SelectedVehicle->PosY ) <=1 )
					{
						// Nichts erforderlich...
					}
					else if ( abs ( x- ( SelectedVehicle->PosX+1 ) ) <=1&&abs ( y-SelectedVehicle->PosY ) <=1 )
					{
						SelectedVehicle->PosX++;
						changed=true;
					}
					else if ( abs ( x- ( SelectedVehicle->PosX+1 ) ) <=1&&abs ( y- ( SelectedVehicle->PosY+1 ) ) <=1 )
					{
						SelectedVehicle->PosX++;SelectedVehicle->PosY++;
						changed=true;
					}
					else if ( abs ( x-SelectedVehicle->PosX ) <=1&&abs ( y- ( SelectedVehicle->PosY+1 ) ) <=1 )
					{
						SelectedVehicle->PosY++;
						changed=true;
					}
				}
				// hans
				if ( keystate[KeysList.KeyCalcPath] )
					engine->AddMoveJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,false,true );
				else
					engine->AddMoveJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,false );
			}
			else
			{
				if ( keystate[KeysList.KeyCalcPath] )
					engine->AddMoveJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,true,true );
				else
					engine->AddMoveJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,true );
			}
		}
		else if ( !HelpActive )
		{
			hud->CheckButtons();
			// Pr�fen, ob die Maus �ber einem Objektmen� ist:
			if ( ( SelectedVehicle&&SelectedVehicle->MenuActive&&SelectedVehicle->MouseOverMenu ( mouse->x,mouse->y ) ) ||
			        ( SelectedBuilding&&SelectedBuilding->MenuActive&&SelectedBuilding->MouseOverMenu ( mouse->x,mouse->y ) ) )
			{
			}
			else
				// Pr�fen, ob geschossen werden soll:
				if ( mouse->cur==GraphicsData.gfx_Cattack&&SelectedVehicle&&!SelectedVehicle->Attacking&&!SelectedVehicle->MoveJobActive )
				{
					switch ( SelectedVehicle->data.can_attack )
					{
						case ATTACK_LAND:
						case ATTACK_SUB_LAND:
							engine->AddAttackJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,SelectedVehicle->data.can_drive==DRIVE_AIR,false,false );
							break;
						case ATTACK_AIR:
						case ATTACK_AIRnLAND:
							engine->AddAttackJob ( SelectedVehicle->PosX+SelectedVehicle->PosY*map->size,mouse->GetKachelOff(),false,SelectedVehicle->data.can_drive==DRIVE_AIR,true,false );
							break;
					}
					// Ggf den mjob stoppen:
					if ( SelectedVehicle->mjob )
					{
						SelectedVehicle->mjob->finished=true;
						SelectedVehicle->mjob=NULL;
						SelectedVehicle->MoveJobActive=false;
					}
				}
				else if ( mouse->cur==GraphicsData.gfx_Cattack&&SelectedBuilding&&!SelectedBuilding->Attacking )
				{
					switch ( SelectedBuilding->data.can_attack )
					{
						case ATTACK_LAND:
						case ATTACK_SUB_LAND:
							engine->AddAttackJob ( SelectedBuilding->PosX+SelectedBuilding->PosY*map->size,mouse->GetKachelOff(),false,false,false,true );
							break;
						case ATTACK_AIR:
						case ATTACK_AIRnLAND:
							engine->AddAttackJob ( SelectedBuilding->PosX+SelectedBuilding->PosY*map->size,mouse->GetKachelOff(),false,false,true,true );
							break;
					}
				}
				else if ( mouse->cur == GraphicsData.gfx_Csteal && SelectedVehicle )
				{
					SelectedVehicle->CommandoOperation ( mouse->GetKachelOff(),true );
				}
				else if ( mouse->cur == GraphicsData.gfx_Cdisable && SelectedVehicle )
				{
					SelectedVehicle->CommandoOperation ( mouse->GetKachelOff(),false );
				}
				else
					// Das Objekt ausw�hlen:
					if ( OverObject&&!hud->Ende )
					{
						if ( SelectedVehicle&& ( OverObject->plane==SelectedVehicle||OverObject->vehicle==SelectedVehicle ) )
						{
							if ( !SelectedVehicle->moving&&!SelectedVehicle->rotating&&SelectedVehicle->owner==ActivePlayer )
							{
								SelectedVehicle->MenuActive=true;
								PlayFX ( SoundData.SNDHudButton );
							}
						}
						else if ( SelectedBuilding&& ( OverObject->base==SelectedBuilding||OverObject->top==SelectedBuilding ) )
						{
							if ( SelectedBuilding->owner==ActivePlayer )
							{
								SelectedBuilding->MenuActive=true;
								PlayFX ( SoundData.SNDHudButton );
							}
						}
						else if ( OverObject->plane&&!OverObject->plane->moving&&!OverObject->plane->rotating )
						{
							ChangeObjectName=false;
							if ( SelectedVehicle==OverObject->plane )
							{
								if ( SelectedVehicle->owner==ActivePlayer )
								{
									SelectedVehicle->MenuActive=true;
									PlayFX ( SoundData.SNDHudButton );
								}
							}
							else
							{
								if ( SelectedVehicle )
								{
									SelectedVehicle->Deselct();
									SelectedVehicle=NULL;
									StopFXLoop ( ObjectStream );
								}
								else if ( SelectedBuilding )
								{
									SelectedBuilding->Deselct();
									SelectedBuilding=NULL;
									StopFXLoop ( ObjectStream );
								}
								SelectedVehicle=OverObject->plane;
								SelectedVehicle->Select();
								ObjectStream=SelectedVehicle->PlayStram();
							}
						}
						else if ( OverObject->vehicle&&!OverObject->vehicle->moving&&!OverObject->vehicle->rotating&&! ( OverObject->plane&& ( OverObject->vehicle->MenuActive||OverObject->vehicle->owner!=ActivePlayer ) ) )
						{
							ChangeObjectName=false;
							if ( SelectedVehicle==OverObject->vehicle )
							{
								if ( SelectedVehicle->owner==ActivePlayer )
								{
									SelectedVehicle->MenuActive=true;
									PlayFX ( SoundData.SNDHudButton );
								}
							}
							else
							{
								if ( SelectedVehicle )
								{
									SelectedVehicle->Deselct();
									SelectedVehicle=NULL;
									StopFXLoop ( ObjectStream );
								}
								else if ( SelectedBuilding )
								{
									SelectedBuilding->Deselct();
									SelectedBuilding=NULL;
									StopFXLoop ( ObjectStream );
								}
								SelectedVehicle=OverObject->vehicle;
								SelectedVehicle->Select();
								ObjectStream=SelectedVehicle->PlayStram();
							}
						}
						else if ( OverObject->top )
						{
							ChangeObjectName=false;
							if ( SelectedBuilding==OverObject->top )
							{
								if ( SelectedBuilding->owner==ActivePlayer )
								{
									SelectedBuilding->MenuActive=true;
									PlayFX ( SoundData.SNDHudButton );
								}
							}
							else
							{
								if ( SelectedVehicle )
								{
									SelectedVehicle->Deselct();
									SelectedVehicle=NULL;
									StopFXLoop ( ObjectStream );
								}
								else if ( SelectedBuilding )
								{
									SelectedBuilding->Deselct();
									SelectedBuilding=NULL;
									StopFXLoop ( ObjectStream );
								}
								SelectedBuilding=OverObject->top;
								SelectedBuilding->Select();
								ObjectStream=SelectedBuilding->PlayStram();
							}
						}
						else if ( OverObject->base&&OverObject->base->owner )
						{
							ChangeObjectName=false;
							if ( SelectedBuilding==OverObject->base )
							{
								if ( SelectedBuilding->owner==ActivePlayer )
								{
									SelectedBuilding->MenuActive=true;
									PlayFX ( SoundData.SNDHudButton );
								}
							}
							else
							{
								if ( SelectedVehicle )
								{
									SelectedVehicle->Deselct();
									SelectedVehicle=NULL;
									StopFXLoop ( ObjectStream );
								}
								else if ( SelectedBuilding )
								{
									SelectedBuilding->Deselct();
									SelectedBuilding=NULL;
									StopFXLoop ( ObjectStream );
								}
								SelectedBuilding=OverObject->base;
								SelectedBuilding->Select();
								ObjectStream=SelectedBuilding->PlayStram();
							}
						}

					}
			// Pr�fen, ob der Name eines Objektes ge�ndert werden soll:
			if ( SelectedVehicle&&SelectedVehicle->owner==ActivePlayer&&mouse->x>=10&&mouse->y>=29&&mouse->x<10+128&&mouse->y<29+10 )
			{
				InputStr=SelectedVehicle->name;
				ChangeObjectName=true;
			}
			else if ( SelectedBuilding&&SelectedBuilding->owner==ActivePlayer&&mouse->x>=10&&mouse->y>=29&&mouse->x<10+128&&mouse->y<29+10 )
			{
				InputStr=SelectedBuilding->name;
				ChangeObjectName=true;
			}
		}
		else if ( OverObject )
		{
			if ( OverObject->plane )
			{
				OverObject->plane->ShowHelp();
			}
			else if ( OverObject->vehicle )
			{
				OverObject->vehicle->ShowHelp();
			}
			else if ( OverObject->top )
			{
				OverObject->top->ShowHelp();
			}
			else if ( OverObject->base )
			{
				OverObject->base->ShowHelp();
			}
			HelpActive=false;
		}
	}
	if ( MouseButton&&!HelpActive )
	{
		hud->CheckOneClick();
	}
	hud->ChechMouseOver();
	// Das Scrollen managen:
	hud->CheckScroll();
	LastMouseButton=MouseButton;
	return 0;
}

// Zeigt eine Nachricht mit Koordinaten an:
void cGame::AddCoords (const char *msg,int x,int y )
{
 	stringstream strStream;
 	//e.g. [85,22] missel MK I is under attack (F1)
 	strStream << "[" << x << "," << y << "] " << msg << " (" << GetKeyString ( KeysList.KeyJumpToAction ) << ")";
	AddMessage ( strStream.str() );
	MsgCoordsX=x;
	MsgCoordsY=y;
}

 void cGame::AddCoords ( const string sMsg, int x, int y)
 {
 	AddCoords ( sMsg.c_str(), x, y);
 }


bool FreeForLanding ( int x,int y )
{
	if ( x<0||x>=game->map->size||y<0||y>=game->map->size||
	        game->map->GO[x+y*game->map->size].vehicle||
	        game->map->GO[x+y*game->map->size].top||
	        game->map->IsWater ( x+y*game->map->size,false ) ||
	        TerrainData.terrain[game->map->Kacheln[x+y*game->map->size]].blocked )
	{
		return false;
	}
	return true;
}

// DrawMap ///////////////////////////////////////////////////////////////////
// Malt die Karte:
void cGame::DrawMap ( bool pure )
{
	int x,y,pos,zoom,OffX,OffY,startX,startY,endX,endY;
	struct sTerrain *terr,*defwater;
	SDL_Rect dest,tmp,scr;
	zoom=hud->Zoom;
	float f = 64.0;
	OffX= ( int ) ( hud->OffX/ ( f/zoom ) );
	OffY= ( int ) ( hud->OffY/ ( f/zoom ) );
	scr.y=0;
	scr.h=scr.w=dest.w=dest.h=zoom;
	dest.y=18-OffY;
	defwater=TerrainData.terrain+map->DefaultWater;
	for ( y=0;y<map->size;y++ )
	{
		dest.x=180-OffX;
		if ( dest.y>=18-zoom )
		{
			pos=y*map->size;
			for ( x=0;x<map->size;x++ )
			{
				if ( dest.x>=180-zoom )
				{
					// Das Terrain malen:
					tmp=dest;
					terr=TerrainData.terrain+map->Kacheln[pos];
					// Pr�fen, ob es ein K�stenst�ck ist:
					if ( terr->overlay )
					{
						scr.x= ( Frame%defwater->frames ) *zoom;
						if ( hud->Nebel&&!ActivePlayer->ScanMap[pos] )
						{
							SDL_BlitSurface ( defwater->shw,&scr,buffer,&tmp );
						}
						else
						{
							SDL_BlitSurface ( defwater->sf,&scr,buffer,&tmp );
						}
						tmp=dest;
					}
					// Ggf den Nebel malen:
					if ( hud->Nebel&&!ActivePlayer->ScanMap[pos] )
					{
						if ( terr->sf_org->w>64 )
						{
							scr.x= ( Frame%terr->frames ) *zoom;
							SDL_BlitSurface ( terr->shw,&scr,buffer,&tmp );
						}
						else
						{
							SDL_BlitSurface ( terr->shw,NULL,buffer,&tmp );
						}
					}
					else
					{
						if ( terr->frames>1 )
						{
							scr.x= ( Frame%terr->frames ) *zoom;
							SDL_BlitSurface ( terr->sf,&scr,buffer,&tmp );
						}
						else
						{
							SDL_BlitSurface ( terr->sf,NULL,buffer,&tmp );
						}
					}
				}
				pos++;
				dest.x+=zoom;
				if ( dest.x>SettingsData.iScreenW-13 ) break;
			}
		}
		dest.y+=zoom;
		if ( dest.y>SettingsData.iScreenH-15 ) break;
	}
	// Gitter malen:
	if ( hud->Gitter )
	{
		dest.x=180;
		dest.y=18+zoom- ( OffY%zoom );
		dest.w=SettingsData.iScreenW-192;
		dest.h=1;
		for ( y=0;y< ( SettingsData.iScreenH-32 ) /zoom+1;y++ )
		{
			SDL_FillRect ( buffer,&dest,GRID_COLOR );
			dest.y+=zoom;
		}
		dest.x=180+zoom- ( OffX%zoom );
		dest.y=18;
		dest.w=1;
		dest.h=SettingsData.iScreenH-32;
		for ( x=0;x< ( SettingsData.iScreenW-192 ) /zoom+1;x++ )
		{
			SDL_FillRect ( buffer,&dest,GRID_COLOR );
			dest.x+=zoom;
		}
	}
	if ( pure ) return;

	// Die FX-Bottom-Effekte anzeigen:
	DisplayFXBottom();

	// Base-Buildings malen:
	startX= ( hud->OffX-1 ) /64;if ( startX<0 ) startX=0;
	startY= ( hud->OffY-1 ) /64;if ( startY<0 ) startY=0;
	startX-=1;if ( startX<0 ) startX=0;
	startY-=1;if ( startY<0 ) startY=0;
	endX=hud->OffX/64+ ( SettingsData.iScreenW-192 ) /hud->Zoom+1;if ( endX>=map->size ) endX=map->size-1;
	endY=hud->OffY/64+ ( SettingsData.iScreenH-32 ) /hud->Zoom+1;if ( endY>=map->size ) endY=map->size-1;
	dest.y=18-OffY+zoom*startY;
	for ( y=startY;y<=endY;y++ )
	{
		dest.x=180-OffX+zoom*startX;
		pos=y*map->size+startX;
		for ( x=startX;x<=endX;x++ )
		{
			if ( ActivePlayer->ScanMap[pos] )
			{
				if ( map->GO[pos].base&&map->GO[pos].base->PosX==x&&map->GO[pos].base->PosY==y )
				{
					map->GO[pos].base->Draw ( &dest );
				}
			}
			pos++;
			dest.x+=zoom;
		}
		dest.y+=zoom;
	}
	// Vehicles malen:
	dest.y=18-OffY+zoom*startY;
	for ( y=startY;y<=endY;y++ )
	{
		dest.x=180-OffX+zoom*startX;
		pos=y*map->size+startX;
		for ( x=startX;x<=endX;x++ )
		{
			if ( ActivePlayer->ScanMap[pos] )
			{
				if ( map->GO[pos].vehicle&&map->GO[pos].vehicle->PosX==x&&map->GO[pos].vehicle->PosY==y )
				{
					map->GO[pos].vehicle->Draw ( &dest );
				}
			}
			pos++;
			dest.x+=zoom;
		}
		dest.y+=zoom;
	}
	// Top_buildings malen:
	startY-=1;if ( startY<0 ) startY=0;
	startX-=1;if ( startX<0 ) startX=0;
	dest.y=18-OffY+zoom*startY;
	for ( y=startY;y<=endY;y++ )
	{
		dest.x=180-OffX+zoom*startX;
		pos=y*map->size+startX;
		for ( x=startX;x<=endX;x++ )
		{
			if ( ActivePlayer->ScanMap[pos]||
			        ( map->GO[pos].top&&map->GO[pos].top->data.is_big&& ( ( x<endX&&ActivePlayer->ScanMap[pos+1] ) || ( y<endY&&ActivePlayer->ScanMap[pos+map->size] ) || ( x<endX&&y<endY&&ActivePlayer->ScanMap[pos+map->size+1] ) ) ) )
			{
				if ( map->GO[pos].top&&map->GO[pos].top->PosX==x&&map->GO[pos].top->PosY==y )
				{
					map->GO[pos].top->Draw ( &dest );
					if ( DebugBase )
					{
						sSubBase *sb;
						char dbstr[30];
						tmp=dest;
						if ( tmp.h>8 ) tmp.h=8;
						sb=map->GO[pos].top->SubBase;
						SDL_FillRect ( buffer,&tmp, ( int ) ( sb ) );
						sprintf ( dbstr,"%X", ( int ) ( sb ) );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1,ClWhite,buffer );
						sprintf ( dbstr,"m %d/%d %+d",sb->Metal,sb->MaxMetal,sb->MetalProd-sb->MetalNeed );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1+8,ClWhite,buffer );
						sprintf ( dbstr,"o %d/%d %+d",sb->Oil,sb->MaxOil,sb->OilProd-sb->OilNeed );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1+16,ClWhite,buffer );
						sprintf ( dbstr,"g %d/%d %+d",sb->Gold,sb->MaxGold,sb->GoldProd-sb->GoldNeed );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1+24,ClWhite,buffer );
					}
				}
			}
			pos++;
			dest.x+=zoom;
		}
		dest.y+=zoom;
	}
	// Flugzeuge malen:
	dest.y=18-OffY+zoom*startY;
	scr.x=0;scr.y=0;
	scr.h=scr.w=zoom;
	if ( SettingsData.bAlphaEffects )
	{
		SDL_SetAlpha ( ActivePlayer->ShieldColor,SDL_SRCALPHA,150 );
	}
	else
	{
		SDL_SetAlpha ( ActivePlayer->ShieldColor,SDL_SRCALPHA,255 );
	}
	for ( y=startY;y<=endY;y++ )
	{
		dest.x=180-OffX+zoom*startX;
		pos=y*map->size+startX;
		for ( x=startX;x<=endX;x++ )
		{
			if ( ActivePlayer->ScanMap[pos] )
			{
				if ( map->GO[pos].plane )
				{
					map->GO[pos].plane->Draw ( &dest );
				}
				if ( hud->Status&&ActivePlayer->ShieldMap&&ActivePlayer->ShieldMap[pos] )
				{
					tmp=dest;
					SDL_BlitSurface ( ActivePlayer->ShieldColor,&scr,buffer,&tmp );
				}
			}
			pos++;
			dest.x+=zoom;
		}
		dest.y+=zoom;
	}
	// Ggf Ressourcen malen:
	if ( hud->Studie|| ( SelectedVehicle&&SelectedVehicle->owner==ActivePlayer&&SelectedVehicle->data.can_survey ) )
	{
		scr.y=0;
		scr.h=scr.w=zoom;
		dest.y=18-OffY+zoom*startY;
		for ( y=startY;y<=endY;y++ )
		{
			dest.x=180-OffX+zoom*startX;
			pos=y*map->size+startX;
			for ( x=startX;x<=endX;x++ )
			{
				if ( ActivePlayer->ResourceMap[pos] )
				{
					if ( map->Resources[pos].typ==RES_NONE )
					{
						scr.x=0;
						tmp=dest;
						SDL_BlitSurface ( ResourceData.res_metal,&scr,buffer,&tmp );
					}
					else
					{
						scr.x=map->Resources[pos].value*zoom;
						tmp=dest;
						if ( map->Resources[pos].typ==RES_METAL )
						{
							SDL_BlitSurface ( ResourceData.res_metal,&scr,buffer,&tmp );
						}
						else if ( map->Resources[pos].typ==RES_OIL )
						{
							SDL_BlitSurface ( ResourceData.res_oil,&scr,buffer,&tmp );
						}
						else
						{
							SDL_BlitSurface ( ResourceData.res_gold,&scr,buffer,&tmp );
						}
					}
				}
				pos++;
				dest.x+=zoom;
			}
			dest.y+=zoom;
		}
	}
	// Ggf den Path malen:
	if ( SelectedVehicle&& ( ( SelectedVehicle->mjob&&SelectedVehicle->mjob->Suspended ) ||SelectedVehicle->BuildPath ) )
	{
		SelectedVehicle->DrawPath();
	}
	// Ggf DebugWache:
	if ( DebugWache )
	{
		char dbstr[10];
		scr.y=0;
		scr.h=scr.w=zoom;
		dest.y=18-OffY+zoom*startY;
		for ( y=startY;y<=endY;y++ )
		{
			dest.x=180-OffX+zoom*startX;
			pos=y*map->size+startX;
			for ( x=startX;x<=endX;x++ )
			{
				if ( ActivePlayer->WachMapAir[pos] )
				{
					if ( ActivePlayer->ScanMap[pos] )
					{
						sprintf ( dbstr,"A+" );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1,ClYellow,buffer );
					}
					else
					{
						sprintf ( dbstr,"A-" );
						fonts->OutTextSmall ( dbstr,dest.x+1,dest.y+1,ClYellow,buffer );
					}
				}
				if ( ActivePlayer->WachMapGround[pos] )
				{
					if ( ActivePlayer->ScanMap[pos] )
					{
						sprintf ( dbstr,"G+" );
						fonts->OutTextSmall ( dbstr,dest.x+10,dest.y+1,ClRed,buffer );
					}
					else
					{
						sprintf ( dbstr,"G-" );
						fonts->OutTextSmall ( dbstr,dest.x+10,dest.y+1,ClRed,buffer );
					}
				}
				pos++;
				dest.x+=zoom;
			}
			dest.y+=zoom;
		}
	}
}

// Platziert ein Vehicle in dem Rechteck:
cVehicle *LandVehicle ( int x,int y,int w,int h,sVehicle *v,cPlayer *p )
{
	cVehicle *vptr=NULL;
	int i,k;
	for ( i=-h/2;i<h/2;i++ )
	{
		for ( k=-w/2;k<w/2;k++ )
		{
			if ( !FreeForLanding ( x+k,y+i ) )
			{
				continue;
			}
			game->engine->AddVehicle ( x+k,y+i,v,p,true );
			vptr=game->map->GO[x+k+ ( y+i ) *game->map->size].vehicle;
			return vptr;
		}
	}
	return vptr;
}

// Erzeugt alle Fahrzeuge an der Landestelle:
void cGame::MakeLanding ( int x,int y, cPlayer *p, TList *list, bool fixed )
{
	sLanding *ptr;
	cVehicle *v;
	int i,k,w,h;

	if ( p==game->ActivePlayer )
	{
		hud->OffX=x*64- ( ( int ) ( ( ( float ) 224/hud->Zoom ) *64 ) ) +32;
		hud->OffY=y*64- ( ( int ) ( ( ( float ) 224/hud->Zoom ) *64 ) ) +32;
		fDrawMap=true;
		hud->DoScroll ( 0 );
	}

	// Ggf Platz f�r die Mine finden:
	if ( fixed )
	{
		bool placed=false;
		cBuilding *b;
		w=2;h=2;
		while ( !placed )
		{
			for ( i=-h/2;i<h/2;i++ )
			{
				for ( k=-w/2;k<w/2;k++ )
				{
					if ( FreeForLanding ( x+k,y+i ) &&FreeForLanding ( x+k+1,y+i ) &&FreeForLanding ( x+k+2,y+i ) &&
					        FreeForLanding ( x+k,y+i+1 ) &&FreeForLanding ( x+k+1,y+i+1 ) &&FreeForLanding ( x+k+2,y+i+1 ) )
					{
						placed=true;
						// Rohstoffe platzieren:
						game->map->Resources[x+k+1+ ( y+i ) *game->map->size].value=10;
						game->map->Resources[x+k+1+ ( y+i ) *game->map->size].typ=RES_METAL;
						game->map->Resources[x+k+2+ ( y+i+1 ) *game->map->size].value=6;
						game->map->Resources[x+k+2+ ( y+i+1 ) *game->map->size].typ=RES_OIL;
						game->map->Resources[x+k+ ( y+i+1 ) *game->map->size].value=4;
						game->map->Resources[x+k+ ( y+i+1 ) *game->map->size].typ=RES_OIL;
						if ( y+i-1>=0 )
						{
							game->map->Resources[x+k+ ( y+i-1 ) *game->map->size].value=3;
							game->map->Resources[x+k+ ( y+i-1 ) *game->map->size].typ=RES_METAL;
							game->map->Resources[x+k+2+ ( y+i-1 ) *game->map->size].value=1;
							game->map->Resources[x+k+2+ ( y+i-1 ) *game->map->size].typ=RES_GOLD;
						}

						// Geb�ude platzieren:
						game->engine->AddBuilding ( x+k,y+i,UnitsData.building+BNrOilStore,p,true );
						game->engine->AddBuilding ( x+k,y+i+1,UnitsData.building+BNrSmallGen,p,true );
						game->engine->AddBuilding ( x+k+1,y+i,UnitsData.building+BNrMine,p,true );
						b=game->map->GO[x+k+ ( y+i ) *game->map->size].top;
						p->base->AddOil ( b->SubBase,4 );
						break;
					}
				}
				if ( placed ) break;
			}
			if ( placed ) break;
			w+=2;
			h+=2;
		}
	}

	w=2;h=2;
	for ( i=0;i<list->Count;i++ )
	{
		ptr=list->LandItems[i];
		v=LandVehicle ( x,y,w,h,UnitsData.vehicle+ptr->id,p );
		while ( !v )
		{
			w+=2;
			h+=2;
			v=LandVehicle ( x,y,w,h,UnitsData.vehicle+ptr->id,p );
		}
		if ( ptr->cargo&&v )
		{
			v->data.cargo=ptr->cargo;
		}
	}
}

// K�mmert sich um die Nachrichten:
void cGame::HandleMessages ( void )
{
	SDL_Rect scr,dest;
	int i,height;
	sMessage *m;
	if ( messages->Count==0 ) return;
	height=0;
	// Alle alten Nachrichten l�schen:
	for ( i=messages->Count-1;i>=0;i-- )
	{
		m=messages->MessageItems[i];
		if ( m->age+MSG_FRAMES<Frame||height>200 )
		{
			free ( m->msg );
			free ( m );
			messages->DeleteMessage ( i );
			/*for(int j=0;j<messages->Count;j++)
			  messages->MessageItems[j]=messages->MessageItems[j+1];*/
			continue;
		}
		height+=14+11*m->len/296;
	}
	if ( messages->Count==0 ) return;
	if ( SettingsData.bAlphaEffects )
	{
		scr.x=0;scr.y=0;
		dest.x=180;dest.y=30;
		dest.w=scr.w=250;
		dest.h=scr.h=height+6;
		SDL_BlitSurface ( GraphicsData.gfx_shadow,&scr,buffer,&dest );
	}
	dest.x=180+2;dest.y=34;
	dest.w=250-4;
	dest.h=height;
	for ( i=0;i<messages->Count;i++ )
	{
		m=messages->MessageItems[i];
		fonts->OutTextBlock ( m->msg,dest,buffer );
		dest.y+=14+11*m->len/300;
	}
}

// F�gt eine neue Nachricht ein:
void cGame::AddMessage ( const char *msg )
{
	sMessage *m;
	m= ( sMessage* ) malloc ( sizeof ( sMessage ) );
	m->chars= ( int ) strlen ( msg );
	m->msg= ( char* ) malloc ( m->chars+1 );
	strcpy ( m->msg,msg );
	if ( m->chars>500 ) m->msg[500]=0;
	m->len=fonts->GetTextLen ( msg );
	m->age=Frame;
	messages->AddMessage ( m );
}

// F�gt eine neue Nachricht ein:
void cGame::AddMessage ( std::string msg )
{
	AddMessage ( msg.c_str() );
}
// F�hrt das �bergebene Kommando aus, und gibt false zur�ck, falls es keins war:
bool cGame::DoCommand ( char *cmd )
{
	if ( strcmp ( cmd,"fps on" ) ==0 ) {DebugFPS=true;FPSstart=SDL_GetTicks();frames=0;cycles=0;return true;}
	if ( strcmp ( cmd,"fps off" ) ==0 ) {DebugFPS=false;return true;}
	/*if(strcmp(cmd,"com on")==0){if(engine->fstcpip){engine->fstcpip->ResetStats();DebugCom=true;Comstart=Comstart.CurrentTime();}return true;}
	if(strcmp(cmd,"com off")==0){DebugCom=false;return true;}*/
	if ( strcmp ( cmd,"base on" ) ==0 ) {DebugBase=true;return true;}
	if ( strcmp ( cmd,"base off" ) ==0 ) {DebugBase=false;return true;}
	if ( strcmp ( cmd,"wache on" ) ==0 ) {DebugWache=true;return true;}
	if ( strcmp ( cmd,"wache off" ) ==0 ) {DebugWache=false;return true;}
	if ( strcmp ( cmd,"fx on" ) ==0 ) {DebugFX=true;return true;}
	if ( strcmp ( cmd,"fx off" ) ==0 ) {DebugFX=false;return true;}
	// if(strcmp(cmd,"ping")==0){engine->Ping();return true;}
	if ( strcmp ( cmd,"trace on" ) ==0 ) {DebugTrace=true;return true;}
	if ( strcmp ( cmd,"trace off" ) ==0 ) {DebugTrace=false;return true;}
	if ( strcmp ( cmd,"log on" ) ==0 ) {DebugLog=true;engine->StartLog();return true;}
	if ( strcmp ( cmd,"log off" ) ==0 ) {DebugLog=false;engine->StopLog();return true;}
	if ( strcmp ( cmd,"show log" ) ==0 ) {ShowLog=true;return true;}
	if ( strcmp ( cmd,"hide log" ) ==0 ) {ShowLog=false;return true;}

	if ( strncmp ( cmd,"color ",6 ) ==0 ) {int cl=0;sscanf ( cmd,"color %d",&cl );cl%=8;ActivePlayer->color=OtherData.colors[cl];return true;}
	if ( strcmp ( cmd,"fog off" ) ==0 ) 
	{
		memset ( ActivePlayer->ScanMap,1,map->size*map->size );
		PlayerCheat=ActivePlayer->name; 
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"Fog Off\"";
		return true;
	}

	if ( strcmp ( cmd,"survey" ) ==0 ) 
	{
		memset ( ActivePlayer->ResourceMap,1,map->size*map->size );
		PlayerCheat=ActivePlayer->name; 
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"Survey\"";
		return true;
	}

	if ( strcmp ( cmd,"credits" ) ==0 )
	{
		ActivePlayer->Credits+=1000;
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"Credits\"";
		return true;
	}
	if ( strncmp ( cmd,"kill ",5 ) ==0 ) 
	{
		int x,y;
		sscanf ( cmd,"kill %d,%d",&x,&y );
		engine->DestroyObject ( x+y*map->size,false );
		engine->DestroyObject ( x+y*map->size,true );
		PlayerCheat=ActivePlayer->name; 
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"Kill\"";
		return true;
	}
	if ( strcmp ( cmd,"god off" ) ==0 ) 
	{
		int i;
		for ( i=0;i<map->size*map->size;i++ ) 
		{
			if ( map->GO[i].plane ) 
			{
				engine->DestroyObject ( i,true );
			}
			if ( map->GO[i].vehicle || ( map->GO[i].base&&map->GO[i].base->owner ) ||map->GO[i].top )
			{
				engine->DestroyObject ( i,false );
			}
			memset ( ActivePlayer->ScanMap,1,map->size*map->size );
		}
		PlayerCheat=ActivePlayer->name; 
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"God Off\"";
		return true;
	}
	if ( strcmp ( cmd,"load" ) ==0 )
	{
		PlayerCheat=ActivePlayer->name; 
		PlayerCheat+=" ";
		PlayerCheat+=lngPack.Translate( "Text~Comp~Cheat");
		PlayerCheat+=" \"Load\"";
		
		if ( SelectedVehicle ) {SelectedVehicle->data.cargo=SelectedVehicle->data.max_cargo;SelectedVehicle->data.ammo=SelectedVehicle->data.max_ammo;SelectedVehicle->ShowDetails();}
		else if ( SelectedBuilding )
		{
			if ( SelectedBuilding->data.can_load==TRANS_METAL )
			{
				SelectedBuilding->SubBase->Metal-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Metal+=SelectedBuilding->data.cargo;
			}
			else if ( SelectedBuilding->data.can_load==TRANS_OIL )
			{
				SelectedBuilding->SubBase->Oil-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Oil+=SelectedBuilding->data.cargo;
			}
			else if ( SelectedBuilding->data.can_load==TRANS_GOLD )
			{
				SelectedBuilding->SubBase->Gold-=SelectedBuilding->data.cargo;
				SelectedBuilding->data.cargo=SelectedBuilding->data.max_cargo;
				SelectedBuilding->SubBase->Gold+=SelectedBuilding->data.cargo;
			}
			SelectedBuilding->data.ammo=SelectedBuilding->data.max_ammo;SelectedBuilding->ShowDetails();
		}
		return true;
	}
	return false;
}

// F�gt einen FX-Effekt ein:
void cGame::AddFX ( eFXTyps typ,int x,int y,int param )
{
	sFX *n;
	n=new sFX;
	n->typ=typ;
	n->PosX=x;
	n->PosY=y;
	n->StartFrame=Frame;
	n->param=param;
	if ( typ==fxTracks||typ==fxTorpedo||typ==fxBubbles||typ==fxCorpse )
	{
		FXListBottom->AddFX ( n );
	}
	else
	{
		FXList->AddFX ( n );
	}
	switch ( typ )
	{
		case fxExploSmall0:
		case fxExploSmall1:
		case fxExploSmall2:
			if ( map->IsWater ( x/64+ ( y/64 ) *map->size ) )
			{
				int nr;
				nr=random ( 3,0 );
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPSmallWet0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPSmallWet1 );
				}
				else
				{
					PlayFX ( SoundData.EXPSmallWet2 );
				}
			}
			else
			{
				int nr;
				nr=random ( 3,0 );
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPSmall0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPSmall1 );
				}
				else
				{
					PlayFX ( SoundData.EXPSmall2 );
				}
			}
			break;
		case fxExploBig0:
		case fxExploBig1:
		case fxExploBig2:
		case fxExploBig3:
		case fxExploBig4:
			if ( map->IsWater ( x/64+ ( y/64 ) *map->size ) )
			{
				if ( random ( 2,0 ) )
				{
					PlayFX ( SoundData.EXPBigWet0 );
				}
				else
				{
					PlayFX ( SoundData.EXPBigWet1 );
				}
			}
			else
			{
				int nr;
				nr=random ( 4,0 );
				if ( nr==0 )
				{
					PlayFX ( SoundData.EXPBig0 );
				}
				else if ( nr==1 )
				{
					PlayFX ( SoundData.EXPBig1 );
				}
				else if ( nr==2 )
				{
					PlayFX ( SoundData.EXPBig2 );
				}
				else
				{
					PlayFX ( SoundData.EXPBig3 );
				}
			}
			break;
		case fxSmoke:
		case fxBubbles:
			n->param=0;
			break;
		case fxRocket:
		case fxTorpedo:
		{
			sFXRocketInfos *ri;
			int dx,dy;
			ri= ( sFXRocketInfos* ) ( n->param );
			ri->fpx=n->PosX;
			ri->fpy=n->PosY;
			dx=ri->ScrX-ri->DestX;
			dy=ri->ScrY-ri->DestY;
			if ( abs ( dx ) >abs ( dy ) )
			{
				if ( ri->ScrX>ri->DestX ) ri->mx=-1;
				else ri->mx=1;
				ri->my=dy/ ( float ) dx* ( -ri->mx );
			}
			else
			{
				if ( ri->ScrY<ri->DestY ) ri->my=-1;
				else ri->my=1;
				ri->mx=dx/ ( float ) dy* ( -ri->my );
			}
			break;
		}
		case fxDarkSmoke:
		{
			float x,y,ax,ay;
			sFXDarkSmoke *dsi;
			dsi=new sFXDarkSmoke;
			dsi->alpha=n->param;
			if ( dsi->alpha>150 ) dsi->alpha=150;
			n->param= ( int ) dsi;
			dsi->fx=n->PosX;
			dsi->fy=n->PosY;

			ax=x=sin ( WindDir );
			ay=y=cos ( WindDir );
			if ( ax<0 ) ax=-ax;
			if ( ay<0 ) ay=-ay;
			if ( ax>ay )
			{
				dsi->dx=x*2+ ( random ( 5,0 ) /10.0 );
				dsi->dy=y*2+ ( ( random ( 15,0 )-7 ) /14.0 );
			}
			else
			{
				dsi->dx=x*2+ ( ( random ( 15,0 )-7 ) /14.0 );
				dsi->dy=y*2+ ( random ( 5,0 ) /10.0 );
			}
			break;
		}
		case fxTracks:
		{
			sFXTracks *tri;
			tri=new sFXTracks;
			tri->alpha=100;
			tri->dir=n->param;
			n->param= ( int ) tri;
			break;
		}
		case fxCorpse:
			n->param=255;
			break;
		case fxAbsorb:
			PlayFX ( SoundData.SNDAbsorb );
			break;
	}
}

// Spielt alle FX-Effekte ab:
void cGame::DisplayFX ( void )
{
	int i;
	if ( !FXList->Count ) return;

	for ( i=FXList->Count-1;i>=0;i-- )
	{
		DrawFX ( i );
	}
}

// Spielt alle FX-Effekte ab:
void cGame::DisplayFXBottom ( void )
{
	int i;
	if ( !FXListBottom->Count ) return;

	for ( i=FXListBottom->Count-1;i>=0;i-- )
	{
		DrawFXBottom ( i );
	}
}

// Malt das FX-Objekt mit der �bergebenen Nummer:
void cGame::DrawFX ( int i )
{
	SDL_Rect scr,dest;
	sFX *fx;

	fx=FXList->FXItems[i];
	if ( ( !ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*map->size] ) &&fx->typ!=fxRocket ) return;
	switch ( fx->typ )
	{
		case fxMuzzleBig:
			if ( Frame-fx->StartFrame>2 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*fx->param;
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_big[1],&scr,buffer,&dest );
			break;
		case fxMuzzleSmall:
			if ( Frame-fx->StartFrame>2 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*fx->param;
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_small[1],&scr,buffer,&dest );
			break;
		case fxMuzzleMed:
			if ( Frame-fx->StartFrame>2 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*fx->param;
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_med[1],&scr,buffer,&dest );
			break;
		case fxMuzzleMedLong:
			if ( Frame-fx->StartFrame>5 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*fx->param;
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_muzzle_med[1],&scr,buffer,&dest );
			break;
		case fxHit:
			if ( Frame-fx->StartFrame>5 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_hit[1],&scr,buffer,&dest );
			break;
		case fxExploSmall0:
			if ( Frame-fx->StartFrame>11 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-32 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-32 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_small0[1],&scr,buffer,&dest );
			break;
		case fxExploSmall1:
			if ( Frame-fx->StartFrame>20 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_small1[1],&scr,buffer,&dest );
			break;
		case fxExploSmall2:
			if ( Frame-fx->StartFrame>10 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_small2[1],&scr,buffer,&dest );
			break;
		case fxExploBig0:
			if ( Frame-fx->StartFrame>14 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big0[1],&scr,buffer,&dest );
			break;
		case fxExploBig1:
			if ( Frame-fx->StartFrame>13 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big1[1],&scr,buffer,&dest );
			break;
		case fxExploBig2:
			if ( Frame-fx->StartFrame>15 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big2[1],&scr,buffer,&dest );
			break;
		case fxExploBig3:
			if ( Frame-fx->StartFrame>8 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big3[1],&scr,buffer,&dest );
			break;
		case fxExploBig4:
			if ( Frame-fx->StartFrame>10 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom*2* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom*2;
			dest.h=scr.h=hud->Zoom*2;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-64 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-64 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_explo_big4[1],&scr,buffer,&dest );
			break;
		case fxSmoke:
			if ( Frame-fx->StartFrame>100/4 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			SDL_SetAlpha ( EffectsData.fx_smoke[1],SDL_SRCALPHA,100- ( Frame-fx->StartFrame ) *4 );
			scr.y=scr.x=0;
			dest.w=scr.w=EffectsData.fx_smoke[1]->h;
			dest.h=scr.h=EffectsData.fx_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_smoke[1],&scr,buffer,&dest );
			break;
		case fxRocket:
		{
			sFXRocketInfos *ri;
			ri= ( sFXRocketInfos* ) fx->param;
			if ( abs ( fx->PosX-ri->DestX ) <64&&abs ( fx->PosY-ri->DestY ) <64 )
			{
				ri->aj->MuzzlePlayed=true;
				delete ri;
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			if ( timer0 )
			{
				int k;
				for ( k=0;k<64;k+=8 )
				{
					if ( SettingsData.bAlphaEffects ) AddFX ( fxSmoke, ( int ) ri->fpx, ( int ) ri->fpy,0 );
					ri->fpx+=ri->mx*8;
					ri->fpy-=ri->my*8;
					DrawFX ( FXList->Count-1 );
				}
			}

			fx->PosX= ( int ) ri->fpx;
			fx->PosY= ( int ) ri->fpy;
			scr.x=ri->dir*EffectsData.fx_rocket[1]->h;
			scr.y=0;
			scr.h=scr.w=dest.h=dest.w=EffectsData.fx_rocket[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_rocket[1],&scr,buffer,&dest );
			break;
		}
		case fxDarkSmoke:
		{
			sFXDarkSmoke *dsi;
			dsi= ( sFXDarkSmoke* ) ( fx->param );
			if ( Frame-fx->StartFrame>50||dsi->alpha<=1 )
			{
				delete fx;
				delete dsi;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x= ( int ) ( 0.375*hud->Zoom ) * ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=EffectsData.fx_dark_smoke[1]->h;
			dest.h=scr.h=EffectsData.fx_dark_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( ( int ) dsi->fx ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( ( int ) dsi->fy ) ) / ( 64.0/hud->Zoom ) ) );

			SDL_SetAlpha ( EffectsData.fx_dark_smoke[1],SDL_SRCALPHA,dsi->alpha );
			SDL_BlitSurface ( EffectsData.fx_dark_smoke[1],&scr,buffer,&dest );

			if ( timer0 )
			{
				dsi->fx+=dsi->dx;
				dsi->fy+=dsi->dy;
				dsi->alpha-=3;
				if ( dsi->alpha<=0 ) dsi->alpha=1;
			}
			break;
		}
		case fxAbsorb:
		{
			if ( Frame-fx->StartFrame>10 )
			{
				delete fx;
				FXList->DeleteFX ( i );
				return;
			}
			scr.x=hud->Zoom* ( Frame-fx->StartFrame );
			scr.y=0;
			dest.w=scr.w=hud->Zoom;
			dest.h=scr.h=hud->Zoom;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_absorb[1],&scr,buffer,&dest );
			break;
		}
	}
}

// Malt das FX-Objekt mit der �bergebenen Nummer:
void cGame::DrawFXBottom ( int i )
{
	SDL_Rect scr,dest;
	sFX *fx;

	fx=FXListBottom->FXItems[i];
	if ( ( !ActivePlayer->ScanMap[fx->PosX/64+fx->PosY/64*map->size] ) &&fx->typ!=fxTorpedo ) return;
	switch ( fx->typ )
	{
		case fxTorpedo:
		{
			sFXRocketInfos *ri;
			ri= ( sFXRocketInfos* ) fx->param;
			int x,y;
			if ( abs ( fx->PosX-ri->DestX ) <64&&abs ( fx->PosY-ri->DestY ) <64 )
			{
				ri->aj->MuzzlePlayed=true;
				delete ri;
				delete fx;
				FXListBottom->DeleteFX ( i );
				return;
			}

			if ( timer0 )
			{
				int k;
				for ( k=0;k<64;k+=8 )
				{
					if ( SettingsData.bAlphaEffects ) AddFX ( fxBubbles, ( int ) ri->fpx, ( int ) ri->fpy,0 );
					ri->fpx+=ri->mx*8;
					ri->fpy-=ri->my*8;
					DrawFXBottom ( FXListBottom->Count-1 );
				}
			}

			fx->PosX= ( int ) ( ri->fpx );
			fx->PosY= ( int ) ( ri->fpy );
			scr.x=ri->dir*EffectsData.fx_rocket[1]->h;
			scr.y=0;
			scr.h=scr.w=dest.h=dest.w=EffectsData.fx_rocket[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-EffectsData.fx_rocket[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_rocket[1],&scr,buffer,&dest );

			x= ( ( int ) ( ( ( dest.x-180 ) +hud->OffX/ ( 64.0/hud->Zoom ) ) /hud->Zoom ) );
			y= ( ( int ) ( ( ( dest.y-18 ) +hud->OffY/ ( 64.0/hud->Zoom ) ) /hud->Zoom ) );

			if ( !map->IsWater ( x+y*map->size,false ) &&
			        ! ( abs ( fx->PosX-ri->DestX ) <64&&abs ( fx->PosY-ri->DestY ) <64 ) &&
			        ! ( map->GO[x+y*map->size].base&&map->GO[x+y*map->size].base->owner&& ( map->GO[x+y*map->size].base->data.is_bridge||map->GO[x+y*map->size].base->data.is_platform ) ) )
			{
				ri->aj->DestX=ri->aj->ScrX;
				ri->aj->DestY=ri->aj->ScrY;
				ri->aj->MuzzlePlayed=true;
				delete ri;
				delete fx;
				FXListBottom->DeleteFX ( i );
				return;
			}
			break;
		}
		case fxTracks:
		{
			sFXTracks *tri;
			tri= ( sFXTracks* ) ( fx->param );
			if ( tri->alpha<=1 )
			{
				delete fx;
				delete tri;
				FXListBottom->DeleteFX ( i );
				return;
			}
			scr.y=0;
			dest.w=scr.w=dest.h=scr.h=EffectsData.fx_tracks[1]->h;
			scr.x=tri->dir*scr.w;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_SetAlpha ( EffectsData.fx_tracks[1],SDL_SRCALPHA,tri->alpha );
			SDL_BlitSurface ( EffectsData.fx_tracks[1],&scr,buffer,&dest );

			if ( timer0 )
			{
				tri->alpha--;
			}
			break;
		}
		case fxBubbles:
			if ( Frame-fx->StartFrame>100/4 )
			{
				delete fx;
				FXListBottom->DeleteFX ( i );
				return;
			}
			SDL_SetAlpha ( EffectsData.fx_smoke[1],SDL_SRCALPHA,100- ( Frame-fx->StartFrame ) *4 );
			scr.y=scr.x=0;
			dest.w=scr.w=EffectsData.fx_smoke[1]->h;
			dest.h=scr.h=EffectsData.fx_smoke[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX- ( fx->PosX-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY- ( fx->PosY-EffectsData.fx_smoke[0]->h/2+32 ) ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_smoke[1],&scr,buffer,&dest );
			break;
		case fxCorpse:
			SDL_SetAlpha ( EffectsData.fx_corpse[1],SDL_SRCALPHA,fx->param-- );
			scr.y=scr.x=0;
			dest.w=scr.w=EffectsData.fx_corpse[1]->h;
			dest.h=scr.h=EffectsData.fx_corpse[1]->h;
			dest.x=180- ( ( int ) ( ( hud->OffX-fx->PosX ) / ( 64.0/hud->Zoom ) ) );
			dest.y=18- ( ( int ) ( ( hud->OffY-fx->PosY ) / ( 64.0/hud->Zoom ) ) );
			SDL_BlitSurface ( EffectsData.fx_corpse[1],&scr,buffer,&dest );

			if ( fx->param<=0 )
			{
				delete fx;
				FXListBottom->DeleteFX ( i );
				return;
			}
			break;
	}
}

// F�gt etwas Dreck ein:
void cGame::AddDirt ( int x,int y,int value,bool big )
{
	cBuilding *n;
	if ( value<=0 ) value=1;
	if ( TerrainData.terrain[map->Kacheln[x+y*map->size]].water||TerrainData.terrain[map->Kacheln[x+y*map->size]].coast ) return;
	if ( big&& ( TerrainData.terrain[map->Kacheln[x+1+y*map->size]].water||TerrainData.terrain[map->Kacheln[x+ ( y+1 ) *map->size]].water||TerrainData.terrain[map->Kacheln[x+1+ ( y+1 ) *map->size]].water ) ) return;
	n=new cBuilding ( NULL,NULL,NULL );
	n->next=DirtList;
	DirtList=n;
	n->prev=NULL;
	map->GO[x+y*map->size].base=n;
	n->PosX=x;n->PosY=y;
	if ( big )
	{
		map->GO[x+1+y*map->size].base=n;
		map->GO[x+1+ ( y+1 ) *map->size].base=n;
		map->GO[x+ ( y+1 ) *map->size].base=n;
		n->DirtTyp=random ( 2,0 );
		n->data.is_big=true;
	}
	else
	{
		n->DirtTyp=random ( 3,0 );
		n->data.is_big=false;
	}
	n->DirtValue=value;
	n->BigDirt=big;
}

// Malt einen Exitpoint:
void cGame::DrawExitPoint ( int x,int y )
{
	SDL_Rect dest, scr;
	int nr;
	int zoom;
	nr=Frame%5;
	zoom = hud->Zoom;
	scr.y=0;
	scr.h=scr.w=zoom;
	scr.x=zoom*nr;
	dest.y=y;
	dest.x=x;
	dest.w=dest.h=zoom;
	SDL_BlitSurface ( GraphicsData.gfx_exitpoints,&scr,buffer,&dest );
}

// L�scht den Dreck:
void cGame::DeleteDirt ( cBuilding *ptr )
{
	if ( ptr->BigDirt )
	{
		map->GO[ptr->PosX+1+ptr->PosY*map->size].base=NULL;
		map->GO[ptr->PosX+1+ ( ptr->PosY+1 ) *map->size].base=NULL;
		map->GO[ptr->PosX+ ( ptr->PosY+1 ) *map->size].base=NULL;
	}
	map->GO[ptr->PosX+ptr->PosY*map->size].base=NULL;
	if ( ptr->prev==NULL )
	{
		DirtList=ptr->next;
	}
	else
	{
		ptr->prev->next=ptr->next;
	}
	delete ptr;
}

// Setzt die Windrichtuing neu (0-360):
void cGame::SetWind ( int dir )
{
	WindDir=dir/57.29577;
}

// Macht die Panel-Annimation:
void cGame::MakePanel ( bool open )
{
	SDL_Rect top,bottom,tmp;
	if ( open )
	{
		PlayFX ( SoundData.SNDPanelOpen );
		top.x=0;top.y= ( SettingsData.iScreenH/2 )-479;
		top.w=bottom.w=171;
		top.h=479;bottom.h=481;
		bottom.x=0;bottom.y= ( SettingsData.iScreenH/2 );
		tmp=top;
		SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
		tmp=bottom;
		SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&tmp );
		while ( top.y>-479 )
		{
			SHOW_SCREEN
			SDL_Delay ( 10 );
			top.y-=10;
			bottom.y+=10;
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			tmp=top;
			SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
			SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&bottom );
		}
	}
	else
	{
		PlayFX ( SoundData.SNDPanelClose );
		top.x=0;top.y=-480;
		top.w=bottom.w=171;
		top.h=479;bottom.h=481;
		bottom.x=0;bottom.y=SettingsData.iScreenH;
		while ( bottom.y>SettingsData.iScreenH/2 )
		{
			SHOW_SCREEN
			SDL_Delay ( 10 );
			top.y+=10;
			if ( top.y> ( SettingsData.iScreenH/2 )-479-9 ) top.y= ( SettingsData.iScreenH/2 )-479;
			bottom.y-=10;
			if ( bottom.y<SettingsData.iScreenH/2+9 ) bottom.y=SettingsData.iScreenH/2;
			SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
			tmp=top;
			SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&tmp );
			tmp=bottom;
			SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&tmp );
		}
		SHOW_SCREEN
		SDL_Delay ( 100 );
	}
}

// Timer /////////////////////////////////////////////////////////////////////
// Wird regelm��ig aufgerufen um Zeitabh�ngige Operationen durchzuf�hren:
Uint32 Timer ( Uint32 interval, void *param )
{
	TimerTime++;
	return interval;
}

// HandleTimer ///////////////////////////////////////////////////////////////
// K�mmert sich um alle Timer:
void cGame::HandleTimer ( void )
{
	static unsigned int last=0,i=0;
	timer0=0;timer1=0;timer2=0;
	if ( TimerTime!=last )
	{
		last=TimerTime;
		i++;
		timer0=1;
		if ( i&0x1 ) timer1=1;
		if ( ( i&0x3 ) ==3 ) timer2=1;
	}
}

// DrawMiniMap ///////////////////////////////////////////////////////////////
// Malt die MiniMap:
void cGame::DrawMiniMap ( bool pure,SDL_Surface *sf )
{
	unsigned int cl,*ptr;
	int x,y,tx,ty,ex,ey;
	sGameObjects *GO;

	GO=map->GO;
	if ( !sf )
	{
		SDL_LockSurface ( GraphicsData.gfx_hud );
		ptr= ( ( unsigned int* ) GraphicsData.gfx_hud->pixels );
	}
	else
	{
		SDL_LockSurface ( sf );
		ptr= ( ( unsigned int* ) sf->pixels );
	}
	// Den Hintregrund malen:
	for ( y=0;y<112;y++ )
	{
		ty= ( int ) ( ( map->size/112.0 ) *y );
		ty*=map->size;
		for ( x=0;x<112;x++ )
		{
			tx= ( int ) ( ( map->size/112.0 ) *x );
			if ( !pure )
			{
				if ( hud->Radar&&!ActivePlayer->ScanMap[tx+ty] )
				{
					cl=* ( unsigned int* ) TerrainData.terrain[map->Kacheln[tx+ty]].shw_org->pixels;
				}
				else if ( ActivePlayer->ScanMap[tx+ty]&&GO[tx+ty].base&&GO[tx+ty].base->detected&&ActivePlayer->ScanMap[tx+ty]&&GO[tx+ty].base->owner&& ( !hud->TNT|| ( GO[tx+ty].base->data.can_attack ) ) )
				{
					cl=* ( unsigned int* ) GO[tx+ty].base->owner->color->pixels;
				}
				else if ( ActivePlayer->ScanMap[tx+ty]&&GO[tx+ty].top&& ( !hud->TNT|| ( GO[tx+ty].top->data.can_attack ) ) )
				{
					cl=* ( unsigned int* ) GO[tx+ty].top->owner->color->pixels;
				}
				else if ( ActivePlayer->ScanMap[tx+ty]&&GO[tx+ty].plane&& ( !hud->TNT|| ( GO[tx+ty].plane->data.can_attack ) ) )
				{
					cl=* ( unsigned int* ) GO[tx+ty].plane->owner->color->pixels;
				}
				else if ( ActivePlayer->ScanMap[tx+ty]&&GO[tx+ty].vehicle&&GO[tx+ty].vehicle->detected&& ( !hud->TNT|| ( GO[tx+ty].vehicle->data.can_attack ) ) )
				{
					cl=* ( unsigned int* ) GO[tx+ty].vehicle->owner->color->pixels;
				}
				else
				{
					cl=* ( unsigned int* ) TerrainData.terrain[map->Kacheln[tx+ty]].sf_org->pixels;
				}
			}
			else
			{
				cl=* ( unsigned int* ) TerrainData.terrain[map->Kacheln[tx+ty]].sf_org->pixels;
			}
			if ( cl==0xFF00FF )
			{
				cl=* ( unsigned int* ) TerrainData.terrain[map->DefaultWater].sf_org->pixels;
			}
			else if ( ( cl&0xFFFFFF ) ==0xCD00CD )
			{
				cl=* ( unsigned int* ) TerrainData.terrain[map->DefaultWater].shw_org->pixels;
			}
			if ( !sf )
			{
				ptr[15+356*GraphicsData.gfx_hud->w+x+y*GraphicsData.gfx_hud->w]=cl;
			}
			else
			{
				ptr[x+y*sf->w]=cl;
			}
		}
	}
	if ( !sf )
	{
		// Den Rahmen malen:
		tx= ( int ) ( ( hud->OffX/64.0 ) * ( 112.0/map->size ) );
		ty= ( int ) ( ( hud->OffY/64.0 ) * ( 112.0/map->size ) );
//    ex=112/(map->size/((448.0/hud->Zoom)));
		ex= ( int ) ( 112/ ( map->size/ ( ( ( SettingsData.iScreenW-192.0 ) /hud->Zoom ) ) ) );
		ey= ( int ) ( ty+112/ ( map->size/ ( ( ( SettingsData.iScreenH-32.0 ) /hud->Zoom ) ) ) );
		ex+=tx;
		for ( y=ty;y<ey;y++ )
		{
			ptr[y*GraphicsData.gfx_hud->w+15+356*GraphicsData.gfx_hud->w+tx]=MINIMAP_COLOR;
			ptr[y*GraphicsData.gfx_hud->w+15+356*GraphicsData.gfx_hud->w+tx+ex-tx-1]=MINIMAP_COLOR;
		}
		for ( x=tx;x<ex;x++ )
		{
			ptr[x+15+356*GraphicsData.gfx_hud->w+ty*GraphicsData.gfx_hud->w]=MINIMAP_COLOR;
			ptr[x+15+356*GraphicsData.gfx_hud->w+ ( ty+ey-1-ty ) *GraphicsData.gfx_hud->w]=MINIMAP_COLOR;
		}
	}
	if ( !sf )
	{
		SDL_UnlockSurface ( GraphicsData.gfx_hud );
	}
	else
	{
		SDL_UnlockSurface ( sf );
	}
}

// MouseMoveCallback /////////////////////////////////////////////////////////
// Wird aufgerufen, wenn die Maus bewegt wurde:
void MouseMoveCallback ( bool force )
{
	static int lx=-1,ly=-1;
	SDL_Rect scr,dest;
	sGameObjects *GO;
	char str[100];
	int x,y;
	mouse->GetKachel ( &x,&y );
	if ( x==lx&&y==ly&&!force ) return;
	lx=x;ly=y;
	// Das Hud wieder herstellen:
	scr.x=262;
	scr.y=25;
	scr.w=64;
	scr.h=16;
	dest.x=265;
	dest.y=SettingsData.iScreenH-21;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	scr.x=64;
	scr.y=198;
	scr.w=215;
	dest.x=342;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	if ( x==-1 )
	{
		return;
	}
	// Die Koordinaten malen:
	sprintf ( str, "%0.3d-%0.3d", x, y );
	fonts->OutTextCenter ( str,265+32, ( SettingsData.iScreenH-21 ) +4,GraphicsData.gfx_hud );

	if ( !game->ActivePlayer->ScanMap[x+y*game->map->size] )
	{
		game->OverObject=NULL;
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			SDL_Rect r;
			r.x=1;r.y=29;
			r.h=3;r.w=35;
			SDL_FillRect ( GraphicsData.gfx_Cattack,&r,0 );
		}
		return;
	}
	// Pr�fen, ob ein GO unter der Maus ist:
	GO=game->map->GO+ ( game->map->size*y+x );
	if ( mouse->cur==GraphicsData.gfx_Csteal&&game->SelectedVehicle )
	{
		game->SelectedVehicle->DrawCommandoCursor ( GO,true );
	}
	else if ( mouse->cur==GraphicsData.gfx_Cdisable&&game->SelectedVehicle )
	{
		game->SelectedVehicle->DrawCommandoCursor ( GO,false );
	}
	if ( GO->vehicle!=NULL&& ( GO->vehicle->detected||GO->vehicle->owner==game->ActivePlayer ) )
	{
		game->OverObject=GO;
		fonts->OutTextCenter ( ( char * ) GO->vehicle->name.c_str(),343+106, ( SettingsData.iScreenH-21 ) +4,GraphicsData.gfx_hud );
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( game->SelectedVehicle )
			{
				game->SelectedVehicle->DrawAttackCursor ( GO,game->SelectedVehicle->data.can_attack );
			}
			else if ( game->SelectedBuilding )
			{
				game->SelectedBuilding->DrawAttackCursor ( GO,game->SelectedBuilding->data.can_attack );
			}
		}
	}
	else if ( GO->plane!=NULL )
	{
		game->OverObject=GO;
		fonts->OutTextCenter ( ( char * ) GO->plane->name.c_str(),343+106, ( SettingsData.iScreenH-21 ) +4,GraphicsData.gfx_hud );
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( game->SelectedVehicle )
			{
				game->SelectedVehicle->DrawAttackCursor ( GO,game->SelectedVehicle->data.can_attack );
			}
			else if ( game->SelectedBuilding )
			{
				game->SelectedBuilding->DrawAttackCursor ( GO,game->SelectedBuilding->data.can_attack );
			}
		}
	}
	else if ( GO->top!=NULL )
	{
		game->OverObject=GO;
		fonts->OutTextCenter ( ( char * ) GO->top->name.c_str(),343+106, ( SettingsData.iScreenH-21 ) +4,GraphicsData.gfx_hud );
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( game->SelectedBuilding )
			{
				game->SelectedBuilding->DrawAttackCursor ( GO,game->SelectedBuilding->data.can_attack );
			}
		}
	}
	else if ( GO->base!=NULL&&GO->base->owner&&GO->base->detected )
	{
		game->OverObject=GO;
		fonts->OutTextCenter ( ( char * ) GO->base->name.c_str(),343+106, ( SettingsData.iScreenH-21 ) +4,GraphicsData.gfx_hud );
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			if ( game->SelectedBuilding )
			{
				game->SelectedBuilding->DrawAttackCursor ( GO,game->SelectedBuilding->data.can_attack );
			}
		}
	}
	else
	{
		// Ggf den AttackCursor neu malen:
		if ( mouse->cur==GraphicsData.gfx_Cattack )
		{
			SDL_Rect r;
			r.x=1;r.y=29;
			r.h=3;r.w=35;
			SDL_FillRect ( GraphicsData.gfx_Cattack,&r,0 );
		}
		game->OverObject=NULL;
	}
	// Band platzieren:
	if ( game->SelectedVehicle&&game->SelectedVehicle->PlaceBand )
	{
		game->SelectedVehicle->FindNextband();
	}
}

// RotateBlinkColor //////////////////////////////////////////////////////////
// Erzeugt eine neue Blink-Farbe:
void cGame::RotateBlinkColor ( void )
{
	static bool dec=true;
	if ( dec )
	{
		BlinkColor-=0x0A0A0A;
		if ( BlinkColor<=0xA0A0A0 ) dec=false;
	}
	else
	{
		BlinkColor+=0x0A0A0A;
		if ( BlinkColor>=0xFFFFFF ) dec=true;
	}
}

// DrawFLC ///////////////////////////////////////////////////////////////////
// K�mmert sich um das FLC-Video:
void cGame::DrawFLC ( void )
{
	SDL_Rect dest;
	string stmp;
	if ( ( FLC==NULL&&video==NULL ) || ( SelectedVehicle==NULL&&SelectedBuilding==NULL ) ) return;
	// Das Video malen:
	dest.x=10;
	dest.y=29;
	dest.w=128;
	dest.h=128;
	if ( FLC )
	{
		SDL_BlitSurface ( FLC->surface,NULL,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( video,NULL,buffer,&dest );
	}
	// Den Namen des Objektes malen:
	if ( SelectedVehicle )
	{
		if ( ChangeObjectName )
		{
			dest.y+=2;
			dest.h=6;
			SDL_FillRect ( buffer,&dest,0x404040 );
			if ( Frame%2 )
			{
				stmp = InputStr; stmp += "_";
				fonts->OutTextSmall ( ( char * ) stmp.c_str(),10,32,ClGreen,buffer );
			}
			else
			{
				fonts->OutTextSmall ( ( char * ) InputStr.c_str(),10,32,ClGreen,buffer );
			}
		}
		else
		{
			fonts->OutTextSmall ( ( char * ) SelectedVehicle->name.c_str(),10,32,ClGreen,buffer );
		}
		fonts->OutTextSmall ( ( char * ) SelectedVehicle->GetStatusStr(),10,40,ClWhite,buffer );
	}
	else if ( SelectedBuilding )
	{
		if ( ChangeObjectName )
		{
			dest.y+=2;
			dest.h=6;
			SDL_FillRect ( buffer,&dest,0x404040 );
			if ( Frame%2 )
			{
				stmp = InputStr; stmp += "_";
				fonts->OutTextSmall ( ( char * ) stmp.c_str(),10,32,ClGreen,buffer );
			}
			else
			{
				fonts->OutTextSmall ( ( char * ) InputStr.c_str(),10,32,ClGreen,buffer );
			}
		}
		else
		{
			fonts->OutTextSmall ( ( char * ) SelectedBuilding->name.c_str(),10,32,ClGreen,buffer );
		}
		fonts->OutTextSmall ( ( char * ) SelectedBuilding->GetStatusStr(),10,40,ClWhite,buffer );
	}
}

// Malt einen Kreis auf das Surface:
void DrawCircle ( int x,int y,int r,int color,SDL_Surface *sf )
{
	int d,da,db,xx,yy,bry;
	unsigned int *ptr;
	if ( x+r<0||x-r>SettingsData.iScreenW||y+r<0||y-r>SettingsData.iScreenH ) return;
	SDL_LockSurface ( sf );
	ptr= ( unsigned int* ) sf->pixels;
	y*=SettingsData.iScreenW;

	d=0;
	xx=0;
	yy=r;
	bry= ( int ) Round ( 0.70710678*r,0 );
	while ( yy>bry )
	{
		da=d+ ( xx<<1 ) +1;
		db=da- ( yy<<1 ) +1;
		if ( abs ( da ) <abs ( db ) )
		{
			d=da;
			xx++;
		}
		else
		{
			d=db;
			xx++;
			yy--;
		}
#define PUTC(xxx,yyy) if((xxx)+x>=0&&(xxx)+x<SettingsData.iScreenW&&(yyy)*SettingsData.iScreenW+y>=0&&(yyy)*SettingsData.iScreenW+y<SettingsData.iScreenH*SettingsData.iScreenW)ptr[(xxx)+x+(yyy)*SettingsData.iScreenW+y]=color;
		PUTC ( xx,yy )
		PUTC ( yy,xx )
		PUTC ( yy,-xx )
		PUTC ( xx,-yy )
		PUTC ( -xx,yy )
		PUTC ( -yy,xx )
		PUTC ( -yy,-xx )
		PUTC ( -xx,-yy )
	}
	SDL_UnlockSurface ( sf );
}

// F�llt die Map mit einem gro�en Kreis:
void SpecialCircleBig ( int x,int y,int r,char *map )
{
	float w=0.017453*45,i,step;
	int rx,ry,x1,x2,k;
	if ( r ) r--;
	if ( !r ) return;
	r*=10;
	step=0.017453*90-acos ( 1.0/r );
	step/=2;
	for ( i=0;i<=w;i+=step )
	{
		rx= ( int ) ( cos ( i ) *r );
		ry= ( int ) ( sin ( i ) *r );
		rx/=10;ry/=10;

		x1=rx+x;x2=-rx+x;
		for ( k=x2;k<=x1+1;k++ )
		{
			if ( k<0 ) continue;
			if ( k>=game->map->size ) break;
			if ( y+ry>=0&&y+ry<game->map->size )
				map[k+ ( y+ry ) *game->map->size]|=1;
			if ( y-ry>=0&&y-ry<game->map->size )
				map[k+ ( y-ry ) *game->map->size]|=1;

			if ( y+ry+1>=0&&y+ry+1<game->map->size )
				map[k+ ( y+ry+1 ) *game->map->size]|=1;
			if ( y-ry+1>=0&&y-ry+1<game->map->size )
				map[k+ ( y-ry+1 ) *game->map->size]|=1;
		}

		x1=ry+x;x2=-ry+x;
		for ( k=x2;k<=x1+1;k++ )
		{
			if ( k<0 ) continue;
			if ( k>=game->map->size ) break;
			if ( y+rx>=0&&y+rx<game->map->size )
				map[k+ ( y+rx ) *game->map->size]|=1;
			if ( y-rx>=0&&y-rx<game->map->size )
				map[k+ ( y-rx ) *game->map->size]|=1;

			if ( y+rx+1>=0&&y+rx+1<game->map->size )
				map[k+ ( y+rx+1 ) *game->map->size]|=1;
			if ( y-rx+1>=0&&y-rx+1<game->map->size )
				map[k+ ( y-rx+1 ) *game->map->size]|=1;
		}
	}
}

// F�llt die Map mit einem Kreis:
void SpecialCircle ( int x,int y,int r,char *map )
{
	float w=0.017453*45,i,step;
	int rx,ry,x1,x2,k;
	if ( !r ) return;
	r*=10;
	step=0.017453*90-acos ( 1.0/r );
	step/=2;
	for ( i=0;i<=w;i+=step )
	{
		rx= ( int ) ( cos ( i ) *r );
		ry= ( int ) ( sin ( i ) *r );
		rx/=10;ry/=10;

		x1=rx+x;x2=-rx+x;
		for ( k=x2;k<=x1;k++ )
		{
			if ( k<0 ) continue;
			if ( k>=game->map->size ) break;
			if ( y+ry>=0&&y+ry<game->map->size )
				map[k+ ( y+ry ) *game->map->size]|=1;
			if ( y-ry>=0&&y-ry<game->map->size )
				map[k+ ( y-ry ) *game->map->size]|=1;
		}

		x1=ry+x;x2=-ry+x;
		for ( k=x2;k<x1+1;k++ )
		{
			if ( k<0 ) continue;
			if ( k>=game->map->size ) break;
			if ( y+rx>=0&&y+rx<game->map->size )
				map[k+ ( y+rx ) *game->map->size]|=1;
			if ( y-rx>=0&&y-rx<game->map->size )
				map[k+ ( y-rx ) *game->map->size]|=1;
		}
	}
}

void SaveVehicle ( cVehicle *v,FILE *fp )
{
	int t;
	fputc ( SAVE_VEHICLE,fp );
	fputc ( ( ( v->data.can_drive==DRIVE_AIR ) ?1:0 ),fp ); // Vehicle
	t=v->PosX+v->PosY*game->map->size;fwrite ( &t,sizeof ( int ),1,fp ); // Offset
	t=v->owner->Nr;fwrite ( &t,sizeof ( int ),1,fp ); // Player Nr
	fwrite ( & ( v->typ->nr ),sizeof ( int ),1,fp ); // Typ-Nr
	fwrite ( & ( v->data ),sizeof ( sUnitData ),1,fp ); // Data

#define FSAVE_V_4(a) fwrite(&(v->a),sizeof(int),1,fp);
#define FSAVE_V_1(a) fwrite(&(v->a),1,1,fp);
	FSAVE_V_4 ( dir )
	FSAVE_V_1 ( IsBuilding )
	FSAVE_V_4 ( BuildingTyp )
	FSAVE_V_4 ( BuildCosts )
	FSAVE_V_4 ( BuildRounds )
	FSAVE_V_4 ( BuildRoundsStart )
	FSAVE_V_4 ( BandX )
	FSAVE_V_4 ( BandY )
	FSAVE_V_1 ( BuildPath )
	FSAVE_V_1 ( IsClearing )
	FSAVE_V_4 ( ClearingRounds )
	FSAVE_V_1 ( ClearBig )
	FSAVE_V_1 ( ShowBigBeton )
	FSAVE_V_1 ( Wachposten )
	FSAVE_V_4 ( FlightHigh )
	FSAVE_V_1 ( LayMines )
	FSAVE_V_1 ( ClearMines )
	FSAVE_V_1 ( detected )
	FSAVE_V_1 ( Loaded )
	FSAVE_V_4 ( CommandoRank )
	FSAVE_V_4 ( Disabled )
	FSAVE_V_1 ( detection_override )
	FSAVE_V_4 ( BigBetonAlpha )

	// ID zum wiederfinden wenn gestored:
	if ( v->Loaded )
	{
		fwrite ( v,sizeof ( int ),1,fp );
	}
	else
	{
		t=0;
		fwrite ( &t,sizeof ( int ),1,fp );
	}

	if ( v->StoredVehicles&&v->StoredVehicles->Count )
	{
		int i;
		fputc ( v->StoredVehicles->Count,fp ); // Vehicle geladen
		for ( i=0;i<v->StoredVehicles->Count;i++ )
		{
			cVehicle *s;
			s=v->StoredVehicles->VehicleItems[i];
			fwrite ( s,sizeof ( int ),1,fp );
		}
		for ( i=0;i<v->StoredVehicles->Count;i++ )
		{
			cVehicle *s;
			s=v->StoredVehicles->VehicleItems[i];
			SaveVehicle ( s,fp );
		}
	}
	else
	{
		fputc ( 0,fp ); // Keine Vehicle geladen
	}
}

void SaveBuilding ( int off,FILE *fp,bool base )
{
	cBuilding *b;
	int t;
	if ( base )
	{
		b=game->map->GO[off].base;
	}
	else
	{
		b=game->map->GO[off].top;
	}
	if ( b->data.is_big&& ( off%game->map->size!=b->PosX||off/game->map->size!=b->PosY ) ) return;
	fputc ( SAVE_BUILDING,fp );
	fputc ( ( base?1:0 ),fp ); // Base
	fwrite ( &off,sizeof ( int ),1,fp ); // Offset
	if ( !b->owner )
	{
		t=-1; // Dirt
		fwrite ( &t,sizeof ( int ),1,fp );
		fwrite ( &t,sizeof ( int ),1,fp );
	}
	else
	{
		t=b->owner->Nr;fwrite ( &t,sizeof ( int ),1,fp ); // Player Nr
		fwrite ( & ( b->typ->nr ),sizeof ( int ),1,fp ); // Typ-Nr
	}
	fwrite ( & ( b->data ),sizeof ( sUnitData ),1,fp ); // Data

#define FSAVE_B_4(a) fwrite(&(b->a),sizeof(int),1,fp);
#define FSAVE_B_1(a) fwrite(&(b->a),1,1,fp);

	FSAVE_B_4 ( DirtTyp )
	FSAVE_B_4 ( DirtValue )
	FSAVE_B_1 ( BigDirt )
	FSAVE_B_1 ( IsWorking )
	FSAVE_B_4 ( MetalProd )
	FSAVE_B_4 ( OilProd )
	FSAVE_B_4 ( GoldProd )
	FSAVE_B_4 ( MaxMetalProd )
	FSAVE_B_4 ( MaxOilProd )
	FSAVE_B_4 ( MaxGoldProd )
	FSAVE_B_4 ( dir )
	FSAVE_B_4 ( BuildSpeed )
	FSAVE_B_1 ( RepeatBuild )
	FSAVE_B_1 ( detected )
	FSAVE_B_4 ( Disabled )

	if ( b->BuildList&&b->BuildList->Count )
	{
		int i;
		fputc ( b->BuildList->Count,fp ); // BuildList vorhanden
		for ( i=0;i<b->BuildList->Count;i++ )
		{
			sBuildList *bl;
			bl=b->BuildList->BuildListItems[i];
			fwrite ( & ( bl->typ->nr ),sizeof ( int ),1,fp );
			fwrite ( & ( bl->metall_remaining ),sizeof ( int ),1,fp );
		}
	}
	else
	{
		fputc ( 0,fp ); // Keine BuildList
	}

	if ( b->StoredVehicles&&b->StoredVehicles->Count )
	{
		int i;
		fputc ( b->StoredVehicles->Count,fp ); // Vehicle geladen
		for ( i=0;i<b->StoredVehicles->Count;i++ )
		{
			cVehicle *s;
			s=b->StoredVehicles->VehicleItems[i];
			fwrite ( s,sizeof ( int ),1,fp );
		}
		for ( i=0;i<b->StoredVehicles->Count;i++ )
		{
			cVehicle *s;
			s=b->StoredVehicles->VehicleItems[i];
			SaveVehicle ( s,fp );
		}
	}
	else
	{
		fputc ( 0,fp ); // Keine Vehicle geladen
	}
}

// Speichert das Spiel ab:
bool cGame::Save ( string name )
{
	FILE *fp;
	int i,t;

	for ( std::string::iterator i = name.begin(); i < name.end(); i++ )
	{
		*i = tolower ( *i );
	}
	ActivePlayer->HotHud=* ( hud );
	if ( ( fp = fopen ( ( SettingsData.sSavesPath + PATH_DELIMITER + name ).c_str() ,"wb" ) ) == NULL )
	{
		cLog::write ( "Can't open Savefile " + name + " for writing", LOG_TYPE_WARNING );
		return false;
	}

	// Map-Name:
	i= ( int ) map->MapName.length() +1;
	fwrite ( &i,sizeof ( int ),1,fp );
	fwrite ( map->MapName.c_str(),1,i,fp );

	// Rundennummer:
	fwrite ( &Runde,sizeof ( int ),1,fp );
	// AlienTechs:
	fwrite ( &AlienTech,sizeof ( bool ),1,fp );
	// HotSeatPlayer:
	fwrite ( &HotSeatPlayer,sizeof ( int ),1,fp );
	// Rundenspiel:
	fwrite ( &PlayRounds,sizeof ( bool ),1,fp );
	fwrite ( &ActiveRoundPlayerNr,sizeof ( int ),1,fp );
	fwrite ( & ( engine->EndeCount ),sizeof ( int ),1,fp );

	// Player:
	i=PlayerList->Count;
	fwrite ( &i,sizeof ( int ),1,fp );
	for ( i=0;i<PlayerList->Count;i++ )
	{
		cPlayer *p;
		p=PlayerList->PlayerItems[i];
		t=p->Nr;fwrite ( &t,sizeof ( int ),1,fp ); // Player Nr
		t=GetColorNr ( p->color );fwrite ( &t,sizeof ( int ),1,fp ); // Color Nr
		t=p->Credits;fwrite ( &t,sizeof ( int ),1,fp ); // Credits
		t= ( int ) p->name.length() +1;fwrite ( &t,sizeof ( int ),1,fp ); // Name Length
		fwrite ( p->name.c_str(),1,t,fp ); // Name
		fwrite ( p->ResourceMap,1,map->size*map->size,fp ); // Ressource-Map

		fwrite ( p->ResearchTechs,sizeof ( sResearch ),8,fp );
		fwrite ( & ( p->ResearchCount ),sizeof ( int ),1,fp );
		fwrite ( & ( p->UnusedResearch ),sizeof ( int ),1,fp );

		fwrite ( p->VehicleData,sizeof ( sUnitData ),UnitsData.vehicle_anz,fp ); // Vehicle-Data
		fwrite ( p->BuildingData,sizeof ( sUnitData ),UnitsData.building_anz,fp ); // Building-Data

		fwrite ( & ( p->HotHud ),sizeof ( cHud ),1,fp ); // Hud-Einstellungen
	}

	// Alle Objekte:
	for ( i=0;i<map->size*map->size;i++ )
	{
		// Ressorcen:
		if ( map->Resources[i].typ )
		{
			fputc ( SAVE_RES,fp );
			fwrite ( &i,sizeof ( int ),1,fp ); // Offset
			fputc ( map->Resources[i].typ,fp ); // Typ
			fputc ( map->Resources[i].value,fp ); // Value
		}
		// Vehicle:
		if ( map->GO[i].vehicle )
		{
			cVehicle *v;
			v=map->GO[i].vehicle;
			if ( v->IsBuilding&& ( v->PosX!=i%map->size||v->PosY!=i/map->size ) )
			{
			}
			else
			{
				SaveVehicle ( v,fp );
			}
		}
		// Plane:
		if ( map->GO[i].plane )
		{
			SaveVehicle ( map->GO[i].plane,fp );
		}
		// Top-Building:
		if ( map->GO[i].top )
		{
			SaveBuilding ( i,fp,false );
		}
		// Base-Building:
		if ( map->GO[i].base )
		{
			SaveBuilding ( i,fp,true );
		}
	}
	fclose ( fp );
	return true;
}

// L�d das Spiel:
void cGame::Load ( string name,int AP,bool MP )
{
	TList *StoredVehicles;
	int i,t,typ;
	char *str;
	FILE *fp;
	if ( name.empty() ) return;
	if ( ( fp = fopen ( ( SettingsData.sSavesPath + PATH_DELIMITER + name ).c_str(),"rb" ) ) == NULL )
	{
		cLog::write ( "Can't open Savegame: " + name, LOG_TYPE_WARNING );
		return ;
	}

	StoredVehicles=new TList;

	// Map-Laden:
	fread ( &i,sizeof ( int ),1,fp );
	str= ( char* ) malloc ( i );
	fread ( str,1,i,fp );
	map->LoadMap ( str );
	free ( str );
	memset ( map->Resources,0,sizeof ( sResources ) *map->size*map->size );

	// Rundennummer:
	fread ( &Runde,sizeof ( int ),1,fp );
	// AlienTechs
	fread ( &AlienTech,sizeof ( bool ),1,fp );
	// HotSeatPlayer:
	fread ( &HotSeatPlayer,sizeof ( int ),1,fp );
	// Rundenspiel:
	fread ( &PlayRounds,sizeof ( bool ),1,fp );
	fread ( &ActiveRoundPlayerNr,sizeof ( int ),1,fp );
	fread ( & ( engine->EndeCount ),sizeof ( int ),1,fp );
	if ( !PlayRounds ) engine->EndeCount=0;

	// Player Laden:
	fread ( &t,sizeof ( int ),1,fp );
	if ( HotSeatPlayer>=t ) HotSeatPlayer=0;
	PlayerList=new TList;
	for ( i=0;i<t;i++ )
	{
		cPlayer *p;
		int nr,cl,credits;

		fread ( &nr,sizeof ( int ),1,fp ); // Nr
		fread ( &cl,sizeof ( int ),1,fp ); // Color
		fread ( &credits,sizeof ( int ),1,fp ); // Credits
		fread ( &typ,sizeof ( int ),1,fp ); // Name Length
		str= ( char* ) malloc ( typ );
		fread ( str,1,typ,fp );
		p=new cPlayer ( str,OtherData.colors[cl],nr );
		free ( str );
		p->Credits=credits;
		if ( i==AP ) ActivePlayer=p;
		p->InitMaps ( map->size );
		fread ( p->ResourceMap,1,map->size*map->size,fp ); // Ressource-Map

		fread ( p->ResearchTechs,sizeof ( sResearch ),8,fp );
		fread ( & ( p->ResearchCount ),sizeof ( int ),1,fp );
		fread ( & ( p->UnusedResearch ),sizeof ( int ),1,fp );

		fread ( p->VehicleData,sizeof ( sUnitData ),UnitsData.vehicle_anz,fp ); // Vehicle-Data
		fread ( p->BuildingData,sizeof ( sUnitData ),UnitsData.building_anz,fp ); // Building-Data

		fread ( & ( p->HotHud ),sizeof ( cHud ),1,fp ); // Hud-Einstellungen

		PlayerList->AddPlayer ( p );
	}

	// Alle Objekte laden:
	while ( ( typ=fgetc ( fp ) ) !=EOF )
	{
		switch ( typ )
		{
				// Ressorcen:
			case SAVE_RES:
				fread ( &i,sizeof ( int ),1,fp ); // Offset
				map->Resources[i].typ=fgetc ( fp ); // Typ
				map->Resources[i].value=fgetc ( fp ); // Value
				break;
				// Vehicle:
			case SAVE_VEHICLE:
			{
				cVehicle *v;
				cPlayer *p;
				bool plane;
				int nr,typnr,off;

				plane=fgetc ( fp ); // Vehicle oder Plane
				fread ( &off,sizeof ( int ),1,fp ); // Offset
				fread ( &nr,sizeof ( int ),1,fp ); // Player Nr
				for ( t=0;t<PlayerList->Count;t++ )
				{
					p=PlayerList->PlayerItems[t];
					if ( p->Nr==nr ) break;
				}
				fread ( &typnr,sizeof ( int ),1,fp ); // Typ-Nr

				engine->AddVehicle ( off%map->size,off/map->size,UnitsData.vehicle+typnr,p,true );
				if ( plane )
				{
					v=map->GO[off].plane;
				}
				else
				{
					v=map->GO[off].vehicle;
				}

				fread ( & ( v->data ),sizeof ( sUnitData ),1,fp ); // Data

#define FLOAD_V_4(a) fread(&(v->a),sizeof(int),1,fp);
#define FLOAD_V_1(a) fread(&(v->a),1,1,fp);
				FLOAD_V_4 ( dir )
				FLOAD_V_1 ( IsBuilding )
				FLOAD_V_4 ( BuildingTyp )
				FLOAD_V_4 ( BuildCosts )
				FLOAD_V_4 ( BuildRounds )
				FLOAD_V_4 ( BuildRoundsStart )
				FLOAD_V_4 ( BandX )
				FLOAD_V_4 ( BandY )
				FLOAD_V_1 ( BuildPath )
				if ( v->BuildPath )
				{
					v->BuildPath=false;
					v->BandX=0;
					v->BandY=0;
				}
				FLOAD_V_1 ( IsClearing )
				FLOAD_V_4 ( ClearingRounds )
				FLOAD_V_1 ( ClearBig )
				FLOAD_V_1 ( ShowBigBeton )
				FLOAD_V_1 ( Wachposten )
				FLOAD_V_4 ( FlightHigh )
				FLOAD_V_1 ( LayMines )
				FLOAD_V_1 ( ClearMines )
				FLOAD_V_1 ( detected )
				FLOAD_V_1 ( Loaded )
				FLOAD_V_4 ( CommandoRank )
				FLOAD_V_4 ( Disabled )
				FLOAD_V_1 ( detection_override )
				FLOAD_V_4 ( BigBetonAlpha )

				FLOAD_V_4 ( OffX ) // ID zum wiederfinden wenn gestored
				if ( v->OffX!=0 )
				{
					StoredVehicles->AddVehicle ( v );
					if ( plane )
					{
						if ( map->GO[off].plane==v ) map->GO[off].plane=NULL;
					}
					else
					{
						if ( map->GO[off].vehicle==v ) map->GO[off].vehicle=NULL;
					}
				}

				if ( ( i=fgetc ( fp ) ) >0&&v->StoredVehicles )
				{
					// Vehicle sind gespeichert:
					while ( i-- )
					{
						fread ( &t,sizeof ( int ),1,fp );
//            v->StoredVehicles->AddVehicle((int*)t); // Die ID (in OffX)
					}
				}

				if ( v->IsBuilding&&v->data.can_build==BUILD_BIG )
				{
					map->GO[off+1].vehicle=v;
					map->GO[off+map->size].vehicle=v;
					map->GO[off+map->size+1].vehicle=v;
				}

				if ( v->Wachposten )
				{
					v->owner->AddWachpostenV ( v );
				}

				break;
			}
			// Building:
			case SAVE_BUILDING:
			{
				cBuilding *b;
				cPlayer *p;
				bool base;
				int nr,typnr;

				base=fgetc ( fp ); // Base oder Top
				fread ( &i,sizeof ( int ),1,fp ); // Offset
				fread ( &nr,sizeof ( int ),1,fp ); // Player Nr
				if ( nr!=-1 )
				{
					for ( t=0;t<PlayerList->Count;t++ )
					{
						p=PlayerList->PlayerItems[t];
						if ( p->Nr==nr ) break;
					}
				}
				else
				{
					p=NULL;
				}
				fread ( &typnr,sizeof ( int ),1,fp ); // Typ-Nr

				if ( !p )
				{
					b=new cBuilding ( NULL,NULL,NULL );
					b->next=DirtList;
					DirtList=b;
					b->prev=NULL;
					map->GO[i].base=b;
					b->PosX=i%map->size;
					b->PosY=i/map->size;
				}
				else
				{
					engine->AddBuilding ( i%map->size,i/map->size,UnitsData.building+typnr,p,true );

					if ( base )
					{
						b=map->GO[i].base;
					}
					else
					{
						b=map->GO[i].top;
					}
				}

				fread ( & ( b->data ),sizeof ( sUnitData ),1,fp ); // Data
				/*        if((b->data.can_load==TRANS_METAL||b->data.can_load==TRANS_GOLD||b->data.can_load==TRANS_OIL)&&b->data.cargo){
				          int c;
				          c=b->data.cargo;
				          b->data.cargo=0;
				          if(b->data.can_load==TRANS_METAL){
				            b->owner->base->AddMetal(b->SubBase,c);
				          }else if(b->data.can_load==TRANS_OIL){
				            b->owner->base->AddOil(b->SubBase,c);
				          }else if(b->data.can_load==TRANS_GOLD){
				            b->owner->base->AddGold(b->SubBase,c);
				          }
				        }*/

#define FLOAD_B_4(a) fread(&(b->a),sizeof(int),1,fp);
#define FLOAD_B_1(a) fread(&(b->a),1,1,fp);

				FLOAD_B_4 ( DirtTyp )
				FLOAD_B_4 ( DirtValue )
				FLOAD_B_1 ( BigDirt )
				FLOAD_B_1 ( IsWorking )
				FLOAD_B_4 ( MetalProd )
				FLOAD_B_4 ( OilProd )
				FLOAD_B_4 ( GoldProd )
				FLOAD_B_4 ( MaxMetalProd )
				FLOAD_B_4 ( MaxOilProd )
				FLOAD_B_4 ( MaxGoldProd )
				FLOAD_B_4 ( dir )
				FLOAD_B_4 ( BuildSpeed )
				FLOAD_B_1 ( RepeatBuild )
				FLOAD_B_1 ( detected )
				FLOAD_B_4 ( Disabled )

				if ( ( i=fgetc ( fp ) ) >0&&b->BuildList )
				{
					// BuildList vorhanden:
					while ( i-- )
					{
						sBuildList *bl;
						bl=new sBuildList;
						b->BuildList->AddBuildList ( bl );

						fread ( & ( t ),sizeof ( int ),1,fp );
						fread ( & ( bl->metall_remaining ),sizeof ( int ),1,fp );

						bl->typ=UnitsData.vehicle+t;
					}
				}

				if ( ( i=fgetc ( fp ) ) >0&&b->StoredVehicles )
				{
					// Vehicle sind gespeichert:
					while ( i-- )
					{
						fread ( &t,sizeof ( int ),1,fp );
//            b->StoredVehicles->Add((int*)t); // Die ID (in OffX)
					}
				}

				if ( b->data.can_attack )
				{
					b->owner->AddWachpostenB ( b );
				}

				break;
			}
		}
	}
	fclose ( fp );

	// Alle SubBases neu berechnen:
	for ( i=0;i<PlayerList->Count;i++ )
	{
		cPlayer *p;
		p=PlayerList->PlayerItems[i];
		p->base->RefreshSubbases();
		p->CalcShields();
	}

	// Sich um die gespeicherten Vehicle k�mmern:
	for ( i=0;i<map->size*map->size;i++ )
	{
		if ( map->GO[i].vehicle&&map->GO[i].vehicle->StoredVehicles )
		{
			cVehicle *v=map->GO[i].vehicle,*v2;
			int k,m;
			for ( k=0;k<v->StoredVehicles->Count;k++ )
			{
				for ( m=0;m<StoredVehicles->Count;m++ )
				{
					if ( StoredVehicles->VehicleItems[m]->OffX== ( ( int ) ( v->StoredVehicles->VehicleItems[k] ) ) )
					{
						StoredVehicles->VehicleItems[m]->OffX=0;
						v->StoredVehicles->VehicleItems[k]=v2=StoredVehicles->VehicleItems[m];
						StoredVehicles->DeleteVehicle ( m );
						CheckRecursivLoaded ( v2,StoredVehicles );
						break;
					}
				}
			}
		}
		else if ( map->GO[i].plane&&map->GO[i].plane->StoredVehicles )
		{
			cVehicle *v=map->GO[i].plane,*v2;
			int k,m;
			for ( k=0;k<v->StoredVehicles->Count;k++ )
			{
				for ( m=0;m<StoredVehicles->Count;m++ )
				{
					if ( StoredVehicles->VehicleItems[m]->OffX== ( ( int ) ( v->StoredVehicles->VehicleItems[k] ) ) )
					{
						StoredVehicles->VehicleItems[m]->OffX=0;
						v->StoredVehicles->VehicleItems[k]=v2=StoredVehicles->VehicleItems[m];
						StoredVehicles->DeleteVehicle ( m );
						CheckRecursivLoaded ( v2,StoredVehicles );
						break;
					}
				}
			}
		}
		else if ( map->GO[i].top&&map->GO[i].top->StoredVehicles )
		{
			cBuilding *b=map->GO[i].top;
			int k,m;
			if ( b->PosX!=i%map->size||b->PosY!=i/map->size ) continue;
			for ( k=0;k<b->StoredVehicles->Count;k++ )
			{
				for ( m=0;m<StoredVehicles->Count;m++ )
				{
					cVehicle *v;
					v=StoredVehicles->VehicleItems[m];
					if ( v->OffX== ( int ) ( b->StoredVehicles->VehicleItems[k] ) )
					{
						v->OffX=0;
						b->StoredVehicles->VehicleItems[k]=v;
						StoredVehicles->DeleteVehicle ( m );

						CheckRecursivLoaded ( v,StoredVehicles );
						break;
					}
				}
			}
		}
		else continue;
		if ( !StoredVehicles->Count ) break;
	}
	delete StoredVehicles;

	if ( !MP )
	{

		if ( HotSeatPlayer!=0&&HotSeat )
		{
			ActivePlayer=PlayerList->PlayerItems[HotSeatPlayer];
		}
		*hud=ActivePlayer->HotHud;
		if ( hud->Zoom!=64 )
		{
			hud->LastZoom=-1;
			hud->ScaleSurfaces();
		}

		Run();

		while ( PlayerList->Count )
		{
			delete PlayerList->PlayerItems[0];
			PlayerList->DeletePlayer ( 0 );
		}
		delete PlayerList;
	}
}

// Pr�ft, ob das Vehicle auch wieder was geladen hat:
bool cGame::CheckRecursivLoaded ( cVehicle *v,TList *StoredVehicles )
{
	cVehicle *vv;
	int i,k;

	if ( StoredVehicles->Count&&v->StoredVehicles&&v->StoredVehicles->Count )
	{
		for ( i=0;i<v->StoredVehicles->Count;i++ )
		{
			for ( k=0;k<StoredVehicles->Count;k++ )
			{
				vv=StoredVehicles->VehicleItems[k];
				if ( vv->OffX== ( int ) ( v->StoredVehicles->VehicleItems[i] ) )
				{
					vv->OffX=0;
					v->StoredVehicles->VehicleItems[i]=vv;
					StoredVehicles->DeleteVehicle ( k );

					CheckRecursivLoaded ( vv,StoredVehicles );
					break;
				}
			}
		}
		return true;
	}
	return false;
}

// Zeigt das Laden/Speichern Men� an:
void cGame::ShowDateiMenu ( void )
{
	SDL_Rect scr,dest;
	int LastMouseX=0,LastMouseY=0,LastB=0,x,b,y,offset=0,selected=-1;
	bool SpeichernPressed=false, BeendenPressed=false, FertigPressed=false;
	bool  HilfePressed=false, UpPressed=false, DownPressed=false, Cursor=true;
	Uint8 *keystate;
	TList *files, *filenums, *filenames;
	TiXmlDocument doc;
	TiXmlNode* rootnode;
	TiXmlNode* node;
	char szTmp[8];

	PlayFX ( SoundData.SNDHudButton );
	mouse->SetCursor ( CHand );
	mouse->draw ( false,buffer );
	// Den Bildschirm blitten:
	SDL_BlitSurface ( GraphicsData.gfx_load_save_menu,NULL,buffer,NULL );
	// Den Text anzeigen:
	fonts->OutTextCenter ( lngPack.Translate( "Text~Game_Start~Title_LoadSave").c_str(),320,12,buffer );
	// Buttons setzen:
	PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Save").c_str(),132,438,0,false );
	PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Exit").c_str(),242,438,1,false );
	PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_OK").c_str(),353,438,2,false );
	PlaceSmallMenuButton ( "? ",464,438,false );
	scr.y=40;
	scr.w=dest.w=28;
	scr.h=dest.h=29;
	dest.y=438;
	scr.x=96;
	dest.x=33;
	SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
	scr.x=96+28*2;
	dest.x=63;
	SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
	// Dateien suchen und Anzeigen:
	if ( !doc.LoadFile ( ( SettingsData.sSavesPath + PATH_DELIMITER + "saves.xml" ).c_str() ) )
	{
		cLog::write ( "Could not load saves.xml",1 );
		return ;
	}
	rootnode=doc.FirstChildElement ( "SavesData" )->FirstChildElement ( "SavesList" );

	files = new TList;
	filenums = new TList;
	filenames = new TList;
	node=rootnode->FirstChildElement();
	if ( node && node->Type() == 1 )
	{
		files->Add ( node->ToElement()->Attribute ( "file" ) );
		filenums->Add ( node->ToElement()->Attribute ( "num" ) );
		filenames->Add ( node->ToElement()->Attribute ( "name" ) );
	}
	while ( node )
	{
		node=node->NextSibling();
		if ( node && node->Type() == 1 )
		{
			files->Add ( node->ToElement()->Attribute ( "file" ) );
			filenums->Add ( node->ToElement()->Attribute ( "num" ) );
			filenames->Add ( node->ToElement()->Attribute ( "name" ) );
		}
	}
	ShowFiles ( files,filenums,filenames,offset,selected,true,false );
	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack ( buffer );
	while ( 1 )
	{
		// Die Engine laufen lassen:
		game->engine->Run();
		HandleTimer();

		// Events holen:
		SDL_PumpEvents();

		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE] )
		{
			InputStr="";
			break;
		}

		if ( DoKeyInp ( keystate ) ||timer2 )
		{
			if ( Cursor )
			{
				Cursor=false;
				ShowFiles ( files,filenums,filenames,offset,selected,true,false );
			}
			else
			{
				Cursor=true;
				ShowFiles ( files,filenums,filenames,offset,selected,false,false );
			}
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();
		x=mouse->x;y=mouse->y;
		if ( x!=LastMouseX||y!=LastMouseY )
		{
			mouse->draw ( true,screen );
		}
		// Klick auf einen Speicher:
		if ( ( x>=15&&x<15+205&&y>45&&y<45+375 ) || ( x>=417&&x<417+205&&y>45&&y<45+375 ) )
		{
			if ( b&&!LastB )
			{
				InputStr = "";
				int checkx = 15, checky = 45;
				// Speicher 1
				for ( int i = 0; i<10; i++ )
				{
					if ( i==5 )
					{
						checkx=418;
						checky=45;
					}
					if ( x>=checkx&&x<checkx+205&&y>checky&&y<checky+73 )
					{
						selected = i + offset;
					}
					checky+=75;
				}
				ShowFiles ( files,filenums,filenames,offset,selected,true,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		// Speichern-Button:
		if ( x>=132&&x<132+109&&y>=438&&y<438+40 )
		{
			if ( b&&!SpeichernPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Save").c_str(),132,438,0,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				SpeichernPressed=true;
			}
			else if ( !b&&LastB )
			{
				PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Save").c_str(),132,438,0,false );
				if ( selected != -1 )
				{
					ShowFiles ( files,filenums,filenames,offset,selected,false,false );
					// Set Savename to lowercase
					for ( std::string::iterator i = SaveLoadFile.begin(); i < SaveLoadFile.end(); i++ )
					{
						*i = tolower ( *i );
					}
					if ( Save ( SaveLoadFile ) )
					{
						// Save new file to saves.xml
						int iNumber = selected + 1;
						// Search if number exists
						for ( int i = 0; i < filenums->Count; i++ )
						{
							// if exists: remove old file & set new values
							if ( atoi ( filenums->Items[i].c_str() ) == iNumber )
							{
								// if name changed, delete old file
								if ( strcmp ( filenames->Items[i].c_str(), InputStr.c_str() ) != NULL )
								{
									if ( FileExists ( ( SettingsData.sSavesPath + PATH_DELIMITER + files->Items[i] ).c_str() ) )
									{
										remove ( ( SettingsData.sSavesPath + PATH_DELIMITER + files->Items[i] ).c_str() );
									}
								}
								// set new values in saves.xml
								node = rootnode->FirstChildElement();
								for ( int j = 0; j < i; j++ )
									node=node->NextSibling();
								node->ToElement()->SetAttribute ( "name",InputStr.c_str() );
								node->ToElement()->SetAttribute ( "file",SaveLoadFile.c_str() );
								break;
							}
							// if doesn't exists: add new node
							else if ( i == filenums->Count - 1 )
							{
								TiXmlElement element ( "Save" );
								node = rootnode->InsertEndChild ( element );
								sprintf ( szTmp,"%d",iNumber );
								node->ToElement()->SetAttribute ( "name",InputStr.c_str() );
								node->ToElement()->SetAttribute ( "file",SaveLoadFile.c_str() );
								node->ToElement()->SetAttribute ( "num",szTmp );
							}
						}
						doc.SaveFile();
						// Clear Lists
						int iCount = files->Count;
						for ( int i = 0; i < iCount; i++ )
						{
							files->DeleteString ( files->Count );
							filenums->DeleteString ( filenums->Count );
							filenames->DeleteString ( filenames->Count );
						}
						// Read filelists new
						node=rootnode->FirstChildElement();
						if ( node && node->Type() == 1 )
						{
							files->Add ( node->ToElement()->Attribute ( "file" ) );
							filenums->Add ( node->ToElement()->Attribute ( "num" ) );
							filenames->Add ( node->ToElement()->Attribute ( "name" ) );
						}
						while ( node )
						{
							node=node->NextSibling();
							if ( node && node->Type() == 1 )
							{
								files->Add ( node->ToElement()->Attribute ( "file" ) );
								filenums->Add ( node->ToElement()->Attribute ( "num" ) );
								filenames->Add ( node->ToElement()->Attribute ( "name" ) );
							}
						}
						selected = -1;
					}
					ShowFiles ( files,filenums,filenames,offset,selected,false,false );
				}
				SHOW_SCREEN
				mouse->draw ( false,screen );
				SpeichernPressed=false;
			}
		}
		else if ( SpeichernPressed )
		{
			PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Save").c_str(),132,438,0,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			SpeichernPressed=false;
		}
		// Beenden-Button:
		if ( x>=242&&x<242+109&&y>=438&&y<438+40 )
		{
			if ( b&&!BeendenPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Exit").c_str(),242,438,1,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				BeendenPressed=true;
			}
			else if ( !b&&LastB )
			{
				PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Exit").c_str(),242,438,1,false );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				BeendenPressed=false;
				End = true;
				return;
			}
		}
		else if ( BeendenPressed )
		{
			PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_Exit").c_str(),242,438,1,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			BeendenPressed=false;
		}
		// Fertig-Button:
		if ( x>=353&&x<353+109&&y>=438&&y<438+40 )
		{
			if ( b&&!FertigPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_OK").c_str(),353,438,2,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				FertigPressed=true;
			}
			else if ( !b&&LastB )
			{
				return;
			}
		}
		else if ( FertigPressed )
		{
			PlaceMenuButton ( lngPack.Translate( "Text~Menu_Main~Button_OK").c_str(),353,438,2,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			FertigPressed=false;
		}
		// Up-Button:
		if ( x>=33&&x<33+29&&y>=438&&y<438+29 )
		{
			if ( b&&!UpPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				scr.x=96+28;
				dest.x=33;
				SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				UpPressed=true;
			}
			else if ( !b&&LastB )
			{
				if ( offset>0 )
				{
					offset-=10;
					selected=-1;
				}
				ShowFiles ( files,filenums,filenames,offset,selected,Cursor,false );
				scr.x=96;
				dest.x=33;
				SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				UpPressed=false;
			}
		}
		else if ( UpPressed )
		{
			scr.x=96;
			dest.x=33;
			SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			UpPressed=false;
		}
		// Down-Button:
		if ( x>=63&&x<63+29&&y>=438&&y<438+29 )
		{
			if ( b&&!DownPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				scr.x=96+28*3;
				dest.x=63;
				SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				DownPressed=true;
			}
			else if ( !b&&LastB )
			{
				if ( offset<90 )
				{
					offset+=10;
					selected=-1;
				}
				ShowFiles ( files,filenums,filenames,offset,selected,Cursor,false );
				scr.x=96+28*2;
				dest.x=63;
				SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				DownPressed=false;
			}
		}
		else if ( DownPressed )
		{
			scr.x=96+28*2;
			dest.x=63;
			SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			DownPressed=false;
		}
		LastMouseX=x;LastMouseY=y;
		LastB=b;
	}
}

void cGame::ShowFiles ( TList *files, TList *filenums, TList *filenames, int offset, int selected, bool cursor, bool firstselect )
{
	SDL_Rect rect;
	int i,x=35,y=72;
	string filename;
	char sztmp[32];
	// Save Nummern ausgeben
	rect.x=25;rect.y=70;
	rect.w=26;rect.h=16;
	for ( i = 0; i < 10; i++ )
	{
		if ( i == 5 )
		{
			rect.x+=398;
			rect.y=70;
			x=435;
			y=72;
		}
		SDL_BlitSurface ( GraphicsData.gfx_load_save_menu,&rect,buffer,&rect );
		if ( i+offset==selected )
		{
			sprintf ( sztmp,"%d", ( offset+i+1 ) );
			fonts->OutTextBigCenterGold ( sztmp,x,y,buffer );
		}
		else
		{
			sprintf ( sztmp,"%d", ( offset+i+1 ) );
			fonts->OutTextBigCenter ( sztmp,x,y,buffer );
		}
		rect.y+=76;
		y+=76;
	}
	// Savenamen mit evtl. Auswahl ausgeben
	rect.x=55;rect.y=82;
	rect.w=153;rect.h=18;
	for ( i = 0; i < 10; i++ )
	{
		if ( i == 5 )
		{
			rect.x+=402;
			rect.y=82;
		}
		SDL_BlitSurface ( GraphicsData.gfx_load_save_menu,&rect,buffer,&rect );
		rect.y+=76;
	}
	x=60;y=87;
	selected++;
	for ( i = offset+1; i <= 10+offset; i++ )
	{
		if ( i == offset+6 )
		{
			x+=402;
			y=87;
		}
		for ( int j = 0; j < filenums->Count; j++ )
		{
			if ( atoi ( filenums->Items[j].c_str() ) == i )
			{
				filename = filenames->Items[j];
				// Dateinamen anpassen und ausgeben
				if ( filename.length() > 15 )
				{
					filename.erase ( 15 );
				}
				if ( i == selected )
				{
					if ( firstselect )
					{
						InputStr = filename;
					}
					else
					{
						filename = InputStr;
					}
					if ( cursor )
					{
						filename+="_";
					}
					SaveLoadFile = InputStr + ".sav";
				}
				fonts->OutText ( ( char * ) filename.c_str(),x,y,buffer );
				break;
			}
			else if ( i == selected && j == filenums->Count-1 )
			{
				if ( InputStr.length() > 15 )
				{
					InputStr.erase ( 15 );
				}
				filename = InputStr;
				if ( cursor )
				{
					filename+="_";
				}
				SaveLoadFile = InputStr + ".sav";
				fonts->OutText ( ( char * ) filename.c_str(),x,y,buffer );
			}
		}
		y+=76;
	}
}

// makes an autosave:
void cGame::MakeAutosave(void)
{
	string filename;
	char *sztmp;
	int i;
	// Alle Autosaves nach hinten verschieben:
	for(i = 4; i >= 0; i--)
	{
		sztmp = (char *) malloc ( SettingsData.sSavesPath.length() + 1 + strlen( ((string) "autosave" + PATH_DELIMITER).c_str()) + 2);
		sprintf(sztmp,"%s%sautosave%d.sav",SettingsData.sSavesPath.c_str(),PATH_DELIMITER,i);
		filename = sztmp;
		if( !( FileExists( filename.c_str() ) ) ) continue;
		if( i == 4 )
		{
			remove(filename.c_str());
		}
		else
		{
			sprintf(sztmp,"%s%sautosave%d.sav",SettingsData.sSavesPath.c_str(),PATH_DELIMITER,i+1);
			rename(filename.c_str(),sztmp);
		}
	}
	Save("autosave0.sav");
	free(sztmp);
}

// Zeigt alle Infos der Objekte unter der Maus an:
void cGame::Trace ( void )
{
	int y,x;
	if ( !OverObject )
	{
		sGameObjects *GO;
		int x,y;
		mouse->GetKachel ( &x,&y );
		if ( x<0||y<0 ) return;
		GO=map->GO+ ( map->size*y+x );
		if ( GO->reserviert ) fonts->OutTextSmall ( "reserviert",180+5,18+5,ClWhite,buffer );
		if ( GO->air_reserviert ) fonts->OutTextSmall ( "air-reserviert",180+5+100,18+5,ClWhite,buffer );
		return;
	}
	if ( OverObject->reserviert ) fonts->OutTextSmall ( "reserviert",180+5,18+5,ClWhite,buffer );
	if ( OverObject->air_reserviert ) fonts->OutTextSmall ( "air-reserviert",180+5+100,18+5,ClWhite,buffer );
	y=18+5+8;
	x=180+5;

	if ( OverObject->vehicle ) {TraceVehicle ( OverObject->vehicle,&y,x );y+=20;}
	if ( OverObject->plane ) {TraceVehicle ( OverObject->plane,&y,x );y+=20;}
	if ( OverObject->top ) {TraceBuilding ( OverObject->top,&y,x );y+=20;}
	if ( OverObject->base ) TraceBuilding ( OverObject->base,&y,x );
}

// Zeigt alle Infos �ber das Vehicle an:
void cGame::TraceVehicle ( cVehicle *v,int *y,int x )
{
	char str[300];

	sprintf ( str,"name: \"%s\"   owner: \"%s\"   posx: %d   posy: %d   offx: %d   offy: %d",v->name.c_str(),v->owner->name.c_str(),v->PosX,v->PosY,v->OffX,v->OffY );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"dir: %d   selected: %d   moving: %d   rotating: %d   mjob: %d   mj_active: %d   menu_active: %d",v->dir,v->selected,v->moving,v->rotating,v->mjob,v->MoveJobActive,v->MenuActive );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"attack_mode: %d   attacking: %d   wachpost: %d   transfer: %d   ditherx: %d   dithery: %d",v->AttackMode,v->Attacking,v->Wachposten,v->Transfer,v->ditherX,v->ditherY );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"is_building: %d   building_typ: %d   build_costs: %d   build_rounds: %d   build_round_start: %d",v->IsBuilding,v->BuildingTyp,v->BuildCosts,v->BuildRounds,v->BuildRoundsStart );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"place_band: %d   bandx: %d   bandy: %d   build_big_saved_pos: %d   build_path: %d",v->PlaceBand,v->BandX,v->BandY,v->BuildBigSavedPos,v->BuildPath );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"build_override: %d   is_clearing: %d   clearing_rounds: %d   clear_big: %d   loaded: %d",v->BuildOverride,v->IsClearing,v->ClearingRounds,v->ClearBig,v->Loaded );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"commando_rank: %d   steal_active: %d   disable_active: %d   disabled: %d   detection_override: %d",v->CommandoRank,v->StealActive,v->DisableActive,v->Disabled,v->detection_override );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"is_locked: %d   detected: %d   clear_mines: %d   lay_mines: %d   repair_active: %d   muni_active: %d",v->IsLocked,v->detected,v->ClearMines,v->LayMines,v->RepairActive,v->MuniActive );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"load_active: %d   activating_vehicle: %d   vehicle_to_activate: %d   stored_vehicles_count: %d",v->LoadActive,v->ActivatingVehicle,v->VehicleToActivate, ( v->StoredVehicles?v->StoredVehicles->Count:0 ) );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	if ( v->StoredVehicles&&v->StoredVehicles->Count )
	{
		cVehicle *vp;
		int i;
		for ( i=0;i<v->StoredVehicles->Count;i++ )
		{
			vp=v->StoredVehicles->VehicleItems[i];
			sprintf ( str,"  store %d: \"%s\"",i,vp->name.c_str() );
			fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;
		}
	}
}

// Zeig alle Infos �ber das Building an:
void cGame::TraceBuilding ( cBuilding *b,int *y,int x )
{
	char str[300];

	sprintf ( str,"name: \"%s\"   owner: \"%s\"   posx: %d   posy: %d   selected: %d",b->name.c_str(), ( b->owner?b->owner->name.c_str() :"<null>" ),b->PosX,b->PosY,b->selected );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"dir: %d   menu_active: %d   attacking_mode: %d   base: %d   sub_base: %d",b->dir,b->MenuActive,b->AttackMode,b->base,b->SubBase );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"attacking: %d   UnitsData.dirt_typ: %d   UnitsData.dirt_value: %d   big_dirt: %d   is_working: %d   transfer: %d",b->Attacking,b->DirtTyp,b->DirtValue,b->BigDirt,b->IsWorking,b->Transfer );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"metal_prod: %d   oil_prod: %d   gold_prod: %d   max_metal_p: %d   max_oil_p: %d   max_gold_p: %d",b->MetalProd,b->OilProd,b->GoldProd,b->MaxMetalProd,b->MaxOilProd,b->MaxGoldProd );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"is_locked: %d   disabled: %d   detected: %d   activating_vehicle: %d   vehicle_to_activate: %d",b->IsLocked,b->Disabled,b->detected,b->ActivatingVehicle,b->VehicleToActivate );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	sprintf ( str,"load_active: %d   stored_vehicles_count: %d",b->LoadActive, ( b->StoredVehicles?b->StoredVehicles->Count:0 ) );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	if ( b->StoredVehicles&&b->StoredVehicles->Count )
	{
		cVehicle *vp;
		int i;
		for ( i=0;i<b->StoredVehicles->Count;i++ )
		{
			vp=b->StoredVehicles->VehicleItems[i];
			sprintf ( str,"  store %d: \"%s\"",i,vp->name.c_str() );
			fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;
		}
	}

	sprintf ( str,"build_speed: %d   repeat_build: %d   build_list_count: %d",b->BuildSpeed,b->RepeatBuild, ( b->BuildList?b->BuildList->Count:0 ) );
	fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;

	if ( b->BuildList&&b->BuildList->Count )
	{
		sBuildList *bl;
		int i;
		for ( i=0;i<b->BuildList->Count;i++ )
		{
			bl=b->BuildList->BuildListItems[i];
			sprintf ( str,"  build %d: %d \"%s\"",i,bl->typ->nr,UnitsData.vehicle[bl->typ->nr].data.name );
			fonts->OutTextSmall ( str,x,*y,ClWhite,buffer );*y+=8;
		}
	}
}

// K�mmert sich um das Ende einer Hot-Seat-Spiel-Runde und gibt true zur�ck,
// wenn alle Spieler einmal dran waren:
bool cGame::MakeHotSeatEnde ( void )
{
	int zoom,x,y;
	cBuilding *b;
	cVehicle *v;
	string stmp;

	sMessage *m;
	while ( messages->Count )
	{
		m=messages->MessageItems[0];
		free ( m->msg );
		free ( m );
		messages->DeleteMessage ( 0 );
	}

	HotSeatPlayer++;
	if ( HotSeatPlayer>=PlayerList->Count ) HotSeatPlayer=0;

	ActivePlayer->HotHud=*hud;
	ActivePlayer=PlayerList->PlayerItems[HotSeatPlayer];
	zoom=hud->LastZoom;
	*hud=ActivePlayer->HotHud;
	x=hud->OffX;
	y=hud->OffY;
	if ( hud->LastZoom!=zoom )
	{
		hud->LastZoom=-1;
		hud->ScaleSurfaces();
	}
	hud->DoAllHud();
	hud->EndeButton ( false );
	hud->OffX=x;
	hud->OffY=y;
	if ( SelectedBuilding ) {SelectedBuilding->Deselct();SelectedBuilding=NULL;}
	if ( SelectedVehicle ) {SelectedVehicle->Deselct();SelectedVehicle=NULL;}


	SDL_Surface *sf;
	SDL_Rect scr;
	sf=SDL_CreateRGBSurface ( SDL_SRCCOLORKEY,SettingsData.iScreenW,SettingsData.iScreenH,32,0,0,0,0 );
	scr.x=15;
	scr.y=356;
	scr.w=scr.h=112;
	SDL_BlitSurface ( sf,NULL,buffer,NULL );
	SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );
	SDL_BlitSurface ( sf,&scr,buffer,&scr );
	stmp=ActivePlayer->name; stmp+=lngPack.Translate( "Text~Game_MP~Comp_Player_Turn");
	ShowOK ( stmp,true );
	if ( HotSeatPlayer!=0 )
	{
		ActivePlayer->base->Rundenende();
		ActivePlayer->DoResearch();
		if ( SelectedVehicle ) SelectedVehicle->ShowDetails();
		else if ( SelectedBuilding ) SelectedBuilding->ShowDetails();
	}

	v=ActivePlayer->VehicleList;
	while ( v )
	{
		v->RefreshData();
		v=v->next;
	}

	b=ActivePlayer->BuildingList;
	while ( b )
	{
		if ( b->data.can_attack ) b->RefreshData();
		b=b->next;
	}

	if ( strcmp ( game->PlayerCheat.c_str(),"" ) )
	{
		string sstmp;
		sstmp = PlayerCheat.substr ( 0,7 );
		if ( !strcmp ( sstmp.c_str(),ActivePlayer->name.c_str() ) )
			PlayerCheat="";
		else
			game->AddMessage ( ( char* ) game->PlayerCheat.c_str() );
	}
	if ( HotSeatPlayer==0 ) return true;
	engine->MakeRundenstartReport();
	return false;
}
