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

#ifndef game_logic_actionAttackH
#define game_logic_actionAttackH

#include "action.h"

class cUnit;

class cActionAttack : public cAction
{
public:
	cActionAttack(const cUnit& aggressor, cPosition targetPosition, const cUnit* targetUnit);
	cActionAttack(cBinaryArchiveOut& archive);

	virtual void serialize(cBinaryArchiveIn& archive) { cAction::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cAction::serialize(archive); serializeThis(archive); }

	virtual void execute(cModel& model) const override;
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & agressorId;
		archive & targetPosition;
		archive & targetId;
	}

	unsigned int agressorId;
	cPosition targetPosition;
	unsigned int targetId;

};

#endif // game_logic_actionAttackH
