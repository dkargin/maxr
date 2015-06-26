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
#include "utility/log.h"
#include "game/data/player/player.h"
#include "settings.h"
#include "game/data/units/vehicle.h"
#include "video.h"
#include "utility/position.h"
#include "utility/random.h"

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

	if ((building->data.surfacePosition == sUnitData::SURFACE_POS_GROUND ||
		 building->data.surfacePosition == sUnitData::SURFACE_POS_ABOVE) &&
		building->getOwner())
		return building;
	return nullptr;
}

cBuilding* cMapField::getBaseBuilding() const
{
	for (cBuilding* building : buildings)
	{
		if (building->data.surfacePosition != sUnitData::SURFACE_POS_GROUND &&
			building->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE &&
			building->getOwner())
		{
			return building;
		}
	}
	return nullptr;
}

cBuilding* cMapField::getRubble() const
{
	for (cBuilding* building : buildings)
		if (!building->getOwner())
			return building;
	return nullptr;
}

cBuilding* cMapField::getMine() const
{
	for (cBuilding* building : buildings)
		if (building->data.explodesOnContact)
			return building;
	return nullptr;
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

cStaticMap::cStaticMap() : size (0), terrains()
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

bool cStaticMap::isValidPosition (const cPosition& position) const
{
	return 0 <= position.x() && position.x() < size && 0 <= position.y() && position.y() < size;
}

void cStaticMap::clear()
{
	filename.clear();
	size = 0;
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
	staticMap (std::move (staticMap_))
{
	const int size = staticMap->getSize().x() * staticMap->getSize().y();
	fields = new cMapField[size];
	Resources.resize (size);
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

void cMap::assignRessources (const cMap& rhs)
{
	Resources = rhs.Resources;
}

//--------------------------------------------------------------------------
static std::string getHexValue (unsigned char byte)
{
	std::string str = "";
	const char hexChars[] = "0123456789ABCDEF";
	const unsigned char high = (byte >> 4) & 0x0F;
	const unsigned char low = byte & 0x0F;

	str += hexChars[high];
	str += hexChars[low];
	return str;
}

//--------------------------------------------------------------------------
std::string cMap::resourcesToString() const
{
	std::string str;
	str.reserve (4 * Resources.size() + 1);
	for (size_t i = 0; i != Resources.size(); ++i)
	{
		str += getHexValue (Resources[i].typ);
		str += getHexValue (Resources[i].value);
	}
	return str;
}

//--------------------------------------------------------------------------
static unsigned char getByteValue (const std::string& str, int index)
{
	unsigned char first = str[index + 0] - '0';
	unsigned char second = str[index + 1] - '0';

	if (first >= 'A' - '0') first -= 'A' - '0' - 10;
	if (second >= 'A' - '0') second -= 'A' - '0' - 10;
	return (first * 16 + second);
}

//--------------------------------------------------------------------------
void cMap::setResourcesFromString (const std::string& str)
{
	for (size_t i = 0; i != Resources.size(); ++i)
	{
		Resources[i].typ = getByteValue (str, 4 * i);
		Resources[i].value = getByteValue (str, 4 * i + 2);
	}
}

void cMap::placeRessources (const std::vector<cPosition>& landingPositions, const cGameSettings& gameSetting)
{
	std::fill (Resources.begin(), Resources.end(), sResources());

	std::vector<int> resSpotTypes(landingPositions.size(), RES_METAL);
	std::vector<T_2<int>> resSpots;
	for (const auto& position : landingPositions)
	{
		resSpots.push_back(T_2<int> ((position.x() & ~1) + (RES_METAL % 2), (position.y() & ~1) + ((RES_METAL / 2) % 2)));
	}

	const eGameSettingsResourceDensity density = gameSetting.getResourceDensity();
	eGameSettingsResourceAmount frequencies[RES_COUNT];

	frequencies[RES_METAL] = gameSetting.getMetalAmount();
	frequencies[RES_OIL] = gameSetting.getOilAmount();
	frequencies[RES_GOLD] = gameSetting.getGoldAmount();

	const std::size_t resSpotCount = (std::size_t) (getSize().x() * getSize().y() * 0.003f * (1.5f + getResourceDensityFactor (density)));
	const std::size_t playerCount = landingPositions.size();
	// create remaining resource positions
	while (resSpots.size() < resSpotCount)
	{
		T_2<int> pos;

		pos.x = 2 + random (getSize().x() - 4);
		pos.y = 2 + random (getSize().y() - 4);
		resSpots.push_back(pos);
	}
	resSpotTypes.resize (resSpotCount);
	// Resourcen gleichmässiger verteilen
	for (std::size_t j = 0; j < 3; j++)
	{
		for (std::size_t i = playerCount; i < resSpotCount; i++)
		{
			T_2<float> d;
			for (std::size_t j = 0; j < resSpotCount; j++)
			{
				if (i == j) continue;

				int diffx1 = resSpots[i].x - resSpots[j].x;
				int diffx2 = diffx1 + (getSize().x() - 4);
				int diffx3 = diffx1 - (getSize().x() - 4);
				int diffy1 = resSpots[i].y - resSpots[j].y;
				int diffy2 = diffy1 + (getSize().y() - 4);
				int diffy3 = diffy1 - (getSize().y() - 4);
				if (abs (diffx2) < abs (diffx1)) diffx1 = diffx2;
				if (abs (diffx3) < abs (diffx1)) diffx1 = diffx3;
				if (abs (diffy2) < abs (diffy1)) diffy1 = diffy2;
				if (abs (diffy3) < abs (diffy1)) diffy1 = diffy3;
				T_2<float> diff (diffx1, diffy1);
				if (diff == T_2<float>::Zero)
				{
					diff.x += 1;
				}
				const float dist = diff.dist();
				d += diff * (10.f / (dist * dist));

			}
			resSpots[i] += T_2<int> (Round (d.x), Round (d.y));
			if (resSpots[i].x < 2) resSpots[i].x += getSize().x() - 4;
			if (resSpots[i].y < 2) resSpots[i].y += getSize().y() - 4;
			if (resSpots[i].x > getSize().x() - 3) resSpots[i].x -= getSize().x() - 4;
			if (resSpots[i].y > getSize().y() - 3) resSpots[i].y -= getSize().y() - 4;

		}
	}
	// Resourcen Typ bestimmen
	for (std::size_t i = playerCount; i < resSpotCount; i++)
	{
		float amount[RES_COUNT] = {0.f, 0.f, 0.f, 0.f};
		for (std::size_t j = 0; j < i; j++)
		{
			const float maxDist = 40.f;
			float dist = sqrtf (resSpots[i].distSqr (resSpots[j]));
			if (dist < maxDist) amount[resSpotTypes[j]] += 1 - sqrtf (dist / maxDist);
		}

		amount[RES_METAL] /= 1.0f;
		amount[RES_OIL] /= 0.8f;
		amount[RES_GOLD] /= 0.4f;

		int type = RES_METAL;
		if (amount[RES_OIL] < amount[type]) type = RES_OIL;
		if (amount[RES_GOLD] < amount[type]) type = RES_GOLD;

		resSpots[i].x &= ~1;
		resSpots[i].y &= ~1;
		resSpots[i].x += type % 2;
		resSpots[i].y += (type / 2) % 2;

		resSpotTypes[i] = ((resSpots[i].y % 2) * 2) + (resSpots[i].x % 2);
	}
	// Resourcen platzieren
	for (std::size_t i = 0; i < resSpotCount; i++)
	{
		T_2<int> pos = resSpots[i];
		T_2<int> p;
		bool hasGold = random (100) < 40;
		const int minx = std::max (pos.x - 1, 0);
		const int maxx = std::min (pos.x + 1, getSize().x() - 1);
		const int miny = std::max (pos.y - 1, 0);
		const int maxy = std::min (pos.y + 1, getSize().y() - 1);
		for (p.y = miny; p.y <= maxy; ++p.y)
		{
			for (p.x = minx; p.x <= maxx; ++p.x)
			{
				T_2<int> absPos = p;
				int type = (absPos.y % 2) * 2 + (absPos.x % 2);

				int index = getOffset (cPosition (absPos.x, absPos.y));
				if (type != RES_NONE &&
					((hasGold && i >= playerCount) || resSpotTypes[i] == RES_GOLD || type != RES_GOLD) &&
					!isBlocked (cPosition (absPos.x, absPos.y)))
				{
					Resources[index].typ = type;
					if (i >= playerCount)
					{
						Resources[index].value = 1 + random (2 + getResourceAmountFactor (frequencies[type]) * 2);
						if (p == pos) Resources[index].value += 3 + random (4 + getResourceAmountFactor (frequencies[type]) * 2);
					}
					else
					{
						Resources[index].value = 1 + 4 + getResourceAmountFactor (frequencies[type]);
						if (p == pos) Resources[index].value += 3 + 2 + getResourceAmountFactor (frequencies[type]);
					}

					Resources[index].value = std::min<unsigned char> (16, Resources[index].value);
				}
			}
		}
	}
}

/* static */ int cMap::getMapLevel (const cBuilding& building)
{
	const sUnitData& data = building.data;

	if (data.surfacePosition == sUnitData::SURFACE_POS_BENEATH_SEA) return 9; // seamine
	if (data.surfacePosition == sUnitData::SURFACE_POS_ABOVE_SEA) return 7; // bridge
	if (data.surfacePosition == sUnitData::SURFACE_POS_BASE && data.canBeOverbuild) return 6; // platform
	if (data.surfacePosition == sUnitData::SURFACE_POS_BASE) return 5; // road
	if (!building.getOwner()) return 4;  // rubble
	if (data.surfacePosition == sUnitData::SURFACE_POS_ABOVE_BASE) return 3; // landmine

	return 1; // other buildings
}

/* static */ int cMap::getMapLevel (const cVehicle& vehicle)
{
	if (vehicle.data.factorSea > 0 && vehicle.data.factorGround == 0) return 8; // ships
	if (vehicle.data.factorAir > 0) return 0; // planes

	return 2; // other vehicles
}

void cMap::addBuilding (cBuilding& building, const cPosition& position)
{
	//big base building are not implemented
	if (building.data.surfacePosition != sUnitData::SURFACE_POS_GROUND && building.data.isBig && building.getOwner()) return;

	const int mapLevel = cMap::getMapLevel (building);
	size_t i = 0;

	if (building.data.isBig)
	{
		auto& field = getField (position);
		i = 0;
		while (i < field.getBuildings().size() && cMap::getMapLevel (*field.getBuildings()[i]) < mapLevel) i++;
		field.addBuilding (building, i);

		auto& fieldEast = getField (position + cPosition (1, 0));
		i = 0;
		while (i < fieldEast.getBuildings().size() && cMap::getMapLevel (*fieldEast.getBuildings()[i]) < mapLevel) i++;
		fieldEast.addBuilding (building, i);

		auto& fieldSouth = getField (position + cPosition (0, 1));
		i = 0;
		while (i < fieldSouth.getBuildings().size() && cMap::getMapLevel (*fieldSouth.getBuildings()[i]) < mapLevel) i++;
		fieldSouth.addBuilding (building, i);

		auto& fieldSouthEast = getField (position + cPosition (1, 1));
		i = 0;
		while (i < fieldSouthEast.getBuildings().size() && cMap::getMapLevel (*fieldSouthEast.getBuildings()[i]) < mapLevel) i++;
		fieldSouthEast.addBuilding (building, i);
	}
	else
	{
		auto& field = getField (position);

		while (i < field.getBuildings().size() && cMap::getMapLevel (*field.getBuildings()[i]) < mapLevel) i++;
		field.addBuilding (building, i);
	}
	addedUnit (building);
}

void cMap::addVehicle (cVehicle& vehicle, const cPosition& position)
{
	auto& field = getField (position);
	if (vehicle.data.factorAir > 0)
	{
		field.addPlane (vehicle, 0);
	}
	else
	{
		field.addVehicle (vehicle, 0);
	}
	addedUnit (vehicle);
}

void cMap::deleteBuilding (const cBuilding& building)
{
	getField (building.getPosition()).removeBuilding (building);

	if (building.data.isBig)
	{
		getField (building.getPosition() + cPosition (1, 0)).removeBuilding (building);
		getField (building.getPosition() + cPosition (1, 1)).removeBuilding (building);
		getField (building.getPosition() + cPosition (0, 1)).removeBuilding (building);
	}
	removedUnit (building);
}

void cMap::deleteVehicle (const cVehicle& vehicle)
{
	if (vehicle.data.factorAir > 0)
	{
		getField (vehicle.getPosition()).removePlane (vehicle);
	}
	else
	{
		getField (vehicle.getPosition()).removeVehicle (vehicle);

		if (vehicle.data.isBig)
		{
			getField (vehicle.getPosition() + cPosition (1, 0)).removeVehicle (vehicle);
			getField (vehicle.getPosition() + cPosition (1, 1)).removeVehicle (vehicle);
			getField (vehicle.getPosition() + cPosition (0, 1)).removeVehicle (vehicle);
		}
	}
	removedUnit (vehicle);
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
	const auto oldPosition = vehicle.getPosition();

	vehicle.setPosition (position);

	if (vehicle.data.factorAir > 0)
	{
		getField (oldPosition).removePlane (vehicle);
		height = std::min<int> (getField (position).getPlanes().size(), height);
		getField (position).addPlane (vehicle, height);
	}
	else
	{
		getField (oldPosition).removeVehicle (vehicle);

		//check, whether the vehicle is centered on 4 map fields
		if (vehicle.data.isBig)
		{
			getField (oldPosition + cPosition (1, 0)).removeVehicle (vehicle);
			getField (oldPosition + cPosition (1, 1)).removeVehicle (vehicle);
			getField (oldPosition + cPosition (0, 1)).removeVehicle (vehicle);

			vehicle.data.isBig = false;
		}

		getField (position).addVehicle (vehicle, 0);
	}
	movedVehicle (vehicle, oldPosition);
}

void cMap::moveVehicleBig (cVehicle& vehicle, const cPosition& position)
{
	if (vehicle.data.isBig)
	{
		Log.write ("Calling moveVehicleBig on a big vehicle", cLog::eLOG_TYPE_NET_ERROR);
		//calling this function twice is always an error.
		//nevertheless try to proceed by resetting the data.isBig flag
		moveVehicle (vehicle, position);
	}

	const auto oldPosition = vehicle.getPosition();

	getField (oldPosition).removeVehicle (vehicle);

	vehicle.setPosition (position);

	getField (position).addVehicle (vehicle, 0);
	getField (position + cPosition (1, 0)).addVehicle (vehicle, 0);
	getField (position + cPosition (1, 1)).addVehicle (vehicle, 0);
	getField (position + cPosition (0, 1)).addVehicle (vehicle, 0);

	vehicle.data.isBig = true;

	movedVehicle (vehicle, oldPosition);
}

bool cMap::possiblePlace (const cVehicle& vehicle, const cPosition& position, bool checkPlayer) const
{
	return possiblePlaceVehicle (vehicle.data, position, vehicle.getOwner(), checkPlayer);
}

bool cMap::possiblePlaceVehicle (const sUnitData& vehicleData, const cPosition& position, const cPlayer* player, bool checkPlayer) const
{
	if (isValidPosition (position) == false) return false;

	const std::vector<cBuilding*>& buildings = getField (position).getBuildings();
	std::vector<cBuilding*>::const_iterator b_it = buildings.begin();
	std::vector<cBuilding*>::const_iterator b_end = buildings.end();

	//search first building, that is not a connector
	if (b_it != b_end && (*b_it)->data.surfacePosition == sUnitData::SURFACE_POS_ABOVE) ++b_it;

	if (vehicleData.factorAir > 0)
	{
		if (checkPlayer && player && !player->canSeeAt (position)) return true;
		//only one plane per field for now
		if (getField (position).getPlanes().size() >= MAX_PLANES_PER_FIELD) return false;
	}
	if (vehicleData.factorGround > 0)
	{
		if (isBlocked (position)) return false;

		if ((isWater (position) && vehicleData.factorSea == 0) ||
			(isCoast (position) && vehicleData.factorCoast == 0))
		{
			if (checkPlayer && player && !player->canSeeAt (position)) return false;

			//vehicle can drive on water, if there is a bridge, platform or road
			if (b_it == b_end) return false;
			if ((*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_SEA &&
				(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_BASE &&
				(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_BASE) return false;
		}
		//check for enemy mines
		if (player && b_it != b_end && (*b_it)->getOwner() != player &&
			(*b_it)->data.explodesOnContact &&
			((*b_it)->isDetectedByPlayer (player) || checkPlayer))
			return false;

		if (checkPlayer && player && !player->canSeeAt (position)) return true;

		if (getField (position).getVehicles().empty() == false) return false;
		if (b_it != b_end)
		{
			// only base buildings and rubble is allowed on the same field with a vehicle
			// (connectors have been skiped, so doesn't matter here)
			if ((*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_SEA &&
				(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_BASE &&
				(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_BASE &&
				(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_BENEATH_SEA &&
				(*b_it)->getOwner()) return false;
		}
	}
	else if (vehicleData.factorSea > 0)
	{
		if (isBlocked (position)) return false;

		if (!isWater (position) &&
			(!isCoast (position) || vehicleData.factorCoast == 0)) return false;

		//check for enemy mines
		if (player && b_it != b_end && (*b_it)->getOwner() != player &&
			(*b_it)->data.explodesOnContact && (*b_it)->isDetectedByPlayer (player))
			return false;

		if (checkPlayer && player && !player->canSeeAt (position)) return true;

		if (getField (position).getVehicles().empty() == false) return false;

		//only bridge and sea mine are allowed on the same field with a ship (connectors have been skiped, so doesn't matter here)
		if (b_it != b_end &&
			(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_SEA &&
			(*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_BENEATH_SEA)
		{
			// if the building is a landmine, we have to check whether it's on a bridge or not
			if ((*b_it)->data.surfacePosition == sUnitData::SURFACE_POS_ABOVE_BASE)
			{
				++b_it;
				if (b_it == b_end || (*b_it)->data.surfacePosition != sUnitData::SURFACE_POS_ABOVE_SEA) return false;
			}
			else return false;
		}
	}
	return true;
}

// can't place it too near to the map border
bool cMap::possiblePlaceBuildingWithMargin (const sUnitData& buildingData, const cPosition& position, int margin, const cVehicle* vehicle) const
{
	if (position.x() < margin || position.x() >= getSize().x() - margin || position.y() < margin || position.y() >= getSize().y() - margin) return false;
	return possiblePlaceBuilding (buildingData, position, vehicle);
}

bool cMap::possiblePlaceBuilding (const sUnitData& buildingData, const cPosition& position, const cVehicle* vehicle) const
{
	if (!isValidPosition (position)) return false;
	if (isBlocked (position)) return false;
	const cMapField& field = getField (position);

	// Check all buildings in this field for a building of the same type. This
	// will prevent roads, connectors and water platforms from building on top
	// of themselves.
	const std::vector<cBuilding*>& buildings = field.getBuildings();
	for (const cBuilding* building : buildings)
	{
		if (building->data.ID == buildingData.ID)
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
		if (buildingData.surfacePosition == building->data.surfacePosition &&
			building->data.canBeOverbuild == sUnitData::OVERBUILD_TYPE_NO) return false;
		switch (building->data.surfacePosition)
		{
			case sUnitData::SURFACE_POS_GROUND:
			case sUnitData::SURFACE_POS_ABOVE_SEA: // bridge
				if (buildingData.surfacePosition != sUnitData::SURFACE_POS_ABOVE &&
					buildingData.surfacePosition != sUnitData::SURFACE_POS_BASE && // mine can be placed on bridge
					buildingData.surfacePosition != sUnitData::SURFACE_POS_BENEATH_SEA && // seamine can be placed under bridge
					building->data.canBeOverbuild == sUnitData::OVERBUILD_TYPE_NO) return false;
				break;
			case sUnitData::SURFACE_POS_BENEATH_SEA: // seamine
			case sUnitData::SURFACE_POS_ABOVE_BASE:  // landmine
				// mine must be removed first
				if (buildingData.surfacePosition != sUnitData::SURFACE_POS_ABOVE) return false;
				break;
			case sUnitData::SURFACE_POS_BASE: // platform, road
				water = coast = false;
				ground = true;
				break;
			case sUnitData::SURFACE_POS_ABOVE: // connector
				break;
		}
	}
	if ((water && buildingData.factorSea == 0) ||
		(coast && buildingData.factorCoast == 0) ||
		(ground && buildingData.factorGround == 0)) return false;

	//can not build on rubble
	if (field.getRubble() &&
		buildingData.surfacePosition != sUnitData::SURFACE_POS_ABOVE &&
		buildingData.surfacePosition != sUnitData::SURFACE_POS_ABOVE_BASE) return false;

	if (field.getVehicles().empty() == false)
	{
		if (!vehicle) return false;
		if (vehicle != field.getVehicles()[0]) return false;
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
