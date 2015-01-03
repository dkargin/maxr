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

#ifndef utility_string_tolowerH
#define utility_string_tolowerH

#include <string>
#include <algorithm>
#include <cctype>

static inline std::string& to_lower (std::string& s)
{
	std::transform (s.begin(), s.end(), s.begin(), static_cast<int (&) (int)> (std::tolower));
	return s;
}

static inline std::string to_lower_copy (const std::string& s)
{
	auto s2 = s;
	return to_lower (s2);
}

#endif // utility_string_tolowerH
