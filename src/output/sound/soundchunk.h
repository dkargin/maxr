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

#ifndef output_sound_soundchunkH
#define output_sound_soundchunkH

#include <string>
#include <memory>
#include <chrono>

#include <SDL_mixer.h>

#include "maxrconfig.h"

class cSoundChunk
{
public:
	cSoundChunk();
	cSoundChunk (cSoundChunk&& other);
	cSoundChunk& operator= (cSoundChunk && other);

	bool operator==(const cSoundChunk& other) const;

	void load (const std::string& fileName);

	bool empty() const;

	std::chrono::milliseconds getLength() const;

	Mix_Chunk* getSdlSound() const;
private:
	cSoundChunk (const cSoundChunk& other) MAXR_DELETE_FUNCTION;
	cSoundChunk& operator= (const cSoundChunk& other) MAXR_DELETE_FUNCTION;

	struct SdlMixChunkDeleter
	{
		void operator() (Mix_Chunk*) const;
	};

	typedef std::unique_ptr<Mix_Chunk, SdlMixChunkDeleter> SaveSdlMixChunkPointer;

	SaveSdlMixChunkPointer sdlSound;
};

#endif // output_sound_soundchunkH
