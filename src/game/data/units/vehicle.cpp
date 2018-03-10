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
#include <cmath>

#include "vehicle.h"

#include "game/logic/attackjob.h"
#include "game/logic/automjobs.h"
#include "game/data/units/building.h"
#include "game/logic/client.h"
#include "game/logic/clientevents.h"
#include "utility/listhelpers.h"
#include "utility/files.h"
#include "game/logic/fxeffects.h"
#include "utility/log.h"
#include "game/data/map/map.h"
#include "utility/pcx.h"
#include "game/data/player/player.h"
#include "game/logic/server.h"
#include "settings.h"
#include "video.h"
#include "sound.h"
#include "utility/unifonts.h"
#include "input/mouse/mouse.h"
#include "output/sound/sounddevice.h"
#include "output/sound/soundchannel.h"
#include "ui/graphical/application.h"
#include "ui/graphical/menu/windows/windowbuildbuildings/windowbuildbuildings.h"
#include "ui/sound/soundmanager.h"
#include "ui/sound/effects/soundeffectvoice.h"

#include "utility/drawing.h"
#include "utility/random.h"
#include "utility/crc.h"

#include "game/logic/jobs/startbuildjob.h"
#include "game/data/map/mapview.h"
#include "game/data/map/mapfieldview.h"


using namespace std;

cVehicleData::cVehicleData()
{

}

bool cVehicleData::setGraphics(const std::string& layer, const cRenderablePtr& sprite)
{
	if(layer == "build")
		build = sprite;
	else if(layer == "build_shadow")
		build_shadow = sprite;
	else if(layer == "clear")
		clear = sprite;
	else if(layer == "clear_shadow")
		clear_shadow = sprite;
	else
		return cStaticUnitData::setGraphics(layer, sprite);
	return true;
}

void cVehicleData::render(cRenderable::sContext& context, const sRenderOps& ops) const
{
	cStaticUnitData::render(context, ops);
}


//-----------------------------------------------------------------------------
// cVehicle Class Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
cVehicle::cVehicle (sVehicleDataPtr sdata, const cDynamicUnitData& dynamicData, cPlayer* owner, unsigned int ID) :
	cUnit (&dynamicData, sdata, owner, ID),
		vehicleData(sdata),
	loaded (false),
	isBuilding (false),
	buildingTyp(),
	buildCosts (0),
	buildTurns (0),
	buildTurnsStart (0),
	buildCostsStart (0),
	isClearing (false),
	clearingTurns (0),
	layMines (false),
	clearMines (false),
	commandoRank (0),
	autoMoveJob(nullptr),
	tileMovementOffset(0, 0),
	buildBigSavedPosition(0, 0),
	bandPosition(0, 0),
	moveJob(nullptr)
{
	ditherX = 0;
	ditherY = 0;
	flightHeight = 0;
	WalkFrame = 0;
	hasAutoMoveJob = false;
	moving = false;
	BuildPath = false;
	bigBetonAlpha = 254;

	DamageFXPointX = random (7) + 26 - 3;
	DamageFXPointY = random (7) + 26 - 3;
	refreshData();

	clearingTurnsChanged.connect ([&]() { statusChanged(); });
	buildingTurnsChanged.connect ([&]() { statusChanged(); });
	buildingTypeChanged.connect ([&]() { statusChanged(); });
	commandoRankChanged.connect ([&]() { statusChanged(); });
	moveJobChanged.connect ([&]() { statusChanged(); });
	autoMoveJobChanged.connect ([&]() { statusChanged(); });
}

//-----------------------------------------------------------------------------
cVehicle::~cVehicle()
{
}

sVehicleDataPtr cVehicle::getVehicleData() const
{
	return this->vehicleData;
}

void cVehicle::render_BuildingOrBigClearing (const cMapView& map, unsigned long long animationTime, cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const
{
	int size = getCellSize();

	assert ((isUnitBuildingABuilding() || (isUnitClearing() && size > 1)) && job == nullptr);

	// draw beton if necessary
#ifdef FIX_THIS
	SDL_Rect tmp = dest;
	if (isUnitBuildingABuilding() && size > 1 && (!map.isWaterOrCoast (getPosition()) || map.getField (getPosition()).getBaseBuilding()))
	{
		SDL_SetSurfaceAlphaMod (GraphicsData.gfx_big_beton.get(), bigBetonAlpha);
		CHECK_SCALING (*GraphicsData.gfx_big_beton, *GraphicsData.gfx_big_beton_org, zoomFactor);
		SDL_BlitSurface (GraphicsData.gfx_big_beton.get(), nullptr, surface, &tmp);
	}
#endif

	vehicleData->renderFactionShadowSprite(vehicleData->build, vehicleData->build_shadow, context, ops);
#ifdef FUCK_THIS
	// draw shadow
	tmp = dest;
	if (ops.shadow)
		blitWithPreScale (vehicleData->build_shw_org.get(), vehicleData->build_shw.get(), nullptr, surface, &tmp, zoomFactor);

	// draw player color
	SDL_Rect src;
	src.y = 0;
	src.h = src.w = (int) (vehicleData->build_org->h * zoomFactor);
	src.x = (animationTime % 4) * src.w;
	SDL_BlitSurface (getOwner()->getColor().getTexture(), nullptr, GraphicsData.gfx_tmp.get(), nullptr);
	blitWithPreScale (vehicleData->build_org.get(), vehicleData->build.get(), &src, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor, 4);

	// draw vehicle
	src.x = 0;
	src.y = 0;
	tmp = dest;
	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_tmp.get(), 254);
	SDL_BlitSurface (GraphicsData.gfx_tmp.get(), &src, surface, &tmp);
#endif
}

void cVehicle::render_smallClearing(unsigned long long animationTime, cRenderContext& context, const cStaticUnitData::sRenderOps& ops) const
{
	int size = getCellSize();
	assert (isUnitClearing() && size == 1 && job == nullptr);

	vehicleData->renderFactionShadowSprite(vehicleData->build, vehicleData->build_shadow, context, ops);

#ifdef FUCK_THIS
	// draw shadow
	//SDL_Rect tmp = dest;
	if (ops.shadow)
		vehicleData->clearing_shadow->render(context);
		//blitWithPreScale (vehicleData->clear_small_shw_org.get(), vehicleData->clear_small_shw.get(), nullptr, surface, &tmp, zoomFactor);

	// draw player color
	SDL_Rect src;
	src.y = 0;
	src.h = src.w = (int) (vehicleData->clear_small_org->h * zoomFactor);
	src.x = (animationTime % 4) * src.w;
	SDL_BlitSurface (getOwner()->getColor().getTexture(), nullptr, GraphicsData.gfx_tmp.get(), nullptr);
	blitWithPreScale (vehicleData->clear_small_org.get(), vehicleData->clear_small.get(), &src, GraphicsData.gfx_tmp.get(), nullptr, zoomFactor, 4);

	// draw vehicle
	src.x = 0;
	src.y = 0;
	tmp = dest;
	SDL_SetSurfaceAlphaMod (GraphicsData.gfx_tmp.get(), 254);
	SDL_BlitSurface (GraphicsData.gfx_tmp.get(), &src, surface, &tmp);
#endif
}

void cVehicle::render (const cMapView* map, unsigned long long animationTime, const cPlayer* activePlayer, SDL_Surface* surface, const SDL_Rect& dest, const cStaticUnitData::sRenderOps& ops) const
{
	cRenderContext context;
	context.surface = surface;
	context.dstRect = dest;
	context.cache = true;
	context.channels["clan"] = this->getOwner()->getClan()+1;
	context.channels["animation"] = animationTime;
	context.channels["direction"] = dir;

	// draw working engineers and bulldozers:
	if (map && job == nullptr)
	{
		/// TODO: Make a proper rendering
		if (isUnitBuildingABuilding() || (isUnitClearing() && cellSize > 1))
		{
			render_BuildingOrBigClearing (*map, animationTime, context, ops);
			return;
		}
		if (isUnitClearing() && !cellSize == 1)
		{
			render_smallClearing (animationTime, context, ops);
			return;
		}
	}

	// draw all other vehicles:
	int alpha = 254;
	if (map)
	{
		if (alphaEffectValue && cSettings::getInstance().isAlphaEffects())
		{
			alpha = alphaEffectValue;
		}

		bool water = map->isWater (getPosition());
		// if the vehicle can also drive on land, we have to check,
		// whether there is a brige, platform, etc.
		// because the vehicle will drive on the bridge
		cBuilding* building = map->getField (getPosition()).getBaseBuilding();
		if(building && staticData->factorGround > 0)
		{
			const auto& surfacePosition = building->getStaticUnitData().surfacePosition;
			if (surfacePosition == cStaticUnitData::SURFACE_POS_BASE ||
				surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_SEA ||
				surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_BASE)
				water = false;
		}

		if (water && (staticData->isStealthOn & TERRAIN_SEA) && detectedByPlayerList.empty() && getOwner() == activePlayer)
			alpha = std::min (alpha, 100);

		context.channels["alphaeffect"] = alpha;
	}
	vehicleData->render(context, ops);
}

void cVehicle::proceedBuilding (cModel& model)
{
	if (isUnitBuildingABuilding() == false || getBuildTurns() == 0)
		return;

	setStoredResources (getStoredResources() - (getBuildCosts() / getBuildTurns()));
	setBuildCosts (getBuildCosts() - (getBuildCosts() / getBuildTurns()));

	setBuildTurns (getBuildTurns() - 1);
	if (getBuildTurns() != 0)
		return;

	const cMap& map = *model.getMap();
	auto building = model.getUnitsData()->getUnit(getBuildingType());
	if(!building)
	{
		// TODO: Shout something to the log!
		assert(false);
		return;
	}

	getOwner()->addTurnReportUnit (getBuildingType());

	// handle pathbuilding
	// here the new building is added (if possible) and
	// the move job to the next field is generated
	// the new build event is generated in cMoveJob::endMove()
	if (BuildPath)
	{
		// Find a next position that either
		// a) is something we can't move to
		//  (in which case we cancel the path building)
		// or b) doesn't have a building type that we're trying to build.
		cPosition nextPosition (getPosition());
		bool found_next  = false;

		while (!found_next && (nextPosition != bandPosition))
		{
			// Calculate the next position in the path.
			if (getPosition().x() > bandPosition.x()) nextPosition.x()--;
			if (getPosition().x() < bandPosition.x()) nextPosition.x()++;
			if (getPosition().y() > bandPosition.y()) nextPosition.y()--;
			if (getPosition().y() < bandPosition.y()) nextPosition.y()++;
			// Can we move to this position?
			// If not, we need to kill the path building now.
			if (!map.possiblePlace (*this, nextPosition, false))
			{
				// Try sidestepping stealth units before giving up.
				//model.sideStepStealthUnit (nextPosition, *this);
				if (!map.possiblePlace (*this, nextPosition, false))
				{
					// We can't build along this path any more.
					break;
				}
			}
			// Can we build at this next position?
			if (map.possiblePlaceBuilding (*building, nextPosition, nullptr))
			{
				// We can build here.
				found_next = true;
				break;
			}
		}

		//If we've found somewhere to move to, move there now.
		const cPosition oldPosition = getPosition();
		if (found_next && model.addMoveJob (*this, nextPosition))
		{
			model.addBuilding (oldPosition, getBuildingType(), getOwner());
			setBuildingABuilding (false);
		}
		else
		{
			if (building->surfacePosition != cStaticUnitData::SURFACE_POS_GROUND)
			{
				// add building immediately
				// if it doesn't require the engineer to drive away
				setBuildingABuilding (false);
				model.addBuilding (getPosition(), getBuildingType(), getOwner());
			}
			BuildPath = false;
			getOwner()->buildPathInterrupted(*this);
		}
	}
	else if (building->surfacePosition != staticData->surfacePosition)
	{
		// add building immediately
		// if it doesn't require the engineer to drive away
		setBuildingABuilding (false);
		model.addBuilding (getPosition(), getBuildingType(), getOwner());
	}
}

void cVehicle::continuePathBuilding(cModel& model)
{
	if (!BuildPath)
		return;

	auto building = model.getUnitsData()->getUnit(getBuildingType());
	if(!building)
	{
		// TODO: Shout something loud to the log
		assert(false);
	}

	if (getStoredResources() >= getBuildCostsStart() && model.getMap()->possiblePlaceBuilding(*building, getPosition(), nullptr, this))
	{
		model.addJob(new cStartBuildJob(*this, getPosition(), getCellSize()));
		setBuildingABuilding(true);
		setBuildCosts(getBuildCostsStart());
		setBuildTurns(getBuildTurnsStart());
	}
	else
	{
		BuildPath = false;
		getOwner()->buildPathInterrupted(*this);
	}
}

bool cVehicle::proceedClearing (cServer& server)
{
	if (isUnitClearing() == false || getClearingTurns() == 0)
		return false;

	setClearingTurns (getClearingTurns() - 1);

	cMap& map = *server.Map;

	if (getClearingTurns() != 0) return true;

	setClearing (false);
	cBuilding* Rubble = map.getField (getPosition()).getRubble();
	// This is another strange task, when clearing machine moves to a center of destroyed object
	if (cellSize > 1)
	{
		map.moveVehicle (*this, buildBigSavedPosition);
		sendStopClear (server, *this, buildBigSavedPosition, *getOwner());
		for (size_t i = 0; i != seenByPlayerList.size(); ++i)
		{
			sendStopClear (server, *this, buildBigSavedPosition, *seenByPlayerList[i]);
		}
	}
	else
	{
		sendStopClear (server, *this, cPosition (-1, -1), *getOwner());
		for (size_t i = 0; i != seenByPlayerList.size(); ++i)
		{
			sendStopClear (server, *this, cPosition (-1, -1), *seenByPlayerList[i]);
		}
	}
	setStoredResources (getStoredResources() + Rubble->getRubbleValue());
	//server.deleteRubble (Rubble);

	return true;
}

//-----------------------------------------------------------------------------
/** Initializes all unit data to its maxiumum values */
//-----------------------------------------------------------------------------

bool cVehicle::refreshData()
{
	// NOTE: according to MAX 1.04 units get their shots/movepoints back even if they are disabled

	if (data.getSpeed() < data.getSpeedMax() || data.getShots() < data.getShotsMax())
	{
		data.setSpeed (data.getSpeedMax());
		data.setShots (std::min (data.getAmmo(), data.getShotsMax()));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
/** Returns a string with the current state */
//-----------------------------------------------------------------------------
string cVehicle::getStatusStr(const cPlayer* player, const cUnitsData& unitsData) const
{
	if (isDisabled())
	{
		string sText;
		sText = lngPack.i18n ("Text~Comp~Disabled") + " (";
		sText += iToStr (getDisabledTurns()) + ")";
		return sText;
	}
	else if (autoMoveJob)
		return lngPack.i18n ("Text~Comp~Surveying");
	else if (isUnitBuildingABuilding())
	{
		const auto& building = unitsData.getUnit(getBuildingType());
		std::string name = building ? building->getName() : "UnknownBuilding";

		if (getOwner() != player)
			return lngPack.i18n ("Text~Comp~Producing");
		else
		{
			string sText;
			if (getBuildTurns())
			{
				sText = lngPack.i18n ("Text~Comp~Producing");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += name + " (";
				sText += iToStr (getBuildTurns());
				sText += ")";

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing");
					sText += ":\n";
					sText += name + " (";
					sText += iToStr (getBuildTurns());
					sText += ")";
				}
				return sText;
			}
			else //small building is rdy + activate after engineere moves away
			{
				sText = lngPack.i18n ("Text~Comp~Producing_Fin");
				sText += lngPack.i18n ("Text~Punctuation~Colon");
				sText += name;

				if (font->getTextWide (sText) > 126)
				{
					sText = lngPack.i18n ("Text~Comp~Producing_Fin");
					sText += ":\n";
					sText += name;
				}
				return sText;
			}
		}
	}
	else if (isUnitClearingMines())
		return lngPack.i18n ("Text~Comp~Clearing_Mine");
	else if (isUnitLayingMines())
		return lngPack.i18n ("Text~Comp~Laying");
	else if (isUnitClearing())
	{
		if (getClearingTurns())
		{
			string sText;
			sText = lngPack.i18n ("Text~Comp~Clearing") + " (";
			sText += iToStr (getClearingTurns()) + ")";
			return sText;
		}
		else
			return lngPack.i18n ("Text~Comp~Clearing_Fin");
	}

	// generate other infos for normal non-unit-related-events and infiltrators
	string sTmp;
	{
		if (moveJob && moveJob->getEndMoveAction().getType() == EMAT_ATTACK)
			sTmp = lngPack.i18n ("Text~Comp~MovingToAttack");
		else if (moveJob)
			sTmp = lngPack.i18n ("Text~Comp~Moving");
		else if (isAttacking())
			sTmp = lngPack.i18n ("Text~Comp~AttackingStatusStr");
		else if (isBeeingAttacked())
			sTmp = lngPack.i18n ("Text~Comp~IsBeeingAttacked");
		else if (isManualFireActive())
			sTmp = lngPack.i18n ("Text~Comp~ReactionFireOff");
		else if (isSentryActive())
			sTmp = lngPack.i18n ("Text~Comp~Sentry");
		else sTmp = lngPack.i18n ("Text~Comp~Waits");

		// extra info only for infiltrators
		// TODO should it be original behavior (as it is now) or
		// don't display CommandRank for enemy (could also be a bug in original...?)
		if ((staticData->hasFlag(UnitFlag::CanCapture) || staticData->hasFlag(UnitFlag::CanDisable)) /* && owner == gameGUI.getClient()->getActivePlayer()*/)
		{
			sTmp += "\n";
			if (getCommandoRank() < 1.f) sTmp += lngPack.i18n ("Text~Comp~CommandoRank_Greenhorn");
			else if (getCommandoRank() < 3.f) sTmp += lngPack.i18n ("Text~Comp~CommandoRank_Average");
			else if (getCommandoRank() < 6.f) sTmp += lngPack.i18n ("Text~Comp~CommandoRank_Veteran");
			else if (getCommandoRank() < 11.f) sTmp += lngPack.i18n ("Text~Comp~CommandoRank_Expert");
			else if (getCommandoRank() < 19.f) sTmp += lngPack.i18n ("Text~Comp~CommandoRank_Elite");
			else sTmp += lngPack.i18n ("Text~Comp~CommandoRank_GrandMaster");
			if (getCommandoRank() > 0.f)
				sTmp += " +" + iToStr ((int)getCommandoRank());

		}

		return sTmp;
	}

	return lngPack.i18n ("Text~Comp~Waits");
}

//-----------------------------------------------------------------------------
/** Reduces the remaining speedCur and shotsCur during movement */
//-----------------------------------------------------------------------------
void cVehicle::DecSpeed (int value)
{
	data.setSpeed (data.getSpeed() - value);

	if (staticData->canAttack == 0 || staticData->hasFlag(UnitFlag::CanDriveAndFire))
		return;

	const int s = data.getSpeed() * data.getShotsMax() / data.getSpeedMax();
	data.setShots (std::min (data.getShots(), s));
}

//-----------------------------------------------------------------------------
void cVehicle::calcTurboBuild (std::array<int, 3>& turboBuildTurns, std::array<int, 3>& turboBuildCosts, int buildCosts) const
{
	turboBuildTurns[0] = 0;
	turboBuildTurns[1] = 0;
	turboBuildTurns[2] = 0;

	// step 1x
	if (getStoredResources() >= buildCosts)
	{
		turboBuildCosts[0] = buildCosts;
		// prevent division by zero
		const auto needsMetal = staticData->needsMetal == 0 ? 1 : staticData->needsMetal;
		turboBuildTurns[0] = (int)ceilf (turboBuildCosts[0] / (float) (needsMetal));
	}

	// step 2x
	// calculate building time and costs
	int a = turboBuildCosts[0];
	int rounds = turboBuildTurns[0];
	int costs = turboBuildCosts[0];

	while (a >= 4 && getStoredResources() >= costs + 4)
	{
		rounds--;
		costs += 4;
		a -= 4;
	}

	if (rounds < turboBuildTurns[0] && rounds > 0 && turboBuildTurns[0])
	{
		turboBuildCosts[1] = costs;
		turboBuildTurns[1] = rounds;
	}

	// step 4x
	a = turboBuildCosts[1];
	rounds = turboBuildTurns[1];
	costs = turboBuildCosts[1];

	while (a >= 10 && costs < staticData->storageResMax - 2)
	{
		int inc = 24 - min (16, a);
		if (costs + inc > getStoredResources()) break;

		rounds--;
		costs += inc;
		a -= 16;
	}

	if (rounds < turboBuildTurns[1] && rounds > 0 && turboBuildTurns[1])
	{
		turboBuildCosts[2] = costs;
		turboBuildTurns[2] = rounds;
	}
}

//-----------------------------------------------------------------------------
/** Scans for resources */
//-----------------------------------------------------------------------------
void cVehicle::doSurvey()
{
	const auto& owner = *getOwner();

	int surveySize = 1;

	cBox<cPosition> aabb = getArea();
	const int minx = std::max (aabb.getMinCorner().x() - surveySize, 0);
	const int maxx = std::min (aabb.getMaxCorner().x() + surveySize, owner.getMapSize().x() - 1);
	const int miny = std::max (aabb.getMinCorner().y() - surveySize, 0);
	const int maxy = std::min (aabb.getMaxCorner().y() + surveySize, owner.getMapSize().y() - 1);

	for (int y = miny; y < maxy; ++y)
	{
		for (int x = minx; x < maxx; ++x)
		{
			const cPosition position (x, y);
			getOwner()->exploreResource (position);
		}
	}
}

//-----------------------------------------------------------------------------
/** Makes the report */
//-----------------------------------------------------------------------------
void cVehicle::makeReport (cSoundManager& soundManager) const
{
	if (isDisabled())
	{
		// Disabled:
		soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIUnitDisabledByEnemy)));
	}
	else if (data.getHitpoints() > data.getHitpointsMax() / 2)
	{
		// Status green
		if (moveJob && moveJob->getEndMoveAction().getType() == EMAT_ATTACK)
		{
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIAttacking)));
		}
		else if (autoMoveJob)
		{
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOISurveying)));
		}
		else if (data.getSpeed() == 0)
		{
			// no more movement
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, VoiceData.VOINoSpeed));
		}
		else if (isUnitBuildingABuilding())
		{
			// Beim bau:
			if (!getBuildTurns())
			{
				// Bau beendet:
				soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIBuildDone)));
			}
		}
		else if (isUnitClearing())
		{
			// removing dirt
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, VoiceData.VOIClearing));
		}
		else if (staticData->canAttack && data.getAmmo() <= data.getAmmoMax() / 4 && data.getAmmo() != 0)
		{
			// red ammo-status but still ammo left
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIAmmoLow)));
		}
		else if (staticData->canAttack && data.getAmmo() == 0)
		{
			// no ammo left
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIAmmoEmpty)));
		}
		else if (isSentryActive())
		{
			// on sentry:
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, VoiceData.VOISentry));
		}
		else if (isUnitClearingMines())
		{
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIClearingMines)));
		}
		else if (isUnitLayingMines())
		{
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, VoiceData.VOILayingMines));
		}
		else
		{
			soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIOK)));
		}
	}
	else if (data.getHitpoints() > data.getHitpointsMax() / 4)
	{
		// Status yellow:
		soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIStatusYellow)));
	}
	else
	{
		// Status red:
		soundManager.playSound (std::make_shared<cSoundEffectVoice> (eSoundEffectType::VoiceUnitStatus, getRandom (VoiceData.VOIStatusRed)));
	}
}

//-----------------------------------------------------------------------------
/** checks, if resources can be transferred to the unit */
//-----------------------------------------------------------------------------
bool cVehicle::canTransferTo(const cPosition& position, const cMapView& map) const
{
	const auto& field = map.getField(position);

	const cUnit* unit = field.getVehicle();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	unit = field.getTopBuilding();
	if (unit)
	{
		return canTransferTo(*unit);
	}

	return false;
}

//------------------------------------------------------------------------------
bool cVehicle::canTransferTo (const cUnit& unit) const
{
	if (!this->isNextTo(unit))
		return false;

	if (&unit == this)
		return false;

	if (unit.getOwner() != getOwner())
		return false;



	if (unit.isAVehicle())
	{
		const cVehicle* v = static_cast<const cVehicle*>(&unit);

		if (v->staticData->storeResType != staticData->storeResType)
			return false;

		if (v->isUnitBuildingABuilding() || v->isUnitClearing())
			return false;

		return true;
	}
	else if (unit.isABuilding())
	{
		const cBuilding* b = static_cast<const cBuilding*>(&unit);

		if (!b->subBase)
			return false;

		if (staticData->storeResType == eResourceType::Metal && b->subBase->getMaxMetalStored() == 0)
			return false;

		if (staticData->storeResType == eResourceType::Oil && b->subBase->getMaxOilStored() == 0)
			return false;

		if (staticData->storeResType == eResourceType::Gold && b->subBase->getMaxGoldStored() == 0)
			return false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::makeAttackOnThis (cModel& model, cUnit* opponentUnit, const string& reasonForLog) const
{
	cMapView mapView(model.getMap(), nullptr);
	const cUnit* target = cAttackJob::selectTarget (getPosition(), opponentUnit->getStaticUnitData().canAttack, mapView, getOwner());
	if (target != this) return false;

	Log.write (" cVehicle: " + reasonForLog + ": attacking (" + iToStr (getPosition().x()) + "," + iToStr (getPosition().y()) + "), Aggressor ID: " + iToStr (opponentUnit->iID) + ", Target ID: " + iToStr(target->getId()), cLog::eLOG_TYPE_NET_DEBUG);

	model.addAttackJob (*opponentUnit, getPosition());

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::makeSentryAttack (cModel& model, cUnit* sentryUnit) const
{
	cMapView mapView(model.getMap(), nullptr);
	if (sentryUnit != 0 && sentryUnit->isSentryActive() && sentryUnit->canAttackObjectAt (getPosition(), mapView, true))
	{
		if (makeAttackOnThis (model, sentryUnit, "sentry reaction"))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::inSentryRange (cModel& model)
{
	for (const auto& player : model.getPlayerList())
	{
		if (player.get() == getOwner()) continue;

		// Don't attack invisible units
		if (!player->canSeeUnit (*this, *model.getMap())) continue;

		// Check sentry type
		if (staticData->factorAir > 0 && player->hasSentriesAir (getPosition()) == 0) continue;
		// Check sentry type
		if (staticData->factorAir == 0 && player->hasSentriesGround (getPosition()) == 0) continue;

		const auto& vehicles = player->getVehicles();
		for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
		{
			const auto& vehicle = *i;
			if (makeSentryAttack (model, vehicle.get()))
				return true;
		}
		const auto& buildings = player->getBuildings();
		for (auto i = buildings.begin(); i != buildings.end(); ++i)
		{
			const auto& building = *i;
			if (makeSentryAttack (model, building.get()))
				return true;
		}
	}

	return provokeReactionFire (model);
}

//-----------------------------------------------------------------------------
bool cVehicle::isOtherUnitOffendedByThis(const cModel& model, const cUnit& otherUnit) const
{
	// don't treat the cheap buildings
	// (connectors, roads, beton blocks) as offendable
	if (otherUnit.isABuilding() && model.getUnitsData()->getDynamicData(otherUnit.data.getId()).getBuildCost() <= 2)
		return false;

	cMapView mapView(model.getMap(), nullptr);
	if (isInWeaponRange (otherUnit.getPosition()) && canAttackObjectAt (otherUnit.getPosition(), mapView, true, false))
	{
		// test, if this vehicle can really attack the opponentVehicle
		cUnit* target = cAttackJob::selectTarget (otherUnit.getPosition(), staticData->canAttack, mapView, getOwner());
		if (target == &otherUnit)
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doesPlayerWantToFireOnThisVehicleAsReactionFire(const cModel& model, const cPlayer* player) const
{
	if (model.getGameSettings()->getGameType() == eGameSettingsGameType::Turns)
	{
		// In the turn based game style,
		// the opponent always fires on the unit if he can,
		// regardless if the unit is offending or not.
		return true;
	}
	else
	{
		// check if there is a vehicle or building of player, that is offended

		const auto& vehicles = player->getVehicles();
		for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
		{
			const auto& opponentVehicle = *i;
			if (isOtherUnitOffendedByThis (model, *opponentVehicle))
				return true;
		}
		const auto& buildings = player->getBuildings();
		for (auto i = buildings.begin(); i != buildings.end(); ++i)
		{
			const auto& opponentBuilding = *i;
			if (isOtherUnitOffendedByThis (model, *opponentBuilding))
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doReactionFireForUnit (cModel& model, cUnit* opponentUnit) const
{
	cMapView mapView(model.getMap(), nullptr);
	if (opponentUnit->isSentryActive() == false && opponentUnit->isManualFireActive() == false
		&& opponentUnit->canAttackObjectAt (getPosition(), mapView, true)
		// Possible TODO: better handling of stealth units.
		// e.g. do reaction fire, if already detected ?
		&& (opponentUnit->isAVehicle() == false || opponentUnit->getStaticUnitData().isStealthOn == TERRAIN_NONE))
	{
		if (makeAttackOnThis (model, opponentUnit, "reaction fire"))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::doReactionFire (cModel& model, cPlayer* player) const
{
	// search a unit of the opponent, that could fire on this vehicle
	// first look for a building
	const auto& buildings = player->getBuildings();
	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& opponentBuilding = *i;
		if (doReactionFireForUnit (model, opponentBuilding.get()))
			return true;
	}
	const auto& vehicles = player->getVehicles();
	for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
	{
		const auto& opponentVehicle = *i;
		if (doReactionFireForUnit (model, opponentVehicle.get()))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::provokeReactionFire (cModel& model)
{
	// unit can't fire, so it can't provoke a reaction fire
	if (staticData->canAttack == false || data.getShots() <= 0 || data.getAmmo() <= 0)
		return false;

	const auto& playerList = model.getPlayerList();
	for (size_t i = 0; i != playerList.size(); ++i)
	{
		cPlayer& player = *playerList[i];
		if (&player == getOwner())
			continue;

		// The vehicle can't be seen by the opposing player.
		// No possibility for reaction fire.
		if (!player.canSeeUnit (*this, *model.getMap()))
			continue;

		if (!doesPlayerWantToFireOnThisVehicleAsReactionFire (model, &player))
			continue;

		if (doReactionFire (model, &player))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::canExitTo(const cPosition& position, const cMapView& map, const cStaticUnitData& unitData) const
{
	if (!map.possiblePlaceVehicle(unitData, position))
		return false;
	if (staticData->factorAir > 0 && (position != getPosition()))
		return false;
	if (!isNextTo(position, unitData.cellSize, unitData.cellSize))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::canExitTo(const cPosition& position, const cMap& map, const cStaticUnitData& unitData) const
{
	if (!map.possiblePlaceVehicle(unitData, position, getOwner()))
		return false;
	if (staticData->factorAir > 0 && (position != getPosition()))
		return false;
	if (!isNextTo(position, unitData.cellSize, unitData.cellSize))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
bool cVehicle::canLoad (const cPosition& position, const cMapView& map, bool checkPosition) const
{
	if (map.isValidPosition (position) == false) return false;

	return canLoad (map.getField (position).getVehicle(), checkPosition);
}

//-----------------------------------------------------------------------------
bool cVehicle::canLoad (const cVehicle* vehicle, bool checkPosition) const
{
	if (loaded) return false;

	if (!vehicle) return false;

	if (vehicle->isUnitLoaded()) return false;

	if (storedUnits.size() >= static_cast<unsigned> (staticData->storageUnitsMax)) return false;


	if (checkPosition && !isNextTo (*vehicle))
		return false;

	if (checkPosition && staticData->factorAir > 0 && (vehicle->getPosition() != getPosition())) return false;

	if (!Contains (staticData->storeUnitsTypes, vehicle->getStaticUnitData().isStorageType)) return false;

	if (vehicle->moving || vehicle->isAttacking()) return false;

	if (vehicle->getOwner() != getOwner() || vehicle->isUnitBuildingABuilding() || vehicle->isUnitClearing()) return false;

	if (vehicle->isBeeingAttacked()) return false;

	return true;
}

//-----------------------------------------------------------------------------
/** Checks, if an object can get ammunition. */
//-----------------------------------------------------------------------------
bool cVehicle::canSupply (const cMapView& map, const cPosition& position, int supplyType) const
{
	if (map.isValidPosition (position) == false) return false;

	const auto& field = map.getField (position);
	if (field.getVehicle()) return canSupply (field.getVehicle(), supplyType);
	else if (field.getPlane()) return canSupply (field.getPlane(), supplyType);
	else if (field.getTopBuilding()) return canSupply (field.getTopBuilding(), supplyType);

	return false;
}

//-----------------------------------------------------------------------------
bool cVehicle::canSupply (const cUnit* unit, int supplyType) const
{
	if (unit == 0)
		return false;

	if (getStoredResources() <= 0)
		return false;

	if (isNextTo (*unit) == false)
		return false;

	if (unit->isAVehicle() && unit->getStaticUnitData().factorAir > 0 && static_cast<const cVehicle*> (unit)->getFlightHeight() > 0)
		return false;

	switch (supplyType)
	{
		case SUPPLY_TYPE_REARM:
			if (unit == this || unit->getStaticUnitData().canAttack == false || unit->data.getAmmo() >= unit->data.getAmmoMax()
				|| (unit->isAVehicle() && static_cast<const cVehicle*> (unit)->isUnitMoving())
				|| unit->isAttacking())
				return false;
			break;
		case SUPPLY_TYPE_REPAIR:
			if (unit == this || unit->data.getHitpoints() >= unit->data.getHitpointsMax()
				|| (unit->isAVehicle() && static_cast<const cVehicle*> (unit)->isUnitMoving())
				|| unit->isAttacking())
				return false;
			break;
		default:
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
void cVehicle::layMine (cModel& model)
{
	if (getStoredResources() <= 0)
		return;

	const cMap& map = *model.getMap();

	const auto& staticMineData = model.getUnitsData()->getBuilding(vehicleData->sweepBuildObject);
	if (!map.possiblePlaceBuilding(*staticMineData, getPosition(), nullptr, this))
		return;

	model.addBuilding(getPosition(), staticMineData->ID, getOwner(), false);
	setStoredResources (getStoredResources() - 1);

	if (getStoredResources() <= 0)
		setLayMines (false);

	return;
}

//-----------------------------------------------------------------------------
void cVehicle::clearMine (cModel& model)
{
	const cMap& map = *model.getMap();
	cBuilding* mine = map.getField (getPosition()).getMine();

	if (!mine || mine->getOwner() != getOwner() || getStoredResources() >= staticData->storageResMax) return;

	// sea minelayer can't collect land mines and vice versa
	if (mine->getStaticUnitData().factorGround > 0 && staticData->factorGround == 0) return;
	if (mine->getStaticUnitData().factorSea > 0 && staticData->factorSea == 0) return;

	model.deleteUnit(mine);
	setStoredResources (getStoredResources() + 1);

	if (getStoredResources() >= staticData->storageResMax) setClearMines (false);

	return;
}

//-----------------------------------------------------------------------------
/** Checks if the target is on a neighbor field and if it can be stolen or disabled */
//-----------------------------------------------------------------------------
bool cVehicle::canDoCommandoAction (const cPosition& position, const cMapView& map, bool steal) const
{
	const auto& field = map.getField (position);

	const cUnit* unit = field.getPlane();
	if (canDoCommandoAction (unit, steal)) return true;

	unit = field.getVehicle();
	if (canDoCommandoAction (unit, steal)) return true;

	unit = field.getBuilding();
	if (canDoCommandoAction (unit, steal)) return true;

	return false;
}

bool cVehicle::canDoCommandoAction (const cUnit* unit, bool steal) const
{
	if (unit == nullptr) return false;

	if ((steal && !hasStaticFlag(UnitFlag::CanCapture)) || (steal == false && !hasStaticFlag(UnitFlag::CanDisable)))
		return false;
	if (data.getShots() == 0) return false;

	if (isNextTo (*unit) == false)
		return false;

	if (steal == false && unit->isDisabled())
		return false;
	if (unit->isABuilding() && static_cast<const cBuilding*>(unit)->isRubble())
		return false;
	if (steal && !unit->getStaticUnitData().hasFlag(UnitFlag::CanBeCaptured))
		return false;
	if (steal == false && !unit->getStaticUnitData().hasFlag(UnitFlag::CanBeDisabled))
		return false;
	if (steal && unit->storedUnits.empty() == false)
		return false;
	if (unit->getOwner() == getOwner())
		return false;
	if (unit->isAVehicle() && unit->getStaticUnitData().factorAir > 0 && static_cast<const cVehicle*> (unit)->getFlightHeight() > 0)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
int cVehicle::calcCommandoChance (const cUnit* destUnit, bool steal) const
{
	if (destUnit == 0)
		return 0;

	// TODO: include cost research and clan modifications ?
	// Or should always the basic version without clanmods be used ?
	// TODO: Bug for buildings ? /3? or correctly /2,
	// because constructing buildings takes two resources per turn ?
	int destTurn = destUnit->data.getBuildCost() / 3;

	int factor = steal ? 1 : 4;
	int srcLevel = (int) getCommandoRank() + 7;

	// The chance to disable or steal a unit depends on
	// the infiltrator ranking and the buildcosts
	// (or 'turns' in the original game) of the target unit.
	// The chance rises linearly with a higher ranking of the infiltrator.
	// The chance of a unexperienced infiltrator will be calculated like
	// he has the ranking 7.
	// Disabling has a 4 times higher chance than stealing.
	int chance = Round ((8.f * srcLevel) / (35 * destTurn) * factor * 100);
	chance = std::min (90, chance);

	return chance;
}

//-----------------------------------------------------------------------------
int cVehicle::calcCommandoTurns (const cUnit* destUnit) const
{
	if (destUnit == 0)
		return 1;

	const int vehiclesTable[13] = { 0, 0, 0, 5, 8, 3, 3, 0, 0, 0, 1, 0, -4 };
	int destTurn, srcLevel;

	if (destUnit->isAVehicle())
	{
		destTurn = destUnit->data.getBuildCost() / 3;
		srcLevel = (int) getCommandoRank();
		if (destTurn > 0 && destTurn < 13)
			srcLevel += vehiclesTable[destTurn];
	}
	else
	{
		destTurn = destUnit->data.getBuildCost() / 2;
		srcLevel = (int) getCommandoRank() + 8;
	}

	int turns = (int) (1.0f / destTurn * srcLevel);
	turns = std::max (turns, 1);
	return turns;
}

//-----------------------------------------------------------------------------
bool cVehicle::isDetectedByPlayer (const cPlayer* player) const
{
	return Contains (detectedByPlayerList, player);
}

//-----------------------------------------------------------------------------
void cVehicle::setDetectedByPlayer(cPlayer* player, bool addToDetectedInThisTurnList /*= true*/)
{
	//TODO: make voice / text massage for owner and player
	bool wasDetected = (detectedByPlayerList.empty() == false);

	if (!isDetectedByPlayer (player))
		detectedByPlayerList.push_back (player);


	if (addToDetectedInThisTurnList && Contains (detectedInThisTurnByPlayerList, player) == false)
		detectedInThisTurnByPlayerList.push_back (player);
}

//-----------------------------------------------------------------------------
void cVehicle::resetDetectedByPlayer (cServer& server, cPlayer* player)
{
	bool wasDetected = (detectedByPlayerList.empty() == false);

	Remove (detectedByPlayerList, player);
	Remove (detectedInThisTurnByPlayerList, player);

	if (wasDetected && detectedByPlayerList.empty()) sendDetectionState (server, *this);
}

//-----------------------------------------------------------------------------
bool cVehicle::wasDetectedInThisTurnByPlayer (const cPlayer* player) const
{
	return Contains (detectedInThisTurnByPlayerList, player);
}

//-----------------------------------------------------------------------------
void cVehicle::clearDetectedInThisTurnPlayerList()
{
	detectedInThisTurnByPlayerList.clear();
}

//-----------------------------------------------------------------------------
void cVehicle::tryResetOfDetectionStateAfterMove (cServer& server)
{
	std::vector<cPlayer*> playersThatDetectThisVehicle = calcDetectedByPlayer (server);

	bool foundPlayerToReset = true;
	while (foundPlayerToReset)
	{
		foundPlayerToReset = false;
		for (unsigned int i = 0; i < detectedByPlayerList.size(); i++)
		{
			if (Contains (playersThatDetectThisVehicle, detectedByPlayerList[i]) == false
				&& Contains (detectedInThisTurnByPlayerList, detectedByPlayerList[i]) == false)
			{
				resetDetectedByPlayer (server, detectedByPlayerList[i]);
				foundPlayerToReset = true;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
std::vector<cPlayer*> cVehicle::calcDetectedByPlayer (cServer& server) const
{
	std::vector<cPlayer*> playersThatDetectThisVehicle;
	// check whether the vehicle has been detected by others
	if (staticData->isStealthOn != TERRAIN_NONE)  // the vehicle is a stealth vehicle
	{
		cMap& map = *server.Map;
		const auto& playerList = server.playerList;
		for (unsigned int i = 0; i < playerList.size(); i++)
		{
			cPlayer& player = *playerList[i];
			if (&player == getOwner())
				continue;
			bool isOnWater = map.isWater (getPosition());
			bool isOnCoast = map.isCoast (getPosition()) && (isOnWater == false);

			// if the vehicle can also drive on land, we have to check,
			// whether there is a brige, platform, etc.
			// because the vehicle will drive on the bridge
			const cBuilding* building = map.getField (getPosition()).getBaseBuilding();
			if (staticData->factorGround > 0 && building
				&& (building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_BASE
				|| building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_SEA
				|| building->getStaticUnitData().surfacePosition == cStaticUnitData::SURFACE_POS_ABOVE_BASE))
			{
				isOnWater = false;
				isOnCoast = false;
			}

			if ((staticData->isStealthOn & TERRAIN_GROUND)
				&& (player.hasLandDetection(getPosition()) || (!(staticData->isStealthOn & TERRAIN_COAST) && isOnCoast)
					|| isOnWater))
			{
				playersThatDetectThisVehicle.push_back (&player);
			}

			if ((staticData->isStealthOn & TERRAIN_SEA)
				&& (player.hasSeaDetection (getPosition()) || isOnWater == false))
			{
				playersThatDetectThisVehicle.push_back (&player);
			}
		}
	}
	return playersThatDetectThisVehicle;
}

//-----------------------------------------------------------------------------
void cVehicle::makeDetection (cServer& server)
{
	// check whether the vehicle has been detected by others
	std::vector<cPlayer*> playersThatDetectThisVehicle = calcDetectedByPlayer (server);
	for (unsigned int i = 0; i < playersThatDetectThisVehicle.size(); i++)
		setDetectedByPlayer (playersThatDetectThisVehicle[i]);

	// detect other units
	if (staticData->canDetectStealthOn == false) return;

	cMap& map = *server.Map;
	const int minx = std::max (getPosition().x() - data.getScan(), 0);
	const int maxx = std::min (getPosition().x() + data.getScan(), map.getSize().x() - 1);
	const int miny = std::max (getPosition().y() - data.getScan(), 0);
	const int maxy = std::min (getPosition().y() + data.getScan(), map.getSize().x() - 1);

	for (int x = minx; x <= maxx; ++x)
	{
		for (int y = miny; y <= maxy; ++y)
		{
			const cPosition position (x, y);

			cVehicle* vehicle = map.getField (position).getVehicle();
			cBuilding* building = map.getField (position).getMine();

			if (vehicle && vehicle->getOwner() != getOwner())
			{
				if ((staticData->canDetectStealthOn & TERRAIN_GROUND) && getOwner()->hasLandDetection(position) && (vehicle->getStaticUnitData().isStealthOn & TERRAIN_GROUND))
				{
					vehicle->setDetectedByPlayer (getOwner());
				}
				if ((staticData->canDetectStealthOn & TERRAIN_SEA) && getOwner()->hasSeaDetection(position) && (vehicle->getStaticUnitData().isStealthOn & TERRAIN_SEA))
				{
					vehicle->setDetectedByPlayer (getOwner());
				}
			}
			if (building && building->getOwner() != getOwner())
			{
				if ((staticData->canDetectStealthOn & AREA_EXP_MINE) && getOwner()->hasMineDetection(position) && (building->getStaticUnitData().isStealthOn & AREA_EXP_MINE))
				{
					building->setDetectedByPlayer (getOwner());
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
cBuilding* cVehicle::getContainerBuilding()
{
	if (!isUnitLoaded()) return nullptr;

	const auto& buildings = getOwner()->getBuildings();
	for (auto i = buildings.begin(); i != buildings.end(); ++i)
	{
		const auto& building = *i;
		if (Contains (building->storedUnits, this)) return building.get();
	}

	return nullptr;
}

cVehicle* cVehicle::getContainerVehicle()
{
	if (!isUnitLoaded()) return nullptr;

	const auto& vehicles = getOwner()->getVehicles();
	for (auto i = vehicles.begin(); i != vehicles.end(); ++i)
	{
		const auto& vehicle = *i;
		if (Contains (vehicle->storedUnits, this)) return vehicle.get();
	}

	return nullptr;
}

uint32_t cVehicle::getChecksum(uint32_t crc) const
{
	crc = cUnit::getChecksum(crc);
	crc = calcCheckSum(hasAutoMoveJob, crc);
	crc = calcCheckSum(bandPosition, crc);
	crc = calcCheckSum(buildBigSavedPosition, crc);
	crc = calcCheckSum(BuildPath, crc);
	crc = calcCheckSum(WalkFrame, crc);
	for ( const auto& p : detectedInThisTurnByPlayerList)
		crc = calcCheckSum(p, crc);
	//std::shared_ptr<cAutoMJob> autoMoveJob;
	crc = calcCheckSum(tileMovementOffset, crc);
	crc = calcCheckSum(loaded, crc);
	crc = calcCheckSum(moving, crc);
	crc = calcCheckSum(isBuilding, crc);
	crc = calcCheckSum(buildingTyp, crc);
	crc = calcCheckSum(buildCosts, crc);
	crc = calcCheckSum(buildTurns, crc);
	crc = calcCheckSum(buildTurnsStart, crc);
	crc = calcCheckSum(buildCostsStart, crc);
	crc = calcCheckSum(isClearing, crc);
	crc = calcCheckSum(clearingTurns, crc);
	crc = calcCheckSum(layMines, crc);
	crc = calcCheckSum(clearMines, crc);
	crc = calcCheckSum(flightHeight, crc);
	crc = calcCheckSum(commandoRank, crc);

	return crc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- methods, that already have been extracted as part of cUnit refactoring
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool cVehicle::canBeStoppedViaUnitMenu() const
{
	return (moveJob != nullptr || (isUnitBuildingABuilding() && getBuildTurns() > 0) || (isUnitClearing() && getClearingTurns() > 0));
}

//-----------------------------------------------------------------------------
void cVehicle::executeAutoMoveJobCommand (cClient& client)
{
	if (!hasStaticFlag(UnitFlag::CanSurvey))
		return;
	if (!autoMoveJob)
	{
		startAutoMoveJob (client);
	}
	else
	{
		stopAutoMoveJob();
	}
}

//-----------------------------------------------------------------------------
bool cVehicle::canLand (const cMap& map) const
{
	// normal vehicles are always "landed"
	if (staticData->factorAir == 0) return true;

	if (moveJob != nullptr || isAttacking()) return false;      //vehicle busy?

	// TODO: We should properly check our size
	// landing pad there?

	bool canLand = false;

	const auto& field = map.getField(getPosition());
	auto owner = getOwner();

	for (const cBuilding* building: field.getBuildings())
	{
		if (building->hasStaticFlag(UnitFlag::CanBeLandedOn) && building->getOwner() == owner)
		{
			canLand = true;
			break;
		}
	}

	if (!canLand)
		return false;

	// is the landing pad already occupied?
	for (const cVehicle* vehicle: field.getPlanes())
	{
		// Check if vehicle is not 'self' and not landed already
		if (vehicle->getFlightHeight() < 64 && vehicle->iID != iID)
			return false;
	}

	// returning true before checking owner, because a stolen vehicle
	// can stay on an enemy landing pad until it is moved
	if (getFlightHeight() == 0)
		return true;

	return true;
}

//-----------------------------------------------------------------------------
void cVehicle::setMoving (bool value)
{
	std::swap (moving, value);
	if (value != moving) movingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setLoaded (bool value)
{
	std::swap (loaded, value);
	if (value != loaded)
	{
		if (loaded) stored();
		else activated();
	}
}

//-----------------------------------------------------------------------------
void cVehicle::setClearing (bool value)
{
	std::swap (isClearing, value);
	if (value != isClearing) clearingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildingABuilding (bool value)
{
	std::swap (isBuilding, value);
	if (value != isBuilding) buildingChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setLayMines (bool value)
{
	std::swap (layMines, value);
	if (value != layMines) layingMinesChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::setClearMines (bool value)
{
	std::swap (clearMines, value);
	if (value != clearMines) clearingMinesChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getClearingTurns() const
{
	return clearingTurns;
}

//-----------------------------------------------------------------------------
void cVehicle::setClearingTurns (int value)
{
	std::swap (clearingTurns, value);
	if (value != clearingTurns) clearingTurnsChanged();
}

//-----------------------------------------------------------------------------
float cVehicle::getCommandoRank() const
{
	return commandoRank;
}

//-----------------------------------------------------------------------------
void cVehicle::setCommandoRank (float value)
{
	std::swap (commandoRank, value);
	if (value != commandoRank) commandoRankChanged();
}

//-----------------------------------------------------------------------------
const sID& cVehicle::getBuildingType() const
{
	return buildingTyp;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildingType (const sID& id)
{
	auto oldId = id;
	buildingTyp = id;
	if (buildingTyp != oldId) buildingTypeChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildCosts() const
{
	return buildCosts;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildCosts (int value)
{
	std::swap (buildCosts, value);
	if (value != buildCosts) buildingCostsChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildTurns() const
{
	return buildTurns;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildTurns (int value)
{
	std::swap (buildTurns, value);
	if (value != buildTurns) buildingTurnsChanged();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildCostsStart() const
{
	return buildCostsStart;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildCostsStart (int value)
{
	std::swap (buildCostsStart, value);
	//if (value != buildCostsStart) event ();
}

//-----------------------------------------------------------------------------
int cVehicle::getBuildTurnsStart() const
{
	return buildTurnsStart;
}

//-----------------------------------------------------------------------------
void cVehicle::setBuildTurnsStart (int value)
{
	std::swap (buildTurnsStart, value);
	//if (value != buildTurnsStart) event ();
}

//-----------------------------------------------------------------------------
int cVehicle::getFlightHeight() const
{
	return flightHeight;
}

//-----------------------------------------------------------------------------
void cVehicle::setFlightHeight (int value)
{
	value = std::min (std::max (value, 0), 64);
	std::swap (flightHeight, value);
	if (flightHeight != value) flightHeightChanged();
}

//-----------------------------------------------------------------------------
cMoveJob* cVehicle::getMoveJob()
{
	return moveJob;
}

//-----------------------------------------------------------------------------
const cMoveJob* cVehicle::getMoveJob() const
{
	return moveJob;
}

//-----------------------------------------------------------------------------
void cVehicle::setMoveJob (cMoveJob* moveJob_)
{
	std::swap (moveJob, moveJob_);
	if (moveJob != moveJob_) moveJobChanged();
}

//-----------------------------------------------------------------------------
cAutoMJob* cVehicle::getAutoMoveJob()
{
	return autoMoveJob.get();
}

//-----------------------------------------------------------------------------
const cAutoMJob* cVehicle::getAutoMoveJob() const
{
	return autoMoveJob.get();
}

//-----------------------------------------------------------------------------
void cVehicle::startAutoMoveJob (cClient& client)
{
	if (autoMoveJob) return;

	autoMoveJob = std::make_shared<cAutoMJob> (client, *this);
	client.addAutoMoveJob (autoMoveJob);

	autoMoveJobChanged();
}

//-----------------------------------------------------------------------------
void cVehicle::stopAutoMoveJob()
{
	if (autoMoveJob)
	{
		autoMoveJob->stop();
		autoMoveJob = nullptr;
		autoMoveJobChanged();
	}
}
