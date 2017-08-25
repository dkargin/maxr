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

#include "actionattack.h"

#include "game/data/model.h"

#include "utility/log.h"

//------------------------------------------------------------------------------
cActionAttack::cActionAttack(const cUnit& aggressor, cPosition targetPosition, const cUnit* targetUnit) :
	cAction(eActiontype::ACTION_ATTACK),
	agressorId(aggressor.getId()),
	targetPosition(targetPosition),
	targetId(targetUnit? targetUnit->getId() : 0)
{};

//------------------------------------------------------------------------------
cActionAttack::cActionAttack(cBinaryArchiveOut& archive) :
	cAction(eActiontype::ACTION_ATTACK)
{
	serializeThis(archive);
}

//------------------------------------------------------------------------------
void cActionAttack::execute(cModel& model) const
{
	//Note: this function handles incoming data from network. Make every possible sanity check!

	//validate aggressor
	cUnit* aggressor = model.getUnitFromID(agressorId);
	if (aggressor == nullptr) return;

	if (aggressor->getOwner()->getId() != playerNr) return;
	if (aggressor->isBeeingAttacked()) return;

	//validate target
	if (!model.getMap()->isValidPosition(targetPosition)) return;

	cPosition validatedTargetPosition = targetPosition;
	if (targetId != 0)
	{
		cUnit* target = model.getUnitFromID(targetId);
		if (target == nullptr) return;

		if (!target->isABuilding() && !target->getIsBig())
		{
			if (targetPosition != target->getPosition())
			{
				Log.write(" cActionAttack: Target coords changed to (" + iToStr(target->getPosition().x()) + "," + iToStr(target->getPosition().y()) + ") to match current unit position", cLog::eLOG_TYPE_NET_DEBUG);
			}
			validatedTargetPosition = target->getPosition();

		}
	}

	// check if attack is possible
	if (aggressor->canAttackObjectAt(validatedTargetPosition, *model.getMap(), true) == false)
	{
		Log.write(" cActionAttack: Attack is not possible", cLog::eLOG_TYPE_NET_WARNING);
		return;
	}
	model.addAttackJob(*aggressor, validatedTargetPosition);
}
