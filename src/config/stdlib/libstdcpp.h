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

#ifndef __GNUC__
#   ifndef MAXR_NO_ASSERT_CONFIG
#       error "Mixing libstdc++ with any other compiler than gcc is not yet supported!"
#   endif
#endif

#ifndef MAXR_NO_UNDERLYING_TYPE
#   if __GNUC__ == 4 && __GNUC_MINOR__ < 7
#       define MAXR_NO_UNDERLYING_TYPE     1
#   endif
#endif

#ifndef MAXR_NO_MAKE_UNIQUE
#   if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 9)
#       define MAXR_NO_MAKE_UNIQUE         1
#   endif
#endif
