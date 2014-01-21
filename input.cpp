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
#include "input.h"
#include "menus.h"

sMouseState::sMouseState()
{
	leftButtonPressed = false;
	rightButtonPressed = false;
	leftButtonReleased = false;
	rightButtonReleased = false;
	wheelUp = false;
	wheelDown = false;
	isDoubleClick = false;
	x = 0;
	y = 0;
}

cInput::cInput() :
	LastClickTicks (0)
{
#if 0 // TODO: [SDL2]: setting for repeat key ?
	// enables that SDL puts the unicode values to the keyevents.
	SDL_EnableUNICODE (1);
	// enables keyrepetition
	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif
}

void cInput::inputkey (cMenu& activeMenu, const SDL_KeyboardEvent& key)
{
	// give the key to the active menu
	// But do not send events to a menu,
	// after an event triggered the termination
	// the user wouldn't expects the menu to execute further events
	// after clicking the exit button
	if (!activeMenu.exiting())
	{
		activeMenu.handleKeyInput (key);
	}
}

bool cInput::IsDoubleClicked()
{
	/* First time this function is called, LastClickTicks
	    has not been initialised yet. */
	if (! LastClickTicks)
	{
		LastClickTicks = SDL_GetTicks();
		return (false);
	}
	else
	{
		const long CurrentClickTicks = SDL_GetTicks();

		/* If the period between the two clicks is smaller
		    or equal to a pre-defined number, we report a
		    DoubleClick event. */

		if (CurrentClickTicks - LastClickTicks <= 500)
		{
			/* Update LastClickTicks and signal a DoubleClick. */

			LastClickTicks = CurrentClickTicks;
			return (true);
		}

		/* Update LastClickTicks and signal a SingleClick. */

		LastClickTicks = CurrentClickTicks;
		return (false);
	}
}

void cInput::inputMouseButton (cMenu& activeMenu, const SDL_MouseButtonEvent& button)
{
	MouseState.x = button.x;
	MouseState.y = button.y;

	if (button.state == SDL_PRESSED)
	{
		if (button.button == SDL_BUTTON_LEFT)
		{
			MouseState.leftButtonPressed = true;
			MouseState.leftButtonReleased = false;
			MouseState.rightButtonReleased = false;
		}
		else if (button.button == SDL_BUTTON_RIGHT)
		{
			MouseState.rightButtonPressed = true;
			MouseState.rightButtonReleased = false;
			MouseState.leftButtonReleased = false;
		}
		MouseState.isDoubleClick = IsDoubleClicked();
	}
	else if (button.state == SDL_RELEASED)
	{
		if (button.button == SDL_BUTTON_LEFT)
		{
			MouseState.leftButtonPressed = false;
			MouseState.leftButtonReleased = true;
			MouseState.rightButtonReleased = false;
		}
		else if (button.button == SDL_BUTTON_RIGHT)
		{
			MouseState.rightButtonPressed = false;
			MouseState.rightButtonReleased = true;
			MouseState.leftButtonReleased = false;
		}
	}

	// do not send events to a menu, after an event triggered the termination
	// the user wouldn't expects the menu to execute further events
	// after clicking the exit button
	if (!activeMenu.exiting())
	{
		activeMenu.handleMouseInput (MouseState);
	}
}

void cInput::inputMouseButton (cMenu& activeMenu, const SDL_MouseWheelEvent& wheel)
{
	MouseState.wheelUp = false;
	MouseState.wheelDown = false;
	if (wheel.y > 0)
	{
		MouseState.wheelUp = true;
		MouseState.leftButtonReleased = false;
		MouseState.rightButtonReleased = false;
	}
	else if (wheel.y < 0)
	{
		MouseState.wheelDown = true;
		MouseState.leftButtonReleased = false;
		MouseState.rightButtonReleased = false;
	}
	// do not send events to a menu, after an event triggered the termination
	// the user wouldn't expects the menu to execute further events
	// after clicking the exit button
	if (!activeMenu.exiting())
	{
		activeMenu.handleMouseInput (MouseState);
	}
}

std::string cInput::getUTF16Char (Uint16 ch)
{
	// convert from UTF-16 to UTF-8
	int count = 1;
	if (ch >= 0x80) count++;

	uint32_t bitmask = 0x0800;
	for (int i = 0; i < 5; i++)
	{
		if (static_cast<uint32_t> (ch) >= bitmask) count++;
		bitmask <<= 5;
	}

	std::string returnStr = "";
	if (count == 1)
	{
		returnStr += static_cast<char> (ch);
	}
	else
	{
		for (int i = count - 1; i >= 0; i--)
		{
			unsigned char c = (ch >> (6 * i)) & 0x3f;
			c |= 0x80;
			if (i == count - 1) c |= 0xff << (8 - count);
			returnStr += c;
		}
	}
	return returnStr;
}
