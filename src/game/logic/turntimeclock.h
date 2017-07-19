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

#ifndef game_logic_turntimeclockH
#define game_logic_turntimeclockH

#include <memory>
#include <chrono>

#include "utility/signal/signal.h"
#include "utility/signal/signalconnectionmanager.h"

class cModel;

class cTurnTimeDeadline
{
public:
	cTurnTimeDeadline (unsigned int startGameTime, const std::chrono::milliseconds& deadline, unsigned int id);
	cTurnTimeDeadline ();

	unsigned int getStartGameTime() const;
	const std::chrono::milliseconds& getDeadline() const;
	unsigned int getId() const;

	void changeDeadline (const std::chrono::milliseconds& deadline);

	uint32_t getChecksum(uint32_t crc) const;

	template <typename T>
	void serialize(T& archive)
	{
		archive & NVP(startGameTime);
		archive & NVP(deadline);
		archive & NVP(id);
	}
private:
	unsigned int startGameTime;
	std::chrono::milliseconds deadline;
	unsigned int id;
};

class cTurnTimeClock
{
public:
	static const std::chrono::seconds alertRemainingTime;

	explicit cTurnTimeClock (const cModel& model);

	void restartFromNow();
	void restartFrom (unsigned int gameTime);

	void stop();
	void stopAt (unsigned int gameTime);

	void resume();
	void resumeAt (unsigned int gameTime);

	unsigned int getStartGameTime() const;

	void clearAllDeadlines();

	unsigned int startNewDeadlineFromNow (const std::chrono::milliseconds& duration);
	unsigned int startNewDeadlineFrom (unsigned int gameTime, const std::chrono::milliseconds& duration);

	void removeDeadline (unsigned int id);
	void changeDeadline(unsigned int turnEndDeadline, const std::chrono::seconds& duration);

	std::chrono::milliseconds getTimeSinceStart() const;
	std::chrono::milliseconds getTimeTillFirstDeadline() const;

	bool hasDeadline() const;

	bool hasReachedAnyDeadline() const;

	mutable cSignal<void ()> secondChanged;
	mutable cSignal<void ()> deadlinesChanged;
	mutable cSignal<void ()> alertTimeReached;

	uint32_t getChecksum(uint32_t crc) const;

	template <typename T>
	void serialize(T& archive)
	{
		archive & NVP(deadlines);
		archive & NVP(startTurnGameTime);
		archive & NVP(stoppedAtTime);
		archive & NVP(nextDeadlineId);
		archive & NVP(stoppedTicks);
		archive & NVP(stopped);
	}
private:
	cSignalConnectionManager signalConnectionManager;

	const cModel& model;
	std::vector<cTurnTimeDeadline> deadlines;

	unsigned int nextDeadlineId;
	unsigned int startTurnGameTime;
	unsigned int stoppedAtTime;
	unsigned int stoppedTicks;
	bool stopped;

	std::chrono::milliseconds getTimeTillDeadlineReached (const cTurnTimeDeadline& deadline) const;

};

#endif // game_logic_turnclockH
