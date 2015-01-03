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

#ifndef input_mouse_cursor_mousecursorH
#define input_mouse_cursor_mousecursorH

struct SDL_Surface;
class cPosition;

class cMouseCursor
{
public:
	virtual ~cMouseCursor() {}

	virtual SDL_Surface* getSurface() const = 0;
	virtual cPosition getHotPoint() const = 0;

	bool operator== (const cMouseCursor& other) const
	{
		return equal (other);
	}
protected:
	virtual bool equal (const cMouseCursor& other) const = 0;
};

#endif // input_mouse_cursor_mousecursorH
