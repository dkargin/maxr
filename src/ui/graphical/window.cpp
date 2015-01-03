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

#include "ui/graphical/window.h"
#include "ui/graphical/application.h"

#include "input/mouse/mouse.h"
#include "input/mouse/cursor/mousecursorsimple.h"
#include "settings.h"
#include "video.h"

//------------------------------------------------------------------------------
cWindow::cWindow (AutoSurface surface_, eWindowBackgrounds backgroundType_) :
	surface (nullptr),
	backgroundType (backgroundType_),
	activeApplication (nullptr),
	closing (false),
	hasBeenDrawnOnce (false)
{
	setSurface (std::move (surface_));
}

//------------------------------------------------------------------------------
bool cWindow::isClosing() const
{
	return closing;
}

//------------------------------------------------------------------------------
void cWindow::close()
{
	closing = true;
}

//------------------------------------------------------------------------------
void cWindow::draw (SDL_Surface& destination, const cBox<cPosition>& clipRect)
{
	if (!hasBeenDrawnOnce)
	{
		switch (backgroundType)
		{
			case eWindowBackgrounds::Black:
				SDL_FillRect (cVideo::buffer, nullptr, 0xFF000000);
				break;
			case eWindowBackgrounds::Alpha:
				// NOTE: this is not fully robust yet! It will not work if an
				// alpha-background-window will call another alpha-background-window.
				// Returning to an alpha-background-window from any other window
				// will not work as expected as well.
				if (cSettings::getInstance().isAlphaEffects())
				{
					Video.applyShadow (nullptr, destination);
				}
				break;
			case eWindowBackgrounds::Transparent:
				// do nothing here
				break;
		}
	}

	SDL_Rect position = getArea().toSdlRect();
	if (surface != nullptr) SDL_BlitSurface (surface.get(), nullptr, &destination, &position);

	hasBeenDrawnOnce = true;

	cWidget::draw (destination, clipRect); // draws all children
}

//------------------------------------------------------------------------------
void cWindow::handleActivated (cApplication& application, bool firstTime)
{
	// one window should not be displayed in multiple applications at once
	// (we have only one application anyway...)
	assert (activeApplication == nullptr);

	hasBeenDrawnOnce = false;
	activeApplication = &application;

	auto mouse = activeApplication->getActiveMouse();
	if (mouse)
	{
		auto defaultCursor = getDefaultCursor();
		if (defaultCursor)
		{
			mouse->setCursor (std::move (defaultCursor));
		}
	}
}

//------------------------------------------------------------------------------
void cWindow::handleDeactivated (cApplication& application, bool removed)
{
	if (activeApplication == &application)
	{
		activeApplication = nullptr;
	}

	if (removed)
	{
		assert (isClosing());

		terminated();
	}
}

//------------------------------------------------------------------------------
bool cWindow::wantsCentered() const
{
	return true;
}

//------------------------------------------------------------------------------
cApplication* cWindow::getActiveApplication() const
{
	return activeApplication;
}

//------------------------------------------------------------------------------
std::unique_ptr<cMouseCursor> cWindow::getDefaultCursor() const
{
	return std::make_unique<cMouseCursorSimple> (eMouseCursorSimpleType::Hand);
}

//------------------------------------------------------------------------------
cMouse* cWindow::getActiveMouse() const
{
	return activeApplication ? activeApplication->getActiveMouse() : nullptr;
}

//------------------------------------------------------------------------------
cKeyboard* cWindow::getActiveKeyboard() const
{
	return activeApplication ? activeApplication->getActiveKeyboard() : nullptr;
}

//------------------------------------------------------------------------------
SDL_Surface* cWindow::getSurface()
{
	return surface.get();
}

//------------------------------------------------------------------------------
void cWindow::setSurface (AutoSurface surface_)
{
	surface = std::move (surface_);
	if (surface != nullptr)
	{
		resize (cPosition (surface->w, surface->h));
	}
}
