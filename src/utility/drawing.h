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

#ifndef utility_drawingH
#define utility_drawingH

#include <SDL.h>
#include "position.h"
#include "box.h"

class cRgbColor;
class cPosition;
template<typename> class cBox;

void drawPoint (SDL_Surface* surface, const cPosition& position, const cRgbColor& color);

void drawLine (SDL_Surface* surface, const cPosition& start, const cPosition& end, const cRgbColor& color);

void drawRectangle (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int thickness = 1);

void drawSelectionCorner (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int cornerSize);

void drawSelectionCorner (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int cornerSize, const cBox<cPosition>& clipRect);

Uint32 getPixel (const SDL_Surface& surface, const cPosition& position);

void putPixel (SDL_Surface& surface, const cPosition& position, Uint32 pixel);

void replaceColor (SDL_Surface& surface, const cRgbColor& sourceColor, const cRgbColor& destinationColor);

/**
 * Blits a surface onto an other one at a given destination area.
 *
 * Additionally the destination area where the surface pixels will be changed will be clipped by an additional input area.
 *
 * @param source The surface to blit onto the the destination surface.
 * @param area The area in the destination where the source should be blit to.
 * @param destination The surface on which the source should be blit onto.
 * @param clipRect The clipping area.
 */
void blitClipped (SDL_Surface& source, const cBox<cPosition>& area, SDL_Surface& destination, const cBox<cPosition>& clipRect);

// Wrapper for drawing sprites from world coordinates
struct sSprite
{
    int id;                     //< Some helper ID.
    SDL_Surface* surface;       //< Pointer to SDL surface
    SDL_Rect srcRect;           //< Area of source surface
    cBox<cVector2> rect;        //< Offset and desired size. In 'world' tile coordinates. That's static data from

    enum FitMode
    {
        FitCenter,              //< Place source bitmap at the center of 'offset' rect
        FitScale,               //< Scale source bitmap to fit 'rect'
        FitTile,                //< Repeat source bitbap to fill in all the area
    }mode;                      //< Rendering mode. That's static data from XML
    cVector2 position;          //< World position of this sprite
    float z;                    //< Z coordinate for additional z-ordering
};

// Sprite that was projected to screen coordinates
struct sProjectedSprite
{
    SDL_Surface* surface;
    SDL_Rect srcRect;
    SDL_Rect dstRect;
    float z;
};

/**
 *
 * @param source sprite
 * @param destination - destination surface
 * @param dstRect - rectangle from destination surface. All rendering should be clipped to this rectangle
 * @param viewArea - area of world, that is projected to destination surface
 */
sProjectedSprite projectSprite(const sSprite& sprite, SDL_Surface* destination, SDL_Rect dstRect, cBox<cVector2> viewArea);

#endif // utility_drawingH
