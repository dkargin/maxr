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

#ifndef utility_logH
#define utility_logH

#include "defines.h"
#include "utility/thread/mutex.h"



class cLog
{
public:
	enum eLogType
	{
		eLOG_TYPE_DEBUG,
		eLOG_TYPE_INFO,
		eLOG_TYPE_WARNING,
		eLOG_TYPE_ERROR,
		eLOG_TYPE_MEM,
		eLOG_TYPE_NET_DEBUG,
		eLOG_TYPE_NET_WARNING,
		eLOG_TYPE_NET_ERROR,
	};

	cLog();
	~cLog();

	/**
	* Writes message with default type (II) to the logfile
	*/
	void write(const char* msg);
	void write(const std::string& msg);

	/**
	* Writes message with given type to logfile
	*
	* @param str Message for the log
	* @param type Type for the log
	*/
	void write (const char* msg, eLogType type);
	void write (const std::string& msg, eLogType type);

	/**
	* Writes a marker into logfile - please use only veeeery few times!
	*/
	void mark();

private:
	FILE* logfile;
	FILE* netLogfile;
	cMutex mutex;

	void checkOpenFile(eLogType type);

	void writeToFile(std::string &msg, FILE* file);

} EX Log;

#endif // utility_logH
