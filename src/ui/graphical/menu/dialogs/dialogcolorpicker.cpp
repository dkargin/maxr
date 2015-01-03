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

#include "ui/graphical/menu/dialogs/dialogcolorpicker.h"

#include "ui/graphical/application.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/pushbutton.h"
#include "ui/graphical/menu/widgets/colorpicker.h"
#include "ui/graphical/menu/widgets/image.h"
#include "ui/graphical/menu/widgets/lineedit.h"
#include "ui/graphical/menu/widgets/tools/validatorint.h"
#include "pcx.h"
#include "main.h"

//------------------------------------------------------------------------------
cDialogColorPicker::cDialogColorPicker (const cRgbColor& color, eWindowBackgrounds backgroundType) :
	cWindow (LoadPCX (GFXOD_DIALOG2), backgroundType)
{
	colorPicker = addChild (std::make_unique<cRgbColorPicker> (cBox<cPosition> (getPosition() + cPosition (35, 35), getPosition() + cPosition (35 + 160, 35 + 135)), color));

	selectedColorImage = addChild (std::make_unique<cImage> (getPosition() + cPosition (210, 35)));
	selectedColorImage->setImage (createSelectedColorSurface().get());

	signalConnectionManager.connect (colorPicker->selectedColorChanged, [this]()
	{
		selectedColorImage->setImage (createSelectedColorSurface().get());

		const auto color = colorPicker->getSelectedColor();
		redValueLineEdit->setText (iToStr (color.r));
		greenValueLineEdit->setText (iToStr (color.g));
		blueValueLineEdit->setText (iToStr (color.b));
	});

	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (210, 100), getPosition() + cPosition (210 + 20, 100 + font->getFontHeight())), "R:"));
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (210, 120), getPosition() + cPosition (210 + 20, 120 + font->getFontHeight())), "G:"));
	addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition() + cPosition (210, 140), getPosition() + cPosition (210 + 20, 140 + font->getFontHeight())), "B:"));

	redValueLineEdit = addChild (std::make_unique<cLineEdit> (cBox<cPosition> (getPosition() + cPosition (230, 100), getPosition() + cPosition (230 + 25, 100 + font->getFontHeight()))));
	redValueLineEdit->setValidator (std::make_unique<cValidatorInt> (0, 255));
	greenValueLineEdit = addChild (std::make_unique<cLineEdit> (cBox<cPosition> (getPosition() + cPosition (230, 120), getPosition() + cPosition (230 + 25, 120 + font->getFontHeight()))));
	greenValueLineEdit->setValidator (std::make_unique<cValidatorInt> (0, 255));
	blueValueLineEdit = addChild (std::make_unique<cLineEdit> (cBox<cPosition> (getPosition() + cPosition (230, 140), getPosition() + cPosition (230 + 25, 140 + font->getFontHeight()))));
	blueValueLineEdit->setValidator (std::make_unique<cValidatorInt> (0, 255));

	redValueLineEdit->setText (iToStr (color.r));
	greenValueLineEdit->setText (iToStr (color.g));
	blueValueLineEdit->setText (iToStr (color.b));

	signalConnectionManager.connect (redValueLineEdit->returnPressed, [this]()
	{
		auto application = getActiveApplication();
		if (application) application->releaseKeyFocus (*redValueLineEdit);
	});
	signalConnectionManager.connect (greenValueLineEdit->returnPressed, [this]()
	{
		auto application = getActiveApplication();
		if (application) application->releaseKeyFocus (*redValueLineEdit);
	});
	signalConnectionManager.connect (blueValueLineEdit->returnPressed, [this]()
	{
		auto application = getActiveApplication();
		if (application) application->releaseKeyFocus (*redValueLineEdit);
	});
	signalConnectionManager.connect (redValueLineEdit->editingFinished, [this] (eValidatorState)
	{
		const auto color = colorPicker->getSelectedColor();
		const auto newRed = atoi (redValueLineEdit->getText().c_str());
		if (newRed != color.r)
		{
			colorPicker->setSelectedColor (color.exchangeRed (newRed));
		}
	});
	signalConnectionManager.connect (greenValueLineEdit->editingFinished, [this] (eValidatorState)
	{
		const auto color = colorPicker->getSelectedColor();
		const auto newGreen = atoi (greenValueLineEdit->getText().c_str());
		if (newGreen != color.g)
		{
			colorPicker->setSelectedColor (color.exchangeGreen (newGreen));
		}
	});
	signalConnectionManager.connect (blueValueLineEdit->editingFinished, [this] (eValidatorState)
	{
		const auto color = colorPicker->getSelectedColor();
		const auto newBlue = atoi (blueValueLineEdit->getText().c_str());
		if (newBlue != color.b)
		{
			colorPicker->setSelectedColor (color.exchangeBlue (newBlue));
		}
	});

	// FIXME: do not disable line edits here.
	//        currently it is disabled because the conversion from RGB to HSV Colors is not stable yet.
	//        This means converting a RGB color to HSV and then back to RGB will not result in the same
	//        RGB color that we started with.
	//        This results in confusing behavior when setting the RGB colors, because the value may will
	//        be changed by the color picking widget (which performs the conversions mentioned above) again.
	redValueLineEdit->disable();
	greenValueLineEdit->disable();
	blueValueLineEdit->disable();

	auto okButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (200, 185), ePushButtonType::Angular, lngPack.i18n ("Text~Others~OK"), FONT_LATIN_NORMAL));
	okButton->addClickShortcut (cKeySequence (cKeyCombination (eKeyModifierType::None, SDLK_RETURN)));
	signalConnectionManager.connect (okButton->clicked, [this]() { done(); });

	auto cencelButton = addChild (std::make_unique<cPushButton> (getPosition() + cPosition (111, 185), ePushButtonType::Angular, lngPack.i18n ("Text~Others~Cancel"), FONT_LATIN_NORMAL));
	cencelButton->addClickShortcut (cKeySequence (cKeyCombination (eKeyModifierType::None, SDLK_ESCAPE)));
	signalConnectionManager.connect (cencelButton->clicked, [this]() { canceled(); });
}

//------------------------------------------------------------------------------
cDialogColorPicker::~cDialogColorPicker()
{}

//------------------------------------------------------------------------------
cRgbColor cDialogColorPicker::getSelectedColor() const
{
	return colorPicker->getSelectedColor();
}

//------------------------------------------------------------------------------
AutoSurface cDialogColorPicker::createSelectedColorSurface()
{
	AutoSurface surface (SDL_CreateRGBSurface (0, 50, 50, 32, 0, 0, 0, 0));

	SDL_FillRect (surface.get(), nullptr, colorPicker->getSelectedColor().toMappedSdlRGBAColor (surface->format));

	return surface;
}