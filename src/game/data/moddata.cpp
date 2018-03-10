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
// Loads game data: units, buildigns, clans
//
///////////////////////////////////////////////////////////////////////////////

#include <SDL_mixer.h>
#include <sstream>

#ifdef WIN32

#else
# include <sys/stat.h>
# include <unistd.h>
#endif

#include "loaddata.h"
#include "maxrversion.h"

#include "utility/autosurface.h"
#include "utility/files.h"
#include "utility/pcx.h"
//#include "utility/unifonts.h"
#include "utility/log.h"

#include "game/data/units/building.h"
#include "game/data/units/vehicle.h"
#include "game/data/player/clans.h"

#include "tinyxml2.h"

#include "settings.h"
#include "sound.h"
#include "video.h"	// do not want this as well
#include "debug.h"
#include "utility/drawing.h"

tinyxml2::XMLElement* XmlGetFirstElement (tinyxml2::XMLDocument& xmlDoc, const char* first, ...);

#include "moddata.h"

using namespace std;
using namespace tinyxml2;

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

// Loads unit data from the path
void LoadLegacyUnitGraphics(std::string srcPath,
				  cSpriteTool& tool,
				  cStaticUnitData& data)
{
	Log.write (" - loading legacy GFX for " + data.getName() + " from " + srcPath, cLog::eLOG_TYPE_INFO);
	std::string sTmpString;

	int size = data.cellSize;
	cVector2 unitSize(size, size);

	// load infoimage
	sTmpString = srcPath + PATH_DELIMITER + "info.pcx";

	if (FileExists (sTmpString))
	{
		Log.write (" - loading portrait" + sTmpString, cLog::eLOG_TYPE_DEBUG);
		data.info = LoadPCX (sTmpString);
	}
	else
	{
		Log.write ("Missing portrait for " + data.getName() + " file " + sTmpString + " does not extis", cLog::eLOG_TYPE_ERROR);
	}

	// Load an old 'static' data
	// New graphics is described inside XMLs
	if(!data.customGraphics)
	{
		// load img
		sTmpString = srcPath + PATH_DELIMITER + "img.pcx";

		if(FileExists(sTmpString))
		{
			auto sprite = tool.makeSprite(sTmpString, unitSize);
			if(sprite)
			{
				sprite->setColorKeyAuto();
				data.image = sprite;
			}
			else
			{
				Log.write (" - can not load file " + sTmpString, cLog::eLOG_TYPE_ERROR);
			}
		}
		// load shadow
		sTmpString = srcPath + PATH_DELIMITER + "shw.pcx";
		if(FileExists(sTmpString))
		{
			auto sprite = tool.makeSprite(sTmpString, unitSize);
			if(sprite)
			{
				sprite->setColorKeyAuto();
				data.shadow = sprite;
			}
		}
		// load overlay graphics, if necessary
		sTmpString = srcPath + PATH_DELIMITER + "overlay.pcx";
		if(FileExists(sTmpString))
			data.overlay = tool.makeSprite(sTmpString, unitSize);
		// load infantery graphics
		if (data.hasFlag(UnitFlag::HasAnimationMovement))
		{
	#ifdef FIX_ANIMATIONS_LATER
			// TODO: Animations should be merged inside new cRenderable system
			SDL_Rect rcDest;
			for (int dir = 0; dir < 8; dir++)
			{
				//ui.directed_image[dir] = spriteTool.makeSprite();
				ui.img[dir] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.img[dir].get(), SDL_TRUE, 0x00FFFFFF);
				SDL_FillRect (ui.img[dir].get(), nullptr, 0x00FF00FF);

				for (int frame = 0; frame < 13; frame++)
				{
					sTmpString = sVehiclePath;
					char sztmp[16];
					TIXML_SNPRINTF (sztmp, sizeof (sztmp), "img%d_%.2d.pcx", dir, frame);
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
							rcDest.x = 64 * frame + 32 - sfTempSurface->w / 2;
							rcDest.y = 32 - sfTempSurface->h / 2;
							SDL_BlitSurface (sfTempSurface.get(), nullptr, ui.img[dir].get(), &rcDest);
						}
					}
				}
				ui.img_org[dir] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.img[dir].get(), SDL_TRUE, 0x00FFFFFF);
				SDL_FillRect (ui.img_org[dir].get(), nullptr, 0x00FFFFFF);
				SDL_BlitSurface (ui.img[dir].get(), nullptr, ui.img_org[dir].get(), nullptr);

				int size = staticData.cellSize;
				ui.img[dir] = AutoSurface (SDL_CreateRGBSurface (0, size*64 * 13, size*64, Video.getColDepth(), 0, 0, 0, 0));

				ui.shw[dir] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.shw[dir].get(), SDL_TRUE, 0x00FF00FF);
				SDL_FillRect (ui.shw[dir].get(), nullptr, 0x00FF00FF);
				ui.shw_org[dir] = AutoSurface (SDL_CreateRGBSurface (0, 64 * 13, 64, Video.getColDepth(), 0, 0, 0, 0));
				SDL_SetColorKey (ui.shw_org[dir].get(), SDL_TRUE, 0x00FF00FF);
				SDL_FillRect (ui.shw_org[dir].get(), nullptr, 0x00FF00FF);

				rcDest.x = 3;
				rcDest.y = 3;
				SDL_BlitSurface (ui.img_org[dir].get(), nullptr, ui.shw_org[dir].get(), &rcDest);
				SDL_LockSurface (ui.shw_org[dir].get());
				Uint32* ptr = static_cast<Uint32*> (ui.shw_org[dir]->pixels);
				for (int j = 0; j < 64 * 13 * 64; j++)
				{
					if (*ptr != 0x00FF00FF)
						*ptr = 0;
					ptr++;
				}
				SDL_UnlockSurface (ui.shw_org[dir].get());
				SDL_BlitSurface (ui.shw_org[dir].get(), nullptr, ui.shw[dir].get(), nullptr);
				SDL_SetSurfaceAlphaMod (ui.shw_org[dir].get(), 50);
				SDL_SetSurfaceAlphaMod (ui.shw[dir].get(), 50);
			}
	#endif
		}
		// load other vehicle graphics
		else
		{
#ifdef FUCK_THIS
			// TODO: Directions should be merged inside new cRenderable system
			for (int n = 0; n < 8; n++)
			{
				// load image
				char sztmp[16];
				TIXML_SNPRINTF (sztmp, sizeof (sztmp), "img%d.pcx", n);
				sTmpString = srcPath + PATH_DELIMITER + sztmp;

				if(FileExists(sTmpString))
				{
					//SDL_SetColorKey (ui.img[n].get(), SDL_TRUE, 0xFFFFFF);
					data.directed_image[n] = tool.makeSprite(sTmpString.c_str(), unitSize);
					if(data.directed_image[n])
					{
						data.directed_image[n]->setColorKey(0xFFFFFF);
					}
					else
					{
						// TODO: Set default image
						Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
					}
				}

				// load shadow
				TIXML_SNPRINTF (sztmp, sizeof (sztmp), "shw%d.pcx", n);
				sTmpString = srcPath + PATH_DELIMITER + sztmp;
				if(FileExists(sTmpString))
				{
					data.directed_shadow[n] = tool.makeSprite(sTmpString, unitSize);
					if (data.directed_shadow[n])
					{
						data.directed_shadow[n]->setAlphaKey(50);
					}
				}
			}
#endif
		}
	}
}

ModData::ModData(const char * _path, cUnitsData* _unitsData)
	:spriteTool(new cSpriteTool()), unitsData(_unitsData), path(_path)
{
	const int tileSize = 64;
	if(auto format = Video.getPixelFormat())
		spriteTool->setPixelFormat(*format);
	spriteTool->setCellSize(tileSize);
	loadMedia = true;
}

void ModData::parseVehicle(tinyxml2::XMLElement* source, sID id, const std::string& name, const char* directory)
{
	auto object = unitsData->makeVehicle(id);
	object->setName(name);
	parseUnitData (source, *object, directory);

	for(tinyxml2::XMLElement* graphic = source->FirstChildElement("Graphic"); graphic; graphic = graphic->NextSiblingElement("Graphic"))
	{
		object->animationMovement = getValueBool(graphic, "Movement");
		object->makeTracks = getValueBool(graphic, "Makes_Tracks");
	}

	std::string sTmpString = directory;
	sTmpString += PATH_DELIMITER"video.flc";
	Log.write (" - loading video " + sTmpString, cLog::eLOG_TYPE_DEBUG);
	if (FileExists (sTmpString.c_str()))
	{
		object->FLCFile = sTmpString;
	}

	// load storageimage
	sTmpString = directory;
	sTmpString += PATH_DELIMITER"store.pcx";
	Log.write (" - loading storageportrait" + sTmpString, cLog::eLOG_TYPE_DEBUG);
	if (FileExists (sTmpString.c_str()))
	{
		object->storage = LoadPCX (sTmpString);
	}
	else
	{
		Log.write ("Missing GFX - your MAXR install seems to be incomplete!", cLog::eLOG_TYPE_ERROR);
	}

	LoadLegacyUnitGraphics(directory, *spriteTool, *object);
}

void ModData::parseBuilding(tinyxml2::XMLElement* source, sID id, const std::string& name, const char* directory)
{
	auto object = this->unitsData->makeBuilding(id);
	object->setName(name);
	this->parseUnitData (source, *object, directory);

	for(tinyxml2::XMLElement* graphic = source->FirstChildElement("Graphic"); graphic; graphic = graphic->NextSiblingElement("Graphic"))
	{
		object->hasBetonUnderground = getValueBool(graphic, "Has_Beton_Underground");
	}

	LoadLegacyUnitGraphics(directory, *spriteTool, *object);

	// load video
	std::string sTmpString = directory;
	sTmpString += PATH_DELIMITER"video.pcx";
	if (FileExists (sTmpString))
		object->video = LoadPCX (sTmpString);

#ifdef VERY_BROKEN
	// I will send connectors to a separate special layer
	// Get Ptr if necessary:
	if (staticData.ID == UnitsDataGlobal.getSpecialIDConnector())
	{
		ui.isConnectorGraphic = true;
		UnitsUiData.ptr_connector = ui.img.get();
		UnitsUiData.ptr_connector_org = ui.img_org.get();
		SDL_SetColorKey (UnitsUiData.ptr_connector, SDL_TRUE, 0xFF00FF);
		UnitsUiData.ptr_connector_shw = ui.img_shadow.get();
		UnitsUiData.ptr_connector_shw_org = ui.img_shadow_original.get();
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
	if (ui.img_org->w > 129 && !ui.isConnectorGraphic && !ui.hasClanLogos)
		ui.hasFrames = ui.img_org->w / ui.img_org->h;
	else
		ui.hasFrames = 0;
#endif
}

void ModData::parseDataFile(const char* path, const char* directory)
{
	if (!FileExists (path))
	{
		Log.write ("ModData::parseDataFile(" + std::string(path) + ") - no such file", cLog::eLOG_TYPE_WARNING);
		return;
	}

	tinyxml2::XMLDocument xml;

	tinyxml2::XMLError xmlErr = xml.LoadFile (path);
	if (xmlErr != XML_NO_ERROR)
	{
		std::stringstream ss;
		ss << "Can't load " + std::string(path) << " error="<<int(xmlErr);
		Log.write (ss.str(), cLog::eLOG_TYPE_WARNING);
		return ;
	}

	Log.write ("Reading values from file " + std::string(path), cLog::eLOG_TYPE_DEBUG);

	tinyxml2::XMLElement* element = xml.RootElement();

	int errors = 0;
	int warnings = 0;

	if (element == nullptr)
	{
		Log.write ("File " + std::string(path) + " has no proper data", cLog::eLOG_TYPE_WARNING);
	}
	else do
	{
		/** We are looking for the following tags: <Vehicle>, <Building> */
		const char* value = element->Value();
		const char *attrName = element->Attribute("name");
		const char *attrID = element->Attribute("ID");

		sID id;
		if(!parseSID(attrID, id))
		{
			errors++;
			Log.write (" - root entity should have proper \'ID\' attribute. Skipping", cLog::eLOG_TYPE_WARNING);
		}
		if(!attrName)
		{
			Log.write (" - root entity should have a \'name\' attribute. Skipping", cLog::eLOG_TYPE_WARNING);
			errors++;
			continue;
		}

		std::string name = attrName;

		Log.write (" - found object name=" + name, cLog::eLOG_TYPE_INFO);

		std::string type = value;

		try
		{
			if(type== "Vehicle")
				parseVehicle(element, id, name, directory);
			else if(type=="Building")
				parseBuilding(element, id, name, directory);
			else
			{
				warnings++;
				Log.write (" - unknown element " + type + " in file " + path, cLog::eLOG_TYPE_WARNING);
			}
		}
		catch(std::runtime_error& e)
		{
			Log.write("Got an exception while loading an object", cLog::eLOG_TYPE_ERROR);
			return;
		}
	}while((element = element->NextSiblingElement()) != nullptr);
}

int ModData::loadVehicles(const char* vehicle_directory)
{
	// TODO merge with loadBuildings. They are mostly the same
	Log.write ("Loading Vehicles", cLog::eLOG_TYPE_INFO);

	tinyxml2::XMLDocument VehiclesXml;

	std::string sTmpString = vehicle_directory;
	sTmpString += PATH_DELIMITER "data.xml";

	if (!FileExists (sTmpString.c_str()))
	{
		return 0;
	}

	if (VehiclesXml.LoadFile (sTmpString.c_str()) != tinyxml2::XML_NO_ERROR)
	{
		Log.write ("Can't load vehicles.xml!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	tinyxml2::XMLElement* xmlElement = XmlGetFirstElement (VehiclesXml, "VehicleData", "Vehicles", nullptr);
	if (xmlElement == nullptr)
	{
		Log.write ("Can't read \"VehicleData->Vehicles\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	// read vehicles.xml
	std::vector<std::string> directories;
	std::vector<int> IDList;

	xmlElement = xmlElement->FirstChildElement();

	if(!xmlElement)
		Log.write ("No vehicles defined in vehicles.xml!", cLog::eLOG_TYPE_WARNING);

	while (xmlElement != nullptr)
	{
		const char* directory = xmlElement->Attribute ("directory");
		if (directory != nullptr)
			directories.push_back (directory);
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

		xmlElement = xmlElement->NextSiblingElement();
	}

	// load found units
	for (const auto& dirname: directories)
	{
		std::string directory = std::string(vehicle_directory) + PATH_DELIMITER + dirname;
		string xmlPath = directory + PATH_DELIMITER + "data.xml";

		parseDataFile(xmlPath.c_str(), directory.c_str());

		if (cSettings::getInstance().isDebug())
			Log.mark();
	}

	unitsData->initializeIDData();

	return 1;
}

int ModData::loadBuildings(const char* buldings_folder)
{
	// TODO merge with loadVehicles. They are mostly the same
	Log.write ("Loading Buildings", cLog::eLOG_TYPE_INFO);

	// read buildings.xml
	std::string sTmpString = buldings_folder;
	sTmpString += PATH_DELIMITER "data.xml";
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
	tinyxml2::XMLElement* xmlElement = XmlGetFirstElement (BuildingsXml, "BuildingsData", "Buildings", nullptr);
	if (xmlElement == nullptr)
	{
		Log.write ("Can't read \"BuildingData->Building\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}
	std::vector<std::string> directories;
	std::vector<int> IDList;

	xmlElement = xmlElement->FirstChildElement();
	if (xmlElement == nullptr)
	{
		Log.write ("There are no buildings in the buildings.xml defined", cLog::eLOG_TYPE_ERROR);
		return 1;
	}

	while (xmlElement != nullptr)
	{
		const char* directory = xmlElement->Attribute ("directory");
		if (directory != nullptr)
			directories.push_back (directory);
		else
		{
			std::string msg = string ("Can't read directory-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		if (xmlElement->Attribute ("num"))
			IDList.push_back (xmlElement->IntAttribute ("num"));
		else
		{
			std::string msg = string ("Can't read num-attribute from \"") + xmlElement->Value() + "\" - node";
			Log.write (msg, cLog::eLOG_TYPE_WARNING);
		}

		#ifdef VERY_BROKEN
		// Will be deleted when explosive stuff moves completely to XML fields
		const char* spezial = xmlElement->Attribute ("special");
		if (spezial != nullptr)
		{
			std::string specialString = spezial;
			// TODO: Get rid of this shit. Leave all data references to XML or scripts
			// C++ should not keep so specific data. Really.
			if (specialString == "connector")
				UnitsDataGlobal.setSpecialIDConnector(sID(1, IDList.back()));
			else if (specialString == "landmine")
				UnitsDataGlobal.setSpecialIDLandMine(sID(1, IDList.back()));
			else if (specialString == "seamine")
				UnitsDataGlobal.setSpecialIDSeaMine(sID(1, IDList.back()));
			else
				Log.write ("Unknown spacial in buildings.xml \"" + specialString + "\"", cLog::eLOG_TYPE_WARNING);
		}
		#endif

		xmlElement = xmlElement->NextSiblingElement();
	}

	/*
	// Will be deleted when explosive stuff moves completely to XML fields
	if (UnitsDataGlobal.getSpecialIDConnector().secondPart  == 0)
		Log.write("special \"connector\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDLandMine().secondPart   == 0)
		Log.write("special \"landmine\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	if (UnitsDataGlobal.getSpecialIDSeaMine().secondPart    == 0)
		Log.write("special \"seamine\" missing in buildings.xml", cLog::eLOG_TYPE_WARNING);
	*/

	// Parse all folders that were declared in buildings.xml
	for (const auto& dirname: directories)
	{
		std::string directory = std::string(buldings_folder) + PATH_DELIMITER + dirname;
		std::string xmlPath = directory + PATH_DELIMITER + "data.xml";

		parseDataFile(xmlPath.c_str(), directory.c_str());

		if (cSettings::getInstance().isDebug())
			Log.mark();
	}

	// Dirtsurfaces
	if (!DEDICATED_SERVER)
	{
		spriteTool->resetRefSize();

		std::string dir = cSettings::getInstance().getBuildingsPath() + PATH_DELIMITER;

		GraphicsData.big_rubble = spriteTool->makeSpriteSheet(dir + "dirt_big.pcx", 2, cVector2(128, 128));
		if (auto sprite = spriteTool->makeSpriteSheet(dir + "dirt_big_shw.pcx", 2, cVector2(128, 128)))
		{
			sprite->setAlphaKey(50);
			GraphicsData.big_rubble_shadow = sprite;
		}

		GraphicsData.small_rubble = spriteTool->makeSpriteSheet(dir + "dirt_small.pcx", 2, cVector2(64, 64));
		if(auto sprite = spriteTool->makeSpriteSheet(dir + "dirt_small_shw.pcx", 2, cVector2(64, 64)))
		{
			sprite->setAlphaKey(50);
			GraphicsData.small_rubble_shadow = sprite;
		}
/*
		LoadGraphicToSurface (UnitsUiData.rubbleBig->img_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_big.pcx");
		UnitsUiData.rubbleBig->img = CloneSDLSurface (*UnitsUiData.rubbleBig->img_org);
		LoadGraphicToSurface(UnitsUiData.rubbleBig->img_shadow_original, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_big_shw.pcx");
		UnitsUiData.rubbleBig->img_shadow = CloneSDLSurface(*UnitsUiData.rubbleBig->img_shadow_original);
		if (UnitsUiData.rubbleBig->img_shadow != nullptr)
			SDL_SetSurfaceAlphaMod(UnitsUiData.rubbleBig->img_shadow.get(), 50);

		LoadGraphicToSurface(UnitsUiData.rubbleSmall->img_org, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_small.pcx");
		UnitsUiData.rubbleSmall->img = CloneSDLSurface(*UnitsUiData.rubbleSmall->img_org);
		LoadGraphicToSurface(UnitsUiData.rubbleSmall->img_shadow_original, cSettings::getInstance().getBuildingsPath().c_str(), "dirt_small_shw.pcx");
		UnitsUiData.rubbleSmall->img_shadow = CloneSDLSurface(*UnitsUiData.rubbleSmall->img_shadow_original);
		if (UnitsUiData.rubbleSmall->img_shadow != nullptr)
			SDL_SetSurfaceAlphaMod(UnitsUiData.rubbleSmall->img_shadow.get(), 50);*/
	}
	return 1;
}

//------------------------------------------------------------------------------
void ModData::parseUnitData (tinyxml2::XMLElement* unitDataXml, cStaticUnitData& staticData, const char* directory)
{
	cDynamicUnitData& dynamicData = unitsData->getDynamicData(staticData.ID);
	dynamicData.setId(staticData.ID);

	//read description
	if (tinyxml2::XMLElement* element = unitDataXml->FirstChildElement("Description"))
	{
		std::string description(element->GetText());
		size_t pos;
		while ((pos = description.find("\\n")) != string::npos)
		{
			description.replace(pos, 2, "\n");
		}
		staticData.setDescription(description);
	}

	// Weapon
	for(tinyxml2::XMLElement* weapon = unitDataXml->FirstChildElement("Weapon"); weapon; weapon = weapon->NextSiblingElement("Weapon"))
	{
		string muzzleType = getValueString (weapon, "Muzzle_Type", "Const");
		if (muzzleType.compare ("Big") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_BIG;
		else if (muzzleType.compare("Rocket") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_ROCKET;
		else if (muzzleType.compare("Small") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_SMALL;
		else if (muzzleType.compare("Med") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_MED;
		else if (muzzleType.compare("Med_Long") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_MED_LONG;
		else if (muzzleType.compare("Rocket_Cluster") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_ROCKET_CLUSTER;
		else if (muzzleType.compare("Torpedo") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_TORPEDO;
		else if (muzzleType.compare("Sniper") == 0)
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_SNIPER;
		else
			staticData.muzzleType = cStaticUnitData::MUZZLE_TYPE_NONE;

		dynamicData.setAmmoMax(getValueInt(weapon, "Ammo_Quantity"));
		dynamicData.setShotsMax(getValueInt(weapon, "Shots"));
		dynamicData.setRange(getValueInt(weapon, "Range"));
		dynamicData.setDamage(getValueInt(weapon, "Damage"));
		staticData.canAttack = getValueInt(weapon, "Can_Attack");

		// TODO: make the code differ between attacking sea units and land units.
		// until this is done being able to attack sea units means being able to attack ground units.
		if (staticData.canAttack & TERRAIN_SEA)
			staticData.canAttack |= TERRAIN_GROUND;

		staticData.setFlag(UnitFlag::CanDriveAndFire, getValueBool(weapon, "Can_Drive_And_Fire"));
	};

	// Production
	for(tinyxml2::XMLElement* production = unitDataXml->FirstChildElement("Production"); production; production = production->NextSiblingElement("Production"))
	{
		dynamicData.setBuildCost(getValueInt (production, "Built_Costs"));

		staticData.canBuild = getValueString (production, "Can_Build", "String");
		staticData.buildAs = getValueString(production, "Build_As", "String");
		staticData.maxBuildFactor = getValueInt(production, "Max_Build_Factor");

		staticData.setFlag(UnitFlag::CanBuildPath, getValueBool(production, "Can_Build_Path"));
		staticData.setFlag(UnitFlag::CanBuildRepeat, getValueBool(production, "Can_Build_Repeat"));
	}

	// Movement
	for(tinyxml2::XMLElement* movement = unitDataXml->FirstChildElement("Movement"); movement; movement = movement->NextSiblingElement("Movement"))
	{
		dynamicData.setSpeedMax (getValueInt (movement, "Movement_Sum") * 4);
		staticData.factorGround = getValueFloat (movement, "Factor_Ground");
		staticData.factorSea = getValueFloat(movement, "Factor_Sea");
		staticData.factorAir = getValueFloat(movement, "Factor_Air");
		staticData.factorCoast = getValueFloat(movement, "Factor_Coast");
	}

	// Abilities
	for(tinyxml2::XMLElement* abilities = unitDataXml->FirstChildElement("Abilities"); abilities; abilities = abilities->NextSiblingElement("Abilities"))
	{
		staticData.cellSize = getValueInt(abilities, "Size");
		if(staticData.cellSize < 1)
			staticData.cellSize = 1;

		dynamicData.setArmor(getValueInt(abilities, "Armor"));
		dynamicData.setHitpointsMax(getValueInt(abilities, "Hitpoints"));
		dynamicData.setScan(getValueInt(abilities, "Scan_Range"));

		staticData.modifiesSpeed = getValueFloat (abilities, "Modifies_Speed");
		staticData.convertsGold = getValueInt(abilities, "Converts_Gold");
		staticData.setFlag(UnitFlag::ConnectsToBase, getValueBool(abilities, "Connects_To_Base"));
		staticData.setFlag(UnitFlag::CanClearArea, getValueBool(abilities, "Can_Clear_Area"));
		staticData.setFlag(UnitFlag::CanBeCaptured, getValueBool(abilities, "Can_Be_Captured"));
		staticData.setFlag(UnitFlag::CanBeDisabled, getValueBool(abilities, "Can_Be_Disabled"));
		staticData.setFlag(UnitFlag::CanCapture, getValueBool(abilities, "Can_Capture"));
		staticData.setFlag(UnitFlag::CanDisable, getValueBool(abilities, "Can_Disable"));
		staticData.setFlag(UnitFlag::CanRepair, getValueBool(abilities, "Can_Repair"));
		staticData.setFlag(UnitFlag::CanRearm, getValueBool(abilities, "Can_Rearm"));
		staticData.setFlag(UnitFlag::CanResearch, getValueBool(abilities, "Can_Research"));
		staticData.setFlag(UnitFlag::CanPlaceMines, getValueBool(abilities, "Can_Place_Mines"));
		staticData.setFlag(UnitFlag::CanSurvey, getValueBool(abilities, "Can_Survey"));
		staticData.setFlag(UnitFlag::DoesSelfRepair, getValueBool(abilities, "Does_Self_Repair"));

		staticData.setFlag(UnitFlag::CanSelfDestroy, getValueBool(abilities, "Can_Self_Destroy"));
		staticData.setFlag(UnitFlag::CanScore, getValueBool(abilities, "Can_Score"));

		staticData.canMineMaxRes = getValueInt(abilities, "Can_Mine_Max_Resource");

		staticData.needsMetal = getValueInt(abilities, "Needs_Metal");
		staticData.needsOil = getValueInt(abilities, "Needs_Oil");
		staticData.needsEnergy = getValueInt(abilities, "Needs_Energy");
		staticData.needsHumans = getValueInt(abilities, "Needs_Humans");
		if (staticData.needsEnergy < 0)
		{
			staticData.produceEnergy = abs(staticData.needsEnergy);
			staticData.needsEnergy = 0;
		}
		else
			staticData.produceEnergy = 0;
		if (staticData.needsHumans < 0)
		{
			staticData.produceHumans = abs(staticData.needsHumans);
			staticData.needsHumans = 0;
		}
		else
			staticData.produceHumans = 0;

		staticData.isStealthOn = getValueInt(abilities, "Is_Stealth_On");
		staticData.canDetectStealthOn = getValueInt(abilities, "Can_Detect_Stealth_On");

		string surfacePosString = getValueString (abilities, "Surface_Position", "Const");
		if (surfacePosString.compare("BeneathSea") == 0)
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_BENEATH_SEA;
		else if (surfacePosString.compare("AboveSea") == 0)
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE_SEA;
		else if (surfacePosString.compare("Base") == 0)
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_BASE;
		else if (surfacePosString.compare("AboveBase") == 0)
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE_BASE;
		else if (surfacePosString.compare("Above") == 0)
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_ABOVE;
		else
			staticData.surfacePosition = cStaticUnitData::SURFACE_POS_GROUND;

		string overbuildString = getValueString (abilities, "Const", "Can_Be_Overbuild");
		if (overbuildString.compare ("Yes") == 0)
			staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_YES;
		else if (overbuildString.compare ("YesNRemove") == 0)
			staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_YESNREMOVE;
		else
			staticData.canBeOverbuild = cStaticUnitData::OVERBUILD_TYPE_NO;

		staticData.setFlag(UnitFlag::CanBeLandedOn, getValueBool (abilities, "Can_Be_Landed_On"));
		staticData.setFlag(UnitFlag::CanWork, getValueBool(abilities, "Is_Activatable"));
		staticData.setFlag(UnitFlag::ExplodesOnContact, getValueBool(abilities, "Explodes_On_Contact"));
		staticData.setFlag(UnitFlag::IsHuman, getValueBool(abilities, "Is_Human"));
	}

	// Storage
	for(tinyxml2::XMLElement* storage = unitDataXml->FirstChildElement("Storage"); storage; storage = storage->NextSiblingElement("Storage"))
	{
		staticData.storageResMax = getValueInt(storage, "Capacity_Resources");

		string storeResString = getValueString (storage, "Capacity_Res_Type", "Const");
		if (storeResString.compare("Metal") == 0)
			staticData.storeResType = eResourceType::Metal;
		else if (storeResString.compare("Oil") == 0)
			staticData.storeResType = eResourceType::Oil;
		else if (storeResString.compare("Gold") == 0)
			staticData.storeResType = eResourceType::Gold;
		else
			staticData.storeResType = eResourceType::None;

		staticData.storageUnitsMax = getValueInt(storage, "Capacity_Units");

		string storeUnitImgString = getValueString (storage, "Capacity_Units_Image_Type", "Const");
		if (storeUnitImgString.compare("Plane") == 0)
			staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_PLANE;
		else if (storeUnitImgString.compare("Human") == 0)
			staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_HUMAN;
		else if (storeUnitImgString.compare("Tank") == 0)
			staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_TANK;
		else if (storeUnitImgString.compare("Ship") == 0)
			staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_SHIP;
		else
			staticData.storeUnitsImageType = cStaticUnitData::STORE_UNIT_IMG_TANK;

		string storeUnitsString = getValueString (storage, "Capacity_Units_Type", "String");
		Split(storeUnitsString, "+", staticData.storeUnitsTypes);

		staticData.isStorageType = getValueString(storage, "Is_Storage_Type", "String");
	}

	// Parsing all <Graphic> blocks
	bool foundCustomGraphics = false;
	for(tinyxml2::XMLElement* graphic = unitDataXml->FirstChildElement("Graphic"); graphic; graphic = graphic->NextSiblingElement("Graphic"))
	{
		spriteTool->reset();
		// Set up base sprite size for all custom sprites
		if(graphic->Attribute("refsize"))
		{
			cPosition size = this->getAttribPos(graphic, "refsize");
			if(size.x() > 0 && size.y() > 0)
			{
				spriteTool->setRefSize(size);
			}
		}
		staticData.hasCorpse = getValueBool(graphic, "Has_Corpse");
		staticData.hasDamageEffect = getValueBool(graphic, "Has_Damage_Effect");
		staticData.hasPlayerColor = getValueBool(graphic, "Has_Player_Color");
		staticData.buildUpGraphic = getValueBool(graphic, "Build_Up");
		staticData.powerOnGraphic = getValueBool(graphic, "Power_On");
		staticData.setFlag(UnitFlag::HasAnimationMovement, getValueBool(graphic, "Movement"));

		/// Iterate through custom graphic elements
		for(XMLElement* gobj = graphic->FirstChildElement(); gobj; gobj = gobj->NextSiblingElement())
		{
			if(parseGraphicObject(gobj, staticData, (std::string(directory)+PATH_DELIMITER).c_str()))
				foundCustomGraphics = true;
		}
	}

	if(foundCustomGraphics)
		staticData.customGraphics = true;

	// load sounds
	Log.write (" - loading sounds", cLog::eLOG_TYPE_DEBUG);
	LoadUnitSoundfile (staticData.Wait,       directory, "wait.ogg");
	LoadUnitSoundfile (staticData.WaitWater,  directory, "wait_water.ogg");
	LoadUnitSoundfile (staticData.Start,      directory, "start.ogg");
	LoadUnitSoundfile (staticData.StartWater, directory, "start_water.ogg");
	LoadUnitSoundfile (staticData.Stop,       directory, "stop.ogg");
	LoadUnitSoundfile (staticData.StopWater,  directory, "stop_water.ogg");
	LoadUnitSoundfile (staticData.Drive,      directory, "drive.ogg");
	LoadUnitSoundfile (staticData.DriveWater, directory, "drive_water.ogg");
	LoadUnitSoundfile (staticData.Attack,     directory, "attack.ogg");
	LoadUnitSoundfile (staticData.Running, directory, "running.ogg");
}

std::shared_ptr<cSprite> ModData::makeSprite(tinyxml2::XMLElement* xml, const char* directory)
{
	std::string dir = directory;
	std::string file;
	if(const char* attr = xml->Attribute("file"))
		file = attr;

	// Example tag: <Sprite file="shw.pcx" layer="shadow"/>
	if(file.empty() || !FileExists(dir+file))
	{
		Log.write (" - <Sprite> should contain proper \"file\" attribute", cLog::eLOG_TYPE_DEBUG);
		return nullptr;
	}
	cSpritePtr gobject = spriteTool->makeSprite(dir+file);

	parseSpriteAttributes(xml, *gobject);
	return gobject;
}

std::shared_ptr<cSpriteList> ModData::makeSpriteList(tinyxml2::XMLElement* xml, const char* directory)
{
	// Example tags: <SpriteList pattern="img%d.pcx" frames="8" channel="direction" layer="main" colorkey="auto"/>
	std::string dir = directory;
	int frames = xml->IntAttribute("frames");
	if(frames < 1)
		frames = 1;

	std::string pattern;
	if(const char* attr = xml->Attribute("pattern"))
		pattern = attr;
	else
	{
		Log.write (" - <MultiSprite> should contain proper \"pattern\" attribute", cLog::eLOG_TYPE_DEBUG);
		return nullptr;
	}

	std::list<std::string> files;
	char tmp[255];
	for(int i = 0; i < frames; i++)
	{
		snprintf(tmp, 254, pattern.c_str(), i);
		std::string file = dir + tmp;
		files.push_back(file);
	}

	cSpriteListPtr gobject = spriteTool->makeSpriteList(files);

	if(const char* attr = xml->Attribute("channel"))
		gobject->setChannel(attr);

	// Set color key
	parseSpriteAttributes(xml, *gobject);
	return gobject;
}

std::shared_ptr<cSpriteList> ModData::makeSpriteSheet(tinyxml2::XMLElement* xml, const char* directory)
{
	// Example tag: <VariantSprite file="img.pcx" frames="9" channel="clan" layer="main"/>

	std::string dir = directory;
	std::string file;
	if(const char* attr = xml->Attribute("file"))
		file = attr;

	int frames = xml->IntAttribute("frames");
	if(frames < 1)
		frames = 1;

	if(file.empty() || !FileExists(dir+file))
	{
		Log.write (" - <MultiSprite> should contain proper \"file\" attribute", cLog::eLOG_TYPE_DEBUG);
		return nullptr;
	}

	cSpriteListPtr gobject = spriteTool->makeSpriteSheet(directory + file, frames);

	if(const char* attr = xml->Attribute("channel"))
		gobject->setChannel(attr);

	parseSpriteAttributes(xml, *gobject);
	return gobject;
}

// Parses common graphic attributes
bool ModData::parseSpriteAttributes(tinyxml2::XMLElement* gobj, cSprite& sprite)
{
	XmlColor color;
	// Set color key
	if(const char* attr = gobj->Attribute("colorkey"))
	{
		if(parseColor(attr, color))
		{
			if(color.isAuto)
			{
				sprite.setColorKeyAuto();
			}
			else
			{
				assert(spriteTool);
				auto clr = color.a >= 0 ?
							spriteTool->mapRGB(color.r, color.g, color.b) :
							spriteTool->mapRGBA(color.r, color.g, color.b, color.a);
				sprite.setColorKey(clr);
			}
			return true;
		}
	}
	return false;
}

bool ModData::parseGraphicObject(tinyxml2::XMLElement* xml, cStaticUnitData& staticData, const char* directory)
{
	// TODO: This loader is still not complete. Complete version should support recursive graphic objects
	std::string type = xml->Name();
	cRenderablePtr gobject;

	if(type == "Sprite")
	{
		gobject = this->makeSprite(xml, directory);
	}
	else if(type == "SpriteSheet")
	{
		gobject = this->makeSpriteSheet(xml, directory);
	}
	else if(type == "SpriteList")
	{
		gobject = this->makeSpriteList(xml, directory);
	}

	// Add graphic object to the unit
	if(gobject)
	{
		std::string layer;
		if(const char* attr = xml->Attribute("layer"))
			layer = attr;
		staticData.setGraphics(layer, gobject);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
int ModData::loadClans()
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

	tinyxml2::XMLElement* xmlStart = clansXml.FirstChildElement ("Clans");
	if (xmlStart == 0)
	{
		Log.write ("Can't read \"Clans\" node!", cLog::eLOG_TYPE_ERROR);
		return 0;
	}

	for (tinyxml2::XMLElement* xml = xmlStart->FirstChildElement ("Clan"); xml; xml = xml->NextSiblingElement ("Clan"))
	{
		string nameAttr = xml->Attribute ("Name");

		cClan* newClan = ClanDataGlobal.makeClan(nameAttr.c_str());

		if(const char * attr = xml->Attribute("mode"))
		{
			if(std::string(attr) != "append")
			{
				newClan->resetAll();
			}
		}

		const XMLElement* descriptionNode = xml->FirstChildElement ("Description");
		if (descriptionNode)
		{
			string descriptionString = descriptionNode->GetText();
			newClan->setDescription (descriptionString);
		}

		for (XMLElement* statsElement = xml->FirstChildElement ("ChangedUnitStat"); statsElement; statsElement = statsElement->NextSiblingElement ("ChangedUnitStat"))
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

		// Find all embarcation blocks
		for (tinyxml2::XMLElement* block = xml->FirstChildElement("Embark"); block; block = block->NextSiblingElement ("Embark"))
		{
			// Iterate through all embark items
			for(tinyxml2::XMLElement* item = block->FirstChildElement(); item; item = item->NextSiblingElement())
			{
				if(!item->Name())
					continue;

				std::string name = item->Name();
				const char* rawid = item->Attribute("ID");
				sID id;

				if(name == "Building")
				{
					if(parseSID(rawid, id))
					{
						cBaseLayoutItem building;
						building.pos = getAttribPos(item, "pos");
						building.ID = id;
						newClan->addEmbarkBuilding(building);
					}
					else
					{
						Log.write ("Embarcation rule for Building must contain valid 'ID'!", cLog::eLOG_TYPE_ERROR);
					}
				}
				else if(name == "Vehicle")
				{
					if(parseSID(rawid, id))
					{
						sLandingUnit unit;
						unit.unitID = id;
						unit.cargo = item->FloatAttribute("metal");
						unit.minCredits = item->IntAttribute("mincredits");
						unit.isDefault = true;
						newClan->addEmbarkUnit(unit);
					}
				}
			}
		}
	}

	return 1;
}

// Tries to parse color attribute
/*static*/ bool ModData::parseColor(const char* rawdata, XmlColor& color, const char* delim)
{
	std::vector<std::string> parts;
	std::string value = rawdata;

	if(value == "auto")
	{
		color.isAuto = true;
		return true;
	}

	// Splitting the string to 3 or more parts
	size_t start = 0;
	size_t delim_pos = 0;
	size_t delim_len = delim ? strlen(delim) : 0;

	if(!delim || !delim_len)
	{
		throw std::runtime_error("ModData::parseColor - invalid delimiter");
	}

	do
	{
		delim_pos = value.find(delim, start);
		if(delim_pos != std::string::npos)
		{
			std::string part(value.begin() + start, value.begin() + delim_pos);
			parts.push_back(part);
			start = delim_pos + delim_len;
		}
		else
		{
			// That was the last one
			std::string part(value.begin() + start, value.end());
			parts.push_back(part);
		}
	}while(delim_pos != std::string::npos && start < value.size());

	if(parts.size() == 3)
	{
		color.r = atoi(parts[0].c_str());
		color.g = atoi(parts[1].c_str());
		color.b = atoi(parts[2].c_str());
		return true;
	}
	else if(parts.size() == 4)
	{
		color.r = atoi(parts[0].c_str());
		color.g = atoi(parts[1].c_str());
		color.b = atoi(parts[2].c_str());
		color.a = atoi(parts[3].c_str());
	}
	return false;
}

/*static*/ bool ModData::parseSID(const char* rawvalue, sID& id, const char* delim)
{
	std::string value = rawvalue;
	if(value.empty())
		return false;

	auto delim_pos = value.find (delim, 0);
	if(delim_pos != std::string::npos)
	{
		id.firstPart = atoi (value.substr (0, delim_pos).c_str());
		id.secondPart = atoi (value.substr(delim_pos, value.length()).c_str());
	}
	else
	{
		return false;
	}
	return true;
}

/*static*/ cPosition ModData::getAttribPos(tinyxml2::XMLElement* element, const char* name, cPosition default_)
{
	if (element->Attribute(name))
	{
		cPosition result;
		std::string value = element->Attribute(name);
		const char* delim = ";";
		//pos="0;-3"
		auto delim_pos = value.find (delim, 0);
		if(delim_pos != std::string::npos)
		{
			auto str_a = value.substr(0, delim_pos);
			auto str_b = value.substr(delim_pos+1, value.length());
			result.x() = atoi (str_a.c_str());
			result.y() = atoi (str_b.c_str());
		}
		return result;
	}
	return default_;
}

/*static*/ int ModData::getValueInt(tinyxml2::XMLElement* block, const char* name, int default_)
{
	if(XMLElement* element = block->FirstChildElement(name))
	{
		if (element->Attribute ("Num"))
		{
			return element->IntAttribute("Num");
		}
	}

	return default_;
}

/*static*/ float ModData::getValueFloat(tinyxml2::XMLElement* block, const char* name, float default_)
{
	if(XMLElement* element = block->FirstChildElement(name))
	{
		if (element->Attribute ("Num"))
		{
			return element->FloatAttribute("Num");
		}
	}

	return default_;
}

/*static*/ std::string ModData::getValueString(tinyxml2::XMLElement* block, const char* name, const char* attrib, const char* default_)
{
	if(XMLElement* element = block->FirstChildElement(name))
	{
		if (element->Attribute (attrib))
		{
			return element->Attribute(attrib);
		}
	}

	return default_;
}

/*static*/ bool ModData::getValueBool(tinyxml2::XMLElement* block, const char* name, bool default_)
{
	if(XMLElement* element = block->FirstChildElement(name))
	{
		if (const char * xmlValue = element->Attribute ("YN"))
		{
			std::string value = xmlValue;
			std::transform(value.begin(), value.end(), value.begin(), ::tolower);

			if (value == "true" ||
				value == "y" ||
				value == "yes")
			{
				return true;
			}
			else if (value == "false" ||
				value == "n" ||
				value == "no")
			{
				return false;
			}
		}
	}

	return default_;
}
