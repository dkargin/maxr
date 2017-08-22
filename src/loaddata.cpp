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
///////////////////////////////////////////////////////////////////////////////
//
// Loads all relevant files and data at the start of the game.
//
//
///////////////////////////////////////////////////////////////////////////////

#include <SDL_mixer.h>
#include <iostream>
#include <sstream>

#ifdef WIN32

#else
# include <sys/stat.h>
# include <unistd.h>
#endif

#include "loaddata.h"
#include "maxrversion.h"
#include "utility/autosurface.h"
#include "game/data/units/building.h"
#include "game/data/player/clans.h"
#include "extendedtinyxml.h"
#include "utility/files.h"
#include "keys.h"
#include "utility/log.h"
#include "main.h"
#include "pcx.h"
#include "settings.h"
#include "sound.h"
#include "tinyxml2.h"
#include "unifonts.h"
#include "game/data/units/vehicle.h"
#include "video.h"
#include "debug.h"

#ifdef WIN32
# include <shlobj.h>
# include <direct.h>
#endif

using namespace std;
using namespace tinyxml2;

tinyxml2::XMLDocument LanguageFile;

/**
 * Writes a Logmessage on the SplashScreen
 * @param sTxt Text to write
 * @param ok 0 writes just text, 1 writes "OK" and else "ERROR"
 * @param pos Horizontal Positionindex on SplashScreen
 */
static void MakeLog (const std::string& sTxt, int ok, int pos);

/**
 * Loads the selected languagepack
 * @return 1 on success
 */
static int LoadLanguage();

/**
 * Loads all Graphics
 * @param path Directory of the graphics
 * @return 1 on success
 */
static int LoadGraphics (const char* path);

/**
 * Loads the Effects
 * @param path Directory of the Effects
 * @return 1 on success
 */
static int LoadEffects (const char* path);

/**
 * Loads all Buildings
 * @param path Directory of the Buildings
 * @return 1 on success
 */
static int LoadBuildings();

/**
 * Loads all Vehicles
 * @param path Directory of the Vehicles
 * @return 1 on success
 */
static int LoadVehicles();

/**
 * Loads the clan values and stores them in the cUnitData class
 * @return 1 on success
 */
static int LoadClans();

/**
 * Loads all Musicfiles
 * @param path Directory of the Vehicles
 * @return 1 on success
 */
static int LoadMusic (const char* path);

/**
 * Loads the unitdata from the data.xml in the unitfolder
 * @param directory Unitdirectory, relative to the main game directory
 */
static void LoadUnitData(cStaticUnitData& staticData, cDynamicUnitData& dynamicData, char const* const directory, int const iID);

static void LoadUnitGraphicProperties (sVehicleUIData& data, char const* directory);
static void LoadUnitGraphicProperties (sBuildingUIData& data, char const* directory);

// LoadData ///////////////////////////////////////////////////////////////////
// Loads all relevant files and data:
int LoadData (void* data)
{
	CR_ENABLE_CRASH_RPT_CURRENT_THREAD();

	volatile int& loadingState = *static_cast<volatile int*> (data);
	loadingState = LOAD_GOING;

	if (!DEDICATED_SERVER)
	{
		const std::string& fontPath = cSettings::getInstance().getFontPath() + PATH_DELIMITER;
		if (!FileExists ((fontPath + "latin_normal.pcx").c_str())
			|| !FileExists ((fontPath + "latin_big.pcx").c_str())
			|| !FileExists ((fontPath + "latin_big_gold.pcx").c_str())
			|| !FileExists ((fontPath + "latin_small.pcx").c_str()))
		{
			Log.write ("Missing a file needed for game. Check log and config! ", cLog::eLOG_TYPE_ERROR);
			loadingState = LOAD_ERROR;
			return 0;
		}

		font = new cUnicodeFont; // init ascii fonts
		font->setTargetSurface (cVideo::buffer);
		Log.mark();
	}

	string sVersion = PACKAGE_NAME;
	sVersion += " BUILD: ";
	sVersion += MAX_BUILD_DATE; sVersion += " ";
	sVersion += PACKAGE_REV;

	MakeLog (sVersion, 0, 0);

	// Load Languagepack
	MakeLog ("Loading languagepack...", 0, 2);

	string sLang = cSettings::getInstance().getLanguage();
	// FIXME: here is the assumption made
	// that the file always exists with lower cases
	for (int i = 0; i <= 2; i++)
	{
		if (sLang[i] < 'a')
		{
			sLang[i] += 'a' - 'A';
		}
	}
	string sTmpString = cSettings::getInstance().getLangPath();
	sTmpString += PATH_DELIMITER "lang_";
	sTmpString += sLang;
	sTmpString += ".xml";
	Log.write ("Using langfile: " + sTmpString, cLog::eLOG_TYPE_DEBUG);
	if (LoadLanguage() != 1 || !FileExists (sTmpString.c_str()))
	{
		MakeLog ("", -1, 2);
		loadingState = LOAD_ERROR;
		SDL_Delay (5000);
		return -1;
	}
	else
	{
		LanguageFile.LoadFile (sTmpString.c_str());
		MakeLog ("", 1, 2);
	}
	Log.mark();

	// Load Keys
	MakeLog (lngPack.i18n ("Text~Init~Keys"), 0, 3);

	try
	{
		KeysList.loadFromFile();
		MakeLog ("", 1, 3);
	}
	catch (std::runtime_error& e)
	{
		Log.write (e.what(), cLog::eLOG_TYPE_ERROR);
		MakeLog ("", -1, 3);
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	Log.mark();

	// Load Fonts
	MakeLog (lngPack.i18n ("Text~Init~Fonts"), 0, 4);
	// -- little bit crude but fonts are already loaded.
	// what to do with this now? -- beko
	// Really loaded with new cUnicodeFont
	MakeLog ("", 1, 4);
	Log.mark();

	// Load Graphics
	MakeLog (lngPack.i18n ("Text~Init~GFX"), 0, 5);

	if (LoadGraphics (cSettings::getInstance().getGfxPath().c_str()) != 1)
	{
		MakeLog ("", -1, 5);
		Log.write ("Error while loading graphics", cLog::eLOG_TYPE_ERROR);
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	else
	{
		MakeLog ("", 1, 5);
	}
	Log.mark();

	// Load Effects
	MakeLog (lngPack.i18n ("Text~Init~Effects"), 0, 6);

	if (LoadEffects (cSettings::getInstance().getFxPath().c_str()) != 1)
	{
		MakeLog ("", -1, 6);
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	else
	{
		MakeLog ("", 1, 6);
	}
	Log.mark();

	// Load Vehicles
	MakeLog (lngPack.i18n ("Text~Init~Vehicles"), 0, 7);

	if (LoadVehicles() != 1)
	{
		MakeLog ("", -1, 7);
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	else
	{
		MakeLog ("", 1, 7);
	}
	Log.mark();

	// Load Buildings
	MakeLog (lngPack.i18n ("Text~Init~Buildings"), 0, 8);

	if (LoadBuildings() != 1)
	{
		MakeLog ("", -1, 8);
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	else
	{
		MakeLog ("", 1, 8);
	}
	Log.mark();

	MakeLog (lngPack.i18n ("Text~Init~Clans"), 0, 9);

	// Load Clan Settings
	if (LoadClans() != 1)
	{
		SDL_Delay (5000);
		loadingState = LOAD_ERROR;
		return -1;
	}
	else
	{
		MakeLog ("", 1, 9);
	}
	Log.mark();


	if (!DEDICATED_SERVER)
	{
		// Load Music
		MakeLog (lngPack.i18n ("Text~Init~Music"), 0, 10);

		if (LoadMusic (cSettings::getInstance().getMusicPath().c_str()) != 1)
		{
			MakeLog ("", -1, 10);
			SDL_Delay (5000);
			loadingState = LOAD_ERROR;
			return -1;
		}
		else
		{
			MakeLog ("", 1, 10);
		}
		Log.mark();

		// Load Sounds
		MakeLog (lngPack.i18n ("Text~Init~Sounds"), 0, 11);
		Log.write ("Loading Sounds", cLog::eLOG_TYPE_INFO);
		SoundData.load (cSettings::getInstance().getSoundsPath().c_str());
		MakeLog ("", 1, 11);
		Log.mark();

		// Load Voices
		MakeLog (lngPack.i18n ("Text~Init~Voices"), 0, 12);
		Log.write ("Loading Voices", cLog::eLOG_TYPE_INFO);
		VoiceData.load (cSettings::getInstance().getVoicesPath().c_str());
		MakeLog ("", 1, 12);
		Log.mark();
	}

	SDL_Delay (1000);
	loadingState = LOAD_FINISHED;
	return 1;
}

// MakeLog ///////////////////////////////////////////////////////////////////
// Writes a Logmessage on the SplashScreen:
/* static */ void MakeLog (const string& sTxt, int ok, int pos)
{
	if (DEDICATED_SERVER)
	{
		cout << sTxt << endl;
		return;
	}
	const SDL_Rect rDest = {22, 152, 228, Uint16 (font->getFontHeight (FONT_LATIN_BIG_GOLD)) };
	const SDL_Rect rDest2 = {250, 152, 230, Uint16 (font->getFontHeight (FONT_LATIN_BIG_GOLD)) };

	switch (ok)
	{
		case 0:
			font->showText (rDest.x, rDest.y + rDest.h * pos, sTxt, FONT_LATIN_NORMAL);
			break;

		case 1:
			font->showText (rDest2.x, rDest2.y + rDest2.h * pos, "OK", FONT_LATIN_BIG_GOLD);
			break;

		default:
			font->showText (rDest2.x, rDest2.y + rDest2.h * pos, "ERROR ..check maxr.log!", FONT_LATIN_BIG_GOLD);
			break;
	}
	// TODO: Warn that screen has been changed
	// so that Video.draw() can be called in main thread.
}

static AutoSurface CloneSDLSurface (SDL_Surface& src)
{
	return AutoSurface (SDL_ConvertSurface (&src, src.format, src.flags));
}

/**
 * Loads a graphic to the surface
 * @param dest Destination surface
 * @param directory Directory of the file
 * @param filename Name of the file
 * @return 1 on success
 */
static int LoadGraphicToSurface (AutoSurface& dest, const char* directory, const char* filename)
{
	string filepath;
	if (strcmp (directory, ""))
	{
		filepath = directory;
		filepath += PATH_DELIMITER;
	}
	filepath += filename;
	if (!FileExists (filepath.c_str()))
	{
		dest = nullptr;
		Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	dest = LoadPCX (filepath);

	filepath.insert (0, "File loaded: ");
	Log.write (filepath.c_str(), cLog::eLOG_TYPE_DEBUG);

	return 1;
}

/**
 * Loads a effectgraphic to the surface
 * @param dest Destination surface
 * @param directory Directory of the file
 * @param filename Name of the file
 * @return 1 on success
 */
static int LoadEffectGraphicToSurface (AutoSurface (&dest) [2], const char* directory, const char* filename)
{
	string filepath;
	if (strcmp (directory, ""))
	{
		filepath = directory;
		filepath += PATH_DELIMITER;
	}
	filepath += filename;
	if (!FileExists (filepath.c_str()))
	{
		Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	dest[0] = LoadPCX (filepath);
	dest[1] = CloneSDLSurface (*dest[0]);

	filepath.insert (0, "Effect successful loaded: ");
	Log.write (filepath.c_str(), cLog::eLOG_TYPE_DEBUG);

	return 1;
}

// LoadEffectAlphacToSurface /////////////////////////////////////////////////
// Loads a effectgraphic as aplha to the surface:
static int LoadEffectAlphaToSurface (AutoSurface (&dest) [2], const char* directory, const char* filename, int alpha)
{
	string filepath;
	if (strcmp (directory, ""))
	{
		filepath = directory;
		filepath += PATH_DELIMITER;
	}
	filepath += filename;
	if (!FileExists (filepath.c_str()))
		return 0;

	dest[0] = LoadPCX (filepath);
	dest[1] = CloneSDLSurface (*dest[0]);
	SDL_SetSurfaceAlphaMod (dest[0].get(), alpha);
	SDL_SetSurfaceAlphaMod (dest[1].get(), alpha);

	filepath.insert (0, "Effectalpha loaded: ");
	Log.write (filepath.c_str(), cLog::eLOG_TYPE_DEBUG);

	return 1;
}

/**
 * Loads a soundfile to the Mix_Chunk
 * @param dest Destination Mix_Chunk
 * @param directory Directory of the file
 * @param filename Name of the file
 * @param localize When true, sVoiceLanguage is appended to the filename. Used for loading voice files.
 * @return 1 on success
 */
static int LoadSoundfile (cSoundChunk& dest, const char* directory, const char* filename, bool localize = false)
{
	string filepath;
	string fullPath;
	if (strcmp (directory, ""))
	{
		filepath = directory;
		filepath += PATH_DELIMITER;
	}
	//add lang code to voice
	fullPath = filepath + filename;
	if (localize && !cSettings::getInstance().getVoiceLanguage().empty())
	{
		fullPath.insert (fullPath.rfind ("."), "_" + cSettings::getInstance().getVoiceLanguage());
		if (FileExists (fullPath.c_str()))
		{
			dest.load (fullPath);
			return 1;
		}
	}

	//no localized voice file. Try opening without lang code
	fullPath = filepath + filename;
	if (!FileExists (fullPath.c_str()))
		return 0;

	dest.load (fullPath);

	return 1;
}

/**
 * Loads a unitsoundfile to the Mix_Chunk. If the file doesn't exists a dummy file will be loaded
 * @param dest Destination Mix_Chunk
 * @param directory Directory of the file, relative to the main vehicles directory
 * @param filename Name of the file
 */
static void LoadUnitSoundfile (cSoundChunk& dest, const char* directory, const char* filename)
{
	string filepath;
	if (strcmp (directory, ""))
		filepath += directory;
	filepath += filename;
	if (SoundData.DummySound.empty())
	{
		string sTmpString;
		sTmpString = cSettings::getInstance().getSoundsPath() + PATH_DELIMITER + "dummy.ogg";
		if (FileExists (sTmpString.c_str()))
		{
			try
			{
				SoundData.DummySound.load (sTmpString);
			}
			catch (std::runtime_error& e)
			{
				Log.write (std::string ("Can't load dummy.ogg: ") + e.what(), cLog::eLOG_TYPE_WARNING);
			}
		}
	}
	// Not using FileExists to avoid unnecessary warnings in log file
	SDL_RWops* file = SDL_RWFromFile (filepath.c_str(), "r");
	if (!file)
	{
		//dest = SoundData.DummySound;
		return;
	}
	SDL_RWclose (file);

	dest.load (filepath);
}

static int LoadLanguage()
{
	// Set the language code
	if (lngPack.SetCurrentLanguage (cSettings::getInstance().getLanguage()) != 0)
	{
		// Not a valid language code, critical fail!
		Log.write ("Not a valid language code!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	if (lngPack.ReadLanguagePack() != 0) // Load the translations
	{
		// Could not load the language, critical fail!
		Log.write ("Could not load the language!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	return 1;
}

static int LoadEffects (const char* path)
{
	Log.write ("Loading Effects", cLog::eLOG_TYPE_INFO);

	if (DEDICATED_SERVER) return 1;

	EffectsData.load (path);
	return 1;
}

void cEffectsData::load (const char* path)
{
	LoadEffectGraphicToSurface (fx_explo_small, path, "explo_small.pcx");
	LoadEffectGraphicToSurface (fx_explo_big, path, "explo_big.pcx");
	LoadEffectGraphicToSurface (fx_explo_water, path, "explo_water.pcx");
	LoadEffectGraphicToSurface (fx_explo_air, path, "explo_air.pcx");
	LoadEffectGraphicToSurface (fx_muzzle_big, path, "muzzle_big.pcx");
	LoadEffectGraphicToSurface (fx_muzzle_small, path, "muzzle_small.pcx");
	LoadEffectGraphicToSurface (fx_muzzle_med, path, "muzzle_med.pcx");
	LoadEffectGraphicToSurface (fx_hit, path, "hit.pcx");
	LoadEffectAlphaToSurface (fx_smoke, path, "smoke.pcx", 100);
	LoadEffectGraphicToSurface (fx_rocket, path, "rocket.pcx");
	LoadEffectAlphaToSurface (fx_dark_smoke, path, "dark_smoke.pcx", 100);
	LoadEffectAlphaToSurface (fx_tracks, path, "tracks.pcx", 100);
	LoadEffectAlphaToSurface (fx_corpse, path, "corpse.pcx", 254);
	LoadEffectAlphaToSurface (fx_absorb, path, "absorb.pcx", 150);
}

static int LoadMusic (const char* path)
{
	Log.write ("Loading music", cLog::eLOG_TYPE_INFO);

	// Prepare music.xml for reading
	tinyxml2::XMLDocument MusicXml;
	string sTmpString = path;
	sTmpString += PATH_DELIMITER "music.xml";
	if (!FileExists (sTmpString.c_str()))
	{
		return 0;
	}
	if (MusicXml.LoadFile (sTmpString.c_str()) != XML_NO_ERROR)
	{
		Log.write ("Can't load music.xml ", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	XMLElement* xmlElement;
#if 0 // unused
	xmlElement = XmlGetFirstElement (MusicXml, "Music", "Menus", "main", nullptr);
	if (!xmlElement || !xmlElement->Attribute ("Text"))
	{
		Log.write ("Can't find \"main\" in music.xml ", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	MainMusicFile = xmlElement->Attribute ("Text");

	xmlElement = XmlGetFirstElement (MusicXml, "Music", "Menus", "credits", nullptr);
	if (!xmlElement || !xmlElement->Attribute ("Text"))
	{
		Log.write ("Can't find \"credits\" in music.xml ", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	CreditsMusicFile = xmlElement->Attribute ("Text");
#endif
	xmlElement = XmlGetFirstElement (MusicXml, "Music", "Game", "bkgcount", nullptr);
	if (!xmlElement || !xmlElement->Attribute ("Num"))
	{
		Log.write ("Can't find \"bkgcount\" in music.xml ", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	int const MusicAnz = xmlElement->IntAttribute ("Num");
	for (int i = 1; i <= MusicAnz; i++)
	{
		std::string name = "bkg" + iToStr (i);
		XMLElement* xmlElement = XmlGetFirstElement (MusicXml, "Music", "Game", name.c_str(), nullptr);
		if (xmlElement && xmlElement->Attribute ("Text"))
		{
			name = string (path) + PATH_DELIMITER + xmlElement->Attribute ("Text");
		}
		else
		{
			Log.write ("Can't find \"bkg" + iToStr (i) + "\" in music.xml", cLog::eLOG_TYPE_WARNING);
			continue;
		}
		if (!FileExists (name.c_str()))
			continue;
		MusicFiles.push_back (name);
	}
	return 1;
}

void cSoundData::load (const char* path)
{
	LoadSoundfile (SNDHudSwitch, path, "HudSwitch.ogg");
	LoadSoundfile (SNDHudButton, path, "HudButton.ogg");
	LoadSoundfile (SNDMenuButton, path, "MenuButton.ogg");
	LoadSoundfile (SNDChat, path, "Chat.ogg");
	LoadSoundfile (SNDObjectMenu, path, "ObjectMenu.ogg");
	LoadSoundfile (EXPBigWet[0], path, "exp_big_wet0.ogg");
	LoadSoundfile (EXPBigWet[1], path, "exp_big_wet1.ogg");
	LoadSoundfile (EXPBig[0], path, "exp_big0.ogg");
	LoadSoundfile (EXPBig[1], path, "exp_big1.ogg");
	LoadSoundfile (EXPBig[2], path, "exp_big2.ogg");
	LoadSoundfile (EXPBig[3], path, "exp_big3.ogg");
	LoadSoundfile (EXPSmallWet[0], path, "exp_small_wet0.ogg");
	LoadSoundfile (EXPSmallWet[1], path, "exp_small_wet1.ogg");
	LoadSoundfile (EXPSmallWet[2], path, "exp_small_wet2.ogg");
	LoadSoundfile (EXPSmall[0], path, "exp_small0.ogg");
	LoadSoundfile (EXPSmall[1], path, "exp_small1.ogg");
	LoadSoundfile (EXPSmall[2], path, "exp_small2.ogg");
	LoadSoundfile (SNDArm, path, "arm.ogg");
	LoadSoundfile (SNDBuilding, path, "building.ogg");
	LoadSoundfile (SNDClearing, path, "clearing.ogg");
	LoadSoundfile (SNDQuitsch, path, "quitsch.ogg");
	LoadSoundfile (SNDActivate, path, "activate.ogg");
	LoadSoundfile (SNDLoad, path, "load.ogg");
	LoadSoundfile (SNDReload, path, "reload.ogg");
	LoadSoundfile (SNDRepair, path, "repair.ogg");
	LoadSoundfile (SNDLandMinePlace, path, "land_mine_place.ogg");
	LoadSoundfile (SNDLandMineClear, path, "land_mine_clear.ogg");
	LoadSoundfile (SNDSeaMinePlace, path, "sea_mine_place.ogg");
	LoadSoundfile (SNDSeaMineClear, path, "sea_mine_clear.ogg");
	LoadSoundfile (SNDPanelOpen, path, "panel_open.ogg");
	LoadSoundfile (SNDPanelClose, path, "panel_close.ogg");
	LoadSoundfile (SNDAbsorb, path, "absorb.ogg");
	LoadSoundfile (SNDHitSmall, path, "hit_small.ogg");
	LoadSoundfile (SNDHitMed, path, "hit_med.ogg");
	LoadSoundfile (SNDHitLarge, path, "hit_large.ogg");
}

void cVoiceData::load (const char* path)
{
	LoadSoundfile (VOIAmmoLow[0], path, "ammo_low1.ogg", true);
	LoadSoundfile (VOIAmmoLow[1], path, "ammo_low2.ogg", true);
	LoadSoundfile (VOIAmmoEmpty[0], path, "ammo_empty1.ogg", true);
	LoadSoundfile (VOIAmmoEmpty[1], path, "ammo_empty2.ogg", true);
	LoadSoundfile (VOIAttacking[0], path, "attacking1.ogg", true);
	LoadSoundfile (VOIAttacking[1], path, "attacking2.ogg", true);
	LoadSoundfile (VOIAttackingEnemy[0], path, "attacking_enemy1.ogg", true);
	LoadSoundfile (VOIAttackingEnemy[1], path, "attacking_enemy2.ogg", true);
	LoadSoundfile (VOIAttackingUs[0], path, "attacking_us.ogg", true);
	LoadSoundfile (VOIAttackingUs[1], path, "attacking_us2.ogg", true);
	LoadSoundfile (VOIAttackingUs[2], path, "attacking_us3.ogg", true);
	LoadSoundfile (VOIBuildDone[0], path, "build_done1.ogg", true);
	LoadSoundfile (VOIBuildDone[1], path, "build_done2.ogg", true);
	LoadSoundfile (VOIBuildDone[2], path, "build_done3.ogg", true);
	LoadSoundfile (VOIBuildDone[3], path, "build_done4.ogg", true);
	LoadSoundfile (VOIClearing, path, "clearing.ogg", true);
	LoadSoundfile (VOIClearingMines[0], path, "clearing_mines.ogg", true);
	LoadSoundfile (VOIClearingMines[1], path, "clearing_mines2.ogg", true);
	LoadSoundfile (VOICommandoFailed[0], path, "commando_failed1.ogg", true);
	LoadSoundfile (VOICommandoFailed[1], path, "commando_failed2.ogg", true);
	LoadSoundfile (VOICommandoFailed[2], path, "commando_failed3.ogg", true);
	LoadSoundfile (VOIDestroyedUs[0], path, "destroyed_us1.ogg", true);
	LoadSoundfile (VOIDestroyedUs[1], path, "destroyed_us2.ogg", true);
	LoadSoundfile (VOIDetected[0], path, "detected1.ogg", true);
	LoadSoundfile (VOIDetected[1], path, "detected2.ogg", true);
	LoadSoundfile (VOILanding[0], path, "landing1.ogg", true);
	LoadSoundfile (VOILanding[1], path, "landing2.ogg", true);
	LoadSoundfile (VOILanding[2], path, "landing3.ogg", true);
	LoadSoundfile (VOILayingMines, path, "laying_mines.ogg", true);
	LoadSoundfile (VOINoPath[0], path, "no_path1.ogg", true);
	LoadSoundfile (VOINoPath[1], path, "no_path2.ogg", true);
	LoadSoundfile (VOINoSpeed, path, "no_speed.ogg", true);
	LoadSoundfile (VOIOK[0], path, "ok1.ogg", true);
	LoadSoundfile (VOIOK[1], path, "ok2.ogg", true);
	LoadSoundfile (VOIOK[2], path, "ok3.ogg", true);
	LoadSoundfile (VOIOK[3], path, "ok4.ogg", true);
	LoadSoundfile (VOIReammo, path, "reammo.ogg", true);
	LoadSoundfile (VOIReammoAll, path, "reammo_all.ogg", true);
	LoadSoundfile (VOIRepaired[0], path, "repaired.ogg", true);
	LoadSoundfile (VOIRepaired[1], path, "repaired2.ogg", true);
	LoadSoundfile (VOIRepairedAll[0], path, "repaired_all1.ogg", true);
	LoadSoundfile (VOIRepairedAll[1], path, "repaired_all2.ogg", true);
	LoadSoundfile (VOIResearchComplete, path, "research_complete.ogg", true);
	LoadSoundfile (VOISaved, path, "saved.ogg", true);
	LoadSoundfile (VOISentry, path, "sentry.ogg", true);
	LoadSoundfile (VOIStartMore, path, "start_more.ogg", true);
	LoadSoundfile (VOIStartNone, path, "start_none.ogg", true);
	LoadSoundfile (VOIStartOne, path, "start_one.ogg", true);
	LoadSoundfile (VOIStatusRed[0], path, "status_red1.ogg", true);
	LoadSoundfile (VOIStatusRed[1], path, "status_red2.ogg", true);
	LoadSoundfile (VOIStatusYellow[0], path, "status_yellow1.ogg", true);
	LoadSoundfile (VOIStatusYellow[1], path, "status_yellow2.ogg", true);
	LoadSoundfile (VOISubDetected, path, "sub_detected.ogg", true);
	LoadSoundfile (VOISurveying[0], path, "surveying.ogg", true);
	LoadSoundfile (VOISurveying[1], path, "surveying2.ogg", true);
	LoadSoundfile (VOITransferDone, path, "transfer_done.ogg", true);
	LoadSoundfile (VOITurnEnd20Sec[0], path, "turn_end_20_sec1.ogg", true);
	LoadSoundfile (VOITurnEnd20Sec[1], path, "turn_end_20_sec2.ogg", true);
	LoadSoundfile (VOITurnEnd20Sec[2], path, "turn_end_20_sec3.ogg", true);
	LoadSoundfile (VOIUnitDisabled, path, "unit_disabled.ogg", true);
	LoadSoundfile (VOIUnitDisabledByEnemy[0], path, "unit_disabled_by_enemy1.ogg", true);
	LoadSoundfile (VOIUnitDisabledByEnemy[1], path, "unit_disabled_by_enemy2.ogg", true);
	LoadSoundfile (VOIUnitStolen[0], path, "unit_stolen1.ogg", true);
	LoadSoundfile (VOIUnitStolen[1], path, "unit_stolen2.ogg", true);
	LoadSoundfile (VOIUnitStolenByEnemy, path, "unit_stolen_by_enemy.ogg", true);
}

static int LoadGraphics (const char* path)
{
	Log.write ("Loading Graphics", cLog::eLOG_TYPE_INFO);
	if (DEDICATED_SERVER) return 1;

	Log.write ("Gamegraphics...", cLog::eLOG_TYPE_DEBUG);
	if (!LoadGraphicToSurface (GraphicsData.gfx_Chand, path, "hand.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cno, path, "no.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cselect, path, "select.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cmove, path, "move.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cmove_draft, path, "move_draft.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Chelp, path, "help.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Ctransf, path, "transf.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cload, path, "load.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cmuni, path, "muni.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cband, path, "band_cur.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cactivate, path, "activate.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Crepair, path, "repair.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Csteal, path, "steal.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cdisable, path, "disable.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cattack, path, "attack.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_Cattackoor, path, "attack_oor.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_hud_stuff, path, "hud_stuff.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_hud_extra_players, path, "hud_extra_players.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_panel_top, path, "panel_top.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_panel_bottom, path, "panel_bottom.pcx") ||
		!LoadGraphicToSurface (GraphicsData.gfx_menu_stuff, path, "menu_stuff.pcx"))
	{
		return 0;
	}
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil1, path, "pf_1.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil2, path, "pf_2.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil3, path, "pf_3.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil4, path, "pf_4.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil6, path, "pf_6.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil7, path, "pf_7.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil8, path, "pf_8.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_Cpfeil9, path, "pf_9.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_context_menu, path, "object_menu2.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_destruction, path, "destruction.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_band_small_org, path, "band_small.pcx");
	GraphicsData.gfx_band_small = CloneSDLSurface (*GraphicsData.gfx_band_small_org);
	LoadGraphicToSurface (GraphicsData.gfx_band_big_org, path, "band_big.pcx");
	GraphicsData.gfx_band_big = CloneSDLSurface (*GraphicsData.gfx_band_big_org);
	LoadGraphicToSurface (GraphicsData.gfx_big_beton_org, path, "big_beton.pcx");
	GraphicsData.gfx_big_beton = CloneSDLSurface (*GraphicsData.gfx_big_beton_org);
	LoadGraphicToSurface (GraphicsData.gfx_storage, path, "storage.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_storage_ground, path, "storage_ground.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_dialog, path, "dialog.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_edock, path, "edock.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_edepot, path, "edepot.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_ehangar, path, "ehangar.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_player_pc, path, "player_pc.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_player_human, path, "player_human.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_player_none, path, "player_none.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_exitpoints_org, path, "activate_field.pcx");
	GraphicsData.gfx_exitpoints = CloneSDLSurface (*GraphicsData.gfx_exitpoints_org);
	LoadGraphicToSurface (GraphicsData.gfx_player_select, path, "customgame_menu.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_menu_buttons, path, "menu_buttons.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_player_ready, path, "player_ready.pcx");
	LoadGraphicToSurface (GraphicsData.gfx_hud_chatbox, path, "hud_chatbox.pcx");

	GraphicsData.DialogPath = cSettings::getInstance().getGfxPath() + PATH_DELIMITER + "dialog.pcx";
	GraphicsData.Dialog2Path = cSettings::getInstance().getGfxPath() + PATH_DELIMITER + "dialog2.pcx";
	GraphicsData.Dialog3Path = cSettings::getInstance().getGfxPath() + PATH_DELIMITER + "dialog3.pcx";
	FileExists (GraphicsData.DialogPath.c_str());
	FileExists (GraphicsData.Dialog2Path.c_str());
	FileExists (GraphicsData.Dialog3Path.c_str());

	Log.write ("Shadowgraphics...", cLog::eLOG_TYPE_DEBUG);
	// Shadow:
	createShadowGfx();
	Video.resolutionChanged.connect ([]()
	{
		createShadowGfx();
	});

	GraphicsData.gfx_tmp = AutoSurface (SDL_CreateRGBSurface (0, 128, 128, Video.getColDepth(), 0, 0, 0, 0));
	SDL_SetColorKey (GraphicsData.gfx_tmp.get(), SDL_TRUE, 0xFF00FF);

	// Glas:
	Log.write ("Glassgraphic...", cLog::eLOG_TYPE_DEBUG);
	LoadGraphicToSurface (GraphicsData.gfx_destruction_glas, path, "destruction_glas.pcx");
	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_destruction_glas.get(), 150);

	// Waypoints:
	Log.write ("Waypointgraphics...", cLog::eLOG_TYPE_DEBUG);
	OtherData.loadWayPoints();

	// Resources:
	Log.write ("Resourcegraphics...", cLog::eLOG_TYPE_DEBUG);
	ResourceData.load (path);
	return 1;
}

void cOtherData::loadWayPoints()
{
	for (int i = 0; i < 60; i++)
	{
		WayPointPfeile[0][i] = CreatePfeil (26, 11, 51, 36, 14, 48, PFEIL_COLOR, 64 - i);
		WayPointPfeile[1][i] = CreatePfeil (14, 14, 49, 14, 31, 49, PFEIL_COLOR, 64 - i);
		WayPointPfeile[2][i] = CreatePfeil (37, 11, 12, 36, 49, 48, PFEIL_COLOR, 64 - i);
		WayPointPfeile[3][i] = CreatePfeil (49, 14, 49, 49, 14, 31, PFEIL_COLOR, 64 - i);
		WayPointPfeile[4][i] = CreatePfeil (14, 14, 14, 49, 49, 31, PFEIL_COLOR, 64 - i);
		WayPointPfeile[5][i] = CreatePfeil (15, 14, 52, 26, 27, 51, PFEIL_COLOR, 64 - i);
		WayPointPfeile[6][i] = CreatePfeil (31, 14, 14, 49, 49, 49, PFEIL_COLOR, 64 - i);
		WayPointPfeile[7][i] = CreatePfeil (48, 14, 36, 51, 11, 26, PFEIL_COLOR, 64 - i);

		WayPointPfeileSpecial[0][i] = CreatePfeil (26, 11, 51, 36, 14, 48, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[1][i] = CreatePfeil (14, 14, 49, 14, 31, 49, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[2][i] = CreatePfeil (37, 11, 12, 36, 49, 48, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[3][i] = CreatePfeil (49, 14, 49, 49, 14, 31, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[4][i] = CreatePfeil (14, 14, 14, 49, 49, 31, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[5][i] = CreatePfeil (15, 14, 52, 26, 27, 51, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[6][i] = CreatePfeil (31, 14, 14, 49, 49, 49, PFEILS_COLOR, 64 - i);
		WayPointPfeileSpecial[7][i] = CreatePfeil (48, 14, 36, 51, 11, 26, PFEILS_COLOR, 64 - i);
	}
}

void cResourceData::load (const char* path)
{
	// metal
	if (LoadGraphicToSurface (res_metal_org, path, "res.pcx") == 1)
	{
		res_metal = CloneSDLSurface (*res_metal_org);
		SDL_SetColorKey (res_metal.get(), SDL_TRUE, 0xFF00FF);
	}

	// gold
	if (LoadGraphicToSurface (res_gold_org, path, "gold.pcx") == 1)
	{
		res_gold = CloneSDLSurface (*res_gold_org);
		SDL_SetColorKey (res_gold.get(), SDL_TRUE, 0xFF00FF);
	}

	// fuel
	if (LoadGraphicToSurface (res_oil_org, path, "fuel.pcx") == 1)
	{
		res_oil = CloneSDLSurface (*res_oil_org);
		SDL_SetColorKey (res_oil.get(), SDL_TRUE, 0xFF00FF);
	}
}

static int LoadVehicles()
{
	Log.write ("Loading Vehicles", cLog::eLOG_TYPE_INFO);

	tinyxml2::XMLDocument VehiclesXml;

	string sTmpString = cSettings::getInstance().getVehiclesPath();
	sTmpString += PATH_DELIMITER "vehicles.xml";
	if (!FileExists (sTmpString.c_str()))
	{
		return 0;
	}
	if (VehiclesXml.LoadFile (sTmpString.c_str()) != XML_NO_ERROR)
	{
		Log.write ("Can't load vehicles.xml!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	XMLElement* xmlElement = XmlGetFirstElement (VehiclesXml, "VehicleData", "Vehicles", nullptr);
	if (xmlElement == nullptr)
	{
		Log.write ("Can't read \"VehicleData->Vehicles\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	// read vehicles.xml
	std::vector<std::string> VehicleList;
	std::vector<int> IDList;
	xmlElement = xmlElement->FirstChildElement();
	if (xmlElement)
	{
		const char* directory = xmlElement->Attribute ("directory");
		if (directory != nullptr)
			VehicleList.push_back (directory);
		else
		{
			string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		if (xmlElement->Attribute ("num"))
			IDList.push_back (xmlElement->IntAttribute ("num"));
		else
		{
			string msg = string ("Can't read num-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}
	}
	else
		Log.write ("No vehicles defined in vehicles.xml!", cLog::eLOG_TYPE_WARNING);
	while (xmlElement != nullptr)
	{
		xmlElement = xmlElement->NextSiblingElement();
		if (xmlElement == nullptr)
			break;

		const char* directory = xmlElement->Attribute ("directory");
		if (directory != nullptr)
			VehicleList.push_back (directory);
		else
		{
			string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		if (xmlElement->Attribute ("num"))
			IDList.push_back (xmlElement->IntAttribute ("num"));
		else
		{
			string msg = string ("Can't read num-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}
	}
	// load found units
	string sVehiclePath;
	UnitsUiData.vehicleUIs.resize(VehicleList.size());
	for (size_t i = 0; i != VehicleList.size(); ++i)
	{
		sVehiclePath = cSettings::getInstance().getVehiclesPath();
		sVehiclePath += PATH_DELIMITER;
		sVehiclePath += VehicleList[i];
		sVehiclePath += PATH_DELIMITER;

		cStaticUnitData staticData;
		cDynamicUnitData dynamicData;
		sVehicleUIData& ui = UnitsUiData.vehicleUIs[i];

		Log.write ("Reading values from XML", cLog::eLOG_TYPE_DEBUG);
		LoadUnitData (staticData, dynamicData, sVehiclePath.c_str(), IDList[i]);

		// load graphics.xml
		LoadUnitGraphicProperties(ui, sVehiclePath.c_str());

		ui.id = staticData.ID;

		if (DEDICATED_SERVER) continue;

		Log.write ("Loading graphics", cLog::eLOG_TYPE_DEBUG);
		// load infantery graphics
		if (ui.animationMovement)
		{
			SDL_Rect rcDest;
			for (int n = 0; n < 8; n++)
			{
				ui.img[n] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.img[n].get(), SDL_TRUE, 0x00FFFFFF);
				SDL_FillRect (ui.img[n].get(), nullptr, 0x00FF00FF);

				for (int j = 0; j < 13; j++)
				{
					sTmpString = sVehiclePath;
					char sztmp[16];
					TIXML_SNPRINTF (sztmp, sizeof (sztmp), "img%d_%.2d.pcx", n, j);
					sTmpString += sztmp;

					if (FileExists (sTmpString.c_str()))
					{
						AutoSurface sfTempSurface (LoadPCX (sTmpString));
						if (!sfTempSurface)
						{
							Log.write (SDL_GetError(), cLog::eLOG_TYPE_WARNING);
						}
						else
						{
							rcDest.x = 64 * j + 32 - sfTempSurface->w / 2;
							rcDest.y = 32 - sfTempSurface->h / 2;
							SDL_BlitSurface (sfTempSurface.get(), nullptr, ui.img[n].get(), &rcDest);
						}
					}
				}
				ui.img_org[n] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.img[n].get(), SDL_TRUE, 0x00FFFFFF);
				SDL_FillRect (ui.img_org[n].get(), nullptr, 0x00FFFFFF);
				SDL_BlitSurface (ui.img[n].get(), nullptr, ui.img_org[n].get(), nullptr);

				ui.shw[n] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.shw[n].get(), SDL_TRUE, 0x00FF00FF);
				SDL_FillRect (ui.shw[n].get(), nullptr, 0x00FF00FF);
				ui.shw_org[n] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.shw_org[n].get(), SDL_TRUE, 0x00FF00FF);
				SDL_FillRect (ui.shw_org[n].get(), nullptr, 0x00FF00FF);

				rcDest.x = 3;
				rcDest.y = 3;
				SDL_BlitSurface (ui.img_org[n].get(), nullptr, ui.shw_org[n].get(), &rcDest);
				SDL_LockSurface (ui.shw_org[n].get());
				Uint32* ptr = static_cast<Uint32*> (ui.shw_org[n]->pixels);
				for (int j = 0; j < 64 * 13 * 64; j++)
				{
					if (*ptr != 0x00FF00FF)
						*ptr = 0;
					ptr++;
				}
				SDL_UnlockSurface (ui.shw_org[n].get());
				SDL_BlitSurface (ui.shw_org[n].get(), nullptr, ui.shw[n].get(), nullptr);
				SDL_SetSurfaceAlphaMod (ui.shw_org[n].get(), 50);
				SDL_SetSurfaceAlphaMod (ui.shw[n].get(), 50);
			}
		}
		// load other vehicle graphics
		else
		{
			for (int n = 0; n < 8; n++)
			{
				// load image
				sTmpString = sVehiclePath;
				char sztmp[16];
				TIXML_SNPRINTF (sztmp, sizeof (sztmp), "img%d.pcx", n);
				sTmpString += sztmp;
				Log.write (sTmpString, cLog::eLOG_TYPE_DEBUG);
				if (FileExists (sTmpString.c_str()))
				{
					ui.img_org[n] = LoadPCX (sTmpString);
					ui.img[n] = CloneSDLSurface (*ui.img_org[n]);
					SDL_SetColorKey (ui.img_org[n].get(), SDL_TRUE, 0xFFFFFF);
					SDL_SetColorKey (ui.img[n].get(), SDL_TRUE, 0xFFFFFF);
				}
				else
				{
					Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
					return -1;
				}

				// load shadow
				sTmpString.replace (sTmpString.length() - 8, 3, "shw");
				if (FileExists (sTmpString.c_str()))
				{
					ui.shw_org[n] = LoadPCX (sTmpString);
					ui.shw[n] = CloneSDLSurface (*ui.shw_org[n]);
					SDL_SetSurfaceAlphaMod (ui.shw[n].get(), 50);
				}
				else
				{
					ui.shw_org[n] = nullptr;
					ui.shw[n]     = nullptr;
				}
			}
		}
		// load video
		ui.FLCFile = sVehiclePath;
		ui.FLCFile += "video.flc";
		Log.write ("Loading video " + ui.FLCFile, cLog::eLOG_TYPE_DEBUG);
		if (!FileExists (ui.FLCFile.c_str()))
		{
			ui.FLCFile = "";
		}

		// load infoimage
		sTmpString = sVehiclePath;
		sTmpString += "info.pcx";
		Log.write ("Loading portrait" + sTmpString, cLog::eLOG_TYPE_DEBUG);
		if (FileExists (sTmpString.c_str()))
		{
			ui.info = LoadPCX (sTmpString);
		}
		else
		{
			Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
			return -1;
		}

		// load storageimage
		sTmpString = sVehiclePath;
		sTmpString += "store.pcx";
		Log.write ("Loading storageportrait" + sTmpString, cLog::eLOG_TYPE_DEBUG);
		if (FileExists (sTmpString.c_str()))
		{
			ui.storage = LoadPCX (sTmpString);
		}
		else
		{
			Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
			return -1;
		}

		// load overlaygraphics if necessary
		Log.write ("Loading overlay", cLog::eLOG_TYPE_DEBUG);
		if (ui.hasOverlay)
		{
			sTmpString = sVehiclePath;
			sTmpString += "overlay.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.overlay_org = LoadPCX (sTmpString);
				ui.overlay = CloneSDLSurface (*ui.overlay_org);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.overlay_org       = nullptr;
				ui.overlay           = nullptr;
				ui.hasOverlay = false;
			}
		}
		else
		{
			ui.overlay_org = nullptr;
			ui.overlay     = nullptr;
		}

		// load buildgraphics if necessary
		Log.write ("Loading buildgraphics", cLog::eLOG_TYPE_DEBUG);
		if (ui.buildUpGraphic)
		{
			// load image
			sTmpString = sVehiclePath;
			sTmpString += "build.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.build_org = LoadPCX (sTmpString);
				ui.build = CloneSDLSurface (*ui.build_org);
				SDL_SetColorKey (ui.build_org.get(), SDL_TRUE, 0xFFFFFF);
				SDL_SetColorKey (ui.build.get(), SDL_TRUE, 0xFFFFFF);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.build_org             = nullptr;
				ui.build                 = nullptr;
				ui.buildUpGraphic = false;
			}
			// load shadow
			sTmpString = sVehiclePath;
			sTmpString += "build_shw.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.build_shw_org = LoadPCX (sTmpString);
				ui.build_shw = CloneSDLSurface (*ui.build_shw_org);
				SDL_SetSurfaceAlphaMod (ui.build_shw.get(), 50);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.build_shw_org         = nullptr;
				ui.build_shw             = nullptr;
				ui.buildUpGraphic = false;
			}
		}
		else
		{
			ui.build_org     = nullptr;
			ui.build         = nullptr;
			ui.build_shw_org = nullptr;
			ui.build_shw     = nullptr;
		}
		// load cleargraphics if necessary
		Log.write ("Loading cleargraphics", cLog::eLOG_TYPE_DEBUG);
		if (staticData.canClearArea)
		{
			// load image (small)
			sTmpString = sVehiclePath;
			sTmpString += "clear_small.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.clear_small_org = LoadPCX (sTmpString);
				ui.clear_small = CloneSDLSurface (*ui.clear_small_org);
				SDL_SetColorKey (ui.clear_small_org.get(), SDL_TRUE, 0xFFFFFF);
				SDL_SetColorKey (ui.clear_small.get(), SDL_TRUE, 0xFFFFFF);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.clear_small_org      = nullptr;
				ui.clear_small          = nullptr;
				staticData.canClearArea = false;
			}
			// load shadow (small)
			sTmpString = sVehiclePath;
			sTmpString += "clear_small_shw.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.clear_small_shw_org = LoadPCX (sTmpString);
				ui.clear_small_shw = CloneSDLSurface (*ui.clear_small_shw_org);
				SDL_SetSurfaceAlphaMod (ui.clear_small_shw.get(), 50);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.clear_small_shw_org  = nullptr;
				ui.clear_small_shw      = nullptr;
				staticData.canClearArea = false;
			}
			// load image (big)
			sTmpString = sVehiclePath;
			sTmpString += "clear_big.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.build_org = LoadPCX (sTmpString);
				ui.build = CloneSDLSurface (*ui.build_org);
				SDL_SetColorKey (ui.build_org.get(), SDL_TRUE, 0xFFFFFF);
				SDL_SetColorKey (ui.build.get(), SDL_TRUE, 0xFFFFFF);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.build_org            = nullptr;
				ui.build                = nullptr;
				staticData.canClearArea = false;
			}
			// load shadow (big)
			sTmpString = sVehiclePath;
			sTmpString += "clear_big_shw.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.build_shw_org = LoadPCX (sTmpString);
				ui.build_shw = CloneSDLSurface (*ui.build_shw_org);
				SDL_SetSurfaceAlphaMod (ui.build_shw.get(), 50);
			}
			else
			{
				Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_WARNING);
				ui.build_shw_org        = nullptr;
				ui.build_shw            = nullptr;
				staticData.canClearArea = false;
			}
		}
		else
		{
			ui.clear_small_org     = nullptr;
			ui.clear_small         = nullptr;
			ui.clear_small_shw_org = nullptr;
			ui.clear_small_shw     = nullptr;
		}

		// load sounds
		Log.write ("Loading sounds", cLog::eLOG_TYPE_DEBUG);
		LoadUnitSoundfile (ui.Wait,       sVehiclePath.c_str(), "wait.ogg");
		LoadUnitSoundfile (ui.WaitWater,  sVehiclePath.c_str(), "wait_water.ogg");
		LoadUnitSoundfile (ui.Start,      sVehiclePath.c_str(), "start.ogg");
		LoadUnitSoundfile (ui.StartWater, sVehiclePath.c_str(), "start_water.ogg");
		LoadUnitSoundfile (ui.Stop,       sVehiclePath.c_str(), "stop.ogg");
		LoadUnitSoundfile (ui.StopWater,  sVehiclePath.c_str(), "stop_water.ogg");
		LoadUnitSoundfile (ui.Drive,      sVehiclePath.c_str(), "drive.ogg");
		LoadUnitSoundfile (ui.DriveWater, sVehiclePath.c_str(), "drive_water.ogg");
		LoadUnitSoundfile (ui.Attack,     sVehiclePath.c_str(), "attack.ogg");

		UnitsDataGlobal.addData(staticData);
		UnitsDataGlobal.addData(dynamicData);
	}
	

	UnitsDataGlobal.initializeIDData();

	return 1;
}

static int LoadBuildings()
{
	Log.write ("Loading Buildings", cLog::eLOG_TYPE_INFO);

	// read buildings.xml
	string sTmpString = cSettings::getInstance().getBuildingsPath();
	sTmpString += PATH_DELIMITER "buildings.xml";
	if (!FileExists (sTmpString.c_str()))
	{
		return 0;
	}

	tinyxml2::XMLDocument BuildingsXml;
	if (BuildingsXml.LoadFile (sTmpString.c_str()) != XML_NO_ERROR)
	{
		Log.write ("Can't load buildings.xml!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	XMLElement* xmlElement = XmlGetFirstElement (BuildingsXml, "BuildingsData", "Buildings", nullptr);
	if (xmlElement == nullptr)
	{
		Log.write ("Can't read \"BuildingData->Building\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	std::vector<std::string> BuildingList;
	std::vector<int> IDList;
	xmlElement = xmlElement->FirstChildElement();
	if (xmlElement == nullptr)
	{
		Log.write ("There are no buildings in the buildings.xml defined", cLog::eLOG_TYPE_ERROR);
		return 1;
	}

	const char* directory = xmlElement->Attribute ("directory");
	if (directory != nullptr)
		BuildingList.push_back (directory);
	else
	{
		string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
		Log.write (msg, cLog::eLOG_TYPE_WARNING);
	}

	if (xmlElement->Attribute ("num"))
		IDList.push_back (xmlElement->IntAttribute ("num"));
	else
	{
		string msg = string ("Can't read num-attribute from \"") + xmlElement->Value() + "\" - node";
		Log.write (msg, cLog::eLOG_TYPE_WARNING);
	}

	const char* spezial = xmlElement->Attribute ("special");
	if (spezial != nullptr)
	{
		string specialString = spezial;
		if (specialString == "mine")            UnitsDataGlobal.setSpecialIDMine(sID(1, IDList.back()));
		else if (specialString == "energy")     UnitsDataGlobal.setSpecialIDSmallGen(sID(1, IDList.back()));
		else if (specialString == "connector")  UnitsDataGlobal.setSpecialIDConnector(sID(1, IDList.back()));
		else if (specialString == "landmine")   UnitsDataGlobal.setSpecialIDLandMine(sID(1, IDList.back()));
		else if (specialString == "seamine")    UnitsDataGlobal.setSpecialIDSeaMine(sID(1, IDList.back()));
		else if (specialString == "smallBeton") UnitsDataGlobal.setSpecialIDSmallBeton(sID(1, IDList.back()));
		else Log.write ("Unknown spacial in buildings.xml \"" + specialString + "\"", cLog::eLOG_TYPE_WARNING);
	}

	while (xmlElement != nullptr)
	{
		xmlElement = xmlElement->NextSiblingElement();
		if (xmlElement == nullptr)
			break;

		const char* directory = xmlElement->Attribute ("directory");
		if (directory != nullptr)
			BuildingList.push_back (directory);
		else
		{
			string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		if (xmlElement->Attribute ("num"))
			IDList.push_back (xmlElement->IntAttribute ("num"));
		else
		{
			string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		const char* spezial = xmlElement->Attribute ("special");
		if (spezial != nullptr)
		{
			string specialString = spezial;
			if      (specialString == "mine")       UnitsDataGlobal.setSpecialIDMine(sID(1, IDList.back()));
			else if (specialString == "energy")     UnitsDataGlobal.setSpecialIDSmallGen(sID(1, IDList.back()));
			else if (specialString == "connector")  UnitsDataGlobal.setSpecialIDConnector(sID(1, IDList.back()));
			else if (specialString == "landmine")   UnitsDataGlobal.setSpecialIDLandMine(sID(1, IDList.back()));
			else if (specialString == "seamine")    UnitsDataGlobal.setSpecialIDSeaMine(sID(1, IDList.back()));
			else if (specialString == "smallBeton") UnitsDataGlobal.setSpecialIDSmallBeton(sID(1, IDList.back()));
			else Log.write ("Unknown spacial in buildings.xml \"" + specialString + "\"", cLog::eLOG_TYPE_WARNING);
		}
	}

	if (UnitsDataGlobal.getSpecialIDMine().secondPart       == 0) Log.write ("special \"mine\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDSmallGen().secondPart   == 0) Log.write("special \"energy\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDConnector().secondPart  == 0) Log.write("special \"connector\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDLandMine().secondPart   == 0) Log.write("special \"landmine\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDSeaMine().secondPart    == 0) Log.write("special \"seamine\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDSmallBeton().secondPart == 0) Log.write("special \"smallBeton\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);

	// load found units
	UnitsUiData.buildingUIs.resize(BuildingList.size());
	for (size_t i = 0; i != BuildingList.size(); ++i)
	{
		string sBuildingPath = cSettings::getInstance().getBuildingsPath();
		sBuildingPath += PATH_DELIMITER;
		sBuildingPath += BuildingList[i];
		sBuildingPath += PATH_DELIMITER;

		cStaticUnitData staticData;
		cDynamicUnitData dynamicData;
		sBuildingUIData& ui = UnitsUiData.buildingUIs[i];

		LoadUnitData (staticData, dynamicData, sBuildingPath.c_str(), IDList[i]);

		ui.id = staticData.ID;

		// load graphics.xml
		LoadUnitGraphicProperties(ui, sBuildingPath.c_str());

		if (DEDICATED_SERVER) continue;

		// load img
		sTmpString = sBuildingPath;
		sTmpString += "img.pcx";
		if (FileExists (sTmpString.c_str()))
		{
			ui.img_org = LoadPCX (sTmpString);
			ui.img = CloneSDLSurface (*ui.img_org);
			SDL_SetColorKey (ui.img_org.get(), SDL_TRUE, 0xFFFFFF);
			SDL_SetColorKey (ui.img.get(), SDL_TRUE, 0xFFFFFF);
		}
		else
		{
			Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
			return -1;
		}
		// load shadow
		sTmpString = sBuildingPath;
		sTmpString += "shw.pcx";
		if (FileExists (sTmpString.c_str()))
		{
			ui.shw_org = LoadPCX (sTmpString);
			ui.shw     = CloneSDLSurface (*ui.shw_org);
			SDL_SetSurfaceAlphaMod (ui.shw.get(), 50);
		}

		// load video
		sTmpString = sBuildingPath;
		sTmpString += "video.pcx";
		if (FileExists (sTmpString.c_str()))
			ui.video = LoadPCX (sTmpString);

		// load infoimage
		sTmpString = sBuildingPath;
		sTmpString += "info.pcx";
		if (FileExists (sTmpString.c_str()))
			ui.info = LoadPCX (sTmpString);

		// load effectgraphics if necessary
		if (ui.powerOnGraphic)
		{
			sTmpString = sBuildingPath;
			sTmpString += "effect.pcx";
			if (FileExists (sTmpString.c_str()))
			{
				ui.eff_org = LoadPCX (sTmpString);
				ui.eff = CloneSDLSurface (*ui.eff_org);
				SDL_SetSurfaceAlphaMod (ui.eff.get(), 10);
			}
		}
		else
		{
			ui.eff_org = nullptr;
			ui.eff     = nullptr;
		}

		// load sounds
		LoadUnitSoundfile (ui.Wait,    sBuildingPath.c_str(), "wait.ogg");
		LoadUnitSoundfile (ui.Start,   sBuildingPath.c_str(), "start.ogg");
		LoadUnitSoundfile (ui.Running, sBuildingPath.c_str(), "running.ogg");
		LoadUnitSoundfile (ui.Stop,    sBuildingPath.c_str(), "stop.ogg");
		LoadUnitSoundfile (ui.Attack,  sBuildingPath.c_str(), "attack.ogg");

		// Get Ptr if necessary:
		if (staticData.ID == UnitsDataGlobal.getSpecialIDConnector())
		{
			ui.isConnectorGraphic = true;
			UnitsUiData.ptr_connector = ui.img.get();
			UnitsUiData.ptr_connector_org = ui.img_org.get();
			SDL_SetColorKey (UnitsUiData.ptr_connector, SDL_TRUE, 0xFF00FF);
			UnitsUiData.ptr_connector_shw = ui.shw.get();
			UnitsUiData.ptr_connector_shw_org = ui.shw_org.get();
			SDL_SetColorKey(UnitsUiData.ptr_connector_shw, SDL_TRUE, 0xFF00FF);
		}
		else if (staticData.ID == UnitsDataGlobal.getSpecialIDSmallBeton())
		{
			UnitsUiData.ptr_small_beton = ui.img.get();
			UnitsUiData.ptr_small_beton_org = ui.img_org.get();
			SDL_SetColorKey(UnitsUiData.ptr_small_beton, SDL_TRUE, 0xFF00FF);
		}

		// Check if there is more than one frame
		// use 129 here because some images from the res_installer are one pixel to large
		if (ui.img_org->w > 129 && !ui.isConnectorGraphic && !ui.hasClanLogos) ui.hasFrames = ui.img_org->w / ui.img_org->h;
		else ui.hasFrames = 0;

		UnitsDataGlobal.addData(staticData);
		UnitsDataGlobal.addData(dynamicData);
	}

	// Dirtsurfaces
	if (!DEDICATED_SERVER)
	{
		LoadGraphicToSurface (UnitsUiData.rubbleBig->img_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_big.pcx");
		UnitsUiData.rubbleBig->img = CloneSDLSurface (*UnitsUiData.rubbleBig->img_org);
		LoadGraphicToSurface(UnitsUiData.rubbleBig->shw_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_big_shw.pcx");
		UnitsUiData.rubbleBig->shw = CloneSDLSurface(*UnitsUiData.rubbleBig->shw_org);
		if (UnitsUiData.rubbleBig->shw != nullptr) SDL_SetSurfaceAlphaMod(UnitsUiData.rubbleBig->shw.get(), 50);

		LoadGraphicToSurface(UnitsUiData.rubbleSmall->img_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_small.pcx");
		UnitsUiData.rubbleSmall->img = CloneSDLSurface(*UnitsUiData.rubbleSmall->img_org);
		LoadGraphicToSurface(UnitsUiData.rubbleSmall->shw_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_small_shw.pcx");
		UnitsUiData.rubbleSmall->shw = CloneSDLSurface(*UnitsUiData.rubbleSmall->shw_org);
		if (UnitsUiData.rubbleSmall->shw != nullptr) SDL_SetSurfaceAlphaMod(UnitsUiData.rubbleSmall->shw.get(), 50);
	}
	return 1;
}


//------------------------------------------------------------------------------
static void LoadUnitData (cStaticUnitData& staticData, cDynamicUnitData& dynamicData, char const* const directory, int const iID)
{
	tinyxml2::XMLDocument unitDataXml;

	string path = directory;
	path += "data.xml";
	if (!FileExists (path.c_str())) return ;

	if (unitDataXml.LoadFile (path.c_str()) != XML_NO_ERROR)
	{
		Log.write ("Can't load " + path, cLog::eLOG_TYPE_WARNING);
		return ;
	}
	// Read minimal game version
	string gameVersion = getXMLAttributeString (unitDataXml, "text", "Unit", "Header", "Game_Version", nullptr);

	//TODO check game version

	//read id
	string idString = getXMLAttributeString (unitDataXml, "ID", "Unit", nullptr);
	char szTmp[100];
	// check whether the id exists twice
	sID id;
	id.firstPart = atoi (idString.substr (0, idString.find (" ", 0)).c_str());
	id.secondPart = atoi (idString.substr (idString.find (" ", 0), idString.length()).c_str());

	for (size_t i = 0; i != UnitsDataGlobal.getStaticUnitsData().size(); ++i)
	{
		if (UnitsDataGlobal.getStaticUnitsData()[i].ID == id)
		{
			TIXML_SNPRINTF (szTmp, sizeof (szTmp), "unit with id %.2d %.2d already exists", id.firstPart, id.secondPart);
			Log.write (szTmp, cLog::eLOG_TYPE_WARNING);
			return ;
		}
	}

	staticData.ID = id;
	dynamicData.setId(id);
	// check whether the read id is the same as the one from vehicles.xml or buildins.xml
	if (iID != atoi (idString.substr (idString.find (" ", 0), idString.length()).c_str()))
	{
		TIXML_SNPRINTF (szTmp, sizeof (szTmp), "ID %.2d %.2d isn't equal with ID for unit \"%s\" ", atoi (idString.substr (0, idString.find (" ", 0)).c_str()), atoi (idString.substr (idString.find (" ", 0), idString.length()).c_str()), directory);
		Log.write (szTmp, cLog::eLOG_TYPE_WARNING);
		return ;
	}
	else
	{
		TIXML_SNPRINTF (szTmp, sizeof (szTmp), "ID %.2d %.2d verified", atoi (idString.substr (0, idString.find (" ", 0)).c_str()), atoi (idString.substr (idString.find (" ", 0), idString.length()).c_str()));
		Log.write (szTmp, cLog::eLOG_TYPE_DEBUG);
	}
	//read name
	staticData.setName(getXMLAttributeString (unitDataXml, "name", "Unit", nullptr));
	//read description
	if (XMLElement* const XMLElement = XmlGetFirstElement (unitDataXml, "Unit", "Description", nullptr))
	{
		std::string description(XMLElement->GetText());
		size_t pos;
		while ((pos = description.find("\\n")) != string::npos)
		{
			description.replace(pos, 2, "\n");
		}
		staticData.setDescription(description);
	}

	// Weapon
	string muzzleType = getXMLAttributeString (unitDataXml, "Const", "Unit", "Weapon", "Muzzle_Type", nullptr);
	if (muzzleType.compare ("Big") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_BIG;
	else if (muzzleType.compare("Rocket") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_ROCKET;
	else if (muzzleType.compare("Small") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_SMALL;
	else if (muzzleType.compare("Med") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_MED;
	else if (muzzleType.compare("Med_Long") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_MED_LONG;
	else if (muzzleType.compare("Rocket_Cluster") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_ROCKET_CLUSTER;
	else if (muzzleType.compare("Torpedo") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_TORPEDO;
	else if (muzzleType.compare("Sniper") == 0) staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_SNIPER;
	else staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_NONE;

	dynamicData.setAmmoMax(getXMLAttributeInt(unitDataXml, "Unit", "Weapon", "Ammo_Quantity", nullptr));
	dynamicData.setShotsMax(getXMLAttributeInt(unitDataXml, "Unit", "Weapon", "Shots", nullptr));
	dynamicData.setRange(getXMLAttributeInt(unitDataXml, "Unit", "Weapon", "Range", nullptr));
	dynamicData.setDamage(getXMLAttributeInt(unitDataXml, "Unit", "Weapon", "Damage", nullptr));
	staticData.canAttack = getXMLAttributeInt(unitDataXml, "Unit", "Weapon", "Can_Attack", nullptr);

	// TODO: make the code differ between attacking sea units and land units.
	// until this is done being able to attack sea units means being able to attack ground units.
	if (staticData.canAttack & TERRAIN_SEA) staticData.canAttack |= TERRAIN_GROUND;

	staticData.canDriveAndFire = getXMLAttributeBool(unitDataXml, "Unit", "Weapon", "Can_Drive_And_Fire", nullptr);

	// Production
	dynamicData.setBuildCost(getXMLAttributeInt (unitDataXml, "Unit", "Production", "Built_Costs", nullptr));

	staticData.canBuild = getXMLAttributeString (unitDataXml, "String", "Unit", "Production", "Can_Build", nullptr);
	staticData.buildAs = getXMLAttributeString(unitDataXml, "String", "Unit", "Production", "Build_As", nullptr);

	staticData.maxBuildFactor = getXMLAttributeInt(unitDataXml, "Unit", "Production", "Max_Build_Factor", nullptr);

	staticData.canBuildPath = getXMLAttributeBool(unitDataXml, "Unit", "Production", "Can_Build_Path", nullptr);
	staticData.canBuildRepeat = getXMLAttributeBool(unitDataXml, "Unit", "Production", "Can_Build_Repeat", nullptr);

	// Movement
	dynamicData.setSpeedMax (getXMLAttributeInt (unitDataXml, "Unit", "Movement", "Movement_Sum", nullptr) * 4);

	staticData.factorGround = getXMLAttributeFloat (unitDataXml, "Unit", "Movement", "Factor_Ground", nullptr);
	staticData.factorSea = getXMLAttributeFloat(unitDataXml, "Unit", "Movement", "Factor_Sea", nullptr);
	staticData.factorAir = getXMLAttributeFloat(unitDataXml, "Unit", "Movement", "Factor_Air", nullptr);
	staticData.factorCoast = getXMLAttributeFloat(unitDataXml, "Unit", "Movement", "Factor_Coast", nullptr);

	// Abilities
	staticData.isBig = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Is_Big", nullptr);
	staticData.connectsToBase = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Connects_To_Base", nullptr);
	dynamicData.setArmor(getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Armor", nullptr));
	dynamicData.setHitpointsMax(getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Hitpoints", nullptr));
	dynamicData.setScan(getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Scan_Range", nullptr));
	staticData.modifiesSpeed = getXMLAttributeFloat (unitDataXml, "Unit", "Abilities", "Modifies_Speed", nullptr);
	staticData.canClearArea = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Clear_Area", nullptr);
	staticData.canBeCaptured = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Be_Captured", nullptr);
	staticData.canBeDisabled = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Be_Disabled", nullptr);
	staticData.canCapture = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Capture", nullptr);
	staticData.canDisable = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Disable", nullptr);
	staticData.canRepair = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Repair", nullptr);
	staticData.canRearm = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Rearm", nullptr);
	staticData.canResearch = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Research", nullptr);
	staticData.canPlaceMines = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Place_Mines", nullptr);
	staticData.canSurvey = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Survey", nullptr);
	staticData.doesSelfRepair = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Does_Self_Repair", nullptr);
	staticData.convertsGold = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Converts_Gold", nullptr);
	staticData.canSelfDestroy = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Self_Destroy", nullptr);
	staticData.canScore = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Can_Score", nullptr);

	staticData.canMineMaxRes = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Can_Mine_Max_Resource", nullptr);

	staticData.needsMetal = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Needs_Metal", nullptr);
	staticData.needsOil = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Needs_Oil", nullptr);
	staticData.needsEnergy = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Needs_Energy", nullptr);
	staticData.needsHumans = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Needs_Humans", nullptr);
	if (staticData.needsEnergy < 0)
	{
		staticData.produceEnergy = abs(staticData.needsEnergy);
		staticData.needsEnergy = 0;
	}
	else staticData.produceEnergy = 0;
	if (staticData.needsHumans < 0)
	{
		staticData.produceHumans = abs(staticData.needsHumans);
		staticData.needsHumans = 0;
	}
	else staticData.produceHumans = 0;

	staticData.isStealthOn = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Is_Stealth_On", nullptr);
	staticData.canDetectStealthOn = getXMLAttributeInt(unitDataXml, "Unit", "Abilities", "Can_Detect_Stealth_On", nullptr);

	string surfacePosString = getXMLAttributeString (unitDataXml, "Const", "Unit", "Abilities", "Surface_Position", nullptr);
	if (surfacePosString.compare("BeneathSea") == 0)staticData.surfacePosition = cStaticUnitData::SURFACE_POS_BENEATH_SEA;
	else if (surfacePosString.compare("AboveSea") == 0) staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE_SEA;
	else if (surfacePosString.compare("Base") == 0) staticData.surfacePosition = cStaticUnitData::SURFACE_POS_BASE;
	else if (surfacePosString.compare("AboveBase") == 0) staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE_BASE;
	else if (surfacePosString.compare("Above") == 0) staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE;
	else staticData.surfacePosition = cStaticUnitData::SURFACE_POS_GROUND;

	string overbuildString = getXMLAttributeString (unitDataXml, "Const", "Unit", "Abilities", "Can_Be_Overbuild", nullptr);
	if (overbuildString.compare ("Yes") == 0) staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_YES;
	else if (overbuildString.compare ("YesNRemove") == 0) staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE;
	else staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_NO;

	staticData.canBeLandedOn = getXMLAttributeBool (unitDataXml, "Unit", "Abilities", "Can_Be_Landed_On", nullptr);
	staticData.canWork = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Is_Activatable", nullptr);
	staticData.explodesOnContact = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Explodes_On_Contact", nullptr);
	staticData.isHuman = getXMLAttributeBool(unitDataXml, "Unit", "Abilities", "Is_Human", nullptr);

	// Storage
	staticData.storageResMax = getXMLAttributeInt(unitDataXml, "Unit", "Storage", "Capacity_Resources", nullptr);

	string storeResString = getXMLAttributeString (unitDataXml, "Const", "Unit", "Storage", "Capacity_Res_Type", nullptr);
	if (storeResString.compare("Metal") == 0) staticData.storeResType = eResourceType::Metal;
	else if (storeResString.compare("Oil") == 0) staticData.storeResType = eResourceType::Oil;
	else if (storeResString.compare("Gold") == 0) staticData.storeResType = eResourceType::Gold;
	else staticData.storeResType = eResourceType::None;

	staticData.storageUnitsMax = getXMLAttributeInt(unitDataXml, "Unit", "Storage", "Capacity_Units", nullptr);

	string storeUnitImgString = getXMLAttributeString (unitDataXml, "Const", "Unit", "Storage", "Capacity_Units_Image_Type", nullptr);
	if (storeUnitImgString.compare("Plane") == 0) staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_PLANE;
	else if (storeUnitImgString.compare("Human") == 0) staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_HUMAN;
	else if (storeUnitImgString.compare("Tank") == 0) staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_TANK;
	else if (storeUnitImgString.compare("Ship") == 0) staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_SHIP;
	else staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_TANK;

	string storeUnitsString = getXMLAttributeString (unitDataXml, "String", "Unit", "Storage", "Capacity_Units_Type", nullptr);
	Split(storeUnitsString, "+", staticData.storeUnitsTypes);

	staticData.isStorageType = getXMLAttributeString(unitDataXml, "String", "Unit", "Storage", "Is_Storage_Type", nullptr);

	// finish
	Log.write ("Unitdata read", cLog::eLOG_TYPE_DEBUG);
	if (cSettings::getInstance().isDebug()) Log.mark();
	return ;
}

//------------------------------------------------------------------------------
static void LoadUnitGraphicProperties(sVehicleUIData& data, char const* directory)
{
	tinyxml2::XMLDocument unitGraphicsXml;

	string path = directory;
	path += "graphics.xml";
	if (!FileExists(path.c_str())) return;

	if (unitGraphicsXml.LoadFile(path.c_str()) != XML_NO_ERROR)
	{
		Log.write("Can't load " + path, cLog::eLOG_TYPE_WARNING);
		return;
	}

	data.hasCorpse = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Corpse", nullptr);
	data.hasDamageEffect = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Damage_Effect", nullptr);
	data.hasPlayerColor = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Player_Color", nullptr);
	data.hasOverlay = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Overlay", nullptr);

	data.buildUpGraphic = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Build_Up", nullptr);
	data.animationMovement = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Movement", nullptr);
	data.powerOnGraphic = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Power_On", nullptr);
	data.isAnimated = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Is_Animated", nullptr);
	data.makeTracks = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Makes_Tracks", nullptr);
}

//------------------------------------------------------------------------------
static void LoadUnitGraphicProperties(sBuildingUIData& data, char const* directory)
{
	tinyxml2::XMLDocument unitGraphicsXml;

	string path = directory;
	path += "graphics.xml";
	if (!FileExists(path.c_str())) return;

	if (unitGraphicsXml.LoadFile(path.c_str()) != XML_NO_ERROR)
	{
		Log.write("Can't load " + path, cLog::eLOG_TYPE_WARNING);
		return;
	}

	data.hasClanLogos = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Clan_Logos", nullptr);
	data.hasDamageEffect = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Damage_Effect", nullptr);
	data.hasBetonUnderground = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Beton_Underground", nullptr);
	data.hasPlayerColor = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Player_Color", nullptr);
	data.hasOverlay = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Has_Overlay", nullptr);

	data.buildUpGraphic = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Build_Up", nullptr);
	data.powerOnGraphic = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Power_On", nullptr);
	data.isAnimated = getXMLAttributeBool(unitGraphicsXml, "Unit", "Graphic", "Animations", "Is_Animated", nullptr);
}

//------------------------------------------------------------------------------
static int LoadClans()
{
	tinyxml2::XMLDocument clansXml;

	string clansXMLPath = CLANS_XML;
	if (!FileExists (clansXMLPath.c_str()))
		return 0;
	if (clansXml.LoadFile (clansXMLPath.c_str()) != XML_NO_ERROR)
	{
		Log.write ("Can't load " + clansXMLPath, cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	XMLElement* xmlElement = clansXml.FirstChildElement ("Clans");
	if (xmlElement == 0)
	{
		Log.write ("Can't read \"Clans\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	for (XMLElement* clanElement = xmlElement->FirstChildElement ("Clan"); clanElement; clanElement = clanElement->NextSiblingElement ("Clan"))
	{
		cClan* newClan = ClanDataGlobal.addClan();
		string nameAttr = clanElement->Attribute ("Name");
		newClan->setName (nameAttr);

		const XMLElement* descriptionNode = clanElement->FirstChildElement ("Description");
		if (descriptionNode)
		{
			string descriptionString = descriptionNode->GetText();
			newClan->setDescription (descriptionString);
		}

		for (XMLElement* statsElement = clanElement->FirstChildElement ("ChangedUnitStat"); statsElement; statsElement = statsElement->NextSiblingElement ("ChangedUnitStat"))
		{
			const char* idAttr = statsElement->Attribute ("UnitID");
			if (idAttr == 0)
			{
				Log.write ("Couldn't read UnitID for ChangedUnitStat for clans", cLog::eLOG_TYPE_ERROR);
				continue;
			}
			string idAttrStr (idAttr);
			sID id;
			id.firstPart = atoi (idAttrStr.substr (0, idAttrStr.find (" ", 0)).c_str());
			id.secondPart = atoi (idAttrStr.substr (idAttrStr.find (" ", 0), idAttrStr.length()).c_str());

			cClanUnitStat* newStat = newClan->addUnitStat (id);

			for (XMLElement* modificationElement = statsElement->FirstChildElement(); modificationElement; modificationElement = modificationElement->NextSiblingElement())
			{
				string modName = modificationElement->Value();
				if (modificationElement->Attribute ("Num"))
				{
					newStat->addModification (modName, modificationElement->IntAttribute ("Num"));
				}
			}
		}
	}
	UnitsDataGlobal.initializeClanUnitData(ClanDataGlobal);

	return 1;
}

void createShadowGfx()
{
	// TODO: reduce size once we use texture.
	GraphicsData.gfx_shadow = AutoSurface (SDL_CreateRGBSurface (0, Video.getResolutionX(), Video.getResolutionY(),
										   Video.getColDepth(),
										   0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000));
	SDL_FillRect (GraphicsData.gfx_shadow.get(), nullptr, SDL_MapRGBA (GraphicsData.gfx_shadow->format, 0, 0, 0, 50));
}

void Split(const std::string& s, const char* seps, std::vector<std::string>& words)
{
	if (s.empty()) return;
	size_t beg = 0;
	size_t end = s.find_first_of(seps, beg);

	while (end != string::npos)
	{
		words.push_back(s.substr(beg, end - beg));
		beg = end + 1;
		end = s.find_first_of(seps, beg);
	}
	words.push_back(s.substr(beg));
}
