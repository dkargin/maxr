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

#include "ui/graphical/menu/widgets/special/chatboxlandingplayerlistviewitem.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/image.h"
#include "game/data/player/player.h"


//------------------------------------------------------------------------------
cPlayerLandingStatus::cPlayerLandingStatus (const cPlayerBasicData& player_) :
	player (&player_),
	selectedPosition (false)
{}

//------------------------------------------------------------------------------
const cPlayerBasicData& cPlayerLandingStatus::getPlayer () const
{
	return *player;
}

//------------------------------------------------------------------------------
bool cPlayerLandingStatus::hasSelectedPosition () const
{
	return selectedPosition;
}

//------------------------------------------------------------------------------
void cPlayerLandingStatus::setHasSelectedPosition (bool value)
{
	std::swap (selectedPosition, value);
	if (value != selectedPosition) hasSelectedPositionChanged ();
}

//------------------------------------------------------------------------------
cChatBoxLandingPlayerListViewItem::cChatBoxLandingPlayerListViewItem (const cPlayerLandingStatus& playerLandingStatus_) :
	cAbstractListViewItem (cPosition (50, 0)),
	playerLandingStatus (&playerLandingStatus_)
{
	const auto& player = playerLandingStatus->getPlayer ();

	readyImage = addChild (std::make_unique<cImage> (getPosition () + cPosition (getSize ().x () - 10, 0)));

	colorImage = addChild (std::make_unique<cImage> (getPosition ()));

	updatePlayerColor ();
	updatePlayerHasSelectedPosition ();

	nameLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (colorImage->getEndPosition ().x () + 4, 0), getPosition () + cPosition (getSize ().x () - readyImage->getSize ().x (), readyImage->getSize ().y ())), player.getName ()));

	fitToChildren ();

	signalConnectionManager.connect (player.nameChanged, std::bind (&cChatBoxLandingPlayerListViewItem::updatePlayerName, this));
	signalConnectionManager.connect (player.colorChanged, std::bind (&cChatBoxLandingPlayerListViewItem::updatePlayerColor, this));
	signalConnectionManager.connect (playerLandingStatus->hasSelectedPositionChanged, std::bind (&cChatBoxLandingPlayerListViewItem::updatePlayerHasSelectedPosition, this));
}

//------------------------------------------------------------------------------
int cChatBoxLandingPlayerListViewItem::getPlayerNumber () const
{
	return playerLandingStatus->getPlayer ().getNr ();
}

//------------------------------------------------------------------------------
void cChatBoxLandingPlayerListViewItem::updatePlayerName ()
{
	nameLabel->setText (playerLandingStatus->getPlayer().getName ());
}

//------------------------------------------------------------------------------
void cChatBoxLandingPlayerListViewItem::updatePlayerColor ()
{
	SDL_Rect src = {0, 0, 10, 10};

	AutoSurface colorSurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_BlitSurface (playerLandingStatus->getPlayer().getColor ().getTexture (), &src, colorSurface.get (), nullptr);

	colorImage->setImage (colorSurface.get ());
}

//------------------------------------------------------------------------------
void cChatBoxLandingPlayerListViewItem::updatePlayerHasSelectedPosition ()
{
	SDL_Rect src = {playerLandingStatus->hasSelectedPosition() ? 10 : 0, 0, 10, 10};

	AutoSurface readySurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_SetColorKey (readySurface.get (), SDL_TRUE, cRgbColor (0, 1, 0).toMappedSdlRGBAColor (readySurface->format));
	SDL_BlitSurface (GraphicsData.gfx_player_ready.get (), &src, readySurface.get (), NULL);

	readyImage->setImage (readySurface.get ());
}

//------------------------------------------------------------------------------
void cChatBoxLandingPlayerListViewItem::handleResized (const cPosition& oldSize)
{
	cAbstractListViewItem::handleResized (oldSize);

	if (oldSize.x () == getSize ().x ()) return;

	readyImage->moveTo (getPosition () + cPosition (getSize ().x () - 10, 0));

	nameLabel->resize (cPosition (getSize ().x () - readyImage->getSize ().x () - colorImage->getSize ().x () - 4, readyImage->getSize ().y ()));

	fitToChildren ();
}