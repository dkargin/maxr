#include "game.h"

bool cGame::wantsToTerminate() const
{
	return terminate;
}

void cGame::exit()
{
	terminate = true;
	terminated();
}

void cGame::resetTerminating()
{
	terminate = false;
}


void cGame::setUnitsData(std::shared_ptr<const cUnitsData> unitsData_)
{
	unitsData = std::move(unitsData_);
}

std::shared_ptr<const cUnitsData> cGame::getUnitsData() const
{
	return unitsData;
}

void cGame::setClanData(std::shared_ptr<const cClanData> clanData_)
{
	clanData = std::move(clanData_);
}

std::shared_ptr<const cClanData> cGame::getClanData() const
{
	return clanData;
}
