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

#include "game/data/report/unit/savedreportsurveyoraiconfused.h"

//------------------------------------------------------------------------------
cSavedReportSurveyorAiConfused::cSavedReportSurveyorAiConfused (const cUnit& unit) :
	cSavedReportUnit (unit)
{}

//------------------------------------------------------------------------------
cSavedReportSurveyorAiConfused::cSavedReportSurveyorAiConfused (cNetMessage& message) :
	cSavedReportUnit (message)
{}

//------------------------------------------------------------------------------
cSavedReportSurveyorAiConfused::cSavedReportSurveyorAiConfused (const tinyxml2::XMLElement& element) :
	cSavedReportUnit (element)
{}

//------------------------------------------------------------------------------
void cSavedReportSurveyorAiConfused::pushInto (cNetMessage& message) const
{
	cSavedReportUnit::pushInto (message);
}

//------------------------------------------------------------------------------
void cSavedReportSurveyorAiConfused::pushInto (tinyxml2::XMLElement& element) const
{
	cSavedReportUnit::pushInto (element);
}

//------------------------------------------------------------------------------
eSavedReportType cSavedReportSurveyorAiConfused::getType() const
{
	return eSavedReportType::SurveyorAiConfused;
}

//------------------------------------------------------------------------------
std::string cSavedReportSurveyorAiConfused::getText() const
{
	return "Surveyor AI: I'm totally confused. Don't know what to do...";
}