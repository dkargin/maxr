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

#ifndef input_mouse_cursor_mousecursoramountH
#define input_mouse_cursor_mousecursoramountH

#include "input/mouse/cursor/mousecursor.h"
#include "maxrconfig.h"
#include "utility/autosurface.h"

enum eMouseCursorAmountType
{
	Steal,
	Disable
};

class cMouseCursorAmount : public cMouseCursor
{
public:
	cMouseCursorAmount (eMouseCursorAmountType type_);
	cMouseCursorAmount (eMouseCursorAmountType type_, int percent_);

	virtual SDL_Surface* getSurface() const MAXR_OVERRIDE_FUNCTION;

	virtual cPosition getHotPoint() const MAXR_OVERRIDE_FUNCTION;

protected:
	virtual bool equal (const cMouseCursor& other) const MAXR_OVERRIDE_FUNCTION;

private:
	eMouseCursorAmountType type;
	int percent;

	mutable AutoSurface surface;

	void generateSurface() const;

	SDL_Surface* getSourceSurface() const;
};

#endif // input_mouse_cursor_mousecursoramountH
