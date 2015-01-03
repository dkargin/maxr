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

#ifndef ui_graphical_menu_windows_windowloadsave_windowloadsaveH
#define ui_graphical_menu_windows_windowloadsave_windowloadsaveH

#include "ui/graphical/menu/windows/windowload/windowload.h"

class cPushButton;
class cTurnTimeClock;

class cWindowLoadSave : public cWindowLoad
{
public:
	explicit cWindowLoadSave (std::shared_ptr<const cTurnTimeClock> turnTimeClock);

	cSignal<void (int, const std::string&)> save;
	cSignal<void ()> exit;

protected:
	virtual void handleSlotClicked (size_t index) MAXR_OVERRIDE_FUNCTION;
	virtual void handleSlotDoubleClicked (size_t index) MAXR_OVERRIDE_FUNCTION;

private:
	cSignalConnectionManager signalConnectionManager;

	cPushButton* saveButton;

	void handleSaveClicked();
};

#endif // ui_graphical_menu_windows_windowloadsave_windowloadsaveH
