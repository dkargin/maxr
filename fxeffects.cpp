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

#include <SDL.h>

#include "fxeffects.h"

#include "client.h"
#include "hud.h"
#include "main.h"
#include "player.h"
#include "video.h"

cFx::cFx (bool bottom_, int x, int y) :
	posX (x),
	posY (y),
	tick (0),
	length (-1),
	bottom (bottom_)
{}

cFx::~cFx()
{}

int cFx::getLength() const
{
	return length;
}

bool cFx::isFinished() const
{
	return tick >= length;
}

void cFx::playSound (const cGameGUI& gameGUI) const
{}

void cFx::run()
{
	tick++;
}

//------------------------------------------------------------------------------

cFxContainer::~cFxContainer()
{
	for (size_t i = 0, size = fxs.size(); i != size; ++i)
	{
		delete fxs[i];
	}
}

void cFxContainer::push_back (cFx* fx)
{
	fxs.push_back (fx);
}

void cFxContainer::push_front (cFx* fx)
{
	fxs.insert (fxs.begin(), fx);
}

void cFxContainer::draw (const cGameGUI& gameGUI, bool bottom) const
{
	for (size_t i = 0, size = fxs.size(); i != size; ++i)
	{
		if (fxs[i]->bottom == bottom)
		{
			fxs[i]->draw (gameGUI);
		}
	}
}

void cFxContainer::run()
{
	for (std::vector<cFx*>::iterator it = fxs.begin(); it != fxs.end();)
	{
		cFx* fx = *it;

		fx->run();
		if (fx->isFinished())
		{
			delete fx;
			it = fxs.erase (it);
		}
		else ++it;
	}
}

//------------------------------------------------------------------------------
cFxMuzzle::cFxMuzzle (int x, int y, int dir_) :
	cFx (false, x, y),
	pImages (NULL),
	dir (dir_)
{}

void cFxMuzzle::draw (const cGameGUI& gameGUI) const
{
	const cPlayer& activePlayer = gameGUI.getClient()->getActivePlayer();
	const cMap& map = *gameGUI.getClient()->getMap();
	if (!activePlayer.ScanMap[map.getOffset (posX / 64, posY / 64)]) return;
	if (pImages == NULL) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	SDL_Rect src;
	src.x = (int) (images[0]->w * gameGUI.getZoom() * dir / 8);
	src.y = 0;
	src.w = images[1]->w / 8;
	src.h = images[1]->h;
	SDL_Rect dest = gameGUI.calcScreenPos (posX, posY);

	SDL_BlitSurface (images[1], &src, cVideo::buffer, &dest);
}

//------------------------------------------------------------------------------
cFxMuzzleBig::cFxMuzzleBig (int x, int y, int dir_) :
	cFxMuzzle (x, y, dir_)
{
	pImages = &EffectsData.fx_muzzle_big;
	length = 6;
}

//------------------------------------------------------------------------------
cFxMuzzleMed::cFxMuzzleMed (int x, int y, int dir_) :
	cFxMuzzle (x, y, dir_)
{
	pImages = &EffectsData.fx_muzzle_med;
	length = 6;
}

//------------------------------------------------------------------------------
cFxMuzzleMedLong::cFxMuzzleMedLong (int x, int y, int dir_) :
	cFxMuzzle (x, y, dir_)
{
	length = 16;
	pImages = &EffectsData.fx_muzzle_med;
}

//------------------------------------------------------------------------------
cFxMuzzleSmall::cFxMuzzleSmall (int x, int y, int dir_) :
	cFxMuzzle (x, y, dir_)
{
	length = 6;
	pImages = &EffectsData.fx_muzzle_small;
}

//------------------------------------------------------------------------------
cFxExplo::cFxExplo (int x, int y, int frames_) :
	cFx (false, x, y),
	pImages (NULL),
	frames (frames_)
{}

void cFxExplo::draw (const cGameGUI& gameGUI) const
{
	const cPlayer& activePlayer = gameGUI.getClient()->getActivePlayer();
	const cMap& map = *gameGUI.getClient()->getMap();
	if (!activePlayer.ScanMap[map.getOffset (posX / 64, posY / 64)]) return;
	if (!pImages) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	const int frame = tick * frames / length;

	SDL_Rect src, dest;
	src.x = (int) (images[0]->w * gameGUI.getZoom() * frame / frames);
	src.y = 0;
	src.w = images[1]->w / frames;
	src.h = images[1]->h;
	dest = gameGUI.calcScreenPos (posX - images[0]->w / (frames * 2), posY - images[0]->h / 2);

	SDL_BlitSurface (images[1], &src, cVideo::buffer, &dest);
}

//------------------------------------------------------------------------------
cFxExploSmall::cFxExploSmall (int x, int y) :
	cFxExplo (x, y, 14)
{
	length = 140;
	pImages = &EffectsData.fx_explo_small;
}

void cFxExploSmall::playSound (const cGameGUI& gameGUI) const
{
	PlayRandomFX (SoundData.EXPSmall);
}

//------------------------------------------------------------------------------
cFxExploBig::cFxExploBig (int x, int y) :
	cFxExplo (x, y, 28)
{
	length = 280;
	pImages = &EffectsData.fx_explo_big;
}


void cFxExploBig::playSound (const cGameGUI& gameGUI) const
{
	const cMap& map = *gameGUI.getClient()->getMap();
	if (map.isWaterOrCoast (posX / 64, posY / 64))
	{
		PlayRandomFX (SoundData.EXPBigWet);
	}
	else
	{
		PlayRandomFX (SoundData.EXPBig);
	}
}

//------------------------------------------------------------------------------
cFxExploAir::cFxExploAir (int x, int y) :
	cFxExplo (x, y, 14)
{
	length = 140;
	pImages = &EffectsData.fx_explo_air;
}

void cFxExploAir::playSound (const cGameGUI& gameGUI) const
{
	PlayRandomFX (SoundData.EXPSmall);
}

//------------------------------------------------------------------------------
cFxExploWater::cFxExploWater (int x, int y) :
	cFxExplo (x, y, 14)
{
	length = 140;
	pImages = &EffectsData.fx_explo_water;
}

void cFxExploWater::playSound (const cGameGUI& gameGUI) const
{
	PlayRandomFX (SoundData.EXPSmallWet);
}

//------------------------------------------------------------------------------
cFxHit::cFxHit (int x, int y) :
	cFxExplo (x, y, 5)
{
	length = 50;
	pImages = &EffectsData.fx_hit;
}

void cFxHit::playSound (const cGameGUI& gameGUI) const
{
	//TODO
}

//------------------------------------------------------------------------------
cFxAbsorb::cFxAbsorb (int x, int y) :
	cFxExplo (x, y, 10)
{
	length = 100;
	pImages = &EffectsData.fx_absorb;
}

void cFxAbsorb::playSound (const cGameGUI& gameGUI) const
{
	PlayFX (SoundData.SNDAbsorb);
}

//------------------------------------------------------------------------------
cFxFade::cFxFade (int x, int y, bool bottom, int start, int end) :
	cFx (bottom, x, y),
	pImages (NULL),
	alphaStart (start),
	alphaEnd (end)
{}

void cFxFade::draw (const cGameGUI& gameGUI) const
{
	const cPlayer& activePlayer = gameGUI.getClient()->getActivePlayer();
	const cMap& map = *gameGUI.getClient()->getMap();
	if (!activePlayer.ScanMap[map.getOffset (posX / 64, posY / 64)]) return;
	if (!pImages) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	const int alpha = (alphaEnd - alphaStart) * tick / length + alphaStart;
	SDL_SetSurfaceAlphaMod (images[1], alpha);

	SDL_Rect dest;
	dest = gameGUI.calcScreenPos (posX - images[0]->w / 2, posY - images[0]->h / 2);
	SDL_BlitSurface (images[1], NULL, cVideo::buffer, &dest);
}

//------------------------------------------------------------------------------
cFxSmoke::cFxSmoke (int x, int y, bool bottom) :
	cFxFade (x, y, bottom, 100, 0)
{
	length = 50;
	pImages = &EffectsData.fx_smoke;
}

//------------------------------------------------------------------------------
cFxCorpse::cFxCorpse (int x, int y) :
	cFxFade (x, y, true, 255, 0)
{
	length = 1024;
	pImages = &EffectsData.fx_corpse;
}

//------------------------------------------------------------------------------
cFxTracks::cFxTracks (int x, int y, int dir_) :
	cFx (true, x, y),
	pImages (NULL),
	alphaStart (100),
	alphaEnd (0),
	dir (dir_)
{
	length = 1024;
	pImages = &EffectsData.fx_tracks;
}

void cFxTracks::draw (const cGameGUI& gameGUI) const
{
	const cPlayer& activePlayer = gameGUI.getClient()->getActivePlayer();
	const cMap& map = *gameGUI.getClient()->getMap();
	if (!activePlayer.ScanMap[map.getOffset (posX / 64, posY / 64)]) return; //ja, nein, vielleicht?
	if (!pImages) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	const int alpha = (alphaEnd - alphaStart) * tick / length + alphaStart;
	SDL_SetSurfaceAlphaMod (images[1], alpha);

	SDL_Rect src, dest;
	src.y = 0;
	src.x = images[1]->w * dir / 4;
	src.w = images[1]->w / 4;
	src.h = images[1]->h;
	dest = gameGUI.calcScreenPos (posX, posY);

	SDL_BlitSurface (images[1], &src, cVideo::buffer, &dest);
}

//------------------------------------------------------------------------------
cFxRocket::cFxRocket (int startX_, int startY_, int endX_, int endY_, int dir_, bool bottom) :
	cFx (bottom, startX_, startY_),
	speed (8),
	pImages (&EffectsData.fx_rocket),
	dir (dir_),
	distance (0),
	startX (startX_),
	startY (startY_),
	endX (endX_),
	endY (endY_)
{
	distance = (int) sqrtf (float (Square (endX - startX) + Square (endY - startY)));
	length = distance / speed;
}

cFxRocket::~cFxRocket()
{
	for (size_t i = 0; i != subEffects.size(); ++i)
		delete subEffects[i];
}

void cFxRocket::draw (const cGameGUI& gameGUI) const
{
	//draw smoke effect
	for (unsigned i = 0; i < subEffects.size(); i++)
	{
		subEffects[i]->draw (gameGUI);
	}

	//draw rocket
	const cPlayer& activePlayer = gameGUI.getClient()->getActivePlayer();
	const cMap& map = *gameGUI.getClient()->getMap();
	if (!activePlayer.ScanMap[map.getOffset (posX / 64, posY / 64)]) return;
	if (!pImages) return;
	if (tick >= length) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	SDL_Rect src, dest;
	src.x = dir * images[1]->w / 8;
	src.y = 0;
	src.h = images[1]->h;
	src.w = images[1]->w / 8;
	dest = gameGUI.calcScreenPos (posX - images[0]->w / 16, posY - images[0]->h / 2);

	SDL_BlitSurface (images[1], &src, cVideo::buffer, &dest);
}

void cFxRocket::run()
{
	//run smoke effect
	for (unsigned i = 0; i < subEffects.size(); i++)
	{
		subEffects[i]->run();
		if (subEffects[i]->isFinished())
		{
			delete subEffects[i];
			subEffects.erase (subEffects.begin() + i);
			i--;
		}
	}

	//add new smoke
	if (tick >= length) return;
	if (cSettings::getInstance().isAlphaEffects())
	{
		subEffects.push_back (new cFxSmoke (posX, posY, bottom));
	}

	//update rocket position
	tick++;
	posX = startX + speed * (endX - startX) * tick / distance;
	posY = startY + speed * (endY - startY) * tick / distance;
}

bool cFxRocket::isFinished() const
{
	return tick >= length && subEffects.empty();
}

cFxDarkSmoke::cFxDarkSmoke (int x, int y, int alpha, float windDir) :
	cFx (false, x, y),
	dx (0),
	dy (0),
	alphaStart (alpha),
	alphaEnd (0),
	frames (50),
	pImages (&EffectsData.fx_dark_smoke)
{
	length = 200;

	const float ax = abs (sinf (windDir));
	const float ay = abs (cosf (windDir));
	if (ax > ay)
	{
		dx = (ax +  random (5)       / 20.0f) / 2.f;
		dy = (ay + (random (15) - 7) / 28.0f) / 2.f;
	}
	else
	{
		dx = (ax + (random (15) - 7) / 28.0f) / 2.f;
		dy = (ay +  random (5)       / 20.0f) / 2.f;
	}
}

void cFxDarkSmoke::draw (const cGameGUI& gameGUI) const
{
	//if (!client.getActivePlayer().ScanMap[posX / 64 + posY / 64 * client.getMap()->size]) return;
	if (!pImages) return;
	AutoSurface (&images) [2] (*pImages);
	CHECK_SCALING (images[1], images[0], gameGUI.getZoom());

	const int frame = tick * frames / length;

	SDL_Rect src, dest;
	src.x = (int) (images[0]->w * gameGUI.getZoom() * frame / frames);
	src.y = 0;
	src.w = images[1]->w / frames;
	src.h = images[1]->h;
	dest = gameGUI.calcScreenPos ((int) (posX + tick * dx), (int) (posY + tick * dy));

	const int alpha = (alphaEnd - alphaStart) * tick / length + alphaStart;
	SDL_SetSurfaceAlphaMod (images[1], alpha);
	SDL_BlitSurface (images[1], &src, cVideo::buffer, &dest);
}
