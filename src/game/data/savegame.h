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

#ifndef game_data_savegameH
#define game_data_savegameH

#include <string>

#include "tinyxml2.h"

class cModel;
class cSaveGameInfo;
class cVersion;

//Versions prior to 1.0 are no longer compatible
#define SAVE_FORMAT_VERSION		((std::string)"1.0")
#define MINIMUM_REQUIRED_SAVE_VERSION ((std::string)"1.0")
#define MINIMUM_REQUIRED_MAXR_VERSION ((std::string)">0.2.9") //TODO: set to next release version!

class cSavegame
{
public:
	cSavegame();

	static std::string getFileName(int slot);

	/* saves the current gamestate to a file */
	void save(const cModel& model, int slot, const std::string& saveName);

	cSaveGameInfo loadSaveInfo(int slot);

	void loadModel(cModel& model, int slot);
	//loadGUIState(...);
private:

	/**
	* Loads header information from old save file with version <1.0.
	* Only loading of header information is supportet, to be able to display
	* these save files in the multiplayer menu. Loading game state from
	* savefiles <1.0 is not supported.
	*/
	void loadLegacyHeader(cSaveGameInfo& info);
	void writeHeader(int slot, const std::string& saveName, const cModel &model);
	bool loadDocument(int slot);
	bool loadVersion(cVersion& version);

	int saveingID; //for requesting GUI states
	int loadedSlot;

	tinyxml2::XMLDocument xmlDocument;
};

#endif //game_data_savegameH
