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

#include "input/mouse/mouse.h"
#include "input/mouse/cursor/mousecursorsimple.h"

#include "main.h"
#include "events/eventmanager.h"
#include "events/mouseevents.h"

//------------------------------------------------------------------------------
cMouse::cMouse() :
	sdlCursor (nullptr, SDL_FreeCursor),
	doubleClickTime (500)
{
	setCursor (std::make_unique<cMouseCursorSimple> (eMouseCursorSimpleType::Hand), true);

	using namespace std::placeholders;

	signalConnectionManager.connect (cEventManager::getInstance().mouseMotionEvent, std::bind (&cMouse::handleMouseMotionEvent, this, _1));
	signalConnectionManager.connect (cEventManager::getInstance().mouseButtonEvent, std::bind (&cMouse::handleMouseButtonEvent, this, _1));
	signalConnectionManager.connect (cEventManager::getInstance().mouseWheelEvent, std::bind (&cMouse::handleMouseWheelEvent, this, _1));
}

//------------------------------------------------------------------------------
cMouse::~cMouse()
{}

//------------------------------------------------------------------------------
cMouse& cMouse::getInstance()
{
	static cMouse instance;
	return instance;
}

//------------------------------------------------------------------------------
void cMouse::handleMouseMotionEvent (const cEventMouseMotion& event)
{
	position = event.getNewPosition();
	moved (*this, event.getOffset());
}

//------------------------------------------------------------------------------
void cMouse::handleMouseButtonEvent (const cEventMouseButton& event)
{
	assert (event.getType() == cEventMouseButton::Down || event.getType() == cEventMouseButton::Up);

	auto button = event.getButton();

	if (event.getType() == cEventMouseButton::Down)
	{
		buttonPressedState[button] = true;

		const auto currentClickTime = std::chrono::steady_clock::now();
		auto& lastClickTime = getLastClickTime (button);
		buttonClickCount[button] = (currentClickTime - lastClickTime) <= doubleClickTime ? getButtonClickCount (button) + 1 : 1;
		lastClickTime = currentClickTime;

		pressed (*this, button);
	}
	else if (event.getType() == cEventMouseButton::Up)
	{
		buttonPressedState[button] = false;
		released (*this, button);
	}
}

//------------------------------------------------------------------------------
void cMouse::handleMouseWheelEvent (const cEventMouseWheel& event)
{
	wheelMoved (*this, event.getAmount());
}

//------------------------------------------------------------------------------
const cPosition& cMouse::getPosition() const
{
	return position;
}

//------------------------------------------------------------------------------
bool cMouse::setCursor (std::unique_ptr<cMouseCursor> cursor_, bool force)
{
	if (cursor_ == nullptr) return false;

	if (!force && cursor != nullptr && *cursor_ == *cursor) return false;

	auto cursorSurface = cursor_->getSurface();
	auto hotPoint = cursor_->getHotPoint();

	auto newSdlCursor = SdlCursorPtrType (SDL_CreateColorCursor (cursorSurface, hotPoint.x(), hotPoint.y()), SDL_FreeCursor);

	SDL_SetCursor (newSdlCursor.get());

	sdlCursor = std::move (newSdlCursor);

	cursor = std::move (cursor_);

	return true;
}

//------------------------------------------------------------------------------
void cMouse::show()
{
	SDL_ShowCursor (true);
}

//------------------------------------------------------------------------------
void cMouse::hide()
{
	SDL_ShowCursor (false);
}

//------------------------------------------------------------------------------
const cMouseCursor* cMouse::getCursor() const
{
	return cursor.get();
}

//------------------------------------------------------------------------------
bool cMouse::isButtonPressed (eMouseButtonType button) const
{
	auto iter = buttonPressedState.find (button);
	if (iter == buttonPressedState.end())
	{
		return buttonPressedState[button] = false; // initialize as unpressed
	}
	else return iter->second;
}

//------------------------------------------------------------------------------
unsigned int cMouse::getButtonClickCount (eMouseButtonType button) const
{
	auto iter = buttonClickCount.find (button);
	if (iter == buttonClickCount.end())
	{
		return buttonClickCount[button] = 0; // initialize with zero
	}
	else return iter->second;
}

//------------------------------------------------------------------------------
std::chrono::steady_clock::time_point& cMouse::getLastClickTime (eMouseButtonType button)
{
	auto iter = lastClickTime.find (button);
	if (iter == lastClickTime.end())
	{
		return lastClickTime[button]; // use default initialization. Is this really correct?!
	}
	else return iter->second;
}
