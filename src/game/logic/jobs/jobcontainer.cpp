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

#include "jobcontainer.h"

#include "job.h"
#include "game/data/units/unit.h"
#include "utility/crc.h"

cJobContainer::~cJobContainer()
{
	clear();
}

void cJobContainer::addJob (cJob& job)
{
	//only one job per unit
	if (job.unit->job)
	{
		std::vector<cJob*>::iterator it = std::find (jobs.begin(), jobs.end(), job.unit->job);
		releaseJob (it);
	}

	jobs.push_back (&job);
	job.unit->job = &job;
}

void cJobContainer::run (cModel& model)
{
	for (std::vector<cJob*>::iterator it = jobs.begin(); it != jobs.end();)
	{
		cJob* job = *it;

		if (!job->finished) job->run (model);

		if (job->finished) it = releaseJob (it);
		else ++it;
	}
}

void cJobContainer::clear()
{
	for (unsigned int i = 0; i < jobs.size(); i++)
	{
		cJob* job = jobs[i];
		if (job->unit)
		{
			assert(job->unit->job == job);
			job->unit->job = nullptr;
		}
		delete job;
	}
	jobs.clear();
}

uint32_t cJobContainer::getChecksum(uint32_t crc) const
{
	for (const auto job : jobs)
	{
		crc = calcCheckSum(*job, crc);
	}
	return crc;
}

std::vector<cJob*>::iterator cJobContainer::releaseJob (std::vector<cJob*>::iterator it)
{
	if (it == jobs.end()) return jobs.end();
	cJob* job = *it;
	if (job->unit)
	{
		assert(job->unit->job == job);
		job->unit->job = nullptr;
	}
	it = jobs.erase (it);
	delete job;
	return it;
}

void cJobContainer::onRemoveUnit (cUnit* unit)
{
	for (std::vector<cJob*>::iterator it = jobs.begin(); it != jobs.end();)
	{
		cJob* job = *it;
		if (job->unit == unit)
		{
			job->unit = nullptr;
			job->finished = true;
		}
		else ++it;
	}
}
