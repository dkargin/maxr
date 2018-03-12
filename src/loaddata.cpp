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
#include <SDL_events.h>
#include <iostream>
#include <sstream>

#ifdef WIN32

#else
# include <sys/stat.h>
# include <unistd.h>
#endif

#include "loaddata.h"
#include "maxrversion.h"

#include "utility/drawing.h"
#include "utility/files.h"
#include "utility/pcx.h"
#include "utility/unifonts.h"
#include "utility/log.h"

#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/data/player/clans.h"

#include "tinyxml2.h"
#include "extendedtinyxml.h"
#include "keys.h"

#include "main.h"

#include "settings.h"
#include "sound.h"
#include "video.h"
#include "debug.h"

#include "game/data/moddata.h"

#ifdef WIN32
# include <shlobj.h>
# include <direct.h>
#endif

using namespace std;
using namespace tinyxml2;

tinyxml2::XMLDocument LanguageFile;

cLoader::cLoader()
	:LoaderNotification(SDL_RegisterEvents(1))

{

}

cLoader::~cLoader()
{
	join();
}

/**
 * Loads the selected languagepack
 * @return 1 on success
 */
int LoadLanguage();

/**
 * Loads all Graphics
 * @param path Directory of the graphics
 * @return 1 on success
 */
int LoadGraphics (const char* path);

/**
 * Loads the Effects
 * @param path Directory of the Effects
 * @return 1 on success
 */
int LoadEffects (const char* path);

/**
 * Loads all Musicfiles
 * @param path Directory of the Vehicles
 * @return 1 on success
 */
static int LoadMusic (const char* path);


void cLoader::join()
{
	if(load_thread)
	{
		SDL_WaitThread(load_thread, nullptr);
		load_thread = nullptr;
	}
}

int cLoader::getEventType() const
{
	return LoaderNotification;
}

int cLoader::threadFn(void * data)
{
	return static_cast<cLoader*>(data)->loadImpl();
}

cLoader::eLoadingState cLoader::load(const char * mod)
{
	pendingMods.clear();
	if(mod)
		pendingMods.push_back(mod);

	assert(load_thread == nullptr);

	load_thread = SDL_CreateThread (&cLoader::threadFn, "loadingData", this);

	return LOAD_GOING;
}

void cLoader::notifyState(eLoadingState newState)
{
	// TODO: Mutex it
	auto oldState = lastState;

	lastState = newState;

	assert(this->LoaderNotification != ((Uint32)-1));

	SDL_Event event;
	SDL_memset(&event, 0, sizeof(event)); /* or SDL_zero(event) */
	event.type = LoaderNotification;
	event.user.code = (int)newState;
	event.user.data1 = reinterpret_cast<void*>(oldState);
	event.user.data2 = 0;
	SDL_PushEvent(&event);
}

// LoadData ///////////////////////////////////////////////////////////////////
// Loads all relevant files and data:

int cLoader::loadImpl()
{
	CR_ENABLE_CRASH_RPT_CURRENT_THREAD();

	// TODO: Mutex it
	auto mods = this->pendingMods;

	loadFolder("data");
	for(auto mod: mods)
	{
		if(!mod.empty())
			loadFolder(mod.c_str());
	}

	finalizeLoading();

	if(useDelay)
		SDL_Delay (1000);

	notifyState(LOAD_FINISHED);
}

void cLoader::finalizeLoading()
{
	// Should do this afer all data is complete
	UnitsDataGlobal.initializeClanUnitData(ClanDataGlobal);
}

int cLoader::loadFolder(const char* path)
{
	notifyState(LOAD_GOING);

	if (!DEDICATED_SERVER)
	{
		const std::string& fontPath = cSettings::getInstance().getFontPath() + PATH_DELIMITER;
		if (!FileExists ((fontPath + "latin_normal.pcx").c_str())
			|| !FileExists ((fontPath + "latin_big.pcx").c_str())
			|| !FileExists ((fontPath + "latin_big_gold.pcx").c_str())
			|| !FileExists ((fontPath + "latin_small.pcx").c_str()))
		{
			Log.write ("Missing a file needed for game. Check log and config! ", cLog::eLOG_TYPE_ERROR);
			notifyState(LOAD_ERROR);
			return 0;
		}

		cUnicodeFont::font.reset(new cUnicodeFont()); // init ascii fonts
		cUnicodeFont::font->setTargetSurface (cVideo::buffer);
		Log.mark();
	}

	string sVersion = PACKAGE_NAME;
	sVersion += " BUILD: ";
	sVersion += MAX_BUILD_DATE; sVersion += " ";
	sVersion += PACKAGE_REV;

	writeConsole (sVersion, 0, 0);

	// Load Languagepack
	writeConsole ("Loading languagepack...", 0, 2);

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
		writeConsole ("", -1, SAME_LINE);
		notifyState(LOAD_ERROR);
		SDL_Delay (5000);
		return -1;
	}
	else
	{
		LanguageFile.LoadFile (sTmpString.c_str());
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();

	// Load Keys
	writeConsole (lngPack.i18n ("Text~Init~Keys"), 0, NEXT_LINE);

	try
	{
		KeysList.loadFromFile();
		writeConsole ("", 1, SAME_LINE);
	}
	catch (std::runtime_error& e)
	{
		Log.write (e.what(), cLog::eLOG_TYPE_ERROR);
		writeConsole ("", -1, SAME_LINE);
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	Log.mark();

	// Load Fonts
	writeConsole (lngPack.i18n ("Text~Init~Fonts"), 0, NEXT_LINE);
	// -- little bit crude but fonts are already loaded.
	// what to do with this now? -- beko
	// Really loaded with new cUnicodeFont
	writeConsole ("", 1, SAME_LINE);
	Log.mark();

	// Load Graphics
	writeConsole (lngPack.i18n ("Text~Init~GFX"), 0, NEXT_LINE);

	if (LoadGraphics (cSettings::getInstance().getGfxPath().c_str()) != 1)
	{
		writeConsole ("", -1, SAME_LINE);
		Log.write ("Error while loading graphics", cLog::eLOG_TYPE_ERROR);
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	else
	{
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();

	// Load Effects
	writeConsole (lngPack.i18n ("Text~Init~Effects"), 0, NEXT_LINE);

	if (LoadEffects (cSettings::getInstance().getFxPath().c_str()) != 1)
	{
		writeConsole ("", -1, SAME_LINE);
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	else
	{
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();

	// Load Vehicles
	writeConsole (lngPack.i18n ("Text~Init~Vehicles"), 0, NEXT_LINE);

	ModData mod("core", &UnitsDataGlobal);
	if (mod.loadVehicles(cSettings::getInstance().getVehiclesPath().c_str()) != 1)
	{
		writeConsole ("", -1, SAME_LINE);
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	else
	{
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();

	// Load Buildings
	writeConsole (lngPack.i18n ("Text~Init~Buildings"), 0);

	if (mod.loadBuildings(cSettings::getInstance().getBuildingsPath().c_str()) != 1)
	{
		writeConsole ("", -1, SAME_LINE);
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	else
	{
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();

	writeConsole (lngPack.i18n ("Text~Init~Clans"), 0);

	// Load Clan Settings
	if (mod.loadClans() != 1)
	{
		SDL_Delay (5000);
		notifyState(LOAD_ERROR);
		return -1;
	}
	else
	{
		writeConsole ("", 1, SAME_LINE);
	}
	Log.mark();


	if (!DEDICATED_SERVER)
	{
		// Load Music
		writeConsole (lngPack.i18n ("Text~Init~Music"), 0);

		if (LoadMusic (cSettings::getInstance().getMusicPath().c_str()) != 1)
		{
			writeConsole ("", -1, SAME_LINE);
			SDL_Delay (5000);
			notifyState(LOAD_ERROR);
			return -1;
		}
		else
		{
			writeConsole ("", 1, SAME_LINE);
		}
		Log.mark();

		// Load Sounds
		writeConsole (lngPack.i18n ("Text~Init~Sounds"), 0, NEXT_LINE);
		Log.write ("Loading Sounds", cLog::eLOG_TYPE_INFO);
		SoundData.load (cSettings::getInstance().getSoundsPath().c_str());
		writeConsole ("", 1, SAME_LINE);
		Log.mark();

		// Load Voices
		writeConsole (lngPack.i18n ("Text~Init~Voices"), 0, NEXT_LINE);
		Log.write ("Loading Voices", cLog::eLOG_TYPE_INFO);
		VoiceData.load (cSettings::getInstance().getVoicesPath().c_str());
		writeConsole ("", 1, SAME_LINE);
		Log.mark();
	}

	return 1;
}

/**
 * Writes a Logmessage on the SplashScreen
 * @param sTxt Text to write
 * @param ok 0 writes just text, 1 writes "OK" and else "ERROR"
 * @param pos Horizontal Positionindex on SplashScreen
 */
void cLoader::writeConsole (const string& sTxt, int ok, int increment)
{
	int pos = this->logPosition;
	logPosition += increment;

	if (DEDICATED_SERVER)
	{
		cout << sTxt << endl;
		return;
	}
	const auto& font = cUnicodeFont::font.get();
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

int LoadLanguage()
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

int LoadEffects (const char* path)
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

int LoadGraphics (const char* path)
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

	int maxUnitSize = 8;
	int cellSize = 64;
	GraphicsData.gfx_tmp = AutoSurface (SDL_CreateRGBSurface (0, maxUnitSize*cellSize, maxUnitSize*cellSize, Video.getColDepth(), 0, 0, 0, 0));
	SDL_SetSurfaceBlendMode(GraphicsData.gfx_tmp.get(), SDL_BLENDMODE_BLEND);
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
