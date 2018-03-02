#ifndef MODDATA_H
#define MODDATA_H

class cUnitsData;
class cStaticUnitData;
class cVehicleData;
class cBuildingData;

struct sID;

namespace tinyxml2
{
	class XMLElement;
}
// Wraps up mod contents from specific folder
class ModData
{
public:
	ModData(const char * path, cUnitsData* unitsData);
	/**
	 * Loads all Buildings
	 * @param path Directory of the Buildings
	 * @return 1 on success
	 */
	int LoadBuildings(const char* path);

	/**
	 * Loads all Vehicles
	 * @param path Directory of the Vehicles
	 * @return 1 on success
	 */
	int LoadVehicles(const char* path);

	/**
	 * @brief parseDataFile parses XML file and adds all the data from it to BD
	 * @param path - path to XML file
	 */
	void parseDataFile(const char* path, const char* directory);

	void parseVehicle(tinyxml2::XMLElement* source, sID id, const std::string& name, const char* directory);
	void parseBuilding(tinyxml2::XMLElement* source, sID id, const std::string& name, const char* directory);

	// Structure contains color that was obtained from XML
	// There could be additinal meanings to this color
	struct XmlColor
	{
		bool isAuto = false;
		int r = -1;
		int g = -1;
		int b = -1;
		int a = -1;
	};

	/**
	 * Loads the clan values and stores them in the cUnitData class
	 * @return 1 on success
	 */
	int LoadClans();
protected:
	/**
	 * Loads the unitdata from the data.xml in the unitfolder
	 * @param source - pointer to XML element, that is used to update an object
	 * @param staticData - reference to static unit data we fill in
	 * @param directory - current directory
	 */
	void parseUnitData(tinyxml2::XMLElement* source, cStaticUnitData& staticData, const char* directory);
	/**
	 * @brief loadGraphicObject
	 * Parses XML contents for graphic object and adds it to the unit
	 * @param gobj - pointer to XML element that contains graphic object
	 * @param staticData - reference to a unit, which will get this object
	 * @param directory - current working directory
	 * @return if graphic object was properly generated
	 */
	bool parseGraphicObject(tinyxml2::XMLElement* gobj, cStaticUnitData& staticData, const char* dir);

	// Factory method to create a sprite object from XML data
	std::shared_ptr<cSprite> makeSprite(tinyxml2::XMLElement* gobj, const char* directory);
	// Factory method to create a sprite list object from XML data
	std::shared_ptr<cSpriteList> makeSpriteList(tinyxml2::XMLElement* gobj, const char* directory);
	// Factory method to create a sprite sheet object from XML data
	std::shared_ptr<cSpriteList> makeSpriteSheet(tinyxml2::XMLElement* gobj, const char* directory);

	// Parses common graphic attributes
	bool parseSpriteAttributes(tinyxml2::XMLElement* gobj, cSprite& sprite);

	// Try to obain ID by unique string name
	sID getVehicleId(const std::string& sid);

protected:
	// XML and generic parsing stuff is here
	/**
	 * Tries to parse color value
	 * It expects format like "134;343;245" or special keywords, like "auto", "red", "green", ...
	 * @param value - raw text value to be parsed
	 * @param color - output color
	 * @returns true if color is parsed
	 */
	static bool parseColor(const char* value, XmlColor& color, const char* delim=";");
	// Get attribute value from specified xml block
	static bool parseSID(const char* value, sID& sid, const char* delim=" ");
	//static sID getAttribSID(tinyxml2::XMLElement* element, const char* name, sID default_ = sID());
	static cPosition getAttribPos(tinyxml2::XMLElement* element, const char* name, cPosition default_ = cPosition(0,0));
	static int getValueInt (tinyxml2::XMLElement* block, const char* name, int default_ = 0);
	static float getValueFloat (tinyxml2::XMLElement* block, const char* name, float default_ = 0);
	static std::string getValueString (tinyxml2::XMLElement* block, const char* name, const char* attrib, const char* default_ = "");
	static bool getValueBool(tinyxml2::XMLElement* block, const char* name, bool default_ = false);

protected:
	std::unique_ptr<cSpriteTool> spriteTool;
	// Database for units.
	// We do not like referencing UnitsDataGlobal.
	cUnitsData* unitsData;
	// Root path for a mod
	std::string path;
	// Should we load media data
	bool loadMedia;
};

#endif // MODDATA_H

