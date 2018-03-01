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

#include "utility/drawing.h"
#include "utility/color.h"
#include "utility/position.h"
#include "utility/box.h"
#include "utility/autosurface.h"
#include "utility/files.h"
#include "utility/pcx.h"

#include <SDL_image.h>

//------------------------------------------------------------------------------
void drawPoint (SDL_Surface* surface, const cPosition& position, const cRgbColor& color)
{
	SDL_Rect rect = {Sint16 (position.x()), Sint16 (position.y()), 1, 1};
	SDL_FillRect (surface, &rect, color.toMappedSdlRGBAColor (surface->format));
}

//------------------------------------------------------------------------------
void drawLine (SDL_Surface* s, const cPosition& start, const cPosition& end, const cRgbColor& color)
{
	auto x0 = start.x();
	auto x1 = end.x();
	auto y0 = start.y();
	auto y1 = end.y();

	bool steep = abs (y1 - y0) > abs (x1 - x0);
	if (steep)
	{
		std::swap (x0, y0);
		std::swap (x1, y1);
	}
	if (x0 > x1)
	{
		std::swap (x0, x1);
		std::swap (y0, y1);
	}

	int dx = x1 - x0;
	int dy = abs (y1 - y0);
	int er = dx / 2;
	int ys = y0 < y1 ? 1 : -1;
	int y = y0;

	for (int x = x0; x < x1; x++)
	{
		if (steep) drawPoint (s, cPosition (y, x), color);
		else drawPoint (s, cPosition (x, y), color);
		er -= dy;
		if (er < 0)
		{
			y += ys;
			er += dx;
		}
	}
}

//------------------------------------------------------------------------------
void drawRectangle (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int thickness)
{
	const cPosition size = rectangle.getMaxCorner() - rectangle.getMinCorner();

	SDL_Rect line_h = {rectangle.getMinCorner().x(), rectangle.getMinCorner().y(), size.x(), thickness};

	const auto sdlColor = color.toMappedSdlRGBAColor (surface.format);

	SDL_FillRect (&surface, &line_h, sdlColor);
	line_h.y += size.y() - thickness;
	SDL_FillRect (&surface, &line_h, sdlColor);
	SDL_Rect line_v = {rectangle.getMinCorner().x(), rectangle.getMinCorner().y(), thickness, size.y()};
	SDL_FillRect (&surface, &line_v, sdlColor);
	line_v.x += size.x() - thickness;
	SDL_FillRect (&surface, &line_v, sdlColor);
}

//------------------------------------------------------------------------------
void drawSelectionCorner (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int cornerSize)
{
	const cPosition size = rectangle.getMaxCorner() - rectangle.getMinCorner();

	SDL_Rect line_h = {rectangle.getMinCorner().x(), rectangle.getMinCorner().y(), cornerSize, 1};

	const auto sdlColor = color.toMappedSdlRGBAColor (surface.format);

	SDL_FillRect (&surface, &line_h, sdlColor);
	line_h.x += size.x() - cornerSize;
	SDL_FillRect (&surface, &line_h, sdlColor);
	line_h.x = rectangle.getMinCorner().x();
	line_h.y += size.y() - 1;
	SDL_FillRect (&surface, &line_h, sdlColor);
	line_h.x += size.x() - cornerSize;
	SDL_FillRect (&surface, &line_h, sdlColor);

	SDL_Rect line_v = {rectangle.getMinCorner().x(), rectangle.getMinCorner().y(), 1, cornerSize};
	SDL_FillRect (&surface, &line_v, sdlColor);
	line_v.y += size.y() - cornerSize;
	SDL_FillRect (&surface, &line_v, sdlColor);
	line_v.x += size.x() - 1;
	line_v.y = rectangle.getMinCorner().y();
	SDL_FillRect (&surface, &line_v, sdlColor);
	line_v.y += size.y() - cornerSize;
	SDL_FillRect (&surface, &line_v, sdlColor);
}

//------------------------------------------------------------------------------
void drawSelectionCorner (SDL_Surface& surface, const cBox<cPosition>& rectangle, const cRgbColor& color, int cornerSize, const cBox<cPosition>& clipRect)
{
	if (!rectangle.intersects (clipRect)) return;

	const cPosition size = rectangle.getSize();
	AutoSurface tempSurface (SDL_CreateRGBSurface (0, size.x(), size.y(), 32, 0, 0, 0, 0));
	SDL_FillRect (tempSurface.get(), nullptr, 0xFF00FF);
	SDL_SetColorKey (tempSurface.get(), SDL_TRUE, 0xFF00FF);

	auto rectangle2 = cBox<cPosition> (cPosition (0, 0), rectangle.getMaxCorner() - rectangle.getMinCorner());
	drawSelectionCorner (*tempSurface, rectangle2, color, cornerSize);

	blitClipped (*tempSurface, rectangle, surface, clipRect);
}

//------------------------------------------------------------------------------
Uint32 getPixel (const SDL_Surface& surface, const cPosition& position)
{
	int bpp = surface.format->BytesPerPixel;

	Uint8* p = (Uint8*)surface.pixels + position.y() * surface.pitch + position.x() * bpp;

	switch (bpp)
	{
		case 1:
			return *p;
			break;

		case 2:
			return * (Uint16*)p;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
			break;

		case 4:
			return * (Uint32*)p;
			break;

		default:
			return 0;
	}
}

//------------------------------------------------------------------------------
void putPixel (SDL_Surface& surface, const cPosition& position, Uint32 pixel)
{
	int bpp = surface.format->BytesPerPixel;
	Uint8* p = (Uint8*)surface.pixels + position.y() * surface.pitch + position.x() * bpp;

	switch (bpp)
	{
		case 1:
			*p = pixel;
			break;

		case 2:
			* (Uint16*)p = pixel;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}
			else
			{
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			* (Uint32*)p = pixel;
			break;
	}
}

//------------------------------------------------------------------------------
void replaceColor (SDL_Surface& surface, const cRgbColor& sourceColor, const cRgbColor& destinationColor)
{
	const auto srcMapped = sourceColor.toMappedSdlRGBAColor (surface.format);
	const auto destMapped = destinationColor.toMappedSdlRGBAColor (surface.format);

	Uint32 key;
	const auto hadKey = SDL_GetColorKey (&surface, &key) == 0;

	AutoSurface temp (SDL_ConvertSurface (&surface, surface.format, surface.flags));

	SDL_SetColorKey (temp.get(), SDL_TRUE, srcMapped);
	SDL_FillRect (&surface, nullptr, destMapped);
	SDL_BlitSurface (temp.get(), nullptr, &surface, nullptr);

	if (hadKey) SDL_SetColorKey (&surface, SDL_TRUE, key);
	else SDL_SetColorKey (&surface, SDL_FALSE, 0);

	// The following version is to slow...
	//
	//SDL_LockSurface (&surface);
	//for (int y = 0; y < surface.h; y++)
	//{
	//	for (int x = 0; x < surface.w; x++)
	//	{
	//		const cPosition position (x, y);
	//		if (getPixel (surface, position) == srcMapped)
	//		{
	//			putPixel (surface, position, destMapped);
	//		}
	//	}
	//}
	//SDL_UnlockSurface (&surface);
}

void blitClipped (SDL_Surface& source, const cBox<cPosition>& area, SDL_Surface& destination, const cBox<cPosition>& clipRect)
{
	auto clipedArea = area.intersection (clipRect);

	SDL_Rect position = clipedArea.toSdlRect();

	clipedArea.getMinCorner() -= area.getMinCorner();
	clipedArea.getMaxCorner() -= area.getMinCorner();

	SDL_Rect sourceRect = clipedArea.toSdlRect();

	SDL_BlitSurface (&source, &sourceRect, &destination, &position);
}

void drawCheckerPattern(SDL_Surface* surface, int cellSize, int colorA, int colorB)
{
	SDL_Rect dst_rect = {0, 0, surface->w, surface->h};
	SDL_FillRect(surface, nullptr, colorA);

	for(int y = 0; y < ceil(dst_rect.h / (float)cellSize); y++)
		for(int x = 0; x < ceil(dst_rect.w / (float)cellSize); x++)
		{
			if(x&1 ^ y&1)
			{
				SDL_Rect cell_rect{dst_rect.x + x*cellSize, dst_rect.y + y*cellSize, cellSize, cellSize};
				SDL_FillRect(surface, &cell_rect, colorB);
			}
		}
}

//------------------------------------------------------------------------------
void cRenderable::setSize(const cVector2& newSize)
{
	this->size = newSize;
}

//------------------------------------------------------------------------------
cBox<cVector2> cRenderable::getRect() const
{
	return cBox<cVector2>(pos, pos + size);
}

//------------------------------------------------------------------------------
void cRenderable::render_simple(SDL_Surface* surf, SDL_Rect rect)
{
	sContext context;
	context.surface = surf;
	context.dstRect = rect;
	context.cache = true;
	context.layer = 0;
	render(context);
}

void cRenderable::setColorKey(int key)
{

}

void cRenderable::setAlphaKey(int alpha)
{

}

void cRenderable::setChannel(const char* channel)
{
	this->channel = channel;
}

void cRenderable::setChannel(const std::string& channel)
{
	this->channel = channel;
}

const char* cRenderable::getChannel() const
{
	return channel.c_str();
}

//------------------------------------------------------------------------------
cSprite::cSprite(SurfacePtr surf, SDL_Rect rect)
{
	surface = surf;
	srcRect = rect;
}

//------------------------------------------------------------------------------
cSprite::operator SDL_Surface*()
{
	return cache.get();
}

void cSprite::render(cRenderable::sContext& context)
{
	SDL_Surface* dst = context.surface;
	SDL_Rect dst_rect = context.dstRect;

	if(!this->surface)
		return;

	SDL_Surface* src = this->surface.get();

	bool refresh = false;
	// Updating the cache to fit an upscaled image
	if(!cache || cache->w < dst_rect.w || cache->h < dst_rect.h)
	{
		int depth = surface->format->BitsPerPixel;
		cache = AutoSurface(SDL_CreateRGBSurface (0, dst_rect.w, dst_rect.h, depth, 0, 0, 0, 0));
		refresh = true;
	}

	if(lastSize.x() != dst_rect.w || lastSize.y() != dst_rect.h)
		refresh = true;

	if(refresh)
	{
		SDL_Rect rc{0,0, dst_rect.w, dst_rect.h};
		SDL_FillRect(cache.get(), &rc, colorkey);
		SDL_BlitScaled(src, &srcRect, cache.get(), &rc);
		lastSize = cPosition(dst_rect.w, dst_rect.h);
		applyBlending(cache.get());
	}

	applyBlending(dst);

	SDL_Rect rc{0,0, dst_rect.w, dst_rect.h};
	// Draw internal cache to the destination surface
	SDL_BlitSurface(cache.get(), &rc, dst, &dst_rect);
}

void cSprite::applyBlending(SDL_Surface* surface) const
{
	// Fix color settings
	if(alpha >= 0)
		SDL_SetSurfaceAlphaMod(surface, alpha);

	if(colorkey >= 0)
		SDL_SetColorKey(surface, SDL_TRUE, colorkey);
	else
		SDL_SetColorKey(surface, SDL_FALSE, 0);
}

void cSprite::setColorKey(int key)
{
	if(key == this->colorkey)
		return;

	colorkey = key;

	if(surface)
	{
		if(colorkey >= 0)
			SDL_SetColorKey(surface.get(), SDL_TRUE, colorkey);
		else
			SDL_SetColorKey(surface.get(), SDL_FALSE, 0);
	}
}

void cSprite::setColorKeyAuto()
{
	if(!surface)
		return;

	//if(SDL_LockSurface(surface.get()))
	{
		auto color = getPixel(*surface.get(), cPosition(0,0));
		//SDL_UnlockSurface(surface.get());
		setColorKey(color);
	}
}

void cSprite::setAlphaKey(int key)
{
	if(key == this->alpha)
		return;

	alpha = key;

	if(surface)
	{
		if(alpha >= 0)
			SDL_SetSurfaceAlphaMod(surface.get(), alpha);
		else
			SDL_SetSurfaceAlphaMod(surface.get(), 255);
	}
}

const SDL_PixelFormat* cSprite::getFormat()
{
	return surface ? nullptr : surface->format;
}

//------------------------------------------------------------------------------
cSpriteList::cSpriteList(SurfacePtr surf, SDL_Rect rect)
	:cSprite(surf, rect)
{
}

void cSpriteList::render(sContext& context)
{
	SDL_Surface* dst = context.surface;
	SDL_Rect dst_rect = context.dstRect;

	if(!this->surface)
		return;

	SDL_Surface* src = this->surface.get();

	bool refresh = false;

	// Updating the cache to fit an upscaled image
	int cacheWidth = dst_rect.w*frames;
	if(!cache || cache->w < cacheWidth || cache->h < dst_rect.h)
	{
		int depth = surface->format->BitsPerPixel;
		cache = AutoSurface(SDL_CreateRGBSurface (0, cacheWidth, dst_rect.h, depth, 0, 0, 0, 0));
		refresh = true;
	}

	if(lastSize.x() != cacheWidth || lastSize.y() != dst_rect.h)
		refresh = true;

	// Need to refill local cache
	// We upscale original frames to desired size.
	// By the way, desired size could be lesser than real cache size.
	if(refresh)
	{
		SDL_Rect rc{0,0, cacheWidth, dst_rect.h};
		SDL_FillRect(cache.get(), &rc, colorkey);
		SDL_BlitScaled(src, &srcRect, cache.get(), &rc);
		// Should update last size
		lastSize = cPosition(dst_rect.w, dst_rect.h);

		applyBlending(cache.get());

		if(!file.empty())
		{
			IMG_SavePNG(cache.get(), (file + ".cache.png").c_str());
		}
	}

	// Fix color settings
	int frame = 0;
	if(context.channels.count(channel))
		frame = context.channels[channel];

	if (frame < 0)
		frame = 0;

	if (frame > frames)
		frame = frame % frames;

	SDL_Rect rc = this->getSrcRect(frame);
	// Draw to the destination surface
	SDL_BlitSurface(cache.get(), &rc, dst, &dst_rect);
}


SDL_Rect cSpriteList::getSrcRect(int frame)
{
	SDL_Rect result;
	result.x = lastSize.x() * frame;
	result.y = 0;
	result.w = lastSize.x();
	result.h = lastSize.y();
	return result;
}

//------------------------------------------------------------------------------
cSpriteTool::cSpriteTool()
{
	cellSize = 64;

}

void cSpriteTool::setCellSize(int size)
{
	cellSize = size;
}

void cSpriteTool::setPixelFormat(const SDL_PixelFormat& format)
{
	memcpy(&this->format, &format, sizeof(SDL_PixelFormat));
}

void cSpriteTool::setRefSize(const cPosition& size)
{
	this->hasRefSize = true;
	refSize = size;
}

void cSpriteTool::resetRefSize()
{
	hasRefSize = false;
}

void cSpriteTool::reset()
{
	this->hasLayer = false;
	this->hasRefSize = false;
}


// @path - path to an image
// @size - size of the sprite in world coordinates
cSpritePtr cSpriteTool::makeSprite(const std::string& path, const cVector2& size, FitMode mode)
{
	if (!FileExists (path.c_str()))
		return nullptr;

	auto surface = LoadPCX(path);
	if(!surface)
		return nullptr;

	SDL_Rect rect = SDL_Rect{0, 0, surface->w, surface->h};

	cSprite* sprite = new cSprite(SurfacePtr(surface.release(), detail::SdlSurfaceDeleter()), rect);

	sprite->setSize(size);
	sprite->mode = mode;
	sprite->file = path;

	return cSpritePtr(sprite);
}

cSpriteListPtr cSpriteTool::makeVariantSprite(const std::string& path, int frames, const cVector2& size, FitMode mode)
{
	if (!FileExists (path.c_str()))
		return nullptr;

	auto surface = LoadPCX(path);
	if(!surface)
		return nullptr;

	if(frames < 1)
		frames = 1;

	SDL_Rect rect = SDL_Rect{0, 0, surface->w, surface->h};
	cSpriteList* sprite = new cSpriteList(SurfacePtr(surface.release(), detail::SdlSurfaceDeleter()), rect);
	sprite->frames = frames;
	sprite->file = path;

	return cSpriteListPtr(sprite);
}

cSpriteListPtr cSpriteTool::makeVariantSprite(const std::list<std::string>& paths, const cVector2& size, FitMode mode)
{
	if(paths.empty())
		return nullptr;

	std::vector<AutoSurface> src;
	for(auto path: paths)
	{
		auto surface = LoadPCX(path);
		if(!surface)
		{
			printf("cSpriteTool::makeVariantSprite - can not load surface from %s\n", path.c_str());
		}
		src.push_back(std::move(surface));
	}

	if(!src.empty())
	{
		int w = src[0]->w;
		int h=  src[0]->h;
		int depth = src[0]->format->BitsPerPixel;
		SDL_Rect rect = {0, 0, int(w * src.size()), h};
		SDL_Surface* surface = SDL_CreateRGBSurface (0, rect.w, rect.h, depth, 0, 0, 0, 0);

		// Put frames to internal surface
		for(int i = 0; i < src.size(); i++)
		{
			auto surf = src[i].get();
			SDL_Rect dst_rect = {i*w, 0, w, h};
			SDL_Rect src_rect = {0, 0, surf->w, surf->h};
			SDL_BlitSurface(surf, &src_rect, surface, &dst_rect);
		}

		cSpriteList* sprite = new cSpriteList(SurfacePtr(surface, detail::SdlSurfaceDeleter()), rect);
		sprite->frames = src.size();
		sprite->mode = mode;
		sprite->file = paths.front();
		return cSpriteListPtr(sprite);
	}

	return nullptr;
}
