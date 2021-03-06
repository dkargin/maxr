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

#include "game/data/map/map.h"

#include "game/data/units/building.h"
#include "utility/listhelpers.h"
#include "utility/files.h"
#include "utility/crc.h"
#include "utility/log.h"
#include "game/data/player/player.h"
#include "settings.h"
#include "game/data/units/vehicle.h"
#include "video.h"
#include "utility/position.h"
#include "game/data/model.h"
#include "mapdownload.h"

#if 1 // TODO: [SDL2]: SDL_SetColors
inline void SDL_SetColors (SDL_Surface* surface, SDL_Color* colors, int index, int size)
{
	SDL_SetPaletteColors (surface->format->palette, colors, index, size);
}
#endif


sTerrain::sTerrain() :
	water (false),
	coast (false),
	blocked (false)
{}

sTerrain::sTerrain(sTerrain&& other) :
	sf(std::move(other.sf)),
	sf_org(std::move(other.sf_org)),
	shw(std::move(other.shw)),
	shw_org(std::move(other.shw_org)),
	water(other.water),
	coast(other.coast),
	blocked(other.blocked)
{}

sTerrain& sTerrain::operator=(sTerrain&& other)
{
	sf = std::move(other.sf);
	sf_org = std::move(other.sf_org);
	shw = std::move(other.shw);
	shw_org = std::move(other.shw_org);
	water = other.water;
	coast = other.coast;
	blocked = other.blocked;

	return *this;
}

cMapField::cMapField()
{}

cVehicle* cMapField::getVehicle() const
{
	if (vehicles.empty()) return nullptr;
	return vehicles[0];
}

cVehicle* cMapField::getPlane() const
{
	if (planes.empty()) return nullptr;
	return planes[0];
}

const std::vector<cBuilding*>& cMapField::getBuildings() const
{
	return buildings;
}

const std::vector<cVehicle*>& cMapField::getVehicles() const
{
	return vehicles;
}

const std::vector<cVehicle*>& cMapField::getPlanes() const
{
	return planes;
}

void cMapField::getUnits (std::vector<cUnit*>& units) const
{
	units.clear();
	units.reserve (vehicles.size() + buildings.size() + planes.size());
	units.insert (units.end(), vehicles.begin(), vehicles.end());
	units.insert (units.end(), buildings.begin(), buildings.end());
	units.insert (units.end(), planes.begin(), planes.end());
}

cBuilding* cMapField::getBuilding() const
{
	if (buildings.empty()) return nullptr;
	return buildings[0];
}

cBuilding* cMapField::getTopBuilding() const
{
	if (buildings.empty()) return nullptr;
	cBuilding* building = buildings[0];

	if ((building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_GROUND ||
		 building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE) &&
		!building->isRubble())
		return building;
	return nullptr;
}

cBuilding* cMapField::getBaseBuilding() const
{
	for (cBuilding* building : buildings)
	{
		if (building->getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_GROUND &&
			building->getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE &&
			!building->isRubble())
		{
			return building;
		}
	}
	return nullptr;
}

cBuilding* cMapField::getRubble() const
{
	for (cBuilding* building : buildings)
		if (building->isRubble())
			return building;
	return nullptr;
}

cBuilding* cMapField::getMine() const
{
	for (cBuilding* building : buildings)
		if (building->getStaticUnitData().hasFlag(UnitFlag::ExplodesOnContact))
			return building;
	return nullptr;
}

bool cMapField::hasBridgeOrPlattform() const
{
	for (cBuilding* building : buildings)
	{
		if ((building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_SEA ||
			building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_BASE) &&
			!building->isRubble())
		{
			return true;
		}
	}
	return false;
}

void cMapField::addBuilding (cBuilding& building, size_t index)
{
	assert (index <= buildings.size());
	buildings.insert (buildings.begin() + index, &building);

	buildingsChanged();
	unitsChanged();
}
void cMapField::addVehicle (cVehicle& vehicle, size_t index)
{
	assert (index <= vehicles.size());
	vehicles.insert (vehicles.begin() + index, &vehicle);

	vehiclesChanged();
	unitsChanged();
}

void cMapField::addPlane (cVehicle& plane, size_t index)
{
	assert (index <= planes.size());
	planes.insert (planes.begin() + index, &plane);

	planesChanged();
	unitsChanged();
}

void cMapField::removeBuilding (const cBuilding& building)
{
	Remove (buildings, &building);

	buildingsChanged();
	unitsChanged();
}

void cMapField::removeVehicle (const cVehicle& vehicle)
{
	Remove (vehicles, &vehicle);

	vehiclesChanged();
	unitsChanged();
}

void cMapField::removePlane (const cVehicle& plane)
{
	Remove (planes, &plane);

	planesChanged();
	unitsChanged();
}

void cMapField::removeAll()
{
	buildings.clear();
	vehicles.clear();
	planes.clear();

	buildingsChanged();
	vehiclesChanged();
	planesChanged();
	unitsChanged();
}

// cStaticMap //////////////////////////////////////////////////

cStaticMap::cStaticMap() : size(0), terrains(), crc(0)
{
}

cStaticMap::~cStaticMap()
{
}

const sTerrain& cStaticMap::getTerrain (const cPosition& position) const
{
	return terrains[Kacheln[getOffset (position)]];
}

bool cStaticMap::isBlocked (const cPosition& position) const
{
	return getTerrain (position).blocked;
}

bool cStaticMap::isCoast (const cPosition& position) const
{
	return getTerrain (position).coast;
}

bool cStaticMap::isWater (const cPosition& position) const
{
	return getTerrain (position).water;
}

bool cStaticMap::isGround(const cPosition& position) const
{
	return !getTerrain(position).water && !getTerrain(position).coast;
}

bool cStaticMap::possiblePlace(const cStaticUnitData& data, const cPosition& position) const
{
	int size = data.cellSize;

	// Check if we are inside the borders
	if (!isValidPosition(position))
		return false;

	// Check if another corner fits the map
	if (size > 1)
	{
		if (!isValidPosition(position + cPosition(size-1, size-1)))
		{
			return false;
		}
	}

	if (data.factorAir > 0)
		return true;

	int sea = 0;
	int coast = 0;
	int ground = 0;

	for(int y = 0; y < size; y++)
		for(int x = 0; x < size; x++)
		{
			auto pos = position.relative(x,y);
			if (isBlocked(pos))
				return false;
			if(isWater(pos))
				sea++;
			if(isCoast(pos))
				coast++;
			if(!isGround(pos))
				ground++;
		}

	// Check for the units which can not stand on sea tiles
	if (data.factorSea == 0 && sea > 0)
		return false;

	// Check for the units which can not stand on coastal tiles
	if (data.factorCoast == 0 && coast > 0)
		return false;

	// Check for the units which can not stand on ground tiles
	if (data.factorGround == 0 && ground > 0)
		return false;

	return true;
}

bool cStaticMap::isValidPosition (const cPosition& position) const
{
	return 0 <= position.x() && position.x() < size && 0 <= position.y() && position.y() < size;
}

void cStaticMap::clear()
{
	filename.clear();
	size = 0;
	crc = 0;
	terrains.clear();
	Kacheln.clear();
}

bool cStaticMap::isValid() const
{
	return !filename.empty();
}

/** Loads a map file */
bool cStaticMap::loadMap (const std::string& filename_)
{
	clear();
	// Open File
	filename = filename_;
	Log.write ("Loading map \"" + filename_ + "\"", cLog::eLOG_TYPE_DEBUG);

	// first try in the factory maps directory
	std::string fullFilename = cSettings::getInstance().getMapsPath() + PATH_DELIMITER + filename;
	SDL_RWops* fpMapFile = SDL_RWFromFile (fullFilename.c_str(), "rb");
	if (fpMapFile == nullptr)
	{
		// now try in the user's map directory
		std::string userMapsDir = getUserMapsDir();
		if (!userMapsDir.empty())
		{
			fullFilename = userMapsDir + filename;
			fpMapFile = SDL_RWFromFile (fullFilename.c_str(), "rb");
		}
	}
	if (fpMapFile == nullptr)
	{
		Log.write ("Cannot load map file: \"" + filename + "\"", cLog::eLOG_TYPE_WARNING);
		clear();
		return false;
	}
	char szFileTyp[4];

	// check for typ
	SDL_RWread (fpMapFile, &szFileTyp, 1, 3);
	szFileTyp[3] = '\0';
	// WRL - interplays original mapformat
	// WRX - mapformat from russian mapeditor
	// DMO - for some reason some original maps have this filetype
	if (strcmp (szFileTyp, "WRL") != 0 && strcmp (szFileTyp, "WRX") != 0 && strcmp (szFileTyp, "DMO") != 0)
	{
		Log.write ("Wrong file format: \"" + filename + "\"", cLog::eLOG_TYPE_WARNING);
		SDL_RWclose (fpMapFile);
		clear();
		return false;
	}
	SDL_RWseek (fpMapFile, 2, SEEK_CUR);

	// Read informations and get positions from the map-file
	const short sWidth = SDL_ReadLE16 (fpMapFile);
	Log.write ("SizeX: " + iToStr (sWidth), cLog::eLOG_TYPE_DEBUG);
	const short sHeight = SDL_ReadLE16 (fpMapFile);
	Log.write ("SizeY: " + iToStr (sHeight), cLog::eLOG_TYPE_DEBUG);
	SDL_RWseek (fpMapFile, sWidth * sHeight, SEEK_CUR); // Ignore Mini-Map
	const Sint64 iDataPos = SDL_RWtell (fpMapFile); // Map-Data
	SDL_RWseek (fpMapFile, sWidth * sHeight * 2, SEEK_CUR);
	const int iNumberOfTerrains = SDL_ReadLE16 (fpMapFile); // Read PicCount
	Log.write ("Number of terrains: " + iToStr (iNumberOfTerrains), cLog::eLOG_TYPE_DEBUG);
	const Sint64 iGraphicsPos = SDL_RWtell (fpMapFile); // Terrain Graphics
	const Sint64 iPalettePos = iGraphicsPos + iNumberOfTerrains * 64 * 64; // Palette
	const Sint64 iInfoPos = iPalettePos + 256 * 3; // Special informations

	if (sWidth != sHeight)
	{
		Log.write ("Map must be quadratic!: \"" + filename + "\"", cLog::eLOG_TYPE_WARNING);
		SDL_RWclose (fpMapFile);
		clear();
		return false;
	}

	// Generate new Map
	this->size = std::max<int> (16, sWidth);
	Kacheln.resize (size * size, 0);
	terrains.resize(iNumberOfTerrains);

	// Load Color Palette
	SDL_RWseek (fpMapFile, iPalettePos, SEEK_SET);
	for (int i = 0; i < 256; i++)
	{
		SDL_RWread (fpMapFile, palette + i, 3, 1);
	}
	//generate palette for terrains with fog
	for (int i = 0; i < 256; i++)
	{
		palette_shw[i].r = (unsigned char) (palette[i].r * 0.6f);
		palette_shw[i].g = (unsigned char) (palette[i].g * 0.6f);
		palette_shw[i].b = (unsigned char) (palette[i].b * 0.6f);
		palette[i].a = 255;
		palette_shw[i].a = 255;
	}

	// Load necessary Terrain Graphics
	for (int iNum = 0; iNum < iNumberOfTerrains; iNum++)
	{
		unsigned char cByte; // one Byte
		// load terrain type info
		SDL_RWseek (fpMapFile, iInfoPos + iNum, SEEK_SET);
		SDL_RWread (fpMapFile, &cByte, 1, 1);

		switch (cByte)
		{
			case 0:
				//normal terrain without special property
				break;
			case 1:
				terrains[iNum].water = true;
				break;
			case 2:
				terrains[iNum].coast = true;
				break;
			case 3:
				terrains[iNum].blocked = true;
				break;
			default:
				Log.write ("unknown terrain type " + iToStr (cByte) + " on tile " + iToStr (iNum) + " found. Handled as blocked!", cLog::eLOG_TYPE_WARNING);
				terrains[iNum].blocked = true;
				//SDL_RWclose (fpMapFile);
				//return false;
		}

		//load terrain graphic
		AutoSurface surface (cStaticMap::loadTerrGraph (fpMapFile, iGraphicsPos, palette, iNum));
		if (surface == nullptr)
		{
			Log.write ("EOF while loading terrain number " + iToStr (iNum), cLog::eLOG_TYPE_WARNING);
			SDL_RWclose (fpMapFile);
			clear();
			return false;
		}
		copySrfToTerData (*surface, iNum);
	}

	// Load map data
	SDL_RWseek (fpMapFile, iDataPos, SEEK_SET);
	for (int iY = 0; iY < size; iY++)
	{
		for (int iX = 0; iX < size; iX++)
		{
			int Kachel = SDL_ReadLE16 (fpMapFile);
			if (Kachel >= iNumberOfTerrains)
			{
				Log.write ("a map field referred to a nonexisting terrain: " + iToStr (Kachel), cLog::eLOG_TYPE_WARNING);
				SDL_RWclose (fpMapFile);
				clear();
				return false;
			}
			Kacheln[iY * size + iX] = Kachel;
		}
	}
	SDL_RWclose (fpMapFile);

	//save crc, to check map file equality when loading a game
	crc = MapDownload::calculateCheckSum(filename);
	return true;
}

/*static*/AutoSurface cStaticMap::loadTerrGraph (SDL_RWops* fpMapFile, Sint64 iGraphicsPos, const SDL_Color (&colors)[256], int iNum)
{
	// Create new surface and copy palette
	AutoSurface surface (SDL_CreateRGBSurface (0, 64, 64, 8, 0, 0, 0, 0));
	surface->pitch = surface->w;

	SDL_SetPaletteColors (surface->format->palette, colors, 0, 256);

	// Go to position of filedata
	SDL_RWseek (fpMapFile, iGraphicsPos + 64 * 64 * (iNum), SEEK_SET);

	// Read pixel data and write to surface
	if (SDL_RWread (fpMapFile, surface->pixels, 1, 64 * 64) != 64 * 64) return 0;
	return surface;
}

/*static*/AutoSurface cStaticMap::loadMapPreview (const std::string& mapName, int* mapSize)
{
	std::string mapPath = cSettings::getInstance().getMapsPath() + PATH_DELIMITER + mapName;
	// if no factory map of that name exists, try the custom user maps

	SDL_RWops* mapFile = SDL_RWFromFile (mapPath.c_str(), "rb");
	if (mapFile == nullptr && !getUserMapsDir().empty())
	{
		mapPath = getUserMapsDir() + mapName;
		mapFile = SDL_RWFromFile (mapPath.c_str(), "rb");
	}

	if (mapFile == nullptr) return nullptr;

	SDL_RWseek (mapFile, 5, SEEK_SET);
	int size = SDL_ReadLE16 (mapFile);
	struct { unsigned char cBlue, cGreen, cRed; } Palette[256];
	short sGraphCount;
	SDL_RWseek (mapFile, 2 + size * size * 3, SEEK_CUR);
	sGraphCount = SDL_ReadLE16 (mapFile);
	SDL_RWseek (mapFile, 64 * 64 * sGraphCount, SEEK_CUR);
	SDL_RWread (mapFile, &Palette, 3, 256);

	AutoSurface mapSurface (SDL_CreateRGBSurface (0, size, size, 8, 0, 0, 0, 0));
	mapSurface->pitch = mapSurface->w;

	mapSurface->format->palette->ncolors = 256;
	for (int j = 0; j < 256; j++)
	{
		mapSurface->format->palette->colors[j].r = Palette[j].cBlue;
		mapSurface->format->palette->colors[j].g = Palette[j].cGreen;
		mapSurface->format->palette->colors[j].b = Palette[j].cRed;
	}
	SDL_RWseek (mapFile, 9, SEEK_SET);
	const int byteReadCount = SDL_RWread (mapFile, mapSurface->pixels, 1, size * size);
	SDL_RWclose (mapFile);

	if (byteReadCount != size * size)
	{
		// error.
		return nullptr;
	}
	const int MAPWINSIZE = 112;
	if (mapSurface->w != MAPWINSIZE || mapSurface->h != MAPWINSIZE) // resize map
	{
		mapSurface = AutoSurface (scaleSurface (mapSurface.get(), nullptr, MAPWINSIZE, MAPWINSIZE));
	}

	if (mapSize != nullptr) *mapSize = size;
	return mapSurface;
}

uint32_t cStaticMap::getChecksum(uint32_t crc)
{
	return calcCheckSum(this->crc, crc);
}

void cStaticMap::copySrfToTerData (SDL_Surface& surface, int iNum)
{
	//before the surfaces are copied, the colortable of both surfaces has to be equal
	//This is needed to make sure, that the pixeldata is copied 1:1

	//copy the normal terrains
	terrains[iNum].sf_org = AutoSurface (SDL_CreateRGBSurface (0, 64, 64, 8, 0, 0, 0, 0));
	SDL_SetPaletteColors (terrains[iNum].sf_org->format->palette, surface.format->palette->colors, 0, 256);
	SDL_BlitSurface (&surface, nullptr, terrains[iNum].sf_org.get(), nullptr);

	terrains[iNum].sf = AutoSurface (SDL_CreateRGBSurface (0, 64, 64, 8, 0, 0, 0, 0));
	SDL_SetPaletteColors (terrains[iNum].sf->format->palette, surface.format->palette->colors, 0, 256);
	SDL_BlitSurface (&surface, nullptr, terrains[iNum].sf.get(), nullptr);

	//copy the terrains with fog
	terrains[iNum].shw_org = AutoSurface (SDL_CreateRGBSurface (0, 64, 64, 8, 0, 0, 0, 0));
	SDL_SetColors (terrains[iNum].shw_org.get(), surface.format->palette->colors, 0, 256);
	SDL_BlitSurface (&surface, nullptr, terrains[iNum].shw_org.get(), nullptr);

	terrains[iNum].shw = AutoSurface (SDL_CreateRGBSurface (0, 64, 64, 8, 0, 0, 0, 0));
	SDL_SetColors (terrains[iNum].shw.get(), surface.format->palette->colors, 0, 256);
	SDL_BlitSurface (&surface, nullptr, terrains[iNum].shw.get(), nullptr);

	//now set the palette for the fog terrains
	SDL_SetColors (terrains[iNum].shw_org.get(), palette_shw, 0, 256);
	SDL_SetColors (terrains[iNum].shw.get(), palette_shw, 0, 256);
}

void cStaticMap::scaleSurfaces (int pixelSize)
{
	for (sTerrain& t : terrains)
	{
		scaleSurface (t.sf_org.get(), t.sf.get(), pixelSize, pixelSize);
		scaleSurface (t.shw_org.get(), t.shw.get(), pixelSize, pixelSize);
	}
}

void cStaticMap::generateNextAnimationFrame()
{
	//change palettes to display next frame
	SDL_Color temp = palette[127];
	memmove (palette + 97, palette + 96, 32 * sizeof (SDL_Color));
	palette[96]  = palette[103];
	palette[103] = palette[110];
	palette[110] = palette[117];
	palette[117] = palette[123];
	palette[123] = temp;

	temp = palette_shw[127];
	memmove (palette_shw + 97, palette_shw + 96, 32 * sizeof (SDL_Color));
	palette_shw[96]  = palette_shw[103];
	palette_shw[103] = palette_shw[110];
	palette_shw[110] = palette_shw[117];
	palette_shw[117] = palette_shw[123];
	palette_shw[123] = temp;

	//set the new palette for all terrain surfaces
	for (sTerrain& terrain : terrains)
	{
		SDL_SetColors (terrain.sf.get(), palette + 96, 96, 127);
		//SDL_SetColors (TerrainInUse[i]->sf_org, palette + 96, 96, 127);
		SDL_SetColors (terrain.shw.get(), palette_shw + 96, 96, 127);
		//SDL_SetColors (TerrainInUse[i]->shw_org, palette_shw + 96, 96, 127);
	}
}

AutoSurface cStaticMap::createBigSurface (int sizex, int sizey) const
{
	AutoSurface mapSurface (SDL_CreateRGBSurface (0, sizex, sizey, Video.getColDepth(), 0, 0, 0, 0));

	if (SDL_MUSTLOCK (mapSurface.get())) SDL_LockSurface (mapSurface.get());
	for (int x = 0; x < mapSurface->w; ++x)
	{
		const int terrainx = std::min ((x * size) / mapSurface->w, size - 1);
		const int offsetx = ((x * size) % mapSurface->w) * 64 / mapSurface->w;

		for (int y = 0; y < mapSurface->h; y++)
		{
			const int terrainy = std::min ((y * size) / mapSurface->h, size - 1);
			const int offsety = ((y * size) % mapSurface->h) * 64 / mapSurface->h;

			const sTerrain& t = this->getTerrain (cPosition (terrainx, terrainy));
			unsigned int ColorNr = * (static_cast<const unsigned char*> (t.sf_org->pixels) + (offsetx + offsety * 64));

			unsigned char* pixel = reinterpret_cast<unsigned char*> (&static_cast<Uint32*> (mapSurface->pixels) [x + y * mapSurface->w]);
			pixel[0] = palette[ColorNr].b;
			pixel[1] = palette[ColorNr].g;
			pixel[2] = palette[ColorNr].r;
		}
	}
	if (SDL_MUSTLOCK (mapSurface.get())) SDL_UnlockSurface (mapSurface.get());
	return mapSurface;
}

// Funktionen der Map-Klasse /////////////////////////////////////////////////
cMap::cMap (std::shared_ptr<cStaticMap> staticMap_) :
	staticMap (std::move (staticMap_)),
	fields(nullptr)
{
	init();

}

void cMap::init()
{
	const int size = staticMap->getSize().x() * staticMap->getSize().y();
	if (Resources.size() != size)
	{
		Resources.resize(size, sResources());
		delete[] fields;
		fields = new cMapField[size];
	}
}

cMap::~cMap()
{
	delete [] fields;
}

cMapField& cMap::operator[] (unsigned int offset) const
{
	return fields[offset];
}

cMapField& cMap::getField (const cPosition& position)
{
	return fields[getOffset (position)];
}

const cMapField& cMap::getField (const cPosition& position) const
{
	return fields[getOffset (position)];
}

bool cMap::isWaterOrCoast (const cPosition& position) const
{
	const sTerrain& terrainType = staticMap->getTerrain (position);
	return terrainType.water | terrainType.coast;
}

//--------------------------------------------------------------------------
std::string cMap::resourcesToString() const
{
	std::string str;
	str.reserve (4 * Resources.size() + 1);
	for (size_t i = 0; i != Resources.size(); ++i)
	{
		str += getHexValue (static_cast<int>(Resources[i].typ));
		str += getHexValue (Resources[i].value);
	}
	return str;
}

//--------------------------------------------------------------------------
void cMap::setResourcesFromString (const std::string& str)
{
	for (size_t i = 0; i != Resources.size(); ++i)
	{
		sResources res;
		res.typ   = static_cast<eResourceType>(getByteValue(str, 4 * i));
		res.value = getByteValue(str, 4 * i + 2);

		Resources.set(i, res);
	}
}

void cMap::placeRessources(cModel& model)
{
	const auto& playerList = model.getPlayerList();
	const auto& gameSettings = *model.getGameSettings();

	Resources.fill(sResources());

	std::vector<eResourceType> resSpotTypes(playerList.size(), eResourceType::Metal);
	std::vector<cPosition> resSpots;
	for (const auto& player : playerList)
	{
		const auto& position = player->getLandingPos();
		resSpots.push_back(cPosition((position.x() & ~1) + 1, position.y() & ~1));
	}

	const eGameSettingsResourceDensity density = gameSettings.getResourceDensity();
	std::map<eResourceType, eGameSettingsResourceAmount> frequencies;

	frequencies[eResourceType::Metal] = gameSettings.getMetalAmount();
	frequencies[eResourceType::Oil] = gameSettings.getOilAmount();
	frequencies[eResourceType::Gold] = gameSettings.getGoldAmount();

	const std::size_t resSpotCount = (std::size_t) (getSize().x() * getSize().y() * 0.003f * (1.5f + getResourceDensityFactor (density)));
	const std::size_t playerCount = playerList.size();
	// create remaining resource positions
	while (resSpots.size() < resSpotCount)
	{
		cPosition pos(
			2 + model.randomGenerator.get (getSize().x() - 4),
			2 + model.randomGenerator.get (getSize().y() - 4)
		);
		resSpots.push_back(pos);
	}
	resSpotTypes.resize (resSpotCount);
	// Resourcen gleichmässiger verteilen
	for (std::size_t j = 0; j < 3; j++)
	{
		for (std::size_t i = playerCount; i < resSpotCount; i++)
		{
			cVector2 d(0, 0);
			for (std::size_t j = 0; j < resSpotCount; j++)
			{
				if (i == j) continue;

				int diffx1 = resSpots[i].x() - resSpots[j].x();
				int diffx2 = diffx1 + (getSize().x() - 4);
				int diffx3 = diffx1 - (getSize().x() - 4);
				int diffy1 = resSpots[i].y() - resSpots[j].y();
				int diffy2 = diffy1 + (getSize().y() - 4);
				int diffy3 = diffy1 - (getSize().y() - 4);
				if (abs (diffx2) < abs (diffx1)) diffx1 = diffx2;
				if (abs (diffx3) < abs (diffx1)) diffx1 = diffx3;
				if (abs (diffy2) < abs (diffy1)) diffy1 = diffy2;
				if (abs (diffy3) < abs (diffy1)) diffy1 = diffy3;

				cVector2 diff (diffx1, diffy1);
				if (diff.l2Norm() == 0.0f)
				{
					diff.x() += 1;
				}

				d += diff * (10.f / diff.l2NormSquared());
			}

			resSpots[i] += cVector2(Round (d.x()), Round (d.y()));

			if (resSpots[i].x() < 2)
				resSpots[i].x() += getSize().x() - 4;
			if (resSpots[i].y() < 2)
				resSpots[i].y() += getSize().y() - 4;
			if (resSpots[i].x() > getSize().x() - 3)
				resSpots[i].x() -= getSize().x() - 4;
			if (resSpots[i].y() > getSize().y() - 3)
				resSpots[i].y() -= getSize().y() - 4;

		}
	}
	// Resourcen Typ bestimmen
	for (std::size_t i = playerCount; i < resSpotCount; i++)
	{
		std::map<eResourceType, double> amount;
		for (std::size_t j = 0; j < i; j++)
		{
			const float maxDist = 40.f;
			cVector2 spotA = resSpots[i];
			cVector2 spotB = resSpots[j];
			float dist = (spotA - spotB).l2Norm();
			if (dist < maxDist)
				amount[resSpotTypes[j]] += 1 - sqrtf (dist / maxDist);
		}

		amount[eResourceType::Metal] /= 1.0f;
		amount[eResourceType::Oil] /= 0.8f;
		amount[eResourceType::Gold] /= 0.4f;

		eResourceType type = eResourceType::Metal;
		if (amount[eResourceType::Oil] < amount[type])
			type = eResourceType::Oil;
		if (amount[eResourceType::Gold] < amount[type])
			type = eResourceType::Gold;

		resSpots[i].x() &= ~1;
		resSpots[i].y() &= ~1;
		resSpots[i].x() += static_cast<int>(type) % 2;
		resSpots[i].y() += (static_cast<int>(type) / 2) % 2;

		resSpotTypes[i] = static_cast<eResourceType>(((resSpots[i].y() % 2) * 2) + (resSpots[i].x() % 2));
	}
	// Resourcen platzieren
	for (std::size_t i = 0; i < resSpotCount; i++)
	{
		cPosition pos = resSpots[i];
		cPosition p;
		bool hasGold = model.randomGenerator.get (100) < 40;
		const int minx = std::max (pos.x() - 1, 0);
		const int maxx = std::min (pos.x() + 1, getSize().x() - 1);
		const int miny = std::max (pos.y() - 1, 0);
		const int maxy = std::min (pos.y() + 1, getSize().y() - 1);

		for (int y = miny; y <= maxy; ++y)
		{
			for (int x = minx; x <= maxx; ++x)
			{
				cPosition absPos(x,y);
				eResourceType type = static_cast<eResourceType>((y % 2) * 2 + (x % 2));

				int index = getOffset (absPos);
				if (type != eResourceType::None &&
					((hasGold && i >= playerCount) || resSpotTypes[i] == eResourceType::Gold || type != eResourceType::Gold) &&
					!isBlocked (absPos))
				{
					sResources res;
					res.typ = type;
					if (i >= playerCount)
					{
						res.value = 1 + model.randomGenerator.get (2 + getResourceAmountFactor (frequencies[type]) * 2);
						if (p == pos) res.value += 3 + model.randomGenerator.get(4 + getResourceAmountFactor(frequencies[type]) * 2);
					}
					else
					{
						res.value = 1 + 4 + getResourceAmountFactor (frequencies[type]);
						if (p == pos) res.value += 3 + 2 + getResourceAmountFactor (frequencies[type]);
					}
					res.value = std::min<unsigned char> (16, res.value);
					Resources.set(index, res);
				}
			}
		}
	}
}

/* static */ int cMap::getMapLevel (const cBuilding& building)
{
	const cStaticUnitData& data = building.getStaticUnitData();

	if (!building.isRubble()) return 4;  // rubble

	if (data.surfacePosition == cStaticUnitData::SURFACE_POS_BENEATH_SEA) return 9; // seamine
	if (data.surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_SEA) return 7; // bridge
	if (data.surfacePosition == cStaticUnitData::SURFACE_POS_BASE && data.canBeOverbuild) return 6; // platform
	if (data.surfacePosition == cStaticUnitData::SURFACE_POS_BASE) return 5; // road

	if (data.surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_BASE) return 3; // landmine

	return 1; // other buildings
}

/* static */ int cMap::getMapLevel (const cVehicle& vehicle)
{
	if (vehicle.getStaticUnitData().factorSea > 0 && vehicle.getStaticUnitData().factorGround == 0) return 8; // ships
	if (vehicle.getStaticUnitData().factorAir > 0) return 0; // planes

	return 2; // other vehicles
}

void cMap::addBuilding (cBuilding& building, const cPosition& position)
{
	int size = building.getCellSize();

	//big base building are not implemented
	if (building.getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_GROUND && size > 1 && !building.isRubble())
		return;

	const int mapLevel = cMap::getMapLevel (building);
	size_t i = 0;

	// We are iterating all over every position inside the building
	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++, i++)
		{
			auto& field = getField (position.relative(x,y));
			i = 0;
			// Adding to the last index. These iterations are really creepy...
			while (i < field.getBuildings().size() && cMap::getMapLevel (*field.getBuildings()[i]) < mapLevel)
				i++;
			field.addBuilding (building, i);
		}
	}

	addedUnit (building);
}

void cMap::addVehicle (cVehicle& vehicle, const cPosition& position)
{
	bool isAir = vehicle.getStaticUnitData().factorAir;

	int size = vehicle.getCellSize();

	// We are iterating all over every position inside the vehicle
	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++)
		{
			auto& field = getField (position.relative(x,y));

			if (isAir > 0)
			{
				field.addPlane (vehicle, 0);
			}
			else
			{
				field.addVehicle (vehicle, 0);
			}
		}
	}

	//if (vehicle.getCellSize() > 1)
	//{
	//	moveVehicleBig(vehicle, position);
	//}

	addedUnit (vehicle);
}

void cMap::deleteBuilding (const cBuilding& object)
{
	// We are iterating all over every occupied position inside the building
	int size = object.getCellSize();
	cPosition pos = object.getPosition();

	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++)
		{
			auto& field = getField (pos.relative(x,y));
			field.removeBuilding (object);
		}
	}
}

void cMap::deleteVehicle (const cVehicle& object)
{
	bool isAir = object.getStaticUnitData().factorAir;

	// We are iterating all over every occupied position inside the vehicle
	int size = object.getCellSize();
	cPosition pos = object.getPosition();

	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++)
		{
			auto& field = getField (pos.relative(x,y));
			field.removeVehicle (object);

			if(isAir)
			{
				field.removePlane(object);
			}
		}
	}

	removedUnit (object);
}

void cMap::deleteUnit (const cUnit& unit)
{
	if (unit.isABuilding())
	{
		deleteBuilding (static_cast<const cBuilding&> (unit));
	}
	else
	{
		assert (unit.isAVehicle());
		deleteVehicle (static_cast<const cVehicle&> (unit));
	}
}

void cMap::moveVehicle (cVehicle& vehicle, const cPosition& position, int height)
{
	int size = vehicle.getCellSize();
	const auto oldPosition = vehicle.getPosition();

	vehicle.setPosition (position);

	if (vehicle.getStaticUnitData().factorAir > 0)
	{
		for(int y = 0; y < size; y++)
			for(int x = 0; x < size; x++)
			{
				getField (oldPosition.relative(x,y)).removePlane (vehicle);
			}

		height = std::min<int> (getField (position).getPlanes().size(), height);
		for(int y = 0; y < size; y++)
			for(int x = 0; x < size; x++)
			{
				getField (oldPosition.relative(x,y)).addPlane (vehicle, height);
			}
	}
	else
	{
		for(int y = 0; y < size; y++)
			for(int x = 0; x < size; x++)
			{
				getField (oldPosition.relative(x,y)).removeVehicle (vehicle);
			}

		// Adding vehicle to the new position
		for(int y = 0; y < size; y++)
			for(int x = 0; x < size; x++)
			{
				getField (position.relative(x,y)).addVehicle (vehicle, 0);
			}
	}

	movedVehicle (vehicle, oldPosition);
}

bool cMap::possiblePlace (const cVehicle& vehicle, const cPosition& position, bool checkPlayer, bool ignoreMovingVehicles) const
{
	std::set<const cVehicle*> toIgnore;
	toIgnore.insert(&vehicle);
	return possiblePlaceVehicle (vehicle.getStaticUnitData(), position, checkPlayer ? vehicle.getOwner() : nullptr, toIgnore, ignoreMovingVehicles);
}

bool cMap::possiblePlaceVehicle(const cStaticUnitData& vehicleData, const cPosition& pos, const cPlayer* player, bool ignoreMovingVehicles) const
{
	std::set<const cVehicle*> toIgnore;
	return this->possiblePlaceVehicle(vehicleData, pos, player, toIgnore, ignoreMovingVehicles);
}

bool cMap::possiblePlaceVehicle(const cStaticUnitData& vehicleData, const cPosition& pos, const cPlayer* player, const std::set<const cVehicle*>& toIgnore, bool ignoreMovingVehicles) const
{
	// TODO: Split it to gathering of all obstacles in area and decision making
	if(!staticMap->possiblePlace(vehicleData, pos))
		return false;

	int size = vehicleData.cellSize;
	for(int y = 0; y < size; y++)
	{
		for(int x = 0; x < size; x++)
		{
			cPosition position = pos.relative(x,y);
			// This is player-related view
			const auto field = cMapFieldView(getField(position), staticMap->getTerrain(position), player);

			const std::vector<cBuilding*> buildings = field.getBuildings();
			std::vector<cBuilding*>::const_iterator b_it = buildings.begin();
			std::vector<cBuilding*>::const_iterator b_end = buildings.end();

			//search first building, that is not a connector
			if (b_it != b_end && (*b_it)->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE)
				++b_it;

			if (vehicleData.factorAir > 0)
			{
				// What? Teleport if we do not see?
				if (player && !player->canSeeAt (position))
					return true;

				const auto& planes = field.getPlanes();
				if (!ignoreMovingVehicles)
				{
					if (planes.size() >= MAX_PLANES_PER_FIELD)
						return false;
				}
				else
				{
					int notMovingPlanes = 0;
					for (const auto& plane : planes)
					{
						if (!plane->isUnitMoving())
						{
							notMovingPlanes++;
						}
					}
					if (notMovingPlanes >= MAX_PLANES_PER_FIELD)
						return false;
				}
			}

			if (vehicleData.factorGround > 0)
			{
				if ((isWater (position) && vehicleData.factorSea == 0) ||
					(isCoast (position) && vehicleData.factorCoast == 0))
				{
					if (player && !player->canSeeAt (position))
						return false;

					//vehicle can drive on water, if there is a bridge, platform or road
					if (b_it == b_end)
						return false;
					auto surfacePosition = (*b_it)->getStaticUnitData().surfacePosition;
					if (surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_SEA &&
							surfacePosition != cStaticUnitData::SURFACE_POS_BASE &&
							surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_BASE)
						return false;
				}
				//check for enemy mines
				if (player &&
					b_it != b_end &&
					(*b_it)->getOwner() != player &&
					(*b_it)->getStaticUnitData().hasFlag(UnitFlag::ExplodesOnContact) &&
					(*b_it)->isDetectedByPlayer(player))
				{
					return false;
				}

				if (player && !player->canSeeAt (position)) return true;

				if (const cVehicle* v = field.getVehicle())
				{
					if(!toIgnore.count(v) && (!ignoreMovingVehicles || !v->isUnitMoving()))
						return false;
				}
				if (b_it != b_end)
				{
					// only base buildings and rubble is allowed on the same field with a vehicle
					// (connectors have been skiped, so doesn't matter here)
					auto surfacePosition = (*b_it)->getStaticUnitData().surfacePosition;
					if (surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_SEA &&
							surfacePosition != cStaticUnitData::SURFACE_POS_BASE &&
							surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_BASE &&
							surfacePosition != cStaticUnitData::SURFACE_POS_BENEATH_SEA &&
							!(*b_it)->isRubble())
						return false;
				}
			}
			else if (vehicleData.factorSea > 0)
			{
				if (!isWater (position) &&
					(!isCoast (position) || vehicleData.factorCoast == 0)) return false;

				//check for enemy mines
				if (player &&
					b_it != b_end &&
					(*b_it)->getOwner() != player &&
					(*b_it)->getStaticUnitData().hasFlag(UnitFlag::ExplodesOnContact) &&
					(*b_it)->isDetectedByPlayer(player))
				{
					return false;
				}

				if (player && !player->canSeeAt (position))
					return true;

				if (const cVehicle* v = field.getVehicle())
				{
					if(!toIgnore.count(v) && (!ignoreMovingVehicles || !v->isUnitMoving()))
						return false;
				}

				//only bridge and sea mine are allowed on the same field with a ship (connectors have been skiped, so doesn't matter here)
				if (b_it != b_end &&
					(*b_it)->getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_SEA &&
					(*b_it)->getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_BENEATH_SEA)
				{
					// if the building is a landmine, we have to check whether it's on a bridge or not
					if ((*b_it)->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_BASE)
					{
						++b_it;
						if (b_it == b_end || (*b_it)->getStaticUnitData().surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_SEA) return false;
					}
					else
						return false;
				}
			}
		}
	}
	return true;
}

bool cMap::possiblePlaceBuilding (const cStaticUnitData& buildingData, const cPosition& position, const cPlayer* player, const cVehicle* vehicle) const
{
	if (!isValidPosition (position))
		return false;
	if (isBlocked (position))
		return false;
	const auto field = cMapFieldView(getField(position), staticMap->getTerrain(position), player);

	// Check all buildings in this field for a building of the same type. This
	// will prevent roads, connectors and water platforms from building on top
	// of themselves.
	const std::vector<cBuilding*>& buildings = field.getBuildings();
	for (const cBuilding* building : buildings)
	{
		if (building->getStaticUnitData().ID == buildingData.ID)
		{
			return false;
		}
	}

	// Determine terrain type
	bool water = isWater (position);
	bool coast = isCoast (position);
	bool ground = !water && !coast;

	for (const cBuilding* building : buildings)
	{
		if (buildingData.surfacePosition == building->getStaticUnitData().surfacePosition &&
			building->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_NO) return false;
		switch (building->getStaticUnitData().surfacePosition)
		{
			case cStaticUnitData::SURFACE_POS_GROUND:
			case cStaticUnitData::SURFACE_POS_ABOVE_SEA: // bridge
				if (buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE &&
					buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_BASE && // mine can be placed on bridge
					buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_BENEATH_SEA && // seamine can be placed under bridge
					building->getStaticUnitData().canBeOverbuild == cStaticUnitData::OVERBUILD_TYPE_NO) return false;
				break;
			case cStaticUnitData::SURFACE_POS_BENEATH_SEA: // seamine
			case cStaticUnitData::SURFACE_POS_ABOVE_BASE:  // landmine
				// mine must be removed first
				if (buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE) return false;
				break;
			case cStaticUnitData::SURFACE_POS_BASE: // platform, road
				water = coast = false;
				ground = true;
				break;
			case cStaticUnitData::SURFACE_POS_ABOVE: // connector
				break;
		}
	}
	if ((water && buildingData.factorSea == 0) ||
		(coast && buildingData.factorCoast == 0) ||
		(ground && buildingData.factorGround == 0)) return false;

	//can not build on rubble
	if (field.getRubble() &&
		buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE &&
		buildingData.surfacePosition != cStaticUnitData::SURFACE_POS_ABOVE_BASE) return false;

	if (field.getVehicle())
	{
		if (!vehicle) return false;
		if (vehicle != field.getVehicle()) return false;
	}
	return true;
}
void cMap::reset()
{
	for (int i = 0; i < getSize().x() * getSize().y(); i++)
	{
		fields[i].removeAll();
	}
}

uint32_t cMap::getChecksum(uint32_t crc) const
{
	crc = staticMap->getChecksum(crc);
	//cMapField* fields;
	crc = calcCheckSum(Resources, crc);

	return crc;
}

/*static*/ int cMap::getResourceDensityFactor (eGameSettingsResourceDensity density)
{
	switch (density)
	{
		case eGameSettingsResourceDensity::Sparse:
			return 0;
		case eGameSettingsResourceDensity::Normal:
			return 1;
		case eGameSettingsResourceDensity::Dense:
			return 2;
		case eGameSettingsResourceDensity::TooMuch:
			return 3;
	}
	assert (false);
	return 0;
}

/*static*/int cMap::getResourceAmountFactor (eGameSettingsResourceAmount amount)
{
	switch (amount)
	{
		case eGameSettingsResourceAmount::Limited:
			return 0;
		case eGameSettingsResourceAmount::Normal:
			return 1;
		case eGameSettingsResourceAmount::High:
			return 2;
		case eGameSettingsResourceAmount::TooMuch:
			return 3;
	}
	assert (false);
	return 0;
}

uint32_t sResources::getChecksum(uint32_t crc) const
{
	crc = calcCheckSum(value, crc);
	crc = calcCheckSum(typ, crc);

	return crc;
}
