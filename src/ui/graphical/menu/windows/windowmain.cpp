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

#include <functional>

#include "ui/graphical/menu/windows/windowmain.h"
#include "pcx.h"
#include "main.h"
#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "utility/random.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/image.h"
#include "maxrversion.h"

//------------------------------------------------------------------------------
cWindowMain::cWindowMain (const std::string& title) :
	cWindow (LoadPCX (GFXOD_MAIN))
{
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (0, 147), getPosition () + cPosition (getArea ().getMaxCorner ().x (), 157)), title, FONT_LATIN_NORMAL, eAlignmentType::CenterHorizontal));

	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (0, 465), getPosition () + cPosition (getArea ().getMaxCorner ().x (), 475)), lngPack.i18n ("Text~Main~Credits_Reloaded") + " " + PACKAGE_VERSION, FONT_LATIN_NORMAL, eAlignmentType::CenterHorizontal));

	infoImage = addChild (std::make_unique<cImage> (getPosition () + cPosition (16, 182), getRandomInfoImage (), &SoundData.SNDHudButton));
	signalConnectionManager.connect (infoImage->clicked, std::bind (&cWindowMain::infoImageClicked, this));
}

//------------------------------------------------------------------------------
cWindowMain::~cWindowMain ()
{}

//------------------------------------------------------------------------------
void cWindowMain::infoImageClicked ()
{
	infoImage->setImage (getRandomInfoImage());
}

//------------------------------------------------------------------------------
SDL_Surface* cWindowMain::getRandomInfoImage ()
{
	int const showBuilding = random (3);
	// I want 3 possible random numbers since a chance of 50:50 is boring
	// (and vehicles are way more cool so I prefer them to be shown) -- beko
	static int lastUnitShow = -1;
	int unitShow = -1;
	SDL_Surface* surface = NULL;

	if (showBuilding == 1 && UnitsData.getNrBuildings () > 0)
	{
		// that's a 33% chance that we show a building on 1
		do
		{
			unitShow = random (UnitsData.getNrBuildings () - 1);
			// make sure we don't show same unit twice
		}
		while (unitShow == lastUnitShow && UnitsData.getNrBuildings () > 1);
		surface = UnitsData.buildingUIs[unitShow].info.get();
	}
	else if (UnitsData.getNrVehicles () > 0)
	{
		// and a 66% chance to show a vehicle on 0 or 2
		do
		{
			unitShow = random (UnitsData.getNrVehicles () - 1);
			// make sure we don't show same unit twice
		}
		while (unitShow == lastUnitShow && UnitsData.getNrVehicles () > 1);
        surface = UnitsData.vehicleUIs[unitShow].info.get ();
	}
	else surface = NULL;
	lastUnitShow = unitShow; //store shown unit
	return surface;
}
