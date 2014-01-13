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
#ifndef baseH
#define baseH

#include <vector>
#include "main.h" // for sUnitData and sUnitData::eStorageResType

class cBuilding;
class cMap;
class cNetMessage;
class cPlayer;
class cServer;

struct sSubBase
{
public:
	explicit sSubBase (cPlayer* owner_);
	/**
	* integrates all building of the given subbase in the own one
	* @author eiko
	*/
	void merge (sSubBase* sb);

	/**
	* returns an unique number to identify the subbase
	* @author eiko
	*/
	int getID() const;

	void addBuilding (cBuilding* b);

	/**
	* adds/subtracts a ressource to/from the subbase
	* @author eiko
	*/
	void addMetal (cServer& server, int value);
	void addOil (cServer& server, int value);
	void addGold (cServer& server, int value);

	/**
	* recalculates the values of all subbases
	* @author eiko
	*/
	void refresh();

	/**
	* inreases the energy production of the subbase by
	* starting offline generators/stations
	* @author eiko
	*/
	bool increaseEnergyProd (cServer& server, int value);

	//-----------------------------------
	//turn end management:

	/**
	* checks if consumers have to be switched off, due to a lack of ressources
	* @return returns true, if consumers have been shut down
	* @author eiko
	*/
	bool checkGoldConsumer (cServer& server);
	bool checkHumanConsumer (cServer& server);
	bool checkMetalConsumer (cServer& server);
	/**
	* - switches off unneeded fuel consumers(=energy producers)
	* - sets the optimal amount of generators and stations
	*   to minimize fuel consumption
	* - increases oil production, if necessary
	* - switches off oil consumers, if too few oil is available
	* @return: returns true, if oil consumers have been shut down,
	*          due to a lack of oil
	* @author eiko
	*/
	bool checkOil (cServer& server);
	/**
	* switches off energy consumers, if necessary
	* @return returns true, if a energy consumers have been shut down
	* @author eiko
	*/
	bool checkEnergy (cServer& server);
	/**
	* checks, if there are consumers, that have to be shut down,
	* due to a lack of a ressources
	* @author eiko
	*/
	void prepareTurnend (cServer& server);
	/**
	* produce ressources, repair/reload buildings etc.
	* @author eiko
	*/
	void makeTurnend (cServer& server);

	//------------------------------------
	//ressource management:

	/** returns the maximum production of a ressource */
	int getMaxMetalProd() const;
	int getMaxGoldProd() const;
	int getMaxOilProd() const;

	/** returns the maximum allowed production
	 * (without decreasing one of the other ones) of a ressource */
	int getMaxAllowedMetalProd() const;
	int getMaxAllowedGoldProd() const;
	int getMaxAllowedOilProd() const;

	/** returns the current production of a ressource */
	int getMetalProd() const;
	int getGoldProd() const;
	int getOilProd() const;

	/** sets the production of a ressource.
	 * If value is bigger then maxAllowed,
	 * it will be reduced to the maximum allowed value */
	void setMetalProd (int value);
	void setGoldProd (int value);
	void setOilProd (int value);

	/** changes the production of a ressource by value. */
	void changeMetalProd (int value);
	void changeGoldProd (int value);
	void changeOilProd (int value);

	bool isDitributionMaximized() const;

	void pushInto (cNetMessage& message) const;
	void popFrom (cNetMessage& message);
private:

	void makeTurnend_reparation (cServer& server, cBuilding& building);
	void makeTurnend_reload (cServer& server, cBuilding& building);
	void makeTurnend_build (cServer& server, cBuilding& building);


	/**
	* calcs the maximum allowed production of a ressource,
	* without decreasing the production of the other two
	* @author eiko
	*/
	int calcMaxAllowedProd (int ressourceType) const;
	/**
	* calcs the maximum possible production of a ressource
	* @author eiko
	*/
	int calcMaxProd (int ressourceType) const;
	/**
	* adds/subtracts ressourcec of the type storeResType to/from the subbase
	* @author eiko
	*/
	void addRessouce (cServer& server, sUnitData::eStorageResType storeResType, int value);

public:
	//private:
	//	friend class cBase;
	std::vector<cBuilding*> buildings;
	cPlayer* owner;

	int MaxMetal;
	int Metal;
	int MaxOil;
	int Oil;
	int MaxGold;
	int Gold;

	int MaxEnergyProd;
	int EnergyProd;
	int MaxEnergyNeed;
	int EnergyNeed;
	int MetalNeed;
	int OilNeed;
	int GoldNeed;
	int MaxMetalNeed;
	int MaxOilNeed;
	int MaxGoldNeed;

	int HumanProd;
	int HumanNeed;
	int MaxHumanNeed;

	int MetalProd;
	int OilProd;
	int GoldProd;
};

class cBase
{
public:
	cBase();
	~cBase();

	/**
	* adds a building to the base and updates the subbase structures
	* @param building the building, that is added to the base
	* @param server when not null, the resulting subbase values are sent to the client
	* @author eiko
	*/
	void addBuilding (cBuilding* building, cServer* server);
	/**
	* deletes a building from the base and updates the subbase structures
	* @param building the building, that is deleted to the base
	* @param server when not null, the resulting subbase values are sent to the client
	* @author eiko
	*/
	void deleteBuilding (cBuilding* building, cServer* server);
	void handleTurnend (cServer& server);
	/**
	* recalculates the values of all subbases
	*@author eiko
	*/
	void refreshSubbases();
	sSubBase* checkNeighbour (int iOff, const cBuilding& Building);

public:
	std::vector<sSubBase*> SubBases;
	cMap* map;
};

#endif
