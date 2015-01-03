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

#include "utility/signal/signalconnectionmanager.h"
#include "utility/signal/signalconnection.h"

//------------------------------------------------------------------------------
cSignalConnectionManager::cSignalConnectionManager()
{}

//------------------------------------------------------------------------------
cSignalConnectionManager::cSignalConnectionManager (cSignalConnectionManager&& other) :
	connections (std::move (other.connections))
{}

//------------------------------------------------------------------------------
cSignalConnectionManager::~cSignalConnectionManager()
{
	disconnectAll();
}

//------------------------------------------------------------------------------
cSignalConnectionManager& cSignalConnectionManager::operator= (cSignalConnectionManager && other)
{
	connections = std::move (other.connections);
	return *this;
}

//------------------------------------------------------------------------------
bool cSignalConnectionManager::disconnect (cSignalConnection& connection)
{
	bool found = false;
	for (auto i = connections.begin(); i != connections.end();)
	{
		if (*i == connection)
		{
			i = connections.erase (i);
			found = true;
		}
		else
		{
			++i;
		}
	}
	if (found)
	{
		connection.disconnect();
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
void cSignalConnectionManager::disconnectAll()
{
	for (auto& connection : connections)
	{
		connection.disconnect();
	}
	clear();
}

//------------------------------------------------------------------------------
void cSignalConnectionManager::clear()
{
	connections.clear();
}
