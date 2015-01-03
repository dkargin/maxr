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

#ifndef input_keyboard_keycombination_H
#define input_keyboard_keycombination_H

#include <string>

#include <SDL.h>

#include "input/keyboard/keymodifiertype.h"

class cKeyCombination
{
public:
	explicit cKeyCombination (const std::string& sequence);
	cKeyCombination (KeyModifierFlags modifiers, SDL_Keycode key);

	std::string toString() const;

	bool operator== (const cKeyCombination& other) const;
	bool operator!= (const cKeyCombination& other) const;

	static bool isRepresentableKey (SDL_Keycode key);
private:
	KeyModifierFlags modifiers;
	SDL_Keycode key;

	void addKey (const std::string& sequence);
};

#endif // input_keyboard_keycombination_H
