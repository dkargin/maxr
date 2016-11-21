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

#ifndef game_logic_jobsH
#define game_logic_jobsH

class cGameTimer;
class cJobContainer;
class cUnit;
class cVehicle;
class cPosition;

#include <vector>
#include "utility/position.h"

/**
* little helper jobs for game time synchonous actions,
* like rotating a unit to a spezific direction or landing/takeoff
*/
class cJob
{
	friend class cJobContainer;
protected:
	cJob (cVehicle& vehicle_, unsigned int id);
public:
	virtual ~cJob() {}
	virtual void run (const cGameTimer& gameTimer) = 0;
	unsigned int getId() const { return id; };

protected:
	unsigned int id;
	bool finished;
	cVehicle* vehicle;
};

class cJobContainer
{
public:
	void addJob (cJob& job);
	void onRemoveUnit (cUnit* unit);
	void run (cGameTimer& gameTimer);
	void clear();
private:
	std::vector<cJob*>::iterator releaseJob (std::vector<cJob*>::iterator it);
private:
	std::vector<cJob*> jobs;
};


class cStartBuildJob : public cJob
{
private:
	cPosition org;
	bool big;
public:
	cStartBuildJob (cVehicle& vehicle, const cPosition& org, bool big, unsigned int id);
	virtual void run (const cGameTimer& gameTimer);
};


class cPlaneTakeoffJob : public cJob
{
private:
	bool takeoff;
public:
	cPlaneTakeoffJob (cVehicle& vehicle_, bool takeoff_, unsigned int id);
	virtual void run (const cGameTimer& gameTimer);
};

#endif // game_logic_jobsH
