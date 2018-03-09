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

#include <list>
#include <string>

#include <SDL.h>
#include "position.h"
#include "box.h"
#include "autosurface.h"


class cRgbColor;
class cPosition;
template<typename> class cBox;

// Raw ARGB color
typedef int32_t ColorRaw;

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

/**
 * A set of values, that necessary to pick proper graphics from all the objects
 */
class cRenderContext
{
public:
	cRenderContext();
	cRenderContext(SDL_Surface* surface, const SDL_Rect& rect);
	SDL_Renderer* renderer = nullptr;
	SDL_Surface* surface = nullptr;
	SDL_Rect dstRect;
	// Channel values. Here are clan variations or animated frames
	std::map<std::string, int> channels;
	// Selected layer
	int layer = -1;

	// Should we use internal cache
	bool cache = true;

	// Option to override colorkey from the sprite
	bool overrideColorkey = false;
	ColorRaw colorkey = -1;

	int alpha = -1;

	void setTarget(SDL_Surface* surface, const SDL_Rect& rect);
};
/**
 * Base class for renderable object
 */
class cRenderable : public std::enable_shared_from_this<cRenderable>
{
public:
	using sContext = cRenderContext;

	virtual ~cRenderable() {}
	virtual cBox<cVector2> getRect() const; 	// Get bounding rectangle

	// Runs actual rendering process
	virtual void render(sContext& context) const = 0;

	// Set logical size
	void setSize(const cVector2& newSize);
	// Set channel
	void setChannel(const char* channel);
	void setChannel(const std::string& channel);
	const char* getChannel() const;

	virtual void setColorKey(int key);
	virtual void setAlphaKey(int alpha = -1);
protected:
	// Some helper ID. IDs are always usefull
	int id;
	// Z coordinate for additional z-ordering
	float z = 0.0;
	// Offset and desired size. In 'world' tile coordinates. That's static data from XML
	cVector2 pos;
	// Desired size. In 'world' tile coordinates. That's static data from XML
	cVector2 size;

	int layer = -1;

	std::string channel;
};

class cSprite;
class cSpriteList;
class cSpriteTool;

typedef std::shared_ptr<cRenderable> cRenderablePtr;
typedef std::shared_ptr<cSprite> cSpritePtr;
typedef std::shared_ptr<cSpriteList> cSpriteListPtr;

enum class FitMode
{
	Center,              //< Place source bitmap at the center of 'offset' rect
	Scale,               //< Scale source bitmap to fit 'rect'
	Tile,                //< Repeat source bitbap to fill in all the area
};

// Wrapper for drawing sprites from world coordinates
class cSprite: public cRenderable
{
	friend class cSpriteTool;
public:
	cSprite(SurfacePtr surf, SDL_Rect srcRect);

	operator SDL_Surface*();

	const SDL_PixelFormat* getFormat();

	// Set colorkey
	void setColorKey(int key);
	// Set colorkey based on color of left topmost pixel
	void setColorKeyAuto();
	// Set alpha blending value
	void setAlphaKey(int alpha = -1);

	// Blit this sprite to output surface
	// Will do rescaling, if necessary
	virtual void render(sContext& context) const override;
	// Applies blending settings to specified surface
	void applyBlending(SDL_Surface* surface) const;
protected:
	// Cached surface. We update this cache every time
	mutable SurfaceUPtr cache;
	// Last used scale
	mutable cPosition lastSize;

	int colorkey = -1;
	int alpha = -1;
	// Shared pointer to SDL surface
	SurfacePtr surface;
	std::shared_ptr<SDL_Texture> texture;
	// Area of source surface. We copy data from that area
	SDL_Rect srcRect;
	// Rendering mode. That's static data from XML
	FitMode mode;
	// Filename for source bitmap
	std::string file;
};

/**
 * Similar to cSprite, but has a a sort of 'variation'
 */
class cSpriteList : public cSprite
{
	friend class cSpriteTool;
public:
	cSpriteList(SurfacePtr surf, SDL_Rect srcRect);

	// Blit this sprite to output surface
	// Will do rescaling, if necessary
	//void blit_and_cache(SDL_Surface* surface, SDL_Rect rect) override;
	virtual void render(sContext& context) const override;

	SDL_Rect getSrcRect(int frame) const;
protected:
	// Number of frames stored
	int frames;
};

// A factory for renderable objects
class cSpriteTool
{
public:
	cSpriteTool();

	// Set reference size
	// It is 'intended' unit size in pixels
	void setRefSize(const cPosition& size);
	void resetRefSize();

	// Set pixel format for all created objects
	void setPixelFormat(const SDL_PixelFormat& format);

	// Set cell size
	void setCellSize(int size);

	// Set default layer for creates sprites
	void setLayer(int layer);

	// Maps RGB to raw color, using default pixel format
	ColorRaw mapRGB(int r, int g, int b);
	ColorRaw mapRGBA(int r, int g, int b, int a);
	// @path - path to an image
	// @size - size of the sprite in world coordinates
	cSpritePtr makeSprite(const std::string& path, const cVector2& size = cVector2(1,1), FitMode mode = FitMode::Scale);

	// Creates a sprite object, that has several variants
	// @paths - a list of paths to an images
	// @size - size of the sprite in world coordinates
	cSpriteListPtr makeSpriteList(const std::list<std::string>& paths, const cVector2& size = cVector2(1,1), FitMode mode = FitMode::Scale);

	// Creates a sprite object, that has several variants
	// @path - path to an image
	// @size - size of the sprite in world coordinates
	cSpriteListPtr makeSpriteSheet(const std::string& path, int variants, const cVector2& size = cVector2(1,1), FitMode mode = FitMode::Scale);

	static void saveImage(SDL_Surface* surf, const SDL_Rect* rect, const std::string& path);
	// Reset current flags (except pixel format)
	void reset();
protected:
	bool hasRefSize = false;
	cPosition refSize;

	bool hasLayer;
	int layer = -1;

	// Reference cell size. Is it deprecated?
	int cellSize;

	// Pixel format used for surfaces
	SDL_PixelFormat format;

	static AutoSurface cache;
};

// Renderable group
class cRenderableGroup
{
public:

	std::list<cRenderablePtr> children;
};

#endif // utility_drawingH
