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

#ifndef game_logic_jobs_airtransportloadjobH
#define game_logic_jobs_airtransportloadjobH

#include "job.h"

#include "utility/serialization/binaryarchive.h"
#include "utility/serialization/xmlarchive.h"
#include "game/data/units/unit.h"
#include "utility/signal/signalconnectionmanager.h"
#include "game/data/units/vehicle.h"


/**
* creates the animation, when a air transporter loads a vehicle. This is a
* job, not a gui animation, because it triggers an action on the model.
*/
class cAirTransportLoadJob : public cJob
{
public:
	cAirTransportLoadJob (cVehicle& loadedVehicle, cUnit& loadingUnit);
	template <typename T>
	cAirTransportLoadJob (T& archive);

	virtual void run (cModel& model) MAXR_OVERRIDE_FUNCTION;
	virtual eJobType getType() const MAXR_OVERRIDE_FUNCTION;

	virtual void serialize(cBinaryArchiveIn& archive) { archive << serialization::makeNvp("type", getType()); serializeThis(archive); }
	virtual void serialize(cXmlArchiveIn& archive) { archive << serialization::makeNvp("type", getType()); serializeThis(archive); }

	virtual uint32_t getChecksum(uint32_t crc) const MAXR_OVERRIDE_FUNCTION;
private:
	template <typename T>
	void serializeThis(T& archive)
	{
		archive & NVP(unit);
		archive & NVP(vehicleToLoad);
		archive & NVP(landing);
	}

	cVehicle* vehicleToLoad;
	cSignalConnectionManager connectionManager;
	bool landing;
};

template <typename T>
cAirTransportLoadJob::cAirTransportLoadJob(T& archive)
{
	serializeThis(archive);
	if (!unit || !vehicleToLoad)
	{
		finished = true;
		return;
	}
	connectionManager.connect(vehicleToLoad->destroyed, [&](){finished = true; });
	unit->job = this;
}

#endif
