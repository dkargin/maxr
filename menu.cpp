//////////////////////////////////////////////////////////////////////////////
// M.A.X. - menu.cpp
//////////////////////////////////////////////////////////////////////////////
#include <time.h>
#include <math.h>
#include "menu.h"
#include "pcx.h"
#include "fonts.h"
#include "mouse.h"
#include "keyinp.h"
#include "sound.h"
#include "dialog.h"
#include "game.h"
#include "log.h"


// Men� vorbereiten:
void EnterMenu ( bool limited )
{
	if ( !limited )
	{
//    TmpSf=SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_SRCCOLORKEY,640,480,32,0,0,0,0);
		TmpSf=GraphicsData.gfx_shadow;
		SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );

		LoadPCXtoSF ( GFXOD_MAIN,TmpSf );
		string txt = "MichaelMoenchs' MAX (Remake by DoctorDeath) ";
		txt+=MAX_VERSION;
		fonts->OutTextCenter ( ( char * ) txt.c_str(),320,465,TmpSf );
	}
	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Hauptmen�",320,147,buffer );

	PlaceButton ( "Einzelspieler",390,190,false );
	PlaceButton ( "Mehrspieler",390,190+35,false );
	PlaceButton ( "Map-Editor",390,190+35*2,false );
	PlaceButton ( "Credits",390,190+35*3,false );
	PlaceButton ( "Beenden",390,190+35*5,false );

	ShowInfo();
	mouse->Show();
	mouse->SetCursor ( CHand );
}

// Zeigt ein Infobild an:
void ShowInfo ( void )
{
	SDL_Rect dest;
	static int LastInfoNr;
	int nr;
	nr=BuildingMainData.building_anz + BuildingMainData.building_anz;
	nr=random ( nr,0 );
	if ( nr == LastInfoNr ) nr++;
	LastInfoNr = nr;
	dest.x=16;
	dest.y=182;
	dest.w=320;
	dest.h=240;
	if ( nr>=VehicleMainData.vehicle_anz )
	{
		nr-=VehicleMainData.vehicle_anz;
		SDL_BlitSurface ( BuildingMainData.building[nr].info,NULL,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( VehicleMainData.vehicle[nr].info,NULL,buffer,&dest );
	}
}

// Men� aufr�umen:
void ExitMenu ( void )
{
//  SDL_FreeSurface(TmpSf);
	SDL_FillRect ( GraphicsData.gfx_shadow,NULL,0x0 );
	SDL_SetAlpha ( GraphicsData.gfx_shadow,SDL_SRCALPHA,50 );
}

void PlaceButton ( char *str,int x,int y,bool pressed )
{
	SDL_Rect scr,dest;
	scr.w=dest.w=200;
	scr.h=dest.h=29;
	scr.x=0;
	if ( pressed ) scr.y=30;else scr.y=0;
	dest.x=x;
	dest.y=y;
	SDL_BlitSurface ( GraphicsData.gfx_menu_stuff,&scr,buffer,&dest );

	fonts->OutTextBigCenter ( str,x+100,y+8,buffer );
}

// Platziert einen kleinen Button:
void PlaceSmallButton ( char *str,int x,int y,bool pressed )
{
	SDL_Rect scr,dest;
	scr.w=dest.w=150;
	scr.h=dest.h=29;
	scr.x=0;
	if ( pressed ) scr.y=90;else scr.y=60;
	dest.x=x;
	dest.y=y;
	SDL_BlitSurface ( GraphicsData.gfx_menu_stuff,&scr,buffer,&dest );

	fonts->OutTextBigCenter ( str,x+150/2,y+8,buffer );
}

// Platziert einen spezielen Men�button:
void PlaceMenuButton ( char *str,int x,int y, int darkness, bool pressed )
{
	SDL_Rect scr,dest;
	scr.w=dest.w=109;
	scr.h=dest.h=40;
	scr.y=0;
	scr.x=218 * darkness;
	if ( pressed ) scr.x+=110;
	if ( pressed ) scr.x--;
	dest.x=x;
	dest.y=y;
	SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );

	fonts->OutTextBigCenter ( str,x+109/2,y+12,buffer );
}

// Platziert einen kleinen spezielen Men�button:
void PlaceSmallMenuButton ( char *str,int x,int y,bool pressed )
{
	SDL_Rect scr,dest;
	scr.w=dest.w=48;
	scr.h=dest.h=40;
	scr.y=40;
	if ( pressed ) scr.x=49;else scr.x=0;
	dest.x=x;
	dest.y=y;
	SDL_BlitSurface ( GraphicsData.gfx_menu_buttons,&scr,buffer,&dest );

	fonts->OutTextBigCenter ( str,x+48/2,y+12,buffer );
}

// Platziert einen ausw�hlbaren Text (zentriert):
void PlaceSelectText ( char *str,int x,int y,bool checked,bool center )
{
	SDL_Rect r;
	int len;
	len=fonts->GetTextLen ( str );
	if ( center )
	{
		fonts->OutTextCenter ( str,x,y,buffer );
	}
	else
	{
		fonts->OutText ( str,x,y,buffer );
		x+=len/2;
	}
	r.x=x-len/2-4;
	r.w=len+8;
	r.h=1;
	r.y=y-2;
	if ( checked ) SDL_FillRect ( buffer,&r,0xE3DACF );else SDL_BlitSurface ( TmpSf,&r,buffer,&r );
	r.y+=14;
	r.w++;
	if ( checked ) SDL_FillRect ( buffer,&r,0xE3DACF );else SDL_BlitSurface ( TmpSf,&r,buffer,&r );
	r.y-=14;
	r.w=1;
	r.h=14;
	if ( checked ) SDL_FillRect ( buffer,&r,0xE3DACF );else SDL_BlitSurface ( TmpSf,&r,buffer,&r );
	r.x+=len+8;
	if ( checked ) SDL_FillRect ( buffer,&r,0xE3DACF );else SDL_BlitSurface ( TmpSf,&r,buffer,&r );
}

// Zeigt das Hauptmen� an:
void RunMainMenu ( void )
{
	bool SPPressed=false,MPPRessed=false,MEPressed=false,CrPressed=false,BePressed=false;
	bool EscHot=true;
	Uint8 *keystate;
	int b,lb=0,lx=-1,ly=-1;

	EnterMenu();
	SHOW_SCREEN

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE]&&EscHot ) break;else if ( !keystate[SDLK_ESCAPE] ) EscHot=true;
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}

		// Den Focus machen:
		if ( DoKeyInp ( keystate ) )
		{
			//ShowCursor=true;
		}

		// Klick aufs Bild:
		if ( b&&!lb&&mouse->x>=16&&mouse->x<16+320&&mouse->y>=182&&mouse->y<182+240 )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			ShowInfo();
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Einzelspieler:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190&&mouse->y<190+29 )
		{
			if ( b&&!lb )
			{
				SPPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Einzelspieler",390,190,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&SPPressed )
			{
				RunSPMenu();
				if ( TmpSf==NULL )
				{
					EnterMenu();
				}
				else
				{
					EnterMenu ( true );
				}
				SHOW_SCREEN
				SPPressed=false;
				EscHot=false;
			}
		}
		else if ( SPPressed )
		{
			SPPressed=false;
			PlaceButton ( "Einzelspieler",390,190,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Mehrspieler:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35&&mouse->y<190+35+29 )
		{
			if ( b&&!lb )
			{
				MPPRessed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Mehrspieler",390,190+35,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&MPPRessed )
			{
				RunMPMenu();
				if ( TmpSf==NULL )
				{
					EnterMenu();
				}
				else
				{
					EnterMenu ( true );
				}
				SHOW_SCREEN
				SPPressed=false;
				EscHot=false;
			}
		}
		else if ( MPPRessed )
		{
			MPPRessed=false;
			PlaceButton ( "Mehrspieler",390,190+35,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Map-Editor:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*2&&mouse->y<190+35*2+29 )
		{
			if ( b&&!lb )
			{
				MEPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Map-Editor",390,190+35*2,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&MEPressed )
			{
				/*cMapEditor *me;
				ExitMenu();

				me=new cMapEditor();
				me->Run();
				delete me;*/

				EnterMenu();
				SHOW_SCREEN
				MEPressed=false;
				EscHot=false;
			}
		}
		else if ( MEPressed )
		{
			MEPressed=false;
			PlaceButton ( "Map-Editor",390,190+35*2,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Credits:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*3&&mouse->y<190+35*3+29 )
		{
			if ( b&&!lb )
			{
				CrPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Credits",390,190+35*3,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&CrPressed )
			{
				/*cCredits *cred;
				cred=new cCredits();
				cred->Run();
				delete cred;*/
				EnterMenu();
				SHOW_SCREEN
				EscHot=false;
				CrPressed=false;
			}
		}
		else if ( CrPressed )
		{
			CrPressed=false;
			PlaceButton ( "Credits",390,190+35*3,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}

		// Beenden:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*5&&mouse->y<190+35*5+29 )
		{
			if ( b&&!lb )
			{
				BePressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Beenden",390,190+35*5,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&BePressed )
			{
				break;
			}
		}
		else if ( BePressed )
		{
			BePressed=false;
			PlaceButton ( "Beenden",390,190+35*5,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}

		lb=b;
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}

	ExitMenu();
	StopMusic();
}

// Zeigt das Multiplayermen� an:
void RunMPMenu ( void )
{
	bool TCPHostPressed=false,TCPClientPressed=false,BackPressed=false,HotSeatPressed=false,LoadHotSeatPressed=false;
	Uint8 *keystate;
	int b,lb=0,lx=-1,ly=-1;

	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Mehrspieler",320,147,buffer );

	PlaceButton ( "TCP/IP Host",390,190,false );
	PlaceButton ( "TCP/IP Client",390,190+35,false );
	PlaceButton ( "Hot Seat",390,190+35*2,false );
	PlaceButton ( "Hot Seat laden",390,190+35*3,false );
	PlaceButton ( "Zur�ck",390,190+35*5,false );

	ShowInfo();
	mouse->Show();
	mouse->SetCursor ( CHand );
	SHOW_SCREEN

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE] ) break;
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}

		// Klick aufs Bild:
		if ( b&&!lb&&mouse->x>=16&&mouse->x<16+320&&mouse->y>=182&&mouse->y<182+240 )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			ShowInfo();
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// TCP Host:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190&&mouse->y<190+29 )
		{
			if ( b&&!lb )
			{
				TCPHostPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "TCP/IP Host",390,190,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&TCPHostPressed )
			{
				MultiPlayer=new cMultiPlayer ( true,true );
				MultiPlayer->RunMenu();
				delete MultiPlayer;
				break;
			}
		}
		else if ( TCPHostPressed )
		{
			TCPHostPressed=false;
			PlaceButton ( "TCP/IP Host",390,190,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// TCP Client:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35&&mouse->y<190+35+29 )
		{
			if ( b&&!lb )
			{
				TCPClientPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "TCP/IP Client",390,190+35,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&TCPClientPressed )
			{
				MultiPlayer=new cMultiPlayer ( false,true );
				MultiPlayer->RunMenu();
				delete MultiPlayer;
				break;
			}
		}
		else if ( TCPClientPressed )
		{
			TCPClientPressed=false;
			PlaceButton ( "TCP/IP Client",390,190+35,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Hot Seat:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*2&&mouse->y<190+35*2+29 )
		{
			if ( b&&!lb )
			{
				HotSeatPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Hot Seat",390,190+35*2,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&HotSeatPressed )
			{
				HeatTheSeat();
				break;
			}
		}
		else if ( HotSeatPressed )
		{
			HotSeatPressed=false;
			PlaceButton ( "Hot Seat",390,190+35*2,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Hot Seat laden:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*3&&mouse->y<190+35*3+29 )
		{
			if ( b&&!lb )
			{
				LoadHotSeatPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Hot Seat laden",390,190+35*3,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&LoadHotSeatPressed )
			{
				if ( ShowDateiMenu() != -1 )
				{
					cMap *map;
					map=new cMap();
					game=new cGame ( NULL, map );
					ExitMenu();
					TmpSf=NULL;
					if ( !LoadFile.empty() )
					{
						game->HotSeat=true;
						game->Load ( LoadFile,0 );
					}
					delete game;game=NULL;
					delete map;
					break;
				}
				EnterMenu();
				RunSPMenu();
				break;
			}
		}
		else if ( LoadHotSeatPressed )
		{
			LoadHotSeatPressed=false;
			PlaceButton ( "Hot Seat laden",390,190+35*3,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Zur�ck:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*5&&mouse->y<190+35*5+29 )
		{
			if ( b&&!lb )
			{
				BackPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Zur�ck",390,190+35*5,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&BackPressed )
			{
				break;
			}
		}
		else if ( BackPressed )
		{
			BackPressed=false;
			PlaceButton ( "Zur�ck",390,190+35*5,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		lb=b;
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}
}

void RunSPMenu ( void )
{
	bool StartTrainingPressed=false, StartNewPressed=false, LoadPressed=false, BackPressed=false;
	Uint8 *keystate;
	int b,lb=0,lx=-1,ly=-1;

	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Einzelspieler",320,147,buffer );

	PlaceButton ( "Training starten",390,190,false );
	PlaceButton ( "Neues Spiel starten",390,190+35,false );
	PlaceButton ( "Spiel laden",390,190+35*2,false );
	PlaceButton ( "Zur�ck",390,190+35*4,false );

	ShowInfo();
	mouse->Show();
	mouse->SetCursor ( CHand );
	SHOW_SCREEN

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE] ) break;
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}
		// Klick aufs Bild:
		if ( b&&!lb&&mouse->x>=16&&mouse->x<16+320&&mouse->y>=182&&mouse->y<182+240 )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			ShowInfo();
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Training starten:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190&&mouse->y<190+29 )
		{
			if ( b&&!lb )
			{
				StartTrainingPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Training starten",390,190,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&StartTrainingPressed )
			{
				StartTrainingPressed=false;
				PlaceButton ( "Training starten",390,190,false );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		else if ( StartTrainingPressed )
		{
			StartTrainingPressed=false;
			PlaceButton ( "Training starten",390,190,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}

		// Neues Spiel starten:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35&&mouse->y<190+29+35 )
		{
			if ( b&&!lb )
			{
				StartNewPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Neues Spiel starten",390,190+35,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&StartNewPressed )
			{
				sOptions options;
				string MapName = "";
				options = RunOptionsMenu ( NULL );
				if ( options.metal == -1 ) break;
				MapName = RunPlanetSelect();

				if ( !MapName.empty() )
				{
					int i,LandX,LandY;
					TList *list, *LandingList;
					cMap *map;
					cPlayer *p;
					map=new cMap;
					sPlayer players;
					if ( !map->LoadMap ( MapName ) )
					{
						delete map;
						break;
					}
					map->PlaceRessources ( options.metal,options.oil,options.gold,options.dichte );
					players = RunPlayerSelect();

					list=new TList;
					list->AddPlayer ( p=new cPlayer ( SettingsData.sPlayerName.c_str(),OtherData.colors[cl_red],1 ) );
					list->AddPlayer ( new cPlayer ( "Player 2",OtherData.colors[cl_green],2 ) );

					game = new cGame ( NULL, map );
					game->AlienTech = options.AlienTech;
					game->PlayRounds = options.PlayRounds;
					game->ActiveRoundPlayerNr = p->Nr;
					game->Init ( list,0 );

					for ( i=0;i<list->Count;i++ )
						list->PlayerItems[i]->InitMaps ( map->size );

					p->Credits=options.credits;

					LandingList = new TList;
					RunHangar ( p, LandingList );
					SelectLanding ( &LandX, &LandY, map );
					game->MakeLanding ( LandX,LandY,p,LandingList,options.FixedBridgeHead );

					ExitMenu();

					game->Run();

					SettingsData.sPlayerName=p->name;
					while ( list->Count )
					{
						delete ( ( cPlayer* ) ( list->PlayerItems[0] ) );
						list->DeletePlayer ( 0 );
					}
					delete game; game=NULL;
					delete map;
					break;
				}
				break;
			}
		}
		else if ( StartNewPressed )
		{
			StartNewPressed=false;
			PlaceButton ( "Neues Spiel starten",390,190+35,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Spiel laden:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*2&&mouse->y<190+35*2+29 )
		{
			if ( b&&!lb )
			{
				LoadPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Spiel laden",390,190+35*2,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&LoadPressed )
			{
				if ( ShowDateiMenu() != -1 )
				{
					cMap *map;
					map=new cMap();
					game=new cGame ( NULL, map );
					ExitMenu();
					TmpSf=NULL;
					if ( !LoadFile.empty() )
					{
						game->Load ( LoadFile,0 );
					}
					delete game; game=NULL;
					delete map;
					break;
				}
				EnterMenu();
				RunSPMenu();
				break;
			}
		}
		else if ( LoadPressed )
		{
			LoadPressed=false;
			PlaceButton ( "Spiel laden",390,190+35*2,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Zur�ck:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=190+35*4&&mouse->y<190+35*4+29 )
		{
			if ( b&&!lb )
			{
				BackPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Zur�ck",390,190+35*4,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&BackPressed )
			{
				break;
			}
		}
		else if ( BackPressed )
		{
			BackPressed=false;
			PlaceButton ( "Zur�ck",390,190+35*4,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		lb=b;
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}
}

// Zeigt die Optionen an:
sOptions RunOptionsMenu ( sOptions *init )
{
	bool OKPressed=false, BackPressed=false;
	sOptions options;
	int b,lb=0,lx=-1,ly=-1;

	if ( init==NULL )
	{
		options.metal=1;
		options.oil=1;
		options.gold=1;
		options.dichte=1;
		options.credits=100;
		options.FixedBridgeHead=true;
		options.AlienTech=false;
		options.PlayRounds=false;
	}
	else
	{
		options=*init;
	}

	TmpSf=GraphicsData.gfx_shadow;
	SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );

	LoadPCXtoSF ( GFXOD_OPTIONS,TmpSf );
	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Spieloptionen",320,11,buffer );

	// Ressourcen:
	fonts->OutTextCenter ( "Ressourcen",110,56,buffer );

	fonts->OutText ( "Metall:",17,86,buffer );
	PlaceSelectText ( "wenig",38,86+16,options.metal==0 );
	PlaceSelectText ( "mittel",38+45,86+16,options.metal==1 );
	PlaceSelectText ( "viel",38+45*2,86+16,options.metal==2 );
	PlaceSelectText ( "extrem",38+45*3,86+16,options.metal==3 );

	fonts->OutText ( "�l:",17,124,buffer );
	PlaceSelectText ( "wenig",38,124+16,options.oil==0 );
	PlaceSelectText ( "mittel",38+45,124+16,options.oil==1 );
	PlaceSelectText ( "viel",38+45*2,124+16,options.oil==2 );
	PlaceSelectText ( "extrem",38+45*3,124+16,options.oil==3 );

	fonts->OutText ( "Gold:",17,162,buffer );
	PlaceSelectText ( "wenig",38,162+16,options.gold==0 );
	PlaceSelectText ( "mittel",38+45,162+16,options.gold==1 );
	PlaceSelectText ( "viel",38+45*2,162+16,options.gold==2 );
	PlaceSelectText ( "extrem",38+45*3,162+16,options.gold==3 );

	// Credits:
	fonts->OutTextCenter ( "Credits",110+211,56,buffer );

	PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
	PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
	PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
	PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
	PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
	PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
	PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );

	// Br�ckenkopf:
	fonts->OutTextCenter ( "Br�ckenkopf",110+211*2,56,buffer );

	PlaceSelectText ( "Mobil",452,86,!options.FixedBridgeHead,false );
	PlaceSelectText ( "Fest",452,86+20,options.FixedBridgeHead,false );

	// AlienTechs:
	fonts->OutTextCenter ( "Alientechnologie",110,251,buffer );

	PlaceSelectText ( "ein",38,281,options.AlienTech );
	PlaceSelectText ( "aus",38,281+20,!options.AlienTech );

	// Ressourcendichte:
	fonts->OutTextCenter ( "Ressourcendichte",110+211,251,buffer );

	PlaceSelectText ( "d�nn",110+130,281,options.dichte==0,false );
	PlaceSelectText ( "normal",110+130,281+20,options.dichte==1,false );
	PlaceSelectText ( "dicht",110+130,281+20*2,options.dichte==2,false );
	PlaceSelectText ( "extrem",110+130,281+20*3,options.dichte==3,false );

	// Spielart:
	fonts->OutTextCenter ( "Spielart",110+211*2,251,buffer );

	PlaceSelectText ( "simultan",452,281,!options.PlayRounds,false );
	PlaceSelectText ( "Runden",452,281+20,options.PlayRounds,false );

	PlaceButton ( "Ok",390,440,false );
	PlaceButton ( "Zur�ck",50,440,false );
	SHOW_SCREEN
	mouse->draw ( false,screen );

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}

		// Klick aufs Metall:
		if ( b&&!lb&&mouse->x>=38-20&&mouse->x<38+20&&mouse->y>=86+16-4&&mouse->y<86+16-4+14 )
		{
			options.metal=0;
			PlaceSelectText ( "wenig",38,86+16,options.metal==0 );
			PlaceSelectText ( "mittel",38+45,86+16,options.metal==1 );
			PlaceSelectText ( "viel",38+45*2,86+16,options.metal==2 );
			PlaceSelectText ( "extrem",38+45*3,86+16,options.metal==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45&&mouse->x<38+20+45&&mouse->y>=86+16-4&&mouse->y<86+16-4+14 )
		{
			options.metal=1;
			PlaceSelectText ( "wenig",38,86+16,options.metal==0 );
			PlaceSelectText ( "mittel",38+45,86+16,options.metal==1 );
			PlaceSelectText ( "viel",38+45*2,86+16,options.metal==2 );
			PlaceSelectText ( "extrem",38+45*3,86+16,options.metal==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*2&&mouse->x<38+20+45*2&&mouse->y>=86+16-4&&mouse->y<86+16-4+14 )
		{
			options.metal=2;
			PlaceSelectText ( "wenig",38,86+16,options.metal==0 );
			PlaceSelectText ( "mittel",38+45,86+16,options.metal==1 );
			PlaceSelectText ( "viel",38+45*2,86+16,options.metal==2 );
			PlaceSelectText ( "extrem",38+45*3,86+16,options.metal==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*3&&mouse->x<38+20+45*3&&mouse->y>=86+16-4&&mouse->y<86+16-4+14 )
		{
			options.metal=3;
			PlaceSelectText ( "wenig",38,86+16,options.metal==0 );
			PlaceSelectText ( "mittel",38+45,86+16,options.metal==1 );
			PlaceSelectText ( "viel",38+45*2,86+16,options.metal==2 );
			PlaceSelectText ( "extrem",38+45*3,86+16,options.metal==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Klick aufs �l:
		if ( b&&!lb&&mouse->x>=38-20&&mouse->x<38+20&&mouse->y>=124+16-4&&mouse->y<124+16-4+14 )
		{
			options.oil=0;
			PlaceSelectText ( "wenig",38,124+16,options.oil==0 );
			PlaceSelectText ( "mittel",38+45,124+16,options.oil==1 );
			PlaceSelectText ( "viel",38+45*2,124+16,options.oil==2 );
			PlaceSelectText ( "extrem",38+45*3,124+16,options.oil==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45&&mouse->x<38+20+45&&mouse->y>=124+16-4&&mouse->y<124+16-4+14 )
		{
			options.oil=1;
			PlaceSelectText ( "wenig",38,124+16,options.oil==0 );
			PlaceSelectText ( "mittel",38+45,124+16,options.oil==1 );
			PlaceSelectText ( "viel",38+45*2,124+16,options.oil==2 );
			PlaceSelectText ( "extrem",38+45*3,124+16,options.oil==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*2&&mouse->x<38+20+45*2&&mouse->y>=124+16-4&&mouse->y<124+16-4+14 )
		{
			options.oil=2;
			PlaceSelectText ( "wenig",38,124+16,options.oil==0 );
			PlaceSelectText ( "mittel",38+45,124+16,options.oil==1 );
			PlaceSelectText ( "viel",38+45*2,124+16,options.oil==2 );
			PlaceSelectText ( "extrem",38+45*3,124+16,options.oil==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*3&&mouse->x<38+20+45*3&&mouse->y>=124+16-4&&mouse->y<124+16-4+14 )
		{
			options.oil=3;
			PlaceSelectText ( "wenig",38,124+16,options.oil==0 );
			PlaceSelectText ( "mittel",38+45,124+16,options.oil==1 );
			PlaceSelectText ( "viel",38+45*2,124+16,options.oil==2 );
			PlaceSelectText ( "extrem",38+45*3,124+16,options.oil==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Klick aufs Gold:
		if ( b&&!lb&&mouse->x>=38-20&&mouse->x<38+20&&mouse->y>=162+16-4&&mouse->y<162+16-4+14 )
		{
			options.gold=0;
			PlaceSelectText ( "wenig",38,162+16,options.gold==0 );
			PlaceSelectText ( "mittel",38+45,162+16,options.gold==1 );
			PlaceSelectText ( "viel",38+45*2,162+16,options.gold==2 );
			PlaceSelectText ( "extrem",38+45*3,162+16,options.gold==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45&&mouse->x<38+20+45&&mouse->y>=162+16-4&&mouse->y<162+16-4+14 )
		{
			options.gold=1;
			PlaceSelectText ( "wenig",38,162+16,options.gold==0 );
			PlaceSelectText ( "mittel",38+45,162+16,options.gold==1 );
			PlaceSelectText ( "viel",38+45*2,162+16,options.gold==2 );
			PlaceSelectText ( "extrem",38+45*3,162+16,options.gold==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*2&&mouse->x<38+20+45*2&&mouse->y>=162+16-4&&mouse->y<162+16-4+14 )
		{
			options.gold=2;
			PlaceSelectText ( "wenig",38,162+16,options.gold==0 );
			PlaceSelectText ( "mittel",38+45,162+16,options.gold==1 );
			PlaceSelectText ( "viel",38+45*2,162+16,options.gold==2 );
			PlaceSelectText ( "extrem",38+45*3,162+16,options.gold==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=38-20+45*3&&mouse->x<38+20+45*3&&mouse->y>=162+16-4&&mouse->y<162+16-4+14 )
		{
			options.gold=3;
			PlaceSelectText ( "wenig",38,162+16,options.gold==0 );
			PlaceSelectText ( "mittel",38+45,162+16,options.gold==1 );
			PlaceSelectText ( "viel",38+45*2,162+16,options.gold==2 );
			PlaceSelectText ( "extrem",38+45*3,162+16,options.gold==3 );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Klick auf die Credits:
		if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4&&mouse->y<86-4+20 )
		{
			options.credits=25;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20&&mouse->y<86-4+20+20 )
		{
			options.credits=50;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20*2&&mouse->y<86-4+20+20*2 )
		{
			options.credits=100;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20*3&&mouse->y<86-4+20+20*3 )
		{
			options.credits=150;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20*4&&mouse->y<86-4+20+20*4 )
		{
			options.credits=200;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20*5&&mouse->y<86-4+20+20*5 )
		{
			options.credits=250;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=86-4+20*6&&mouse->y<86-4+20+20*6 )
		{
			options.credits=300;
			PlaceSelectText ( "kaum (25)",110+130,86,options.credits==25,false );
			PlaceSelectText ( "sehr wenige (50)",110+130,86+20,options.credits==50,false );
			PlaceSelectText ( "wenige (100)",110+130,86+20*2,options.credits==100,false );
			PlaceSelectText ( "normal (150)",110+130,86+20*3,options.credits==150,false );
			PlaceSelectText ( "viele (200)",110+130,86+20*4,options.credits==200,false );
			PlaceSelectText ( "sehr viele (250)",110+130,86+20*5,options.credits==250,false );
			PlaceSelectText ( "extrem (300)",110+130,86+20*6,options.credits==300,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Br�ckenkopf:
		if ( b&&!lb&&mouse->x>=452&&mouse->x<452+100&&mouse->y>=86-4&&mouse->y<86-4+14 )
		{
			options.FixedBridgeHead=false;
			PlaceSelectText ( "Mobil",452,86,!options.FixedBridgeHead,false );
			PlaceSelectText ( "Fest",452,86+20,options.FixedBridgeHead,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=452&&mouse->x<452+100&&mouse->y>=86-4+20&&mouse->y<86-4+14+20 )
		{
			options.FixedBridgeHead=true;
			PlaceSelectText ( "Mobil",452,86,!options.FixedBridgeHead,false );
			PlaceSelectText ( "Fest",452,86+20,options.FixedBridgeHead,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// AlienTech:
		if ( b&&!lb&&mouse->x>=30&&mouse->x<38+100&&mouse->y>=281-4&&mouse->y<281-4+14 )
		{
			options.AlienTech=true;
			PlaceSelectText ( "ein",38,281,options.AlienTech );
			PlaceSelectText ( "aus",38,281+20,!options.AlienTech );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=30&&mouse->x<38+100&&mouse->y>=281-4+20&&mouse->y<281-4+14+20 )
		{
			options.AlienTech=false;
			PlaceSelectText ( "ein",38,281,options.AlienTech );
			PlaceSelectText ( "aus",38,281+20,!options.AlienTech );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Ressourcendichte:
		if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=281-4&&mouse->y<281-4+20 )
		{
			options.dichte=0;
			PlaceSelectText ( "d�nn",110+130,281,options.dichte==0,false );
			PlaceSelectText ( "normal",110+130,281+20,options.dichte==1,false );
			PlaceSelectText ( "dicht",110+130,281+20*2,options.dichte==2,false );
			PlaceSelectText ( "extrem",110+130,281+20*3,options.dichte==3,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=281+20-4&&mouse->y<281+20-4+20 )
		{
			options.dichte=1;
			PlaceSelectText ( "d�nn",110+130,281,options.dichte==0,false );
			PlaceSelectText ( "normal",110+130,281+20,options.dichte==1,false );
			PlaceSelectText ( "dicht",110+130,281+20*2,options.dichte==2,false );
			PlaceSelectText ( "extrem",110+130,281+20*3,options.dichte==3,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=281+20*2-4&&mouse->y<281+20*2-4+20 )
		{
			options.dichte=2;
			PlaceSelectText ( "d�nn",110+130,281,options.dichte==0,false );
			PlaceSelectText ( "normal",110+130,281+20,options.dichte==1,false );
			PlaceSelectText ( "dicht",110+130,281+20*2,options.dichte==2,false );
			PlaceSelectText ( "extrem",110+130,281+20*3,options.dichte==3,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=110+130&&mouse->x<110+130+100&&mouse->y>=281+20*3-4&&mouse->y<281+20*3-4+20 )
		{
			options.dichte=3;
			PlaceSelectText ( "d�nn",110+130,281,options.dichte==0,false );
			PlaceSelectText ( "normal",110+130,281+20,options.dichte==1,false );
			PlaceSelectText ( "dicht",110+130,281+20*2,options.dichte==2,false );
			PlaceSelectText ( "extrem",110+130,281+20*3,options.dichte==3,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Spielart:
		if ( b&&!lb&&mouse->x>=452&&mouse->x<452+100&&mouse->y>=281-4&&mouse->y<281-4+20 )
		{
			options.PlayRounds=false;
			PlaceSelectText ( "simultan",452,281,!options.PlayRounds,false );
			PlaceSelectText ( "Runden",452,281+20,options.PlayRounds,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&mouse->x>=452&&mouse->x<452+100&&mouse->y>=281+20-4&&mouse->y<281+20-4+20 )
		{
			options.PlayRounds=true;
			PlaceSelectText ( "simultan",452,281,!options.PlayRounds,false );
			PlaceSelectText ( "Runden",452,281+20,options.PlayRounds,false );
			PlayFX ( SoundData.SNDObjectMenu );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Zur�ck:
		if ( mouse->x>=50&&mouse->x<50+200&&mouse->y>=440&&mouse->y<440+29 )
		{
			if ( b&&!lb )
			{
				BackPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Zur�ck",50,440,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&BackPressed )
			{
				options.metal=-1;
				break;
			}
		}
		else if ( BackPressed )
		{
			BackPressed=false;
			PlaceButton ( "Zur�ck",50,440,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Ok:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=440&&mouse->y<440+29 )
		{
			if ( b&&!lb )
			{
				OKPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Ok",390,440,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&OKPressed )
			{
				break;
			}
		}
		else if ( OKPressed )
		{
			OKPressed=false;
			PlaceButton ( "Ok",390,440,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		lb=b;
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}

	TmpSf=NULL;
	return options;
}

// Startet die Planetenauswahl (gibt den Namen des Planeten zur�ck):
string RunPlanetSelect ( void )
{
	bool OKPressed=false;
	Uint8 *keystate;
	int b,lb=0,offset=0,selected=-1,i,lx=-1,ly=-1;
	TList *files;
	SDL_Rect scr;
	TiXmlDocument doc;
	TiXmlNode* rootnode;
	TiXmlNode* node;

	TmpSf=GraphicsData.gfx_shadow;
	SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );

	LoadPCXtoSF ( GFXOD_PLANET_SELECT,TmpSf );
	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Planetenauswahl",320,11,buffer );

	PlaceButton ( "Ok",390,440,false );

	files = new TList;
	int k = 1;

	if ( !doc.LoadFile ( "maps//maps.xml" ) )
	{
		cLog::write ( "Could not load maps.xml",1 );
		return "";
	}
	rootnode=doc.FirstChildElement ( "MapData" )->FirstChildElement ( "MapList" );

	files = new TList;
	node=rootnode->FirstChildElement();
	if ( node )
		files->Add ( node->ToElement()->Attribute ( "file" ) );
	while ( node )
	{
		node=node->NextSibling();
		if ( node && node->Type() ==1 )
			files->Add ( node->ToElement()->Attribute ( "file" ) );
	}

	ShowPlanets ( files,offset,selected );
	mouse->Show();
	mouse->SetCursor ( CHand );
	SHOW_SCREEN

#define TEAR_DOWN TmpSf=NULL;while(files->Count){/*delete (files->Items(0));*/files->DeleteString(0);}delete files;

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE] ) break;
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}
		// Ok:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=440&&mouse->y<440+29&&selected>=0 )
		{
			if ( b&&!lb )
			{
				OKPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Ok",390,440,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&OKPressed )
			{
				string name;
				name = files->Items[selected];
				name.replace ( name.length()-3,3,"map" );
				TEAR_DOWN
				return name;
			}
		}
		else if ( OKPressed )
		{
			OKPressed=false;
			PlaceButton ( "Ok",390,440,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Up:
		if ( mouse->x>=293&&mouse->x<293+24&&mouse->y>=440&&mouse->y<440+24&&b&&!lb&&offset>0 )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			offset-=4;
			ShowPlanets ( files,offset,selected );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Down:
		if ( mouse->x>=321&&mouse->x<321+24&&mouse->y>=440&&mouse->y<440+24&&b&&!lb&&files->Count-8-offset>0 )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			offset+=4;
			ShowPlanets ( files,offset,selected );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Klick auf eine Map:
		if ( b&&!lb )
		{
			scr.x=25;
			scr.y=90;
			for ( i=0;i<8;i++ )
			{
				if ( i+offset>=files->Count ) break;

				if ( mouse->x>=scr.x&&mouse->x<scr.x+112&&mouse->y>=scr.y&&mouse->y<scr.y+112 )
				{
					selected=i+offset;
					PlayFX ( SoundData.SNDObjectMenu );
					ShowPlanets ( files,offset,selected );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					break;
				}

				scr.x+=158;
				if ( i==3 )
				{
					scr.x=25;
					scr.y+=197;
				}
			}
		}

		lx=mouse->x;
		ly=mouse->y;
		lb=b;
		SDL_Delay ( 1 );
	}

	TEAR_DOWN
	return "";
}

// Zeigt die Planeten an:
void ShowPlanets ( TList *files,int offset,int selected )
{
	SDL_Surface *sf;
	SDL_Rect scr,dest;
	string name;
	string pathname;
	int i,size;
	FILE *fp;

	scr.w=640;
	scr.h=390;
	scr.x=0;
	scr.y=38;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );

	scr.x=25;
	scr.y=90;
	scr.h=scr.w=112;
	for ( i=0;i<8;i++ )
	{
		if ( i+offset>=files->Count ) break;
		name = files->Items[i+offset];
		pathname = name;
		pathname.insert ( 0,SettingsData.sMapsPath );
		sf = SDL_LoadBMP ( pathname.c_str() );
		if ( sf!=NULL )
		{
			SDL_BlitSurface ( sf,NULL,buffer,&scr );
		}

		pathname.replace ( pathname.length()-3, 3, "map" );
		fp=fopen ( pathname.c_str(),"rb" );
		fseek ( fp,21,SEEK_CUR );
		fread ( &size,sizeof ( int ),1,fp );
		fclose ( fp );

		if ( selected==i+offset )
		{
			SDL_Rect r;
			r=scr;
			r.x-=4;
			r.y-=4;
			r.w+=8;
			r.h=4;
			SDL_FillRect ( buffer,&r,0x00C000 );
			r.w=4;
			r.h=112+8;
			SDL_FillRect ( buffer,&r,0x00C000 );
			r.x+=112+4;
			SDL_FillRect ( buffer,&r,0x00C000 );
			r.x-=112+4;
			r.y+=112+4;
			r.w=112+8;
			r.h=4;
			SDL_FillRect ( buffer,&r,0x00C000 );

			char tmp[16];
			sprintf ( tmp,"%d",size );
			name.insert ( 0,"> " );
			name.replace ( name.length()-4, 2, " (" );
			name.replace ( name.length()-2, 3, tmp );
			name.insert ( name.length(),"x" );
			name.replace ( name.length(), 3, tmp );
			name.insert ( name.length(),") <" );
		}
		else
		{
			char tmp[16];
			sprintf ( tmp,"%d",size );
			name.replace ( name.length()-4, 2, " (" );
			name.replace ( name.length()-2, 3, tmp );
			name.insert ( name.length(),"x" );
			name.replace ( name.length(), 3, tmp );
			name.insert ( name.length(),")" );
		}
		fonts->OutTextCenter ( ( char * ) name.c_str(),scr.x+77-21,scr.y-42,buffer );

		scr.x+=158;
		if ( i==3 )
		{
			scr.x=25;
			scr.y+=197;
		}

		if ( sf!=NULL )
		{
			SDL_FreeSurface ( sf );
		}
	}

	// Die Up-Down Buttons machen:
	if ( offset )
	{
		scr.x=130;scr.y=452;
		dest.h=scr.h=dest.w=scr.w=25;
		dest.x=293;
		dest.y=440;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	else
	{
		dest.h=dest.w=25;
		dest.x=293;
		dest.y=440;
		SDL_BlitSurface ( TmpSf,&dest,buffer,&dest );
	}
	if ( files->Count-8-offset>0 )
	{
		scr.x=103;scr.y=452;
		dest.h=scr.h=dest.w=scr.w=25;
		dest.x=321;
		dest.y=440;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	else
	{
		dest.h=dest.w=25;
		dest.x=321;
		dest.y=440;
		SDL_BlitSurface ( TmpSf,&dest,buffer,&dest );
	}
}

// Startet die Spielerauswahl (gibt die Spielereinstellungen):
sPlayer RunPlayerSelect ( void )
{
	bool OKPressed=false;
	int b,lb=0,offset=0,lx=-1,ly=-1;
	sPlayer players;

	for ( int i = 0; i < 4; i++ )
	{
		players.clan[i] = "NONE";
		players.what[i] = 0;
	}
	players.what[0] = 1;
	players.what[1] = 2;

	SDL_BlitSurface ( GraphicsData.gfx_player_select,NULL,buffer,NULL );
	fonts->OutTextCenter ( "Spielerauswahl",320,11,buffer );
	fonts->OutTextCenter ( "Team",100,35,buffer );
	fonts->OutTextCenter ( "Mensch",200,35,buffer );
	fonts->OutTextCenter ( "Computer",310,35,buffer );
	fonts->OutTextCenter ( "Keiner",420,35,buffer );
	fonts->OutTextCenter ( "Clan",535,35,buffer );

	PlaceButton ( "Ok",390,440,false );
	ShowPlayerStates ( players );

	mouse->Show();
	mouse->SetCursor ( CHand );
	SHOW_SCREEN
	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}
		// �nderungen:
		if ( b&&!lb )
		{
			int x = 175;
			int y = 67;
			for ( int i = 0; i < 12; i++ )
			{
				if ( mouse->x>=x&&mouse->x<x+55&&mouse->y>=y&&mouse->y<y+71 )
				{
					PlayFX ( SoundData.SNDObjectMenu );
					if ( i == 0 ) players.what[0] = 1;
					if ( i == 1 ) players.what[0] = 2;
					if ( i == 2 ) players.what[0] = 0;

					if ( i == 3 ) players.what[1] = 1;
					if ( i == 4 ) players.what[1] = 2;
					if ( i == 5 ) players.what[1] = 0;

					if ( i == 6 ) players.what[2] = 1;
					if ( i == 7 ) players.what[2] = 2;
					if ( i == 8 ) players.what[2] = 0;

					if ( i == 9 ) players.what[3] = 1;
					if ( i == 10 ) players.what[3] = 2;
					if ( i == 11 ) players.what[3] = 0;
					ShowPlayerStates ( players );
					SHOW_SCREEN
					mouse->draw ( false,screen );
				}
				if ( x == 395 )
				{
					x = 175;
					y += 92;
				}
				else x += 110;
			}
		}
		// Ok:
		if ( mouse->x>=390&&mouse->x<390+200&&mouse->y>=440&&mouse->y<440+29 && ( players.what[0] == 1 || players.what[1] == 1 || players.what[2] == 1 || players.what[3] == 1 ) )
		{
			if ( b&&!lb )
			{
				OKPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Ok",390,440,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&OKPressed )
			{
				return players;
			}
		}
		else if ( OKPressed )
		{
			OKPressed=false;
			PlaceButton ( "Ok",390,440,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		lx=mouse->x;
		ly=mouse->y;
		lb=b;
		SDL_Delay ( 1 );
	}
	return players;
}

void ShowPlayerStates ( sPlayer players )
{
	SDL_Rect dest,norm1,norm2;
	dest.w = norm1.w = norm2.w = 55;
	dest.h = norm1.h = norm2.h = 71;
	dest.x = 394;
	dest.y = 67;
	for ( int i = 0; i<4;i++ )
	{
		dest.y = norm1.y = norm2.y = 67 + 92*i;
		// Nichts
		if ( players.what[i] == 0 )
		{
			dest.x = 394;
			SDL_BlitSurface ( GraphicsData.gfx_player_none,NULL,buffer,&dest );
			norm1.x = 394 - 110;
			norm2.x = 394 - 219;
		}
		// Spieler
		if ( players.what[i] == 1 )
		{
			norm1.x = 394 - 110;
			norm2.x = 394;
			dest.x = 394 - 219;
			SDL_BlitSurface ( GraphicsData.gfx_player_human,NULL,buffer,&dest );
		}
		// Computer
		if ( players.what[i] == 2 )
		{
			norm1.x = 394;
			norm2.x = 394 - 219;
			dest.x = 394 - 110;
			SDL_BlitSurface ( GraphicsData.gfx_player_pc,NULL,buffer,&dest );
		}
		SDL_BlitSurface ( GraphicsData.gfx_player_select,&norm1,buffer,&norm1 );
		SDL_BlitSurface ( GraphicsData.gfx_player_select,&norm2,buffer,&norm2 );
	}
}

// Zeigt den Hngar an:
void ShowBars ( int credits,int StartCredits,TList *landing,int selected );
void MakeUpgradeSliderVehicle ( sUpgrades *u,int nr,cPlayer *p );
void MakeUpgradeSliderBuilding ( sUpgrades *u,int nr,cPlayer *p );
int CalcSteigerung ( int org, int variety );
int CalcPrice ( int value,int org, int variety );
void MakeUpgradeSubButtons ( bool tank,bool plane,bool ship,bool build,bool tnt,bool kauf );
void ShowSelectionList ( TList *list,int selected,int offset,bool beschreibung,int credits,cPlayer *p );

void RunHangar ( cPlayer *player,TList *LandingList )
{
	bool tank=true,plane=false,ship=false,build=false,tnt=false,kauf=true;
	bool FertigPressed=false,Beschreibung=SettingsData.bShowDescription;
	bool DownPressed=false,UpPressed=false,KaufPressed=false;
	bool Down2Pressed=false,Up2Pressed=false,EntfernenPressed=false;
	bool LadungUpPressed=false,LadungDownPressed=false;
	int b,lb=0,i,selected=0,offset=0,x,y,StartCredits=player->Credits,lx=-1,ly=-1;
	int LandingOffset=0,LandingSelected=0;
	SDL_Rect scr,dest;
	TList *list,*selection;

	TmpSf=GraphicsData.gfx_shadow;
	SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );

	LoadPCXtoSF ( GFXOD_HANGAR,TmpSf );
	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );

	// Die Liste erstellen:
	list=new TList;
	for ( i=0;i<VehicleMainData.vehicle_anz;i++ )
	{
		sHUp *n;
		SDL_Surface *sf;
		ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].img_org[0],VehicleMainData.vehicle[i].img[0],VehicleMainData.vehicle[i].img_org[0]->w/2,VehicleMainData.vehicle[i].img_org[0]->h/2 );
		sf=SDL_CreateRGBSurface ( SDL_SRCCOLORKEY,VehicleMainData.vehicle[i].img[0]->w,VehicleMainData.vehicle[i].img[0]->h,32,0,0,0,0 );
		SDL_SetColorKey ( sf,SDL_SRCCOLORKEY,0xFF00FF );
		SDL_BlitSurface ( OtherData.colors[cl_grey],NULL,sf,NULL );
		SDL_BlitSurface ( VehicleMainData.vehicle[i].img[0],NULL,sf,NULL );
		ScaleSurfaceAdv2 ( VehicleMainData.vehicle[i].img_org[0],VehicleMainData.vehicle[i].img[0],VehicleMainData.vehicle[i].img_org[0]->w,VehicleMainData.vehicle[i].img_org[0]->h );
		n=new sHUp;
		n->sf=sf;
		n->id=i;
		n->costs=VehicleMainData.vehicle[i].data.costs;
		n->vehicle=true;
		MakeUpgradeSliderVehicle ( n->upgrades,i,player );
		list->AddHUp ( n );
	}
	for ( i=0;i<BuildingMainData.building_anz;i++ )
	{
		sHUp *n;
		SDL_Surface *sf;
		if ( BuildingMainData.building[i].data.is_big )
		{
			ScaleSurfaceAdv2 ( BuildingMainData.building[i].img_org,BuildingMainData.building[i].img,BuildingMainData.building[i].img_org->w/4,BuildingMainData.building[i].img_org->h/4 );
		}
		else
		{
			ScaleSurfaceAdv2 ( BuildingMainData.building[i].img_org,BuildingMainData.building[i].img,BuildingMainData.building[i].img_org->w/2,BuildingMainData.building[i].img_org->h/2 );
		}
		sf=SDL_CreateRGBSurface ( SDL_SRCCOLORKEY,BuildingMainData.building[i].img->w,BuildingMainData.building[i].img->h,32,0,0,0,0 );
		SDL_SetColorKey ( sf,SDL_SRCCOLORKEY,0xFF00FF );
		if ( !BuildingMainData.building[i].data.is_connector&&!BuildingMainData.building[i].data.is_road )
		{
			SDL_BlitSurface ( OtherData.colors[cl_grey],NULL,sf,NULL );
		}
		else
		{
			SDL_FillRect ( sf,NULL,0xFF00FF );
		}
		SDL_BlitSurface ( BuildingMainData.building[i].img,NULL,sf,NULL );
		ScaleSurfaceAdv2 ( BuildingMainData.building[i].img_org,BuildingMainData.building[i].img,BuildingMainData.building[i].img_org->w,BuildingMainData.building[i].img_org->h );
		n=new sHUp;
		n->sf=sf;
		n->id=i;
		n->costs=BuildingMainData.building[i].data.costs;
		n->vehicle=false;
		MakeUpgradeSliderBuilding ( n->upgrades,i,player );
		list->AddHUp ( n );
	}
	// Die Selection erstellen:
	selection=new TList;
	CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );
	ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
	MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );

	// Die Landeliste anzeigen:
	ShowLandingList ( LandingList,LandingSelected,LandingOffset );

	// Die Bars anzeigen:
	ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );

	SHOW_SCREEN
	mouse->draw ( false,screen );

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();
		x=mouse->x;y=mouse->y;

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}

		// Fertig:
		if ( mouse->x>=447&&mouse->x<447+55&&mouse->y>=452&&mouse->y<452+24 )
		{
			if ( b&&!lb )
			{
				FertigPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				scr.x=308;scr.y=231;
				scr.w=dest.w=55;
				scr.h=dest.h=24;
				dest.x=447;dest.y=452;
				SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&FertigPressed )
			{
				break;
			}
		}
		else if ( FertigPressed )
		{
			FertigPressed=false;
			scr.x=447;scr.y=452;
			scr.w=55;scr.h=24;
			SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Beschreibung Haken:
		if ( x>=292&&x<292+16&&y>=265&&y<265+15&&b&&!lb )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			Beschreibung=!Beschreibung;
			SettingsData.bShowDescription=Beschreibung;
			if ( Beschreibung )
			{
				dest.x=scr.x=291;
				dest.y=scr.y=264;
				dest.w=scr.w=17;
				dest.h=scr.h=17;
				SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&dest );
			}
			else
			{
				scr.x=393;
				scr.y=46;
				dest.x=291;
				dest.y=264;
				dest.w=scr.w=18;
				dest.h=scr.h=17;
				SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			}
			ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Down-Button:
		if ( x>=491&&x<491+18&&y>=386&&y<386+17&&b&&!DownPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x=249;
			scr.y=151;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=491;
			dest.y=386;
			if ( offset<selection->Count-9 )
			{
				offset++;
				if ( selected<offset ) selected=offset;
				ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			DownPressed=true;
		}
		else if ( !b&&DownPressed )
		{
			scr.x=491;
			scr.y=386;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=491;
			dest.y=386;
			SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			DownPressed=false;
		}
		// Up-Button:
		if ( x>=470&&x<470+18&&y>=386&&y<386+17&&b&&!UpPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x=230;
			scr.y=151;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=470;
			dest.y=386;
			if ( offset!=0 )
			{
				offset--;
				if ( selected>=offset+9 ) selected=offset+8;
				ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			UpPressed=true;
		}
		else if ( !b&&UpPressed )
		{
			scr.x=470;
			scr.y=386;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=470;
			dest.y=386;
			SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			UpPressed=false;
		}
		// Klick in die Liste:
		if ( x>=490&&x<490+70&&y>=60&&y<60+315&&b&&!lb )
		{
			int nr;
			nr= ( y-60 ) / ( 32+2 );
			if ( selection->Count<9 )
			{
				if ( nr>=selection->Count ) nr=-1;
			}
			else
			{
				if ( nr>=10 ) nr=-1;
				nr+=offset;
			}
			if ( nr!=-1 )
			{
				int last_selected=selected;
				PlayFX ( SoundData.SNDObjectMenu );
				selected=nr;
				ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
				// Doppelklick pr�fen:
				if ( last_selected==nr&&selection->HUpItems[selected]->costs<=player->Credits )
				{
					sLanding *n;
					n=new sLanding;
					n->cargo=0;
					n->sf=selection->HUpItems[selected]->sf;
					n->id=selection->HUpItems[selected]->id;
					n->costs=selection->HUpItems[selected]->costs;
					LandingList->AddLanding ( n );
					LandingSelected=LandingList->Count-1;
					while ( LandingSelected>=LandingOffset+5 ) LandingOffset++;

					if ( LandingSelected<0 ) LandingSelected=0;
					ShowLandingList ( LandingList,LandingSelected,LandingOffset );
					player->Credits-=selection->HUpItems[selected]->costs;
					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
				}
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		// Klick auf einen Upgrade-Slider:
		if ( b&&!lb&&x>=283&&x<301+18&&selection->Count )
		{
			sHUp *ptr=selection->HUpItems[selected];
			for ( i=0;i<8;i++ )
			{
				if ( !ptr->upgrades[i].active ) continue;
				if ( ptr->upgrades[i].Purchased&&x<283+18&&y>=293+i*19&&y<293+i*19+19 )
				{
					int variety;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "hitpoints" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "armor" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "ammo" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "damage" ) == 0 )
						variety = 0;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "speed" ) == 0 )
						variety = 1;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "shots" ) == 0 )
						variety = 2;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "range" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "scan" ) == 0 )
						variety = 3;
					* ( ptr->upgrades[i].value )-=CalcSteigerung ( ptr->upgrades[i].StartValue,variety );
					ptr->upgrades[i].NextPrice=CalcPrice ( * ( ptr->upgrades[i].value ),ptr->upgrades[i].StartValue,variety );
					player->Credits+=ptr->upgrades[i].NextPrice;
					ptr->upgrades[i].Purchased--;

					PlayFX ( SoundData.SNDObjectMenu );
					ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					break;
				}
				else if ( ptr->upgrades[i].NextPrice<=player->Credits&&x>=301&&y>=293+i*19&&y<293+i*19+19 )
				{
					int variety;
					player->Credits-=ptr->upgrades[i].NextPrice;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "hitpoints" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "armor" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "ammo" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "damage" ) == 0 )
						variety = 0;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "speed" ) == 0 )
						variety = 1;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "shots" ) == 0 )
						variety = 2;
					if ( strcmp ( ptr->upgrades[i].name.c_str(), "range" ) == 0 || strcmp ( ptr->upgrades[i].name.c_str(), "scan" ) == 0 )
						variety = 3;
					* ( ptr->upgrades[i].value ) +=CalcSteigerung ( ptr->upgrades[i].StartValue,variety );
					ptr->upgrades[i].NextPrice=CalcPrice ( * ( ptr->upgrades[i].value ),ptr->upgrades[i].StartValue,variety );
					ptr->upgrades[i].Purchased++;

					PlayFX ( SoundData.SNDObjectMenu );
					ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					break;
				}
			}
		}

		// Klick auf einen der SubSelctionButtons:
		if ( b&&!lb&&x>=467&&x<467+32&&y>=411&&y<411+31 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			tank=!tank;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );
			ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=467+33&&x<467+32+33&&y>=411&&y<411+31 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			plane=!plane;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=467+33*2&&x<467+32+33*2&&y>=411&&y<411+31 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			ship=!ship;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=467+33*3&&x<467+32+33*3&&y>=411&&y<411+31 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			build=!build;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=467+33*4&&x<467+32+33*4&&y>=411&&y<411+31 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			tnt=!tnt;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=542&&x<542+16&&y>=459 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			kauf=false;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else if ( b&&!lb&&x>=542&&x<542+16&&y>=445 )
		{
			PlayFX ( SoundData.SNDHudSwitch );
			kauf=true;
			CreateSelectionList ( selection,list,&selected,&offset,tank,plane,ship,build,tnt,kauf );      ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );
			MakeUpgradeSubButtons ( tank,plane,ship,build,tnt,kauf );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Kauf-Button:
		if ( x>=590&&x<590+41&&y>=386&&y<386+23&&b&&!KaufPressed&&selection->HUpItems[selected]->costs<=player->Credits&&kauf )
		{
			PlayFX ( SoundData.SNDMenuButton );
			scr.x=380;
			scr.y=274;
			dest.w=scr.w=41;
			dest.h=scr.h=23;
			dest.x=590;
			dest.y=386;

			sLanding *n;
			n=new sLanding;
			n->cargo=0;
			n->sf=selection->HUpItems[selected]->sf;
			n->id=selection->HUpItems[selected]->id;
			n->costs=selection->HUpItems[selected]->costs;
			LandingList->AddLanding ( n );
			LandingSelected=LandingList->Count-1;
			while ( LandingSelected>=LandingOffset+5 ) LandingOffset++;

			ShowLandingList ( LandingList,LandingSelected,LandingOffset );
			player->Credits-=n->costs;
			ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
			ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			KaufPressed=true;
		}
		else if ( !b&&KaufPressed )
		{
			scr.x=590;
			scr.y=386;
			dest.w=scr.w=41;
			dest.h=scr.h=23;
			dest.x=590;
			dest.y=386;
			SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			KaufPressed=false;
		}
		// Down2-Button:
		if ( x>=327&&x<327+18&&y>=240&&y<240+17&&b&&!Down2Pressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x=230;
			scr.y=151;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=327;
			dest.y=240;
			if ( LandingOffset!=0 )
			{
				LandingOffset--;
				if ( LandingSelected>=LandingOffset+5 ) LandingSelected=LandingOffset+4;
				ShowLandingList ( LandingList,LandingSelected,LandingOffset );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			Down2Pressed=true;
		}
		else if ( !b&&Down2Pressed )
		{
			scr.x=327;
			scr.y=240;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=327;
			dest.y=240;
			SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			Down2Pressed=false;
		}
		// Up2-Button:
		if ( x>=347&&x<347+18&&y>=240&&y<240+17&&b&&!Up2Pressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			scr.x=249;
			scr.y=151;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=347;
			dest.y=240;
			if ( LandingOffset<LandingList->Count-5 )
			{
				LandingOffset++;
				if ( LandingSelected<LandingOffset ) LandingSelected=LandingOffset;
				ShowLandingList ( LandingList,LandingSelected,LandingOffset );
			}
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			Up2Pressed=true;
		}
		else if ( !b&&Up2Pressed )
		{
			scr.x=347;
			scr.y=240;
			dest.w=scr.w=18;
			dest.h=scr.h=17;
			dest.x=347;
			dest.y=240;
			SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			Up2Pressed=false;
		}
		// Entfernen-Button:
		if ( x>=412&&x<412+53&&y>=240&&y<240+23&&b&&!EntfernenPressed )
		{
			PlayFX ( SoundData.SNDMenuButton );
			scr.x=0;
			scr.y=352;
			dest.w=scr.w=53;
			dest.h=scr.h=23;
			dest.x=412;
			dest.y=240;
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			EntfernenPressed=true;
		}
		else if ( EntfernenPressed&&!b&&lb )
		{
			// Vehicle aus der Liste entfernen:
			if ( LandingList->Count&&LandingList->Count>LandingSelected&&LandingSelected>=0 )
			{
				player->Credits+=LandingList->LandItems[LandingSelected]->costs;
				player->Credits+=LandingList->LandItems[LandingSelected]->cargo/5;

				delete LandingList->LandItems[LandingSelected];
				LandingList->DeleteLanding ( LandingSelected );
				ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
				ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );

				if ( LandingSelected>=LandingList->Count )
				{
					LandingSelected--;
				}
				if ( LandingList->Count-LandingOffset<5&&LandingOffset>0 )
				{
					LandingOffset--;
				}
				if ( LandingSelected<0 ) LandingSelected=0;
				ShowLandingList ( LandingList,LandingSelected,LandingOffset );
			}

			scr.x=412;
			scr.y=240;
			dest.w=scr.w=53;
			dest.h=scr.h=23;
			dest.x=412;
			dest.y=240;
			SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			EntfernenPressed=false;
		}
		// Klick in die LandingListe:
		if ( x>=330&&x<330+128&&y>=22&&y<22+210&&b&&!lb )
		{
			int nr;
			nr= ( y-22 ) / ( 32+10 );
			if ( LandingList->Count<5 )
			{
				if ( nr>=LandingList->Count ) nr=-1;
			}
			else
			{
				if ( nr>=10 ) nr=-1;
				nr+=LandingOffset;
			}
			if ( nr!=-1 )
			{
				int last_selected=LandingSelected;
				PlayFX ( SoundData.SNDObjectMenu );
				LandingSelected=nr;
				ShowLandingList ( LandingList,LandingSelected,LandingOffset );
				ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
				// Doppelklick pr�fen:
				if ( last_selected==nr )
				{
					if ( LandingList->Count&&LandingList->Count>LandingSelected&&LandingSelected>=0 )
					{
						player->Credits+=LandingList->LandItems[LandingSelected]->costs;
						player->Credits+=LandingList->LandItems[LandingSelected]->cargo/5;
						delete LandingList->LandItems[LandingSelected];
						LandingList->DeleteLanding ( LandingSelected );
						ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
						ShowSelectionList ( selection,selected,offset,Beschreibung,player->Credits,player );

						if ( LandingSelected>=LandingList->Count )
						{
							LandingSelected--;
						}
						if ( LandingList->Count-LandingOffset<5&&LandingOffset>0 )
						{
							LandingOffset--;
						}
						if ( LandingSelected<0 ) LandingSelected=0;
						ShowLandingList ( LandingList,LandingSelected,LandingOffset );
					}
				}
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}

		if ( LandingSelected>=0&&LandingList->Count&&LandingSelected<LandingList->Count )
		{
			sLanding *ptr;
			ptr=LandingList->LandItems[LandingSelected];
			if ( VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_METAL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_OIL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_GOLD )
			{

				// LadungUp-Button:
				if ( x>=413&&x<413+18&&y>=424&&y<424+17&&b&&!LadungDownPressed&&ptr->cargo<VehicleMainData.vehicle[ptr->id].data.max_cargo&&player->Credits>0 )
				{
					PlayFX ( SoundData.SNDObjectMenu );
					scr.x=249;
					scr.y=151;
					dest.w=scr.w=18;
					dest.h=scr.h=17;
					dest.x=413;
					dest.y=424;

					ptr->cargo+=5;
					player->Credits--;
					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					ShowLandingList ( LandingList,LandingSelected,LandingOffset );

					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					LadungDownPressed=true;
				}
				else if ( !b&&LadungDownPressed )
				{
					scr.x=413;
					scr.y=424;
					dest.w=scr.w=18;
					dest.h=scr.h=17;
					dest.x=413;
					dest.y=424;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					LadungDownPressed=false;
				}
				// LadungDown-Button:
				if ( x>=433&&x<433+18&&y>=424&&y<424+17&&b&&!LadungUpPressed&&ptr->cargo>0 )
				{
					PlayFX ( SoundData.SNDObjectMenu );
					scr.x=230;
					scr.y=151;
					dest.w=scr.w=18;
					dest.h=scr.h=17;
					dest.x=433;
					dest.y=424;

					ptr->cargo-=5;
					player->Credits++;
					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					ShowLandingList ( LandingList,LandingSelected,LandingOffset );

					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					LadungUpPressed=true;
				}
				else if ( !b&&LadungUpPressed )
				{
					scr.x=433;
					scr.y=424;
					dest.w=scr.w=18;
					dest.h=scr.h=17;
					dest.x=433;
					dest.y=424;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					LadungUpPressed=false;
				}
				// Klick auf den Ladungsbalken:
				if ( b&&!lb&&x>=422&&x<422+20&&y>=301&&y<301+115 )
				{
					int value;
					value= ( ( ( int ) ( ( 115- ( y-301 ) ) * ( VehicleMainData.vehicle[ptr->id].data.max_cargo/115.0 ) ) ) /5 ) *5;
					PlayFX ( SoundData.SNDObjectMenu );

					if ( ( 115- ( y-301 ) ) >=110 ) value=VehicleMainData.vehicle[ptr->id].data.max_cargo;

					if ( value<ptr->cargo )
					{
						player->Credits+= ( ptr->cargo-value ) /5;
						ptr->cargo=value;
					}
					else if ( value>ptr->cargo&&player->Credits>0 )
					{
						value-=ptr->cargo;
						while ( value>0&&player->Credits>0&&ptr->cargo<VehicleMainData.vehicle[ptr->id].data.max_cargo )
						{
							ptr->cargo+=5;
							player->Credits--;
							value-=5;
						}
						if ( ptr->cargo>VehicleMainData.vehicle[ptr->id].data.max_cargo )
						{
							ptr->cargo=VehicleMainData.vehicle[ptr->id].data.max_cargo;
						}
					}

					ShowBars ( player->Credits,StartCredits,LandingList,LandingSelected );
					ShowLandingList ( LandingList,LandingSelected,LandingOffset );
					SHOW_SCREEN
					mouse->draw ( false,screen );
				}
			}
		}

		lb=b;
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}

	TmpSf=NULL;
	while ( list->Count )
	{
		sHUp *ptr;
		ptr = list->HUpItems[0];
		for ( i=0;i<8;i++ )
		{
			if ( ptr->upgrades[i].active&&ptr->upgrades[i].Purchased )
			{
				if ( ptr->vehicle )
				{
					player->VehicleData[ptr->id].version++;
				}
				else
				{
					player->BuildingData[ptr->id].version++;
				}
				break;
			}
		}
		SDL_FreeSurface ( ptr->sf );
		delete ptr;
		list->DeleteHUp ( 0 );
	}
	delete list;
}

// Macht die Upgradeschieber f�r Vehicle:
void MakeUpgradeSliderVehicle ( sUpgrades *u,int nr,cPlayer *p )
{
	sVehicleData *d;
	int i;
	for ( i=0;i<8;i++ )
	{
		u[i].active=false;
		u[i].Purchased=0;
		u[i].value=NULL;
	}
	d=p->VehicleData+nr;
	i=0;

	if ( d->can_attack )
	{
		// Damage:
		u[i].active=true;
		u[i].value=& ( d->damage );
		u[i].NextPrice=2*CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.damage, 0 );
		u[i].name = "damage";
		i++;
		// Shots:
		u[i].active=true;
		u[i].value=& ( d->max_shots );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.max_shots, 2 );
		u[i].name = "shots";
		i++;
		// Range:
		u[i].active=true;
		u[i].value=& ( d->range );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.range, 3 );
		u[i].name = "range";
		i++;
		// Ammo:
		u[i].active=true;
		u[i].value=& ( d->max_ammo );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.max_ammo, 0 );
		u[i].name = "ammo";
		i++;
	}
	if ( d->can_transport==TRANS_METAL||d->can_transport==TRANS_OIL||d->can_transport==TRANS_GOLD )
	{
		i++;
	}
	// Armor:
	u[i].active=true;
	u[i].value=& ( d->armor );
	u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.armor, 0 );
	u[i].name = "armor";
	i++;
	// Hitpoints:
	u[i].active=true;
	u[i].value=& ( d->max_hit_points );
	u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.max_hit_points, 0 );
	u[i].name = "hitpoints";
	i++;
	// Scan:
	u[i].active=true;
	u[i].value=& ( d->scan );
	u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.scan, 3 );
	u[i].name = "scan";
	i++;
	// Speed:
	u[i].active=true;
	u[i].value=& ( d->max_speed );
	u[i].NextPrice=CalcPrice ( * ( u[i].value ),VehicleMainData.vehicle[nr].data.max_speed, 1 );
	u[i].name = "speed";
	i++;
	// Costs:
	i++;

	for ( i=0;i<8;i++ )
	{
		if ( u[i].value==NULL ) continue;
		u[i].StartValue=* ( u[i].value );
	}
}

// Macht die Upgradeschieber f�r Buildings:
void MakeUpgradeSliderBuilding ( sUpgrades *u,int nr,cPlayer *p )
{
	sBuildingData *d;
	int i;
	for ( i=0;i<8;i++ )
	{
		u[i].active=false;
		u[i].Purchased=0;
		u[i].value=NULL;
	}
	d=p->BuildingData+nr;
	i=0;

	if ( d->can_attack )
	{
		// Damage:
		u[i].active=true;
		u[i].value=& ( d->damage );
		u[i].NextPrice=2*CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.damage, 0 );
		u[i].name = "damage";
		i++;
		if ( !d->is_expl_mine )
		{
			// Shots:
			u[i].active=true;
			u[i].value=& ( d->max_shots );
			u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.max_shots, 2 );
			u[i].name = "shots";
			i++;
			// Range:
			u[i].active=true;
			u[i].value=& ( d->range );
			u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.range, 3 );
			u[i].name = "range";
			i++;
			// Ammo:
			u[i].active=true;
			u[i].value=& ( d->max_ammo );
			u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.max_ammo, 0 );
			u[i].name = "ammo";
			i++;
		}
	}
	if ( d->max_shield )
	{
		// Range:
		u[i].active=true;
		u[i].value=& ( d->range );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.range, 3 );
		u[i].name = "range";
		i++;
	}
	if ( d->can_load==TRANS_METAL||d->can_load==TRANS_OIL||d->can_load==TRANS_GOLD )
	{
		i++;
	}
	if ( d->energy_prod )
	{
		i+=2;
	}
	if ( d->human_prod )
	{
		i++;
	}
	// Armor:
	if ( d->armor!=1 )
	{
		u[i].active=true;
		u[i].value=& ( d->armor );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.armor, 0 );
		u[i].name = "armor";
	}
	i++;
	// Hitpoints:
	u[i].active=true;
	u[i].value=& ( d->max_hit_points );
	u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.max_hit_points, 0 );
	u[i].name = "hitpoints";
	i++;
	// Scan:
	if ( d->scan && d->scan!=1 )
	{
		u[i].active=true;
		u[i].value=& ( d->scan );
		u[i].NextPrice=CalcPrice ( * ( u[i].value ),BuildingMainData.building[nr].data.scan, 3 );
		u[i].name = "scan";
		i++;
	}
	// Energieverbrauch:
	if ( d->energy_need )
	{
		i++;
	}
	// Humanverbrauch:
	if ( d->human_need )
	{
		i++;
	}
	// Metallverbrauch:
	if ( d->metal_need )
	{
		i++;
	}
	// Goldverbrauch:
	if ( d->gold_need )
	{
		i++;
	}
	// Costs:
	i++;

	for ( i=0;i<8;i++ )
	{
		if ( u[i].value==NULL ) continue;
		u[i].StartValue=* ( u[i].value );
	}
}

// Berechnet den Preis f�r ein Upgrade:
int CalcPrice ( int value,int org, int variety )
{
	int tmp;
	double a, b, c;
	switch ( variety )
	{
			// Treffer, Panzerung, Munition & Angriff
		case 0:
			switch ( org )
			{
				case 2:
					if ( value==2 ) return 39;
					if ( value==3 ) return 321;
					break;
				case 4:
					a=0.0016091639;
					b=-0.073815318;
					c=6.0672869;
					break;
				case 6:
					a=0.000034548596;
					b=-0.27217472;
					c=6.3695123;
					break;
				case 8:
					a=0.00037219059;
					b=2.5148748;
					c=5.0938608;
					break;
				case 9:
					a=0.000059941694;
					b=1.3962889;
					c=4.6045196;
					break;
				case 10:
					a=0.000033736018;
					b=1.4674423;
					c=5.5606209;
					break;
				case 12:
					a=0.0000011574058;
					b=0.23439586;
					c=6.113616;
					break;
				case 14:
					a=0.0000012483447;
					b=1.4562373;
					c=5.8250952;
					break;
				case 15:
					a=0.00000018548742;
					b=-0.33519669;
					c=6.3333527;
					break;
				case 16:
					a=0.000010898263;
					b=5.0297434;
					c=5.0938627;
					break;
				case 18:
					a=0.00000017182818;
					b=2.0009536;
					c=5.8937153;
					break;
				case 20:
					a=0.00000004065782;
					b=1.6533066;
					c=6.0601538;
					break;
				case 22:
					a=0.0000000076942857;
					b=-0.45461813;
					c=6.4148588;
					break;
				case 24:
					a=0.00000076484313;
					b=8.0505377;
					c=5.1465019;
					break;
				case 28:
					a=0.00000015199858;
					b=5.1528048;
					c=5.4700225;
					break;
				case 32:
					a=0.00000030797077;
					b=8.8830596;
					c=5.1409486;
					break;
				case 56:
					a=0.000000004477053;
					b=11.454622;
					c=5.4335099;
					break;
				default:
					return -1;
			}
			break;
			// Geschwindgigkeit
		case 1:
			org=org/2;
			value=value/2;
			switch ( org )
			{
				case 5:
					a=0.00040716128;
					b=-0.16662054;
					c=6.2234362;
					break;
				case 6:
					a=0.00038548127;
					b=0.48236948;
					c=5.827724;
					break;
				case 7:
					a=0.000019798772;
					b=-0.31204765;
					c=6.3982628;
					break;
				case 9:
					a=0.0000030681294;
					b=-0.25372812;
					c=6.3995668;
					break;
				case 10:
					a=0.0000062019158;
					b=-0.23774407;
					c=6.1901333;
					break;
				case 12:
					a=0.0000064901101;
					b=0.93320705;
					c=5.8395847;
					break;
				case 14:
					a=0.0000062601892;
					b=2.1588132;
					c=5.5866699;
					break;
				case 15:
					a=0.00000027748628;
					b=-0.0031671959;
					c=6.2349744;
					break;
				case 16:
					a=0.0000011401659;
					b=1.8660343;
					c=5.7884287;
					break;
				case 18:
					a=0.00000093928003;
					b=2.9224069;
					c=5.6503159;
					break;
				case 20:
					a=0.00000003478867;
					b=0.44735558;
					c=6.2388156;
					break;
				case 24:
					a=0.0000000038623391;
					b=-0.4486039;
					c=6.4245686;
					break;
				case 28:
					a=0.000000039660207;
					b=1.6425505;
					c=5.8842817;
					break;
				default:
					return -2;
			}
			break;
			// Sch�sse
		case 2:
			switch ( org )
			{
				case 1:
					return 720;
					break;
				case 2:
					if ( value==2 ) return 79;
					if ( value==3 ) return 641;
					break;
				default:
					return -3;
			}
			break;
			// Reichweite, Scan
		case 3:
			switch ( org )
			{
				case 3:
					if ( value==3 ) return 61;
					if ( value==4 ) return 299;
					break;
				case 4:
					a=0.010226741;
					b=-0.001141961;
					c=5.8477272;
					break;
				case 5:
					a=0.00074684696;
					b=-0.24064936;
					c=6.2377712;
					break;
				case 6:
					a=0.0000004205569;
					b=-2.5074874;
					c=8.1868728;
					break;
				case 7:
					a=0.00018753949;
					b=0.42735532;
					c=5.9259322;
					break;
				case 8:
					a=0.000026278484;
					b=0.0026600724;
					c=6.2281618;
					break;
				case 9:
					a=0.000017724816;
					b=0.35087138;
					c=6.1028354;
					break;
				case 10:
					a=0.000011074461;
					b=-0.41358078;
					c=6.2067919;
					break;
				case 11:
					a=0.0000022011968;
					b=-0.97456761;
					c=6.4502985;
					break;
				case 12:
					a=0.0000000034515189;
					b=-4.4597674;
					c=7.9715326;
					break;
				case 14:
					a=0.0000028257552;
					b=0.78730358;
					c=5.9483863;
					break;
				case 18:
					a=0.00000024289322;
					b=0.64536566;
					c=6.11706;
					break;
				default:
					return -4;
			}
			break;
	}

	tmp= ( int ) ( Round ( ( a*pow ( ( value-b ),c ) ), 0 ) );
	return tmp;
}

// Berechnet die Steigerung bei eim Upgrade:
int CalcSteigerung ( int org, int variety )
{
	int tmp = 0;
	switch ( variety )
	{
		case 0:
		{
			if ( org == 2 || org == 4 || org == 6 || org == 8 )
				tmp = 1;
			if ( org == 9 || org == 10 || org == 12 || org == 14 || org == 15 || org == 16 || org == 18 || org == 20 || org == 22 || org == 24 )
				tmp = 2;
			if ( org == 28 || org == 32 )
				tmp = 5;
			if ( org == 56 )
				tmp = 10;
			break;
		}
		case 1:
		{
			org=org/2;
			if ( org == 5 || org == 6 || org == 7 || org == 9 )
				tmp = 1;
			if ( org == 10 || org == 12 || org ==14 || org == 15 || org == 16 || org == 18 || org == 20 )
				tmp = 2;
			if ( org == 28 )
				tmp = 5;
			tmp=tmp*2;
			break;
		}
		case 2:
			tmp = 1;
			break;
		case 3:
		{
			if ( org == 3 || org ==4 || org == 5 || org == 6 || org == 7 || org == 8 || org == 9 )
				tmp = 1;
			if ( org == 10 || org ==11 || org == 12 || org == 14 || org == 18 )
				tmp = 2;
			break;
		}
	}
	return tmp;
}

// Malt die SubButtons im Upgradefenster:
void MakeUpgradeSubButtons ( bool tank,bool plane,bool ship,bool build,bool tnt,bool kauf )
{
	SDL_Rect scr,dest;
	scr.x=152;scr.y=479;
	dest.x=467;dest.y=411;
	dest.w=scr.w=32;dest.h=scr.h=31;
	// Tank:
	if ( !tank )
	{
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&dest,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	dest.x+=33;
	scr.x+=33;
	// Plane:
	if ( !plane )
	{
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&dest,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	dest.x+=33;
	scr.x+=33;
	// Ship:
	if ( !ship )
	{
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&dest,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	dest.x+=33;
	scr.x+=33;
	// Building:
	if ( !build )
	{
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&dest,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	dest.x+=33;
	scr.x+=33;
	// TNT:
	if ( !tnt )
	{
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&dest,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	// Kauf:
	scr.x=54;scr.y=352;
	scr.w=scr.h=dest.w=dest.h=16;
	dest.x=542;
	dest.y=446;
	if ( !kauf )
	{
		SDL_BlitSurface ( TmpSf,&dest,buffer,&dest );
		dest.y=462;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
	}
	else
	{
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
		dest.y=462;
		SDL_BlitSurface ( TmpSf,&dest,buffer,&dest );
	}
}

// Zeigt die Bars an:
void ShowBars ( int credits,int StartCredits,TList *landing,int selected )
{
	char str[50];
	SDL_Rect scr,dest;
	scr.x=dest.x=371;
	scr.y=dest.y=301;
	scr.w=dest.w=22;
	scr.h=dest.h=115;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
	scr.x=dest.x=312;
	scr.y=dest.y=265;
	scr.w=dest.w=150;
	scr.h=dest.h=30;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );
	sprintf ( str,"%d",credits );
	fonts->OutTextCenter ( "Credits",381,275,buffer );
	fonts->OutTextCenter ( str,381,275+10,buffer );

	scr.x=118;
	scr.y=336;
	scr.w=dest.w=16;
	scr.h=dest.h=115* ( int ) ( ( credits/ ( float ) StartCredits ) );
	dest.x=375;
	dest.y=301+115-dest.h;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );

	scr.x=dest.x=422;
	scr.y=dest.y=301;
	scr.w=dest.w=20;
	scr.h=dest.h=115;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&dest );

	if ( selected>=0&&landing->Count&&selected<landing->Count )
	{
		sLanding *ptr;
		ptr=landing->LandItems[selected];
		if ( VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_METAL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_OIL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_GOLD )
		{
			sprintf ( str,"%d",ptr->cargo );
			fonts->OutTextCenter ( "Ladung",430,275,buffer );
			fonts->OutTextCenter ( str,430,275+10,buffer );

			scr.x=133;
			scr.y=336;
			scr.w=dest.w=20;
			scr.h=dest.h=115* ( int ) ( ( ptr->cargo/ ( float ) VehicleMainData.vehicle[ptr->id].data.max_cargo ) );
			dest.x=422;
			dest.y=301+115-dest.h;
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
		}
	}
}

// Liefert die Kachel, die auf der gro�en Karte unter den Koordinaten liegt:
int GetKachelBig ( int x,int y, cMap *map )
{
	double fak;
	int nr;
	if ( x<0||x>=SettingsData.iScreenW-192||y<0||y>=SettingsData.iScreenH-32 ) return 0;

	x=x* ( int ) ( 448.0/ ( SettingsData.iScreenW-192 ) );
	y=y* ( int ) ( 448.0/ ( SettingsData.iScreenH-32 ) );

	if ( map->size<448 )
	{
		fak=448.0/map->size;
		x= ( int ) ( x/fak );
		y= ( int ) ( y/fak );
		nr=map->Kacheln[x+y*map->size];
	}
	else
	{
		fak=map->size/448.0;
		x= ( int ) ( x*fak );
		y= ( int ) ( y*fak );
		nr=map->Kacheln[x+y*map->size];
	}
	return nr;
}

// W�hlt die Landestelle aus:
void SelectLanding ( int *x,int *y,cMap *map )
{
	SDL_Rect top,bottom;
	int b,lx=-1,ly=-1,i,k,nr,fakx,faky,off;
	sTerrain *t;

	fakx= ( int ) ( ( SettingsData.iScreenW-192.0 ) /game->map->size );
	faky= ( int ) ( ( SettingsData.iScreenH-32.0 ) /game->map->size );

	// Die Karte malen:
	SDL_LockSurface ( buffer );
	for ( i=0;i<SettingsData.iScreenW-192;i++ )
	{
		for ( k=0;k<SettingsData.iScreenH-32;k++ )
		{
			nr=GetKachelBig ( ( i/fakx ) *fakx, ( k/faky ) *faky, map );
			t=TerrainData.terrain+nr;
			off= ( i%fakx ) * ( t->sf_org->h/fakx ) + ( k%faky ) * ( t->sf_org->h/faky ) *t->sf_org->w;
			nr=* ( ( int* ) ( t->sf_org->pixels ) +off );
			if ( nr==0xFF00FF )
			{
				t=TerrainData.terrain+map->DefaultWater;
				off= ( i%fakx ) * ( t->sf_org->h/fakx ) + ( k%faky ) * ( t->sf_org->h/faky ) *t->sf_org->w;
				nr=* ( ( int* ) ( t->sf_org->pixels ) +off );
			}
			( ( int* ) ( buffer->pixels ) ) [i+180+ ( k+18 ) *buffer->w]=nr;
		}
	}
	SDL_UnlockSurface ( buffer );

	// Hud dr�ber legen:
	game->hud->DoAllHud();
	SDL_BlitSurface ( GraphicsData.gfx_hud,NULL,buffer,NULL );


	top.x=0;top.y= ( SettingsData.iScreenH/2 )-479;
	top.w=bottom.w=171;

	top.h=479;bottom.h=481;
	bottom.x=0;bottom.y= ( SettingsData.iScreenH/2 );
	SDL_BlitSurface ( GraphicsData.gfx_panel_top,NULL,buffer,&top );
	SDL_BlitSurface ( GraphicsData.gfx_panel_bottom,NULL,buffer,&bottom );

	SHOW_SCREEN

	t=TerrainData.terrain+GetKachelBig ( mouse->x-180,mouse->y-18, map );
	if ( mouse->x>=180&&mouse->x<SettingsData.iScreenW-12&&mouse->y>=18&&mouse->y<SettingsData.iScreenH-14&&! ( t->water||t->coast||t->blocked ) )
	{
		mouse->SetCursor ( CMove );
	}
	else
	{
		mouse->SetCursor ( CNo );
	}
	mouse->draw ( false,screen );

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();
		t=TerrainData.terrain+GetKachelBig ( mouse->x-180,mouse->y-18, map );
		if ( mouse->x>=180&&mouse->x<SettingsData.iScreenW-12&&mouse->y>=18&&mouse->y<SettingsData.iScreenH-14&&! ( t->water||t->coast||t->blocked ) )
		{
			mouse->SetCursor ( CMove );
		}
		else
		{
			mouse->SetCursor ( CNo );
		}

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
//      SDL_UpdateRect(screen,0,0,0,0);
//      SHOW_SCREEN
		}

		if ( b&&mouse->cur==GraphicsData.gfx_Cmove )
		{
			*x= ( int ) ( ( mouse->x-180 ) / ( 448.0/game->map->size ) * ( 448.0/ ( SettingsData.iScreenW-192 ) ) );
			*y= ( int ) ( ( mouse->y-18 ) / ( 448.0/game->map->size ) * ( 448.0/ ( SettingsData.iScreenH-32 ) ) );
			break;
		}

		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
	}
}

// Zeigt die Liste mit den ausgew�hlten Landefahrzeugen an:
void ShowLandingList ( TList *list,int selected,int offset )
{
	sLanding *ptr;
	SDL_Rect scr,dest,text;
	char str[100];
	int i,t;
	scr.x=330;scr.y=11;
	scr.w=128;scr.h=233;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
	scr.x=0;scr.y=0;
	scr.w=32;scr.h=32;
	dest.x=340;dest.y=20;
	dest.w=32;dest.h=32;
	text.x=375;text.y=32;
	for ( i=offset;i<list->Count;i++ )
	{
		if ( i>=offset+5 ) break;
		ptr=list->LandItems[i];
		// Das Bild malen:
		SDL_BlitSurface ( ptr->sf,&scr,buffer,&dest );
		// Ggf noch Rahmen drum:
		if ( selected==i )
		{
			SDL_Rect tmp;
			tmp=dest;
			tmp.x-=4;
			tmp.y-=4;
			tmp.h=1;
			tmp.w=8;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x+=30;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y+=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x-=30;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y=dest.y-4;
			tmp.w=1;
			tmp.h=8;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x+=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y+=31;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x-=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
		}
		// Text ausgeben:
		t=0;
		str[0]=0;
		while ( VehicleMainData.vehicle[ptr->id].data.name[t]&&fonts->GetTextLen ( str ) <70 )
		{
			str[t]=VehicleMainData.vehicle[ptr->id].data.name[t];str[++t]=0;
		}
		str[t]='.';
		str[t+1]=0;
		fonts->OutText ( str,text.x,text.y,buffer );
		if ( VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_METAL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_OIL||VehicleMainData.vehicle[ptr->id].data.can_transport==TRANS_GOLD )
		{
			sprintf ( str," (%d/%d)",ptr->cargo,VehicleMainData.vehicle[ptr->id].data.max_cargo );
			fonts->OutText ( str,text.x,text.y+10,buffer );
		}
		text.y+=32+10;
		dest.y+=32+10;
	}
}

// Stellt die Selectionlist zusammen:
void CreateSelectionList ( TList *selection,TList *images,int *selected,int *offset,bool tank,bool plane,bool ship,bool build,bool tnt,bool kauf )
{
	sBuildingData *bd;
	sVehicleData *vd;
	int i;
	while ( selection->Count )
	{
		selection->Delete ( 0 );
	}
	if ( kauf )
	{
		plane=false;
		ship=false;
		build=false;
	}
	for ( i=0;i<images->Count;i++ )
	{
		if ( images->HUpItems[i]->vehicle )
		{
			if ( ! ( tank||ship||plane ) ) continue;
			vd=& ( VehicleMainData.vehicle[images->HUpItems[i]->id].data );
			if ( vd->is_alien&&kauf ) continue;
			if ( vd->is_human&&kauf ) continue;
			if ( tnt&&!vd->can_attack ) continue;
			if ( vd->can_drive==DRIVE_AIR&&!plane ) continue;
			if ( vd->can_drive==DRIVE_SEA&&!ship ) continue;
			if ( ( vd->can_drive==DRIVE_LAND||vd->can_drive==DRIVE_LANDnSEA ) &&!tank ) continue;
			selection->AddHUp ( images->HUpItems[i] );
		}
		else
		{
			if ( !build ) continue;
			bd=& ( BuildingMainData.building[ ( ( sHUp* ) ( images->HUpItems[i] ) )->id].data );
			if ( tnt&&!bd->can_attack ) continue;
			selection->AddHUp ( images->HUpItems[i] );
		}
	}
	if ( *offset>=selection->Count-9 )
	{
		*offset=selection->Count-9;
		if ( *offset<0 ) *offset=0;
	}
	if ( *selected>=selection->Count )
	{
		*selected=selection->Count-1;
		if ( *selected<0 ) *selected=0;
	}
}

// Zeigt die Liste mit den Images an:
void ShowSelectionList ( TList *list,int selected,int offset,bool beschreibung,int credits, cPlayer *p )
{
	sHUp *ptr;
	SDL_Rect dest,scr,text;
	char str[100];
	int i,t,k;
	scr.x=479;scr.y=52;
	scr.w=150;scr.h=330;
	SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&scr );
	scr.x=0;scr.y=0;
	scr.w=32;scr.h=32;
	dest.x=490;dest.y=58;
	dest.w=32;dest.h=32;
	text.x=530;text.y=70;
	if ( list->Count==0 )
	{
		scr.x=0;scr.y=0;
		scr.w=316;scr.h=256;
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&scr );
		scr.x=11;scr.y=290;
		scr.w=346;scr.h=176;
		SDL_BlitSurface ( GraphicsData.gfx_upgrade,&scr,buffer,&scr );
		return;
	}
	for ( i=offset;i<list->Count;i++ )
	{
		if ( i>=offset+9 ) break;
		// Das Bild malen:
		ptr = list->HUpItems[i];
		SDL_BlitSurface ( ptr->sf,&scr,buffer,&dest );
		// Ggf noch Rahmen drum:
		if ( selected==i )
		{
			SDL_Rect tmp;
			tmp=dest;
			tmp.x-=4;
			tmp.y-=4;
			tmp.h=1;
			tmp.w=8;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x+=30;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y+=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x-=30;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y=dest.y-4;
			tmp.w=1;
			tmp.h=8;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x+=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.y+=31;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			tmp.x-=38;
			SDL_FillRect ( buffer,&tmp,0xE0E0E0 );
			// Das Bild neu malen:
			tmp.x=11;tmp.y=13;
			if ( ptr->vehicle )
			{
				tmp.w=VehicleMainData.vehicle[ptr->id].info->w;
				tmp.h=VehicleMainData.vehicle[ptr->id].info->h;
				SDL_BlitSurface ( VehicleMainData.vehicle[ptr->id].info,NULL,buffer,&tmp );
			}
			else
			{
				tmp.w=BuildingMainData.building[ptr->id].info->w;
				tmp.h=BuildingMainData.building[ptr->id].info->h;
				SDL_BlitSurface ( BuildingMainData.building[ptr->id].info,NULL,buffer,&tmp );
			}
			// Ggf die Beschreibung ausgeben:
			if ( beschreibung )
			{
				tmp.x+=10;tmp.y+=10;
				tmp.w-=20;tmp.h-=20;
				if ( ptr->vehicle )
				{
					fonts->OutTextBlock ( VehicleMainData.vehicle[ptr->id].text,tmp,buffer );
				}
				else
				{
					fonts->OutTextBlock ( BuildingMainData.building[ptr->id].text,tmp,buffer );
				}
			}
			// Die Details anzeigen:
			{
				cVehicle *tv;
				cBuilding *tb;
				tmp.x=11;
				tmp.y=290;
				tmp.w=346;
				tmp.h=176;
				SDL_BlitSurface ( GraphicsData.gfx_upgrade,&tmp,buffer,&tmp );
				if ( ptr->vehicle )
				{
					tv=new cVehicle ( VehicleMainData.vehicle+ptr->id,p );
					tv->ShowBigDetails();
					delete tv;
				}
				else
				{
					tb=new cBuilding ( BuildingMainData.building+ptr->id,p,NULL );
					tb->ShowBigDetails();
					delete tb;
				}
			}
			// Die Texte anzeigen/Slider machen:
			for ( k=0;k<8;k++ )
			{
				SDL_Rect scr,dest;
				if ( !ptr->upgrades[k].active ) continue;
				sprintf ( str,"%d",ptr->upgrades[k].NextPrice );
				fonts->OutText ( str,322,296+k*19,buffer );

				if ( ptr->upgrades[k].Purchased )
				{
					scr.x=380;scr.y=256;
					dest.w=scr.w=18;dest.h=scr.h=17;
					dest.x=283;dest.y=293+k*19;
					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
				}
				if ( ptr->upgrades[k].NextPrice<=credits )
				{
					scr.x=399;scr.y=256;
					dest.w=scr.w=18;dest.h=scr.h=17;
					dest.x=301;dest.y=293+k*19;
					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff,&scr,buffer,&dest );
				}
			}
		}
		// Text ausgeben:
		t=0;
		if ( ptr->vehicle )
		{
			sprintf ( str,"%d",VehicleMainData.vehicle[ptr->id].data.costs );
			fonts->OutTextCenter ( str,616,text.y,buffer );
			str[0]=0;
			while ( VehicleMainData.vehicle[ptr->id].data.name[t]&&fonts->GetTextLen ( str ) <70 )
			{
				str[t]=VehicleMainData.vehicle[ptr->id].data.name[t];str[++t]=0;
			}
		}
		else
		{
			str[0]=0;
			while ( BuildingMainData.building[ptr->id].data.name[t]&&fonts->GetTextLen ( str ) <85 )
			{
				str[t]=BuildingMainData.building[ptr->id].data.name[t];str[++t]=0;
			}
		}
		str[t]='.';
		str[t+1]=0;
		fonts->OutText ( str,text.x,text.y,buffer );
		text.y+=32+2;
		dest.y+=32+2;
	}
}

// Liefert die Numemr der Farbe zur�ck:
int GetColorNr ( SDL_Surface *sf )
{
	if ( sf==OtherData.colors[cl_red] ) return cl_red;
	if ( sf==OtherData.colors[cl_blue] ) return cl_blue;
	if ( sf==OtherData.colors[cl_green] ) return cl_green;
	if ( sf==OtherData.colors[cl_grey] ) return cl_grey;
	if ( sf==OtherData.colors[cl_orange] ) return cl_orange;
	if ( sf==OtherData.colors[cl_yellow] ) return cl_yellow;
	if ( sf==OtherData.colors[cl_purple] ) return cl_purple;
	if ( sf==OtherData.colors[cl_aqua] ) return cl_aqua;
	return cl_red;
}

cMultiPlayer::cMultiPlayer ( bool host,bool tcp )
{
	NextPlayerID=0;
	PlayerList=new TList;
	PlayerList->AddPlayer ( MyPlayer=new cPlayer ( SettingsData.sPlayerName,OtherData.colors[cl_red],NextPlayerID++ ) );

	MessageList=new TList;
	ChatList=new TList;
	this->host=host;
	this->tcp=tcp;

	if ( tcp )
	{
		if ( host )
		{
			fstcpip=new cFSTcpIp ( true );
			Titel="TCP/IP Host";
			IP="-";
		}
		else
		{
			fstcpip=new cFSTcpIp ( false );
			Titel="TCP/IP Client";
			IP=SettingsData.sIP;
		}
	}
//  fstcpip->FSTcpIpMessageFuntion=ReceiveMenuMessage;
	Port=SettingsData.iPort;

	map="";
	no_options=true;
	WaitForGo=false;
	LetsGo=false;
	game=NULL;
	map_obj=NULL;
}

cMultiPlayer::~cMultiPlayer ( void )
{
	while ( ChatList->Count )
	{
		ChatList->DeleteString ( 0 );
	}
	delete ChatList;
	while ( PlayerList->Count )
	{
		delete PlayerList->PlayerItems[0];
		PlayerList->DeletePlayer ( 0 );
	}
	delete PlayerList;
	if ( game )
	{
		while ( game->PlayerList->Count )
		{
			delete game->PlayerList->PlayerItems[0];
			game->PlayerList->DeletePlayer ( 0 );
		}
		delete game;game=NULL;
	}
	if ( map_obj )
	{
		delete map_obj;map_obj=NULL;
	}
	if ( strcmp ( IP.c_str(),"-" ) ) SettingsData.sIP=IP;
	SettingsData.iPort=Port;
}

// Zeigt das Chatmen� an:
void cMultiPlayer::RunMenu ( void )
{
	bool PlanetPressed=false,OptionsPressed=false,StartHostConnect=false,SendenPressed=false;
	bool OKPressed=false,BackPressed=false,ShowCursor=true,LadenPressed=false;
	int b, lb=0,lx=-1,ly=-1;
	string ChatStr, stmp;
	char sztmp[256];
	SDL_Rect scr,dest;
	Uint8 *keystate;
	unsigned int time;
	int Focus;
	int LastStatus=STAT_CLOSED;
	int LastConnectionCount=0;

#define FOCUS_IP   0
#define FOCUS_PORT 1
#define FOCUS_NAME 2
#define FOCUS_CHAT 3

	LoadPCXtoSF ( GFXOD_MULT,TmpSf );
	SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );

	Focus=FOCUS_NAME;
	ChatStr="";
	fonts->OutTextCenter ( ( char * ) Titel.c_str(),320,11,buffer );

	fonts->OutText ( "IP:",20,245,buffer );
	fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );
	fonts->OutText ( "Port:",228,245,buffer );
	sprintf ( sztmp,"%d",Port );
	fonts->OutText ( sztmp,228,260,buffer );
	fonts->OutText ( "Spielername:",352,245,buffer );
	fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );
	fonts->OutText ( "Farbe:",500,245,buffer );
	dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
	SDL_BlitSurface ( MyPlayer->color,&scr,buffer,&dest );

	if ( host )
	{
		PlaceSmallButton ( "Planet w�hlen",470,42,false );
		PlaceSmallButton ( "Optionen",470,42+35,false );
		PlaceSmallButton ( "Spiel laden",470,42+35*2,false );
		PlaceSmallButton ( "Start Host",470,200,false );
		PlaceButton ( "Ok",390,450,false );
	}
	else
	{
		PlaceSmallButton ( "Connect",470,200,false );
	}
	PlaceSmallButton ( "Senden",470,416,false );

	PlaceButton ( "Zur�ck",50,450,false );

	// Den Focus vorbereiten:
	switch ( Focus )
	{
		case FOCUS_IP:
			InputStr=IP;
			break;
		case FOCUS_PORT:
			InputStr=Port;
			break;
		case FOCUS_NAME:
			InputStr=MyPlayer->name;
			break;
		case FOCUS_CHAT:
			InputStr=ChatStr;
			break;
	}

	mouse->SetCursor ( CHand );
	DisplayGameSettings();
	DisplayPlayerList();
	SHOW_SCREEN
	mouse->draw ( false,screen );
	time=SDL_GetTicks();
	Refresh=false;

	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Tasten pr�fen:
		keystate=SDL_GetKeyState ( NULL );
		if ( keystate[SDLK_ESCAPE] ) break;
		// Die Maus machen:
		mouse->GetPos();
		b=mouse->GetMouseButton();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}

		// Den Focus machen:
		if ( DoKeyInp ( keystate ) )
		{
			ShowCursor=true;
		}
		if ( ShowCursor )
		{
			static bool CursorOn=false;
			ShowCursor=false;
			CursorOn=!CursorOn;
			switch ( Focus )
			{
				case FOCUS_IP:
					stmp = InputStr; stmp += "_";
					while ( fonts->GetTextLen ( ( char * ) stmp.c_str() ) >176 )
						InputStr.erase ( InputStr.length()-1, 0 );
					IP=InputStr;
					scr.x=20;scr.y=260;
					scr.w=188;scr.h=16;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
					stmp = IP; stmp += ( CursorOn?"_":"" );
					fonts->OutText ( ( char * ) stmp.c_str(),20,260,buffer );
					break;
				case FOCUS_PORT:
					stmp = InputStr; stmp += "_";
					while ( fonts->GetTextLen ( ( char * ) stmp.c_str() ) >98 )
						InputStr.erase ( InputStr.length()-1, 0 );
					Port= atoi ( InputStr.c_str() );
					scr.x=228;scr.y=260;
					scr.w=108;scr.h=16;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
					stmp = InputStr; stmp += ( CursorOn?"_":"" );
					fonts->OutText ( ( char * ) stmp.c_str(),228,260,buffer );
					break;
				case FOCUS_NAME:
					stmp = InputStr; stmp += "_";
					while ( fonts->GetTextLen ( ( char * ) stmp.c_str() ) >98 )
						InputStr.erase ( InputStr.length()-1, 0 );
					if ( strcmp ( MyPlayer->name.c_str(),InputStr.c_str() ) )
					{
						MyPlayer->name=InputStr;
						DisplayPlayerList();
						ChangeFarbeName();
					}
					scr.x=352;scr.y=260;
					scr.w=108;scr.h=16;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
					stmp = MyPlayer->name; stmp += ( CursorOn?"_":"" );
					fonts->OutText ( ( char * ) stmp.c_str(),352,260,buffer );
					break;
				case FOCUS_CHAT:
					stmp = InputStr; stmp += "_";
					while ( fonts->GetTextLen ( ( char * ) stmp.c_str() ) >425 )
						InputStr.erase ( InputStr.length()-1, 0 );
					ChatStr=InputStr;
					scr.x=20;scr.y=423;
					scr.w=430;scr.h=16;
					SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
					stmp = ChatStr; stmp += ( CursorOn?"_":"" );
					fonts->OutText ( ( char * ) stmp.c_str(),20,423,buffer );
					break;
			}
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		else
		{
			/*unsigned short hour,min,sec,msec;
			int t;
			(time.CurrentTime()-time).DecodeTime(&hour,&min,&sec,&msec);
			t=(((int)hour*24+min)*60+sec)*1000+msec;
			if(t>500){
			  ShowCursor=true;
			  time=time.CurrentTime();
			}*/
		}

		// Zur�ck:
		if ( mouse->x>=50&&mouse->x<50+200&&mouse->y>=440&&mouse->y<440+29 )
		{
			if ( b&&!lb )
			{
				BackPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Zur�ck",50,450,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&BackPressed )
			{
				if ( fstcpip )
					delete fstcpip;
				break;
			}
		}
		else if ( BackPressed )
		{
			BackPressed=false;
			PlaceButton ( "Zur�ck",50,450,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Ok:
		if ( host&&mouse->x>=390&&mouse->x<390+200&&mouse->y>=440&&mouse->y<440+29&& ( ( !no_options&&!map.empty() ) || ( !SaveGame.empty() ) ) &&!WaitForGo )
		{
			if ( b&&!lb )
			{
				OKPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceButton ( "Ok",390,450,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			else if ( !b&&OKPressed )
			{
				if ( TestPlayerList() && ( TestPlayerListLoad() ||SaveGame.empty() ) )
				{
					/*if(!SaveGame.empty()){
					         unsigned char *msg,*ptr;
					         int file_size,blocks,half,buffer_size;
					         bool start=true;
					         FILE *fp;
					         // Savegame �bertragen:
					         fp=fopen((SavePath+SaveGame).c_str(),"rb");
					         if(!fp){
					           break;
					         }
					         fseek(fp,0,SEEK_END);
					         file_size=ftell(fp);
					         fseek(fp,0,SEEK_SET);
					         blocks=(file_size/240);
					         half=file_size-blocks*240;
					         buffer_size=blocks*243+half+3;

					         ptr=msg=(unsigned char*)malloc(buffer_size);
					         while(blocks--){
					           ptr[0]='#';
					           ptr[1]=243;
					           if(start){
					             start=false;
					             ptr[2]=MSG_SAVEGAME_START;
					           }else{
					             ptr[2]=MSG_SAVEGAME_PART;
					           }
					           fread(ptr+3,1,240,fp);
					           ptr+=243;
					         }
					         if(half){
					           ptr[0]='#';
					           ptr[1]=half+3;
					           if(start){
					             start=false;
					             ptr[2]=MSG_SAVEGAME_START;
					           }else{
					             ptr[2]=MSG_SAVEGAME_PART;
					           }
					           fread(ptr+3,1,half,fp);
					         }

					         fstcpip->Send(msg,buffer_size);

					         fclose(fp);
					         free(msg);
					       }*/
					char sztmp[256];
					string msg;
					sprintf ( sztmp,"%d",SettingsData.Checksum );
					msg=sztmp; msg+="#";
					msg+=MAX_VERSION;
					AddChatLog ( "check for go" );

					WaitForGo=true;
					ClientsToGo=fstcpip->GetConnectionCount();
					fstcpip->FSTcpIpSend ( MSG_CHECK_FOR_GO,msg.c_str(), ( int ) msg.length(),-1 );
				}
				OKPressed=false;
				PlaceButton ( "Ok",390,450,false );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		else if ( OKPressed )
		{
			OKPressed=false;
			PlaceButton ( "Ok",390,450,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Farbe-Next:
		if ( b&&!lb&&mouse->x>=596&&mouse->x<596+18&&mouse->y>=256&&mouse->y<256+18&&!WaitForGo )
		{
			int nr;
			PlayFX ( SoundData.SNDObjectMenu );
			nr=GetColorNr ( MyPlayer->color ) +1;
			if ( nr>7 ) nr=0;
			MyPlayer->color=OtherData.colors[nr];
			fonts->OutText ( "Farbe:",500,245,buffer );
			dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
			SDL_BlitSurface ( MyPlayer->color,&scr,buffer,&dest );
			DisplayPlayerList();
			ChangeFarbeName();
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Farbe-Prev:
		if ( b&&!lb&&mouse->x>=478&&mouse->x<478+18&&mouse->y>=256&&mouse->y<256+18&&!WaitForGo )
		{
			int nr;
			PlayFX ( SoundData.SNDObjectMenu );
			nr=GetColorNr ( MyPlayer->color )-1;
			if ( nr<0 ) nr=7;
			MyPlayer->color=OtherData.colors[nr];
			fonts->OutText ( "Farbe:",500,245,buffer );
			dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
			SDL_BlitSurface ( MyPlayer->color,&scr,buffer,&dest );
			DisplayPlayerList();
			ChangeFarbeName();
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Host-Buttons:
		if ( host&&SaveGame.empty() )
		{
			// Planet w�hlen:
			if ( mouse->x>=470&&mouse->x<470+150&&mouse->y>=42&&mouse->y<42+29 )
			{
				if ( b&&!lb )
				{
					PlanetPressed=true;
					PlayFX ( SoundData.SNDMenuButton );
					PlaceSmallButton ( "Planet w�hlen",470,42,true );
					SHOW_SCREEN
					mouse->draw ( false,screen );
				}
				else if ( !b&&PlanetPressed )
				{
					map=RunPlanetSelect();
					SaveGame="";

					TmpSf=GraphicsData.gfx_shadow;
					SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );
					LoadPCXtoSF ( GFXOD_MULT,TmpSf );
					SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
					DisplayGameSettings();
					DisplayPlayerList();
					fonts->OutTextCenter ( ( char * ) Titel.c_str(),320,11,buffer );
					fonts->OutText ( "IP:",20,245,buffer );
					fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );
					fonts->OutText ( "Port:",228,245,buffer );
					sprintf ( sztmp,"%d",Port );
					fonts->OutText ( sztmp,228,260,buffer );
					fonts->OutText ( "Spielername:",352,245,buffer );
					fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );
					fonts->OutText ( "Farbe:",500,245,buffer );
					dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
					SDL_BlitSurface ( MyPlayer->color,&scr,buffer,&dest );
					PlaceSmallButton ( "Planet w�hlen",470,42,false );
					PlaceSmallButton ( "Optionen",470,42+35,false );
					PlaceSmallButton ( "Spiel laden",470,42+35*2,false );
					PlaceSmallButton ( "Start Host",470,200,false );
					PlaceSmallButton ( "Senden",470,416,false );
					PlaceButton ( "Zur�ck",50,450,false );
					PlaceButton ( "Ok",390,450,false );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					SendOptions();
				}
			}
			else if ( PlanetPressed )
			{
				PlanetPressed=false;
				PlaceSmallButton ( "Planet w�hlen",470,42,false );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			// Optionen:
			if ( mouse->x>=470&&mouse->x<470+150&&mouse->y>=42+35&&mouse->y<42+29+35&&!WaitForGo )
			{
				if ( b&&!lb )
				{
					OptionsPressed=true;
					PlayFX ( SoundData.SNDMenuButton );
					PlaceSmallButton ( "Optionen",470,42+35,true );
					SHOW_SCREEN
					mouse->draw ( false,screen );
				}
				else if ( !b&&OptionsPressed )
				{
					if ( no_options )
					{
						options=RunOptionsMenu ( NULL );
					}
					else
					{
						options=RunOptionsMenu ( &options );
					}
					no_options=false;
					SaveGame="";

					TmpSf=GraphicsData.gfx_shadow;
					SDL_SetAlpha ( TmpSf,SDL_SRCALPHA,255 );
					LoadPCXtoSF ( GFXOD_MULT,TmpSf );
					SDL_BlitSurface ( TmpSf,NULL,buffer,NULL );
					DisplayGameSettings();
					DisplayPlayerList();
					fonts->OutTextCenter ( ( char * ) Titel.c_str(),320,11,buffer );
					fonts->OutText ( "IP:",20,245,buffer );
					fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );
					fonts->OutText ( "Port:",228,245,buffer );
					sprintf ( sztmp,"%d",Port );
					fonts->OutText ( sztmp,228,260,buffer );
					fonts->OutText ( "Spielername:",352,245,buffer );
					fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );
					fonts->OutText ( "Farbe:",500,245,buffer );
					dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
					SDL_BlitSurface ( MyPlayer->color,&scr,buffer,&dest );
					PlaceSmallButton ( "Planet w�hlen",470,42,false );
					PlaceSmallButton ( "Optionen",470,42+35,false );
					PlaceSmallButton ( "Spiel laden",470,42+35*2,false );
					PlaceSmallButton ( "Start Host",470,200,false );
					PlaceSmallButton ( "Senden",470,416,false );
					PlaceButton ( "Zur�ck",50,450,false );
					PlaceButton ( "Ok",390,450,false );
					SHOW_SCREEN
					mouse->draw ( false,screen );
					SendOptions();
				}
			}
			else if ( OptionsPressed )
			{
				OptionsPressed=false;
				PlaceSmallButton ( "Optionen",470,42+35,false );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
			// Spiel laden:
			/*	  if(mouse->x>=470&&mouse->x<470+150&&mouse->y>=42+35*2&&mouse->y<42+29+32*2){
			        if(b&&!lb){
			          LadenPressed=true;
			          PlayFX(SoundData.SNDMenuButton);
			          PlaceSmallButton("Spiel laden",470,42+35*2,true);
			          SHOW_SCREEN
			          mouse->draw(false,screen);
					}else if(!b&&LadenPressed){
			          string tmp;
			          tmp=InputStr;
			          ShowDialog("Dateiname:",true,SavePath,1);
			          if(!InputStr.IsEmpty()){
			            SaveGame=InputStr;

			            map_obj=new cMap;
			            game=new cGame(fstcpip,map_obj);
			            game->Load(SaveGame,0,true);
			          }
			          {
			            InputStr=tmp;
			            TmpSf=GraphicsData.gfx_shadow;
			            SDL_SetAlpha(TmpSf,SDL_SRCALPHA,255);
			            LoadPCXtoSF(GfxODPath+GFXOD_MULT,TmpSf);
			            SDL_BlitSurface(TmpSf,NULL,buffer,NULL);
			            DisplayGameSettings();
			            DisplayPlayerList();
			            fonts->OutTextCenter(Titel.c_str(),320,11,buffer);
			            fonts->OutText("IP:",20,245,buffer);
			            fonts->OutText(IP.c_str(),20,260,buffer);
			            fonts->OutText("Port:",228,245,buffer);
			            fonts->OutText(((AnsiString)Port).c_str(),228,260,buffer);
			            fonts->OutText("Spielername:",352,245,buffer);
			            fonts->OutText(MyPlayer->name.c_str(),352,260,buffer);
			            fonts->OutText("Farbe:",500,245,buffer);
			            dest.x=505;dest.y=260;scr.w=dest.w=83;scr.h=dest.h=10;scr.x=0;scr.y=0;
			            SDL_BlitSurface(MyPlayer->color,&scr,buffer,&dest);
			            if(SaveGame.IsEmpty())PlaceSmallButton("Planet w�hlen",470,42,false);
			            if(SaveGame.IsEmpty())PlaceSmallButton("Optionen",470,42+35,false);
			            if(SaveGame.IsEmpty())PlaceSmallButton("Spiel laden",470,42+35*2,false);
			            PlaceSmallButton("Start Host",470,200,false);
			            PlaceSmallButton("Senden",470,416,false);
			            PlaceButton("Zur�ck",50,450,false);
			            PlaceButton("Ok",390,450,false);
			            SHOW_SCREEN
			            mouse->draw(false,screen);
			            SendOptions();
			          }
			        }
			      }else if(LadenPressed){
			        LadenPressed=false;
			        if(SaveGame.empty())PlaceSmallButton("Spiel laden",470,42+35*2,false);
			        SHOW_SCREEN
			        mouse->draw(false,screen);
			      }*/
		}
		// Host/Connect:
		if ( b&&mouse->x>=470&&mouse->x<470+150&&mouse->y>=200&&mouse->y<200+29&&!WaitForGo )
		{
			if ( !lb )
			{
				StartHostConnect=true;
				PlayFX ( SoundData.SNDMenuButton );
				if ( host ) PlaceSmallButton ( "Start Host",470,200,true );
				else PlaceSmallButton ( "Connect",470,200,true );

				if ( host )
				{
					fstcpip->FSTcpIpClose();
					fstcpip->SetTcpIpPort ( Port );
					FSTcpIpReceiveThread = SDL_CreateThread ( Open,NULL );
					if ( fstcpip->status==STAT_OPENED )
					{
						AddChatLog ( "Fehler beim �ffnen des Sockets" );
					}
					else
					{
						sprintf ( sztmp,"%d",Port );
						stmp="Spiel offen (Port: "; stmp+=sztmp; stmp+=")";
						AddChatLog ( stmp );;
					}
				}
				else
				{
					fstcpip->FSTcpIpClose();
					fstcpip->SetIp ( "192.168.0.3" );
					fstcpip->SetTcpIpPort ( Port );
					FSTcpIpReceiveThread = SDL_CreateThread ( Open,NULL );
					SDL_WaitThread ( FSTcpIpReceiveThread,NULL );
					FSTcpIpReceiveThread=NULL;
					if ( fstcpip->status!=STAT_CONNECTED )
					{
						AddChatLog ( "Fehler beim Verbinden" );
					}
					else
					{
						sprintf ( sztmp,"%d",Port );
						stmp="Verbinde mit "; stmp+=IP; stmp+=":"; stmp+=sztmp;
						AddChatLog ( stmp );
					}
				}

				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		else if ( StartHostConnect )
		{
			StartHostConnect=false;
			if ( host ) PlaceSmallButton ( "Start Host",470,200,false );
			else PlaceSmallButton ( "Connect",470,200,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Senden:
		if ( ( b&&mouse->x>=470&&mouse->x<470+150&&mouse->y>=416&&mouse->y<416+29 ) || ( InputEnter&&Focus==FOCUS_CHAT ) )
		{
			if ( !lb|| ( InputEnter&&Focus==FOCUS_CHAT ) )
			{
				SendenPressed=true;
				PlayFX ( SoundData.SNDMenuButton );
				PlaceSmallButton ( "Senden",470,416,true );

				if ( !ChatStr.empty() )
				{
					PlayFX ( SoundData.SNDChat );
					ChatStr.insert ( 0,": " );
					ChatStr.insert ( 0,MyPlayer->name );

					if ( ChatStr.length() >=200 )
					{
						ChatStr.erase ( 200 );
					}
					fstcpip->FSTcpIpSend ( MSG_CHAT, ( char * ) ChatStr.c_str(), ( int ) ChatStr.length(),-1 );

					AddChatLog ( ChatStr );
					ChatStr="";
					if ( Focus==FOCUS_CHAT ) InputStr="";
					scr.x=20;scr.y=423;scr.w=430;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) ChatStr.c_str(),20,423,buffer );
				}

				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		else if ( SendenPressed )
		{
			SendenPressed=false;
			PlaceSmallButton ( "Senden",470,416,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
		}
		// Klick auf die IP:
		if ( !host&&b&&!lb&&mouse->x>=20&&mouse->x<20+188&&mouse->y>=250&&mouse->y<250+30 )
		{
			Focus=FOCUS_IP;
			InputStr=IP;
			ShowCursor=true;
			sprintf ( sztmp,"%d",Port );
			scr.x=20;scr.y=260;scr.w=188;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );scr.x=228;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( sztmp,228,260,buffer );scr.x=352;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );scr.x=20;scr.y=423;scr.w=430;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) ChatStr.c_str(),20,423,buffer );
			// Klick auf den Port:
		}
		else if ( b&&!lb&&mouse->x>=228&&mouse->x<228+108&&mouse->y>=250&&mouse->y<250+30 )
		{
			Focus=FOCUS_PORT;
			sprintf ( sztmp,"%d",Port );
			InputStr=sztmp;
			ShowCursor=true;
			scr.x=20;scr.y=260;scr.w=188;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );scr.x=228;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( sztmp,228,260,buffer );scr.x=352;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );scr.x=20;scr.y=423;scr.w=430;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) ChatStr.c_str(),20,423,buffer );
			// Klick auf den Namen:
		}
		else if ( b&&!lb&&mouse->x>=352&&mouse->x<352+108&&mouse->y>=250&&mouse->y<250+30 )
		{
			Focus=FOCUS_NAME;
			InputStr=MyPlayer->name;
			ShowCursor=true;
			sprintf ( sztmp,"%d",Port );
			scr.x=20;scr.y=260;scr.w=188;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );scr.x=228;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( sztmp,228,260,buffer );scr.x=352;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );scr.x=20;scr.y=423;scr.w=430;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) ChatStr.c_str(),20,423,buffer );
			// Klick auf den ChatStr:
		}
		else if ( b&&!lb&&mouse->x>=20&&mouse->x<20+425&&mouse->y>=420&&mouse->y<420+30 )
		{
			Focus=FOCUS_CHAT;
			InputStr=ChatStr;
			ShowCursor=true;
			sprintf ( sztmp,"%d",Port );
			scr.x=20;scr.y=260;scr.w=188;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) IP.c_str(),20,260,buffer );scr.x=228;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( sztmp,228,260,buffer );scr.x=352;scr.y=260;scr.w=108;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) MyPlayer->name.c_str(),352,260,buffer );scr.x=20;scr.y=423;scr.w=430;scr.h=16;SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );fonts->OutText ( ( char * ) ChatStr.c_str(),20,423,buffer );
		}

		// Das WaitForGo machen:
		if ( host&&WaitForGo )
		{
			if ( ClientsToGo>fstcpip->GetConnectionCount() )
			{
				AddChatLog ( "abort go" );
				WaitForGo=false;
			}
			else if ( ClientsToGo<=0&&SaveGame.empty() )
			{
				ClientSettingsList=new TList;

				AddChatLog ( "lets go" );
				fstcpip->FSTcpIpSend ( MSG_LETS_GO,"",0,-1 );

				// Das Spiel machen:
				TList *LandingList;
				int i,LandX,LandY;
				map_obj=new cMap();
				if ( map_obj->LoadMap ( map ) )
				{

					map_obj->PlaceRessources ( options.metal,options.oil,options.gold,options.dichte );
					game=new cGame ( fstcpip,map_obj );
					game->AlienTech=options.AlienTech;
					game->PlayRounds=options.PlayRounds;
					game->ActiveRoundPlayerNr=MyPlayer->Nr;
					game->Init ( PlayerList,0 );

					for ( i=0;i<PlayerList->Count;i++ )
					{
						PlayerList->PlayerItems[i]->InitMaps ( map_obj->size );
					}
					MyPlayer->Credits=options.credits;

					LandingList=new TList;
					RunHangar ( MyPlayer,LandingList );

					SelectLanding ( &LandX,&LandY,map_obj );

					ServerWait ( LandX,LandY,LandingList );
//-          fstcpip->RxFunc=game->engine->ReceiveNetMsg;

					while ( LandingList->Count )
					{
						delete LandingList->LandItems[0];
						LandingList->DeleteLanding ( 0 );
					}
					delete LandingList;

					ExitMenu();

					fstcpip->min_clients=PlayerList->Count-1;
					game->Run();
					SettingsData.sPlayerName=MyPlayer->name;

					while ( PlayerList->Count )
					{
						delete PlayerList->PlayerItems[0];
						PlayerList->DeletePlayer ( 0 );
					}
					delete game;game=NULL;
					break;
				}
				else
				{
					AddChatLog ( "Error loading map" );
					delete ClientSettingsList;
				}
			}
			else if ( ClientsToGo<=0&&!SaveGame.empty() )
			{
				/*unsigned char msg[3];
				int i;

				msg[0]='#';
				msg[1]=3;
				msg[2]=MSG_LETS_GO;
				fstcpip->Send(msg,3);

				ExitMenu();
				fstcpip->RxFunc=game->engine->ReceiveNetMsg;
				TmpSf=NULL;

				for(i=0;i<game->PlayerList->Count;i++){
				  if(((cPlayer*)(game->PlayerList->Items[i]))->name==MyPlayer->name){
				    game->ActivePlayer=(cPlayer*)(game->PlayerList->Items[i]);
				    break;
				  }
				}

				fstcpip->MinConnections=PlayerList->Count-1;
				*(game->hud)=game->ActivePlayer->HotHud;
				if(game->hud->Zoom!=64){
				  game->hud->LastZoom=-1;
				  game->hud->ScaleSurfaces();
				}
				game->Run();
				sPlayerName=MyPlayer->name;

				break;*/
			}
		}

		// Das LetsGo machen:
		if ( !host&&LetsGo&&SaveGame.empty() )
		{
			// Das Spiel machen:
			int i,LandX,LandY,nr;
			TList *LandingList;
			map_obj=new cMap();
			LetsGo=false;
			if ( map_obj->LoadMap ( map ) )
			{

				for ( i=0;i<PlayerList->Count;i++ )
				{
					if ( PlayerList->PlayerItems[i]==MyPlayer ) {nr=i;break;}
				}

				game=new cGame ( fstcpip,map_obj );
				game->AlienTech=options.AlienTech;
				game->PlayRounds=options.PlayRounds;
				game->ActiveRoundPlayerNr=-1;
				game->Init ( PlayerList,nr );

				for ( i=0;i<PlayerList->Count;i++ )
				{
					PlayerList->PlayerItems[i]->InitMaps ( map_obj->size );
				}

				MyPlayer->Credits=options.credits;

				LandingList=new TList;
				RunHangar ( MyPlayer,LandingList );

				SelectLanding ( &LandX,&LandY,map_obj );

				// Settings �bertragen:
				ClientWait ( LandX,LandY,LandingList );
//-        fstcpip->RxFunc=game->engine->ReceiveNetMsg;

				while ( LandingList->Count )
				{
					delete LandingList->LandItems[0];
					LandingList->DeleteLanding ( 0 );
				}
				delete LandingList;

				ExitMenu();

				fstcpip->min_clients=PlayerList->Count-1;
				game->Run();
				SettingsData.sPlayerName=MyPlayer->name;

				while ( PlayerList->Count )
				{
					delete PlayerList->PlayerItems[0];
					PlayerList->DeletePlayer ( 0 );
				}
				delete game;game=NULL;
				break;
			}
			else
			{
				AddChatLog ( "Error loading map" );
				delete map_obj;map_obj=NULL;
			}
		}
		else if ( !host&&LetsGo&&!SaveGame.empty() )
		{
			/*int i;

			map_obj=new cMap;
			game=new cGame(fstcpip,map_obj);
			game->Load(SaveGame,0,true);

			ExitMenu();
			TmpSf=NULL;
			fstcpip->RxFunc=game->engine->ReceiveNetMsg;

			for(i=0;i<game->PlayerList->Count;i++){
			  if(((cPlayer*)(game->PlayerList->Items[i]))->name==MyPlayer->name){
			    game->ActivePlayer=(cPlayer*)(game->PlayerList->Items[i]);
			    break;
			  }
			}

			fstcpip->MinConnections=PlayerList->Count-1;
			*(game->hud)=game->ActivePlayer->HotHud;
			if(game->hud->Zoom!=64){
			  game->hud->LastZoom=-1;
			  game->hud->ScaleSurfaces();
			}
			game->Run();
			sPlayerName=MyPlayer->name;

			break;*/
		}

		// Ggf Chatlogs anzeigen:
		ShowChatLog();
		// Ggf weitere Daten anzeigen:
		if ( Refresh )
		{
			Refresh=false;
			DisplayGameSettings();
			DisplayPlayerList();
		}

		// Ggf Meldung �ber Status�nderung machen:
		if ( LastStatus!=fstcpip->status )
		{
			LastStatus=fstcpip->status;
			switch ( LastStatus )
			{
				case STAT_CONNECTED:
					if ( host )
					{
						AddChatLog ( "fstcpip: new connection" );
					}
					else
					{
						AddChatLog ( "fstcpip: Verbunden" );
						ClientConnectedCallBack();
					}
					break;
					/*case STAT_WAITING:
					  AddChatLog("fstcpip: Wartet");
					  break;*/
				case STAT_CLOSED:
					AddChatLog ( "fstcpip: closed" );
					if ( !host ) ClientDistconnect();
					break;
					/*case STAT_OPENED:
					  AddChatLog("fstcpip: open");
					  if(host)ServerDisconnect();
					  break;*/
			}
		}
		if ( host )
		{
			if ( LastConnectionCount>fstcpip->GetConnectionCount() )
			{
				ServerDisconnect();
			}
			LastConnectionCount=fstcpip->GetConnectionCount();
		}

		lx=mouse->x;
		ly=mouse->y;
		lb=b;
		if ( fstcpip->status==STAT_CONNECTED && !FSTcpIpReceiveThread )
			FSTcpIpReceiveThread = SDL_CreateThread ( Receive,NULL );
		HandleMenuMessages();
		SDL_Delay ( 1 );
	}
	TmpSf=NULL;
}

// Empf�ngt eine Nachricht f�rs Men�:
void cMultiPlayer::HandleMenuMessages()
{
	cNetMessage *msg;
	string msgstring;
	int iMessageCount = MessageList->Count;
	if ( iMessageCount<1 ) return;
	for ( int i=0;i<iMessageCount;i++ )
	{
		msg = MessageList->NetMessageItems[0];
		msgstring = ( char * ) msg->msg;
		switch ( msg->typ )
		{
				// Chatnachricht:
			case MSG_CHAT:
				AddChatLog ( msgstring );
				PlayFX ( SoundData.SNDChat );
				MessageList->DeleteNetMessage ( 0 );
				break;
				// Neuer Spieler meldet sich an:
			case MSG_SIGNING_IN:
			{
				cPlayer *p;
				char sztmp[256];
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				p=new cPlayer ( Strings->Items[0],OtherData.colors[atoi ( Strings->Items[1].c_str() ) ],NextPlayerID++ );
				PlayerList->AddPlayer ( p );
				Refresh=true;
				string smsg;
				sprintf ( sztmp,"%d",p->Nr );
				smsg=Strings->Items[2]; smsg+="#"; smsg+=sztmp;
				fstcpip->FSTcpIpSend ( MSG_YOUR_ID_IS, ( char * ) smsg.c_str(), ( int ) smsg.length(),-1 );
				SendPlayerList();
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Mitteilung �ber die eigene ID:
			case MSG_YOUR_ID_IS:
			{
				cPlayer *p;
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				if ( MyPlayer->Nr!=atoi ( Strings->Items[0].c_str() ) )
				{
					MessageList->DeleteNetMessage ( 0 );
					break;
				}
				for ( i=0;i<PlayerList->Count;i++ )
				{
					p=PlayerList->PlayerItems[i];
					if ( p==MyPlayer )
					{
						p->Nr=atoi ( Strings->Items[1].c_str() );
						break;
					}
				}
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Ein Client �ndert seinen Namen:
			case MSG_MY_NAME_CHANGED:
			{
				cPlayer *p;
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				int i;
				for ( i=0;i<PlayerList->Count;i++ )
				{
					p=PlayerList->PlayerItems[i];
					if ( p->Nr!=atoi ( Strings->Items[0].c_str() ) ) continue;
					p->color=OtherData.colors[atoi ( Strings->Items[1].c_str() ) ];
					p->name=Strings->Items[2];
					Refresh=true;
					SendPlayerList();
					break;
				}
				if ( i==PlayerList->Count )
				{
					p=new cPlayer ( Strings->Items[2],OtherData.colors[atoi ( Strings->Items[1].c_str() ) ],atoi ( Strings->Items[0].c_str() ) );
					PlayerList->AddPlayer ( p );
					SendPlayerList();
					Refresh=true;
				}
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Bekommt die Liste mit den Spielern:
			case MSG_PLAYER_LIST:
			{
				int count,myID;
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				myID=MyPlayer->Nr;
				while ( PlayerList->Count )
				{
					delete PlayerList->PlayerItems[0];
					PlayerList->DeletePlayer ( 0 );
				}
				count=atoi ( Strings->Items[0].c_str() );
				for ( int i = 0;count--;i++ )
				{
					cPlayer *p;
					int id,color;
					id=atoi ( Strings->Items[i*3+1].c_str() );
					color=atoi ( Strings->Items[i*3+2].c_str() );

					p=new cPlayer ( Strings->Items[i*3+3],OtherData.colors[color],id );
					if ( id==myID ) MyPlayer=p;
					PlayerList->AddPlayer ( p );
				}
				Refresh=true;
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// �bertr�gt die Optionen:
			case MSG_OPTIONS:
			{
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				no_options=atoi ( Strings->Items[0].c_str() );

				SaveGame=Strings->Items[1];

				if ( !no_options )
				{
					options.AlienTech=atoi ( Strings->Items[2].c_str() );
					options.credits=atoi ( Strings->Items[3].c_str() );
					options.dichte=atoi ( Strings->Items[4].c_str() );
					options.FixedBridgeHead=atoi ( Strings->Items[5].c_str() );
					options.gold=atoi ( Strings->Items[6].c_str() );
					options.metal=atoi ( Strings->Items[7].c_str() );
					options.oil=atoi ( Strings->Items[8].c_str() );
					options.PlayRounds=atoi ( Strings->Items[9].c_str() );
					map=Strings->Items[10];
				}
				else
					map=Strings->Items[4];
				Refresh=true;
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Fordert einen Client auf sich zu identifizieren:
			case MSG_WHO_ARE_YOU:
			{
				ChangeFarbeName();
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Pr�fen, ob der Client bereit ist zum Go:
			case MSG_CHECK_FOR_GO:
			{
				char sztmp[256];
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				FILE *fp;
				string mapstr;
				mapstr=SettingsData.sMapsPath; mapstr+=map;
				fp=fopen ( mapstr.c_str(),"rb" );
				if ( atoi ( Strings->Items[0].c_str() ) ==SettingsData.Checksum && strcmp ( Strings->Items[1].c_str(),MAX_VERSION ) ==0 && fp )
				{
					string new_msg;
					sprintf ( sztmp,"%d",MyPlayer->Nr );
					new_msg=sztmp;
					fstcpip->FSTcpIpSend ( MSG_READY_TO_GO,new_msg.c_str(), ( int ) new_msg.length(),-1 );
					AddChatLog ( "host wants to go: ready to go" );
				}
				else
				{
					string new_msg;
					sprintf ( sztmp,"%d",MyPlayer->Nr );
					new_msg=sztmp;
					fstcpip->FSTcpIpSend ( MSG_NO_GO,new_msg.c_str(), ( int ) new_msg.length(),-1 );
					AddChatLog ( "host wants to go: no go" );
				}
				fclose ( fp );
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Benachrichtigung �ber einen nicht bereiten Client:
			case MSG_NO_GO:
			{
				cPlayer *p;
				int i;
				for ( i=0;i<PlayerList->Count;i++ )
				{
					p=PlayerList->PlayerItems[i];
					if ( p->Nr==atoi ( msgstring.c_str() ) )
					{
						string log;
						log=p->name; log+=": no go";
						AddChatLog ( log );
						break;
					}
				}
				WaitForGo=false;
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Benachrichtigung �ber einen bereiten Client:
			case MSG_READY_TO_GO:
			{
				cPlayer *p;
				int i;
				for ( i=0;i<PlayerList->Count;i++ )
				{
					p=PlayerList->PlayerItems[i];
					if ( p->Nr==atoi ( msgstring.c_str() ) )
					{
						string log;
						log=p->name; log+=": ready to go";
						AddChatLog ( log );
						break;
					}
				}
				ClientsToGo--;
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Benachrichtigung, dass es jetzt los geht:
			case MSG_LETS_GO:
				LetsGo=true;
				MessageList->DeleteNetMessage ( 0 );
				break;
				// Die Ressourcen:
			case MSG_RESSOURCES:
			{
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				int off;
				if ( map_obj==NULL ) break;
				for ( int i=0;i<Strings->Count;i++ )
				{
					off=atoi ( Strings->Items[i].c_str() );
					map_obj->Resources[off].typ= ( unsigned char ) atoi ( Strings->Items[i++].c_str() );
					map_obj->Resources[off].value= ( unsigned char ) atoi ( Strings->Items[i++].c_str() );
				}
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Empfang der Upgrades eines Players:
			case MSG_PLAYER_UPGRADES:
			{
				cPlayer *p;
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				int nr,i;
				nr=atoi ( Strings->Items[0].c_str() );
				for ( i=0;i<PlayerList->Count;i++ )
				{
					p=PlayerList->PlayerItems[i];
					if ( p->Nr==nr )
						break;
				}
				if ( p==MyPlayer )
				{
					MessageList->DeleteNetMessage ( 0 );
					break;
				}
				for ( int i=1;i<Strings->Count;i++ )
				{
					if ( atoi ( Strings->Items[i].c_str() ) ==0 )
					{
						p->VehicleData[i].damage=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].max_shots=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].range=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].max_ammo=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].armor=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].max_hit_points=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].scan=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].max_speed=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].version++;
					}
					else
					{
						p->BuildingData[i].damage=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].max_shots=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].range=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].max_ammo=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].armor=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].max_hit_points=atoi ( Strings->Items[i++].c_str() );
						p->BuildingData[i].scan=atoi ( Strings->Items[i++].c_str() );
						p->VehicleData[i].version++;
					}
				}
				MessageList->DeleteNetMessage ( 0 );
				break;
			}
			// Landedaten eines Players:
			case MSG_PLAYER_LANDING:
			{
				TList *Strings;
				Strings = SplitMessage ( msgstring );
				sClientSettings *cs=NULL;
				int nr,i,max;

				nr=atoi ( Strings->Items[2].c_str() );
				if ( nr==MyPlayer->Nr )
				{
					MessageList->DeleteNetMessage ( 0 );
					break;
				}

				for ( i=0;i<ClientSettingsList->Count;i++ )
				{
					cs=ClientSettingsList->ClientSettingsItems[i];
					if ( cs->nr==nr )
					{
						break;
					}
				}

				if ( cs==NULL||i==ClientSettingsList->Count )
				{
					cs=new sClientSettings;
					cs->LandX=atoi ( Strings->Items[0].c_str() );
					cs->LandY=atoi ( Strings->Items[1].c_str() );
					cs->nr=nr;
					cs->LandingList=new TList;
					ClientSettingsList->AddClientSettings ( cs );
				}
				max=atoi ( Strings->Items[3].c_str() );

				for ( i=4;i< ( max*2+4 );i++ )
				{
					sLanding *l;
					l=new sLanding;
					l->id=atoi ( Strings->Items[i].c_str() );
					l->cargo=atoi ( Strings->Items[i++].c_str() );
					cs->LandingList->AddLanding ( l );
				}
				MessageList->DeleteNetMessage ( 0 );
				break;
			}

		}
	}
}

// Zeigt die Settings f�r das Spiel an:
void cMultiPlayer::DisplayGameSettings ( void )
{
	string str;
	char sztmp[256];
	SDL_Rect r;

	r.x=192;r.y=52;
	r.w=246;r.h=176;

	if ( !host )
	{
		SDL_BlitSurface ( TmpSf,&r,buffer,&r );
	}

	str="Version: "; str+=MAX_VERSION; str+="\n";
	sprintf ( sztmp,"%d",SettingsData.Checksum );
	str+="Checksum: "; str+=sztmp; str+="\n";
	str+="\n";

	if ( !host&&fstcpip->status!=STAT_CONNECTED )
	{
		str+="nicht verbunden";
		fonts->OutTextBlock ( ( char * ) str.c_str(),r,buffer );
		return;
	}

	if ( !SaveGame.empty() )
	{
		char *tmpstr;
		FILE *fp;
		int len;
		str+="Savegame: "; str+=SaveGame; str+="\n";
		if ( host )
		{
			fp=fopen ( ( SaveGame ).c_str(),"rb" );
			if ( fp==NULL )
			{
				str+="Fehler beim �ffnen\n";
			}
			else
			{
				fread ( &len,sizeof ( int ),1,fp );
				tmpstr= ( char* ) malloc ( len );
				fread ( tmpstr,1,len,fp );
				map=tmpstr;
				free ( tmpstr );
				sprintf ( sztmp,"%d",game->Runde );
				str+="Runde: "; str+=sztmp; str+="\n";

				if ( host&&game )
				{
					int i;
					str+="Player: ";
					for ( i=0;i<game->PlayerList->Count;i++ )
					{
						str+=game->PlayerList->PlayerItems[i]->name;
						if ( i<game->PlayerList->Count-1 ) str+=",";
					}
					str+="\n";
				}

				fclose ( fp );
			}
		}
		str+="\n";
	}

	if ( !map.empty() )
	{
		FILE *fp;
		string mapstr;
		mapstr=SettingsData.sMapsPath; mapstr+=map;
		fp=fopen ( mapstr.c_str(),"rb" );
		if ( !fp )
		{
			str+="Map: "; str+=map; str+=" !NICHT VORHANDEN!\n";
			fclose ( fp );
		}
		else
		{
			SDL_Surface *sf;
			SDL_Rect r;
			int size=0;
			str+="Map: "; str+=map;
			if ( fp )
			{
				fseek ( fp,21,SEEK_SET );
				fread ( &size,sizeof ( int ),1,fp );
				fclose ( fp );
			}
			sprintf ( sztmp,"%d",size );
			str+=" ("; str+=sztmp; str+="x"; str+=sztmp; str+=")\n";

			r.x=20;r.y=60;r.w=150;r.h=20;
			SDL_BlitSurface ( TmpSf,&r,buffer,&r );
			mapstr=map; mapstr+=" ("; mapstr+=sztmp; mapstr+="x"; mapstr+=sztmp; mapstr+=")";
			fonts->OutTextCenter ( ( char * ) mapstr.c_str(),90,65,buffer );

			string mapstr;
			mapstr=SettingsData.sMapsPath; mapstr+=map; mapstr.replace ( mapstr.length()-3,3,"bmp" );
			fp=fopen ( mapstr.c_str(),"rb" );
			if ( fp )
			{
				sf=SDL_LoadBMP ( mapstr.c_str() );
				if ( sf!=NULL )
				{
					SDL_Rect dest;
					dest.w=dest.h=112;
					dest.x=33;dest.y=106;
					SDL_BlitSurface ( sf,NULL,buffer,&dest );
					SDL_FreeSurface ( sf );
				}
			}
		}
	}
	else
	{
		if ( SaveGame.empty() ) str+=">keine Map gew�hlt<\n";
	}
	str+="\n";
	if ( SaveGame.empty() )
	{
		if ( !no_options )
		{
			str+="Metall: "; str+= ( options.metal<2? ( options.metal<1?"wenig":"mittel" ) : ( options.metal<3?"viel":"extrem" ) ); str+="\n";
			str+="�l: "; str+= ( options.oil<2? ( options.oil<1?"wenig":"mittel" ) : ( options.oil<3?"viel":"extrem" ) ); str+="\n";
			str+="Gold: "; str+= ( options.gold<2? ( options.gold<1?"wenig":"mittel" ) : ( options.gold<3?"viel":"extrem" ) ); str+="\n";
			str+="Ressourcendichte: "; str+= ( options.dichte<2? ( options.dichte<1?"d�nn":"normal" ) : ( options.gold<3?"dicht":"extrem" ) ); str+="\n";
			str+="Credits: "; sprintf ( sztmp,"%d",options.credits ); str+=sztmp; str+="\n";
			str+="Br�ckenkopf: "; str+= ( options.FixedBridgeHead?"fest":"mobil" ); str+="\n";
			str+="Alientechnologie: "; str+= ( options.AlienTech?"an":"aus" ); str+="\n";
			str+="Spielart: "; str+= ( options.PlayRounds?"Runden":"simultan" ); str+="\n";
		}
		else
		{
			str+=">keine Optionen gew�hlt<\n";
		}
	}

	fonts->OutTextBlock ( ( char * ) str.c_str(),r,buffer );
}

TList* cMultiPlayer::SplitMessage ( string msg )
{
	TList *Strings;
	Strings = new TList;
	int npos=0;
	for ( int i=0; npos!=string::npos; i++ )
	{
		Strings->Items[i]=msg.substr ( npos, ( msg.find ( "#",npos )-npos ) );
		npos= ( int ) msg.find ( "#",npos );
		if ( npos!=string::npos )
			npos++;
	}
	return Strings;
}


void cMultiPlayer::ShowChatLog ( void )
{
	string str;
	int i;
	if ( !ChatList->Count ) return;
	for ( i=ChatList->Count-1;i>=0;i-- )
	{
		str=ChatList->Items[i];
		while ( fonts->GetTextLen ( ( char * ) str.c_str() ) >410 )
		{
			str.erase ( str.length()-1,0 );
		}
		SDL_Rect scr,dest;
		scr.x=27;scr.y=298+11;
		dest.w=scr.w=420;dest.h=scr.h=8*11;
		dest.x=27;dest.y=298;
		SDL_BlitSurface ( buffer,&scr,buffer,&dest );
		dest.y=298+8*11;
		dest.h=11;
		SDL_BlitSurface ( TmpSf,&dest,buffer,&dest );
		fonts->OutText ( ( char * ) str.c_str(),dest.x,dest.y,buffer );
	}
	while ( ChatList->Count>0 )
		ChatList->DeleteString ( 0 );
	SHOW_SCREEN
	mouse->draw ( false,screen );
}

// F�gt einen ChatLogEintrag hinzu:
void cMultiPlayer::AddChatLog ( string str )
{
	ChatList->Add ( str );
}

// Zeigt die Liste ,it den Spielern an:
void cMultiPlayer::DisplayPlayerList ( void )
{
	SDL_Rect scr,dest;
	cPlayer *p;
	int i;
	scr.x=465;scr.y=287;
	scr.w=162;scr.h=116;
	SDL_BlitSurface ( TmpSf,&scr,buffer,&scr );
	scr.x=0;scr.y=0;
	dest.w=dest.h=scr.w=scr.h=10;
	dest.x=476;dest.y=297;

	for ( i=0;i<PlayerList->Count;i++ )
	{
		p=PlayerList->PlayerItems[i];

		SDL_BlitSurface ( p->color,&scr,buffer,&dest );
		fonts->OutText ( ( char * ) p->name.c_str(),dest.x+16,dest.y,buffer );

		dest.y+=16;
	}
	SHOW_SCREEN
}

// Callback f�r einen Client, der eine Connection bekommt:
void cMultiPlayer::ClientConnectedCallBack ( void )
{
	char sztmp[32];
	string msg;
	MyPlayer->Nr=100+random ( 1000000,1 );
	msg=MyPlayer->name; msg+="#";
	sprintf ( sztmp,"%d",GetColorNr ( MyPlayer->color ) );
	msg+=sztmp; msg+="#";
	sprintf ( sztmp,"%d",MyPlayer->Nr );
	msg+=sztmp;
	fstcpip->FSTcpIpSend ( MSG_SIGNING_IN, ( char * ) msg.c_str(), ( int ) msg.length(),-1 );
}

// Meldet einen Disconnect, wenn man Client ist:
void cMultiPlayer::ClientDistconnect ( void )
{
	Refresh=true;
	while ( PlayerList->Count )
	{
		if ( PlayerList->PlayerItems[0]!=MyPlayer )
		{
			delete PlayerList->PlayerItems[0];
		}
		PlayerList->DeletePlayer ( 0 );
	}
	PlayerList->AddPlayer ( MyPlayer );
	MyPlayer->Nr=0;
}

// Meldet einen Disconnect, wenn man Server ist:
void cMultiPlayer::ServerDisconnect ( void )
{
	while ( PlayerList->Count )
	{
		if ( PlayerList->PlayerItems[0]!=MyPlayer )
		{
			delete PlayerList->PlayerItems[0];
		}
		PlayerList->DeletePlayer ( 0 );
	}
	PlayerList->AddPlayer ( MyPlayer );

	fstcpip->FSTcpIpSend ( MSG_WHO_ARE_YOU,"",0,-1 );
	Refresh=true;
}

// Wird aufgerufen, wenn die Farbe/Name ge�ndert wurden:
void cMultiPlayer::ChangeFarbeName ( void )
{
	string msg;
	char sztmp[256];
	if ( fstcpip->server && !fstcpip->GetConnectionCount() ) return;
	if ( !fstcpip->server && fstcpip->status!=STAT_CONNECTED ) return;

	if ( fstcpip->server )
	{
		SendPlayerList();
		return;
	}
	sprintf ( sztmp,"%d",MyPlayer->Nr );
	msg=sztmp; msg+="#";
	sprintf ( sztmp,"%d",GetColorNr ( MyPlayer->color ) );
	msg+=sztmp; msg+="#";
	msg+=MyPlayer->name;
	fstcpip->FSTcpIpSend ( MSG_MY_NAME_CHANGED, ( char * ) msg.c_str(), ( int ) msg.length(),-1 );
}

// Versendet eine Liste mit allen Spielern:
void cMultiPlayer::SendPlayerList ( void )
{
	char sztmp[256];
	string msg;
	cPlayer *p;
	sprintf ( sztmp,"%d",PlayerList->Count );
	msg=sztmp; msg+="#";
	for ( int i=0; i<PlayerList->Count; i++ )
	{
		p=PlayerList->PlayerItems[i];
		sprintf ( sztmp,"%d",p->Nr );
		msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",GetColorNr ( p->color ) );
		msg+=sztmp; msg+="#";
		msg+=p->name; if ( i!=PlayerList->Count-1 ) msg+="#";
	}
	fstcpip->FSTcpIpSend ( MSG_PLAYER_LIST, ( char * ) msg.c_str(), ( int ) msg.length(),-1 );
}

// �bertr�gt die Spieloptionen:
void cMultiPlayer::SendOptions ( void )
{
	char sztmp[256];
	string msg;
	sprintf ( sztmp,"%d",no_options );
	msg=sztmp; msg+="#";
	msg+=SaveGame; msg+="#";
	if ( !no_options )
	{
		sprintf ( sztmp,"%d",options.AlienTech ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.credits ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.dichte ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.FixedBridgeHead ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.gold ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.metal ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.oil ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",options.PlayRounds ); msg+=sztmp; msg+="#";
	}
	msg+=map;

	fstcpip->FSTcpIpSend ( MSG_OPTIONS, msg.c_str(), ( int ) msg.length(),-1 );
}

// Sendet die Ressourcmap an alle Clients:
void cMultiPlayer::TransmitRessources ( void )
{
	string msg;
	char sztmp[256];
	int i;
	for ( i=0;i<map_obj->size*map_obj->size;i++ )
	{
		if ( !map_obj->Resources[i].typ ) continue;
		if ( msg.length() >0 ) msg+="#";
		sprintf ( sztmp,"%d",no_options ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",map_obj->Resources[i].typ ); msg+=sztmp; msg+="#";
		sprintf ( sztmp,"%d",map_obj->Resources[i].value ); msg+=sztmp;
		if ( msg.length() >200 )
		{
			fstcpip->FSTcpIpSend ( MSG_RESSOURCES,msg.c_str(), ( int ) msg.length(),-1 );
//      SDL_Delay(10);
			msg="";
		}
	}
	if ( msg.length() >0 )
	{
		fstcpip->FSTcpIpSend ( MSG_RESSOURCES,msg.c_str(), ( int ) msg.length(),-1 );
//    SDL_Delay(10);
	}
}

// Wartet auf alle anderen Spieler (als Server):
void cMultiPlayer::ServerWait ( int LandX,int LandY,TList *LandingList )
{
	int lx=-1,ly=-1;
	int i;
	fonts->OutTextBigCenter ( "Warte auf andere Spieler...",320,235,buffer );
	SHOW_SCREEN
	mouse->SetCursor ( CHand );
	mouse->draw ( false,screen );

	while ( ClientSettingsList->Count<PlayerList->Count-1 )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();

		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
		HandleMenuMessages();
	}

	// Alle Upgrades �bertragen:
	for ( i=0;i<PlayerList->Count;i++ )
	{
		TransmitPlayerUpgrades ( PlayerList->PlayerItems[i] );
	}

	TransmitPlayerLanding ( MyPlayer->Nr,LandX,LandY,LandingList );
	game->MakeLanding ( LandX,LandY,MyPlayer,LandingList,options.FixedBridgeHead );

	// Alle Landungen �bertragen:
	while ( ClientSettingsList->Count )
	{
		sClientSettings *cs;
		cPlayer *p;
		cs=ClientSettingsList->ClientSettingsItems[0];

		TransmitPlayerLanding ( cs->nr,cs->LandX,cs->LandY,cs->LandingList );

		for ( i=0;i<PlayerList->Count;i++ )
		{
			p=PlayerList->PlayerItems[i];
			if ( p->Nr==cs->nr ) break;
		}
		game->MakeLanding ( cs->LandX,cs->LandY,p,cs->LandingList,options.FixedBridgeHead );

		while ( cs->LandingList->Count )
		{
			sLanding *l;
			l=cs->LandingList->LandItems[0];
			delete l;
			cs->LandingList->DeleteLanding ( 0 );
		}
		delete cs;
		ClientSettingsList->DeleteClientSettings ( 0 );
	}
	delete ClientSettingsList;

	// Die Ressourcen �bertragen:
	TransmitRessources();

	fstcpip->FSTcpIpSend ( MSG_LETS_GO,"",0,-1 );
}

// �bertr�gt alle Settings und wartet auf die Daten des Servers:
void cMultiPlayer::ClientWait ( int LandX,int LandY,TList *LandingList )
{
	int lx=-1,ly=-1;
	int i;
	fonts->OutTextBigCenter ( "Warte auf andere Spieler...",320,235,buffer );
	SHOW_SCREEN
	mouse->SetCursor ( CHand );
	mouse->draw ( false,screen );

	ClientSettingsList=new TList;
	TransmitPlayerUpgrades ( MyPlayer );
	TransmitPlayerLanding ( MyPlayer->Nr,LandX,LandY,LandingList );

	while ( !LetsGo )
	{
		// Events holen:
		SDL_PumpEvents();
		// Die Maus machen:
		mouse->GetPos();
		if ( mouse->x!=lx||mouse->y!=ly )
		{
			mouse->draw ( true,screen );
		}
		lx=mouse->x;
		ly=mouse->y;
		SDL_Delay ( 1 );
		HandleMenuMessages();
	}

	// Alle Landungen durchf�hren:
	game->MakeLanding ( LandX,LandY,MyPlayer,LandingList,options.FixedBridgeHead );
	while ( ClientSettingsList->Count )
	{
		sClientSettings *cs;
		cPlayer *p;
		cs=ClientSettingsList->ClientSettingsItems[0];

		for ( i=0;i<PlayerList->Count;i++ )
		{
			p=PlayerList->PlayerItems[i];
			if ( p->Nr==cs->nr ) break;
		}
		game->MakeLanding ( cs->LandX,cs->LandY,p,cs->LandingList,options.FixedBridgeHead );

		while ( cs->LandingList->Count )
		{
			sLanding *l;
			l=cs->LandingList->LandItems[0];
			delete l;
			cs->LandingList->DeleteLanding ( 0 );
		}
		delete cs;
		ClientSettingsList->DeleteClientSettings ( 0 );
	}
	delete ClientSettingsList;
}

// �bertr�gt alle Upgrades dieses Players:
void cMultiPlayer::TransmitPlayerUpgrades ( cPlayer *p )
{
	string msg;
	char sztmp[256];
	int i;
	sprintf ( sztmp,"%d",p->Nr );
	msg=sztmp;

	for ( i=0;i<VehicleMainData.vehicle_anz;i++ )
	{
		if ( p->VehicleData[i].damage!=VehicleMainData.vehicle[i].data.damage||
		        p->VehicleData[i].max_shots!=VehicleMainData.vehicle[i].data.max_shots||
		        p->VehicleData[i].range!=VehicleMainData.vehicle[i].data.range||
		        p->VehicleData[i].max_ammo!=VehicleMainData.vehicle[i].data.max_ammo||
		        p->VehicleData[i].armor!=VehicleMainData.vehicle[i].data.armor||
		        p->VehicleData[i].max_hit_points!=VehicleMainData.vehicle[i].data.max_hit_points||
		        p->VehicleData[i].scan!=VehicleMainData.vehicle[i].data.scan||
		        p->VehicleData[i].max_speed!=VehicleMainData.vehicle[i].data.max_speed )
		{
			if ( msg.length() >0 ) msg+="#";
			msg+="0";msg+="#";
			sprintf ( sztmp,"%d",i ); msg+=sztmp; msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].damage ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].max_shots ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].range ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].max_ammo ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].armor ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].max_hit_points ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].scan ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->VehicleData[i].max_speed ); msg+=sztmp;
		}
		if ( msg.length() >200 )
		{
			fstcpip->FSTcpIpSend ( MSG_PLAYER_UPGRADES,msg.c_str(), ( int ) msg.length(),10 );
			SDL_Delay ( 1 );
			msg="";
		}
	}

	for ( i=0;i<BuildingMainData.building_anz;i++ )
	{
		if ( p->BuildingData[i].damage!=BuildingMainData.building[i].data.damage||
		        p->BuildingData[i].max_shots!=BuildingMainData.building[i].data.max_shots||
		        p->BuildingData[i].range!=BuildingMainData.building[i].data.range||
		        p->BuildingData[i].max_ammo!=BuildingMainData.building[i].data.max_ammo||
		        p->BuildingData[i].armor!=BuildingMainData.building[i].data.armor||
		        p->BuildingData[i].max_hit_points!=BuildingMainData.building[i].data.max_hit_points||
		        p->BuildingData[i].scan!=BuildingMainData.building[i].data.scan )
		{
			if ( msg.length() >0 ) msg+="#";
			msg+="1";msg+="#";
			sprintf ( sztmp,"%d",i ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].damage ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].max_shots ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].range ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].max_ammo ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].armor ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].max_hit_points ); msg+=sztmp;msg+="#";
			sprintf ( sztmp,"%d",p->BuildingData[i].scan ); msg+=sztmp;msg+="#";
		}
		if ( msg.length() >200 )
		{
			fstcpip->FSTcpIpSend ( MSG_PLAYER_UPGRADES,msg.c_str(), ( int ) msg.length(),10 );
			SDL_Delay ( 1 );
			msg="";
		}
	}
	if ( msg.length() >1 )
	{
		fstcpip->FSTcpIpSend ( MSG_PLAYER_UPGRADES,msg.c_str(), ( int ) msg.length(),10 );
		SDL_Delay ( 1 );
	}
}

// �bertr�gt die Landungsdaten des Players:
void cMultiPlayer::TransmitPlayerLanding ( int nr,int x,int y,TList *ll )
{
	string msg;
	char sztmp[256];
	int i;
	sprintf ( sztmp,"%d",x );msg=sztmp;msg+="#";
	sprintf ( sztmp,"%d",y );msg+=sztmp;msg+="#";
	sprintf ( sztmp,"%d",nr );msg+=sztmp;msg+="#";
	sprintf ( sztmp,"%d",ll->Count );msg+=sztmp;
	for ( i=0;i<ll->Count;i++ )
	{
		sLanding *l;
		l=ll->LandItems[i];
		if ( msg.length() >0 ) msg+="#";
		sprintf ( sztmp,"%d",l->id );msg+=sztmp;msg+="#";
		sprintf ( sztmp,"%d",l->cargo );msg+=sztmp;
		if ( msg.length() >200 )
		{
			fstcpip->FSTcpIpSend ( MSG_PLAYER_LANDING,msg.c_str(), ( int ) msg.length(),10 );
			SDL_Delay ( 1 );
			msg="";
		}
	}
	if ( msg.length() >0 )
	{
		fstcpip->FSTcpIpSend ( MSG_PLAYER_LANDING,msg.c_str(), ( int ) msg.length(),10 );
		SDL_Delay ( 1 );
	}
}

// Pr�ft, ob die Spielerliste ok ist:
bool cMultiPlayer::TestPlayerList ( void )
{
	int i,k;
	for ( i=0;i<PlayerList->Count;i++ )
	{
		for ( k=0;k<PlayerList->Count;k++ )
		{
			if ( i==k ) continue;
			if ( strcmp ( PlayerList->PlayerItems[i]->name.c_str(),PlayerList->PlayerItems[k]->name.c_str() ) ==0 )
			{
				string log;
				log="Der Spielername ";
				log+=PlayerList->PlayerItems[i]->name;
				log+=" ist doppelt vorhanden!";
				AddChatLog ( log );
				return false;
			}
		}
	}
	return true;
}

// Pr�ft, ob alle Spieler aus dem Savegame da sind:
bool cMultiPlayer::TestPlayerListLoad ( void )
{
	int i,k,found;
	if ( SaveGame.empty() ) return false;

	if ( PlayerList->Count>game->PlayerList->Count )
	{
		AddChatLog ( "zu viele Spieler!" );
		return false;
	}
	if ( PlayerList->Count<game->PlayerList->Count )
	{
		AddChatLog ( "zu wenig Spieler!" );
		return false;
	}

	found=0;
	for ( i=0;i<PlayerList->Count;i++ )
	{
		cPlayer *a;
		a=PlayerList->PlayerItems[i];
		for ( k=0;k<game->PlayerList->Count;k++ )
		{
			cPlayer *b;
			b=game->PlayerList->PlayerItems[k];
			if ( strcmp ( b->name.c_str(),a->name.c_str() ) ==0 ) {found++;break;}
		}
	}
	if ( found!=PlayerList->Count )
	{
		AddChatLog ( "ein Spieler hat nicht den richtigen Namen!" );
		return false;
	}

	if ( game->PlayRounds && strcmp ( game->PlayerList->PlayerItems[0]->name.c_str(),MyPlayer->name.c_str() ) )
	{
		string log;
		log="Spieler ";
		log+=game->PlayerList->PlayerItems[0]->name.c_str();
		log+=" muss der Host im Rundenspiel sein!";
		AddChatLog ( log );
		return false;
	}

	return true;
}

// Startet ein Hot-Seat-Spiel:
void HeatTheSeat ( void )
{
	string stmp;
	char sztmp[32];
	// Anzahl der Spieler holen:
	int PlayerAnz;
	PlayerAnz=ShowNumberInput ( "Anzahl der Spieler (>=2):" );
	if ( PlayerAnz<2 ) PlayerAnz=2;

	// Spiel erstellen:
	string MapName;
	MapName=RunPlanetSelect();
	if ( MapName.empty() ) return;

	TList *list,*LandingList;
	int i,LandX,LandY;
	sOptions options;
	cPlayer *p;
	cMap *map;

	map=new cMap;
	if ( !map->LoadMap ( MapName ) )
	{
		delete map;
		return;
	}
	options=RunOptionsMenu ( NULL );

	map->PlaceRessources ( options.metal,options.oil,options.gold,options.dichte );

	list=new TList;
	for ( i=1;i<=PlayerAnz;i++ )
	{
		stmp = "Player";
		sprintf ( sztmp,"%d",i );
		stmp+=sztmp;
		list->AddPlayer ( p=new cPlayer ( stmp,OtherData.colors[ ( i-1 ) %8],i ) );
		p->Credits=options.credits;
	}

	game=new cGame ( NULL, map );
	game->AlienTech=options.AlienTech;
	game->PlayRounds=options.PlayRounds;
	game->ActiveRoundPlayerNr=p->Nr;
	game->Init ( list,0 );

	for ( i=0;i<list->Count;i++ )
	{
		p=list->PlayerItems[i];
		p->InitMaps ( map->size );

		stmp=p->name; stmp+=" ist am Zug.";
		ShowOK ( stmp,true );

		LandingList=new TList;
		RunHangar ( p,LandingList );

		SelectLanding ( &LandX,&LandY,map );
		game->ActivePlayer=p;
		game->MakeLanding ( LandX,LandY,p,LandingList,options.FixedBridgeHead );
		p->HotHud=* ( game->hud );

		while ( LandingList->Count )
		{
			delete LandingList->LandItems[0];
			LandingList->DeleteLanding ( 0 );
		}
		delete LandingList;
	}

	ExitMenu();

	p=list->PlayerItems[0];
	game->ActivePlayer=p;
	* ( game->hud ) =p->HotHud;
	stmp=p->name; stmp+=" ist am Zug.";
	ShowOK ( stmp,true );
	game->HotSeat=true;
	game->HotSeatPlayer=0;
	game->Run();

	SettingsData.sPlayerName=p->name;
	while ( list->Count )
	{
		delete list->PlayerItems[0];
		list->Delete ( 0 );
	}
	delete game;game=NULL;
	delete map;
	delete list;
}

// Zeigt das Laden Men� an:
int ShowDateiMenu ( void )
{
	SDL_Rect scr,dest;
	int LastMouseX=0,LastMouseY=0,LastB=0,x,b,y,offset=0,selected=-1;
	bool FertigPressed=false, UpPressed=false, DownPressed=false;
	bool  HilfePressed=false, LadenPressed=false, Cursor=true;
	TList *files;
	TiXmlDocument doc;
	TiXmlNode* rootnode;
	TiXmlNode* node;

	PlayFX ( SoundData.SNDHudButton );
	mouse->SetCursor ( CHand );
	mouse->draw ( false,buffer );
	// Den Bildschirm blitten:
	SDL_BlitSurface ( GraphicsData.gfx_load_save_menu,NULL,buffer,NULL );
	// Den Text anzeigen:
	fonts->OutTextCenter ( "Laden Men�",320,12,buffer );
	// Buttons setzen;
	PlaceMenuButton ( "Fertig",353,438,2,false );
	PlaceSmallMenuButton ( "? ",464,438,false );
	PlaceMenuButton ( "Laden",514,438,4,false );
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
	if ( !doc.LoadFile ( "saves//saves.xml" ) )
	{
		cLog::write ( "Could not load saves.xml",1 );
		return 0;
	}
	rootnode=doc.FirstChildElement ( "SavesData" )->FirstChildElement ( "SavesList" );

	files = new TList;
	node=rootnode->FirstChildElement();
	if ( node )
		files->Add ( node->ToElement()->Attribute ( "file" ) );
	while ( node )
	{
		node=node->NextSibling();
		if ( node && node->Type() ==1 )
			files->Add ( node->ToElement()->Attribute ( "file" ) );
	}

	ShowFiles ( files,offset,selected );
	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack ( buffer );
	while ( 1 )
	{
		// Events holen:
		SDL_PumpEvents();

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
						selected = i+offset;
					checky+=75;
				}
				ShowFiles ( files,offset,selected );
				SHOW_SCREEN
				mouse->draw ( false,screen );
			}
		}
		// Fertig-Button:
		if ( x>=353&&x<353+109&&y>=438&&y<438+40 )
		{
			if ( b&&!FertigPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				PlaceMenuButton ( "Fertig",353,438,2,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				FertigPressed=true;
			}
			else if ( !b&&LastB )
			{
				return -1;
			}
		}
		else if ( FertigPressed )
		{
			PlaceMenuButton ( "Fertig",353,438,2,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			FertigPressed=false;
		}
		// Laden-Button:
		if ( x>=514&&x<514+109&&y>=438&&y<438+40 )
		{
			if ( b&&!LadenPressed )
			{
				PlayFX ( SoundData.SNDMenuButton );
				PlaceMenuButton ( "Laden",514,438,4,true );
				SHOW_SCREEN
				mouse->draw ( false,screen );
				LadenPressed=true;
			}
			else if ( !b&&LastB )
			{
				PlaceMenuButton ( "Laden",514,438,4,false );
				if ( selected != -1 )
				{
					ShowFiles ( files,offset,selected );
					return 1;
				}
				SHOW_SCREEN
				mouse->draw ( false,screen );
				LadenPressed=false;
			}
		}
		else if ( LadenPressed )
		{
			PlaceMenuButton ( "Laden",514,438,4,false );
			SHOW_SCREEN
			mouse->draw ( false,screen );
			LadenPressed=false;
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
				ShowFiles ( files,offset,selected );
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
				ShowFiles ( files,offset,selected );
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
	return -1;
}

// Zeigt die Saves an
void ShowFiles ( TList *files, int offset, int selected )
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
	for ( i = offset; i < 10+offset; i++ )
	{
		if ( i == offset+5 )
		{
			x+=402;
			y=87;
		}
		if ( i>=files->Count && i != selected )
		{
			y+=76;
			continue;
		}
		if ( i != selected || ( selected < files->Count && selected != -1 ) )
		{
			filename = files->Items[i];
			// Dateinamen anpassen und ausgeben
			filename.erase ( filename.length()-4 );
			if ( filename.length() > 15 )
				filename.erase ( 15 );
		}
		else
		{
			if ( InputStr.length() > 15 )
				InputStr.erase ( 15 );
			filename = InputStr;
		}
		if ( i == selected )
			LoadFile = filename;
		fonts->OutText ( ( char * ) filename.c_str(),x,y,buffer );
		y+=76;
	}
}
