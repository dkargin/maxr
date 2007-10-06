//////////////////////////////////////////////////////////////////////////////
// M.A.X. - hud.cpp
//////////////////////////////////////////////////////////////////////////////
#include "hud.h"
#include "main.h"
#include "mouse.h"
#include "game.h"
#include "sound.h"
#include "prefer.h"
#include "keyinp.h"
#include "fonts.h"

// Funktionen der Hud-Klasse /////////////////////////////////////////////////
cHud::cHud ( void )
{
	TNT=false;
	Radar=false;
	Nebel=false;
	Gitter=false;
	Scan=false;
	Reichweite=false;
	Munition=false;
	Treffer=false;
	Farben=false;
	Status=false;
	Studie=false;
	LastOverEnde=false;
	Lock=false;
	Zoom=64;
	LastZoom=64;
	OffX=0;
	OffY=0;
	Praeferenzen=false;
	PlayFLC=true;
	PausePressed=false;PlayPressed=false;
	HelpPressed=false;ChatPressed=false;
	EndePressed=false;ErledigenPressed=false;
	NextPressed=false;PrevPressed=false;
	CenterPressed=false;DateiPressed=false;
	LogPressed=false;
	Ende=false;
}

cHud::~cHud ( void )
{}

void cHud::SwitchTNT ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=362;
			scr.y=24;
		}
	else
	{
		scr.x=334;
		scr.y=24;
	}
	dest.x=136;
	dest.y=413;
	dest.w=scr.w=27;
	dest.h=scr.h=28;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	TNT=set;
	game->fDrawHud=true;
	game->fDrawMMap=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchRadar ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=362;
			scr.y=53;
		}
	else
	{
		scr.x=334;
		scr.y=53;
	}
	dest.x=136;
	dest.y=387;
	dest.w=scr.w=27;
	dest.h=scr.h=26;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Radar=set;
	game->fDrawHud=true;
	game->fDrawMMap=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchNebel ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=110;
			scr.y=78;
		}
	else
	{
		scr.x=277;
		scr.y=78;
	}
	dest.x=112;
	dest.y=330;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Nebel=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchGitter ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=110;
			scr.y=61;
		}
	else
	{
		scr.x=277;
		scr.y=61;
	}
	dest.x=112;
	dest.y=313;
	dest.w=scr.w=55;
	dest.h=scr.h=17;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Gitter=set;
	game->fDrawHud=true;
	game->fDrawMap=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchScan ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=110;
			scr.y=44;
		}
	else
	{
		scr.x=277;
		scr.y=44;
	}
	dest.x=112;
	dest.y=296;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Scan=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchReichweite ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=55;
			scr.y=78;
		}
	else
	{
		scr.x=222;
		scr.y=78;
	}
	dest.x=57;
	dest.y=330;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Reichweite=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchMunition ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=55;
			scr.y=61;
		}
	else
	{
		scr.x=222;
		scr.y=61;
	}
	dest.x=57;
	dest.y=313;
	dest.w=scr.w=55;
	dest.h=scr.h=17;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Munition=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchTreffer ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=55;
			scr.y=44;
		}
	else
	{
		scr.x=222;
		scr.y=44;
	}
	dest.x=57;
	dest.y=296;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Treffer=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchFarben ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=0;
			scr.y=78;
		}
	else
	{
		scr.x=167;
		scr.y=78;
	}
	dest.x=2;
	dest.y=330;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Farben=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchStatus ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=0;
			scr.y=61;
		}
	else
	{
		scr.x=167;
		scr.y=61;
	}
	dest.x=2;
	dest.y=313;
	dest.w=scr.w=55;
	dest.h=scr.h=17;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Status=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchStudie ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=0;
			scr.y=44;
		}
	else
	{
		scr.x=167;
		scr.y=44;
	}
	dest.x=2;
	dest.y=296;
	dest.w=scr.w=55;
	dest.h=scr.h=18;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Studie=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::SwitchLock ( bool set )
{
	SDL_Rect scr,dest;
	if ( set )
		{
			scr.x=397;
			scr.y=298;
		}
	else
	{
		scr.x=397;
		scr.y=321;
	}
	dest.x=32;
	dest.y=227;
	dest.w=scr.w=21;
	dest.h=scr.h=22;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	Lock=set;
	game->fDrawHud=true;
	PlayFX ( SNDHudSwitch );
}

void cHud::DoZoom ( int x,int y )
{
	if ( x<=12 ) x=0;else x-=12;
	if ( x>104 ) x=104;
//  x=64-(int)(x*(64-((448.0/game->map->size)<5?5:(448.0/game->map->size)))/104);
	x=64- ( int ) ( x* ( 64- ( ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) <5?5: ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) ) ) /104 );
	if ( x<1 ) x=1;
	SetZoom ( x,y );
}

void cHud::SetZoom ( int zoom,int DestY )
{
	static int lastz=64;
	SDL_Rect scr,dest;
//  if(zoom<448/game->map->size)zoom=448/game->map->size;
	if ( zoom< ( cSettingsData.iScreenW-192 ) /game->map->size ) zoom= ( cSettingsData.iScreenW-192 ) /game->map->size;
	if ( zoom<5 ) zoom=5;
	else if ( zoom>64 ) zoom=64;
	Zoom=zoom;
//  zoom-=((448.0/game->map->size)<5?5:(448.0/game->map->size));
	zoom-= ( int ) ( ( ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) <5?5: ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) ) );
	scr.x=0;
	scr.y=0;
	dest.x=19;
	dest.y=DestY;
	dest.w=scr.w=132;
	dest.h=scr.h=20;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
//  zoom=106-(int)(zoom*106.0/(64-((448.0/game->map->size)<5?5:(448.0/game->map->size))));
	zoom=106- ( int ) ( zoom*106.0/ ( 64- ( ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) <5?5: ( ( cSettingsData.iScreenW-192.0 ) /game->map->size ) ) ) );
	scr.x=132;
	scr.y=1;
	dest.x=20+zoom;
	dest.y=DestY+1;
	dest.w=scr.w=25;
	dest.h=scr.h=14;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	game->fDrawHud=true;
	game->fDrawMap=true;
	game->fDrawMMap=true;

	if ( lastz!=Zoom )
	{
		int off;
//    off=(int)(((448.0/(float)lastz*64)-(448.0/(float)Zoom*64))/2);
		off= ( int ) ( ( ( ( cSettingsData.iScreenW-192.0 ) / ( float ) lastz*64 )- ( ( cSettingsData.iScreenW-192.0 ) / ( float ) Zoom*64 ) ) /2 );
		lastz=Zoom;

		OffX+=off;
		OffY+=off;
//    off=game->map->size*64-(int)((448.0/Zoom)*64);
		off=game->map->size*64- ( int ) ( ( ( cSettingsData.iScreenW-192.0 ) /Zoom ) *64 );
		while ( OffX>off ) OffX--;
		while ( OffY>off ) OffY--;
		while ( OffX<0 ) OffX++;
		while ( OffY<0 ) OffY++;
	}
	ScaleSurfaces();
}

void cHud::CheckButtons ( void )
{
	int x,y;
	x=mouse->x;
	y=mouse->y;
	if ( x<170 )
	{
		if ( x>=136&&x<=136+27&&y>=413&&y<=413+28 ) {SwitchTNT ( !TNT );return;}
		if ( x>=136&&x<=136+27&&y>=387&&y<=387+28 ) {SwitchRadar ( !Radar );return;}
		if ( x>=112&&x<=112+55&&y>=332&&y<=332+17 ) {SwitchNebel ( !Nebel );return;}
		if ( x>=112&&x<=112+55&&y>=314&&y<=314+17 ) {SwitchGitter ( !Gitter );return;}
		if ( x>=112&&x<=112+55&&y>=296&&y<=296+17 ) {SwitchScan ( !Scan );return;}
		if ( x>=57&&x<=57+55&&y>=332&&y<=332+17 ) {SwitchReichweite ( !Reichweite );return;}
		if ( x>=57&&x<=57+55&&y>=314&&y<=314+17 ) {SwitchMunition ( !Munition );return;}
		if ( x>=57&&x<=57+55&&y>=296&&y<=296+17 ) {SwitchTreffer ( !Treffer );return;}
		if ( x>=0&&x<=55&&y>=332&&y<=332+17 ) {SwitchFarben ( !Farben );return;}
		if ( x>=0&&x<=55&&y>=314&&y<=314+17 ) {SwitchStatus ( !Status );return;}
		if ( x>=0&&x<=55&&y>=296&&y<=296+17 ) {SwitchStudie ( !Studie );return;}
		if ( x>=32&&x<=52&&y>=227&&y<=248 ) {SwitchLock ( !Lock );return;}
	}
}

void cHud::CheckOneClick ( void )
{
	static int lastX=-1;
	int x,y;
	x=mouse->x;
	y=mouse->y;
	if ( x<170&&y<296 )
	{
		if ( x!=lastX&&x>=20&&y>=272&&x<=150&&y<=292 ) {DoZoom ( x-20 );lastX=x;return;}
	}
	if ( x>=15&&x<15+112&&y>=356&&y<356+112 ) {DoMinimapClick ( x,y );return;}
	lastX=x;
}

void cHud::DoAllHud ( void )
{
	bool s;
	s=cSettingsData.bSoundEnabled;
	cSettingsData.bSoundEnabled=false;
	SwitchTNT ( TNT );
	SwitchRadar ( Radar );
	SwitchNebel ( Nebel );
	SwitchGitter ( Gitter );
	SwitchScan ( Scan );
	SwitchReichweite ( Reichweite );
	SwitchMunition ( Munition );
	SwitchTreffer ( Treffer );
	SwitchFarben ( Farben );
	SwitchStatus ( Status );
	SwitchStudie ( Studie );
	SwitchLock ( Lock );
	SetZoom ( Zoom );
	cSettingsData.bSoundEnabled=s;
	ShowRunde();
}

void cHud::CheckScroll ( bool pure )
{
	int x,y;
	x=mouse->x;
	y=mouse->y;

	if ( x<=0&&y>30&&y<cSettingsData.iScreenH-30-18 ) {mouse->SetCursor ( CPfeil4 );DoScroll ( 4 );return;}
	if ( x>=cSettingsData.iScreenW-18&&y>30&&y<cSettingsData.iScreenH-30-18 ) {mouse->SetCursor ( CPfeil6 );DoScroll ( 6 );return;}

	if ( ( x<=0&&y<=30 ) || ( y<=0&&x<=30 ) ) {mouse->SetCursor ( CPfeil7 );DoScroll ( 7 );return;}
	if ( ( x>=cSettingsData.iScreenW-18&&y<=30 ) || ( y<=0&&x>=cSettingsData.iScreenW-30-18 ) ) {mouse->SetCursor ( CPfeil9 );DoScroll ( 9 );return;}

	if ( y<=0&&x>30&&x<cSettingsData.iScreenW-30-18 ) {mouse->SetCursor ( CPfeil8 );DoScroll ( 8 );return;}
	if ( y>=cSettingsData.iScreenH-18&&x>30&&x<cSettingsData.iScreenW-30-18 ) {mouse->SetCursor ( CPfeil2 );DoScroll ( 2 );return;}

	if ( ( x<=0&&y>=cSettingsData.iScreenH-30-18 ) || ( y>=cSettingsData.iScreenH-18&&x<=30 ) ) {mouse->SetCursor ( CPfeil1 );DoScroll ( 1 );return;}
	if ( ( x>=cSettingsData.iScreenW-18&&y>=cSettingsData.iScreenH-30-18 ) || ( y>=cSettingsData.iScreenH-18&&x>=cSettingsData.iScreenW-30-18 ) ) {mouse->SetCursor ( CPfeil3 );DoScroll ( 3 );return;}

	if ( pure )
	{
		if ( mouse->SetCursor ( CHand ) )
		{
			game->fDrawMap=true;
		}
		return;
	}

	if ( game->SelectedVehicle&&game->SelectedVehicle->PlaceBand&&game->SelectedVehicle->owner==game->ActivePlayer )
	{
		if ( x>=180 )
		{
			if ( mouse->SetCursor ( CBand ) )
			{
				game->fDrawMap=true;
			}
		}
		else
		{
			if ( mouse->SetCursor ( CNo ) )
			{
				game->fDrawMap=true;
			}
		}
	}
	else if ( ( game->SelectedVehicle&&game->SelectedVehicle->Transfer&&game->SelectedVehicle->owner==game->ActivePlayer ) || ( game->SelectedBuilding&&game->SelectedBuilding->Transfer&&game->SelectedBuilding->owner==game->ActivePlayer ) )
	{
		if ( game->SelectedVehicle )
		{
			if ( game->OverObject&&game->SelectedVehicle->CanTransferTo ( game->OverObject ) )
			{
				if ( mouse->SetCursor ( CTransf ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else
		{
			if ( game->OverObject&&game->SelectedBuilding->CanTransferTo ( game->OverObject ) )
			{
				if ( mouse->SetCursor ( CTransf ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
	}
	else if ( !game->HelpActive )
	{

		if ( x<180 )
		{
			if ( mouse->SetCursor ( CHand ) )
			{
				game->OverObject=NULL;
				game->fDrawMap=true;
			}
			return;
		}

		if ( y<2+21&&y>=2&&x>=390&&x<390+72 )
		{
			if ( mouse->SetCursor ( CHand ) )
			{
				game->fDrawMap=true;
			}
			game->OverObject=NULL;
			LastOverEnde=true;
			return;
		}
		else if ( LastOverEnde )
		{
			LastOverEnde=false;
			MouseMoveCallback ( true );
		}

		if ( ( game->SelectedVehicle&&game->SelectedVehicle->MenuActive&&game->SelectedVehicle->MouseOverMenu ( x,y ) ) ||
		        ( game->SelectedBuilding&&game->SelectedBuilding->MenuActive&&game->SelectedBuilding->MouseOverMenu ( x,y ) ) )
		{
			if ( mouse->SetCursor ( CHand ) )
			{
				game->fDrawMap=true;
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->AttackMode&&game->SelectedVehicle->owner==game->ActivePlayer&&x>=180&&y>=18&&x<cSettingsData.iScreenW-12&&y<cSettingsData.iScreenH-14 )
		{
			if ( game->SelectedVehicle->IsInRange ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CAttack ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->StealActive&&game->SelectedVehicle->owner==game->ActivePlayer&&x>=180&&y>=18&&x<cSettingsData.iScreenW-12&&y<cSettingsData.iScreenH-14 )
		{
			if ( game->SelectedVehicle->IsInRangeCommando ( mouse->GetKachelOff(),true ) )
			{
				if ( mouse->SetCursor ( CSteal ) )
				{
					game->fDrawMap=true;
					MouseMoveCallback ( true );
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->DisableActive&&game->SelectedVehicle->owner==game->ActivePlayer&&x>=180&&y>=18&&x<cSettingsData.iScreenW-12&&y<cSettingsData.iScreenH-14 )
		{
			if ( game->SelectedVehicle->IsInRangeCommando ( mouse->GetKachelOff(),false ) )
			{
				if ( mouse->SetCursor ( CDisable ) )
				{
					game->fDrawMap=true;
					MouseMoveCallback ( true );
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedBuilding&&game->SelectedBuilding->AttackMode&&game->SelectedBuilding->owner==game->ActivePlayer&&x>=180&&y>=18&&x<cSettingsData.iScreenW-12&&y<cSettingsData.iScreenH-14 )
		{
			if ( game->SelectedBuilding->IsInRange ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CAttack ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->CanAttackObject ( mouse->GetKachelOff() ) )
		{
			if ( mouse->SetCursor ( CAttack ) )
			{
				game->fDrawMap=true;
				MouseMoveCallback ( true );
			}
		}
		else if ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->CanAttackObject ( mouse->GetKachelOff() ) )
		{
			if ( mouse->SetCursor ( CAttack ) )
			{
				game->fDrawMap=true;
				MouseMoveCallback ( true );
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->MuniActive )
		{
			if ( game->SelectedVehicle->CanMuni ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CMuni ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->RepairActive )
		{
			if ( game->SelectedVehicle->CanRepair ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CRepair ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->OverObject&&! ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&& ( ( game->SelectedVehicle->data.can_drive!=DRIVE_AIR&&!game->OverObject->vehicle&& ( !game->OverObject->top||game->OverObject->top->data.is_connector ) ) || ( game->SelectedVehicle->data.can_drive==DRIVE_AIR&&!game->OverObject->plane ) ) ) &&! ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->BuildList&&game->SelectedBuilding->BuildList->Count&&!game->SelectedBuilding->IsWorking&&game->SelectedBuilding->BuildList->BuildListItems[0]->metall_remaining<=0 ) &&! ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->LoadActive ) &&! ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->ActivatingVehicle ) &&! ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->LoadActive )
		          &&! ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->ActivatingVehicle ) )
		{
			if ( mouse->SetCursor ( CSelect ) )
			{
				game->fDrawMap=true;
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&& ( ( !game->SelectedVehicle->IsBuilding&&!game->SelectedVehicle->IsClearing&&!game->SelectedVehicle->LoadActive&&!game->SelectedVehicle->ActivatingVehicle ) || ( game->SelectedVehicle->IsBuilding&&game->SelectedVehicle->BuildRounds==0 ) || ( game->SelectedVehicle->IsClearing&&game->SelectedVehicle->ClearingRounds==0 ) ) &&x>=180&&y>=18&&x<180+ ( cSettingsData.iScreenW-192 ) &&y<18+ ( cSettingsData.iScreenH-32 ) )
		{
			if ( game->SelectedVehicle->MoveJobActive )
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
			else if ( game->SelectedVehicle->CanDrive ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CMove ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->BuildList&&game->SelectedBuilding->BuildList->Count&&!game->SelectedBuilding->IsWorking&&game->SelectedBuilding->BuildList->BuildListItems[0]->metall_remaining<=0 )
		{
			if ( game->SelectedBuilding->CanExitTo ( mouse->GetKachelOff(),game->SelectedBuilding->BuildList->BuildListItems[0]->typ ) )
			{
				if ( mouse->SetCursor ( CActivate ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->ActivatingVehicle )
		{
			if ( game->SelectedBuilding->CanExitTo ( mouse->GetKachelOff(),game->SelectedBuilding->StoredVehicles->VehicleItems[game->SelectedBuilding->VehicleToActivate]->typ ) )
			{
				if ( mouse->SetCursor ( CActivate ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->ActivatingVehicle )
		{
			if ( game->SelectedVehicle->CanExitTo ( mouse->GetKachelOff(),game->SelectedVehicle->StoredVehicles->VehicleItems[game->SelectedVehicle->VehicleToActivate]->typ ) )
			{
				if ( mouse->SetCursor ( CActivate ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedBuilding&&game->SelectedBuilding->owner==game->ActivePlayer&&game->SelectedBuilding->LoadActive )
		{
			if ( game->SelectedBuilding->CanLoad ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CLoad ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else if ( game->SelectedVehicle&&game->SelectedVehicle->owner==game->ActivePlayer&&game->SelectedVehicle->LoadActive )
		{
			if ( game->SelectedVehicle->CanLoad ( mouse->GetKachelOff() ) )
			{
				if ( mouse->SetCursor ( CLoad ) )
				{
					game->fDrawMap=true;
				}
			}
			else
			{
				if ( mouse->SetCursor ( CNo ) )
				{
					game->fDrawMap=true;
				}
			}
		}
		else
		{
			if ( mouse->SetCursor ( CHand ) )
			{
				game->fDrawMap=true;
			}
		}
	}
	else
	{
		if ( mouse->SetCursor ( CHelp ) )
		{
			game->fDrawMap=true;
		}
	}
}

void cHud::DoScroll ( int dir )
{
	static int lx=0,ly=0;
	cHud *hud;
	int step;
	if ( game->SelectedBuilding )
	{
		game->SelectedBuilding->MenuActive=false;
	}
	hud=game->hud;
	step=64/Zoom;
	step*=cSettingsData.iScrollSpeed;
	switch ( dir )
	{
		case 1:
			hud->OffX-=step;
			hud->OffY+=step;
			break;
		case 2:
			hud->OffY+=step;
			break;
		case 3:
			hud->OffX+=step;
			hud->OffY+=step;
			break;
		case 4:
			hud->OffX-=step;
			break;
		case 6:
			hud->OffX+=step;
			break;
		case 7:
			hud->OffX-=step;
			hud->OffY-=step;
			break;
		case 8:
			hud->OffY-=step;
			break;
		case 9:
			hud->OffX+=step;
			hud->OffY-=step;
			break;
	}
	if ( hud->OffX<0 ) hud->OffX=0;
	if ( hud->OffY<0 ) hud->OffY=0;
//  step=game->map->size*64-(int)((448.0/Zoom)*64);
	step=game->map->size*64- ( int ) ( ( ( cSettingsData.iScreenW-192.0 ) /Zoom ) *64 );
	if ( hud->OffX>=step ) hud->OffX=step;
	step=game->map->size*64- ( int ) ( ( ( cSettingsData.iScreenH-32.0 ) /Zoom ) *64 );
	if ( hud->OffY>=step ) hud->OffY=step;
	if ( lx==OffX&&ly==OffY ) return;
	game->fDrawMap=true;
	game->fDrawMMap=true;
	lx=OffX;ly=OffY;
}

void cHud::DoMinimapClick ( int x,int y )
{
	static int lx=0,ly=0;
	if ( lx==x&&ly==y ) return;
	lx=x;ly=y;
	OffX= ( int ) ( ( game->map->size/112.0 ) * ( x-15 ) *64- ( 224.0/Zoom ) *64 );
	OffY= ( int ) ( ( game->map->size/112.0 ) * ( y-356 ) *64- ( 224.0/Zoom ) *64 );
	game->fDrawMMap=true;
	game->fDrawMap=true;
	DoScroll ( 0 );
}

void cHud::ChechMouseOver ( void )
{
	static int lb=0;
	int x,y,b;
	x=mouse->x;
	y=mouse->y;
	b=mouse->GetMouseButton();
	if ( b==4 ) b=0;
	if ( y>274|| ( x>179&&y>22 ) ) return;
	// Der Pr�ferenzen-Button:
	if ( x>=86&&x<=86+67&&y>=4&&y<=4+18 )
	{
		if ( b ) PraeferenzenButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			game->ChangeObjectName=false;
			game->ChatInput=false;
			DoPraeferenzen();
			PraeferenzenButton ( false );
		}
	}
	else if ( Praeferenzen )
	{
		PraeferenzenButton ( false );
	}
	// Der Pause-Button:
	if ( x>=146&&x<164&&y>=143&&y<161 )
	{
		if ( b ) PauseButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			PlayFLC=false;
			PauseButton ( false );
		}
	}
	else if ( PausePressed )
	{
		PauseButton ( false );
	}
	// Der Play-Button:
	if ( x>=146&&x<164&&y>=123&&y<140 )
	{
		if ( b ) PlayButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			PlayFLC=true;
			PlayButton ( false );
		}
	}
	else if ( PlayPressed )
	{
		PlayButton ( false );
	}
	// Der Hilfe-Button:
	if ( x>=20&&x<46&&y>=250&&y<274 )
	{
		if ( b ) HelpButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			game->HelpActive=true;
			HelpButton ( false );
		}
	}
	else if ( HelpPressed )
	{
		HelpButton ( false );
	}
	// Der Chat-Button:
	if ( x>=51&&x<100&&y>=252&&y<272 )
	{
		if ( b ) ChatButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			game->ChatInput=true;
			InputStr="";
			ChatButton ( false );
		}
	}
	else if ( ChatPressed )
	{
		ChatButton ( false );
	}
	// Der Log-Button:
	if ( x>=102&&x<150&&y>=252&&y<272 )
	{
		if ( b ) LogButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			// Log-Men� aufrufen...
			game->AddMessage ( "what ever..." );

			LogButton ( false );
		}
	}
	else if ( LogPressed )
	{
		LogButton ( false );
	}
	// Der Ende-Button:
	if ( x>=392&&x<460&&y>=5&&y<20 )
	{
		if ( b ) EndeButton ( true );
		else if ( lb&&!Ende )
		{
			MakeMeMyEnd();
		}
	}
	else if ( EndePressed&&!Ende )
	{
		EndeButton ( false );
	}
	// Der Erledigen-Button:
	if ( x>=99&&x<124&&y>=227&&y<251 )
	{
		if ( b ) ErledigenButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			if ( game->SelectedVehicle&&game->SelectedVehicle->mjob&&game->SelectedVehicle->mjob->Suspended&&game->SelectedVehicle->data.speed )
			{
				game->SelectedVehicle->mjob->CalcNextDir();
				game->engine->AddActiveMoveJob ( game->SelectedVehicle->mjob );
			}
			ErledigenButton ( false );
		}
	}
	else if ( ErledigenPressed )
	{
		ErledigenButton ( false );
	}
	// Der Next-Button:
	if ( x>=124&&x<163&&y>=227&&y<250 )
	{
		if ( b ) NextButton ( true );
		else if ( lb )
		{
			cVehicle *v;
			PlayFX ( SNDHudButton );
			v=game->ActivePlayer->GetNextVehicle();
			if ( v )
			{
				if ( game->SelectedVehicle )
				{
					game->SelectedVehicle->Deselct();
					StopFXLoop ( game->ObjectStream );
				}
				v->Select();
				v->Center();
				game->ObjectStream=v->PlayStram();
				game->SelectedVehicle=v;
			}
			NextButton ( false );
		}
	}
	else if ( NextPressed )
	{
		NextButton ( false );
	}
	// Der Prev-Button:
	if ( x>=60&&x<98&&y>=227&&y<250 )
	{
		if ( b ) PrevButton ( true );
		else if ( lb )
		{
			cVehicle *v;
			PlayFX ( SNDHudButton );
			v=game->ActivePlayer->GetPrevVehicle();
			if ( v )
			{
				if ( game->SelectedVehicle )
				{
					game->SelectedVehicle->Deselct();
					StopFXLoop ( game->ObjectStream );
				}
				v->Select();
				v->Center();
				game->ObjectStream=v->PlayStram();
				game->SelectedVehicle=v;
			}
			PrevButton ( false );
		}
	}
	else if ( PrevPressed )
	{
		PrevButton ( false );
	}
	// Der Center-Button:
	if ( x>=4&&x<26&&y>=227&&y<249 )
	{
		if ( b ) CenterButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			if ( game->SelectedVehicle )
			{
				game->SelectedVehicle->Center();
			}
			else if ( game->SelectedBuilding )
			{
				game->SelectedBuilding->Center();
			}
			CenterButton ( false );
		}
	}
	else if ( CenterPressed )
	{
		CenterButton ( false );
	}
	// Der Datei-Button:
	if ( x>=17&&x<84&&y>=3&&y<23 )
	{
		if ( b ) DateiButton ( true );
		else if ( lb )
		{
			PlayFX ( SNDHudButton );
			game->ShowDateiMenu();
			DateiButton ( false );
		}
	}
	else if ( DateiPressed )
	{
		DateiButton ( false );
	}
	lb=b;
}

void cHud::PraeferenzenButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=86;
	dest.y=4;
	dest.w=scr.w=67;
	dest.h=scr.h=20;
	if ( set )
		{
			scr.x=195;
			scr.y=0;
			Praeferenzen=true;
		}
	else
	{
		scr.x=0;
		scr.y=169;
		Praeferenzen=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

// Setzt den Monitor zur�ck:
void cHud::ResetVideoMonitor ( void )
{
	SDL_Rect scr,dest;
	scr.x=295;
	scr.y=98;
	dest.x=10;
	dest.y=29;
	scr.w=dest.w=128;
	scr.h=dest.h=128;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::PauseButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=146;
	dest.y=143;
	dest.w=scr.w=19;
	dest.h=scr.h=19;
	if ( set )
		{
			scr.x=176;
			scr.y=0;
			PausePressed=true;
		}
	else
	{
		scr.x=19;
		scr.y=132;
		PausePressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::PlayButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=146;
	dest.y=123;
	dest.w=scr.w=19;
	dest.h=scr.h=18;
	if ( set )
		{
			scr.x=157;
			scr.y=0;
			PlayPressed=true;
		}
	else
	{
		scr.x=0;
		scr.y=132;
		PlayPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::HelpButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=20;
	dest.y=250;
	dest.w=scr.w=26;
	dest.h=scr.h=24;
	if ( set )
		{
			scr.x=366;
			scr.y=0;
			HelpPressed=true;
		}
	else
	{
		scr.x=268;
		scr.y=151;
		HelpPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::ChatButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=51;
	dest.y=252;
	dest.w=scr.w=49;
	dest.h=scr.h=20;
	if ( set )
		{
			scr.x=210;
			scr.y=21;
			ChatPressed=true;
		}
	else
	{
		scr.x=245;
		scr.y=130;
		ChatPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::LogButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=102;
	dest.y=252;
	dest.w=scr.w=49;
	dest.h=scr.h=20;
	if ( set )
		{
			scr.x=160;
			scr.y=21;
			LogPressed=true;
		}
	else
	{
		scr.x=196;
		scr.y=129;
		LogPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::EndeButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=391;
	dest.y=4;
	dest.w=scr.w=70;
	dest.h=scr.h=17;
	if ( set )
		{
			scr.x=22;
			scr.y=21;
			EndePressed=true;
		}
	else
	{
		scr.x=0;
		scr.y=151;
		EndePressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::ShowRunde ( void )
{
	SDL_Rect scr,dest;
	char str[10];
	scr.x=156;
	scr.y=215;
	dest.w=scr.w=55;
	dest.h=scr.h=15;
	dest.x=471;
	dest.y=5;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
	sprintf ( str,"%d",game->Runde );
	fonts->OutTextCenter ( str,498,8,GraphicsData.gfx_hud );
	game->fDrawHud=true;
}

void cHud::ErledigenButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=99;
	dest.y=227;
	dest.w=scr.w=25;
	dest.h=scr.h=24;
	if ( set )
		{
			scr.x=262;
			scr.y=0;
			ErledigenPressed=true;
		}
	else
	{
		scr.x=132;
		scr.y=172;
		ErledigenPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::NextButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=124;
	dest.y=227;
	dest.w=scr.w=39;
	dest.h=scr.h=23;
	if ( set )
		{
			scr.x=288;
			scr.y=0;
			NextPressed=true;
		}
	else
	{
		scr.x=158;
		scr.y=172;
		NextPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::PrevButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=60;
	dest.y=227;
	dest.w=scr.w=38;
	dest.h=scr.h=23;
	if ( set )
		{
			scr.x=327;
			scr.y=0;
			PrevPressed=true;
		}
	else
	{
		scr.x=198;
		scr.y=172;
		PrevPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::CenterButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=4;
	dest.y=227;
	dest.w=scr.w=21;
	dest.h=scr.h=22;
	if ( set )
		{
			scr.x=0;
			scr.y=21;
			CenterPressed=true;
		}
	else
	{
		scr.x=139;
		scr.y=149;
		CenterPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::DateiButton ( bool set )
{
	SDL_Rect scr,dest;
	dest.x=17;
	dest.y=3;
	dest.w=scr.w=67;
	dest.h=scr.h=20;
	if ( set )
		{
			scr.x=93;
			scr.y=21;
			DateiPressed=true;
		}
	else
	{
		scr.x=71;
		scr.y=151;
		DateiPressed=false;
	}
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,GraphicsData.gfx_hud,&dest );
}

void cHud::ScaleSurfaces ( void )
{
	int i, k, sizex, sizey;
	TList *tlist;
	int fak;
	if ( Zoom==LastZoom ) return;

	// Terrain:
	tlist=game->map->TerrainInUse;
	for ( i=0;i<tlist->Count;i++ )
	{
		sTerrain *t;
		t= tlist->TerItems[i];
		if ( Zoom==64 )
		{
			t->sf->w=t->sf_org->w;t->sf->h=t->sf_org->h;
			if ( t->overlay )
			{
				SDL_SetColorKey ( t->sf_org,SDL_SRCCOLORKEY,-1 );
				SDL_SetColorKey ( t->shw_org,SDL_SRCCOLORKEY,-1 );
			}
			SDL_BlitSurface ( t->sf_org,NULL,t->sf,NULL );
			t->shw->w=t->shw_org->w;t->shw->h=t->shw_org->h;
			SDL_BlitSurface ( t->shw_org,NULL,t->shw,NULL );
			if ( t->overlay )
			{
				SDL_SetColorKey ( t->sf_org,SDL_SRCCOLORKEY,0xFF00FF );
				SDL_SetColorKey ( t->shw_org,SDL_SRCCOLORKEY,0xFFCD00CD );
				SDL_SetColorKey ( t->sf,SDL_SRCCOLORKEY,0xFF00FF );
				SDL_SetColorKey ( t->shw,SDL_SRCCOLORKEY,0xCD00CD );
			}
		}
		else
		{
			ScaleSurface2 ( t->sf_org,t->sf,Zoom );
			ScaleSurface2 ( t->shw_org,t->shw,Zoom );
			if ( t->overlay )
			{
				SDL_SetColorKey ( t->shw,SDL_SRCCOLORKEY,0xFFCD00CD );
			}
		}
	}
	// Vehicles:
	fak= ( int ) ( Zoom/64.0 );
	for ( i=0;i<VehicleMainData.vehicle_anz;i++ )
	{
		for ( k=0;k<8;k++ )
		{
			sizex= ( VehicleMainData.vehicle[i].img_org[k]->w*fak );
			sizey= ( VehicleMainData.vehicle[i].img_org[k]->h*fak );
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].img_org[k],VehicleMainData.vehicle[i].img[k],sizex,sizey );
			sizex= ( VehicleMainData.vehicle[i].shw_org[k]->w*fak );
			sizey= ( VehicleMainData.vehicle[i].shw_org[k]->h*fak );
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].shw_org[k],VehicleMainData.vehicle[i].shw[k],sizex,sizey );
		}
		if ( VehicleMainData.vehicle[i].build_org )
		{
			sizey= ( VehicleMainData.vehicle[i].build_org->h*fak );
			sizex=sizey*4;
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].build_org,VehicleMainData.vehicle[i].build,sizex,sizey );
			sizex= ( VehicleMainData.vehicle[i].build_shw_org->w*fak );
			sizey= ( VehicleMainData.vehicle[i].build_shw_org->h*fak );
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].build_shw_org,VehicleMainData.vehicle[i].build_shw,sizex,sizey );
		}
		if ( VehicleMainData.vehicle[i].clear_small_org )
		{
			sizey= ( VehicleMainData.vehicle[i].clear_small_org->h*fak );
			sizex=sizey*4;
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].clear_small_org,VehicleMainData.vehicle[i].clear_small,sizex,sizey );
			sizex= ( VehicleMainData.vehicle[i].clear_small_shw_org->w*fak );
			sizey= ( VehicleMainData.vehicle[i].clear_small_shw_org->h*fak );
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].clear_small_shw_org,VehicleMainData.vehicle[i].clear_small_shw,sizex,sizey );
		}
		if ( VehicleMainData.vehicle[i].overlay_org )
		{
			sizey= ( VehicleMainData.vehicle[i].overlay_org->h*fak );
			sizex= ( VehicleMainData.vehicle[i].overlay_org->w*fak );
			ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].overlay_org,VehicleMainData.vehicle[i].overlay,sizex,sizey );
		}
	}
	// Buildings:
	fak= ( int ) ( Zoom/64.0 );
	for ( i=0;i<BuildingMainData.building_anz;i++ )
	{
		ScaleSurfaceAdv2 ( BuildingMainData.building[i].img_org,BuildingMainData.building[i].img,BuildingMainData.building[i].img_org->w*fak,BuildingMainData.building[i].img_org->h*fak );
		ScaleSurfaceAdv2 ( BuildingMainData.building[i].shw_org,BuildingMainData.building[i].shw,BuildingMainData.building[i].shw_org->w*fak,BuildingMainData.building[i].shw_org->h*fak );
		if ( BuildingMainData.building[i].eff_org )
		{
			if ( Zoom==64 ) ScaleSurfaceAdv2 ( BuildingMainData.building[i].eff_org,BuildingMainData.building[i].eff,BuildingMainData.building[i].eff_org->w*fak,BuildingMainData.building[i].eff_org->h*fak );
			else ScaleSurfaceAdv2Spec ( BuildingMainData.building[i].eff_org,BuildingMainData.building[i].eff,BuildingMainData.building[i].eff_org->w*fak,BuildingMainData.building[i].eff_org->h*fak );
		}
	}
	ScaleSurfaceAdv2 ( BuildingMainData.dirt_small_org,BuildingMainData.dirt_small,BuildingMainData.dirt_small_org->w*fak,BuildingMainData.dirt_small_org->h*fak );
	ScaleSurfaceAdv2 ( BuildingMainData.dirt_small_shw_org,BuildingMainData.dirt_small_shw,BuildingMainData.dirt_small_shw_org->w*fak,BuildingMainData.dirt_small_shw_org->h*fak );
	ScaleSurfaceAdv2 ( BuildingMainData.dirt_big_org,BuildingMainData.dirt_big,BuildingMainData.dirt_big_org->w*fak,BuildingMainData.dirt_big_org->h*fak );
	ScaleSurfaceAdv2 ( BuildingMainData.dirt_big_shw_org,BuildingMainData.dirt_big_shw,BuildingMainData.dirt_big_shw_org->w*fak,BuildingMainData.dirt_big_shw_org->h*fak );

	// B�nder:
	ScaleSurface2 ( GraphicsData.gfx_band_small_org,GraphicsData.gfx_band_small,Zoom );
	ScaleSurface2 ( GraphicsData.gfx_band_big_org,GraphicsData.gfx_band_big,Zoom*2 );

	// Resources:
	ScaleSurface2 ( ResourceData.res_metal_org,ResourceData.res_metal,Zoom );
	ScaleSurface2 ( ResourceData.res_oil_org,ResourceData.res_oil,Zoom );
	ScaleSurface2 ( ResourceData.res_gold_org,ResourceData.res_gold,Zoom );

	// Big Beton:
	ScaleSurface2 ( GraphicsData.gfx_big_beton_org,GraphicsData.gfx_big_beton,Zoom*2 );

	// Andere:
	ScaleSurface2 ( GraphicsData.gfx_exitpoints_org,GraphicsData.gfx_exitpoints,Zoom );
	// ScaleSurface2(GraphicsData.gfx_build_finished_org,GraphicsData.gfx_build_finished,Zoom*2);

	// FX:
#define SCALE_FX(a,b) ScaleSurfaceAdv2(a[0],a[1],(int)((a[0]->w/a[0]->h*b)),b);
	SCALE_FX ( EffectsData.fx_explo_small0,Zoom );
	SCALE_FX ( EffectsData.fx_explo_small1,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_small2,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_big0,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_big1,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_big2,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_big3,Zoom*2 );
	SCALE_FX ( EffectsData.fx_explo_big4,Zoom*2 );
	SCALE_FX ( EffectsData.fx_muzzle_big,Zoom );
	SCALE_FX ( EffectsData.fx_muzzle_small,Zoom );
	SCALE_FX ( EffectsData.fx_muzzle_med,Zoom );
	SCALE_FX ( EffectsData.fx_hit,Zoom );
	SCALE_FX ( EffectsData.fx_smoke, ( int ) ( Zoom/4 ) );
	SCALE_FX ( EffectsData.fx_rocket, ( int ) ( Zoom/2.28 ) );
	SCALE_FX ( EffectsData.fx_dark_smoke, ( int ) ( 0.375*Zoom ) );
	SCALE_FX ( EffectsData.fx_tracks,Zoom );
	SCALE_FX ( EffectsData.fx_corpse,Zoom );
	SCALE_FX ( EffectsData.fx_absorb,Zoom );

	LastZoom=Zoom;
}

// Dr�ck Ende:
void cHud::MakeMeMyEnd ( void )
{
	PlayFX ( SNDHudButton );
	if ( game->engine->CheckVehiclesMoving ( false ) )
	{
		EndeButton ( false );
		game->AddMessage ( "Warten Sie bis alle Bewegungen abgeschlossen sind" );
	}
	else
	{
//    if(game->engine->DoEndActions()&&(!game->engine->com||game->engine->com->server)){
		if ( game->engine->DoEndActions() )
		{
			game->AddMessage ( "Verbleibende Bewegungen werden durchgef�hrt" );
			game->WantToEnd=true;
			if ( !game->HotSeat )
				Ende=true;
		}
		else
		{
			if ( game->HotSeat )
			{
				if ( game->MakeHotSeatEnde() )
				{
					game->engine->EndePressed ( game->ActivePlayer->Nr );
					Ende=true;
				}
			}
			else
			{
				game->engine->EndePressed ( game->ActivePlayer->Nr );
				Ende=true;
			}
		}
	}
}
