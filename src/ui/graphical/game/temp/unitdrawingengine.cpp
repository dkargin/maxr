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

#include <algorithm>

#include "ui/graphical/game/temp/unitdrawingengine.h"
#include "ui/graphical/game/animations/animationtimer.h"
#include "ui/graphical/game/unitselection.h"
#include "video.h"
#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/data/player/player.h"
#include "game/data/map/map.h"
#include "utility/random.h"
#include "utility/drawing.h"
#include "utility/box.h"

//--------------------------------------------------------------------------
cUnitDrawingEngine::cUnitDrawingEngine (std::shared_ptr<cAnimationTimer> animationTimer_, std::shared_ptr<const cFrameCounter> frameCounter) :
	animationTimer (std::move (animationTimer_)),
	drawingCache (frameCounter),
	blinkColor (cRgbColor::white()),
	shouldDrawHits (false),
	shouldDrawStatus (false),
	shouldDrawAmmo (false),
	shouldDrawColor (false)
{
	signalConnectionManager.connect (animationTimer->triggered100ms, std::bind (&cUnitDrawingEngine::rotateBlinkColor, this));
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::setDrawHits (bool drawHits)
{
	shouldDrawHits = drawHits;
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::setDrawStatus (bool drawStatus)
{
	shouldDrawStatus = drawStatus;
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::setDrawAmmo (bool drawAmmo)
{
	shouldDrawAmmo = drawAmmo;
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::setDrawColor (bool drawColor)
{
	shouldDrawColor = drawColor;
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::drawBuilding(const cBuilding& building, SDL_Rect destination, float zoomFactor, const cUnitSelection* unitSelection, const cPlayer* player)
{
	unsigned long long animationTime = animationTimer->getAnimationTime(); //call getAnimationTime only once in this method and save the result,
	//to avoid a changing time within this method

	SDL_Rect dest = {0,0, destination.w, destination.h};

	int cellSize = building.getCellSize();

	bool bDraw = false;
	SDL_Surface* drawingSurface = drawingCache.getCachedImage (building, zoomFactor, animationTime);
	if (drawingSurface == nullptr)
	{
		// no cached image found. building needs to be redrawn.
		bDraw = true;
		drawingSurface = drawingCache.createNewEntry (building, zoomFactor, animationTime);
	}

	if (drawingSurface == nullptr)
	{
		// image will not be cached. So blitt directly to the screen buffer.
		dest = destination;
		drawingSurface = cVideo::buffer;
	}

	if (bDraw)
	{
		cStaticUnitData::sRenderOps ops;
		ops.shadow = true;//cSettings::getInstance().isShadows();
		ops.underlay = true;
		building.render(animationTime, drawingSurface, dest, ops);
	}

	// now check, whether the image has to be blitted to screen buffer
	if (drawingSurface != cVideo::buffer)
	{
		dest = destination;
		SDL_BlitSurface (drawingSurface, nullptr, cVideo::buffer, &dest);

		// all following graphic operations are drawn directly to buffer
		dest = destination;
	}

	if (building.isRubble())
		return;

#ifdef DISABLE_THIS
	// draw the effect if necessary
	if (building.uiData->powerOnGraphic && cSettings::getInstance().isAnimations() && (building.isUnitWorking() || !building.getStaticUnitData().hasFlag(UnitFlag::CanWork)))
	{
		/*
		SDL_Rect tmp = dest;
		if(building.uiData->img_effect && building.uiData->img_effect_original)
		{
			SDL_SetSurfaceAlphaMod (building.uiData->img_effect.get(), building.effectAlpha);
			CHECK_SCALING (*building.uiData->img_effect, *building.uiData->img_effect_original, zoomFactor);
			SDL_BlitSurface (building.uiData->img_effect.get(), nullptr, cVideo::buffer, &tmp);
		}*/
		if(cSpritePtr sprite = building.uiData->effect)
		{
			SDL_Rect tmp = dest;
			sprite->render_simple(cVideo::buffer, tmp);
		}
	}
#endif
	// draw the mark, when a build order is finished
	if (building.getOwner() == player && ((!building.isBuildListEmpty() && !building.isUnitWorking() && building.getBuildListItem (0).getRemainingMetal() <= 0) ||
		(building.getStaticUnitData().hasFlag(UnitFlag::CanResearch) && building.getOwner()->isCurrentTurnResearchAreaFinished(building.getResearchArea()))))
	{
		const cRgbColor finishedMarkColor = cRgbColor::green();
		cPosition minCorner(dest.x + 2, dest.y + 2);
		cPosition maxCorner(
				dest.x + 2 + (destination.w - 3),
				dest.y + 2 + (destination.h - 3));
		const cBox<cPosition> d(minCorner, maxCorner);

		drawRectangle (*cVideo::buffer, d, finishedMarkColor.exchangeGreen (255 - 16 * (animationTime % 0x8)), 3);
	}

#if 0
	// disabled color-frame for buildings
	//   => now it's original game behavior - see ticket #542 (GER) = FIXED
	// but maybe as setting interresting
	//   => ticket #784 (ENG) (so I just commented it) = TODO

	// draw a colored frame if necessary
	if (shouldDrawColor)
	{
		const Uint32 color = 0xFF000000 | *static_cast<Uint32*> (building.owner->getColorSurface()->pixels);
		SDL_Rect d = {Sint16 (dest.x + 1), Sint16 (dest.y + 1), building.data.isBig ? 2 * destination.w - 1 : destination.w - 1, building.data.isBig ? 2 * destination.h - 1 : destination.h - 1};

		DrawRectangle (cVideo::buffer, d, color, 1);
	}
#endif
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::drawVehicle(const cVehicle& vehicle, SDL_Rect destination, float zoomFactor, const cMapView& map, const cUnitSelection* unitSelection, const cPlayer* player)
{
	unsigned long long animationTime = animationTimer->getAnimationTime(); //call getAnimationTime only once in this method and save the result,
	//to avoid a changing time within this method

	// calculate screen position
	int ox = (int) (vehicle.getMovementOffset().x() * zoomFactor);
	int oy = (int) (vehicle.getMovementOffset().y() * zoomFactor);

	destination.x += ox;
	destination.y += oy;

	if (vehicle.getFlightHeight() > 0)
	{
		destination.x += vehicle.ditherX;
		destination.y += vehicle.ditherY;
	}

	SDL_Rect dest = {0,0, destination.w, destination.h};

	bool bDraw = false;
	SDL_Surface* drawingSurface = drawingCache.getCachedImage (vehicle, zoomFactor, map, animationTime);
	if (drawingSurface == nullptr)
	{
		// no cached image found. building needs to be redrawn.
		bDraw = true;
		drawingSurface = drawingCache.createNewEntry (vehicle, zoomFactor, map, animationTime);
	}

	if (drawingSurface == nullptr)
	{
		// image will not be cached. So blitt directly to the screen buffer.
		dest = destination;
		drawingSurface = cVideo::buffer;
	}

	if (bDraw)
	{
		cStaticUnitData::sRenderOps ops;
		ops.shadow = true;//cSettings::getInstance().isShadows();
		ops.underlay = true;
		//ops.shadow = cSettings::getInstance().isShadows();
		vehicle.render (&map, animationTime, player, drawingSurface, dest, ops);
	}

	// now check, whether the image has to be blitted to screen buffer
	if (drawingSurface != cVideo::buffer)
	{
		dest = destination;
		SDL_BlitSurface (drawingSurface, nullptr, cVideo::buffer, &dest);
	}

	// remove the dithering for the following operations
	if (vehicle.getFlightHeight() > 0)
	{
		destination.x -= vehicle.ditherX;
		destination.y -= vehicle.ditherY;
	}

	// remove movement offset for working units
	if (vehicle.isUnitBuildingABuilding() || vehicle.isUnitClearing())
	{
		destination.x -= ox;
		destination.y -= oy;
	}

	// draw indication, when building is complete
	if (vehicle.isUnitBuildingABuilding() && vehicle.getBuildTurns() == 0 && vehicle.getOwner() == player && !vehicle.BuildPath)
	{
		int offset = 2;
		const cRgbColor finishedMarkColor = cRgbColor::green();
		//const cBox<cPosition> d (cPosition (destination.x + 2, destination.y + 2), cPosition (destination.x + 2 + (vehicle.getIsBig() ? 2 * destination.w - 3 : destination.w - 3), destination.y + 2 + (vehicle.getIsBig() ? 2 * destination.h - 3 : destination.h - 3)));
		cPosition minCorner(destination.x + offset, destination.y + offset);
		cPosition maxCorner(
				destination.x + destination.w - offset-1,
				destination.y + destination.h - offset-1);
		const cBox<cPosition> d(minCorner, maxCorner);

		drawRectangle (*cVideo::buffer, d, finishedMarkColor.exchangeGreen (255 - 16 * (animationTime % 0x8)), 3);
	}

	// Draw the colored frame if necessary
	if (shouldDrawColor)
	{
		int offset = 1;
		//const cBox<cPosition> d (cPosition (destination.x + 1, destination.y + 1), cPosition (destination.x + 1 + (vehicle.getIsBig() ? 2 * destination.w - 1 : destination.w - 1), destination.y + 1 + (vehicle.getIsBig() ? 2 * destination.h - 1 : destination.h - 1)));
		cPosition minCorner(destination.x + offset, destination.y + offset);
		cPosition maxCorner(
				destination.x + destination.w - 2*offset,
				destination.y + destination.h - 2*offset);
		const cBox<cPosition> d(minCorner, maxCorner);

		drawRectangle (*cVideo::buffer, d, vehicle.getOwner()->getColor().getColor());
	}

	// draw the group selected frame if necessary
	if (unitSelection && unitSelection->getSelectedUnitsCount() > 1 && unitSelection->isSelected (vehicle))
	{
		const cRgbColor groupSelectionColor = cRgbColor::yellow();
		cPosition minCorner(destination.x + 2, destination.y + 2);
		cPosition maxCorner(
				destination.x + 2 + (destination.w - 3),
				destination.y + 2 + (destination.h - 3));
		const cBox<cPosition> d (minCorner, maxCorner);

		drawRectangle (*cVideo::buffer, d, groupSelectionColor, 1);
	}
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::drawHealthBar (const cUnit& unit, SDL_Rect destination)
{
	SDL_Rect r1;
	r1.x = destination.x + destination.w / 10 + 1;
	r1.y = destination.y + destination.h / 10;
	r1.w = destination.w * 8 / 10;
	r1.h = std::max(destination.h / 8, 6);

	/*
	if (unit.getCellSize() > 1)
	{
		r1.w += destination.w*(unit.getCellSize()-1);
		r1.h *= 2;
	}*/

	if (r1.h <= 2)
		r1.h = 3;

	SDL_Rect r2;
	r2.x = r1.x + 1;
	r2.y = r1.y + 1;
	r2.w = (int) (((float) (r1.w - 2) / unit.data.getHitpointsMax()) * unit.data.getHitpoints());
	r2.h = r1.h - 2;

	SDL_FillRect (cVideo::buffer, &r1, 0xFF000000);

	Uint32 color;
	if (unit.data.getHitpoints() > unit.data.getHitpointsMax() / 2)
		color = 0xFF04AE04; // green
	else if (unit.data.getHitpoints() > unit.data.getHitpointsMax() / 4)
		color = 0xFFDBDE00; // orange
	else
		color = 0xFFE60000; // red
	SDL_FillRect (cVideo::buffer, &r2, color);
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::drawMunBar (const cUnit& unit, SDL_Rect destination)
{
	SDL_Rect r1;
	r1.x = destination.x + destination.w / 10 + 1;
	r1.y = destination.y + destination.h / 10 + destination.h / 8;
	r1.w = destination.w * 8 / 10;
	r1.h = destination.h / 8;

	if (r1.h <= 2)
	{
		r1.y += 1;
		r1.h = 3;
	}

	SDL_Rect r2;
	r2.x = r1.x + 1;
	r2.y = r1.y + 1;
	r2.w = (int) (((float) (r1.w - 2) / unit.data.getAmmoMax()) * unit.data.getAmmo());
	r2.h = r1.h - 2;

	SDL_FillRect (cVideo::buffer, &r1, 0xFF000000);

	if (unit.data.getAmmo() > unit.data.getAmmoMax() / 2)
		SDL_FillRect (cVideo::buffer, &r2, 0xFF04AE04);
	else if (unit.data.getAmmo() > unit.data.getAmmoMax() / 4)
		SDL_FillRect (cVideo::buffer, &r2, 0xFFDBDE00);
	else
		SDL_FillRect (cVideo::buffer, &r2, 0xFFE60000);
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::drawStatus (const cUnit& unit, SDL_Rect destination)
{
	SDL_Rect speedSymbol = {244, 97, 8, 10};
	SDL_Rect shotsSymbol = {254, 97, 5, 10};
	SDL_Rect disabledSymbol = {150, 109, 25, 25};
	SDL_Rect dest;

	if (unit.isDisabled())
	{
		if (destination.w < 25)
			return;
		dest.x = destination.x + destination.w / 2 - 12;
		dest.y = destination.y + destination.h / 2 - 12;
		if (unit.getCellSize() > 1)
		{
			dest.y += (destination.h / 2);
			dest.x += (destination.w / 2);
		}
		SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get(), &disabledSymbol, cVideo::buffer, &dest);
	}
	else
	{
		dest.y = destination.y + destination.h - 11;
		dest.x = destination.x + destination.w / 2 - 4;
		if (unit.getCellSize() > 1)
		{
			dest.y += (destination.h / 2);
			dest.x += (destination.w / 2);
		}
		if (unit.data.getSpeed() >= 4)
		{
			if (unit.data.getShots())
				dest.x -= destination.w / 4;

			SDL_Rect destCopy = dest;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get(), &speedSymbol, cVideo::buffer, &destCopy);
		}

		dest.x = destination.x + destination.w / 2 - 4;
		if (unit.data.getShots())
		{
			if (unit.data.getSpeed())
				dest.x += destination.w / 4;
			SDL_BlitSurface (GraphicsData.gfx_hud_stuff.get(), &shotsSymbol, cVideo::buffer, &dest);
		}
	}
}

//--------------------------------------------------------------------------
void cUnitDrawingEngine::rotateBlinkColor()
{
	static bool dec = true;
	if (dec)
	{
		blinkColor.r -= 0x0A;
		blinkColor.g -= 0x0A;
		blinkColor.b -= 0x0A;
		if (blinkColor.r <= 0xA0) dec = false;
	}
	else
	{
		blinkColor.r += 0x0A;
		blinkColor.g += 0x0A;
		blinkColor.b += 0x0A;
		if (blinkColor.r >= (0xFF - 0x0A)) dec = true;
	}
}
