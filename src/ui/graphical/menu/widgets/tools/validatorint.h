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

#ifndef ui_graphical_widgets_tools_validatorintH
#define ui_graphical_widgets_tools_validatorintH

#include "maxrconfig.h"
#include "ui/graphical/menu/widgets/tools/validator.h"

class cValidatorInt : public cValidator
{
public:
	cValidatorInt();
	cValidatorInt (int minValue, int maxValue);

	virtual eValidatorState validate (const std::string& text) const MAXR_OVERRIDE_FUNCTION;

	virtual void fixup (std::string& text) const MAXR_OVERRIDE_FUNCTION;

private:
	int minValue;
	int maxValue;
};

#endif // ui_graphical_widgets_tools_validatorintH
