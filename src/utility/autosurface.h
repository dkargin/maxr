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

#ifndef utility_autosurfaceH
#define utility_autosurfaceH

#include <memory>
#include <SDL.h>

#include "maxrconfig.h"

namespace detail
{

struct SdlSurfaceDeleter
{
	void operator() (SDL_Surface* surface)
	{
		SDL_FreeSurface (surface);
	}
};

}

typedef std::unique_ptr<SDL_Surface, detail::SdlSurfaceDeleter> AutoSurface;

/* Prevent accidentally freeing the SDL_Surface owned by an AutoSurface */
void SDL_FreeSurface (const AutoSurface&) MAXR_DELETE_FUNCTION;

#endif // utility_autosurfaceH
