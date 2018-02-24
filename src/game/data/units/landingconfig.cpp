#include "landingunit.h"

#include "game/data/gamesettings.h"

//------------------------------------------------------------------------------
void createInitial(sLandingConfig& config, const cGameSettings& gameSettings, const cUnitsData& unitsData)
{
    if (gameSettings.getBridgeheadType() == eGameSettingsBridgeheadType::Mobile)
        return;

    if(config.state != 0)
        return;

    int clan = config.clan;

#ifdef FIX_THIS
    const auto& constructorID = unitsData.getConstructorData().ID;
    const auto& engineerID = unitsData.getEngineerData().ID;
    const auto& surveyorID = unitsData.getSurveyorData().ID;

    config.landingUnits.push_back(sLandingUnit::make(constructorID, 40, true));
    config.landingUnits.push_back(sLandingUnit::make(engineerID, 20, true));
    config.landingUnits.push_back(sLandingUnit::make(surveyorID, 0, true));

    const auto& smallGenData = unitsData.getSmallGeneratorData();
    const auto& mineData = unitsData.getMineData();

    config.baseLayout.push_back(cBaseLayoutItem{cPosition(-smallGenData.cellSize, 0), smallGenData.ID});
    config.baseLayout.push_back(cBaseLayoutItem{cPosition(0, 0), mineData.ID});

    if (clan == 7)
    {
        const int startCredits = gameSettings.getStartCredits();

        size_t numAddConstructors = 0;
        size_t numAddEngineers = 0;

        if (startCredits < 100)
        {
            numAddEngineers = 1;
        }
        else if (startCredits < 150)
        {
            numAddEngineers = 1;
            numAddConstructors = 1;
        }
        else if (startCredits < 200)
        {
            numAddEngineers = 2;
            numAddConstructors = 1;
        }
        else if (startCredits < 300)
        {
            numAddEngineers = 2;
            numAddConstructors = 2;
        }
        else
        {
            numAddEngineers = 3;
            numAddConstructors = 2;
        }

        for (size_t i = 0; i != numAddConstructors; ++i)
        {
            config.landingUnits.push_back (sLandingUnit::make(constructorID, 0, true));
        }
        for (size_t i = 0; i != numAddEngineers; ++i)
        {
            config.landingUnits.push_back (sLandingUnit::make(engineerID, 0, true));
        }
    }
#endif
    config.state = 1;
}
