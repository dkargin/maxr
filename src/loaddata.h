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
//
//
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef loaddataH
#define loaddataH

#include <string>
#include <vector>

struct SDL_Thread;
struct SDL_Mutex;
///////////////////////////////////////////////////////////////////////////////
// Defines
// ------------------------
//
///////////////////////////////////////////////////////////////////////////////

class cLoader
{

public:
	enum eLoadingState
	{
		LOAD_IDLE = 0,
		LOAD_GOING = 1,
		LOAD_ERROR = 2,
		LOAD_FINISHED = 3,
	};

public:
	cLoader();
	~cLoader();

	/**
	 * Loads all relevant files and data
	 * This is asynchronous function
	 * @param mod - name of the mod.
	 * @return current loading state
	 */
	eLoadingState load(const char *mod = nullptr);

	// Get current loading state
	int getState() const;

	// Get event type for polling SDL event queue
	int getEventType() const;

	// Wait until internal thread is complete
	void join();
protected:
	static int threadFn(void * data);

	int loadImpl();
	// Called after all data is loaded
	// Checks data integrity and creates cross-links.
	void finalizeLoading();

	// Loads all the data from specified folder
	int loadFolder(const char * path);

	void notifyState(eLoadingState newState);

	enum ConsoleLine
	{
		SAME_LINE = 0,
		NEXT_LINE = 1,
	};

	void writeConsole (const std::string& sTxt, int ok, int increment=1);
private:
	SDL_Thread* load_thread = nullptr;
	SDL_Mutex* guard = nullptr;
	volatile eLoadingState lastState = LOAD_IDLE;

	const int LoaderNotification;

	// Mods to be loaded
	std::vector<std::string> pendingMods;

	// Current log position in graphical console
	int logPosition = 0;

	// Should we use some delays to let user see messages
	bool useDelay = false;
};


void createShadowGfx();

/**
* Splits a string s by "word" according to one of separators seps.
*/
void Split (const std::string& s, const char* seps, std::vector<std::string>& words);

#endif
