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

#ifndef game_logic_jobs_jobcontainerH
#define game_logic_jobs_jobcontainerH

#include "utility/serialization/serialization.h"
#include "job.h"

class cJob;
class cUnit;
class cModel;

class cJobContainer
{
public:
	~cJobContainer();
	void addJob (cJob& job);
	void onRemoveUnit (cUnit* unit);
	void run (cModel& model);
	void clear();
	uint32_t getChecksum(uint32_t crc) const;

	template<typename T>
	void save(T& archive)
	{
		archive << serialization::makeNvp("numJobs", (int)jobs.size());
		for (auto job : jobs)
		{
			archive << serialization::makeNvp("job", *job);
		}
	}
	template<typename T>
	void load(T& archive)
	{
		int numJobs;
		archive >> NVP(numJobs);
		jobs.resize(numJobs);
		for (auto& job : jobs)
		{
			job = cJob::createFrom(archive, "job");
		}
	}
	SERIALIZATION_SPLIT_MEMBER();

private:
	std::vector<cJob*>::iterator releaseJob (std::vector<cJob*>::iterator it);
private:
	std::vector<cJob*> jobs;
};

#endif