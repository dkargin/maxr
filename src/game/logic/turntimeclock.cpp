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

#include <algorithm>

#include "game/logic/turntimeclock.h"
#include "game/data/model.h"
#include "game/logic/gametimer.h"

const std::chrono::seconds cTurnTimeClock::alertRemainingTime (20);

//------------------------------------------------------------------------------
cTurnTimeDeadline::cTurnTimeDeadline (unsigned int startGameTime_, const std::chrono::milliseconds& deadline_, unsigned int id_) :
	startGameTime (startGameTime_),
	deadline (deadline_),
	id (id_)
{}

//------------------------------------------------------------------------------
cTurnTimeDeadline::cTurnTimeDeadline() :
	startGameTime(0),
	deadline(0),
	id(0)
{}

//------------------------------------------------------------------------------
unsigned int cTurnTimeDeadline::getStartGameTime() const
{
	return startGameTime;
}

//------------------------------------------------------------------------------
const std::chrono::milliseconds& cTurnTimeDeadline::getDeadline() const
{
	return deadline;
}

//------------------------------------------------------------------------------
unsigned int cTurnTimeDeadline::getId() const
{
	return id;
}

//------------------------------------------------------------------------------
void cTurnTimeDeadline::changeDeadline (const std::chrono::milliseconds& deadline_)
{
	deadline = deadline_;
}

//------------------------------------------------------------------------------
uint32_t cTurnTimeDeadline::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(startGameTime, crc);
	crc = calcCheckSum(deadline.count(), crc);
	crc = calcCheckSum(id, crc);

	return crc;
}

//------------------------------------------------------------------------------
cTurnTimeClock::cTurnTimeClock (const cModel& model) :
	model (model),
	startTurnGameTime (0),
	stoppedTicks (0),
	stopped (false)
{
	std::chrono::seconds lastCheckedSeconds (0);
	std::chrono::seconds lastTimeTillFirstDeadline (std::numeric_limits<std::chrono::seconds::rep>::max());
	signalConnectionManager.connect (model.gameTimeChanged, [lastCheckedSeconds, lastTimeTillFirstDeadline, this]() mutable
	{
		const std::chrono::seconds currentSeconds(this->model.getGameTime() / 100);
		if (currentSeconds != lastCheckedSeconds)
		{
			lastCheckedSeconds = currentSeconds;
			secondChanged();

			if (hasDeadline())
			{
				const auto currentTimeTillFirstDeadline = std::chrono::duration_cast<std::chrono::seconds> (getTimeTillFirstDeadline());
				if (lastTimeTillFirstDeadline > alertRemainingTime && currentTimeTillFirstDeadline <= alertRemainingTime)
				{
					alertTimeReached();
				}
				lastTimeTillFirstDeadline = currentTimeTillFirstDeadline;
			}
		}
	});
}

//------------------------------------------------------------------------------
void cTurnTimeClock::restartFromNow()
{
	restartFrom(model.getGameTime());
}

//------------------------------------------------------------------------------
void cTurnTimeClock::restartFrom (unsigned int gameTime)
{
	startTurnGameTime = gameTime;

	if (stopped) stoppedAtTime = gameTime;
	stoppedTicks = 0;

	secondChanged();
}

//------------------------------------------------------------------------------
void cTurnTimeClock::stop()
{
	stopAt(model.getGameTime());
}

//------------------------------------------------------------------------------
void cTurnTimeClock::stopAt (unsigned int gameTime)
{
	if (stopped) return;

	stoppedAtTime = gameTime;

	stopped = true;
}

//------------------------------------------------------------------------------
void cTurnTimeClock::resume()
{
	resumeAt(model.getGameTime());
}

//------------------------------------------------------------------------------
void cTurnTimeClock::resumeAt (unsigned int gameTime)
{
	if (!stopped) return;

	stoppedTicks += gameTime - stoppedAtTime;

	stopped = false;
}

//------------------------------------------------------------------------------
unsigned int cTurnTimeClock::getStartGameTime() const
{
	return startTurnGameTime;
}

//------------------------------------------------------------------------------
void cTurnTimeClock::clearAllDeadlines()
{
	deadlines.clear();
	deadlinesChanged();
}

//------------------------------------------------------------------------------
unsigned int cTurnTimeClock::startNewDeadlineFromNow (const std::chrono::milliseconds& duration)
{
	return startNewDeadlineFrom(model.getGameTime(), duration);
}

//------------------------------------------------------------------------------
unsigned int cTurnTimeClock::startNewDeadlineFrom (unsigned int gameTime, const std::chrono::milliseconds& duration)
{
	cTurnTimeDeadline deadline(gameTime, duration, nextDeadlineId);
	nextDeadlineId++;

	deadlines.push_back (deadline);
	deadlinesChanged();
	return deadline.getId();
}

//------------------------------------------------------------------------------
void cTurnTimeClock::removeDeadline (unsigned int id)
{
	for (auto i = deadlines.begin(); i != deadlines.end(); ++i)
	{
		if ((*i).getId() == id)
		{
			deadlines.erase (i);
			deadlinesChanged();
			return;
		}
	}
}

//------------------------------------------------------------------------------
void cTurnTimeClock::changeDeadline(unsigned int id, const std::chrono::seconds& duration)
{
	for (auto i = deadlines.begin(); i != deadlines.end(); ++i)
	{
		if (i->getId() == id)
		{
			i->changeDeadline(duration);
			deadlinesChanged();
			return;
		}
	}
}


//------------------------------------------------------------------------------
std::chrono::milliseconds cTurnTimeClock::getTimeSinceStart() const
{
	if (startTurnGameTime > model.getGameTime()) return std::chrono::milliseconds(0);

	const auto ticksSinceStart = model.getGameTime() - startTurnGameTime;
	const auto ticksStopped = stoppedTicks + (stopped ? model.getGameTime() - stoppedAtTime : 0);
	assert (ticksSinceStart >= ticksStopped);
	return std::chrono::milliseconds ((ticksSinceStart - ticksStopped) * GAME_TICK_TIME);
}

//------------------------------------------------------------------------------
std::chrono::milliseconds cTurnTimeClock::getTimeTillFirstDeadline() const
{
	if (deadlines.empty()) return std::chrono::milliseconds (0);

	auto minTime = getTimeTillDeadlineReached (deadlines[0]);
	for (auto i = deadlines.begin() + 1; i != deadlines.end(); ++i)
	{
		minTime = std::min (minTime, getTimeTillDeadlineReached (*i));
	}
	return minTime;
}

//------------------------------------------------------------------------------
bool cTurnTimeClock::hasReachedAnyDeadline() const
{
	for (auto i = deadlines.begin(); i != deadlines.end(); ++i)
	{
		if (getTimeTillDeadlineReached (*i) <= std::chrono::milliseconds (0))
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
uint32_t cTurnTimeClock::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(deadlines, crc);
	crc = calcCheckSum(startTurnGameTime, crc);
	crc = calcCheckSum(stoppedAtTime, crc);
	crc = calcCheckSum(nextDeadlineId, crc);
	crc = calcCheckSum(stoppedTicks, crc);
	crc = calcCheckSum(stopped, crc);

	return crc;
}

//------------------------------------------------------------------------------
bool cTurnTimeClock::hasDeadline() const
{
	return !deadlines.empty();
}

//------------------------------------------------------------------------------
std::chrono::milliseconds cTurnTimeClock::getTimeTillDeadlineReached (const cTurnTimeDeadline& deadline) const
{
	const auto ticksStopped = stoppedTicks + (stopped ? model.getGameTime() - stoppedAtTime : 0);
	const auto deadlineEndMilliseconds = (deadline.getStartGameTime() + ticksStopped) * GAME_TICK_TIME + deadline.getDeadline().count();
	const auto currentTimeMilliseconds = model.getGameTime() * GAME_TICK_TIME;

	return std::chrono::milliseconds (deadlineEndMilliseconds < currentTimeMilliseconds ? 0 : deadlineEndMilliseconds - currentTimeMilliseconds);
}
