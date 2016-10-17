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

#include "ui/graphical/game/hud.h"

#include "ui/graphical/game/widgets/unitvideowidget.h"
#include "ui/graphical/game/widgets/unitdetailshud.h"
#include "ui/graphical/game/widgets/unitrenamewidget.h"
#include "ui/graphical/game/widgets/turntimeclockwidget.h"

#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/menu/widgets/checkbox.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/lineedit.h"
#include "ui/graphical/menu/widgets/slider.h"

#include "defines.h"
#include "settings.h"
#include "video.h"
#include "pcx.h"
#include "main.h"
#include "game/data/units/unit.h"
#include "keys.h"
#include "game/logic/turnclock.h"
#include "game/logic/turntimeclock.h"

//------------------------------------------------------------------------------
cHud::cHud (std::shared_ptr<cAnimationTimer> animationTimer) :
	player (nullptr)
{
	surface = generateSurface();
	resize (cPosition (surface->w, surface->h));

	endButton = addChild (std::make_unique<cPushButton> (cPosition (391, 4), ePushButtonType::HudEnd, lngPack.i18n ("Text~Others~End"), FONT_LATIN_NORMAL));
	endButton->addClickShortcut (KeysList.keyEndTurn);
	signalConnectionManager.connect (endButton->clicked, [&]() { endClicked(); });

	auto preferencesButton = addChild (std::make_unique<cPushButton> (cPosition (86, 4), ePushButtonType::HudPreferences, lngPack.i18n ("Text~Others~Settings"), FONT_LATIN_SMALL_WHITE));
	signalConnectionManager.connect (preferencesButton->clicked, [&]() { preferencesClicked(); });
	auto filesButton = addChild (std::make_unique<cPushButton> (cPosition (17, 3), ePushButtonType::HudFiles, lngPack.i18n ("Text~Others~Files"), FONT_LATIN_SMALL_WHITE));
	signalConnectionManager.connect (filesButton->clicked, [&]() { filesClicked(); });

	surveyButton = addChild (std::make_unique<cCheckBox> (cPosition (2, 296), lngPack.i18n ("Text~Others~Survey"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_00, false, &SoundData.SNDHudSwitch));
	surveyShortcut = &surveyButton->addClickShortcut (KeysList.keySurvey);
	signalConnectionManager.connect (surveyButton->toggled, [&]() { surveyToggled(); });

	hitsButton = addChild (std::make_unique<cCheckBox> (cPosition (57, 296), lngPack.i18n ("Text~Others~Hitpoints_7"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_01, false, &SoundData.SNDHudSwitch));
	hitsShortcut = &hitsButton->addClickShortcut (KeysList.keyHitpoints);
	signalConnectionManager.connect (hitsButton->toggled, [&]() { hitsToggled(); });

	scanButton = addChild (std::make_unique<cCheckBox> (cPosition (112, 296), lngPack.i18n ("Text~Others~Scan"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_02, false, &SoundData.SNDHudSwitch));
	scanShortcut = &scanButton->addClickShortcut (KeysList.keyScan);
	signalConnectionManager.connect (scanButton->toggled, [&]() { scanToggled(); });

	statusButton = addChild (std::make_unique<cCheckBox> (cPosition (2, 296 + 18), lngPack.i18n ("Text~Others~Status"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_10, false, &SoundData.SNDHudSwitch));
	statusShortcut = &statusButton->addClickShortcut (KeysList.keyStatus);
	signalConnectionManager.connect (statusButton->toggled, [&]() { statusToggled(); });

	ammoButton = addChild (std::make_unique<cCheckBox> (cPosition (57, 296 + 18), lngPack.i18n ("Text~Others~Ammo"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_11, false, &SoundData.SNDHudSwitch));
	ammoShortcut = &ammoButton->addClickShortcut (KeysList.keyAmmo);
	signalConnectionManager.connect (ammoButton->toggled, [&]() { ammoToggled(); });

	gridButton = addChild (std::make_unique<cCheckBox> (cPosition (112, 296 + 18), lngPack.i18n ("Text~Others~Grid"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_12, false, &SoundData.SNDHudSwitch));
	gridShortcut = &gridButton->addClickShortcut (KeysList.keyGrid);
	signalConnectionManager.connect (gridButton->toggled, [&]() { gridToggled(); });

	colorButton = addChild (std::make_unique<cCheckBox> (cPosition (2, 296 + 18 + 16), lngPack.i18n ("Text~Others~Color"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_20, false, &SoundData.SNDHudSwitch));
	colorShortcut = &colorButton->addClickShortcut (KeysList.keyColors);
	signalConnectionManager.connect (colorButton->toggled, [&]() { colorToggled(); });

	rangeButton = addChild (std::make_unique<cCheckBox> (cPosition (57, 296 + 18 + 16), lngPack.i18n ("Text~Others~Range"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_21, false, &SoundData.SNDHudSwitch));
	rangeShortcut = &rangeButton->addClickShortcut (KeysList.keyRange);
	signalConnectionManager.connect (rangeButton->toggled, [&]() { rangeToggled(); });

	fogButton = addChild (std::make_unique<cCheckBox> (cPosition (112, 296 + 18 + 16), lngPack.i18n ("Text~Others~Fog"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudIndex_22, false, &SoundData.SNDHudSwitch));
	fogShortcut = &fogButton->addClickShortcut (KeysList.keyFog);
	signalConnectionManager.connect (fogButton->toggled, [&]() { fogToggled(); });

	lockButton = addChild (std::make_unique<cCheckBox> (cPosition (32, 227), eCheckBoxType::HudLock, false, &SoundData.SNDHudSwitch));
	signalConnectionManager.connect (lockButton->toggled, [&]() { lockToggled(); });

	miniMapAttackUnitsOnlyButton = addChild (std::make_unique<cCheckBox> (cPosition (136, 413), eCheckBoxType::HudTnt, false, &SoundData.SNDHudSwitch));
	signalConnectionManager.connect (miniMapAttackUnitsOnlyButton->toggled, [&]() { miniMapAttackUnitsOnlyToggled(); });
	miniMapZoomFactorButton = addChild (std::make_unique<cCheckBox> (cPosition (136, 387), eCheckBoxType::Hud2x, false, &SoundData.SNDHudSwitch));
	signalConnectionManager.connect (miniMapZoomFactorButton->toggled, [&]() { miniMapZoomFactorToggled(); });

	auto helpButton = addChild (std::make_unique<cPushButton> (cPosition (20, 250), ePushButtonType::HudHelp));
	signalConnectionManager.connect (helpButton->clicked, [&]() { helpClicked(); });
	auto centerButton = addChild (std::make_unique<cPushButton> (cPosition (4, 227), ePushButtonType::HudCenter));
	centerButton->addClickShortcut (KeysList.keyCenterUnit);
	signalConnectionManager.connect (centerButton->clicked, [&]() { centerClicked(); });

	auto reportsButton = addChild (std::make_unique<cPushButton> (cPosition (101, 252), ePushButtonType::HudReport, lngPack.i18n ("Text~Others~Log")));
	signalConnectionManager.connect (reportsButton->clicked, [&]() { reportsClicked(); });
	chatButton = addChild (std::make_unique<cCheckBox> (cPosition (51, 252), lngPack.i18n ("Text~Others~Chat"), FONT_LATIN_SMALL_WHITE, eCheckBoxTextAnchor::Left, eCheckBoxType::HudChat));
	signalConnectionManager.connect (chatButton->toggled, [&]() { chatToggled(); });

	auto nextButton = addChild (std::make_unique<cPushButton> (cPosition (124, 227), ePushButtonType::HudNext, ">>"));
	nextButton->addClickShortcut (KeysList.keyUnitNext);
	signalConnectionManager.connect (nextButton->clicked, [&]() { nextClicked(); });
	auto prevButton = addChild (std::make_unique<cPushButton> (cPosition (60, 227), ePushButtonType::HudPrev, "<<"));
	prevButton->addClickShortcut (KeysList.keyUnitPrev);
	signalConnectionManager.connect (prevButton->clicked, [&]() { prevClicked(); });
	auto doneButton = addChild (std::make_unique<cPushButton> (cPosition (99, 227), ePushButtonType::HudDone, lngPack.i18n ("Text~Others~Proceed_4")));
	doneButton->addClickShortcut (KeysList.keyUnitDone);
	signalConnectionManager.connect (doneButton->clicked, [&]() { doneClicked(); });

	coordsLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (cPosition (265, getEndPosition().y() - 18), cPosition (265 + 64, getEndPosition().y() - 18 + 10)), "", FONT_LATIN_NORMAL, eAlignmentType::CenterHorizontal));
	unitNameLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (cPosition (343, getEndPosition().y() - 18), cPosition (343 + 212, getEndPosition().y() - 18 + 10)), "", FONT_LATIN_NORMAL, eAlignmentType::CenterHorizontal));
	turnLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (cPosition (471, 7), cPosition (471 + 55, 7 + 10)), "", FONT_LATIN_NORMAL, eAlignmentType::CenterHorizontal));
	turnTimeClockWidget = addChild (std::make_unique<cTurnTimeClockWidget> (cBox<cPosition> (cPosition (537, 7), cPosition (537 + 55, 7 + 10))));

	zoomSlider = addChild (std::make_unique<cSlider> (cBox<cPosition> (cPosition (22, 275), cPosition (22 + 126, 275 + 15)), 0, 100, eOrientationType::Horizontal, eSliderHandleType::HudZoom, eSliderType::Invisible));
	signalConnectionManager.connect (zoomSlider->valueChanged, [&]() { zoomChanged(); });
	auto zoomPlusButton = addChild (std::make_unique<cPushButton> (cBox<cPosition> (cPosition (2, 275), cPosition (2 + 15, 275 + 15))));
	signalConnectionManager.connect (zoomPlusButton->clicked, std::bind (&cHud::handleZoomPlusClicked, this));
	auto zoomMinusButton = addChild (std::make_unique<cPushButton> (cBox<cPosition> (cPosition (152, 275), cPosition (152 + 15, 275 + 15))));
	signalConnectionManager.connect (zoomMinusButton->clicked, std::bind (&cHud::handleZoomMinusClicked, this));

	unitVideo = addChild (std::make_unique<cUnitVideoWidget> (cBox<cPosition> (cPosition (10, 29), cPosition (10 + 125, 29 + 125)), animationTimer));
	signalConnectionManager.connect (unitVideo->clicked, [this]()
	{
		if (unitVideo->hasAnimation ())
		{
			unitVideo->toggle ();
		}
	});
	auto playButton = addChild (std::make_unique<cPushButton> (cPosition (146, 123), ePushButtonType::HudPlay));
	signalConnectionManager.connect (playButton->clicked, std::bind (&cUnitVideoWidget::start, unitVideo));
	auto stopButton = addChild (std::make_unique<cPushButton> (cPosition (146, 143), ePushButtonType::HudStop));
	signalConnectionManager.connect (stopButton->clicked, std::bind (&cUnitVideoWidget::stop, unitVideo));

	unitDetails = addChild (std::make_unique<cUnitDetailsHud> (cBox<cPosition> (cPosition (8, 171), cPosition (8 + 155, 171 + 48))));

	unitRenameWidget = addChild (std::make_unique<cUnitRenameWidget> (cPosition (12, 30), 123));
	signalConnectionManager.connect (unitRenameWidget->unitRenameTriggered, [&]()
	{
		if (unitRenameWidget->getUnit())
		{
			triggeredRenameUnit (*unitRenameWidget->getUnit(), unitRenameWidget->getUnitName());
		}
	});
}

//------------------------------------------------------------------------------
void cHud::setPlayer (std::shared_ptr<const cPlayer> player_)
{
	player = std::move (player_);
	unitRenameWidget->setPlayer (player.get());
	unitDetails->setPlayer (player.get());
}

//------------------------------------------------------------------------------
void cHud::setTurnClock (std::shared_ptr<const cTurnClock> turnClock_)
{
	turnClock = std::move (turnClock_);

	turnClockSignalConnectionManager.disconnectAll();
	if (turnClock != nullptr)
	{
		turnLabel->setText (iToStr (turnClock->getTurn()));
		turnClockSignalConnectionManager.connect (turnClock->turnChanged, [&]()
		{
			turnLabel->setText (iToStr (turnClock->getTurn()));
		});
	}
}

//------------------------------------------------------------------------------
void cHud::setTurnTimeClock (std::shared_ptr<const cTurnTimeClock> turnTimeClock)
{
	turnTimeClockWidget->setTurnTimeClock (std::move (turnTimeClock));
}

//------------------------------------------------------------------------------
void cHud::setGameSettings (std::shared_ptr<const cGameSettings> gameSettings)
{
	unitDetails->setGameSettings (std::move (gameSettings));
}

//------------------------------------------------------------------------------
bool cHud::isAt (const cPosition& position) const
{
	cBox<cPosition> hole (cPosition (panelLeftWidth, panelTopHeight), getEndPosition() - cPosition (panelRightWidth, panelBottomHeight));

	if (hole.withinOrTouches (position))
	{
		// end button reaches into the hole
		return endButton->isAt (position);
	}
	return true;
}

//------------------------------------------------------------------------------
void cHud::draw (SDL_Surface& destination, const cBox<cPosition>& clipRect)
{
	if (surface != nullptr)
	{
		auto position = getArea().toSdlRect();
		//SDL_Rect rect = {0, getEndPosition ().y () - panelBottomHeight * 2, getSize ().x (), panelBottomHeight * 2};
		SDL_BlitSurface (surface.get(), nullptr, &destination, &position);
	}

	cWidget::draw (destination, clipRect);
}

//------------------------------------------------------------------------------
AutoSurface cHud::generateSurface()
{
	AutoSurface surface (SDL_CreateRGBSurface (0, Video.getResolutionX(), Video.getResolutionY(), Video.getColDepth(), 0, 0, 0, 0));

	SDL_FillRect (surface.get(), nullptr, 0x00FF00FF);
	SDL_SetColorKey (surface.get(), SDL_TRUE, 0x00FF00FF);

	const std::string gfxPath = cSettings::getInstance().getGfxPath() + PATH_DELIMITER;
	{
		AutoSurface tmpSurface (LoadPCX (gfxPath + "hud_left.pcx"));
		if (tmpSurface != nullptr)
		{
			SDL_BlitSurface (tmpSurface.get(), nullptr, surface.get(), nullptr);
		}
	}

	SDL_Rect dest;
	{
		AutoSurface tmpSurface (LoadPCX (gfxPath + "hud_top.pcx"));
		if (tmpSurface != nullptr)
		{
			SDL_Rect src = {0, 0, Uint16 (tmpSurface->w), Uint16 (tmpSurface->h)};
			dest.x = panelLeftWidth;
			dest.y = 0;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
			src.x = 1275;
			src.w = 18;
			src.h = panelTopHeight;
			dest.x = surface->w - panelTopHeight;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
		}
	}

	{
		AutoSurface tmpSurface (LoadPCX (gfxPath + "hud_right.pcx"));
		if (tmpSurface != nullptr)
		{
			SDL_Rect src = {0, 0, Uint16 (tmpSurface->w), Uint16 (tmpSurface->h)};
			dest.x = surface->w - panelRightWidth;
			dest.y = panelTopHeight;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
		}
	}

	{
		AutoSurface tmpSurface (LoadPCX (gfxPath + "hud_bottom.pcx"));
		if (tmpSurface != nullptr)
		{
			SDL_Rect src = {0, 0, Uint16 (tmpSurface->w), Uint16 (tmpSurface->h)};
			dest.x = panelLeftWidth;
			dest.y = surface->h - 24;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
			src.x = 1275;
			src.w = 23;
			src.h = 24;
			dest.x = surface->w - 23;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
			src.x = 1299;
			src.w = 16;
			src.h = 22;
			dest.x = panelLeftWidth - 16;
			dest.y = surface->h - 22;
			SDL_BlitSurface (tmpSurface.get(), &src, surface.get(), &dest);
		}
	}

	if (Video.getResolutionY() > 480)
	{
		AutoSurface tmpSurface (LoadPCX (gfxPath + "logo.pcx"));
		if (tmpSurface != nullptr)
		{
			dest.x = 9;
			dest.y = Video.getResolutionY() - panelTotalHeight - 15;
			SDL_BlitSurface (tmpSurface.get(), nullptr, surface.get(), &dest);
		}
	}
	return surface;
}

//------------------------------------------------------------------------------
void cHud::setMinimalZoomFactor (float zoomFactor)
{
	zoomSlider->setMaxValue ((int)std::ceil (100. - 100. * zoomFactor));
}

//------------------------------------------------------------------------------
void cHud::setZoomFactor (float zoomFactor)
{
	zoomSlider->setValue (static_cast<int> (100. - zoomFactor * 100));
}

//------------------------------------------------------------------------------
float cHud::getZoomFactor() const
{
	return 1.f - (float)zoomSlider->getValue() / 100;
}

//------------------------------------------------------------------------------
void cHud::increaseZoomFactor (double percent)
{
	zoomSlider->increase ((int) ((zoomSlider->getMaxValue() - zoomSlider->getMinValue()) * percent));
}

//------------------------------------------------------------------------------
void cHud::decreaseZoomFactor (double percent)
{
	zoomSlider->decrease ((int) ((zoomSlider->getMaxValue() - zoomSlider->getMinValue()) * percent));
}

//------------------------------------------------------------------------------
void cHud::lockEndButton()
{
	endButton->lock();
}

//------------------------------------------------------------------------------
void cHud::unlockEndButton()
{
	endButton->unlock();
}

//------------------------------------------------------------------------------
void cHud::setSurveyActive (bool value)
{
	surveyButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getSurveyActive() const
{
	return surveyButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setHitsActive (bool value)
{
	hitsButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getHitsActive() const
{
	return hitsButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setScanActive (bool value)
{
	scanButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getScanActive() const
{
	return scanButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setStatusActive (bool value)
{
	statusButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getStatusActive() const
{
	return statusButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setAmmoActive (bool value)
{
	ammoButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getAmmoActive() const
{
	return ammoButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setGridActive (bool value)
{
	gridButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getGridActive() const
{
	return gridButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setColorActive (bool value)
{
	colorButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getColorActive() const
{
	return colorButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setRangeActive (bool value)
{
	rangeButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getRangeActive() const
{
	return rangeButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setFogActive (bool value)
{
	fogButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getFogActive() const
{
	return fogButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setLockActive (bool value)
{
	lockButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getLockActive() const
{
	return lockButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setChatActive (bool value)
{
	chatButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getChatActive() const
{
	return chatButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setMiniMapZoomFactorActive (bool value)
{
	miniMapZoomFactorButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getMiniMapZoomFactorActive() const
{
	return miniMapZoomFactorButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setMiniMapAttackUnitsOnly (bool value)
{
	miniMapAttackUnitsOnlyButton->setChecked (value);
}

//------------------------------------------------------------------------------
bool cHud::getMiniMapAttackUnitsOnly() const
{
	return miniMapAttackUnitsOnlyButton->isChecked();
}

//------------------------------------------------------------------------------
void cHud::setCoordinatesText (const std::string& text)
{
	coordsLabel->setText (text);
}

//------------------------------------------------------------------------------
void cHud::setUnitNameText (const std::string& text)
{
	unitNameLabel->setText (text);
}

//------------------------------------------------------------------------------
void cHud::startUnitVideo()
{
	unitVideo->start();
}

//------------------------------------------------------------------------------
void cHud::stopUnitVideo()
{
	unitVideo->stop();
}

//------------------------------------------------------------------------------
bool cHud::isUnitVideoPlaying()
{
	return unitVideo->isPlaying();
}

//------------------------------------------------------------------------------
void cHud::resizeToResolution()
{
	surface = generateSurface();
	resize (cPosition (surface->w, surface->h));

	coordsLabel->moveTo (cPosition (265, getEndPosition().y() - 18));
	unitNameLabel->moveTo (cPosition (343, getEndPosition().y() - 18));
}

//------------------------------------------------------------------------------
void cHud::activateShortcuts()
{
	surveyShortcut->activate();
	hitsShortcut->activate();
	scanShortcut->activate();
	statusShortcut->activate();
	ammoShortcut->activate();
	gridShortcut->activate();
	colorShortcut->activate();
	rangeShortcut->activate();
	fogShortcut->activate();
}

//------------------------------------------------------------------------------
void cHud::deactivateShortcuts()
{
	surveyShortcut->deactivate();
	hitsShortcut->deactivate();
	scanShortcut->deactivate();
	statusShortcut->deactivate();
	ammoShortcut->deactivate();
	gridShortcut->deactivate();
	colorShortcut->deactivate();
	rangeShortcut->deactivate();
	fogShortcut->deactivate();
}

//------------------------------------------------------------------------------
void cHud::handleZoomPlusClicked()
{
	zoomSlider->decrease ((zoomSlider->getMaxValue() - zoomSlider->getMinValue()) / 6);
}

//------------------------------------------------------------------------------
void cHud::handleZoomMinusClicked()
{
	zoomSlider->increase ((zoomSlider->getMaxValue() - zoomSlider->getMinValue()) / 6);
}

//------------------------------------------------------------------------------
void cHud::setActiveUnit (const cUnit* unit)
{
	unitRenameWidget->setUnit (unit);
	unitVideo->setUnit (unit);
	unitDetails->setUnit (unit);
}