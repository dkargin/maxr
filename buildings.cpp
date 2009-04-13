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
#include "buttons.h"
#include "math.h"
#include "buildings.h"
#include "main.h"
#include "unifonts.h"
#include "mouse.h"
#include "files.h"
#include "pcx.h"
#include "events.h"
#include "serverevents.h"
#include "client.h"
#include "server.h"
#include "upgradecalculator.h"

//--------------------------------------------------------------------------
int& sMineValues::GetProd(ResourceKind const resource)
{
	switch (resource)
	{
		case TYPE_METAL: return iMetalProd;
		case TYPE_OIL:   return iOilProd;
		case TYPE_GOLD:  return iGoldProd;

		default: throw std::logic_error("Invalid resource kind");
	}
}

//--------------------------------------------------------------------------
int sMineValues::GetMaxProd(ResourceKind const resource) const
{
	switch (resource)
	{
		case TYPE_METAL: return iMaxMetalProd;
		case TYPE_OIL:   return iMaxOilProd;
		case TYPE_GOLD:  return iMaxGoldProd;

		default: throw std::logic_error("Invalid resource kind");
	}
}


//--------------------------------------------------------------------------
/** struct for the upgrade list */
//--------------------------------------------------------------------------
struct sUpgradeStruct
{
public:
	sUpgradeStruct(SDL_Surface* const sf_, bool const vehicle_, int const id_) 
	: sf(sf_)
	, vehicle(vehicle_)
	, id(id_)
	{}

	SDL_Surface* const sf;
	bool const vehicle;
	int const id;
	sUpgradeNew upgrades[8];
};


//--------------------------------------------------------------------------
// cBuilding Implementation
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
cBuilding::cBuilding ( sBuilding *b, cPlayer *Owner, cBase *Base )
{
	PosX = 0;
	PosY = 0;
	RubbleTyp = 0;
	RubbleValue = 0;
	EffectAlpha = 0;
	EffectInc = true;
	dir = 0;
	StartUp = 0;
	IsWorking = false;
	researchArea = cResearch::kAttackResearch;
	IsLocked = false;
	typ = b;
	owner = Owner;
	base = Base;

	if ( Owner == NULL || b == NULL )
	{
		if ( b != NULL )
		{
			data = b->data;
		}
		else
		{
			memset ( &data, 0, sizeof ( sUnitData ) );
		}

		BuildList = NULL;

		bSentryStatus = false;
		return;
	}

	data = owner->BuildingData[typ->nr];

	selected = false;
	MenuActive = false;
	AttackMode = false;
	Transfer = false;
	BaseN = false;
	BaseBN = false;
	BaseE = false;
	BaseBE = false;
	BaseS = false;
	BaseBS = false;
	BaseW = false;
	BaseBW = false;
	Attacking = false;
	LoadActive = false;
	ActivatingVehicle = false;
	bIsBeeingAttacked = false;
	RepeatBuild = false;
	hasBeenAttacked = false;

	if ( data.can_attack != ATTACK_NONE ) bSentryStatus = true;
	else bSentryStatus = false;
	
	MetalProd = 0;
	GoldProd = 0;
	OilProd = 0;
	MaxMetalProd = 0;
	MaxGoldProd = 0;
	MaxOilProd = 0;
	Disabled = 0;
	data.hit_points = data.max_hit_points;
	data.ammo = data.max_ammo;
	SubBase = NULL;
	BuildSpeed = 0;
	BuildList = NULL;

	if ( data.can_build )
		BuildList = new cList<sBuildList*>;

	if ( data.is_big )
	{
		DamageFXPointX  = random(64) + 32;
		DamageFXPointY  = random(64) + 32;
		DamageFXPointX2 = random(64) + 32;
		DamageFXPointY2 = random(64) + 32;
	}
	else
	{
		DamageFXPointX = random(64 - 24);
		DamageFXPointY = random(64 - 24);
	}

	refreshData();
}

//--------------------------------------------------------------------------
cBuilding::~cBuilding ()
{
	if ( BuildList )
	{
		while (BuildList->Size())
		{
			sBuildList *ptr;
			ptr = (*BuildList)[0];
			delete ptr;
			BuildList->Delete( 0 );
		}
		delete BuildList;
	}

	while (StoredVehicles.Size())
	{
		cVehicle *v;
		v = StoredVehicles[0];

		if ( v->prev )
		{
			cVehicle *vp;
			vp = v->prev;
			vp->next = v->next;

			if ( v->next )
				v->next->prev = vp;
		}
		else
		{
			v->owner->VehicleList = v->next;

			if ( v->next )
				v->next->prev = NULL;
		}
		delete v;

		StoredVehicles.Delete ( 0 );
	}

	if ( bSentryStatus )
	{
		owner->deleteSentryBuilding ( this );
	}

	if ( IsLocked )
	{
		cPlayer *p;

		for (unsigned int i = 0; i < Client->PlayerList->Size(); i++)
		{
			p = (*Client->PlayerList)[i];
			p->DeleteLock ( this );
		}
	}
	if ( Server )
	{
		for ( unsigned int i = 0; i < Server->AJobs.Size(); i++ )
		{
			if ( Server->AJobs[i]->building == this ) Server->AJobs[i]->building = NULL;
		}
	}
	if ( Client )
	{
		for ( unsigned int i = 0; i < Client->attackJobs.Size(); i++ )
		{
			if ( Client->attackJobs[i]->building == this ) Client->attackJobs[i]->building = NULL;
		}
	}
}

//----------------------------------------------------
/** Returns a string with the current state */
//----------------------------------------------------
string cBuilding::getStatusStr ()
{
	if ( IsWorking )
	{
		// Factory:
		if (data.can_build && BuildList && BuildList->Size() && owner == Client->ActivePlayer)
		{
			sBuildList *ptr;
			ptr = (*BuildList)[0];

			if ( ptr->metall_remaining > 0 )
			{
				string sText;
				int iRound;

				iRound = ( int ) ceil ( ptr->metall_remaining / ( double ) MetalPerRound );
				sText = lngPack.i18n ( "Text~Comp~Producing" ) + ": ";
				sText += ( string ) owner->VehicleData[ptr->typ->nr].name + " (";
				sText += iToStr ( iRound ) + ")";

				if ( font->getTextWide ( sText, FONT_LATIN_SMALL_WHITE ) > 126 )
				{
					sText = lngPack.i18n ( "Text~Comp~Producing" ) + ":\n";
					sText += ( string ) owner->VehicleData[ptr->typ->nr].name + " (";
					sText += iToStr ( iRound ) + ")";
				}

				return sText;
			}
			else
			{
				return lngPack.i18n ( "Text~Comp~Producing_Fin" );
			}
		}

		// Research Center
		if (data.can_research && owner == Client->ActivePlayer)
		{
			string sText = lngPack.i18n ( "Text~Comp~Working" ) + "\n";
			for (int area = 0; area < cResearch::kNrResearchAreas; area++)
			{
				if (owner->researchCentersWorkingOnArea[area] > 0)
				{
					switch (area)
					{
						case cResearch::kAttackResearch: sText += lngPack.i18n ( "Text~Vehicles~Damage" ); break;
						case cResearch::kShotsResearch: sText += lngPack.i18n ( "Text~Hud~Shots" ); break;
						case cResearch::kRangeResearch: sText += lngPack.i18n ( "Text~Hud~Range" ); break;
						case cResearch::kArmorResearch: sText += lngPack.i18n ( "Text~Vehicles~Armor" ); break;
						case cResearch::kHitpointsResearch: sText += lngPack.i18n ( "Text~Hud~Hitpoints" ); break;
						case cResearch::kSpeedResearch: sText += lngPack.i18n ( "Text~Hud~Speed" ); break;
						case cResearch::kScanResearch: sText += lngPack.i18n ( "Text~Hud~Scan" ); break;
						case cResearch::kCostResearch: sText += lngPack.i18n ( "Text~Vehicles~Costs" ); break;
					}
					sText += ": " + iToStr (owner->researchLevel.getRemainingTurns (area, owner->researchCentersWorkingOnArea[area])) + "\n";
				}
			}
			return sText;
		}

		// Goldraffinerie:
		if ( data.gold_need && owner == Client->ActivePlayer )
		{
			string sText;
			sText = lngPack.i18n ( "Text~Comp~Working" ) + "\n";
			sText += lngPack.i18n ( "Text~Title~Credits" ) + ": ";
			sText += iToStr ( owner->Credits );
			return sText.c_str();
		}

		return lngPack.i18n ( "Text~Comp~Working" );
	}

	if ( Disabled )
	{
		string sText;
		sText = lngPack.i18n ( "Text~Comp~Disabled" ) + " (";
		sText += iToStr ( Disabled ) + ")";
		return sText.c_str();
	}
	if ( bSentryStatus )
	{
		return lngPack.i18n ( "Text~Comp~Sentry" );
	}

	return lngPack.i18n ( "Text~Comp~Waits" );
}

//--------------------------------------------------------------------------
/** Refreshs all data to the maximum values */
//--------------------------------------------------------------------------
int cBuilding::refreshData ()
{
	if ( data.shots < data.max_shots )
	{
		if ( data.ammo >= data.max_shots )
			data.shots = data.max_shots;
		else
			data.shots = data.ammo;
		return 1;
	}
	return 0;
}

//--------------------------------------------------------------------------
/** generates the name for the building depending on the versionnumber */
//--------------------------------------------------------------------------
void cBuilding::GenerateName ()
{
	string rome, tmp_name;
	int nr, tmp;
	string::size_type tmp_name_idx;
	rome = "";
	nr = data.version;

	// generate the roman versionnumber (correct until 899)

	if ( nr > 100 )
	{
		tmp = nr / 100;
		nr %= 100;

		while ( tmp-- )
			rome += "C";
	}

	if ( nr >= 90 )
	{
		rome += "XC";
		nr -= 90;
	}

	if ( nr >= 50 )
	{
		nr -= 50;
		rome += "L";
	}

	if ( nr >= 40 )
	{
		nr -= 40;
		rome += "XL";
	}

	if ( nr >= 10 )
	{
		tmp = nr / 10;
		nr %= 10;

		while ( tmp-- )
			rome += "X";
	}

	if ( nr == 9 )
	{
		nr -= 9;
		rome += "IX";
	}

	if ( nr >= 5 )
	{
		nr -= 5;
		rome += "V";
	}

	if ( nr == 4 )
	{
		nr -= 4;
		rome += "IV";
	}

	while ( nr-- )
		rome += "I";

	// concatenate the name
	// name=(string)data.name + " MK "+rome;
/*
	name = ( string ) data.name;

	name += " MK ";

	name += rome;
*/
	if ( name.length() == 0 )
	{
		// prefix
		name = "MK ";
		name += rome;
		name += " ";
		// object name
		name += ( string ) data.name;
	}
	else
	{
		// check for MK prefix
		tmp_name = name.substr(0,2);
		if ( 0 == (int)tmp_name.compare("MK") )
		{
			// current name, without prefix
			tmp_name_idx = name.find_first_of(" ", 4 );
			if( tmp_name_idx != string::npos )
			{
				tmp_name = ( string )name.substr(tmp_name_idx);
				// prefix
				name = "MK ";
				name += rome;
				// name
				name += tmp_name;
			}
			else
			{
				tmp_name = name;
				// prefix
				name = "MK ";
				name += rome;
				name += " ";
				// name
				name += tmp_name;
			}
		}
		else
		{
			tmp_name = name;
			name = "MK ";
			name += rome;
			name += " ";
			name += tmp_name;
		}
	}
}

//--------------------------------------------------------------------------
void cBuilding::Draw ( SDL_Rect *screenPos )
{
	SDL_Rect src, tmp, dest;
	src.x = 0;
	src.y = 0;
	float factor = (float)(Client->Hud.Zoom/64.0);

	// draw the damage effects
	if ( Client->iTimer1 && !data.is_base && !data.is_connector && data.hit_points < data.max_hit_points && SettingsData.bDamageEffects && ( owner == Client->ActivePlayer || Client->ActivePlayer->ScanMap[PosX+PosY*Client->Map->size] ) )
	{
		int intense = ( int ) ( 200 - 200 * ( ( float ) data.hit_points / data.max_hit_points ) );
		Client->addFX ( fxDarkSmoke, PosX*64 + DamageFXPointX, PosY*64 + DamageFXPointY, intense );

		if ( data.is_big && intense > 50 )
		{
			intense -= 50;
			Client->addFX ( fxDarkSmoke, PosX*64 + DamageFXPointX2, PosY*64 + DamageFXPointY2, intense );
		}
	}

	dest.x = dest.y = 0;
	bool bDraw = false;
	SDL_Surface* drawingSurface = Client->bCache.getCachedImage(this);
	if ( drawingSurface == NULL )
	{
		//no cached image found. building needs to be redrawn.
		bDraw = true;
		drawingSurface = Client->bCache.createNewEntry(this);
	}

	if ( drawingSurface == NULL )
	{
		//image will not be cached. So blitt directly to the screen buffer.
		dest = *screenPos;
		drawingSurface = buffer;
	}
		
	if ( bDraw )
	{
		// check, if it is dirt:
		if ( !owner )
		{
			if ( data.is_big )
			{
				if ( !UnitsData.dirt_big ) return;
				src.w = src.h = (int)(UnitsData.dirt_big_org->h*factor);
			}
			else
			{
				if ( !UnitsData.dirt_small ) return;
				src.w = src.h = (int)(UnitsData.dirt_small_org->h*factor);
			}

			src.x = src.w * RubbleTyp;

			src.y = 0;

			// draw the shadows
			if ( SettingsData.bShadows )
			{
				if ( data.is_big )
				{
					CHECK_SCALING( UnitsData.dirt_big_shw, UnitsData.dirt_big_shw_org, factor );
					SDL_BlitSurface ( UnitsData.dirt_big_shw, &src, drawingSurface, &tmp );
				}
				else
				{
					CHECK_SCALING( UnitsData.dirt_small_shw, UnitsData.dirt_small_shw_org, factor );
					SDL_BlitSurface ( UnitsData.dirt_small_shw, &src, drawingSurface, &tmp );
				}
			}

			// draw the building
			tmp = dest;

			if ( data.is_big )
			{
				CHECK_SCALING( UnitsData.dirt_big, UnitsData.dirt_big_org, factor);
				SDL_BlitSurface ( UnitsData.dirt_big, &src, drawingSurface, &tmp );
			}
			else
			{
				CHECK_SCALING( UnitsData.dirt_small, UnitsData.dirt_small_org, factor);
				SDL_BlitSurface ( UnitsData.dirt_small, &src, drawingSurface, &tmp );
			}

			return;
		}

		// read the size:
		if ( data.has_frames )
		{
			src.w = Client->Hud.Zoom;
			src.h = Client->Hud.Zoom;
		}
		else
		{
			src.w = (int)(typ->img_org->w*factor);
			src.h = (int)(typ->img_org->h*factor);
		}
		
		// draw the concrete
		tmp = dest;
		if ( !data.build_on_water && !data.is_expl_mine )
		{
			if ( data.is_big )
			{
				CHECK_SCALING( GraphicsData.gfx_big_beton, GraphicsData.gfx_big_beton_org, factor);

				if ( StartUp && SettingsData.bAlphaEffects )
					SDL_SetAlpha ( GraphicsData.gfx_big_beton, SDL_SRCALPHA, StartUp );
				else
					SDL_SetAlpha ( GraphicsData.gfx_big_beton, SDL_SRCALPHA, 255 );
				
				SDL_BlitSurface ( GraphicsData.gfx_big_beton, NULL, drawingSurface, &tmp );
			}
			else
			{
				CHECK_SCALING( UnitsData.ptr_small_beton, UnitsData.ptr_small_beton_org, factor);
				if ( !data.is_road && !data.is_connector )
				{
					if ( StartUp && SettingsData.bAlphaEffects )
						SDL_SetAlpha ( UnitsData.ptr_small_beton, SDL_SRCALPHA, StartUp );
					else
						SDL_SetAlpha ( UnitsData.ptr_small_beton, SDL_SRCALPHA, 255 );

					SDL_BlitSurface ( UnitsData.ptr_small_beton, NULL, drawingSurface, &tmp );
					SDL_SetAlpha ( UnitsData.ptr_small_beton, SDL_SRCALPHA, 255 );
				}
			}
		}

		tmp = dest;

		// draw the connector slots:
		if ( this->SubBase && !StartUp )
		{
			DrawConnectors (  drawingSurface, dest );
		}

		// draw the shadows
		if ( SettingsData.bShadows && !data.is_connector )
		{
			if ( StartUp && SettingsData.bAlphaEffects ) 
				SDL_SetAlpha ( typ->shw, SDL_SRCALPHA, StartUp / 5 );
			else
				SDL_SetAlpha ( typ->shw, SDL_SRCALPHA, 50 );

			CHECK_SCALING( typ->shw, typ->shw_org, factor);
			blittShadow ( typ->shw, NULL, drawingSurface, &tmp );
			//SDL_BlitSurface ( typ->shw, NULL, drawingSurface, &tmp );
		}

		// blit the players color
		if ( !data.is_road && !data.is_connector )
		{
			SDL_BlitSurface ( owner->color, NULL, GraphicsData.gfx_tmp, NULL );

			if ( data.has_frames )
			{
				if ( data.is_annimated && SettingsData.bAnimations && !Disabled )
				{
					src.x = ( Client->iFrame % data.has_frames ) * Client->Hud.Zoom;
				}
				else
				{
					src.x = dir * Client->Hud.Zoom;
				}

				CHECK_SCALING( typ->img, typ->img_org, factor);
				SDL_BlitSurface ( typ->img, &src, GraphicsData.gfx_tmp, NULL );

				src.x = 0;
			}
			else
			{
				CHECK_SCALING( typ->img, typ->img_org, factor);
				SDL_BlitSurface ( typ->img, NULL, GraphicsData.gfx_tmp, NULL );
			}
		}
		else if ( !data.is_connector )
		{
			SDL_FillRect ( GraphicsData.gfx_tmp, NULL, 0xFF00FF );

			CHECK_SCALING( typ->img, typ->img_org, factor);
			SDL_BlitSurface ( typ->img, NULL, GraphicsData.gfx_tmp, NULL );
		}

		// draw the building 
		tmp = dest;

		src.x = 0;
		src.y = 0;
		
		if ( !data.is_connector )
		{
			if ( StartUp && SettingsData.bAlphaEffects )
			{
				SDL_SetAlpha ( GraphicsData.gfx_tmp, SDL_SRCALPHA, StartUp );
				SDL_BlitSurface ( GraphicsData.gfx_tmp, &src, drawingSurface, &tmp );
				SDL_SetAlpha ( GraphicsData.gfx_tmp, SDL_SRCALPHA, 255 );
			}
			else
				SDL_BlitSurface ( GraphicsData.gfx_tmp, &src, drawingSurface, &tmp );
		}
	}

	//now check, whether the image has to be blitted to screen buffer
	if ( drawingSurface != buffer )
	{
		tmp = *screenPos;
		SDL_BlitSurface( drawingSurface, NULL, buffer, &tmp );

		//all folling graphic operations are drawn directly to buffer
		dest = *screenPos;
	}


	if ( StartUp )
	{
		if ( Client->iTimer0 )
			StartUp += 25;

		if ( StartUp >= 255 )
			StartUp = 0;
	}

	// draw the effect if necessary
	if ( data.has_effect && SettingsData.bAnimations && ( IsWorking || !data.can_work ) )
	{
		tmp = dest;
		SDL_SetAlpha ( typ->eff, SDL_SRCALPHA, EffectAlpha );

		CHECK_SCALING( typ->eff, typ->eff_org, factor);
		SDL_BlitSurface ( typ->eff, NULL, buffer, &tmp );

		if ( Client->iTimer0 )
		{
			if ( EffectInc )
			{
				EffectAlpha += 30;

				if ( EffectAlpha > 220 )
				{
					EffectAlpha = 255;
					EffectInc = false;
				}
			}
			else
			{
				EffectAlpha -= 30;

				if ( EffectAlpha < 30 )
				{
					EffectAlpha = 0;
					EffectInc = true;
				}
			}
		}
	}

	// draw the mark, when a build order is finished 
	if (BuildList && BuildList->Size() && !IsWorking && (*BuildList)[0]->metall_remaining <= 0 && owner == Client->ActivePlayer)
	{
		SDL_Rect d, t;
		int max, nr;
		nr = 0xFF00 - ( ( Client->iFrame % 0x8 ) * 0x1000 );
		max = ( Client->Hud.Zoom - 2 ) * 2;
		d.x = dest.x + 2;
		d.y = dest.y + 2;
		d.w = max;
		d.h = 1;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.y += max - 1;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.y = dest.y + 2;
		d.w = 1;
		d.h = max;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.x += max - 1;
		SDL_FillRect ( buffer, &d, nr );
	}

	// draw a colored frame if necessary
	if ( Client->Hud.Farben )
	{
		SDL_Rect d, t;
		int nr = *((unsigned int*)(owner->color->pixels));
		int max = data.is_big ? (Client->Hud.Zoom - 1) * 2 : Client->Hud.Zoom - 1;

		d.x = dest.x + 1;
		d.y = dest.y + 1;
		d.w = max;
		d.h = 1;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.y += max - 1;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.y = dest.y + 1;
		d.w = 1;
		d.h = max;
		t = d;
		SDL_FillRect ( buffer, &d, nr );
		d = t;
		d.x += max - 1;
		SDL_FillRect ( buffer, &d, nr );
	}

	// draw a colored frame if necessary
	if ( selected )
	{
		SDL_Rect d, t;
		int max = data.is_big ? Client->Hud.Zoom * 2 : Client->Hud.Zoom;
		int len = max / 4;

		d.x = dest.x + 1;
		d.y = dest.y + 1;
		d.w = len;
		d.h = 1;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.x += max - len - 1;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.y += max - 2;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.x = dest.x + 1;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.y = dest.y + 1;
		d.w = 1;
		d.h = len;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.x += max - 2;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.y += max - len - 1;
		t = d;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
		d = t;
		d.x = dest.x + 1;
		SDL_FillRect ( buffer, &d, Client->iBlinkColor );
	}

	//draw health bar
	if ( Client->Hud.Treffer )
		DrawHelthBar();

	//draw ammo bar
	if ( Client->Hud.Munition && data.can_attack && data.max_ammo > 0 )
		DrawMunBar();

	//draw status
	if ( Client->Hud.Status )
		drawStatus();

	//attack job debug output
	if ( Client->bDebugAjobs )
	{
		cBuilding* serverBuilding = NULL;
		if ( Server ) serverBuilding = Server->Map->fields[PosX + PosY*Server->Map->size].getBuildings();
		if ( bIsBeeingAttacked ) font->showText(dest.x + 1,dest.y + 1, "C: attacked", FONT_LATIN_SMALL_WHITE );
		if ( serverBuilding && serverBuilding->bIsBeeingAttacked ) font->showText(dest.x + 1,dest.y + 9, "S: attacked", FONT_LATIN_SMALL_YELLOW );
		if ( Attacking ) font->showText(dest.x + 1,dest.y + 17, "C: attacking", FONT_LATIN_SMALL_WHITE );
		if ( serverBuilding && serverBuilding->Attacking ) font->showText(dest.x + 1,dest.y + 25, "S: attacking", FONT_LATIN_SMALL_YELLOW );
	}
}

//--------------------------------------------------------------------------
/** Get the numer of menu items */
//--------------------------------------------------------------------------
int cBuilding::GetMenuPointAnz ()
{
	int nr = 2;

	if ( !typ->data.is_road )
		nr++;

	if ( typ->data.can_build )
		nr++;

	if ( typ->data.can_load == TRANS_METAL || typ->data.can_load == TRANS_OIL || typ->data.can_load == TRANS_GOLD )
		nr++;

	if ( typ->data.can_attack && data.shots )
		nr++;

	if ( typ->data.can_work && ( IsWorking || typ->data.can_build == BUILD_NONE ) )
		nr++;

	if ( typ->data.is_mine )
		nr++;

	if ( bSentryStatus || data.can_attack )
		nr++;

	if ( typ->data.can_load == TRANS_VEHICLES || typ->data.can_load == TRANS_MEN || typ->data.can_load == TRANS_AIR )
		nr += 2;

	if ( typ->data.can_research && IsWorking )
		nr++;

	if ( data.version != owner->BuildingData[typ->nr].version && SubBase && SubBase->Metal >= 2 )
		nr += 2;

	if ( data.gold_need )
		nr++;

	return nr;
}

//--------------------------------------------------------------------------
/** Returns the size and position of the menu */
//--------------------------------------------------------------------------
SDL_Rect cBuilding::GetMenuSize ()
{
	SDL_Rect dest;
	int i, size;
	dest.x = GetScreenPosX();
	dest.y = GetScreenPosY();
	dest.h = i = GetMenuPointAnz() * 22;
	dest.w = 42;
	size = Client->Hud.Zoom;

	if ( data.is_big )
		size *= 2;

	if ( dest.x + size + 42 >= SettingsData.iScreenW - 12 )
		dest.x -= 42;
	else
		dest.x += size;

	if ( dest.y - ( i - size ) / 2 <= 24 )
	{
		dest.y -= ( i - size ) / 2;
		dest.y += - ( dest.y - 24 );
	}
	else if ( dest.y - ( i - size ) / 2 + i >= SettingsData.iScreenH - 24 )
	{
		dest.y -= ( i - size ) / 2;
		dest.y -= ( dest.y + i ) - ( SettingsData.iScreenH - 24 );
	}
	else
		dest.y -= ( i - size ) / 2;

	return dest;
}

//--------------------------------------------------------------------------
/** returns true, if the mouse coordinates are inside the menu area */
//--------------------------------------------------------------------------
bool cBuilding::MouseOverMenu (int mx, int my)
{
	SDL_Rect r;
	r = GetMenuSize();

	if ( mx < r.x || mx > r.x + r.w )
		return false;

	if ( my < r.y || my > r.y + r.h )
		return false;

	return true;
}

//--------------------------------------------------------------------------
/** displays the self destruction menu */
//--------------------------------------------------------------------------
void cBuilding::SelfDestructionMenu ()
{
	if (showSelfdestruction())
	{
		//TODO: self destruction
	}
}

//--------------------------------------------------------------------------
// Shows the details with big symbols:
//--------------------------------------------------------------------------
void cBuilding::ShowBigDetails ()
{
#define DIALOG_W 640
#define DIALOG_H 480
#define COLUMN_1 dest.x+27
#define COLUMN_2 dest.x+42
#define COLUMN_3 dest.x+95
#define DOLINEBREAK dest.y = y + 14; SDL_FillRect ( buffer, &dest, 0xFC0000 ); y += 19;

	SDL_Rect dest = { MENU_OFFSET_X + 16, MENU_OFFSET_Y, 242, 1 };
	int y;
	y = dest.y + 297;

	if ( data.can_attack )
	{
		// Damage:
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.damage ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Damage" ) );
		DrawSymbolBig ( SBAttack, COLUMN_3, y - 3, 160, data.damage, typ->data.damage, buffer );
		DOLINEBREAK

		if ( !data.is_expl_mine )
		{
			// Shots:
			font->showTextCentered ( COLUMN_1, y, iToStr ( data.max_shots ) );
			font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Shoots" ) );
			DrawSymbolBig ( SBShots, COLUMN_3, y + 2, 160, data.max_shots, typ->data.max_shots, buffer );
			DOLINEBREAK

			// Range:
			font->showTextCentered ( COLUMN_1, y, iToStr ( data.range ) );
			font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Range" ) );
			DrawSymbolBig ( SBRange, COLUMN_3, y - 2, 160, data.range, typ->data.range, buffer );
			DOLINEBREAK

			// Ammo:
			font->showTextCentered ( COLUMN_1, y, iToStr ( data.max_ammo ) );
			font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Ammo" ) );
			DrawSymbolBig ( SBAmmo, COLUMN_3, y - 2, 160, data.max_ammo, typ->data.max_ammo, buffer );
			DOLINEBREAK
		}
	}

	if ( data.can_load == TRANS_METAL || data.can_load == TRANS_OIL || data.can_load == TRANS_GOLD )
	{
		// Metall:
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.max_cargo ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Cargo" ) );

		switch ( data.can_load )
		{

			case TRANS_METAL:
				DrawSymbolBig ( SBMetal, COLUMN_3, y - 2, 160, data.max_cargo, typ->data.max_cargo, buffer );
				break;

			case TRANS_OIL:
				DrawSymbolBig ( SBOil, COLUMN_3, y - 2, 160, data.max_cargo, typ->data.max_cargo, buffer );
				break;

			case TRANS_GOLD:
				DrawSymbolBig ( SBGold, COLUMN_3, y - 2, 160, data.max_cargo, typ->data.max_cargo, buffer );
				break;
		}

		DOLINEBREAK
	}

	if ( data.energy_prod )
	{
		// Energieproduktion:
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.energy_prod ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Produce" ) );
		DrawSymbolBig ( SBEnergy, COLUMN_3, y - 2, 160, data.energy_prod, typ->data.energy_prod, buffer );
		DOLINEBREAK

		// Verbrauch:
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.oil_need ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Usage" ) );
		DrawSymbolBig ( SBOil, COLUMN_3, y - 2, 160, data.oil_need, typ->data.oil_need, buffer );
		DOLINEBREAK
	}

	if ( data.human_prod )
	{
		// Humanproduktion:
		font->showText ( COLUMN_1, y, iToStr ( data.human_prod ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Produce" ) );
		DrawSymbolBig ( SBHuman, COLUMN_3, y - 2, 160, data.human_prod, typ->data.human_prod, buffer );
		DOLINEBREAK
	}

	// Armor:
	font->showTextCentered ( COLUMN_1, y, iToStr ( data.armor ) );
	font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Armor" ) );
	DrawSymbolBig ( SBArmor, COLUMN_3, y - 2, 160, data.armor, typ->data.armor, buffer );
	DOLINEBREAK

	// Hitpoints:
	font->showTextCentered ( COLUMN_1, y, iToStr ( data.max_hit_points ) );
	font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Hitpoints" ) );
	DrawSymbolBig ( SBHits, COLUMN_3, y - 1, 160, data.max_hit_points, typ->data.max_hit_points, buffer );
	DOLINEBREAK

	// Scan:
	if ( data.scan )
	{
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.scan ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Scan" ) );
		DrawSymbolBig ( SBScan, COLUMN_3, y - 2, 160, data.scan, typ->data.scan, buffer );
		DOLINEBREAK

	}

	// energy consumption:
	if ( data.energy_need )
	{
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.energy_need ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Usage" ) );
		DrawSymbolBig ( SBEnergy, COLUMN_3, y - 2, 160, data.energy_need, typ->data.energy_need, buffer );
		DOLINEBREAK

	}

	// humans needed:
	if ( data.human_need )
	{
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.human_need ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Produce" ) );
		DrawSymbolBig ( SBHuman, COLUMN_3, y - 2, 160, data.human_need, typ->data.human_need, buffer );
		DOLINEBREAK

	}

	// raw material consumption:
	if ( data.metal_need )
	{
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.metal_need ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Usage" ) );
		DrawSymbolBig ( SBMetal, COLUMN_3, y - 2, 160, data.metal_need, typ->data.metal_need, buffer );
		DOLINEBREAK

	}

	// gold consumption:
	if ( data.gold_need )
	{
		font->showTextCentered ( COLUMN_1, y, iToStr ( data.gold_need ) );
		font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Usage" ) );
		DrawSymbolBig ( SBGold, COLUMN_3, y - 2, 160, data.gold_need, typ->data.gold_need, buffer );
		DOLINEBREAK

	}

	// Costs:
	font->showTextCentered ( COLUMN_1, y, iToStr ( data.iBuilt_Costs ) );
	font->showText ( COLUMN_2, y, lngPack.i18n ( "Text~Vehicles~Costs" ) );
	DrawSymbolBig ( SBMetal, COLUMN_3, y - 2, 160, data.iBuilt_Costs, typ->data.iBuilt_Costs, buffer );
}

//--------------------------------------------------------------------------
void cBuilding::updateNeighbours (cMap *Map)
{
	int iPosOff = PosX+PosY*Map->size;
	if ( !data.is_big )
	{
		owner->base.checkNeighbour ( iPosOff-Map->size, this );
		owner->base.checkNeighbour ( iPosOff+1, this );
		owner->base.checkNeighbour ( iPosOff+Map->size, this );
		owner->base.checkNeighbour ( iPosOff-1, this );
	}
	else
	{
		owner->base.checkNeighbour ( iPosOff-Map->size, this );
		owner->base.checkNeighbour ( iPosOff-Map->size+1, this );
		owner->base.checkNeighbour ( iPosOff+2, this );
		owner->base.checkNeighbour ( iPosOff+2+Map->size, this );
		owner->base.checkNeighbour ( iPosOff+Map->size*2, this );
		owner->base.checkNeighbour ( iPosOff+Map->size*2+1, this );
		owner->base.checkNeighbour ( iPosOff-1, this );
		owner->base.checkNeighbour ( iPosOff-1+Map->size, this );
	}
	CheckNeighbours( Map );
}

//--------------------------------------------------------------------------
/** Checks, if there are neighbours */
//--------------------------------------------------------------------------
void cBuilding::CheckNeighbours ( cMap *Map )
{
#define CHECK_NEIGHBOUR(x,y,m)								\
	if(x >= 0 && x < Map->size && y >= 0 && y < Map->size ) \
	{														\
		cBuilding* b = Map->fields[(x) + (y) * Map->size].getTopBuilding();		\
		if ( b && b->owner == owner && b->SubBase )			\
			{m=true;}else{m=false;}							\
	}														\

	if ( !data.is_big )
	{
		CHECK_NEIGHBOUR ( PosX    , PosY - 1, BaseN )
		CHECK_NEIGHBOUR ( PosX + 1, PosY    , BaseE )
		CHECK_NEIGHBOUR ( PosX    , PosY + 1, BaseS )
		CHECK_NEIGHBOUR ( PosX - 1, PosY    , BaseW )
	}
	else
	{
		
		CHECK_NEIGHBOUR ( PosX    , PosY - 1, BaseN  )
		CHECK_NEIGHBOUR ( PosX + 1, PosY - 1, BaseBN )
		CHECK_NEIGHBOUR ( PosX + 2, PosY    , BaseE  )
		CHECK_NEIGHBOUR ( PosX + 2, PosY + 1, BaseBE )
		CHECK_NEIGHBOUR ( PosX    , PosY + 2, BaseS  )
		CHECK_NEIGHBOUR ( PosX + 1, PosY + 2, BaseBS )
		CHECK_NEIGHBOUR ( PosX - 1, PosY    , BaseW  )
		CHECK_NEIGHBOUR ( PosX - 1, PosY + 1, BaseBW )
	}
}

//--------------------------------------------------------------------------
/** Draws the connectors at the building: */
//--------------------------------------------------------------------------
void cBuilding::DrawConnectors ( SDL_Surface* surface, SDL_Rect dest )
{
	SDL_Rect src, temp;
	int zoom = Client->Hud.Zoom;
	float factor = (float)(zoom/64.0);

	CHECK_SCALING( UnitsData.ptr_connector, UnitsData.ptr_connector_org, factor);
	CHECK_SCALING( UnitsData.ptr_connector_shw, UnitsData.ptr_connector_shw_org, factor);

	src.y = 0;
	src.x = 0;
	src.h = src.w = UnitsData.ptr_connector->h;
	
	if ( !data.is_big )
	{
		if      (  BaseN &&  BaseE &&  BaseS &&  BaseW ) src.x = 15;
		else if (  BaseN &&  BaseE &&  BaseS && !BaseW ) src.x = 13; 
		else if (  BaseN &&  BaseE && !BaseS &&  BaseW ) src.x = 12;
		else if (  BaseN &&  BaseE && !BaseS && !BaseW ) src.x =  8;
		else if (  BaseN && !BaseE &&  BaseS &&  BaseW ) src.x = 11;
		else if (  BaseN && !BaseE &&  BaseS && !BaseW ) src.x =  5;
		else if (  BaseN && !BaseE && !BaseS &&  BaseW ) src.x =  7;
		else if (  BaseN && !BaseE && !BaseS && !BaseW ) src.x =  1;
		else if ( !BaseN &&  BaseE &&  BaseS &&  BaseW ) src.x = 14;
		else if ( !BaseN &&  BaseE &&  BaseS && !BaseW ) src.x =  9;
		else if ( !BaseN &&  BaseE && !BaseS &&  BaseW ) src.x =  6;
		else if ( !BaseN &&  BaseE && !BaseS && !BaseW ) src.x =  2;
		else if ( !BaseN && !BaseE &&  BaseS &&  BaseW ) src.x = 10;
		else if ( !BaseN && !BaseE &&  BaseS && !BaseW ) src.x =  3;
		else if ( !BaseN && !BaseE && !BaseS &&  BaseW ) src.x =  4;
		else if ( !BaseN && !BaseE && !BaseS && !BaseW ) src.x =  0;
		src.x *= src.h;

		if ( src.x != 0 || data.is_connector )
		{
			//blit shadow
			temp = dest;
			if ( SettingsData.bShadows ) blittShadow( UnitsData.ptr_connector_shw, &src, surface, &temp );
			//blit the image
			temp = dest;
			SDL_BlitSurface ( UnitsData.ptr_connector, &src, surface, &temp );
		}

	}
	else
	{
		//make connector stubs of big buildings.
		//upper left field
		src.x = 0;
		if      (  BaseN &&  BaseW ) src.x = 7;
		else if (  BaseN && !BaseW ) src.x = 1;
		else if ( !BaseN &&  BaseW ) src.x = 4;
		src.x *= src.h;

		if ( src.x != 0 )
		{
			temp = dest;
			if ( SettingsData.bShadows ) blittShadow( UnitsData.ptr_connector_shw, &src, surface, &temp );
			temp = dest;
			SDL_BlitSurface ( UnitsData.ptr_connector, &src, surface, &temp );
		}

		//upper right field
		src.x = 0;
		dest.x += zoom;
		if      (  BaseBN &&  BaseE ) src.x = 8;
		else if (  BaseBN && !BaseE ) src.x = 1;
		else if ( !BaseBN &&  BaseE ) src.x = 2;
		src.x *= src.h;

		if ( src.x != 0 )
		{
			temp = dest;
			if ( SettingsData.bShadows ) blittShadow( UnitsData.ptr_connector_shw, &src, surface, &temp );
			temp = dest;
			SDL_BlitSurface ( UnitsData.ptr_connector, &src, surface, &temp );
		}

		//lower right field
		src.x = 0;
		dest.y += zoom;
		if      (  BaseBE &&  BaseBS ) src.x = 9;
		else if (  BaseBE && !BaseBS ) src.x = 2;
		else if ( !BaseBE &&  BaseBS ) src.x = 3;
		src.x *= src.h;

		if ( src.x != 0 )
		{
			temp = dest;
			if ( SettingsData.bShadows ) blittShadow( UnitsData.ptr_connector_shw, &src, surface, &temp );
			temp = dest;
			SDL_BlitSurface ( UnitsData.ptr_connector, &src, surface, &temp );
		}

		//lower left field
		src.x = 0;
		dest.x -= zoom;
		if      (  BaseS &&  BaseBW ) src.x = 10;
		else if (  BaseS && !BaseBW ) src.x =  3;
		else if ( !BaseS &&  BaseBW ) src.x =  4;
		src.x *= src.h;

		if ( src.x != 0 )
		{
			temp = dest;
			if ( SettingsData.bShadows ) blittShadow( UnitsData.ptr_connector_shw, &src, surface, &temp );
			temp = dest;
			SDL_BlitSurface ( UnitsData.ptr_connector, &src, surface, &temp );
		}
	}
}

//--------------------------------------------------------------------------
/** starts the building for the server thread */
//--------------------------------------------------------------------------
void cBuilding::ServerStartWork ()
{
	cBuilding *b;

	if ( IsWorking )
	{
		sendDoStartWork(this);
		return;
	}

	if ( Disabled )
	{
		sendChatMessageToClient("Text~Comp~Building_Disabled", SERVER_ERROR_MESSAGE, owner->Nr );
		return;
	}

	// needs human workers:
	if ( data.human_need )
	{
		if ( SubBase->HumanProd < SubBase->HumanNeed + data.human_need )
		{
			sendChatMessageToClient ( "Text~Comp~Team_Low", SERVER_ERROR_MESSAGE, owner->Nr );
			return;
		}
		SubBase->HumanNeed += data.human_need;
	}

	// Energiegeneratoren / Energy generators:
	if ( data.energy_prod )
	{
		// check if there is enough Oil for the generators (current prodiction + reserves)
		if ( data.oil_need + SubBase->OilNeed > SubBase->Oil + SubBase->OilProd )
		{
			int MaxSubBaseOilProd = 0; // maximal possible Oil Production in the current SubBase
			// not enough Oil, so check if Oil production in current SubBase can be adjusted or not
			for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
			{
				// search for active mines in the SubBase
				if ( !SubBase->buildings[i]->data.is_mine || !SubBase->buildings[i]->IsWorking )
					continue;

				// store SubBase Oil production information
				MaxSubBaseOilProd += SubBase->buildings[i]->MaxOilProd;
			}
			// will adjusted Oil production help?
			if ( data.oil_need + SubBase->OilNeed > SubBase->Oil + MaxSubBaseOilProd )
			{
				// not enough Oil even with adjustments - so give up
				if(Client)if(Client->iTurn != 1) //HACK to prevent warning from being shown after auto start of factory in first turn
				{
					sendChatMessageToClient ( "Text~Comp~Fuel_Insufficient", SERVER_ERROR_MESSAGE, owner->Nr );
				}
				return;
			}
			else
			{
				// with adjustments, there will be enough Oil to burn - so make adjustments

				int FreeProdPower = 0; // local var - unexploited mining power
				int FreeOilProdPower = 0; // local var - unexploited Oil mining power
				int NeededOilAdj = 0; // local var - needed adjustments to Oil production
				int OrigNeededOilAdj = 0; // local temp variable to hold original NeededOilAdj variable's contents

				OrigNeededOilAdj = NeededOilAdj = data.oil_need + SubBase->OilNeed - SubBase->Oil - SubBase->OilProd;
				// try to exploit currently unexploited mining power first
				for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
				{
					// if made the needed adjustments, exit from the loop cycle
					if ( NeededOilAdj == 0 )
						break;

					b = SubBase->buildings[i];
					// search for active mines in the SubBase
					if ( !b->data.is_mine || !b->IsWorking )
						continue;

					// is there unexploited mining power? (max is 16 per mine)
					FreeProdPower = 16 - b->OilProd - b->MetalProd -b->GoldProd;
					// possible to increase Oil mining in the current mine?
					FreeOilProdPower = b->MaxOilProd - b->OilProd;
					// do checks and make adjustments
					while ( FreeProdPower > 0 && FreeOilProdPower > 0 && NeededOilAdj > 0 )
					{
						FreeProdPower--;
						FreeOilProdPower--;
						NeededOilAdj--;
						b->OilProd++;
						SubBase->OilProd++;
					}
				}
				// need to make more adjustments? By trying to reduce RAW material production (decrease Metal production first as Gold is more valuable)
				if ( NeededOilAdj > 0 )
				{
					// try to reduce RAW material production
					for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
					{
						// if made the needed adjustments, exit from the loop cycle
						if ( NeededOilAdj == 0 )
							break;

						b = SubBase->buildings[i];
						// search for active mines in the SubBase
						if ( !b->data.is_mine || !b->IsWorking )
							continue;

						// possible to increase Oil mining in the current mine?
						FreeOilProdPower = b->MaxOilProd - b->OilProd;

						// do checks and make adjustments
						while ( FreeOilProdPower > 0 && b->MetalProd > 0 && NeededOilAdj > 0 )
						{
							b->MetalProd--; // decrease Metal Production
							SubBase->MetalProd--;
							FreeOilProdPower--; // decrease Free Oil production power counter
							NeededOilAdj--; // decrease needed Adjustments to Oil production in SubBase counter
							b->OilProd++; // increase Oil production
							SubBase->OilProd++;
						}
					}
				}
				// need to make further adjustments? By decreasing even the Gold production?
/*
				if ( NeededOilAdj > 0 )
				{
					// Who cares... this is relatively imossible to occure - TODO
				}
*/
				// need to make even further adjustments??? - then it's time to give up
				if ( NeededOilAdj > 0 )
				{
					// and send warning message to Client only if actual adjustments has been made
					if ( OrigNeededOilAdj - NeededOilAdj != 0 ) 
					{
						sendChatMessageToClient ( "Text~Comp~Adjustments_Made", SERVER_ERROR_MESSAGE, owner->Nr );
					}
					else
					{
						sendChatMessageToClient ( "Text~Comp~Fuel_Insufficient", SERVER_ERROR_MESSAGE, owner->Nr );
					}
					return;
				}
				else
				{
					// adjustments successed so send warning message to Client about Adjustments and change EnergyProd and oilNeed info
					sendChatMessageToClient ( "Text~Comp~Adjustments_Made", SERVER_ERROR_MESSAGE, owner->Nr );

					SubBase->EnergyProd += data.energy_prod;
					SubBase->OilNeed += data.oil_need;
				}
			}
		}
		else
		{
			SubBase->EnergyProd += data.energy_prod;
			SubBase->OilNeed += data.oil_need;
		}
	}

	// Energieverbraucher / Energy consumers:
	else
		if ( data.energy_need )
		{
			if ( data.energy_need + SubBase->EnergyNeed > SubBase->MaxEnergyProd )
			{	
				sendChatMessageToClient ( "Text~Comp~Energy_Insufficient", SERVER_ERROR_MESSAGE, owner->Nr );
				return;
			}
			else
			{
				if ( data.energy_need + SubBase->EnergyNeed > SubBase->EnergyProd )
				{
					if(Client)if(Client->iTurn != 1) //HACK to prevent warning from being shown after auto start of factory in first turn
					{
						sendChatMessageToClient ( "Text~Comp~Energy_ToLow", SERVER_INFO_MESSAGE, owner->Nr );
					}
					/*
					 * Workaround for powering up Mining stations when there's not enough power and Oil.
					 *	Check unpowered generators, and Oil production limits in current mine.
					 *	If the circumstances are acceptable, then lie to the offline power generator
					 *	before calling ServerStartWork() and correct our lies after it powered up.
					 */

					for ( unsigned int i = 0; i < SubBase->buildings.Size(); i++)
					{
						b = SubBase->buildings[i];
						// in first round, only search for turned off small generators
						if ( !b->data.energy_prod || b->data.is_big || b->IsWorking )
							continue;

						// try to start an offline small generator if found
						b->ServerStartWork();

						if ( data.energy_need + SubBase->EnergyNeed <= SubBase->EnergyProd )
							break;

						/* Code execution reached this point, so there IS an offline power generator,
						 * but it could not be started, possibly due to lack of Oil. Now it's time to
						 * check if we are trying to start an offline mine or something else.
						 */

						// check if this is an offline mine that has enough Oil production power to start a new small generator or not
						if ( this->data.is_mine && !this->IsWorking && (this->MaxOilProd >= 2) )
						{
							// let's fake the SubBase booking about current Oil production while we starting the new small generator
							SubBase->OilProd += 2; // we want to start a small generator, that needs 2 barrels of Oil
							// now try to start the offline small generator we found earlier
							b->ServerStartWork();
							// correct the SubBase booking
							SubBase->OilProd -= 2;
							// break the loop cycle if we successed this time
							if ( data.energy_need + SubBase->EnergyNeed <= SubBase->EnergyProd )
								break;
						}
					}

					for ( unsigned int i = 0; i < SubBase->buildings.Size(); i++)
					{
						if ( data.energy_need + SubBase->EnergyNeed <= SubBase->EnergyProd )
							break;

						b = SubBase->buildings[i];
						// search for turned off generators
						if ( !b->data.energy_prod || b->IsWorking )
							continue;

						b->ServerStartWork();
					}
					// something went wrong
					if ( data.energy_need + SubBase->EnergyNeed > SubBase->EnergyProd )
					{
						sendChatMessageToClient("Text~Comp~Energy_Insufficient", SERVER_ERROR_MESSAGE, owner->Nr);
						return;
					}

					SubBase->EnergyNeed += data.energy_need;
				}
				else
				{
					SubBase->EnergyNeed += data.energy_need;
				}
			}
		}

	// raw material conumer:
	if ( data.metal_need )
		SubBase->MetalNeed += min(MetalPerRound, (*BuildList)[0]->metall_remaining);

	// gold consumer:
	if ( data.gold_need )
		SubBase->GoldNeed += data.gold_need;

	// Minen:
	if ( data.is_mine )
	{
		SubBase->MetalProd += MetalProd;
		SubBase->OilProd += OilProd;
		SubBase->GoldProd += GoldProd;
	}

	// research building
	if ( data.can_research )
	{
		owner->ResearchCount++;
		owner->researchCentersWorkingOnArea[researchArea]++;
	}
	
	IsWorking = true;
	sendSubbaseValues(SubBase, owner->Nr);
	sendDoStartWork(this);
}

//------------------------------------------------------------
/** starts the building in the client thread */
//------------------------------------------------------------
void cBuilding::ClientStartWork()
{
	if (IsWorking) 
		return;
	IsWorking = true;
	EffectAlpha = 0;
	if (selected)
	{
		StopFXLoop (Client->iObjectStream);
		PlayFX (typ->Start);
		Client->iObjectStream = playStream ();
		ShowDetails();
	}
	if (data.can_research) 
		owner->startAResearch (researchArea);
}

//--------------------------------------------------------------------------
/** Stops the building's working in the server thread */
//--------------------------------------------------------------------------
void cBuilding::ServerStopWork ( bool override )
{
	if ( !IsWorking )
	{
		sendDoStopWork(this);
		return;
	}

	// energy generators
	if ( data.energy_prod )
	{
		if ( SubBase->EnergyNeed > SubBase->EnergyProd - data.energy_prod && !override )
		{
			sendChatMessageToClient ( "Text~Comp~Energy_IsNeeded", SERVER_ERROR_MESSAGE, owner->Nr );
			return;
		}

		SubBase->EnergyProd -= data.energy_prod;
		SubBase->OilNeed -= data.oil_need;
	}
	// Energy consumers:
	else if ( data.energy_need )
		SubBase->EnergyNeed -= data.energy_need;

	// raw material consumer:
	if ( data.metal_need )
		SubBase->MetalNeed -= min(MetalPerRound, (*BuildList)[0]->metall_remaining);

	// gold consumer
	if ( data.gold_need )
		SubBase->GoldNeed -= data.gold_need;

	// human consumer
	if ( data.human_need )
		SubBase->HumanNeed -= data.human_need;

	// Minen:
	if ( data.is_mine )
	{
		SubBase->MetalProd -= MetalProd;
		SubBase->OilProd -= OilProd;
		SubBase->GoldProd -= GoldProd;
	}
	
	if ( data.can_research )
	{
		owner->ResearchCount--;
		owner->researchCentersWorkingOnArea[researchArea]--;
	}
	
	IsWorking = false;
	sendSubbaseValues(SubBase, owner->Nr);
	sendDoStopWork(this);
}

//------------------------------------------------------------
/** stops the building in the client thread */
//------------------------------------------------------------
void cBuilding::ClientStopWork()
{
	if (!IsWorking) 
		return;
	IsWorking = false;
	if (selected)
	{
		StopFXLoop (Client->iObjectStream);
		PlayFX (typ->Stop);
		Client->iObjectStream = playStream ();
		ShowDetails ();
	}
	if (data.can_research) 
		owner->stopAResearch (researchArea);
}

//------------------------------------------------------------
bool cBuilding::CanTransferTo ( cMapField *OverUnitField )
{
	cBuilding *b;
	cVehicle *v;
	int x, y;
	mouse->GetKachel ( &x, &y );

	if ( OverUnitField->getVehicles() )
	{
		v = OverUnitField->getVehicles();

		if ( v->owner != Client->ActivePlayer )
			return false;

		if ( v->data.can_transport != data.can_load )
			return false;

		if ( v->IsBuilding || v->IsClearing )
			return false;

		for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
		{
			b = SubBase->buildings[i];

			if ( b->data.is_big )
			{
				if ( x < b->PosX - 1 || x > b->PosX + 2 || y < b->PosY - 1 || y > b->PosY + 2 )
					continue;
			}
			else
			{
				if ( x < b->PosX - 1 || x > b->PosX + 1 || y < b->PosY - 1 || y > b->PosY + 1 )
					continue;
			}

			return true;
		}

		return false;
	}
	else
		if ( OverUnitField->getTopBuilding() )
		{
			b = OverUnitField->getTopBuilding();

			if ( b == this )
				return false;

			if ( b->SubBase != SubBase )
				return false;

			if ( b->owner != Client->ActivePlayer )
				return false;

			if ( data.can_load != b->data.can_load )
				return false;

			return true;
		}

	return false;
}

//--------------------------------------------------------------------------
bool cBuilding::isNextTo( int x, int y) const
{
	if ( x + 1 < PosX || y + 1 < PosY ) return false;

	if ( data.is_big )
	{
		if ( x - 2 > PosX || y - 2 > PosY ) return false;
	}
	else
	{
		if ( x - 1 > PosX || y - 1 > PosY ) return false;
	}

	return true;
}


//--------------------------------------------------------------------------
/** draws the exit points for a vehicle of the given type: */
//--------------------------------------------------------------------------
void cBuilding::DrawExitPoints ( sVehicle *typ )
{
	int spx, spy, size;
	spx = GetScreenPosX();
	spy = GetScreenPosY();
	size = Client->Map->size;

	if ( canExitTo ( PosX - 1, PosY - 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx - Client->Hud.Zoom, spy - Client->Hud.Zoom );

	if ( canExitTo ( PosX    , PosY - 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx, spy - Client->Hud.Zoom );

	if ( canExitTo ( PosX + 1, PosY - 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom, spy - Client->Hud.Zoom );

	if ( canExitTo ( PosX + 2, PosY - 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom*2, spy - Client->Hud.Zoom );

	if ( canExitTo ( PosX - 1, PosY    , Client->Map, typ ) )
		Client->drawExitPoint ( spx - Client->Hud.Zoom, spy );

	if ( canExitTo ( PosX + 2, PosY    , Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom*2, spy );

	if ( canExitTo ( PosX - 1, PosY + 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx - Client->Hud.Zoom, spy + Client->Hud.Zoom );

	if ( canExitTo ( PosX + 2, PosY + 1, Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom*2, spy + Client->Hud.Zoom );

	if ( canExitTo ( PosX - 1, PosY + 2, Client->Map, typ ) )
		Client->drawExitPoint ( spx - Client->Hud.Zoom, spy + Client->Hud.Zoom*2 );

	if ( canExitTo ( PosX    , PosY + 2, Client->Map, typ ) )
		Client->drawExitPoint ( spx, spy + Client->Hud.Zoom*2 );

	if ( canExitTo ( PosX + 1, PosY + 2, Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom, spy + Client->Hud.Zoom*2 );

	if ( canExitTo ( PosX + 2, PosY + 2, Client->Map, typ ) )
		Client->drawExitPoint ( spx + Client->Hud.Zoom*2, spy + Client->Hud.Zoom*2 );
}

//--------------------------------------------------------------------------
bool cBuilding::canExitTo ( const int x, const int y, const cMap* map, const sVehicle *typ ) const
{
	if ( !map->possiblePlaceVehicle( typ->data, x, y ) ) return false;
	if ( !isNextTo(x, y) ) return false;

	return true;
}

//--------------------------------------------------------------------------
bool cBuilding::canLoad ( int offset, cMap *Map )
{
	if ( offset < 0 || offset > Map->size*Map->size ) return false;

	if ( data.can_load == TRANS_AIR ) return canLoad ( Map->fields[offset].getPlanes() );
	if ( data.can_load != TRANS_AIR ) return canLoad ( Map->fields[offset].getVehicles() );

	return false;
}

//--------------------------------------------------------------------------
/** returns, if the vehicle can be loaded from its position: */
//--------------------------------------------------------------------------
bool cBuilding::canLoad ( cVehicle *Vehicle )
{
	if ( !Vehicle ) return false;

	if ( Vehicle->Loaded ) return false;

	if ( data.cargo == data.max_cargo ) return false;

	if ( !isNextTo ( Vehicle->PosX, Vehicle->PosY ) ) return false;

	if ( data.can_load == TRANS_MEN && !Vehicle->data.is_human ) return false;

	if ( data.can_load != TRANS_MEN && Vehicle->data.is_human ) return false;

	if ( !( data.build_on_water ? ( Vehicle->data.can_drive == DRIVE_SEA ) : ( Vehicle->data.can_drive != DRIVE_SEA ) ) ) return false;

	if ( Vehicle->ClientMoveJob && ( Vehicle->moving || Vehicle->Attacking || Vehicle->MoveJobActive ) ) return false;

	if ( Vehicle->owner == owner && !Vehicle->IsBuilding && !Vehicle->IsClearing ) return true;

	return false;
}

//--------------------------------------------------------------------------
/** loads a vehicle: */
//--------------------------------------------------------------------------
void cBuilding::storeVehicle( cVehicle *Vehicle, cMap *Map  )
{
	Map->deleteVehicle ( Vehicle );
	if ( Vehicle->bSentryStatus )
	{
		Vehicle->owner->deleteSentryVehicle( Vehicle );
		Vehicle->bSentryStatus = false;
	}

	Vehicle->Loaded = true;

	StoredVehicles.Add ( Vehicle );
	data.cargo++;

	if ( data.cargo == data.max_cargo ) LoadActive = false;

	owner->DoScan();
}

//--------------------------------------------------------------------------
/** Shows the storage screen. */
//--------------------------------------------------------------------------
void cBuilding::ShowStorage ()
{
	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b, to;
	SDL_Surface *sf;
	SDL_Rect src, dest;
	bool DownPressed = false, DownEnabled = false;
	bool UpPressed = false, UpEnabled = false;
	bool AlleAktivierenEnabled = false;
	bool AlleAufladenEnabled = false;
	bool AlleReparierenEnabled = false;
	bool AlleUpgradenEnabled = false;
	int offset = 0;
	Client->isInMenu = true;

#define BUTTON__W 77
#define BUTTON__H 23

	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };

	SDL_Rect rBtnAllActive = {rDialog.x + 518, rDialog.y + 246, BUTTON__W, BUTTON__H};

	//IMPORTANT: These are just for reference! If you change them you'll
	//have to change them at MakeStorageButtonsAlle too -- beko
	SDL_Rect rBtnRefuel = {rDialog.x + 518, rDialog.y + 271, BUTTON__W, BUTTON__H};
	SDL_Rect rBtnRepair = {rDialog.x + 518, rDialog.y + 271 + 25,  BUTTON__W, BUTTON__H};
	SDL_Rect rBtnUpgrade = {rDialog.x + 518, rDialog.y + 271 + 25*2, BUTTON__W, BUTTON__H};

	LoadActive = false;
	mouse->SetCursor ( CHand );
	mouse->draw ( false, buffer );

	if ( data.can_load == TRANS_AIR )
	{
		SDL_BlitSurface ( GraphicsData.gfx_storage, NULL, buffer, &rDialog );
		to = 4;
	}
	else
	{
		src.x = 480;
		src.y = 0;
		src.w = 640 - 480;
		src.h = 480;
		dest.x = rDialog.x + src.x;
		dest.y = rDialog.y + src.y;
		SDL_BlitSurface ( GraphicsData.gfx_storage, &src, buffer, &dest );
		dest.x = rDialog.x;
		dest.y = rDialog.y;
		SDL_BlitSurface ( GraphicsData.gfx_storage_ground, NULL, buffer, &dest );
		to = 6;
	}

	// Alle Buttons machen:

	NormalButton btn_done(rDialog.x + 518, rDialog.y + 371, "Text~Button~Done");
	btn_done.Draw();

	// Down:
	if ((int)StoredVehicles.Size() > to)
	{
		DownEnabled = true;
		src.x = 103;
		src.y = 452;
		src.h = src.w = 25;
		dest.x = rDialog.x + 530;
		dest.y = rDialog.y + 426;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	}

	// Alle Aktivieren:
	if (StoredVehicles.Size())
	{
		drawButton ( lngPack.i18n ( "Text~Button~Active" ), false, rBtnAllActive.x, rBtnAllActive.y, buffer );
		AlleAktivierenEnabled = true;
	}
	else
	{
		drawButton ( lngPack.i18n ( "Text~Button~Active" ), true, rBtnAllActive.x, rBtnAllActive.y, buffer );
	}
	//FIXME: reimplement all repair, all reload and all upgrade
	//MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );

	// Vehicles anzeigen:
	DrawStored ( offset );

	// Metallreserven anzeigen:
	ShowStorageMetalBar();

	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack ( buffer );

	mouse->MoveCallback = false;

	while ( 1 )
	{
		if (  Client->SelectedBuilding != this ) break;
		if ( !Client->isInMenu ) break;

		Client->handleTimer();
		Client->doGameActions();

		// Events holen:
		EventHandler->HandleEvents();

		// Die Maus machen:
		mouse->GetPos();

		b = (int)Client->getMouseState().leftButtonPressed;

		x = mouse->x;

		y = mouse->y;

		if ( x != LastMouseX || y != LastMouseY )
		{
			mouse->draw ( true, screen );
		}

		// Down-Button:
		if ( DownEnabled )
		{
			if ( x >= rDialog.x + 530 && x < rDialog.x + 530 + 25 && y >= rDialog.y + 426 && y < rDialog.y + 426 + 25 && b && !DownPressed )
			{
				PlayFX ( SoundData.SNDObjectMenu );
				src.x = 530;
				src.y = 426;
				src.w = 25;
				src.h = 25;
				dest.x = rDialog.x + 530;
				dest.y = rDialog.y + 426;

				offset += to;

				if ((int)StoredVehicles.Size() <= offset + to)
					DownEnabled = false;

				DrawStored ( offset );

				SDL_BlitSurface ( GraphicsData.gfx_storage, &src, buffer, &dest );

				src.x = 130;
				src.y = 452;
				src.w = 25;
				src.h = 25;
				dest.x = rDialog.x + 504;
				dest.y = rDialog.y + 426;
				SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

				UpEnabled = true;

				SHOW_SCREEN
				mouse->draw ( false, screen );

				DownPressed = true;
			}
			else
				if ( !b && DownPressed && DownEnabled )
				{
					src.x = 103;
					src.y = 452;
					src.w = 25;
					src.h = 25;
					dest.x = rDialog.x + 530;
					dest.y = rDialog.y + 426;
					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
					SHOW_SCREEN
					mouse->draw ( false, screen );
					DownPressed = false;
				}
		}

		// Up-Button:
		if ( UpEnabled )
		{
			if ( x >= rDialog.x + 504 && x < rDialog.x + 504 + 25 && y >= rDialog.y + 426 && y < rDialog.y + 426 + 25 && b && !UpPressed )
			{
				PlayFX ( SoundData.SNDObjectMenu );
				src.x = 504;
				src.y = 426;
				src.w = 25;
				src.h = 25;
				dest.x = rDialog.x + 504;
				dest.y = rDialog.y + 426;

				offset -= to;

				if ( offset == 0 )
					UpEnabled = false;

				DrawStored ( offset );

				SDL_BlitSurface ( GraphicsData.gfx_storage, &src, buffer, &dest );

				mouse->draw ( false, screen );

				UpPressed = true;

				if ((int)StoredVehicles.Size() > to)
				{
					DownEnabled = true;
					src.x = 103;
					src.y = 452;
					src.h = src.w = 25;
					dest.x = rDialog.x + 530;
					dest.y = rDialog.y + 426;
					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
				}

				SHOW_SCREEN
			}
			else
				if ( !b && UpPressed && UpEnabled )
				{
					src.x = 130;
					src.y = 452;
					src.w = 25;
					src.h = 25;
					dest.x = rDialog.x + 504;
					dest.y = rDialog.y + 426;
					SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
					SHOW_SCREEN
					mouse->draw ( false, screen );
					UpPressed = false;
				}
		}

		if (btn_done.CheckClick(x, y, b > LastB, b < LastB))
		{
			break;
		}

		// Alle Aktivieren:
		if ( x >= rBtnAllActive.x && x < rBtnAllActive.x + rBtnAllActive.w && y >= rBtnAllActive.y && y < rBtnAllActive.y + rBtnAllActive.h && b && !LastB && AlleAktivierenEnabled )
		{
			PlayFX ( SoundData.SNDMenuButton );
			drawButton ( lngPack.i18n ( "Text~Button~Active" ), false, rBtnAllActive.x, rBtnAllActive.y, buffer );
			SHOW_SCREEN
			mouse->draw ( false, screen );

			while ( b )
			{
				Client->doGameActions();
				EventHandler->HandleEvents();
				b = (int)Client->getMouseState().leftButtonPressed;
			}

			Client->OverUnitField = NULL;

			mouse->MoveCallback = true;

			PlayFX ( SoundData.SNDActivate );

			bool hasCheckedPlace[16];
			for ( int i = 0; i < 16; i++ )
			{
				hasCheckedPlace[i] = false;
			}

			for ( unsigned int i = 0; i < StoredVehicles.Size(); i++ )
			{
				cVehicle *vehicle = StoredVehicles[i];
				bool activated = false;
				for ( int ypos = PosY-1, poscount = 0; ypos <= PosY+2; ypos++ )
				{
					if ( ypos < 0 || ypos >= Client->Map->size ) continue;
					for ( int xpos = PosX-1; xpos <= PosX+2; xpos++, poscount++ )
					{
						if ( xpos < 0 || xpos >= Client->Map->size || ( ( ypos == PosY || ypos == PosY+1 ) && ( xpos == PosX || xpos == PosX+1 ) ) ) continue;
						if ( canExitTo ( xpos, ypos, Client->Map, vehicle->typ ) && !hasCheckedPlace[poscount] )
						{
							sendWantActivate ( iID, false, vehicle->iID, xpos, ypos );
							hasCheckedPlace[poscount] = true;
							activated = true;
							break;
						}
					}
					if ( activated ) break;
				}
			}

			break;
		}

		// Alle Aufladen:
		if ( x >= rBtnRefuel.x && x < rBtnRefuel.x + rBtnRefuel.w && y >= rBtnRefuel.y && y < rBtnRefuel.y + rBtnRefuel.h && b && !LastB && AlleAufladenEnabled )
		{
			PlayFX ( SoundData.SNDMenuButton );

// TODO: reimplement
//			for (unsigned int i = 0; i < StoredVehicles.Size(); i++)
//			{
//				cVehicle *v;
//				v = StoredVehicles[i];
//
//				if ( v->data.ammo != v->data.max_ammo )
//				{
//					v->data.ammo = v->data.max_ammo;
//					owner->base.AddMetal ( SubBase, -2 );
//					//SendUpdateStored ( i );
//
//					if ( SubBase->Metal < 2 )
//						break;
//				}
//			}

			DrawStored ( offset );

			PlayVoice ( VoiceData.VOILoaded );
			ShowStorageMetalBar();
			AlleAufladenEnabled = false;
			MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );

			SHOW_SCREEN
			mouse->draw ( false, screen );

		}

		// Alle Reparieren:
		if ( x >= rBtnRepair.x && x < rBtnRepair.x + rBtnRepair.w && y >= rBtnRepair.y && y < rBtnRepair.y + rBtnRepair.h && b && !LastB && AlleReparierenEnabled )
		{
			PlayFX ( SoundData.SNDMenuButton );
			dest.x = rDialog.x + 511;
			dest.y = rDialog.y + 251 + 25 * 2;
// TODO: reimplement
//			for (unsigned int i = 0; i < StoredVehicles.Size(); i++)
//			{
//				cVehicle *v;
//				v = StoredVehicles[i];
//
//				if ( v->data.hit_points != v->data.max_hit_points )
//				{
//					v->data.hit_points = v->data.max_hit_points;
//					owner->base.AddMetal ( SubBase, -2 );
//					//SendUpdateStored ( i );
//
//					if ( SubBase->Metal < 2 )
//						break;
//				}
//			}

			DrawStored ( offset );

			PlayVoice ( VoiceData.VOIRepaired );
			ShowStorageMetalBar();
			AlleReparierenEnabled = false;
			MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// Alle Upgraden:
		if ( x >= rBtnUpgrade.x && x < rBtnUpgrade.x + rBtnUpgrade.w && y >= rBtnUpgrade.y && y < rBtnUpgrade.y + rBtnUpgrade.h && b && !LastB && AlleUpgradenEnabled )
		{
			PlayFX ( SoundData.SNDMenuButton );
// TODO: reimplement
//			for (unsigned int i = 0; i < StoredVehicles.Size(); i++)
//			{
//				cVehicle *v;
//				v = StoredVehicles[i];
//
//				if ( v->data.version != owner->VehicleData[v->typ->nr].version )
//				{
//
//					Update ( v->data, owner->VehicleData[v->typ->nr] )
//
//					v->GenerateName();
//					owner->base.AddMetal ( SubBase, -2 );
//					SendUpdateStored ( i );
//
//					if ( SubBase->Metal < 2 )
//						break;
//				}
//			}

			DrawStored ( offset );

			ShowStorageMetalBar();
			AlleUpgradenEnabled = false;
			MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// Buttons unter den Vehicles:

		if ( data.can_load == TRANS_AIR )
		{
			sf = GraphicsData.gfx_storage;
		}
		else
		{
			sf = GraphicsData.gfx_storage_ground;
		}

		for ( int i = 0;i < to;i++ )
		{
			cVehicle *v;

			if ((int)StoredVehicles.Size() <= i + offset)
				break;

			v = StoredVehicles[i + offset];

			if ( data.can_load == TRANS_AIR )
			{
				switch ( i )
				{

					case 0:
						dest.x = rDialog.x + 44;
						dest.y = rDialog.y + 191;
						break;

					case 1:
						dest.x = rDialog.x + 270;
						dest.y = rDialog.y + 191;
						break;

					case 2:
						dest.x = rDialog.x + 44;
						dest.y = rDialog.y + 426;
						break;

					case 3:
						dest.x = rDialog.x + 270;
						dest.y = rDialog.y + 426;
						break;
				}
			}
			else
			{
				switch ( i )
				{

					case 0:
						dest.x = rDialog.x + 8;
						dest.y = rDialog.y + 191;
						break;

					case 1:
						dest.x = rDialog.x + 163;
						dest.y = rDialog.y + 191;
						break;

					case 2:
						dest.x = rDialog.x + 318;
						dest.y = rDialog.y + 191;
						break;

					case 3:
						dest.x = rDialog.x + 8;
						dest.y = rDialog.y + 426;
						break;

					case  4:
						dest.x = rDialog.x + 163;
						dest.y = rDialog.y + 426;
						break;

					case 5:
						dest.x = rDialog.x + 318;
						dest.y = rDialog.y + 426;
						break;
				}
			}

			// Aktivieren:
			if ( x >= dest.x && x < dest.x + 73 && y >= dest.y && y < dest.y + 23 && b && !LastB )
			{
				PlayFX ( SoundData.SNDMenuButton );
				ActivatingVehicle = true;
				VehicleToActivate = i + offset;
				drawButton ( lngPack.i18n ( "Text~Button~Active" ), true, dest.x, dest.y, buffer );
				SHOW_SCREEN
				mouse->draw ( false, screen );

				while ( b )
				{
					Client->doGameActions();
					EventHandler->HandleEvents();
					b = (int)Client->getMouseState().leftButtonPressed;
				}

				Client->OverUnitField = NULL;

				mouse->MoveCallback = true;
				Client->isInMenu = false;
				return;
			}
			// Reparatur:
			dest.x += 75;
			if ( x >= dest.x && x < dest.x + 73 && y >= dest.y && y < dest.y + 23 && b && !LastB && v->data.hit_points < v->data.max_hit_points )
			{
				PlayFX ( SoundData.SNDMenuButton );
				wantRedrawedStoredOffset = offset;
				sendWantSupply ( v->iID, true, iID, false, SUPPLY_TYPE_REPAIR );
				//MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );
				drawButton ( lngPack.i18n ( "Text~Button~Repair" ), true, dest.x, dest.y, buffer );

				SHOW_SCREEN
				mouse->draw ( false, screen );
			}

			// Aufladen:
			dest.x -= 75;
			dest.y += 25;
			if ( x >= dest.x && x < dest.x + 73 && y >= dest.y && y < dest.y + 23 && b && !LastB && v->data.ammo < v->data.max_ammo )
			{
				PlayFX ( SoundData.SNDMenuButton );
				wantRedrawedStoredOffset = offset;
				sendWantSupply ( v->iID, true, iID, false, SUPPLY_TYPE_REARM );
				//MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );
				drawButton ( lngPack.i18n ( "Text~Button~Reload" ), true, dest.x, dest.y, buffer );

				SHOW_SCREEN
				mouse->draw ( false, screen );
			}

			// Upgrade:
			dest.x += 75;
			if ( x >= dest.x && x < dest.x + 73 && y >= dest.y && y < dest.y + 23 && b && !LastB 
				&& v->data.version != owner->VehicleData[v->typ->nr].version )
			{
				PlayFX ( SoundData.SNDMenuButton );
				sendWantUpgrade (i + offset, false);
				//MakeStorageButtonsAlle ( &AlleAufladenEnabled, &AlleReparierenEnabled, &AlleUpgradenEnabled );
				drawButton ( lngPack.i18n ( "Text~Button~Upgrade" ), true, dest.x, dest.y, buffer );
				
				SHOW_SCREEN
				mouse->draw ( false, screen );
			}
			
		}

		LastMouseX = x;

		LastMouseY = y;
		LastB = b;
	}

	mouse->MoveCallback = true;
	Client->isInMenu = false;
}

//--------------------------------------------------------------------------
/** Draws all pictures of the loaded vehicles */
//--------------------------------------------------------------------------
void cBuilding::DrawStored ( int off )
{
	SDL_Rect src, dest;
	SDL_Surface *sf;
	cVehicle *vehicleV;
	int i, to;

	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };

	sf = SDL_CreateRGBSurface ( SDL_HWSURFACE | SDL_SRCCOLORKEY, DIALOG_W, DIALOG_H, SettingsData.iColourDepth, 0, 0, 0, 0 );

	if ( data.can_load == TRANS_AIR )
	{
		to = 4;
		SDL_BlitSurface ( GraphicsData.gfx_storage, NULL, sf, NULL );
		//sf=GraphicsData.gfx_storage;
	}
	else
	{
		to = 6;
		SDL_BlitSurface ( GraphicsData.gfx_storage_ground, NULL, sf, NULL );
		//sf=GraphicsData.gfx_storage_ground;
	}

	for ( i = 0;i < to;i++ )
	{
		if (i + off >= (int)StoredVehicles.Size())
		{
			vehicleV = NULL;
		}
		else
		{
			vehicleV = StoredVehicles[i + off];
		}

		// Das Bild malen:
		if ( data.can_load == TRANS_AIR )
			//4 possible bays on screen
		{
			switch ( i )
			{

				case 0 :
					dest.x = rDialog.x + ( src.x = 17 );
					dest.y = rDialog.y + ( src.y = 9 );
					break;

				case 1 :
					dest.x = rDialog.x + ( src.x = 243 );
					dest.y = rDialog.y + ( src.y = 9 );
					break;

				case 2 :
					dest.x = rDialog.x + ( src.x = 17 );
					dest.y = rDialog.y + ( src.y = 244 );
					break;

				case 3 :
					dest.x = rDialog.x + ( src.x = 243 );
					dest.y = rDialog.y + ( src.y = 244 );
					break;
			}

			src.w = 200; //hangarwidth

			src.h = 128; //hangarheight
		}
		else
		{
			//6 possible bays on screen

			switch ( i )
			{

				case 0 :
					dest.x = rDialog.x + ( src.x = 17 );
					dest.y = rDialog.y + ( src.y = 9 );
					break;

				case 1 :
					dest.x = rDialog.x + ( src.x = 172 );
					dest.y = rDialog.y + ( src.y = 9 );
					break;

				case 2 :
					dest.x = rDialog.x + ( src.x = 327 );
					dest.y = rDialog.y + ( src.y = 9 );
					break;

				case 3 :
					dest.x = rDialog.x + ( src.x = 17 );
					dest.y = rDialog.y + ( src.y = 244 );
					break;

				case 4 :
					dest.x = rDialog.x + ( src.x = 172 );
					dest.y = rDialog.y + ( src.y = 244 );
					break;

				case 5 :
					dest.x = rDialog.x + ( src.x = 327 );
					dest.y = rDialog.y + ( src.y = 244 );
					break;
			}

			src.w = 128; //hangarwidth

			src.h = 128; //hangarsize
		}

		SDL_BlitSurface ( sf, &src, buffer, &dest );

		if ( vehicleV )
		{
			SDL_BlitSurface ( vehicleV->typ->storage, NULL, buffer, &dest );
			// Den Namen ausgeben:
			font->showText ( dest.x + 5, dest.y + 5, vehicleV->name, FONT_LATIN_SMALL_WHITE );

			if ( vehicleV->data.version != vehicleV->owner->VehicleData[vehicleV->typ->nr].version )
			{
				font->showText ( dest.x + 5, dest.y + 15, "(" + lngPack.i18n ( "Text~Comp~Dated" ) + ")", FONT_LATIN_SMALL_WHITE );
			}
		}
		else
		{
			if ( data.iCapacity_Units_Sea_Max > 0 )
			{
				SDL_BlitSurface ( GraphicsData.gfx_edock, NULL, buffer, &dest );
			}
			else if (data.iCapacity_Units_Air_Max > 0)
			{
				SDL_BlitSurface ( GraphicsData.gfx_ehangar, NULL, buffer, &dest );
			}
			else
			{
				SDL_BlitSurface ( GraphicsData.gfx_edepot, NULL, buffer, &dest );
			}
		}
		// Die Buttons malen:
		// Aktivieren:
		if ( to == 4 )
		{
			dest.x += 27;
		}
		else
		{
			dest.x -= 9;
		}

		dest.y += 182;

		if ( vehicleV )
		{
			src.x = 156;
			src.y = 431;
			drawButton ( lngPack.i18n ( "Text~Button~Active" ), false, dest.x, dest.y, buffer );
		}
		else
		{
			drawButton ( lngPack.i18n ( "Text~Button~Active" ), true, dest.x, dest.y, buffer );
		}

		// Reparieren:
		dest.x += 75;

		if ( vehicleV && vehicleV->data.hit_points != vehicleV->data.max_hit_points && SubBase->Metal >= 2 )
		{
			drawButton ( lngPack.i18n ( "Text~Button~Repair" ), false, dest.x, dest.y, buffer );
		}
		else
		{
			drawButton ( lngPack.i18n ( "Text~Button~Repair" ), true, dest.x, dest.y, buffer );
		}

		// Aufladen:
		dest.x -= 75;

		dest.y += 25;

		if ( vehicleV && vehicleV->data.ammo != vehicleV->data.max_ammo && SubBase->Metal >= 2 )
		{
			drawButton ( lngPack.i18n ( "Text~Button~Reload" ), false, dest.x, dest.y, buffer );
		}
		else
		{
			drawButton ( lngPack.i18n ( "Text~Button~Reload" ), true, dest.x, dest.y, buffer );
		}

		// Upgrade:
		dest.x += 75;

		if ( vehicleV && vehicleV->data.version != vehicleV->owner->VehicleData[vehicleV->typ->nr].version && SubBase->Metal >= 2 )
		{
			drawButton ( lngPack.i18n ( "Text~Button~Upgrade" ), false, dest.x, dest.y, buffer );
		}
		else
		{
			drawButton ( lngPack.i18n ( "Text~Button~Upgrade" ), true, dest.x, dest.y, buffer );
		}

		// Display the additional info:
		dest.x -= 66;
		dest.y -= 69 - 6;
		src.w = 128;
		src.h = 30;
		src.x = dest.x - rDialog.x;
		src.y = dest.y - rDialog.y;
		SDL_BlitSurface ( sf, &src, buffer, &dest );
		dest.x += 6;

		if ( vehicleV )
		{
			// Die Hitpoints anzeigen:
			DrawNumber ( dest.x + 13, dest.y, vehicleV->data.hit_points, vehicleV->data.max_hit_points, buffer );
			font->showText ( dest.x + 27, dest.y, lngPack.i18n ( "Text~Hud~Hitpoints" ), FONT_LATIN_SMALL_WHITE );

			DrawSymbol ( SHits, dest.x + 60, dest.y, 58, vehicleV->data.hit_points, vehicleV->data.max_hit_points, buffer );
			// Die Munition anzeigen:

			if ( vehicleV->data.can_attack )
			{
				dest.y += 15;
				DrawNumber ( dest.x + 13, dest.y, vehicleV->data.ammo, vehicleV->data.max_ammo, buffer );

				font->showText ( dest.x + 27, dest.y, lngPack.i18n ( "Text~Hud~AmmoShort" ), FONT_LATIN_SMALL_WHITE );
				DrawSymbol ( SAmmo, dest.x + 60, dest.y, 58, vehicleV->data.ammo, vehicleV->data.max_ammo, buffer );
			}
		}
	}

	SDL_FreeSurface ( sf );
}

//--------------------------------------------------------------------------
/** Display the raw material bar in the storage screen */
//--------------------------------------------------------------------------
void cBuilding::ShowStorageMetalBar ()
{
	SDL_Rect src, dest;
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };

	//redraw metalbar to clean it from prior draws
	dest.x = rDialog.x + ( src.x = 490 );
	dest.y = rDialog.y + ( src.y = 80 );
	src.w = 136;
	src.h = 145;
	SDL_BlitSurface ( GraphicsData.gfx_storage, &src, buffer, &dest );

	//draw metalamount over the metalbar
	font->showTextCentered ( rDialog.x + 557, rDialog.y + 86, lngPack.i18n ( "Text~Title~Metal" ) + ": " + iToStr ( SubBase->Metal ) );


	//BEGIN fill metal bar
	src.x = 135;
	src.y = 335;
	dest.x = rDialog.x + 546;
	dest.y = rDialog.y + 106;
	src.w = 20;

	/*Gosh, this is tricky. I'll try to make an example. The metalbar graphic is 115px high.
	* We've eg. storages for metal 125 and we have an metal amount of 49 so this would look
	* like this: height of metalbar = 115 / (125/49)
	* e voila - metalbar is 45px height. So we blit 45px and draw them at the bottom of the
	* empty metal zylinder on storage.pcx
	*								-- beko
	*/
	// src.h = Round ( 115 / ( float ) ( SubBase->MaxMetal / ( float ) SubBase->Metal ) );
	src.h = Round ( 115 * SubBase->Metal / ( float ) SubBase->MaxMetal ); //	a/(b/c) = a*c/b
	dest.y += 115 - src.h;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	//END fill metal bar
}

//-----------------------------------------------------------------------
void cBuilding::sendWantUpgrade (int storageSlot, bool upgradeAll)
{
	cNetMessage* msg = new cNetMessage (GAME_EV_WANT_VEHICLE_UPGRADE);
	msg->pushInt32(iID);
	if (!upgradeAll)
		msg->pushInt16(storageSlot);
	msg->pushBool (upgradeAll);
	Client->sendNetMessage (msg);
}

//-----------------------------------------------------------------------
// Unloads a vehicle
void cBuilding::exitVehicleTo( cVehicle *Vehicle, int offset, cMap *Map )
{
	for ( unsigned int i = 0; i < StoredVehicles.Size(); i++ )
	{
		if ( StoredVehicles[i] == Vehicle )
		{
			StoredVehicles.Delete ( i );
			break;
		}
		if ( i == StoredVehicles.Size()-1 ) return;
	}

	data.cargo--;

	ActivatingVehicle = false;

	ActivatingVehicle = false;

	Map->addVehicle ( Vehicle, offset );

	Vehicle->PosX = offset % Map->size;
	Vehicle->PosY = offset / Map->size;
	Vehicle->Loaded = false;

	owner->DoScan();
}

//--------------------------------------------------------------------------
void cBuilding::MakeStorageButtonsAlle ( bool *AlleAufladenEnabled, bool *AlleReparierenEnabled, bool *AlleUpgradenEnabled )
{
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };
	SDL_Rect rBtnRefuel = {rDialog.x + 518, rDialog.y + 271, BUTTON__W, BUTTON__H};
	SDL_Rect rBtnRepair = {rDialog.x + 518, rDialog.y + 271 + 25,  BUTTON__W, BUTTON__H};
	SDL_Rect rBtnUpgrade = {rDialog.x + 518, rDialog.y + 271 + 25*2, BUTTON__W, BUTTON__H};



	*AlleAufladenEnabled = false;
	*AlleReparierenEnabled = false;
	*AlleUpgradenEnabled = false;

	if ( SubBase->Metal >= 2 )
	{
		for (unsigned int i = 0; i < StoredVehicles.Size(); i++)
		{
			cVehicle const* const v = StoredVehicles[i];
			if (v->data.ammo != v->data.max_ammo)
			{
				*AlleAufladenEnabled = true;
			}

			if (v->data.hit_points != v->data.max_hit_points)
			{
				*AlleReparierenEnabled = true;
			}

			if (v->data.version != v->owner->VehicleData[v->typ->nr].version && SubBase->Metal >= 2)
			{
				*AlleUpgradenEnabled = true;
			}
		}
	}

	// Alle Aufladen:
	if ( *AlleAufladenEnabled )
	{
		drawButton ( lngPack.i18n ( "Text~Button~Reload" ), false, rBtnRefuel.x, rBtnRefuel.y, buffer );
	}
	else
	{
		drawButton ( lngPack.i18n ( "Text~Button~Reload" ), true, rBtnRefuel.x, rBtnRefuel.y, buffer );
	}

	// Alle Reparieren:
	if ( *AlleReparierenEnabled )
	{
		drawButton ( lngPack.i18n ( "Text~Button~Repair" ), false, rBtnRepair.x, rBtnRepair.y, buffer );
	}
	else
	{
		drawButton ( lngPack.i18n ( "Text~Button~Repair" ), true, rBtnRepair.x, rBtnRepair.y, buffer );
	}

	// Alle Upgraden:
	if ( *AlleUpgradenEnabled )
	{
		drawButton ( lngPack.i18n ( "Text~Button~Upgrade" ), false, rBtnUpgrade.x, rBtnUpgrade.y, buffer );
	}
	else
	{
		drawButton ( lngPack.i18n ( "Text~Button~Upgrade" ), true, rBtnUpgrade.x, rBtnUpgrade.y, buffer );
	}
}


//---------------------------------------------------------------------
/** Displays the research screen. */
//---------------------------------------------------------------------
void cBuilding::ShowResearch ()
{
	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b;
	Client->isInMenu = true;

	//Dialog Research width
#define DLG_RSRCH_W GraphicsData.gfx_research->w
	//Dialog research height
#define DLD_RSRCH_H GraphicsData.gfx_research->h

	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DLG_RSRCH_W / 2, SettingsData.iScreenH / 2 - DLD_RSRCH_H / 2, DLG_RSRCH_W, DLD_RSRCH_H };
	SDL_Rect rTitle = {rDialog.x + 89, rDialog.y + 19, 182, 21};
	SDL_Rect rTxtLabs = {rDialog.x + 24, rDialog.y + 52, 68, 21};
	SDL_Rect rTxtThemes = {rDialog.x + 177, rDialog.y + 52, 45, 21};
	SDL_Rect rTxtRounds = {rDialog.x + 291, rDialog.y + 52, 45, 21};

	mouse->SetCursor (CHand);
	mouse->draw (false, buffer);
	Client->drawMap ();
	SDL_BlitSurface (GraphicsData.gfx_hud, NULL, buffer, NULL);

	if (SettingsData.bAlphaEffects)
		SDL_BlitSurface (GraphicsData.gfx_shadow, NULL, buffer, NULL);
	SDL_BlitSurface (GraphicsData.gfx_research, NULL, buffer, &rDialog);

	//draw titles
	font->showTextCentered ( rTitle.x + rTitle.w / 2, rTitle.y, lngPack.i18n ( "Text~Title~Labs" ) );
	font->showTextCentered ( rTxtLabs.x + rTxtLabs.w / 2, rTxtLabs.y, lngPack.i18n ( "Text~Comp~Labs" ) );
	font->showTextCentered ( rTxtThemes.x + rTxtThemes.w / 2, rTxtThemes.y, lngPack.i18n ( "Text~Comp~Themes" ) );
	font->showTextCentered ( rTxtRounds.x + rTxtRounds.w / 2, rTxtRounds.y, lngPack.i18n ( "Text~Comp~Turns" ) );

	NormalButton btn_cancel (rDialog.x +  91, rDialog.y + 294, "Text~Button~Cancel");
	NormalButton btn_done (rDialog.x + 193, rDialog.y + 294, "Text~Button~Done");
	btn_cancel.Draw ();
	btn_done.Draw ();

	int startResearchCenters = owner->ResearchCount;
	int newResearchSettings[cResearch::kNrResearchAreas];
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		newResearchSettings[i] = owner->researchCentersWorkingOnArea[i];
	
	// draw the sliders
	ShowResearchSliders(newResearchSettings, startResearchCenters);

	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack (buffer);

	while (true)
	{
		if (Client->SelectedBuilding != this) 
			break;
		if (Client->isInMenu == false) 
			break;

		Client->handleTimer();
		Client->doGameActions();
		EventHandler->HandleEvents();

		mouse->GetPos();
		b = (int)Client->getMouseState().leftButtonPressed;
		x = mouse->x;
		y = mouse->y;

		if (x != LastMouseX || y != LastMouseY)
			mouse->draw (true, screen);

		bool const down = (b > LastB);
		bool const up   = (b < LastB);

		if (btn_cancel.CheckClick(x, y, down, up))
		{
			// nothing to do...
			break;
		}

		if (btn_done.CheckClick(x, y, down, up))
		{
			sendWantResearchChange (newResearchSettings);
			break;
		}

		// Calculate the setting changes due to a mouseclick
		if (b && !LastB)
			handleResearchSliderMouseClick (x, y, newResearchSettings, startResearchCenters);

		LastMouseX = x;
		LastMouseY = y;
		LastB = b;
	}
	Client->isInMenu = false;
}

//------------------------------------------------------------------
/** Displays the sliders for the research areas. */
//------------------------------------------------------------------
void cBuilding::ShowResearchSliders (int newResearchSettings[cResearch::kNrResearchAreas], int startResearchCenters)
{
	int unusedResearch = startResearchCenters;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		unusedResearch -= newResearchSettings[i];
	
	SDL_Rect src, dest;
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DLG_RSRCH_W / 2, SettingsData.iScreenH / 2 - DLD_RSRCH_H / 2, DLG_RSRCH_W, DLD_RSRCH_H };
	SDL_Rect rTxtDescr = {rDialog.x + 183, rDialog.y + 72, 12, 21};
	string sTxtTheme = "";

	for (int area = 0; area < cResearch::kNrResearchAreas; area++)
	{
		src.x = 20;
		src.y = 70 + area * 28;
		dest.x = 20 + rDialog.x;
		dest.y = 70 + rDialog.y + area * 28;
		src.w = 316;
		src.h = 18;
		SDL_BlitSurface (GraphicsData.gfx_research, &src, buffer, &dest);

		// display the current nr of research centers for this area
		font->showTextCentered (dest.x + 21 + 2, dest.y + 1, iToStr (newResearchSettings[area]));
		// display the current research level of this area
		font->showTextCentered (258 + rDialog.x, dest.y + 1, iToStr (owner->researchLevel.getCurResearchLevel(area)));

		if (newResearchSettings[area] > 0)
			font->showTextCentered (rDialog.x + 313, dest.y + 1, iToStr (owner->researchLevel.getRemainingTurns (area, newResearchSettings[area])));

		// Display the left arrow
		if (newResearchSettings[area] == 0)
		{
			src.w = 19;
			src.h = 18;
			src.x = 237;
			src.y = 177;
			dest.x = 71 + rDialog.x;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
		}

		// Display the right arrow
		if (unusedResearch <= 0)
		{
			src.w = 19;
			src.h = 18;
			src.x = 257;
			src.y = 177;
			dest.x = 143 + rDialog.x;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
		}

		// Display the sliders
		src.w = 14;
		src.h = 17;
		src.x = 412;
		src.y = 46;
		// draw the slider-rect at a position that shows the proportion of research centers used for this area
		dest.x = 90 + rDialog.x + (int) (36 * ((float) (newResearchSettings[area]) / startResearchCenters));
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);

		switch (area)
		{
			case 0: sTxtTheme = lngPack.i18n ( "Text~Vehicles~Damage" ); break;
			case 1:	sTxtTheme = lngPack.i18n ( "Text~Hud~Shots" ); break;
			case 2:	sTxtTheme = lngPack.i18n ( "Text~Hud~Range" ); break;
			case 3: sTxtTheme = lngPack.i18n ( "Text~Hud~Armor" ); break;
			case 4: sTxtTheme = lngPack.i18n ( "Text~Hud~Hitpoints" ); break;
			case 5: sTxtTheme = lngPack.i18n ( "Text~Hud~Speed" ); break;
			case 6: sTxtTheme = lngPack.i18n ( "Text~Hud~Scan" ); break;
			case 7: sTxtTheme = lngPack.i18n ( "Text~Vehicles~Costs" ); break;
		}

		dest.x = rTxtDescr.x;
		//dest.w = rTxtDescr.w; //not used right now
		//dest.h = rTxtDescr.h; //not used right now
		dest.y = rTxtDescr.y + area * 28;
		font->showText (dest, sTxtTheme);
	}
}

//--------------------------------------------------------------
/** handles mouseclicks on the research sliders */
//--------------------------------------------------------------
void cBuilding::handleResearchSliderMouseClick (int mouseX, int mouseY, int newResearchSettings[cResearch::kNrResearchAreas], int startResearchCenters)
{
	int unusedResearch = startResearchCenters;
	for (int i = 0; i < cResearch::kNrResearchAreas; i++)
		unusedResearch -= (newResearchSettings)[i];

	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DLG_RSRCH_W / 2, SettingsData.iScreenH / 2 - DLD_RSRCH_H / 2, DLG_RSRCH_W, DLD_RSRCH_H };
	SDL_Rect rArrowLeft = {rDialog.x + 71 , rDialog.y + 70 , 19, 18 };
	SDL_Rect rArrowRight = {rDialog.x + 143 , rDialog.y + 70 , 19, 18 };
	bool changed = false;
	for (int area = 0; area < cResearch::kNrResearchAreas; area++)
	{
		// the left arrow
		if (mouseX >= rArrowLeft.x && mouseX < rArrowLeft.x +  rArrowLeft.w && mouseY >= rArrowLeft.y + area*28 && mouseY < rArrowLeft.y + rArrowLeft.h + area*28 
			&& (newResearchSettings)[area] > 0)
		{
			(newResearchSettings)[area] -= 1;
			unusedResearch += 1;
			changed = true;
		}

		// the right arrow
		if (mouseX >= rArrowRight.x && mouseX < rArrowRight.x +  rArrowRight.w && mouseY >= rArrowRight.y + area*28 && mouseY < rArrowRight.y + rArrowRight.h + area*28 
			&& unusedResearch > 0)
		{
			(newResearchSettings)[area] += 1;
			unusedResearch -= 1;
			changed = true;
		}
		
		// click directly into the slider
		if (rArrowLeft.x + rArrowLeft.w + 1 <= mouseX && mouseX < rArrowRight.x - 1 
			&& rArrowLeft.y + area*28 <= mouseY && mouseY < rArrowLeft.y + rArrowLeft.h + area*28)
		{
			int sliderWidth = rArrowRight.x - 1 - (rArrowLeft.x + rArrowLeft.w + 1);
			int clickXInsideSlider = mouseX - (rArrowLeft.x + rArrowLeft.w + 1);
			int wantedResearchForArea = (int) (((clickXInsideSlider * startResearchCenters) / (float)sliderWidth) + 0.5f);
			if (wantedResearchForArea <= newResearchSettings[area])
			{
				unusedResearch += newResearchSettings[area] - wantedResearchForArea;
				newResearchSettings[area] = wantedResearchForArea;
			}
			else
			{
				int wantedIncrement = wantedResearchForArea - newResearchSettings[area];
				int possibleIncrement = (wantedIncrement >= unusedResearch) ? unusedResearch : wantedIncrement;
				newResearchSettings[area] += possibleIncrement;
				unusedResearch -= possibleIncrement;
			}
			changed = true;
		}
	}

	if (changed)
	{
		ShowResearchSliders (newResearchSettings, startResearchCenters);
		SHOW_SCREEN
		mouse->draw (false, screen);
	}
}

//------------------------------------------------------------------------
void cBuilding::sendWantResearchChange (int newResearchSettings[cResearch::kNrResearchAreas])
{
	cNetMessage* msg = new cNetMessage (GAME_EV_WANT_RESEARCH_CHANGE);
	for (int area = 0; area < cResearch::kNrResearchAreas; area++)
		msg->pushInt16 (newResearchSettings[area]);
	msg->pushInt16 (owner->Nr);
	Client->sendNetMessage (msg);
}

//------------------------------------------------------------------------
/** Display and handle the upgrade screen. */
//------------------------------------------------------------------------
void cBuilding::ShowUpgrade ()
{
	int lastMouseX = 0;
	int lastMouseY = 0;
	int x = 0;
	int y = 0;
	int lastLeftButtonPressed = 0;
	int leftButtonPressed = 0;
	SDL_Rect src, dest;
	bool showDescription = SettingsData.bShowDescription;
	bool downPressed = false; // is the down-button currently pressed (for navigating down the list of units)
	bool upPressed = false; // is the up-button currently pressed (for navigating up the list of units)
	int selected = 0;
	int offset = 0;
	int startCredits = owner->Credits;
	int curCredits = startCredits;
	Client->isInMenu = true;

#define BUTTON__W 77
#define BUTTON__H 23

	SDL_Rect rDialog = { MENU_OFFSET_X, MENU_OFFSET_Y, DIALOG_W, DIALOG_H };
	SDL_Rect rTitle = { MENU_OFFSET_X + 330, MENU_OFFSET_Y + 11, 154, 13 };
	SDL_Rect rTxtDescription = { MENU_OFFSET_X + 141, MENU_OFFSET_Y + 266, 150, 13 };

	mouse->SetCursor (CHand);
	mouse->draw (false, buffer);
	SDL_BlitSurface (GraphicsData.gfx_upgrade, NULL, buffer, &rDialog);
	
	NormalButton btn_cancel (MENU_OFFSET_X + 360, MENU_OFFSET_Y + 452, "Text~Button~Cancel");
	NormalButton btn_done (MENU_OFFSET_X + 447, MENU_OFFSET_Y + 452, "Text~Button~Done");
	btn_cancel.Draw ();
	btn_done.Draw ();

	font->showTextCentered (rTitle.x + rTitle.w / 2, rTitle.y, lngPack.i18n ("Text~Title~Updates"));
	font->showTextCentered (rTxtDescription.x + rTxtDescription.w / 2, rTxtDescription.y, lngPack.i18n ("Text~Comp~Description"));

	// the description checkbox
	if (showDescription)
	{
		src.x = 291;
		src.y = 264;
		dest.x = MENU_OFFSET_X + 291;
		dest.y = MENU_OFFSET_Y + 264;
		src.w = 17;
		src.h = 17;
		SDL_BlitSurface ( GraphicsData.gfx_upgrade, &src, buffer, &dest );
	}
	else
	{
		src.x = 393;
		src.y = 46;
		dest.x = MENU_OFFSET_X + 291;
		dest.y = MENU_OFFSET_Y + 264;
		src.w = 18;
		src.h = 17;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	}

	// Initialize the upgrade structs for all unit types
	cList<sUpgradeStruct*> allUpgradeStructs;

	float newZoom = Client->Hud.Zoom / 64.0f;

	// create and add upgrade structs for all vehicles
	for (unsigned int vehicleTypeIdx = 0; vehicleTypeIdx < UnitsData.vehicle.Size (); vehicleTypeIdx++)
	{
		SDL_Surface *sf;
		scaleSurface ( UnitsData.vehicle[vehicleTypeIdx].img_org[0], UnitsData.vehicle[vehicleTypeIdx].img[0],
					   UnitsData.vehicle[vehicleTypeIdx].img_org[0]->w / 2, UnitsData.vehicle[vehicleTypeIdx].img_org[0]->h / 2 );
		sf = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, UnitsData.vehicle[vehicleTypeIdx].img[0]->w, UnitsData.vehicle[vehicleTypeIdx].img[0]->h, 32, 0, 0, 0, 0 );
		SDL_SetColorKey ( sf, SDL_SRCCOLORKEY, 0xFF00FF );
		SDL_BlitSurface ( Client->ActivePlayer->color, NULL, sf, NULL );
		SDL_BlitSurface ( UnitsData.vehicle[vehicleTypeIdx].img[0], NULL, sf, NULL );
		scaleSurface ( UnitsData.vehicle[vehicleTypeIdx].img_org[0], UnitsData.vehicle[vehicleTypeIdx].img[0], 
					   (int) (UnitsData.vehicle[vehicleTypeIdx].img_org[0]->w * newZoom), (int) (UnitsData.vehicle[vehicleTypeIdx].img_org[0]->h * newZoom) );
		sUpgradeStruct* upgradeStruct = new sUpgradeStruct(sf, true, vehicleTypeIdx);
		initUpgradesVehicle (upgradeStruct->upgrades, vehicleTypeIdx);
		allUpgradeStructs.Add (upgradeStruct);
	}

	// create and add upgrade structs for all buildings
	for (unsigned int  buildingTypeIdx = 0; buildingTypeIdx < UnitsData.building.Size(); buildingTypeIdx++)
	{
		SDL_Surface *sf;
		if (UnitsData.building[buildingTypeIdx].data.is_big)
		{
			scaleSurface (UnitsData.building[buildingTypeIdx].img_org, 
						  UnitsData.building[buildingTypeIdx].img, 
						  UnitsData.building[buildingTypeIdx].img_org->w / 4, 
						  UnitsData.building[buildingTypeIdx].img_org->h / 4);
		}
		else
		{
			scaleSurface (UnitsData.building[buildingTypeIdx].img_org, 
						  UnitsData.building[buildingTypeIdx].img, 
						  UnitsData.building[buildingTypeIdx].img_org->w / 2, 
						  UnitsData.building[buildingTypeIdx].img_org->h / 2);
		}
		sf = SDL_CreateRGBSurface (SDL_SRCCOLORKEY, 
								   UnitsData.building[buildingTypeIdx].img->w, 
								   UnitsData.building[buildingTypeIdx].img->h, 
								   32, 0, 0, 0, 0 );
		SDL_SetColorKey (sf, SDL_SRCCOLORKEY, 0xFF00FF);
		if (UnitsData.building[buildingTypeIdx].data.is_connector == false && UnitsData.building[buildingTypeIdx].data.is_road == false)
			SDL_BlitSurface (Client->ActivePlayer->color, NULL, sf, NULL);
		else
			SDL_FillRect (sf, NULL, 0xFF00FF);
		
		SDL_BlitSurface (UnitsData.building[buildingTypeIdx].img, NULL, sf, NULL);
		scaleSurface (UnitsData.building[buildingTypeIdx].img_org, 
					  UnitsData.building[buildingTypeIdx].img, 
					  (int) (UnitsData.building[buildingTypeIdx].img_org->w * newZoom), 
					  (int) (UnitsData.building[buildingTypeIdx].img_org->h * newZoom));
		sUpgradeStruct* upgradeStruct = new sUpgradeStruct(sf, false, buildingTypeIdx);
		initUpgradesBuilding (upgradeStruct->upgrades, buildingTypeIdx);
		allUpgradeStructs.Add (upgradeStruct);
	}

	
	cList<sUpgradeStruct*> selection;

	// create the list of units (displayed at the right side), depending on the state of the client-flags bUpShowTank, bUpShowPlane, bUpShowShip, bUpShowBuild, bUpShowTNT
	CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
	// display the list of units selectable for upgrading and the values of the currently selected unit 
	ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
	// display the buttons for selecting unit subcategories
	ShowUpgradeSubButtons ();

	// show the available credits
	ShowGoldBar (startCredits, curCredits);

	// display the buffer
	SHOW_SCREEN
	mouse->GetBack (buffer);
	mouse->MoveCallback = false;

	// exit, when cancle or done was clicked by the player
	while (1)
	{
//		if (Client->SelectedBuilding == NULL)
//			break;
		if (  Client->SelectedBuilding != this ) break;
		if ( !Client->isInMenu ) break;

		Client->handleTimer ();
		Client->doGameActions ();

		// get the events
		EventHandler->HandleEvents();

		// udpate the mouse
		mouse->GetPos();

		leftButtonPressed = (int)Client->getMouseState().leftButtonPressed;
		x = mouse->x;
		y = mouse->y;

		if (x != lastMouseX || y != lastMouseY) // mouse moved
			mouse->draw (true, screen);

		// the Down-Button was pressed (to navigate down the list of units)
		if (x >= MENU_OFFSET_X + 491 && x < MENU_OFFSET_X + 491 + 18 
			&& y >= MENU_OFFSET_Y + 386 && y < MENU_OFFSET_Y + 386 + 17 
			&& leftButtonPressed && !downPressed)
		{
			int leftSpace = (int) selection.Size () - 9 - offset;
			if (leftSpace != 0)
			{
				if (leftSpace >= 9)
					offset += 9;
				else if (leftSpace > 0)
					offset += leftSpace;
				if (selected < offset)
					selected = offset;
				ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			}

			PlayFX (SoundData.SNDObjectMenu);
			src.x = 249;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 491;
			dest.y = MENU_OFFSET_Y + 386;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
			SHOW_SCREEN
			mouse->draw (false, screen);
			downPressed = true;
		}
		else if (!leftButtonPressed && downPressed/* && (offset < selection.Size () - 9)*/) // down button was released -> reset the button graphics
		{
			src.x = 491;
			src.y = 386;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 491;
			dest.y = MENU_OFFSET_Y + 386;
			SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
			SHOW_SCREEN
			mouse->draw (false, screen);
			downPressed = false;
		}

		// the Up-Button was pressed (to navigate up the list of units)
		if (x >= MENU_OFFSET_X + 470 && x < MENU_OFFSET_X + 470 + 18 
			&& y >= MENU_OFFSET_Y + 386 && y < MENU_OFFSET_Y + 386 + 17 
			&& leftButtonPressed && !upPressed)
		{
			if (offset > 0)
			{
				if (offset >= 9)
					offset -= 9;
				else
					offset = 0;
				if (selected >= offset + 9)
					selected = offset + 8;
				ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			}

			PlayFX (SoundData.SNDObjectMenu);
			src.x = 230;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 470;
			dest.y = MENU_OFFSET_Y + 386;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
			SHOW_SCREEN
			mouse->draw (false, screen);
			upPressed = true;
		}
		else if (!leftButtonPressed && upPressed/* && offset > 0*/) // up button was released -> reset the button graphics
		{
			src.x = 470;
			src.y = 386;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 470;
			dest.y = MENU_OFFSET_Y + 386;
			SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
			SHOW_SCREEN
			mouse->draw (false, screen);
			upPressed = false;
		}

		const bool down = (leftButtonPressed > lastLeftButtonPressed); // TODO: refactor this strange mouseUp and mouseDown handling...
		const bool up   = (leftButtonPressed < lastLeftButtonPressed);

		// the user clicked the cancle button!
		if (btn_cancel.CheckClick (x, y, down, up))
		{
			break;
		}

		// the user clicked the done button => send the upgrades to the server
		if (btn_done.CheckClick(x, y, down, up))
		{
			sendUpgrades (allUpgradeStructs, owner);
			break;
		}

		// Description checkbox:
		if (x >= MENU_OFFSET_X + 292 && x < MENU_OFFSET_X + 292 + 16 
			&& y >= MENU_OFFSET_Y + 265 && y < MENU_OFFSET_Y + 265 + 15 
			&& leftButtonPressed && !lastLeftButtonPressed)
		{
			PlayFX (SoundData.SNDObjectMenu);
			showDescription = !showDescription;
			SettingsData.bShowDescription = showDescription;

			if (showDescription)
			{
				src.x = 291;
				src.y = 264;
				dest.x = MENU_OFFSET_X + 291;
				dest.y = MENU_OFFSET_Y + 264;
				src.w = 17;
				src.h = 17;
				SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
			}
			else
			{
				src.x = 393;
				src.y = 46;
				dest.x = MENU_OFFSET_X + 291;
				dest.y = MENU_OFFSET_Y + 264;
				src.w = 18;
				src.h = 17;
				SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
			}

			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			SHOW_SCREEN
			mouse->draw (false, screen);
		}

		// Click inside the list of upgradeable units (user selects another unit)
		if (x >= MENU_OFFSET_X + 490 && x < MENU_OFFSET_X + 490 + 70 
			&& y >= MENU_OFFSET_Y + 60 && y < MENU_OFFSET_Y + 60 + 315 
			&& leftButtonPressed && !lastLeftButtonPressed)
		{
			int nr;
			nr = (y - MENU_OFFSET_Y - 60) / (32 + 2);

			if (selection.Size() < 9)
			{
				if (nr >= (int)selection.Size ())
					nr = -1;
			}
			else
			{
				if (nr >= 10)
					nr = -1;
				nr += offset;
			}

			if (nr != -1)
			{
				PlayFX (SoundData.SNDObjectMenu);
				selected = nr;
				ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
				SHOW_SCREEN
				mouse->draw (false, screen);
			}
		}

		// User clicks on an upgrade-slider
		if (leftButtonPressed && !lastLeftButtonPressed 
			&& x >= MENU_OFFSET_X + 283 && x < MENU_OFFSET_X + 301 + 18 
			&& selection.Size ())
		{
			sUpgradeStruct* upgradeStructSelectedUnit = selection[selected];

			for (int i = 0; i < 8; i++)
			{
				if (!upgradeStructSelectedUnit->upgrades[i].active)
					continue;

				// left slider was pressed (revert the upgrade)
				if (upgradeStructSelectedUnit->upgrades[i].purchased 
					&& x < MENU_OFFSET_X + 283 + 18 && y >= MENU_OFFSET_Y + 293 + i*19 && y < MENU_OFFSET_Y + 293 + i*19 + 19)
				{
					int upgradeType = -1;
					if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Hitpoints")) == 0)
						upgradeType = cUpgradeCalculator::kHitpoints;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Armor")) == 0)
						upgradeType = cUpgradeCalculator::kArmor;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Ammo")) == 0)
						upgradeType = cUpgradeCalculator::kAmmo;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Damage")) == 0)
						upgradeType = cUpgradeCalculator::kAttack;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Speed")) == 0)
						upgradeType = cUpgradeCalculator::kSpeed;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Shots")) == 0)
						upgradeType = cUpgradeCalculator::kShots;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Range")) == 0)
						upgradeType = cUpgradeCalculator::kRange;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Scan")) == 0)
						upgradeType = cUpgradeCalculator::kScan;

					cUpgradeCalculator& uc = cUpgradeCalculator::instance();
					if (upgradeType != cUpgradeCalculator::kSpeed)
					{
						upgradeStructSelectedUnit->upgrades[i].curValue -= uc.calcIncreaseByUpgrade (upgradeStructSelectedUnit->upgrades[i].startValue);
						upgradeStructSelectedUnit->upgrades[i].nextPrice = uc.calcPrice (upgradeStructSelectedUnit->upgrades[i].curValue, 
																						 upgradeStructSelectedUnit->upgrades[i].startValue, upgradeType, owner->researchLevel);
					}
					else
					{
						upgradeStructSelectedUnit->upgrades[i].curValue -= 4 * uc.calcIncreaseByUpgrade (upgradeStructSelectedUnit->upgrades[i].startValue / 4);
						upgradeStructSelectedUnit->upgrades[i].nextPrice = uc.calcPrice (upgradeStructSelectedUnit->upgrades[i].curValue / 4, 
																						 upgradeStructSelectedUnit->upgrades[i].startValue / 4, upgradeType, owner->researchLevel);
					}

					curCredits += upgradeStructSelectedUnit->upgrades[i].nextPrice;

					upgradeStructSelectedUnit->upgrades[i].purchased--;

					PlayFX (SoundData.SNDObjectMenu);
					ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
					ShowGoldBar (startCredits, curCredits);
					SHOW_SCREEN
					mouse->draw (false, screen);
					break;
				}
				else if (upgradeStructSelectedUnit->upgrades[i].nextPrice <= curCredits && upgradeStructSelectedUnit->upgrades[i].nextPrice > 0
						 && x >= MENU_OFFSET_X + 301 && y >= MENU_OFFSET_Y + 293 + i*19 && y < MENU_OFFSET_Y + 293 + i*19 + 19) // upgrade kaufen (slider rechts geclickt)
				{
					curCredits -= upgradeStructSelectedUnit->upgrades[i].nextPrice;

					int upgradeType = -1;
					if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Hitpoints")) == 0)
						upgradeType = cUpgradeCalculator::kHitpoints;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Armor")) == 0)
						upgradeType = cUpgradeCalculator::kArmor;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Ammo")) == 0)
						upgradeType = cUpgradeCalculator::kAmmo;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Damage")) == 0)
						upgradeType = cUpgradeCalculator::kAttack;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Speed")) == 0)
						upgradeType = cUpgradeCalculator::kSpeed;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Shots")) == 0)
						upgradeType = cUpgradeCalculator::kShots;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Range")) == 0)
						upgradeType = cUpgradeCalculator::kRange;
					else if (upgradeStructSelectedUnit->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Scan")) == 0)
						upgradeType = cUpgradeCalculator::kScan;

					cUpgradeCalculator& uc = cUpgradeCalculator::instance();
					if (upgradeType != cUpgradeCalculator::kSpeed)
					{
						upgradeStructSelectedUnit->upgrades[i].curValue += uc.calcIncreaseByUpgrade (upgradeStructSelectedUnit->upgrades[i].startValue);
						upgradeStructSelectedUnit->upgrades[i].nextPrice = uc.calcPrice (upgradeStructSelectedUnit->upgrades[i].curValue, 
																						 upgradeStructSelectedUnit->upgrades[i].startValue, upgradeType, owner->researchLevel);
					}
					else
					{
						upgradeStructSelectedUnit->upgrades[i].curValue += 4 * uc.calcIncreaseByUpgrade (upgradeStructSelectedUnit->upgrades[i].startValue / 4);
						upgradeStructSelectedUnit->upgrades[i].nextPrice = uc.calcPrice (upgradeStructSelectedUnit->upgrades[i].curValue / 4, 
																						 upgradeStructSelectedUnit->upgrades[i].startValue / 4, upgradeType, owner->researchLevel);
					}

					upgradeStructSelectedUnit->upgrades[i].purchased++;

					PlayFX (SoundData.SNDObjectMenu);
					ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
					ShowGoldBar (startCredits, curCredits);
					SHOW_SCREEN
					mouse->draw (false, screen);
					break;
				}
			}
		}

		// click on a subcategory selection button (e.g. "show ships")
		if (leftButtonPressed && !lastLeftButtonPressed 
			&& x >= MENU_OFFSET_X + 467 && x < MENU_OFFSET_X + 467 + 32 && y >= MENU_OFFSET_Y + 411 && y < MENU_OFFSET_Y + 411 + 31)
		{
			PlayFX (SoundData.SNDHudSwitch);
			Client->bUpShowTank = !Client->bUpShowTank;
			CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			ShowUpgradeSubButtons ();
			SHOW_SCREEN
			mouse->draw (false, screen);
		}
		else if (leftButtonPressed && !lastLeftButtonPressed 
				 && x >= MENU_OFFSET_X + 467 + 33 && x < MENU_OFFSET_X + 467 + 32 + 33 && y >= MENU_OFFSET_Y + 411 && y < MENU_OFFSET_Y + 411 + 31)
		{
			PlayFX (SoundData.SNDHudSwitch);
			Client->bUpShowPlane = !Client->bUpShowPlane;
			CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			ShowUpgradeSubButtons ();
			SHOW_SCREEN
			mouse->draw (false, screen);
		}
		else if (leftButtonPressed && !lastLeftButtonPressed 
				 && x >= MENU_OFFSET_X + 467 + 33*2 && x < MENU_OFFSET_X + 467 + 32 + 33*2 && y >= MENU_OFFSET_Y + 411 && y < MENU_OFFSET_Y + 411 + 31)
		{
			PlayFX (SoundData.SNDHudSwitch);
			Client->bUpShowShip = !Client->bUpShowShip;
			CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			ShowUpgradeSubButtons ();
			SHOW_SCREEN
			mouse->draw (false, screen);
		}
		else if (leftButtonPressed && !lastLeftButtonPressed 
				 && x >= MENU_OFFSET_X + 467 + 33*3 && x < MENU_OFFSET_X + 467 + 32 + 33*3 && y >= MENU_OFFSET_Y + 411 && y < MENU_OFFSET_Y + 411 + 31)
		{
			PlayFX (SoundData.SNDHudSwitch);
			Client->bUpShowBuild = !Client->bUpShowBuild;
			CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			ShowUpgradeSubButtons ();
			SHOW_SCREEN
			mouse->draw (false, screen);
		}
		else if (leftButtonPressed && !lastLeftButtonPressed 
				 && x >= MENU_OFFSET_X + 467 + 33*4 && x < MENU_OFFSET_X + 467 + 32 + 33*4 && y >= MENU_OFFSET_Y + 411 && y < MENU_OFFSET_Y + 411 + 31)
		{
			PlayFX (SoundData.SNDHudSwitch);
			Client->bUpShowTNT = !Client->bUpShowTNT;
			CreateUpgradeList (selection, allUpgradeStructs, selected, offset);
			ShowUpgradeList (selection, selected, offset, showDescription, curCredits);
			ShowUpgradeSubButtons ();
			SHOW_SCREEN
			mouse->draw (false, screen);
		}

		lastMouseX = x;
		lastMouseY = y;
		lastLeftButtonPressed = leftButtonPressed;
	}

	// clean up
	while (allUpgradeStructs.Size())
	{
		sUpgradeStruct *ptr;
		ptr = allUpgradeStructs[0];
		SDL_FreeSurface ( ptr->sf );
		delete ptr;
		allUpgradeStructs.Delete ( 0 );
	}

	mouse->MoveCallback = true;
	Client->isInMenu = false;
}

//-------------------------------------------------------------------------------
/** Displays the list of units selectable for upgrading and the values of the currently selected unit. */
//-------------------------------------------------------------------------------
void cBuilding::ShowUpgradeList (cList<sUpgradeStruct*>& list, int const selected, int const offset, bool const showDescription, int curCredits)
{
	sUpgradeStruct *ptr;
	SDL_Rect dest, src, text = { MENU_OFFSET_X + 530, MENU_OFFSET_Y + 70, 80, 0 };
	src.x = 479;
	src.y = 52;
	dest.x = MENU_OFFSET_X + 479;
	dest.y = MENU_OFFSET_Y + 52;
	src.w = 150;
	src.h = 330;
	SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	src.x = 0;
	src.y = 0;
	src.w = 32;
	src.h = 32;
	dest.x = MENU_OFFSET_X + 490;
	dest.y = MENU_OFFSET_Y + 58;

	if (list.Size () == 0)
	{
		src.x = 0;
		src.y = 0;
		dest.x = MENU_OFFSET_X;
		dest.y = MENU_OFFSET_Y;
		src.w = 316;
		src.h = 256;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
		src.x = 11;
		src.y = 290;
		dest.x = MENU_OFFSET_X + 11;
		dest.y = MENU_OFFSET_Y + 290;
		src.w = 346;
		src.h = 176;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
		return;
	}

	for (unsigned int i = offset; i < list.Size (); i++)
	{
		if ((int)i >= offset + 9)
			break;

		// Das Bild malen:
		ptr = list[i];

		SDL_BlitSurface (ptr->sf, &src, buffer, &dest);

		if (selected == i)
		{
			// Ggf noch Rahmen drum:
			SDL_Rect tmp, tmp2;
			tmp = dest;
			tmp.x -= 4;
			tmp.y -= 4;
			tmp.h = 1;
			tmp.w = 8;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.x += 30;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.y += 38;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.x -= 30;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.y = dest.y - 4;
			tmp.w = 1;
			tmp.h = 8;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.x += 38;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.y += 31;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			tmp.x -= 38;
			SDL_FillRect (buffer, &tmp, 0xE0E0E0);
			// Das Bild neu malen:
			tmp.x = MENU_OFFSET_X + 11;
			tmp.y = MENU_OFFSET_Y + 13;

			if (ptr->vehicle)
			{
				tmp.w = UnitsData.vehicle[ptr->id].info->w;
				tmp.h = UnitsData.vehicle[ptr->id].info->h;
				SDL_BlitSurface (UnitsData.vehicle[ptr->id].info, NULL, buffer, &tmp);
			}
			else
			{
				tmp.w = UnitsData.building[ptr->id].info->w;
				tmp.h = UnitsData.building[ptr->id].info->h;
				SDL_BlitSurface (UnitsData.building[ptr->id].info, NULL, buffer, &tmp);
			}

			// Ggf die Beschreibung ausgeben:
			if (showDescription)
			{
				tmp.x += 10;
				tmp.y += 10;
				tmp.w -= 20;
				tmp.h -= 20;

				if (ptr->vehicle)
				{
					font->showTextAsBlock (tmp, UnitsData.vehicle[ptr->id].text);

				}
				else
				{
					font->showTextAsBlock (tmp, UnitsData.building[ptr->id].text);

				}
			}

			// Die Details anzeigen:
			{
				tmp.x = 11;
				tmp.y = 290;
				tmp2.x = MENU_OFFSET_X + 11;
				tmp2.y = MENU_OFFSET_Y + 290;
				tmp.w = tmp2.w = 346;
				tmp.h = tmp2.h = 176;
				SDL_BlitSurface (GraphicsData.gfx_upgrade, &tmp, buffer, &tmp2);

				sUpgradeStruct* selectedUpgrade = list[selected];
				if (ptr->vehicle)
				{
					cVehicle tempVeh (&UnitsData.vehicle[ptr->id], Client->ActivePlayer);
					// now set the current values (which are already locally upgraded) for tempVeh 
					for (int upgradeType = 0; upgradeType < 8; upgradeType++)
					{
						if (selectedUpgrade->upgrades[upgradeType].active)
						{
							if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Hitpoints")) == 0)
								tempVeh.data.max_hit_points = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Armor")) == 0)
								tempVeh.data.armor = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Ammo")) == 0)
								tempVeh.data.max_ammo = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Damage")) == 0)
								tempVeh.data.damage = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Speed")) == 0)
								tempVeh.data.max_speed = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Shots")) == 0)
								tempVeh.data.max_shots = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Range")) == 0)
								tempVeh.data.range = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Scan")) == 0)
								tempVeh.data.scan = selectedUpgrade->upgrades[upgradeType].curValue;
						}
					}
					tempVeh.ShowBigDetails ();
				}
				else
				{
					cBuilding tempBuild (&UnitsData.building[ptr->id], Client->ActivePlayer, NULL);
					// now set the current values (which are already locally upgraded) for tempBuild 
					for (int upgradeType = 0; upgradeType < 8; upgradeType++)
					{
						if (selectedUpgrade->upgrades[upgradeType].active)
						{
							if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Hitpoints")) == 0)
								tempBuild.data.max_hit_points = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Armor")) == 0)
								tempBuild.data.armor = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Ammo")) == 0)
								tempBuild.data.max_ammo = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Damage")) == 0)
								tempBuild.data.damage = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Speed")) == 0)
								tempBuild.data.max_speed = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Shots")) == 0)
								tempBuild.data.max_shots = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Range")) == 0)
								tempBuild.data.range = selectedUpgrade->upgrades[upgradeType].curValue;
							else if (selectedUpgrade->upgrades[upgradeType].name.compare (lngPack.i18n ("Text~Vehicles~Scan")) == 0)
								tempBuild.data.scan = selectedUpgrade->upgrades[upgradeType].curValue;
						}
					}
					tempBuild.ShowBigDetails();
				}
			}

			// Die Texte anzeigen/Slider machen:
			for (int k = 0;k < 8;k++)
			{
				SDL_Rect src, dest;

				if (!ptr->upgrades[k].active)
					continue;

				//sprintf (str,"%d",ptr->upgrades[k].NextPrice);

				if (ptr->upgrades[k].nextPrice > 0)
					font->showText (MENU_OFFSET_X + 322, MENU_OFFSET_Y + 296 + k*19, iToStr (ptr->upgrades[k].nextPrice));

				if (ptr->upgrades[k].purchased)
				{
					src.x = 380;
					src.y = 256;
					src.w = 18;
					src.h = 17;
					dest.x = MENU_OFFSET_X + 283;
					dest.y = MENU_OFFSET_Y + 293 + k * 19;
					SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
				}

				if (ptr->upgrades[k].nextPrice <= curCredits && ptr->upgrades[k].nextPrice > 0)
				{
					src.x = 399;
					src.y = 256;
					src.w = 18;
					src.h = 17;
					dest.x = MENU_OFFSET_X + 301;
					dest.y = MENU_OFFSET_Y + 293 + k * 19;
					SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
				}
			}
		}

		// Text ausgeben:
		string sTmp;

		if (ptr->vehicle)
		{
			sTmp = UnitsData.vehicle[ptr->id].data.name;
		}
		else
		{
			sTmp = UnitsData.building[ptr->id].data.name;
		}


		if (font->getTextWide (sTmp, FONT_LATIN_SMALL_WHITE) > text.w)
		{
			text.y -= font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
			font->showTextAsBlock (text, sTmp, FONT_LATIN_SMALL_WHITE);
			text.y += font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
		}
		else
		{
			font->showText (text, sTmp, FONT_LATIN_SMALL_WHITE);
		}

		text.y += 32 + 2;
		dest.y += 32 + 2;
	}
}

//-------------------------------------------------------------------------------
/** Displays the available credits as a gold bar and a number. */
//-------------------------------------------------------------------------------
void cBuilding::ShowGoldBar (int startCredits, int curCredits)
{
	//char str[50];
	SDL_Rect src, dest;
	src.x = 371;
	src.y = 301;
	dest.x = MENU_OFFSET_X + 371;
	dest.y = MENU_OFFSET_Y + 301;
	src.w = 22;
	src.h = 115;
	SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	src.x = 312;
	src.y = 265;
	dest.x = MENU_OFFSET_X + 312;
	dest.y = MENU_OFFSET_Y + 265;
	src.w = 150;
	src.h = 26;
	SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	//sprintf (str,"Credits: %d", curCredits);

	font->showTextCentered (MENU_OFFSET_X + 381, MENU_OFFSET_Y + 275, "Credits: " + iToStr (curCredits));

	src.x = 118;
	src.y = 336;
	src.w = 16;
	src.h = (int) (115 * (curCredits / (float) startCredits));
	dest.x = MENU_OFFSET_X + 375;
	dest.y = MENU_OFFSET_Y + 301 + 115 - src.h;
	SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
}

//-------------------------------------------------------------------------------
/** initializes the upgrades in u[] for the given vehicle type by setting the available
	upgrade sliders and calculating the current price for the next upgrade */
//-------------------------------------------------------------------------------
void cBuilding::initUpgradesVehicle(sUpgradeNew u[], int vehicleTypeIdx)
{
	cResearch& researchLevel = owner->researchLevel;
	
	// initialize the upgrades with empty values
	for (int i = 0; i < 8; i++)
	{
		u[i].active = false;
		u[i].purchased = 0;
		u[i].curValue = -1;
	}

	sUnitData& currentVersion = owner->VehicleData[vehicleTypeIdx]; // get the owner's current version of the unitData
	const sUnitData& startVersion = UnitsData.vehicle[vehicleTypeIdx].data;
	
	int i = 0;

	if (currentVersion.can_attack)
	{
		// Damage:
		u[i].active = true;
		u[i].startValue = startVersion.damage;
		u[i].curValue = currentVersion.damage;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.damage, startVersion.damage, cUpgradeCalculator::kAttack, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Damage");
		i++;
		// Shots:
		u[i].active = true;
		u[i].startValue = startVersion.max_shots;
		u[i].curValue = currentVersion.max_shots;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_shots, startVersion.max_shots, cUpgradeCalculator::kShots, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Shots");
		i++;
		// Range:
		u[i].active = true;
		u[i].startValue = startVersion.range;
		u[i].curValue = currentVersion.range;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.range, startVersion.range, cUpgradeCalculator::kRange, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Range");
		i++;
		// Ammo:
		u[i].active = true;
		u[i].startValue = startVersion.max_ammo;
		u[i].curValue = currentVersion.max_ammo;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_ammo, startVersion.max_ammo, cUpgradeCalculator::kAmmo, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Ammo");
		i++;
	}

	if (currentVersion.can_transport == TRANS_METAL || currentVersion.can_transport == TRANS_OIL || currentVersion.can_transport == TRANS_GOLD)
		i++;

	// Armor:
	u[i].active = true;
	u[i].startValue = startVersion.armor;
	u[i].curValue = currentVersion.armor;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.armor, startVersion.armor, cUpgradeCalculator::kArmor, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Armor");
	i++;

	// Hitpoints:
	u[i].active = true;
	u[i].startValue = startVersion.max_hit_points;
	u[i].curValue = currentVersion.max_hit_points;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_hit_points, startVersion.max_hit_points, cUpgradeCalculator::kHitpoints, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Hitpoints");
	i++;

	// Scan:
	u[i].active = true;
	u[i].startValue = startVersion.scan;
	u[i].curValue = currentVersion.scan;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.scan, startVersion.scan, cUpgradeCalculator::kScan, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Scan");
	i++;

	// Speed:
	u[i].active = true;
	u[i].startValue = startVersion.max_speed;
	u[i].curValue = currentVersion.max_speed;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_speed / 4, startVersion.max_speed / 4, cUpgradeCalculator::kSpeed, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Speed");
	i++;

	// Costs:
	i++;

//	for (i = 0; i < 8; i++)
//	{
//		if (u[i].curValue == -1)
//			continue;
//
//		u[i].startValue = u[i].curValue;
//	}
}

//-------------------------------------------------------------------------------
/** initializes the upgrades in u[] for the given building type by setting the available
	upgrade sliders and calculating the current price for the next upgrade */
//-------------------------------------------------------------------------------
void cBuilding::initUpgradesBuilding (sUpgradeNew u[], int buildingTypeIdx)
{
	cResearch& researchLevel = owner->researchLevel;

	// initialize the upgrades with empty values
	for (int i = 0; i < 8; i++)
	{
		u[i].active = false;
		u[i].purchased = 0;
		u[i].curValue = -1;
	}

	sUnitData& currentVersion = owner->BuildingData[buildingTypeIdx]; // get the owner's current version of the unitData
	const sUnitData& startVersion = UnitsData.building[buildingTypeIdx].data;

	int i = 0;

	if (currentVersion.can_attack)
	{
		// Damage:
		u[i].active = true;
		u[i].startValue = startVersion.damage;
		u[i].curValue = currentVersion.damage;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.damage, startVersion.damage, cUpgradeCalculator::kAttack, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Damage");
		i++;

		if (!currentVersion.is_expl_mine)
		{
			// Shots:
			u[i].active = true;
			u[i].startValue = startVersion.max_shots;
			u[i].curValue = currentVersion.max_shots;
			u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_shots, startVersion.max_shots, cUpgradeCalculator::kShots, researchLevel);
			u[i].name = lngPack.i18n ("Text~Vehicles~Shots");
			i++;
			// Range:
			u[i].active = true;
			u[i].startValue = startVersion.range;
			u[i].curValue = currentVersion.range;
			u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.range, startVersion.range, cUpgradeCalculator::kRange, researchLevel);
			u[i].name = lngPack.i18n ("Text~Vehicles~Range");
			i++;
			// Ammo:
			u[i].active = true;
			u[i].startValue = startVersion.max_ammo;			
			u[i].curValue = currentVersion.max_ammo;
			u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_ammo, startVersion.max_ammo, cUpgradeCalculator::kAmmo, researchLevel);
			u[i].name = lngPack.i18n ("Text~Vehicles~Ammo");
			i++;
		}
	}

	if (currentVersion.can_load == TRANS_METAL || currentVersion.can_load == TRANS_OIL || currentVersion.can_load == TRANS_GOLD)
		i++;

	if (currentVersion.energy_prod)
		i += 2;

	if (currentVersion.human_prod)
		i++;

	// Armor:
	u[i].active = true;
	u[i].startValue = startVersion.armor;
	u[i].curValue = currentVersion.armor;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.armor, startVersion.armor, cUpgradeCalculator::kArmor, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Armor");
	i++;
	// Hitpoints:
	u[i].active = true;
	u[i].startValue = startVersion.max_hit_points;
	u[i].curValue = currentVersion.max_hit_points;
	u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.max_hit_points, startVersion.max_hit_points, cUpgradeCalculator::kHitpoints, researchLevel);
	u[i].name = lngPack.i18n ("Text~Vehicles~Hitpoints");
	i++;
	// Scan:
	if (currentVersion.scan)
	{
		u[i].active = true;
		u[i].startValue = startVersion.scan;
		u[i].curValue = currentVersion.scan;
		u[i].nextPrice = cUpgradeCalculator::instance().calcPrice (currentVersion.scan, startVersion.scan, cUpgradeCalculator::kScan, researchLevel);
		u[i].name = lngPack.i18n ("Text~Vehicles~Scan");
		i++;
	}

	// energy consumption:
	if (currentVersion.energy_need)
		i++;

	// Human consumption:
	if (currentVersion.human_need)
		i++;

	// raw material consumption
	if (currentVersion.metal_need)
		i++;

	// Gold consumption
	if (currentVersion.gold_need)
		i++;

	// Costs:
	i++;

//	for (int i = 0; i < 8; i++)
//	{
//		if (u[i].curValue == -1)
//			continue;
//
//		u[i].startValue = u[i].curValue;
//	}
}

//-------------------------------------------------------------------------------
/** Constructs the list of upgradeStructs, that are currently selectable (depending on the states of the subselection-buttons). */
//-------------------------------------------------------------------------------
void cBuilding::CreateUpgradeList (cList<sUpgradeStruct*>& selection, cList<sUpgradeStruct*>& allUpgradeStructs, int& selected, int& offset)
{
	sUnitData *bd;
	sUnitData *vd;

	while (selection.Size())
	{
		selection.Delete (0);
	}

	for (unsigned int i = 0; i < allUpgradeStructs.Size(); i++)
	{
		if (allUpgradeStructs[i]->vehicle)
		{
			if (! (Client->bUpShowTank || Client->bUpShowShip || Client->bUpShowPlane))
				continue;

			vd = &UnitsData.vehicle[allUpgradeStructs[i]->id].data;

			if (Client->bUpShowTNT && !vd->can_attack)
				continue;

			if (vd->can_drive == DRIVE_AIR && !Client->bUpShowPlane)
				continue;

			if (vd->can_drive == DRIVE_SEA && !Client->bUpShowShip)
				continue;

			if ((vd->can_drive == DRIVE_LAND || vd->can_drive == DRIVE_LANDnSEA) && !Client->bUpShowTank)
				continue;

			selection.Add(allUpgradeStructs[i]);
		}
		else
		{
			if (!Client->bUpShowBuild)
				continue;

			bd = &UnitsData.building[allUpgradeStructs[i]->id].data;

			if (Client->bUpShowTNT && !bd->can_attack)
				continue;

			selection.Add(allUpgradeStructs[i]);
		}
	}

	if (offset >= (int)selection.Size () - 9)
	{
		offset = (int)selection.Size () - 9;
		if (offset < 0)
			offset = 0;
	}

	if (selected >= (int)selection.Size())
	{
		selected = (int)selection.Size() - 1;
		if (selected < 0)
			selected = 0;
	}
}

//-------------------------------------------------------------------------------
/** Displays the buttons for making subselections */
//-------------------------------------------------------------------------------
void cBuilding::ShowUpgradeSubButtons ()
{
	SDL_Rect src, dest;
	dest.x = MENU_OFFSET_X + 467;
	dest.y = MENU_OFFSET_Y + 411;
	src.w = 32;
	src.h = 31;
	// Tank:

	if (!Client->bUpShowTank)
	{
		src.x = 467;
		src.y = 411;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	}
	else
	{
		src.x = 152;
		src.y = 479;
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
	}

	dest.x += 33;
	// Plane:

	if (!Client->bUpShowPlane)
	{
		src.x = 467 + 33;
		src.y = 411;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	}
	else
	{
		src.x = 152 + 33;
		src.y = 479;
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
	}

	dest.x += 33;
	// Ship:

	if (!Client->bUpShowShip)
	{
		src.x = 467 + 66;
		src.y = 411;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	}
	else
	{
		src.x = 152 + 66;
		src.y = 479;
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
	}

	dest.x += 33;
	// Building:

	if (!Client->bUpShowBuild)
	{
		src.x = 467 + 99;
		src.y = 411;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	}
	else
	{
		src.x = 152 + 99;
		src.y = 479;
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
	}

	dest.x += 33;
	// TNT:

	if (!Client->bUpShowTNT)
	{
		src.x = 467 + 132;
		src.y = 411;
		SDL_BlitSurface (GraphicsData.gfx_upgrade, &src, buffer, &dest);
	}
	else
	{
		src.x = 152 + 132;
		src.y = 479;
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff, &src, buffer, &dest);
	}
}

//------------------------------------------------------------------------
void cBuilding::sendUpgrades (cList<sUpgradeStruct*>& allUpgradeStructs, cPlayer* player)
{	
	cNetMessage* msg = NULL;
	int iCount = 0;
	
	// send vehicles
	for (size_t i = 0; i < allUpgradeStructs.Size(); i++)
	{
		sUpgradeStruct* curUpgrade = allUpgradeStructs[i];
		bool purchased = (curUpgrade->upgrades[0].purchased || curUpgrade->upgrades[1].purchased || curUpgrade->upgrades[2].purchased
						  || curUpgrade->upgrades[3].purchased || curUpgrade->upgrades[4].purchased || curUpgrade->upgrades[5].purchased 
						  || curUpgrade->upgrades[6].purchased || curUpgrade->upgrades[7].purchased);
		if (purchased)
		{
			if (msg == NULL)
			{
				msg = new cNetMessage (GAME_EV_WANT_BUY_UPGRADES);
				iCount = 0;
			}
			
			if (curUpgrade->vehicle == true)
			{
				sUnitData& currentVersion = player->VehicleData[curUpgrade->id];
				
				msg->pushInt16 (findUpgradeValue (curUpgrade, 0, currentVersion.max_speed));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 1, currentVersion.scan));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 2, currentVersion.max_hit_points));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 3, currentVersion.armor));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 4, currentVersion.max_ammo));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 5, currentVersion.range));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 6, currentVersion.max_shots));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 7, currentVersion.damage));
				msg->pushInt16 (currentVersion.ID.iSecondPart);
				msg->pushInt16 (currentVersion.ID.iFirstPart);
				msg->pushBool (true); // true for vehicle
			}
			else // building
			{
				sUnitData& currentVersion = player->BuildingData[curUpgrade->id];
				
				msg->pushInt16 (findUpgradeValue (curUpgrade, 1, currentVersion.scan));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 2, currentVersion.max_hit_points));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 3, currentVersion.armor));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 4, currentVersion.max_ammo));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 5, currentVersion.range));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 6, currentVersion.max_shots));
				msg->pushInt16 (findUpgradeValue (curUpgrade, 7, currentVersion.damage));
				msg->pushInt16 (currentVersion.ID.iSecondPart);
				msg->pushInt16 (currentVersion.ID.iFirstPart);
				msg->pushBool (false); // false for building				
			}	
			iCount++; // msg contains one more upgrade struct
			
			// the msg would be too long, if another upgrade would be written into it. So send it and put the next upgrades in a new message.
			if (msg->iLength + 38 > PACKAGE_LENGTH) 
			{
				msg->pushInt16 (iCount);
				msg->pushInt16 (player->Nr);
				Client->sendNetMessage (msg);
				msg = NULL;
			}
			
		}
	}
	if (msg != NULL)
	{
		msg->pushInt16 (iCount);
		msg->pushInt16 (player->Nr);
		Client->sendNetMessage (msg);
	}
}

//------------------------------------------------------------------------
int cBuilding::findUpgradeValue (sUpgradeStruct* upgradeStruct, int upgradeType, int defaultValue)
{
	switch (upgradeType)
	{
		case 0:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Speed")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 1:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Scan")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 2:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Hitpoints")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 3:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Armor")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 4:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Ammo")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 5:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Range")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 6:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Shots")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
		case 7:
			for (int i = 0; i < 8; i++)
			{
				if (upgradeStruct->upgrades[i].active && upgradeStruct->upgrades[i].name.compare (lngPack.i18n ("Text~Vehicles~Damage")) == 0)
					return upgradeStruct->upgrades[i].curValue;
			}
			break;
	}
	return defaultValue; // the specified upgrade was not found...
}


//-------------------------------------------------------------------------------
// Draws big symbols for the info menu:
//-------------------------------------------------------------------------------
void cBuilding::DrawSymbolBig ( eSymbolsBig sym, int x, int y, int maxx, int value, int orgvalue, SDL_Surface *sf)
{
	SDL_Rect src, dest;
	int i, offx;

	switch ( sym )
	{

		case SBSpeed:
			src.x = 0;
			src.y = 109;
			src.w = 11;
			src.h = 12;
			break;

		case SBHits:
			src.x = 11;
			src.y = 109;
			src.w = 7;
			src.h = 11;
			break;

		case SBAmmo:
			src.x = 18;
			src.y = 109;
			src.w = 9;
			src.h = 14;
			break;

		case SBAttack:
			src.x = 27;
			src.y = 109;
			src.w = 10;
			src.h = 14;
			break;

		case SBShots:
			src.x = 37;
			src.y = 109;
			src.w = 15;
			src.h = 7;
			break;

		case SBRange:
			src.x = 52;
			src.y = 109;
			src.w = 13;
			src.h = 13;
			break;

		case SBArmor:
			src.x = 65;
			src.y = 109;
			src.w = 11;
			src.h = 14;
			break;

		case SBScan:
			src.x = 76;
			src.y = 109;
			src.w = 13;
			src.h = 13;
			break;

		case SBMetal:
			src.x = 89;
			src.y = 109;
			src.w = 12;
			src.h = 15;
			break;

		case SBOil:
			src.x = 101;
			src.y = 109;
			src.w = 11;
			src.h = 12;
			break;

		case SBGold:
			src.x = 112;
			src.y = 109;
			src.w = 13;
			src.h = 10;
			break;

		case SBEnergy:
			src.x = 125;
			src.y = 109;
			src.w = 13;
			src.h = 17;
			break;

		case SBHuman:
			src.x = 138;
			src.y = 109;
			src.w = 12;
			src.h = 16;
			break;
	}

	maxx -= src.w;

	if ( orgvalue < value )
	{
		maxx -= src.w + 3;
	}

	offx = src.w;

	while ( offx*value >= maxx )
	{
		offx--;

		if ( offx < 4 )
		{
			value /= 2;
			orgvalue /= 2;
			offx = src.w;
		}
	}

	dest.x = x;

	dest.y = y;
	
	for ( i = 0;i < value;i++ )
	{
		if ( i == orgvalue )
		{
			SDL_Rect mark;
			dest.x += src.w + 3;
			mark.x = dest.x - src.w / 2;
			mark.y = dest.y;
			mark.w = 1;
			mark.h = src.h;
			SDL_FillRect ( sf, &mark, 0xFC0000 );
		}

		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, sf, &dest );

		dest.x += offx;
	}
}

//-------------------------------------------------------------------------------
/** checks the resources that are available under the mining station */
//--------------------------------------------------------------------------
void cBuilding::CheckRessourceProd ( void )
{
	int pos, max_cap;
	pos = PosX + PosY * Server->Map->size;

	if ( Server->Map->Resources[pos].typ == RES_METAL )
	{
		MaxMetalProd += Server->Map->Resources[pos].value;
	}
	else
		if ( Server->Map->Resources[pos].typ == RES_OIL )
		{
			MaxOilProd += Server->Map->Resources[pos].value;
		}
		else
			if ( Server->Map->Resources[pos].typ == RES_GOLD )
			{
				MaxGoldProd += Server->Map->Resources[pos].value;
			}

	pos++;

	if ( Server->Map->Resources[pos].typ == RES_METAL )
	{
		MaxMetalProd += Server->Map->Resources[pos].value;
	}
	else
		if ( Server->Map->Resources[pos].typ == RES_OIL )
		{
			MaxOilProd += Server->Map->Resources[pos].value;
		}
		else
			if ( Server->Map->Resources[pos].typ == RES_GOLD )
			{
				MaxGoldProd += Server->Map->Resources[pos].value;
			}

	pos += Server->Map->size;

	if ( Server->Map->Resources[pos].typ == RES_METAL )
	{
		MaxMetalProd += Server->Map->Resources[pos].value;
	}
	else
		if ( Server->Map->Resources[pos].typ == RES_OIL )
		{
			MaxOilProd += Server->Map->Resources[pos].value;
		}
		else
			if ( Server->Map->Resources[pos].typ == RES_GOLD )
			{
				MaxGoldProd += Server->Map->Resources[pos].value;
			}

	pos--;

	if ( Server->Map->Resources[pos].typ == RES_METAL )
	{
		MaxMetalProd += Server->Map->Resources[pos].value;
	}
	else
		if ( Server->Map->Resources[pos].typ == RES_OIL )
		{
			MaxOilProd += Server->Map->Resources[pos].value;
		}
		else
			if ( Server->Map->Resources[pos].typ == RES_GOLD )
			{
				MaxGoldProd += Server->Map->Resources[pos].value;
			}

	if ( data.is_alien )
	{
		max_cap = 24;
	}
	else
	{
		max_cap = 16;
	}

	// Rohstoffe verteilen:
	pos = max_cap;

	MetalProd = MaxMetalProd;

	if ( MetalProd > max_cap )
	{
		MetalProd = max_cap;
		pos = 0;
	}
	else
	{
		pos -= MaxMetalProd;
	}

	if ( pos > 0 )
	{
		if ( MaxOilProd - pos < 0 )
		{
			OilProd = MaxOilProd;
			pos -= MaxOilProd;
		}
		else
		{
			OilProd = pos;
			pos = 0;
		}
	}

	if ( pos > 0 )
	{
		if ( MaxGoldProd - pos < 0 )
		{
			GoldProd = MaxGoldProd;
			pos -= MaxGoldProd;
		}
		else
		{
			GoldProd = pos;
			pos = 0;
		}
	}
}

//-------------------------------------------------------------------------------
/** Display the ressource (mine) manager */
//--------------------------------------------------------------------------
void cBuilding::showMineManager ()
{
	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b;
	SDL_Rect src, dest;
	bool IncMetalPressed = false;
	bool DecMetalPressed = false;
	bool IncOilPressed = false;
	bool DecOilPressed = false;
	bool IncGoldPressed = false;
	bool DecGoldPressed = false;
	int MaxM = 0, MaxO = 0, MaxG = 0;
	int iFreeM = 0, iFreeO = 0, iFreeG = 0;
	int iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd;
	Client->isInMenu = true;

	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };
	SDL_Rect rTitle = {rDialog.x + 230, rDialog.y + 11, 174, 13};
	SDL_Rect rInfo1 = {rDialog.x + 46, rDialog.y + 78, 70, 11};
	SDL_Rect rInfo2 = {rInfo1.x, rInfo1.y + 37, rInfo1.w, rInfo1.h};
	SDL_Rect rInfo3 = {rInfo1.x, rInfo1.y + 37*2, rInfo1.w, rInfo1.h};

	if ( SettingsData.bAlphaEffects )
	{
		SDL_BlitSurface ( GraphicsData.gfx_shadow, NULL, buffer, NULL );
	}

	//blit menu img
	SDL_BlitSurface ( GraphicsData.gfx_mine_manager, NULL, buffer, &rDialog );

	mouse->SetCursor ( CHand );


	font->showTextCentered ( rTitle.x + rTitle.w / 2, rTitle.y, lngPack.i18n ( "Text~Title~Mine" ) );

	for ( int i = 0; i < 3; i++ )
	{
		switch ( i )
		{

			case 0:

				font->showTextCentered ( rInfo1.x + rInfo1.w / 2, rInfo1.y, lngPack.i18n ( "Text~Title~Metal" ) );

				break;

			case 1:
				rInfo1.y += 120;
				font->showTextCentered ( rInfo1.x + rInfo1.w / 2, rInfo1.y, lngPack.i18n ( "Text~Title~Oil" ) );

				break;

			case 2:
				rInfo1.y += 120;
				font->showTextCentered ( rInfo1.x + rInfo1.w / 2, rInfo1.y, lngPack.i18n ( "Text~Title~Gold" ) );

				break;
		}

		font->showTextCentered ( rInfo2.x + rInfo2.w / 2, rInfo2.y, lngPack.i18n ( "Text~Vehicles~Usage" ) );

		rInfo2.y += 121;
		font->showTextCentered ( rInfo3.x + rInfo3.w / 2, rInfo3.y, lngPack.i18n ( "Text~Comp~Reserve" ) );

		rInfo3.y += 121;
	}

	BigButton btn_done(rDialog.x + 514, rDialog.y + 430, "Text~Button~Done");
	btn_done.Draw();

	// generate list with mine datas. only use temporary cache so that original data wouldn't be changed
	cList<sMineValues*> Mines;

	for (unsigned int i = 0; i < SubBase->buildings.Size(); i++)
	{
		if (SubBase->buildings[i]->data.is_mine && SubBase->buildings[i]->IsWorking)
		{
			cBuilding *Building;
			Building = SubBase->buildings[i];

			sMineValues *MineValues = new sMineValues;
			MineValues->iMetalProd = Building->MetalProd;
			MineValues->iOilProd = Building->OilProd;
			MineValues->iGoldProd = Building->GoldProd;

			MineValues->iMaxMetalProd = Building->MaxMetalProd;
			MineValues->iMaxOilProd = Building->MaxOilProd;
			MineValues->iMaxGoldProd = Building->MaxGoldProd;

			Mines.Add ( MineValues );

			MaxM += Building->MaxMetalProd;
			MaxO += Building->MaxOilProd;
			MaxG += Building->MaxGoldProd;
		}
	}

	iTempSBMetalProd = SubBase->MetalProd;
	iTempSBOilProd = SubBase->OilProd;
	iTempSBGoldProd = SubBase->GoldProd;
/*
//#define DO_MINE_INC(a,b) for(i=0;i<mines.Size();i++){if(mines[i]->MetalProd+mines[i]->OilProd+mines[i]->GoldProd<16&&mines[i]->a<mines[i]->b){mines[i]->a++;break;}}
#define DO_MINE_INC(a,b) for(i=0;i<mines.Size();i++){if(mines[i]->MetalProd+mines[i]->OilProd+mines[i]->GoldProd<(mines[i]->data.is_alien?24:16)&&mines[i]->a<mines[i]->b){mines[i]->a++;break;}}
#define DO_MINE_DEC(a) for(i=0;i<mines.Size();i++){if(mines[i]->a>0){mines[i]->a--;break;}}
//#define CALC_MINE_FREE FreeM=0;FreeO=0;FreeG=0;for(i=0;i<mines.Size();i++){int ges=mines[i]->MetalProd+mines[i]->OilProd+mines[i]->GoldProd;if(ges<16){int t;ges=16-ges;t=mines[i]->MaxMetalProd-mines[i]->MetalProd;FreeM+=(ges<t?ges:t);t=mines[i]->MaxOilProd-mines[i]->OilProd;FreeO+=(ges<t?ges:t);t=mines[i]->MaxGoldProd-mines[i]->GoldProd;FreeG+=(ges<t?ges:t);}}
#define CALC_MINE_FREE FreeM=0;FreeO=0;FreeG=0;for(i=0;i<mines.Size();i++){int ges=mines[i]->MetalProd+mines[i]->OilProd+mines[i]->GoldProd;if(ges<(mines[i]->data.is_alien?24:16)){int t;ges=(mines[i]->data.is_alien?24:16)-ges;t=mines[i]->MaxMetalProd-mines[i]->MetalProd;FreeM+=(ges<t?ges:t);t=mines[i]->MaxOilProd-mines[i]->OilProd;FreeO+=(ges<t?ges:t);t=mines[i]->MaxGoldProd-mines[i]->GoldProd;FreeG+=(ges<t?ges:t);}}
*/
	calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );

	MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );

	// Die Reserve malen:
	DrawMineBar ( TRANS_METAL, SubBase->Metal, SubBase->MaxMetal, 2, true, 0 );
	DrawMineBar ( TRANS_OIL, SubBase->Oil, SubBase->MaxOil, 2, true, 0 );
	DrawMineBar ( TRANS_GOLD, SubBase->Gold, SubBase->MaxGold, 2, true, 0 );

	SHOW_SCREEN
	mouse->GetBack ( buffer );
	mouse->draw ( false, screen );

	while ( 1 )
	{
		if (  Client->SelectedBuilding != this ) break;
		if ( !Client->isInMenu ) break;

		Client->handleTimer();
		Client->doGameActions();

		// Events holen:
		EventHandler->HandleEvents();
		Client->doGameActions();

		// Die Maus machen:
		mouse->GetPos();

		b = (int)Client->getMouseState().leftButtonPressed;

		x = mouse->x;

		y = mouse->y;

		if ( x != LastMouseX || y != LastMouseY )
		{
			mouse->draw ( true, screen );
		}

		if ( btn_done.CheckClick( x, y, b > LastB, b < LastB ) )
		{
			sendChangeResources ( this, iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd );
			break;
		}

		// Aufs Metall geklickt:
		if ( x >= rDialog.x + 174 && x < rDialog.x + 174 + 240 && y >= rDialog.y + 70 && y < rDialog.y + 70 + 30 && b && !LastB )
		{
			int t;
			PlayFX ( SoundData.SNDObjectMenu );
			t =  Round ( ( x -rDialog.x -174 ) * ( MaxM / 240.0 ) );

			if ( t < iTempSBMetalProd )
			{
				for ( ;abs ( iTempSBMetalProd - t ) && iTempSBMetalProd > 0; )
				{
					iTempSBMetalProd--;
					doMineDec(TYPE_METAL, Mines);
				}

				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
			}
			else
			{
				for ( ;abs ( iTempSBMetalProd - t ) && iFreeM; )
				{
					iTempSBMetalProd++;
					doMineInc(TYPE_METAL, Mines);
					calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				}
			}

			MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// click on oil:
		if ( x >= rDialog.x + 174 && x < rDialog.x + 174 + 240 && y >= rDialog.y + 190 && y < rDialog.y + 190 + 30 && b && !LastB )
		{
			int t;
			PlayFX ( SoundData.SNDObjectMenu );
			t = Round ( ( x -rDialog.x - 174 ) * ( MaxO / 240.0 ) );

			if ( t < iTempSBOilProd )
			{
				for ( ;abs ( iTempSBOilProd - t ) && iTempSBOilProd > 0; )
				{
					iTempSBOilProd--;
					doMineDec(TYPE_OIL, Mines);
				}

				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
			}
			else
			{
				for ( ;abs ( iTempSBOilProd - t ) && iFreeO; )
				{
					iTempSBOilProd++;
					doMineInc(TYPE_OIL, Mines);
					calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				}
			}

			MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// Aufs Gold geklickt:
		if ( x >= rDialog.x + 174 && x < rDialog.x + 174 + 240 && y >= rDialog.y + 310 && y < rDialog.y + 310 + 30 && b && !LastB )
		{
			int t;
			PlayFX ( SoundData.SNDObjectMenu );
			t = Round ( ( x -rDialog.x - 174 ) * ( MaxG / 240.0 ) );

			if ( t < iTempSBGoldProd )
			{
				for ( ;abs ( iTempSBGoldProd - t ) && iTempSBGoldProd > 0; )
				{
					iTempSBGoldProd--;
					doMineDec(TYPE_GOLD, Mines);
				}

				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
			}
			else
			{
				for ( ;abs ( iTempSBGoldProd - t ) && iFreeG; )
				{
					iTempSBGoldProd++;
					doMineInc(TYPE_GOLD, Mines);
					calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				}
			}

			MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// IncMetal-Button:
		if ( x >= rDialog.x + 421 && x < rDialog.x + 421 + 26 && y >= rDialog.y + 71 && y < rDialog.y + 71 + 27 && b && !IncMetalPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 308;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 421;
			dest.y = rDialog.y + 71;

			if ( iFreeM )
			{
				iTempSBMetalProd++;
				doMineInc(TYPE_METAL, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			IncMetalPressed = true;
		}
		else
			if ( !b && IncMetalPressed )
			{
				src.x = 421;
				src.y = 71;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 421;
				dest.y = rDialog.y + 71;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				IncMetalPressed = false;
			}

		// DecMetal-Button:
		if ( x >= rDialog.x + 139 && x < rDialog.x + 139 + 26 && y >= rDialog.y + 71 && y < rDialog.y + 71 + 27 && b && !DecMetalPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 280;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 139;
			dest.y = rDialog.y + 71;

			if ( iTempSBMetalProd > 0 )
			{
				iTempSBMetalProd--;
				doMineDec(TYPE_METAL, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			DecMetalPressed = true;
		}
		else
			if ( !b && DecMetalPressed )
			{
				src.x = 139;
				src.y = 71;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 139;
				dest.y = rDialog.y + 71;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				DecMetalPressed = false;
			}

		// IncOil-Button:
		if ( x >= rDialog.x + 421 && x < rDialog.x + 421 + 26 && y >= rDialog.y + 191 && y < rDialog.y + 191 + 27 && b && !IncOilPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 308;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 421;
			dest.y = rDialog.y + 191;

			if ( iFreeO )
			{
				iTempSBOilProd++;
				doMineInc(TYPE_OIL, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			IncOilPressed = true;
		}
		else
			if ( !b && IncOilPressed )
			{
				src.x = 421;
				src.y = 191;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 421;
				dest.y = rDialog.y + 191;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				IncOilPressed = false;
			}

		// DecOil-Button:
		if ( x >= rDialog.x + 139 && x < rDialog.x + 139 + 26 && y >= rDialog.y + 191 && y < rDialog.y + 191 + 27 && b && !DecOilPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 280;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 139;
			dest.y = rDialog.y + 191;

			if ( iTempSBOilProd > 0 )
			{
				iTempSBOilProd--;
				doMineDec(TYPE_OIL, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			DecOilPressed = true;
		}
		else
			if ( !b && DecOilPressed )
			{
				src.x = 139;
				src.y = 191;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 139;
				dest.y = rDialog.y + 191;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				DecOilPressed = false;
			}

		// IncGold-Button:
		if ( x >= rDialog.x + 421 && x < rDialog.x + 421 + 26 && y >= rDialog.y + 311 && y < rDialog.y + 311 + 27 && b && !IncGoldPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 308;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 421;
			dest.y = rDialog.y + 311;

			if ( iFreeG )
			{
				iTempSBGoldProd++;
				doMineInc(TYPE_GOLD, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			IncGoldPressed = true;
		}
		else
			if ( !b && IncGoldPressed )
			{
				src.x = 421;
				src.y = 311;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 421;
				dest.y = rDialog.y + 311;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				IncGoldPressed = false;
			}

		// DecGold-Button:
		if ( x >= rDialog.x + 139 && x < rDialog.x + 139 + 26 && y >= rDialog.y + 311 && y < rDialog.y + 311 + 27 && b && !DecGoldPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 122;
			src.y = 280;
			src.w = 26;
			src.h = 27;
			dest.x = rDialog.x + 139;
			dest.y = rDialog.y + 311;

			if ( iTempSBGoldProd > 0 )
			{
				iTempSBGoldProd--;
				doMineDec(TYPE_GOLD, Mines);
				calcMineFree ( &Mines, &iFreeM, &iFreeO, &iFreeG );
				MakeMineBars ( iTempSBMetalProd, iTempSBOilProd, iTempSBGoldProd, MaxM, MaxO, MaxG, &iFreeM, &iFreeO, &iFreeG );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			DecGoldPressed = true;
		}
		else
			if ( !b && DecGoldPressed )
			{
				src.x = 139;
				src.y = 311;
				src.w = 26;
				src.h = 27;
				dest.x = rDialog.x + 139;
				dest.y = rDialog.y + 311;
				SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				DecGoldPressed = false;
			}

		LastMouseX = x;

		LastMouseY = y;
		LastB = b;
	}
	Client->isInMenu = false;
}

//--------------------------------------------------------------------------
/** Displays the horizontal bars in the mine (resource) manager screen */
//--------------------------------------------------------------------------
void cBuilding::MakeMineBars ( int iTempSBMetalProd, int iTempSBOilProd, int iTempSBGoldProd, int MaxM, int MaxO, int MaxG, int *FreeM, int *FreeO, int *FreeG )
{
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };
	string sTmp1 = "";
	string sTmp2 = " / " + lngPack.i18n ( "Text~Comp~Turn" ) + ")";
	DrawMineBar ( TRANS_METAL, iTempSBMetalProd, MaxM, 0, true, MaxM - iTempSBMetalProd - *FreeM );
	DrawMineBar ( TRANS_OIL, iTempSBOilProd, MaxO, 0, true, MaxO - iTempSBOilProd - *FreeO );
	DrawMineBar ( TRANS_GOLD, iTempSBGoldProd, MaxG, 0, true, MaxG - iTempSBGoldProd - *FreeG );

	DrawMineBar ( TRANS_METAL, SubBase->MetalNeed, SubBase->MaxMetalNeed, 1, false, 0 );
	sTmp1 = iToStr ( SubBase->MetalNeed ) + " (" + iToStr ( iTempSBMetalProd - SubBase->MetalNeed );

	font->showTextCentered ( rDialog.x + 174 + 120, rDialog.y + 70 + 8 + 37, sTmp1 + sTmp2, FONT_LATIN_BIG );

	DrawMineBar ( TRANS_OIL, SubBase->OilNeed, SubBase->MaxOilNeed, 1, false, 0 );
	sTmp1 = iToStr ( SubBase->OilNeed ) + " (" + iToStr ( iTempSBOilProd - SubBase->OilNeed );

	font->showTextCentered ( rDialog.x + 174 + 120, rDialog.y + 190 + 8 + 37, sTmp1 + sTmp2, FONT_LATIN_BIG );

	DrawMineBar ( TRANS_GOLD, SubBase->GoldNeed, SubBase->MaxGoldNeed, 1, false, 0 );
	sTmp1 = iToStr ( SubBase->GoldNeed ) + " (" + iToStr ( iTempSBGoldProd - SubBase->GoldNeed );

	font->showTextCentered ( rDialog.x + 174 + 120, rDialog.y + 310 + 8 + 37, sTmp1 + sTmp2, FONT_LATIN_BIG );
}

//--------------------------------------------------------------------------
/* Displays one resource bar */
//--------------------------------------------------------------------------
void cBuilding::DrawMineBar ( int typ, int value, int max_value, int offy, bool number, int fixed )
{
	SDL_Rect src, dest;
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };
	SDL_Rect rScr;

	switch ( typ )
	{

		case TRANS_METAL:
			src.y = 339;
			dest.y = rDialog.y + 70;
			break;

		case TRANS_OIL:
			src.y = 369;
			dest.y = rDialog.y + 190;
			break;

		case TRANS_GOLD:
			src.y = 400;
			dest.y = rDialog.y + 310;
			break;
	}

	dest.x = rDialog.x + 174;

	rScr.w = 240;
	rScr.h = src.h = 30;
	dest.y += offy * 37;
	rScr.x = dest.x - rDialog.x;
	rScr.y = dest.y - rDialog.y;
	SDL_BlitSurface ( GraphicsData.gfx_mine_manager, &rScr, buffer, &dest );

	if ( max_value == 0 )
	{
		src.w = 0;
		src.x = 156;
	}
	else
	{
		src.w = ( int ) ( ( ( float ) value / max_value ) * 240 );
		src.x = 156 + ( 240 - ( int ) ( ( ( float ) value / max_value ) * 240 ) );
	}

	dest.x = rDialog.x + 174;

	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

	if ( fixed && src.w != 240 && max_value != 0 )
	{
		src.w = ( int ) ( ( ( float ) fixed /  max_value ) * 240 );
		dest.x = rDialog.x + 174 + 240 - src.w;
		src.x = 156;
		src.y = 307;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	}
	else
		if ( max_value == 0 )
		{
			src.w = 240;
			dest.x = rDialog.x + 174;
			src.x = 156;
			src.y = 307;
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
		}

	if ( number )
		font->showTextCentered ( rDialog.x + 174 + 120, dest.y + 8, iToStr ( value ), FONT_LATIN_BIG );
}

//--------------------------------------------------------------------------
/** Checks if the target is in range */
//--------------------------------------------------------------------------
bool cBuilding::IsInRange ( int off, cMap *Map )
{
	int x, y;
	x = off % Map->size;
	y = off / Map->size;
	x -= PosX;
	y -= PosY;

	if ( sqrt ( ( double ) ( x*x + y*y ) ) <= data.range )
	{
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------
/** Checks if the building is able to attack the object */
//--------------------------------------------------------------------------
bool cBuilding::CanAttackObject ( int off, cMap *Map, bool override )
{
	cVehicle *v = NULL;
	cBuilding *b = NULL;

	if ( !data.can_attack )
		return false;

	if ( !data.shots )
		return false;

	if ( !data.ammo )
		return false;

	if ( Attacking )
		return false;

	if ( bIsBeeingAttacked )
		return false;

	if ( off < 0 )
		return false;

	if ( !IsInRange ( off, Map ) )
		return false;

	if ( !owner->ScanMap[off] )
		return override?true:false;

	if ( override )
		return true;

	selectTarget(v, b, off, data.can_attack, Map );

	if ( v )
	{
		if ( Client && ( v == Client->SelectedVehicle || v->owner == Client->ActivePlayer ) )
			return false;
	}
	else if ( b )
	{
		if ( Client && ( b == Client->SelectedBuilding || b->owner == Client->ActivePlayer ) )
			return false;
	}
	else
		return false;

	return true;
}

//--------------------------------------------------------------------------
/** Draw the attack cursor */
//--------------------------------------------------------------------------
void cBuilding::DrawAttackCursor ( int offset )
{
	SDL_Rect r;
	int wp, wc, t = 0;
	cVehicle *v;
	cBuilding *b;

	selectTarget(v, b, offset, data.can_attack, Client->Map );

	if ( !(v || b) || ( v && v == Client->SelectedVehicle ) || ( b && b == Client->SelectedBuilding ) )
	{
		r.x = 1;
		r.y = 29;
		r.h = 3;
		r.w = 35;
		SDL_FillRect ( GraphicsData.gfx_Cattack, &r, 0 );
		return;
	}

	if ( v )
		t = v->data.hit_points;
	else if ( b )
		t = b->data.hit_points;

	if ( t )
	{
		if ( v )
			wc = ( int ) ( ( float ) t / v->data.max_hit_points * 35 );
		else  if ( b )
			wc = ( int ) ( ( float ) t / b->data.max_hit_points * 35 );
	}
	else
	{
		wc = 0;
	}

	if ( v )
		t = v->CalcHelth ( data.damage );
	else  if ( b )
		t = b->CalcHelth ( data.damage );

	if ( t )
	{
		if ( v )
			wp = ( int ) ( ( float ) t / v->data.max_hit_points * 35 );
		else  if ( b )
			wp = ( int ) ( ( float ) t / b->data.max_hit_points * 35 );
	}
	else
	{
		wp = 0;
	}

	r.x = 1;

	r.y = 29;
	r.h = 3;
	r.w = wp;

	if ( r.w )
		SDL_FillRect ( GraphicsData.gfx_Cattack, &r, 0x00FF00 );

	r.x += r.w;

	r.w = wc - wp;

	if ( r.w )
		SDL_FillRect ( GraphicsData.gfx_Cattack, &r, 0xFF0000 );

	r.x += r.w;

	r.w = 35 - wc;

	if ( r.w )
		SDL_FillRect ( GraphicsData.gfx_Cattack, &r, 0 );
}

//--------------------------------------------------------------------------
/** Rotates the building in the given direction */
//--------------------------------------------------------------------------
void cBuilding::RotateTo ( int Dir )
{
	int i, t, dest;

	if ( dir == Dir )
		return;

	t = dir;

	for ( i = 0;i < 8;i++ )
	{
		if ( t == Dir )
		{
			dest = i;
			break;
		}

		t++;

		if ( t > 7 )
			t = 0;
	}

	if ( dest < 4 )
		dir++;
	else
		dir--;

	if ( dir < 0 )
		dir += 8;
	else
		if ( dir > 7 )
			dir -= 8;
}

//--------------------------------------------------------------------------
/** Displays the build menu */
//--------------------------------------------------------------------------
void cBuilding::ShowBuildMenu ()
{
	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b;
	SDL_Rect src, dest;
	bool Wiederholen = false;
	bool DownPressed = false;
	bool UpPressed = false;
	bool Down2Pressed = false;
	bool Up2Pressed = false;
	int selected = 0, offset = 0, BuildSpeed;
	int build_selected = 0, build_offset = 0;
	bool showDetailsBuildlist = true; //wenn false, stattdessen die Details der in der toBuild Liste gewaehlten Einheit anzeigen
	Client->isInMenu = true;

#define BUTTON__W 77
#define BUTTON__H 23

	SDL_Rect rDialog = { MENU_OFFSET_X, MENU_OFFSET_Y, DIALOG_W, DIALOG_H };
	SDL_Rect rTxtDescription = {MENU_OFFSET_X + 141, MENU_OFFSET_Y + 266, 150, 13};
	SDL_Rect rTitle = {MENU_OFFSET_X + 330, MENU_OFFSET_Y + 11, 154, 13};
	SDL_Rect rTxtRepeat = {MENU_OFFSET_X + 370, MENU_OFFSET_Y + 326, 76, 17};

	mouse->SetCursor ( CHand );
	mouse->draw ( false, buffer );
	SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, NULL, buffer, &rDialog );

	NormalButton btn_cancel(MENU_OFFSET_X + 300, MENU_OFFSET_Y + 452, "Text~Button~Cancel");
	NormalButton btn_done(  MENU_OFFSET_X + 387, MENU_OFFSET_Y + 452, "Text~Button~Done");
	NormalButton btn_delete(MENU_OFFSET_X + 388, MENU_OFFSET_Y + 292, "Text~Button~Delete");
	NormalButton btn_build( MENU_OFFSET_X + 561, MENU_OFFSET_Y + 441, "Text~Button~Build");
	btn_cancel.Draw();
	btn_done.Draw();
	btn_delete.Draw();
	btn_build.Draw();

	font->showTextCentered ( rTxtDescription.x + rTxtDescription.w / 2, rTxtDescription.y, lngPack.i18n ( "Text~Comp~Description" ) );
	font->showTextCentered ( rTitle.x + rTitle.w / 2, rTitle.y, lngPack.i18n ( "Text~Title~Build" ) );
	font->showTextCentered ( rTxtRepeat.x + rTxtRepeat.w / 2, rTxtRepeat.y, lngPack.i18n ( "Text~Comp~Repeat" ) );

	// the checkbox
	if ( SettingsData.bShowDescription )
	{
		src.x = 291;
		src.y = 264;
		dest.x = MENU_OFFSET_X + 291;
		dest.y = MENU_OFFSET_Y + 264;
		src.w = 17;
		src.h = 17;
		SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
	}
	else
	{
		src.x = 393;
		src.y = 46;
		dest.x = MENU_OFFSET_X + 291;
		dest.y = MENU_OFFSET_Y + 264;
		src.w = 18;
		src.h = 17;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	}

	// Die Images erstellen:
	cList<sBuildStruct*> images;

	float newzoom = (float)(Client->Hud.Zoom / 64.0);

	for (size_t i = 0; i < UnitsData.vehicle.Size(); ++i)
	{
		const sVehicle& vehicle = UnitsData.vehicle[i];

		SDL_Surface *sf;
		bool land = false, water = false;

		int x = PosX - 2, y = PosY - 1;

		for ( int j = 0; j < 12; j++ )
		{
			if ( j == 4 ||  j == 6 || j == 8 )
			{
				x -= 3;
				y += 1;
			}
			else if ( j == 5 || j == 7 )
			{
				x += 3;
			}
			else
			{
				x++;
			}

			int off = x + y * Client->Map->size;
			cBuildingIterator bi = Client->Map->fields[off].getBuildings();
			while ( bi && (bi->data.is_connector || bi->data.is_expl_mine) ) bi++;

			if ( !Client->Map->IsWater ( off, true, true ) || bi && (( bi->data.is_road || bi->data.is_bridge || bi->data.is_platform ) ))
			{
				land = true;
			}
			else
			{
				water = true;
			}
		}

		if ( vehicle.data.can_drive == DRIVE_SEA && !water )
			continue;
		else if ( vehicle.data.can_drive == DRIVE_LAND && !land )
			continue;

		if ( data.can_build == BUILD_AIR && vehicle.data.can_drive != DRIVE_AIR )
			continue;
		else if ( data.can_build == BUILD_BIG && !vehicle.data.build_by_big )
			continue;
		else if ( data.can_build == BUILD_SEA && vehicle.data.can_drive != DRIVE_SEA )
			continue;
		else if ( data.can_build == BUILD_SMALL && ( vehicle.data.can_drive == DRIVE_AIR ||vehicle.data.can_drive == DRIVE_SEA || vehicle.data.build_by_big || vehicle.data.is_human ) )
			continue;
		else if ( data.can_build == BUILD_MAN && !vehicle.data.is_human )
			continue;
		else if ( !data.build_alien && vehicle.data.is_alien )
			continue;
		else if ( data.build_alien && !vehicle.data.is_alien )
			continue;

		scaleSurface ( vehicle.img_org[0], vehicle.img[0], vehicle.img_org[0]->w / 2, vehicle.img_org[0]->h / 2 );

		sf = SDL_CreateRGBSurface ( SDL_SRCCOLORKEY, vehicle.img[0]->w, vehicle.img[0]->h, 32, 0, 0, 0, 0 );

		SDL_SetColorKey ( sf, SDL_SRCCOLORKEY, 0xFF00FF );

		SDL_BlitSurface ( Client->ActivePlayer->color, NULL, sf, NULL );

		SDL_BlitSurface ( vehicle.img[0], NULL, sf, NULL );

		scaleSurface ( vehicle.img_org[0], vehicle.img[0], (int) (vehicle.img_org[0]->w* newzoom ), (int) ( vehicle.img_org[0]->h* newzoom ) );

		sBuildStruct* const n = new sBuildStruct(sf, vehicle.data.ID);
		images.Add( n );
	}


	// Die Bauliste anlegen:
	cList<sBuildStruct*> to_build;

	for (unsigned int i = 0; i < BuildList->Size(); i++)
	{
		sBuildList *ptr;
		ptr = (*BuildList)[i];

		//fuer jeden Eintrag in der toBuild-Liste das bereits erstellte Bild in der Auswahlliste suchen
		//und in die toBuild-Liste kopieren.

		for (unsigned int k = 0; k < images.Size(); k++)
		{
			sBuildStruct *bs;
			bs = images[k];

			if ( bs->ID.getVehicle()->nr == ptr->typ->nr )
			{
				sBuildStruct* const n = new sBuildStruct(images[k]->sf, images[k]->ID, ptr->metall_remaining);
				to_build.Add ( n );

				break;
			}
		}
	}

	BuildSpeed = this->BuildSpeed;

	//show details of the first item in to_build list, if it exists
	if (to_build.Size() > 0)
	{
		showDetailsBuildlist = false;
	}
	else
	{
		showDetailsBuildlist = true;
	}

	ShowBuildList ( images, selected, offset, showDetailsBuildlist );

	DrawBuildButtons ( BuildSpeed );
	ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );

	if ( !RepeatBuild )
	{
		// Den Wiederholen Haken machen:
		src.x = 393;
		src.y = 46;
		dest.x = MENU_OFFSET_X + 447;
		dest.y = MENU_OFFSET_Y + 322;
		src.w = 18;
		src.h = 17;
		SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
	}
	else
	{
		Wiederholen = true;
	}

	// Den Buffer anzeigen:
	SHOW_SCREEN
	mouse->GetBack ( buffer );

	mouse->MoveCallback = false;

	while ( 1 )
	{
		if (  Client->SelectedBuilding != this ) break;
		if ( !Client->isInMenu ) break;

		Client->doGameActions();

		// Events holen:
		EventHandler->HandleEvents();

		// Die Maus machen:
		mouse->GetPos();

		b = (int)Client->getMouseState().leftButtonPressed;

		x = mouse->x;

		y = mouse->y;

		if ( x != LastMouseX || y != LastMouseY )
		{
			mouse->draw ( true, screen );
		}

		// Down-Button:
		if ( x >= MENU_OFFSET_X + 491 && x < MENU_OFFSET_X + 491 + 18 && y >= MENU_OFFSET_Y + 440 && y < MENU_OFFSET_Y + 440 + 17 && b && !DownPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 249;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 491;
			dest.y = MENU_OFFSET_Y + 440;

			offset += 9;

			if (offset > (int)images.Size() - 9)
			{
				offset = (int)images.Size() - 9;
			}
			if ( offset < 0 )
			{
				offset = 0;
			}

			if ( selected < offset )
				selected = offset;

			ShowBuildList ( images, selected, offset, showDetailsBuildlist );


			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );

			DownPressed = true;
		}
		else
			if ( !b && DownPressed )
			{
				src.x = 491;
				src.y = 440;
				src.w = 18;
				src.h = 17;
				dest.x = MENU_OFFSET_X + 491;
				dest.y = MENU_OFFSET_Y + 440;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				DownPressed = false;
			}

		// Up-Button:
		if ( x >= MENU_OFFSET_X + 471 && x < MENU_OFFSET_X + 471 + 18 && y >= MENU_OFFSET_Y + 440 && y < MENU_OFFSET_Y + 440 + 17 && b && !UpPressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 230;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 471;
			dest.y = MENU_OFFSET_Y + 440;

			offset -= 9;

			if ( offset < 0 )
			{
				offset = 0;
			}

			if ( selected > offset + 8 )
				selected = offset + 8;

			ShowBuildList ( images, selected, offset, showDetailsBuildlist );

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );

			UpPressed = true;
		}
		else
			if ( !b && UpPressed )
			{
				src.x = 471;
				src.y = 440;
				src.w = 18;
				src.h = 17;
				dest.x = MENU_OFFSET_X + 471;
				dest.y = MENU_OFFSET_Y + 440;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				UpPressed = false;
			}

		// Down2-Button:
		if ( x >= MENU_OFFSET_X + 327 && x < MENU_OFFSET_X + 327 + 18 && y >= MENU_OFFSET_Y + 293 && y < MENU_OFFSET_Y + 293 + 17 && b && !Down2Pressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 230;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 327;
			dest.y = MENU_OFFSET_Y + 293;

			if ( build_offset != 0 )
			{
				build_offset--;
				ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			Down2Pressed = true;
		}
		else
			if ( !b && Down2Pressed )
			{
				src.x = 327;
				src.y = 293;
				src.w = 18;
				src.h = 17;
				dest.x = MENU_OFFSET_X + 327;
				dest.y = MENU_OFFSET_Y + 293;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				Down2Pressed = false;
			}

		// Up2-Button:
		if ( x >= MENU_OFFSET_X + 347 && x < MENU_OFFSET_X + 347 + 18 && y >= MENU_OFFSET_Y + 293 && y < MENU_OFFSET_Y + 293 + 17 && b && !Up2Pressed )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			src.x = 249;
			src.y = 151;
			src.w = 18;
			src.h = 17;
			dest.x = MENU_OFFSET_X + 347;
			dest.y = MENU_OFFSET_Y + 293;

			if (build_offset < (int)to_build.Size() - 5)
			{
				build_offset++;
				ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );
			}

			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );

			SHOW_SCREEN
			mouse->draw ( false, screen );
			Up2Pressed = true;
		}
		else
			if ( !b && Up2Pressed )
			{
				src.x = 347;
				src.y = 293;
				src.w = 18;
				src.h = 17;
				dest.x = MENU_OFFSET_X + 347;
				dest.y = MENU_OFFSET_Y + 293;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
				SHOW_SCREEN
				mouse->draw ( false, screen );
				Up2Pressed = false;
			}

		bool const down = b > LastB;
		bool const up   = b < LastB;

		if (btn_build.CheckClick(x, y, down, up))
		{
			// Vehicle in die Bauliste aufnehmen:
			sBuildStruct* const n = new sBuildStruct(images[selected]->sf, images[selected]->ID);
			to_build.Add ( n );

			if ((int)to_build.Size() > build_offset + 5)
			{
				build_offset = (int)to_build.Size() - 5;
			}

			if ( build_selected < build_offset )
				build_selected = build_offset;

			ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		if (btn_delete.CheckClick(x, y, down, up))
		{
			// Vehicle aus der Bauliste entfernen:
			if (to_build.Size() && (int)to_build.Size() > build_selected && build_selected >= 0)
			{
				delete to_build[build_selected];
				to_build.Delete ( build_selected );

				if (build_selected >= (int)to_build.Size())
				{
					build_selected--;
				}

				if (to_build.Size() - build_offset < 5 && build_offset > 0)
				{
					build_offset--;
				}

				ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );
			}

			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		if (btn_cancel.CheckClick(x, y, down, up))
		{
			break;
		}

		if (btn_done.CheckClick(x, y, down, up))
		{
			// send build list to server
			this->BuildSpeed = BuildSpeed;
			sendWantBuildList ( this, &to_build, Wiederholen );
			break;
		}

		// description checkbox:
		if ( x >= MENU_OFFSET_X + 292 && x < MENU_OFFSET_X + 292 + 16 && y >= MENU_OFFSET_Y + 265 && y < MENU_OFFSET_Y + 265 + 15 && b && !LastB )
		{
			PlayFX ( SoundData.SNDObjectMenu );

			SettingsData.bShowDescription = !SettingsData.bShowDescription;

			if ( SettingsData.bShowDescription )
			{
				src.x = 291;
				src.y = 264;
				dest.x = MENU_OFFSET_X + 291;
				dest.y = MENU_OFFSET_Y + 264;
				src.w = 17;
				src.h = 17;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
			}
			else
			{
				src.x = 393;
				src.y = 46;
				dest.x = MENU_OFFSET_X + 291;
				dest.y = MENU_OFFSET_Y + 264;
				src.w = 18;
				src.h = 17;
				SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
			}

			ShowBuildList ( images, selected, offset, showDetailsBuildlist );

			ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );
			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// Wiederholen Haken:
		if ( x >= MENU_OFFSET_X + 447 && x < MENU_OFFSET_X + 447 + 16 && y >= MENU_OFFSET_Y + 322 && y < MENU_OFFSET_Y + 322 + 15 && b && !LastB )
		{
			PlayFX ( SoundData.SNDObjectMenu );
			Wiederholen = !Wiederholen;

			if ( Wiederholen )
			{
				src.x = 447;
				src.y = 322;
				dest.x = MENU_OFFSET_X + 447;
				dest.y = MENU_OFFSET_Y + 322;
				src.w = 18;
				src.h = 17;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
			}
			else
			{
				src.x = 393;
				src.y = 46;
				dest.x = MENU_OFFSET_X + 447;
				dest.y = MENU_OFFSET_Y + 322;
				src.w = 18;
				src.h = 17;
				SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, buffer, &dest );
			}

			SHOW_SCREEN

			mouse->draw ( false, screen );
		}

		// 1x Button:
		if ( x >= MENU_OFFSET_X + 292 && x < MENU_OFFSET_X + 292 + 76 && y >= MENU_OFFSET_Y + 345 && y < MENU_OFFSET_Y + 345 + 22 && b && !LastB )
		{
			PlayFX ( SoundData.SNDMenuButton );
			BuildSpeed = 0;
			DrawBuildButtons ( BuildSpeed );
			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// 2x Button:
		if ( x >= MENU_OFFSET_X + 292 && x < MENU_OFFSET_X + 292 + 76 && y >= MENU_OFFSET_Y + 369 && y < MENU_OFFSET_Y + 369 + 22 && b && !LastB && data.can_build != BUILD_MAN )
		{
			PlayFX ( SoundData.SNDMenuButton );
			BuildSpeed = 1;
			DrawBuildButtons ( BuildSpeed );
			SHOW_SCREEN
			mouse->draw ( false, screen );

		}

		// 4x Button:
		if ( x >= MENU_OFFSET_X + 292 && x < MENU_OFFSET_X + 292 + 76 && y >= MENU_OFFSET_Y + 394 && y < MENU_OFFSET_Y + 394 + 22 && b && !LastB && data.can_build != BUILD_MAN )
		{
			PlayFX ( SoundData.SNDMenuButton );
			BuildSpeed = 2;
			DrawBuildButtons ( BuildSpeed );
			SHOW_SCREEN
			mouse->draw ( false, screen );
		}

		// Klick in die Liste:
		if ( x >= MENU_OFFSET_X + 490 && x < MENU_OFFSET_X + 490 + 70 && y >= MENU_OFFSET_Y + 60 && y < MENU_OFFSET_Y + 60 + 368 && b && !LastB )
		{
			int nr;
			nr = ( y - 60 - MENU_OFFSET_Y ) / ( 32 + 10 );

			if (images.Size() < 9)
			{
				if (nr >= (int)images.Size())
					nr = -1;
			}
			else
			{
				if ( nr >= 10 )
					nr = -1;

				nr += offset;
			}

			if ( nr != -1 )
			{
				PlayFX ( SoundData.SNDObjectMenu );

				// second klick on the Unit?

				if ( ( nr == selected ) && showDetailsBuildlist )
				{
					//insert selected Vehicle in to_build list
					sBuildStruct* const n = new sBuildStruct(images[selected]->sf, images[selected]->ID);
					to_build.Add ( n );

					if ((int)to_build.Size() > build_offset + 5)
					{
						build_offset = (int)to_build.Size() - 5;
					}

					if ( build_selected < build_offset )
						build_selected = build_offset;
				}

				selected = nr;

				showDetailsBuildlist = true;
				ShowBuildList ( images, selected, offset, showDetailsBuildlist );
				ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );

				SHOW_SCREEN
				mouse->draw ( false, screen );
			}
		}

		// Klick in die to_build Liste:
		if ( x >= MENU_OFFSET_X + 330 && x < MENU_OFFSET_X + 330 + 128 && y >= MENU_OFFSET_Y + 60 && y < MENU_OFFSET_Y + 60 + 210 && b && !LastB )
		{
			int nr;
			nr = ( y - 60 - MENU_OFFSET_Y ) / ( 32 + 10 );

			if (to_build.Size() < 5)
			{
				if (nr >= (int)to_build.Size())
					nr = -1;
			}
			else
			{
				if ( nr >= 10 )
					nr = -1;

				nr += build_offset;
			}

			if ( nr != -1 )
			{
				PlayFX ( SoundData.SNDObjectMenu );

				// second klick on the Unit?

				if ( ( build_selected == nr ) && !showDetailsBuildlist )
				{
					//remove vehicle from to_build queue
					if (to_build.Size() && (int)to_build.Size() > build_selected && build_selected >= 0)
					{
						delete to_build[build_selected];
						to_build.Delete ( build_selected );

						if (build_selected >= (int)to_build.Size())
						{
							build_selected--;
						}

						if (to_build.Size() - build_offset < 5 && build_offset > 0)
						{
							build_offset--;
						}
					}
				}

				build_selected = nr;

				showDetailsBuildlist = false;
				ShowBuildList ( images, selected, offset, showDetailsBuildlist );
				ShowToBuildList ( to_build, build_selected, build_offset, !showDetailsBuildlist );

				SHOW_SCREEN
				mouse->draw ( false, screen );
			}
		}

		LastMouseX = x;

		LastMouseY = y;
		LastB = b;

	}

	// clean up the images
	while (images.Size())
	{
		sBuildStruct *ptr;
		ptr = images[0];
		SDL_FreeSurface ( ptr->sf );
		delete ptr;
		images.Delete( 0 );
	}

	while (to_build.Size())
	{
		delete to_build[0];
		to_build.Delete ( 0 );
	}

	mouse->MoveCallback = true;
	Client->isInMenu = false;
}

//--------------------------------------------------------------------------
/** Displays the list with all buildable units. If showInfo==true also all infos for the selected unit. */
//--------------------------------------------------------------------------
void cBuilding::ShowBuildList(cList<sBuildStruct*>& list, int const selected, int const offset, bool const showInfo)
{
	sBuildStruct *ptr;
	SDL_Rect dest, src, text = { MENU_OFFSET_X + 530, MENU_OFFSET_Y + 70, 80, 16 };
	src.x = 479;
	src.y = 52;
	dest.x = MENU_OFFSET_X + 479;
	dest.y = MENU_OFFSET_Y + 52;
	src.w = 150;
	src.h = 378;
	SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
	src.x = 373;
	src.y = 344;
	dest.x = MENU_OFFSET_X + 373;
	dest.y = MENU_OFFSET_Y + 344;
	src.w = 77;
	src.h = 72;
	SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );
	src.x = 0;
	src.y = 0;
	src.w = 32;
	src.h = 32;
	dest.x = MENU_OFFSET_X + 490;
	dest.y = MENU_OFFSET_Y + 58;
	
	for (unsigned int i = offset; i < list.Size(); i++)
	{
		if ( (int)i >= offset + 9 )
			break;

		// Das Bild malen:
		ptr = list[i];

		SDL_BlitSurface ( ptr->sf, &src, buffer, &dest );

		if ( selected == i )
		{
			if (showInfo)
			{
				//doppelten Rahmen drum malen
				SDL_Rect tmp;
				tmp = dest;
				tmp.x -= 3;
				tmp.y -= 3;
				tmp.h = 1;
				tmp.w = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 28;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 28;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 3;
				tmp.w = 1;
				tmp.h = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 29;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );

				tmp = dest;
				tmp.x -= 5;
				tmp.y -= 5;
				tmp.h = 1;
				tmp.w = 10;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 5;
				tmp.w = 1;
				tmp.h = 10;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 31;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );

				// draw the big picture
				tmp.x = MENU_OFFSET_X + 11;
				tmp.y = MENU_OFFSET_Y + 13;
				tmp.w = ptr->ID.getVehicle()->info->w;
				tmp.h = ptr->ID.getVehicle()->info->h;
				SDL_BlitSurface ( ptr->ID.getVehicle()->info, NULL, buffer, &tmp );


				// Ggf die Beschreibung ausgeben:

				if ( SettingsData.bShowDescription )
				{
					tmp.x += 10;
					tmp.y += 10;
					tmp.w -= 20;
					tmp.h -= 20;
					font->showTextAsBlock ( tmp, ptr->ID.getVehicle()->text );

				}

				// Die Details anzeigen:
				{
					SDL_Rect tmp2;
					tmp.x = 11;
					tmp.y = 290;
					tmp2.x = MENU_OFFSET_X + 11;
					tmp2.y = MENU_OFFSET_Y + 290;
					tmp.w = tmp2.w = 260;
					tmp.h = tmp2.h = 176;
					SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &tmp, buffer, &tmp2 );
					cVehicle tv(ptr->ID.getVehicle(), Client->ActivePlayer);
					tv.ShowBigDetails();
				}

				// Die Bauzeiten eintragen:
				int iTurboBuildRounds[3];

				int iTurboBuildCosts[3];

				CalcTurboBuild ( iTurboBuildRounds, iTurboBuildCosts, ptr->ID.getUnitData ( owner )->iBuilt_Costs );

				//sprintf ( str,"%d",iTurboBuildRounds[0]) ;
				font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 350, iToStr ( iTurboBuildRounds[0] ) );

				//sprintf ( str,"%d",iTurboBuildCosts[0] );
				font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 350, iToStr ( iTurboBuildCosts[0] ) );


				if ( iTurboBuildRounds[1] > 0 )
				{

					//sprintf ( str,"%d", iTurboBuildRounds[1] );
					font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 375, iToStr ( iTurboBuildRounds[1] ) );

					//sprintf ( str,"%d", iTurboBuildCosts[1] );
					font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 375, iToStr ( iTurboBuildCosts[1] ) );

				}

				if ( iTurboBuildRounds[2] > 0 )
				{
					font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 400, iToStr ( iTurboBuildRounds[2] ) );
					//sprintf ( str,"%d", iTurboBuildRounds[2] );

					font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 400, iToStr ( iTurboBuildCosts[2] ) );
					//sprintf ( str,"%d", iTurboBuildCosts[2] );

				}
			}
			else //showInfo == false
			{
				//einfachen Rahmen drum malen
				SDL_Rect tmp;
				tmp = dest;
				tmp.x -= 4;
				tmp.y -= 4;
				tmp.h = 1;
				tmp.w = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 4;
				tmp.w = 1;
				tmp.h = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 31;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
			}
		}

		// Text ausgeben:
		string sTmp = ptr->ID.getUnitData()->name;

		if ( font->getTextWide ( sTmp, FONT_LATIN_SMALL_WHITE ) > text.w )
		{
			text.y -= font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
			font->showTextAsBlock ( text, sTmp, FONT_LATIN_SMALL_WHITE);
			text.y += font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
		}
		else
		{
			font->showText ( text, sTmp, FONT_LATIN_SMALL_WHITE);
		}


		font->showTextCentered ( MENU_OFFSET_X + 616, text.y, iToStr ( ptr->ID.getUnitData ( owner )->iBuilt_Costs ), FONT_LATIN_SMALL_WHITE );
		text.y += 32 + 10;
		dest.y += 32 + 10;
	}
}

//--------------------------------------------------------------------------
/** draws the Buildspeed-Buttons */
//--------------------------------------------------------------------------
void cBuilding::DrawBuildButtons ( int speed )
{
	SDL_Rect rBtnSpeed1 = {MENU_OFFSET_X + 292, MENU_OFFSET_Y + 345, BUTTON__W, BUTTON__H};
	SDL_Rect rBtnSpeed2 = {MENU_OFFSET_X + 292, MENU_OFFSET_Y + 370, BUTTON__W, BUTTON__H};
	SDL_Rect rBtnSpeed3 = {MENU_OFFSET_X + 292, MENU_OFFSET_Y + 395, BUTTON__W, BUTTON__H};

	string sTmp = lngPack.i18n ( "Text~Button~Build" );

	if ( speed == 0 )
		drawButton ( sTmp + " x1", true, rBtnSpeed1.x, rBtnSpeed1.y, buffer );
	else
		drawButton ( sTmp + " x1", false, rBtnSpeed1.x, rBtnSpeed1.y, buffer );

	if ( speed == 1 )
		drawButton ( sTmp + " x2", true, rBtnSpeed2.x, rBtnSpeed2.y, buffer );
	else
		drawButton ( sTmp + " x2", false, rBtnSpeed2.x, rBtnSpeed2.y, buffer );

	if ( speed == 2 )
		drawButton ( sTmp + " x4", true, rBtnSpeed3.x, rBtnSpeed3.y, buffer );
	else
		drawButton ( sTmp + " x4", false, rBtnSpeed3.x, rBtnSpeed3.y, buffer );
}

//--------------------------------------------------------------------------
/** Draws the list with all build orders. If showInfo==true additionally all details for the selected unit. */
//--------------------------------------------------------------------------
void cBuilding::ShowToBuildList(cList<sBuildStruct*>& list, int const selected, int const offset, bool const showInfo)
{
	sBuildStruct *ptr;
	SDL_Rect src, dest, text = { MENU_OFFSET_X + 375, MENU_OFFSET_Y + 70, 80, 16};
	src.x = 330;
	src.y = 49;
	dest.x = MENU_OFFSET_X + 330;
	dest.y = MENU_OFFSET_Y + 49;
	src.w = 128;
	src.h = 233;
	SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &src, buffer, &dest );


	src.x = 0;
	src.y = 0;
	src.w = 32;
	src.h = 32;
	dest.x = MENU_OFFSET_X + 340;
	dest.y = MENU_OFFSET_Y + 58;

	for (unsigned int i = offset; i < list.Size(); i++)
	{
		if ( (int)i >= offset + 5 )
			break;

		ptr = list[i];

		// Das Bild malen:
		SDL_BlitSurface ( ptr->sf, &src, buffer, &dest );

		if ( selected == i )
		{
			if (showInfo)
			{
				//dopelten Rahmen drum malen
				SDL_Rect tmp, tmp2;
				tmp = dest;
				tmp.x -= 3;
				tmp.y -= 3;
				tmp.h = 1;
				tmp.w = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 28;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 28;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 3;
				tmp.w = 1;
				tmp.h = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 29;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 36;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );

				tmp = dest;
				tmp.x -= 5;
				tmp.y -= 5;
				tmp.h = 1;
				tmp.w = 10;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 5;
				tmp.w = 1;
				tmp.h = 10;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 31;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 40;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );

				// Draw the big picture
				tmp.x = MENU_OFFSET_X + 11;
				tmp.y = MENU_OFFSET_Y + 13;
				tmp.w = ptr->ID.getVehicle()->info->w;
				tmp.h = ptr->ID.getVehicle()->info->h;
				SDL_BlitSurface ( ptr->ID.getVehicle()->info, NULL, buffer, &tmp );


				// Ggf die Beschreibung ausgeben:

				if ( SettingsData.bShowDescription )
				{
					tmp.x += 10;
					tmp.y += 10;
					tmp.w -= 20;
					tmp.h -= 20;
					font->showTextAsBlock ( tmp, ptr->ID.getVehicle()->text );

				}

				// Die Details anzeigen:
				{
					tmp.x = 11;
					tmp.y = 290;
					tmp2.x = MENU_OFFSET_X + 11;
					tmp2.y = MENU_OFFSET_Y + 290;
					tmp.w = tmp2.w = 260;
					tmp.h = tmp2.h = 176;
					SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &tmp, buffer, &tmp2 );
					cVehicle tv(ptr->ID.getVehicle(), Client->ActivePlayer);
					tv.ShowBigDetails();
				}

				// Die Bauzeiten eintragen:
				int iTurboBuildRounds[3];

				int iTurboBuildCosts[3];

				CalcTurboBuild ( iTurboBuildRounds, iTurboBuildCosts, ptr->ID.getUnitData( owner )->iBuilt_Costs, ptr->iRemainingMetal );

				tmp.x = 373;
				tmp.y = 344;
				tmp2.x = MENU_OFFSET_X + 373;
				tmp2.y = MENU_OFFSET_Y + 344;
				tmp.w = tmp2.w = 77;
				tmp.h = tmp2.h = 72;
				SDL_BlitSurface ( GraphicsData.gfx_fac_build_screen, &tmp, buffer, &tmp2 );

				//sprintf ( str,"%d",iTurboBuildRounds[0]);
				font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 350, iToStr ( iTurboBuildRounds[0] ) );
				//sprintf ( str,"%d",iTurboBuildCosts[0] );
				font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 350, iToStr ( iTurboBuildCosts[0] ) );

				if ( iTurboBuildRounds[1] > 0 )
				{
					//sprintf ( str,"%d", iTurboBuildRounds[1] );
					font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 375, iToStr ( iTurboBuildRounds[1] ) );
					//sprintf ( str,"%d", iTurboBuildCosts[1] );
					font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 375, iToStr ( iTurboBuildCosts[1] ) );
				}

				if ( iTurboBuildRounds[2] > 0 )
				{
					//sprintf ( str,"%d", iTurboBuildRounds[2] );
					font->showTextCentered ( MENU_OFFSET_X + 389, MENU_OFFSET_Y + 400, iToStr ( iTurboBuildRounds[2] ) );
					//sprintf ( str,"%d", iTurboBuildCosts[2] );
					font->showTextCentered ( MENU_OFFSET_X + 429, MENU_OFFSET_Y + 400, iToStr ( iTurboBuildCosts[2] ) );
				}
			}
			else
			{
				//einfachen Rahmen drum
				SDL_Rect tmp;
				tmp = dest;
				tmp.x -= 4;
				tmp.y -= 4;
				tmp.h = 1;
				tmp.w = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 30;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y = dest.y - 4;
				tmp.w = 1;
				tmp.h = 8;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x += 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.y += 31;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
				tmp.x -= 38;
				SDL_FillRect ( buffer, &tmp, 0xE0E0E0 );
			}
		}

		// Text ausgeben:
		string sTmp = ptr->ID.getUnitData()->name;
		if ( font->getTextWide ( sTmp, FONT_LATIN_SMALL_WHITE ) > text.w )
		{
			text.y -= font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
			font->showTextAsBlock ( text, sTmp, FONT_LATIN_SMALL_WHITE);
			text.y += font->getFontHeight(FONT_LATIN_SMALL_WHITE) / 2;
		}
		else
			font->showText ( text, sTmp, FONT_LATIN_SMALL_WHITE);

		text.y += 32 + 10;
		dest.y += 32 + 10;
	}
}

//--------------------------------------------------------------------------
/** calculates the costs and the duration of the 3 buildspeeds for the vehicle with the given id
	iRemainingMetal is only needed for recalculating costs of vehicles in the Buildqueue and is set per default to -1 */
//--------------------------------------------------------------------------
void cBuilding::CalcTurboBuild ( int *iTurboBuildRounds, int *iTurboBuildCosts, int iVehicleCosts, int iRemainingMetal )
{
	//first calc costs for a new Vehical
	iTurboBuildCosts[0] = iVehicleCosts;

	iTurboBuildCosts[1] = iTurboBuildCosts[0];

	while ( iTurboBuildCosts[1] + ( 2 * data.iNeeds_Metal ) <= 2*iTurboBuildCosts[0] )
	{
		iTurboBuildCosts[1] += 2 * data.iNeeds_Metal;
	}

	iTurboBuildCosts[2] = iTurboBuildCosts[1];

	while ( iTurboBuildCosts[2] + ( 4 * data.iNeeds_Metal ) <= 3*iTurboBuildCosts[0] )
	{
		iTurboBuildCosts[2] += 4 * data.iNeeds_Metal;
	}

	//now this is a litle bit tricky ...
	//trying to calculate a plausible value, if we are changing the speed of an already started build-job
	if ( iRemainingMetal >= 0 )
	{
		float WorkedRounds;

		switch ( BuildSpeed )  //BuildSpeed here is the previous build speed
		{

			case 0:
				WorkedRounds = ( iTurboBuildCosts[0] - iRemainingMetal ) / ( float ) ( 1 * data.iNeeds_Metal );
				iTurboBuildCosts[0] -= ( int ) ( 1    *  1 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[1] -= ( int ) ( 0.5  *  4 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[2] -= ( int ) ( 0.25 * 12 * data.iNeeds_Metal * WorkedRounds );
				break;

			case 1:
				WorkedRounds = ( iTurboBuildCosts[1] - iRemainingMetal ) / ( float ) ( 4 * data.iNeeds_Metal );
				iTurboBuildCosts[0] -= ( int ) ( 2   *  1 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[1] -= ( int ) ( 1   *  4 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[2] -= ( int ) ( 0.5 * 12 * data.iNeeds_Metal * WorkedRounds );
				break;

			case 2:
				WorkedRounds = ( iTurboBuildCosts[2] - iRemainingMetal ) / ( float ) ( 12 * data.iNeeds_Metal );
				iTurboBuildCosts[0] -= ( int ) ( 4 *  1 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[1] -= ( int ) ( 2 *  4 * data.iNeeds_Metal * WorkedRounds );
				iTurboBuildCosts[2] -= ( int ) ( 1 * 12 * data.iNeeds_Metal * WorkedRounds );
				break;
		}
	}


	//calc needed Rounds
	iTurboBuildRounds[0] = ( int ) ceil ( iTurboBuildCosts[0] / ( double ) ( 1 * data.iNeeds_Metal ) );

	if ( data.can_build != BUILD_MAN )
	{
		iTurboBuildRounds[1] = ( int ) ceil ( iTurboBuildCosts[1] / ( double ) ( 4 * data.iNeeds_Metal ) );
		iTurboBuildRounds[2] = ( int ) ceil ( iTurboBuildCosts[2] / ( double ) ( 12 * data.iNeeds_Metal ) );
	}
	else
	{
		iTurboBuildRounds[1] = 0;
		iTurboBuildRounds[2] = 0;
	}

	//now avoid different costs at the same number of rounds

	/* macht mehr Probleme, als dass es hilft
	switch (BuildSpeed) //old buildspeed
	{
		case 0:
			if (iTurboBuildRounds[1] == iTurboBuildRounds[0]) iTurboBuildCosts[1] = iTurboBuildCosts[0];
			if (iTurboBuildRounds[2] == iTurboBuildRounds[0]) iTurboBuildCosts[2] = iTurboBuildCosts[0];
			break;
		case 1:
			if (iTurboBuildRounds[0] == iTurboBuildRounds[1]) iTurboBuildCosts[0] = iTurboBuildCosts[1];
			if (iTurboBuildRounds[2] == iTurboBuildRounds[1]) iTurboBuildCosts[2] = iTurboBuildCosts[1];
			break;
		case 2:
			if (iTurboBuildRounds[0] == iTurboBuildRounds[2]) iTurboBuildCosts[1] = iTurboBuildCosts[2];
			if (iTurboBuildRounds[1] == iTurboBuildRounds[2]) iTurboBuildCosts[2] = iTurboBuildCosts[2];
			break;
	}*/
}

//--------------------------------------------------------------------------
/** Returns the screen x position of the building */
//--------------------------------------------------------------------------
int cBuilding::GetScreenPosX () const
{
	return 180 - ( ( int ) ( ( Client->Hud.OffX ) / ( 64.0 / Client->Hud.Zoom ) ) ) + Client->Hud.Zoom*PosX;
}

//--------------------------------------------------------------------------
/** Returns the screen x position of the building */
//--------------------------------------------------------------------------
int cBuilding::GetScreenPosY () const
{
	return 18 - ( ( int ) ( ( Client->Hud.OffY ) / ( 64.0 / Client->Hud.Zoom ) ) ) + Client->Hud.Zoom*PosY;
}

//--------------------------------------------------------------------------
/** returns the remaining hitpoints after an attack */
//--------------------------------------------------------------------------
int cBuilding::CalcHelth ( int damage )
{
	damage -= data.armor;

	if ( damage <= 0 )
	{
		//minimum damage is 1
		damage = 1;
	}

	int hp;
	hp = data.hit_points - damage;

	if ( hp < 0 )
	{
		return 0;
	}

	return hp;
}

//--------------------------------------------------------------------------
/** draws the building menu */
//--------------------------------------------------------------------------
void cBuilding::DrawMenu ( sMouseState *mouseState )
{
	int nr = 0, SelMenu = -1, ExeNr = -1;
	static int LastNr = -1;
	bool bSelection = false;
	SDL_Rect dest;
	dest = GetMenuSize();

	if ( bIsBeeingAttacked ) return;

	if ( ActivatingVehicle )
	{
		MenuActive = false;
		return;
	}

	if (BuildList && BuildList->Size() && !IsWorking && (*BuildList)[0]->metall_remaining <= 0)
		return;

	if ( mouseState && mouseState->leftButtonReleased && MouseOverMenu ( mouse->x, mouse->y ) )
	{
		SelMenu = ( mouse->y - dest.y ) / 22;
		LastNr = SelMenu;
	}
	else
		if ( MouseOverMenu ( mouse->x, mouse->y ) )
		{
			ExeNr = LastNr;
			LastNr = -1;
		}
		else
		{
			SelMenu = -1;
			LastNr = -1;
		}

	// Angriff:
	if ( typ->data.can_attack && data.shots )
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }


		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			AttackMode = true;
			Client->Hud.CheckScroll();
			Client->mouseMoveCallback ( true );
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Attack" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Bauen:
	if ( typ->data.can_build )
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			ShowBuildMenu();
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Build" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Verteilen:
	if ( typ->data.is_mine && IsWorking )
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			showMineManager();
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Dist" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Transfer:
	if ( typ->data.can_load == TRANS_METAL || typ->data.can_load == TRANS_OIL || typ->data.can_load == TRANS_GOLD )
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			Transfer = true;
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Transfer" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Start:
	if (typ->data.can_work &&
			!IsWorking         &&
			(
				(BuildList && BuildList->Size()) ||
				typ->data.can_build == BUILD_NONE
			))
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			sendWantStartWork(this);
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Start" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Stop:
	if ( IsWorking )
	{
		if ( SelMenu == nr ) { bSelection = true; }
		else { bSelection = false; }

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			sendWantStopWork(this);
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Stop" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Sentry status:
	if ( bSentryStatus || data.can_attack )
	{
		bSelection = SelMenu == nr || bSentryStatus;

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			sendChangeSentry ( iID, false );
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Sentry" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// Aktivieren/Laden:
	if ( typ->data.can_load == TRANS_VEHICLES || typ->data.can_load == TRANS_MEN || typ->data.can_load == TRANS_AIR )
	{
		// Aktivieren:
		if ( SelMenu == nr ) bSelection = true;
		else bSelection = false;

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			ShowStorage();
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Active" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
		// Laden:

		if ( SelMenu == nr || LoadActive ) bSelection = true;
		else bSelection = false;

		if ( ExeNr == nr )
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			LoadActive = !LoadActive;
			return;
		}

		drawContextItem( lngPack.i18n ( "Text~Context~Load" ), bSelection, dest.x, dest.y, buffer );

		dest.y += 22;
		nr++;
	}

	// research
	if (typ->data.can_research && IsWorking)
	{
		bSelection = (SelMenu == nr);
		if (ExeNr == nr)
		{
			MenuActive = false;
			PlayFX (SoundData.SNDObjectMenu);
			ShowResearch();
			return;
		}
		drawContextItem (lngPack.i18n ("Text~Context~Research"), bSelection, dest.x, dest.y, buffer);
		dest.y += 22;
		nr++;
	}

	// upgradescreen
	if (data.gold_need)
	{
		// update this
		bSelection = (SelMenu == nr);
		if (ExeNr == nr)
		{
			MenuActive = false;
			PlayFX (SoundData.SNDObjectMenu);
			ShowUpgrade();
			return;
		}
		drawContextItem (lngPack.i18n ("Text~Context~Upgrades"), bSelection, dest.x, dest.y, buffer);
		dest.y += 22;
		nr++;
	}

	// Updates:
	if ( data.version != owner->BuildingData[typ->nr].version && SubBase && SubBase->Metal >= 2 )
	{
		// Update all buildings of this type in this subbase
		bSelection = (SelMenu == nr);
		if (ExeNr == nr)
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			sendUpgradeBuilding (this, true);
			return;
		}
		drawContextItem (lngPack.i18n ("Text~Context~UpAll"), bSelection, dest.x, dest.y, buffer);
		dest.y += 22;
		nr++;

		// update this building
		bSelection = (SelMenu == nr);
		if (ExeNr == nr)
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			sendUpgradeBuilding (this, false);
			return;
		}
		drawContextItem (lngPack.i18n ("Text~Context~Upgrade"), bSelection, dest.x, dest.y, buffer);
		dest.y += 22;
		nr++;
	}

	// Self destruct
	if (!data.is_road)
	{
		bSelection = (SelMenu == nr);
		if (ExeNr == nr)
		{
			MenuActive = false;
			PlayFX ( SoundData.SNDObjectMenu );
			//TODO: implement self destruction
			Client->addMessage ( lngPack.i18n ( "Text~Error_Messages~INFO_Not_Implemented" ) );
			//SelfDestructionMenu();
			return;
		}
		drawContextItem (lngPack.i18n ("Text~Context~Destroy"), bSelection, dest.x, dest.y, buffer);
		dest.y += 22;
		nr++;
	}

	// Info:
	bSelection = (SelMenu == nr);
	if (ExeNr == nr)
	{
		MenuActive = false;
		PlayFX (SoundData.SNDObjectMenu);
		ShowHelp();
		return;
	}
	drawContextItem (lngPack.i18n ("Text~Context~Info"), bSelection, dest.x, dest.y, buffer);
	dest.y += 22;
	nr++;

	// Done:
	bSelection = (SelMenu == nr);
	if (ExeNr == nr)
	{
		MenuActive = false;
		PlayFX (SoundData.SNDObjectMenu);
		return;
	}
	drawContextItem (lngPack.i18n ("Text~Context~Done"), bSelection, dest.x, dest.y, buffer);
}

//------------------------------------------------------------------------
void cBuilding::sendUpgradeBuilding (cBuilding* building, bool upgradeAll)
{	
	if (building == 0 || building->owner == 0)
		return;
	
	sUnitData& currentVersion = building->data;
	sUnitData& upgradedVersion = building->owner->BuildingData[building->typ->nr];
	if (currentVersion.version >= upgradedVersion.version)
		return; // already uptodate

	cNetMessage* msg = new cNetMessage (GAME_EV_WANT_BUILDING_UPGRADE);
	msg->pushBool (upgradeAll);
	msg->pushInt32 (building->iID);
	
	Client->sendNetMessage (msg);	
}

//------------------------------------------------------------------------
void cBuilding::upgradeToCurrentVersion ()
{
	sUnitData& upgradeVersion = owner->BuildingData[typ->nr];
	data.version = upgradeVersion.version; // TODO: iVersion?
	
	if (data.hit_points == data.max_hit_points)
		data.hit_points = upgradeVersion.max_hit_points; // TODO: check behaviour in original
	data.max_hit_points = upgradeVersion.max_hit_points;

	data.max_ammo = upgradeVersion.max_ammo; // don't change the current ammo-amount!
	
	data.armor = upgradeVersion.armor;
	data.scan = upgradeVersion.scan;
	data.range = upgradeVersion.range;
	data.max_shots = upgradeVersion.max_shots; // TODO: check behaviour in original
	data.damage = upgradeVersion.damage;
	data.iBuilt_Costs = upgradeVersion.iBuilt_Costs;

	GenerateName();

	if (this == Client->SelectedBuilding)
		ShowDetails();
}


//------------------------------------------------------------------------
/** centers on this building */
//--------------------------------------------------------------------------
void cBuilding::Center ()
{
	Client->Hud.OffX = PosX * 64 - ( ( int ) ( ( ( float ) (SettingsData.iScreenW - 192) / (2 * Client->Hud.Zoom) ) * 64 ) ) + 32;
	Client->Hud.OffY = PosY * 64 - ( ( int ) ( ( ( float ) (SettingsData.iScreenH - 32 ) / (2 * Client->Hud.Zoom) ) * 64 ) ) + 32;
	Client->bFlagDrawMap = true;
	Client->Hud.DoScroll ( 0 );
}

//------------------------------------------------------------------------
/** draws the available ammunition over the building: */
//--------------------------------------------------------------------------
void cBuilding::DrawMunBar ( void ) const
{
	SDL_Rect r1, r2;
	r1.x = GetScreenPosX() + Client->Hud.Zoom/10 + 1;
	r1.w = Client->Hud.Zoom * 8 / 10 ;
	r1.h = Client->Hud.Zoom / 8;
	r1.y = GetScreenPosY() + Client->Hud.Zoom/10 + Client->Hud.Zoom / 8;

	if ( r1.h <= 2 )
	{
		r1.h = 3;
		r1.y += 1;
	}

	r2.x = r1.x + 1;
	r2.y = r1.y + 1;
	r2.h = r1.h - 2;
	r2.w = ( int ) ( ( ( float ) ( r1.w - 2 ) ) / data.max_ammo  * data.ammo );

	SDL_FillRect ( buffer, &r1, 0 );

	if ( data.ammo > data.max_ammo / 2 )
	{
		SDL_FillRect ( buffer, &r2, 0x04AE04 );
	}
	else if ( data.ammo > data.max_ammo / 4 )
	{
		SDL_FillRect ( buffer, &r2, 0xDBDE00 );
	}
	else
	{
		SDL_FillRect ( buffer, &r2, 0xE60000 );
	}
}

//------------------------------------------------------------------------
/** draws the health bar over the building */
//--------------------------------------------------------------------------
void cBuilding::DrawHelthBar ( void ) const
{
	SDL_Rect r1, r2;
	r1.x = GetScreenPosX() + Client->Hud.Zoom/10 + 1;
	r1.w = Client->Hud.Zoom * 8 / 10 ;
	r1.h = Client->Hud.Zoom / 8;
	r1.y = GetScreenPosY() + Client->Hud.Zoom/10;

	if ( data.is_big )
	{
		r1.w += Client->Hud.Zoom;
		r1.h *= 2;
	}

	if ( r1.h <= 2  )
		r1.h = 3;

	r2.x = r1.x + 1;
	r2.y = r1.y + 1;
	r2.h = r1.h - 2;
	r2.w = ( int ) ( ( ( float ) ( r1.w - 2 ) / data.max_hit_points ) * data.hit_points );

	SDL_FillRect ( buffer, &r1, 0 );

	if ( data.hit_points > data.max_hit_points / 2 )
	{
		SDL_FillRect ( buffer, &r2, 0x04AE04 );
	}
	else if ( data.hit_points > data.max_hit_points / 4 )
	{
		SDL_FillRect ( buffer, &r2, 0xDBDE00 );
	}
	else
	{
		SDL_FillRect ( buffer, &r2, 0xE60000 );
	}
}

//--------------------------------------------------------------------------
void cBuilding::drawStatus() const
{
	SDL_Rect dest;
	SDL_Rect shotsSymbol = {254, 97, 5, 10 };
	SDL_Rect disabledSymbol = {150, 109, 25, 25};

	if ( Disabled )
	{
		if ( Client->Hud.Zoom < 25 ) return;
		dest.x = GetScreenPosX() + Client->Hud.Zoom/2 - 12;
		dest.y = GetScreenPosY() + Client->Hud.Zoom/2 - 12;
		SDL_BlitSurface( GraphicsData.gfx_hud_stuff, &disabledSymbol, buffer, &dest );
	}
	else
	{
		dest.y = GetScreenPosY() + Client->Hud.Zoom - 11;
		dest.x = GetScreenPosX() + Client->Hud.Zoom/2 - 4;
		if ( data.shots )
		{
			SDL_BlitSurface( GraphicsData.gfx_hud_stuff, &shotsSymbol, buffer, &dest );
		}
	}
}

//--------------------------------------------------------------------------
void cBuilding::Select ()
{
	selected = true;
	// Das Video laden:
	if ( Client->FLC != NULL )
	{
		FLI_Close ( Client->FLC );
		Client->FLC = NULL;
		Client->sFLCname = "";
	}
	Client->video = typ->video;

	// Sound abspielen:
	if ( !IsWorking )
		PlayFX ( SoundData.SNDHudButton );

	// Die Eigenschaften anzeigen:
	ShowDetails();
}

//--------------------------------------------------------------------------
void cBuilding::Deselct ()
{
	SDL_Rect src, dest;
	selected = false;
	MenuActive = false;
	AttackMode = false;
	Transfer = false;
	LoadActive = false;
	ActivatingVehicle = false;
	// Den Hintergrund wiederherstellen:
	src.x = 0;
	src.y = 215;
	src.w = 155;
	src.h = 48;
	dest.x = 8;
	dest.y = 171;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, GraphicsData.gfx_hud, &dest );
	StopFXLoop ( Client->iObjectStream );
	Client->iObjectStream = -1;
	Client->bFlagDrawHud = true;
}

//--------------------------------------------------------------------------
void cBuilding::ShowDetails ()
{
	SDL_Rect src, dest;
	// Den Hintergrund wiederherstellen:
	src.x = 0;
	src.y = 215;
	src.w = 155;
	src.h = 48;
	dest.x = 8;
	dest.y = 171;
	SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &src, GraphicsData.gfx_hud, &dest );
	// Die Hitpoints anzeigen:
	DrawNumber ( 31, 177, data.hit_points, data.max_hit_points, GraphicsData.gfx_hud );
	font->showText ( 55, 177, lngPack.i18n ( "Text~Hud~Hitpoints" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

	DrawSymbol ( SHits, 88, 174, 70, data.hit_points, data.max_hit_points, GraphicsData.gfx_hud );
	// additional values:

	if ( data.can_load && owner == Client->ActivePlayer )
	{
		// Load:
		DrawNumber ( 31, 189, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
		font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~Cargo" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

		switch ( data.can_load )
		{

			case TRANS_METAL:
				DrawSymbol ( SMetal, 88, 186, 70, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;

			case TRANS_OIL:
				DrawSymbol ( SOil, 88, 186, 70, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;

			case TRANS_GOLD:
				DrawSymbol ( SGold, 88, 187, 70, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;

			case TRANS_VEHICLES:
				DrawSymbol ( STrans, 88, 186, 70, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;

			case TRANS_MEN:
				DrawSymbol ( SHuman, 88, 186, 70, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;

			case TRANS_AIR:
				DrawSymbol ( SAir, 88, 186, 50, data.cargo, data.max_cargo, GraphicsData.gfx_hud );
				break;
		}

		// Gesamt:
		if ( data.can_load == TRANS_METAL || data.can_load == TRANS_OIL || data.can_load == TRANS_GOLD )
		{
			font->showText ( 55, 201, lngPack.i18n ( "Text~Hud~Total" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			switch ( data.can_load )
			{

				case TRANS_METAL:
					DrawNumber ( 31, 201, SubBase->Metal, SubBase->MaxMetal, GraphicsData.gfx_hud );
					DrawSymbol ( SMetal, 88, 198, 70, SubBase->Metal, SubBase->MaxMetal, GraphicsData.gfx_hud );
					break;

				case TRANS_OIL:
					DrawNumber ( 31, 201, SubBase->Oil, SubBase->MaxOil, GraphicsData.gfx_hud );
					DrawSymbol ( SOil, 88, 198, 70, SubBase->Oil, SubBase->MaxOil, GraphicsData.gfx_hud );
					break;

				case TRANS_GOLD:
					DrawNumber ( 31, 201, SubBase->Gold, SubBase->MaxGold, GraphicsData.gfx_hud );
					DrawSymbol ( SGold, 88, 199, 70, SubBase->Gold, SubBase->MaxGold, GraphicsData.gfx_hud );
					break;
			}
		}
	}
	else if ( data.can_attack && !data.is_expl_mine )
	{
		if ( owner == Client->ActivePlayer )
		{
			// Munition:
			DrawNumber ( 31, 189, data.ammo, data.max_ammo, GraphicsData.gfx_hud );
			font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~AmmoShort" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawSymbol ( SAmmo, 88, 187, 70, data.ammo, data.max_ammo, GraphicsData.gfx_hud );
		}

		// shots:
		DrawNumber ( 31, 212, data.shots, data.max_shots, GraphicsData.gfx_hud );

		font->showText ( 55, 212, lngPack.i18n ( "Text~Hud~Shots" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

		DrawSymbol ( SShots, 88, 212, 70, data.shots, data.max_shots, GraphicsData.gfx_hud );
	}
	else if ( data.energy_prod )
	{
		// EnergieProduktion:
		DrawNumber ( 31, 189, ( IsWorking ? data.energy_prod : 0 ), data.energy_prod, GraphicsData.gfx_hud );
		font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~Energy" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

		DrawSymbol ( SEnergy, 88, 187, 70, ( IsWorking ? data.energy_prod : 0 ), data.energy_prod, GraphicsData.gfx_hud );

		if ( owner == Client->ActivePlayer )
		{
			// Gesammt:
			font->showText ( 55, 201, lngPack.i18n ( "Text~Hud~Total" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawNumber ( 31, 201, SubBase->EnergyProd, SubBase->MaxEnergyProd, GraphicsData.gfx_hud );
			DrawSymbol ( SEnergy, 88, 199, 70, SubBase->EnergyProd, SubBase->MaxEnergyProd, GraphicsData.gfx_hud );
			// Verbrauch:
			font->showText ( 55, 212, lngPack.i18n ( "Text~Hud~Usage" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawNumber ( 31, 212, SubBase->EnergyNeed, SubBase->MaxEnergyNeed, GraphicsData.gfx_hud );
			DrawSymbol ( SEnergy, 88, 212, 70, SubBase->EnergyNeed, SubBase->MaxEnergyNeed, GraphicsData.gfx_hud );
		}
	}
	else if ( data.human_prod )
	{
		// HumanProduktion:
		DrawNumber ( 31, 189, data.human_prod, data.human_prod, GraphicsData.gfx_hud );
		font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~Teams" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

		DrawSymbol ( SHuman, 88, 187, 70, data.human_prod, data.human_prod, GraphicsData.gfx_hud );

		if ( owner == Client->ActivePlayer )
		{
			// Gesammt:
			font->showText ( 55, 201, lngPack.i18n ( "Text~Hud~Total" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawNumber ( 31, 201, SubBase->HumanProd, SubBase->HumanProd, GraphicsData.gfx_hud );
			DrawSymbol ( SHuman, 88, 199, 70, SubBase->HumanProd, SubBase->HumanProd, GraphicsData.gfx_hud );
			// Verbrauch:
			font->showText ( 55, 212, lngPack.i18n ( "Text~Hud~Usage" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawNumber ( 31, 212, SubBase->HumanNeed, SubBase->MaxHumanNeed, GraphicsData.gfx_hud );
			DrawSymbol ( SHuman, 88, 210, 70, SubBase->HumanNeed, SubBase->MaxHumanNeed, GraphicsData.gfx_hud );
		}
	}
	else if ( data.human_need )
	{
		// HumanNeed:
		if ( IsWorking )
		{
			DrawNumber ( 31, 189, data.human_need, data.human_need, GraphicsData.gfx_hud );
			font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~Usage" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawSymbol ( SHuman, 88, 187, 70, data.human_need, data.human_need, GraphicsData.gfx_hud );
		}
		else
		{
			DrawNumber ( 31, 189, 0, data.human_need, GraphicsData.gfx_hud );
			font->showText ( 55, 189, lngPack.i18n ( "Text~Hud~Usage" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawSymbol ( SHuman, 88, 187, 70, 0, data.human_need, GraphicsData.gfx_hud );
		}

		if ( owner == Client->ActivePlayer )
		{
			// Gesammt:
			font->showText ( 55, 201, lngPack.i18n ( "Text~Hud~Total" ), FONT_LATIN_SMALL_WHITE, GraphicsData.gfx_hud );

			DrawNumber ( 31, 201, SubBase->HumanNeed, SubBase->MaxHumanNeed, GraphicsData.gfx_hud );
			DrawSymbol ( SHuman, 88, 199, 70, SubBase->HumanNeed, SubBase->MaxHumanNeed, GraphicsData.gfx_hud );
		}
	}
	Client->bFlagDrawHud = true;
}

//--------------------------------------------------------------------------
/** draws a row of symbols */
//--------------------------------------------------------------------------
void cBuilding::DrawSymbol ( eSymbols sym, int x, int y, int maxx, int value, int maxvalue, SDL_Surface *sf )
{
	SDL_Rect full, empty, dest;
	int i, to, step, offx;

	switch ( sym )
	{
		case SSpeed:
			full.x = 0;
			empty.y = full.y = 98;
			empty.w = full.w = 7;
			empty.h = full.h = 7;
			empty.x = 7;
			break;

		case SHits:
			empty.y = full.y = 98;
			empty.w = full.w = 6;
			empty.h = full.h = 9;

			if ( value > maxvalue / 2 )
			{
				full.x = 14;
				empty.x = 20;
			}
			else
				if ( value > maxvalue / 4 )
				{
					full.x = 26;
					empty.x = 32;
				}
				else
				{
					full.x = 38;
					empty.x = 44;
				}

			break;

		case SAmmo:
			full.x = 50;
			empty.y = full.y = 98;
			empty.w = full.w = 5;
			empty.h = full.h = 7;
			empty.x = 55;
			break;

		case SMetal:
			full.x = 60;
			empty.y = full.y = 98;
			empty.w = full.w = 7;
			empty.h = full.h = 10;
			empty.x = 67;
			break;

		case SEnergy:
			full.x = 74;
			empty.y = full.y = 98;
			empty.w = full.w = 7;
			empty.h = full.h = 7;
			empty.x = 81;
			break;

		case SShots:
			full.x = 88;
			empty.y = full.y = 98;
			empty.w = full.w = 8;
			empty.h = full.h = 4;
			empty.x = 96;
			break;

		case SOil:
			full.x = 104;
			empty.y = full.y = 98;
			empty.w = full.w = 8;
			empty.h = full.h = 9;
			empty.x = 112;
			break;

		case SGold:
			full.x = 120;
			empty.y = full.y = 98;
			empty.w = full.w = 9;
			empty.h = full.h = 8;
			empty.x = 129;
			break;

		case STrans:
			full.x = 138;
			empty.y = full.y = 98;
			empty.w = full.w = 16;
			empty.h = full.h = 8;
			empty.x = 154;
			break;

		case SHuman:
			full.x = 170;
			empty.y = full.y = 98;
			empty.w = full.w = 8;
			empty.h = full.h = 9;
			empty.x = 178;
			break;

		case SAir:
			full.x = 186;
			empty.y = full.y = 98;
			empty.w = full.w = 21;
			empty.h = full.h = 8;
			empty.x = 207;
			break;
	}

	to = maxvalue;

	step = 1;
	offx = full.w;

	while ( offx*to > maxx )
	{
		offx--;

		if ( offx < 4 )
		{
			to /= 2;
			step *= 2;
			offx = full.w;
		}
	}

	dest.x = x;
	dest.y = y;
	
	for ( i = 0;i < to;i++ )
	{
		if ( value > 0 )
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &full, sf, &dest );
		else
			SDL_BlitSurface ( GraphicsData.gfx_hud_stuff, &empty, sf, &dest );

		dest.x += offx;
		value -= step;
	}
}

//--------------------------------------------------------------------------
/** Draws a number/number on the surface */
//--------------------------------------------------------------------------
void cBuilding::DrawNumber ( int x, int y, int value, int maxvalue, SDL_Surface *sf )
{
	if ( value > maxvalue / 2 )
		font->showTextCentered ( x, y, iToStr ( value ) + "/" + iToStr ( maxvalue ), FONT_LATIN_SMALL_GREEN, sf );
	else if ( value > maxvalue / 4 )
		font->showTextCentered ( x, y, iToStr ( value ) + "/" + iToStr ( maxvalue ), FONT_LATIN_SMALL_YELLOW, sf );
	else
		font->showTextCentered ( x, y, iToStr ( value ) + "/" + iToStr ( maxvalue ), FONT_LATIN_SMALL_RED, sf );
}

//----------------------------------------------------------------
/** Playback of the soundstream that belongs to this building */
//----------------------------------------------------------------
int cBuilding::playStream ()
{
	if ( IsWorking )
		return PlayFXLoop ( typ->Running );

	return 0;
}

//--------------------------------------------------------------------------
/** Displays the help screen */
//--------------------------------------------------------------------------
void cBuilding::ShowHelp ()
{
#define DIALOG_W 640
#define DIALOG_H 480
#define BUTTON_W 150
#define BUTTON_H 29

	int LastMouseX = 0, LastMouseY = 0, LastB = 0, x, y, b;
	SDL_Rect rDialog = { SettingsData.iScreenW / 2 - DIALOG_W / 2, SettingsData.iScreenH / 2 - DIALOG_H / 2, DIALOG_W, DIALOG_H };
	SDL_Rect rDialogSrc = {0, 0, DIALOG_W, DIALOG_H};
	SDL_Rect rInfoTxt = {rDialog.x + 11, rDialog.y + 13, typ->info->w, typ->info->h};
	SDL_Rect rTxt = {rDialog.x + 345, rDialog.y + 66, 274, 181};
	SDL_Rect rTitle = {rDialog.x + 332, rDialog.y + 11, 152, 15};
	SDL_Surface *SfDialog;
	Client->isInMenu = true;

	PlayFX ( SoundData.SNDHudButton );
	mouse->SetCursor ( CHand );
	mouse->draw ( false, buffer );

	if ( SettingsData.bAlphaEffects )
		SDL_BlitSurface ( GraphicsData.gfx_shadow, NULL, buffer, NULL );

	SfDialog = SDL_CreateRGBSurface ( SDL_HWSURFACE | SDL_SRCCOLORKEY, DIALOG_W, DIALOG_H, SettingsData.iColourDepth, 0, 0, 0, 0 );
	if ( FileExists ( GFXOD_HELP ) )
		LoadPCXtoSF ( GFXOD_HELP, SfDialog );

	// Den Hilfebildschirm blitten:
	SDL_BlitSurface ( SfDialog, &rDialogSrc, buffer, &rDialog );

	// Das Infobild blitten:
	SDL_BlitSurface ( typ->info, NULL, buffer, &rInfoTxt );

	//show menu title
	font->showTextCentered ( rTitle.x + rTitle.w / 2, rTitle.y, lngPack.i18n ( "Text~Title~Unitinfo" ) );


	// show text
	font->showTextAsBlock ( rTxt, typ->text );

	// get unit details
	ShowBigDetails();

	NormalButton btn_done(rDialog.x + 474, rDialog.y + 452, "Text~Button~Done");
	btn_done.Draw();

	SHOW_SCREEN 	// Den Buffer anzeigen
	mouse->GetBack ( buffer );

	while ( 1 )
	{
		if (  Client->SelectedBuilding != this && !Client->bHelpActive ) break;
		if ( !Client->isInMenu ) break;

		Client->handleTimer();
		Client->doGameActions();

		// Events holen:
		EventHandler->HandleEvents();

		// Die Maus machen:
		mouse->GetPos();
		b = (int)Client->getMouseState().leftButtonPressed;
		x = mouse->x;
		y = mouse->y;

		if ( x != LastMouseX || y != LastMouseY )
			mouse->draw ( true, screen );

		if (btn_done.CheckClick(x, y, b > LastB, b < LastB))
			break;

		LastMouseX = x;
		LastMouseY = y;
		LastB = b;
	}

	SDL_FreeSurface ( SfDialog );
	Client->isInMenu = false;
}

//--------------------------------------------------------------------------
bool cBuilding::isDetectedByPlayer( cPlayer* player )
{
	for (unsigned int i = 0; i < DetectedByPlayerList.Size(); i++)
	{
		if ( DetectedByPlayerList[i] == player ) return true;
	}
	return false;
}

//--------------------------------------------------------------------------
void cBuilding::doMineInc(ResourceKind const resource, cList<sMineValues*>& Mines)
{
	for (unsigned int i = 0; i < Mines.Size(); ++i)
	{
		sMineValues& m = *Mines[i];
		if (m.iMetalProd + m.iOilProd + m.iGoldProd >= 16) continue;

		int&      prod     = m.GetProd(resource);
		int const max_prod = m.GetMaxProd(resource);
		if (prod < max_prod)
		{
			++prod;
			break;
		}
	}
}

//--------------------------------------------------------------------------
void cBuilding::doMineDec(ResourceKind const resource, cList<sMineValues*>& Mines)
{
	for (unsigned int i = 0; i < Mines.Size(); ++i)
	{
		int& prod = Mines[i]->GetProd(resource);
		if (prod > 0)
		{
			--prod;
			break;
		}
	}
}

//--------------------------------------------------------------------------
void cBuilding::calcMineFree ( cList<sMineValues*> *Mines, int *iFreeM, int *iFreeO, int *iFreeG )
{
	*iFreeM = 0;
	*iFreeO = 0;
	*iFreeG = 0;
	for (unsigned int i = 0; i < Mines->Size(); i++)
	{
		sMineValues* const m = (*Mines)[i];
		int iGes = m->iMetalProd + m->iOilProd + m->iGoldProd;
		if ( iGes < 16 )
		{
			int t;
			iGes = 16 - iGes;
			t = m->iMaxMetalProd - m->iMetalProd;
			*iFreeM += ( iGes < t ? iGes : t );
			t = m->iMaxOilProd - m->iOilProd;
			*iFreeO += ( iGes < t ? iGes : t );
			t = m->iMaxGoldProd - m->iGoldProd;
			*iFreeG += ( iGes < t ? iGes : t );
		}
	}
}

//--------------------------------------------------------------------------
void cBuilding::setDetectedByPlayer( cPlayer* player )
{
	if (!isDetectedByPlayer( player ))
		DetectedByPlayerList.Add( player );
}

//--------------------------------------------------------------------------
void cBuilding::resetDetectedByPlayer( cPlayer* player )
{
	for ( unsigned int i = 0; i < DetectedByPlayerList.Size(); i++ )
	{
		if ( DetectedByPlayerList[i] == player ) DetectedByPlayerList.Delete(i);
	}
}

//--------------------------------------------------------------------------
void cBuilding::makeDetection()
{
	//check whether the building has been detected by others
	if ( !data.is_expl_mine ) return;

	int offset = PosX + PosY * Server->Map->size;
	for ( unsigned int i = 0; i < Server->PlayerList->Size(); i++ )
	{
		cPlayer* player = (*Server->PlayerList)[i];
		if ( player == owner ) continue;
		if ( player->DetectMinesMap[offset] )
		{
			setDetectedByPlayer( player );
		}
	}
}

//--------------------------------------------------------------------------
void sBuilding::scaleSurfaces( float factor )
{
	scaleSurface ( img_org, img, (int)(img_org->w*factor), (int)(img_org->h*factor) );
	scaleSurface ( shw_org, shw, (int)(shw_org->w*factor), (int)(shw_org->h*factor) );
	if ( eff_org ) scaleSurface ( eff_org, eff, (int)(eff_org->w*factor), (int)(eff_org->h*factor) );
}
