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

#include "ui/graphical/menu/widgets/special/lobbyplayerlistviewitem.h"
#include "ui/graphical/menu/widgets/label.h"
#include "ui/graphical/menu/widgets/image.h"
#include "game/data/player/player.h"

//------------------------------------------------------------------------------
cLobbyPlayerListViewItem::cLobbyPlayerListViewItem (std::shared_ptr<cPlayerBasicData> player_) :
	cAbstractListViewItem (cPosition (50, 0)),
	player (std::move (player_))
{
	assert (player != nullptr);

	readyImage = addChild (std::make_unique<cImage> (getPosition () + cPosition (getSize ().x () - 10, 0)));
	signalConnectionManager.connect (readyImage->clicked, [&](){ readyClicked(); });

	colorImage = addChild (std::make_unique<cImage> (getPosition ()));

	updatePlayerColor ();
	updatePlayerReady ();

	nameLabel = addChild (std::make_unique<cLabel> (cBox<cPosition> (getPosition () + cPosition (colorImage->getEndPosition ().x () + 4, 0), cPosition (readyImage->getPosition().x(), readyImage->getEndPosition().y())), player->getName ()));

	fitToChildren ();

	signalConnectionManager.connect (player->nameChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerName, this));
	signalConnectionManager.connect (player->colorChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerColor, this));
	signalConnectionManager.connect (player->readyChanged, std::bind (&cLobbyPlayerListViewItem::updatePlayerReady, this));
}

//------------------------------------------------------------------------------
const std::shared_ptr<cPlayerBasicData>& cLobbyPlayerListViewItem::getPlayer () const
{
	return player;
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerName ()
{
	nameLabel->setText (player->getName ());
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerColor ()
{
	SDL_Rect src = {0, 0, 10, 10};

	AutoSurface colorSurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_BlitSurface (player->getColor ().getTexture(), &src, colorSurface.get (), nullptr);

	colorImage->setImage (colorSurface.get ());
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::updatePlayerReady ()
{
	SDL_Rect src = {player->isReady () ? 10 : 0, 0, 10, 10};

	AutoSurface readySurface (SDL_CreateRGBSurface (0, src.w, src.h, Video.getColDepth (), 0, 0, 0, 0));
	SDL_SetColorKey (readySurface.get (), SDL_TRUE, 0xFF00FF);
	SDL_FillRect (readySurface.get (), NULL, 0xFF00FF);
	SDL_BlitSurface (GraphicsData.gfx_player_ready.get (), &src, readySurface.get (), NULL);

	readyImage->setImage (readySurface.get ());
}

//------------------------------------------------------------------------------
void cLobbyPlayerListViewItem::handleResized (const cPosition& oldSize)
{
	cAbstractListViewItem::handleResized (oldSize);

	if (oldSize.x () == getSize ().x ()) return;

	readyImage->moveTo (getPosition () + cPosition (getSize ().x () - 10, 0));

	nameLabel->resize (cPosition (getSize ().x () - readyImage->getSize ().x () - colorImage->getSize().x() - 4, readyImage->getSize ().y ()));

	fitToChildren ();
}
