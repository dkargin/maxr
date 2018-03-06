# dkargin's development notes #

## Notable problems I've created ##

 - Rendering connector's attachments to the buildings is broken now. I've cut this part of code from the buildings. Connectors themselves should deal with it, not the buildings
 - Rendering pipeline hopes that size of unit's graphics is consistent with actual unit's size. In fact - it is not consistent, and should not.
 - Finish building job is broken:
 	- Markers are drawn in wrong places.
 	- Exit build area is broken
 - Pick building position
 - Building a series of structures, aka 'path building' should be reworked for new grid sizes
 - All exit points are broken
 - All range calculations are broken
 - UI interaction for move orders should be updated for multitile units.
 - Shadows are cropped. Should provide a larger area to render shadows
 - Render order for shadows is broken. All shadows should be rendered at once

Broken functions. Should be fixed:

 - `void cVehicle::render_smallClearing` - 
 - `void cVehicle::render_BuildingOrBigClearing` - building process is so tricky ...
 - `void cBuilding::render_rubble` - I've just disabled all rubble-related stuff
 - `void cActionInitNewGame::execute(cModel& model) const` - disabled check for credits. Players 
 can overspend
 - ActionFinishBuild - exit points are wrong
 - void cGameMapWidget::drawExitPoints() is not consistent with actual exit points. Rendering code should be coupled with actual logic code
 - Unit selection is broken. 
 std::pair<bool, cPosition> cMouseModeSelectBuildPosition::findNextBuildPosition seems like broken


## Stuff that I do not like ##

1. The way XML data is organized. We could just iterate through directories and eat all the files. Fixed mostly. There is directory index, like vehicles.xml and buildings.xml, but it can be removed later
2. No modder wants to deal with numeric IDs in XML. Just type text IDs and reference them there and there.
3. Data flow in rendering pipeline. Unit should not deal with camera scaling when it tries to render itself. 

## Scaling rework ##

Now mostly all the units are multi-tile ones. So we need to rework a lot of interactions. 
Building process should not need the unit to move to the center of building area. Unit could do its building stuff standing near construction site. It will simplify a lot of things for everyone


<TileGen name="connectors" file="connectors.pcx" size="64x64">
	<!--Central connector not connected anywhere-->
	<Rule pos="0x0">
		000
		010
		000
	</Rule>
	<!--Use this tile if there is a connection from the upper side-->
	<Rule pos="1x0">
		010
		010
		000
	</Rule>
</TileGen>

## Fiddling with cropped sprites ##

Mine:
	Shadow image 170x140
	Two tiles = 128x128
	Base size =128x128

Small energy:
	Image=64x64
	Effect=50x49
	Shadow=90x90

Need to rescale local rect accordingly

So we have: 
vec2i refsize - reference size to determine intended size of the unit. Some sprites are larger than default tile, and some of them are smaller. We should track this stuff
Rect srcSize - area from the source image
RectF relativeArea - where do we draw this sprite, in unit's coordinates

relativeArea and refsize replace each other

We could recalculate size, according to refSize and srcRect:

```
size.x() *= float(refSize.x()) / float(srcRect.x())
size.y() *= float(refSize.y()) / float(srcRect.y())
```

#ifdef FIX_THIS
#ifdef OVERLAYS_TO_BE_FIXED
#ifdef FIX_BUILD_ANIMATION
#ifdef FIX_MINE_LAYER
#ifdef FIX_BUILDING_UNDERLAY

Tag "Graphic" has block "Animations" with: "Build_Up", "Power_On"

Graphic layers:
 - shadow
 - underlay - beton and so on
 - main - main graphics
 - effect - is drawn over main (or under?)
 - overlay - is drawn over the rest modules

Channels:
 - animation 	- selected animation frame
 - direction 	- where unit is facing
 - clan			- selected clan


Render order:
1. Player color to the cache
2. Object's sprite. Set colormask = rgb{255,255,255}, dest=cache
3. Draw object to main surface. Dest = screen, colorkey = rgb{255,0,255}

Animations are generated from snprintf(sztmp, sizeof (sztmp), "img%d_%.2d.pcx", dir, frame);

Local plan:

1. Exit points are so borken. Fixed a bit, but still broken
2. Builder's graphics are broken. Need to restore it using new codebase.
	- When iterating visible map for a units: should check builders, cause they can occupy much more space.
	- Draw sprites where necessary
3. Faction colors are broken
4. Generator effects are broken
5. Shadows are not so sharp. 											DONE
6. Shadows are cropped. Every shadow image is larger than actual tile sizes
7. Missing beton underlay for most of buildings. Fix XMLs for it 		DONE
8. Shadows mess with ground layer
9. Fix sprite loading for rubble/wrecs/...

if (cellSize > 1)
{
	CHECK_SCALING (*UnitsUiData.rubbleBig->shw, *UnitsUiData.rubbleBig->shw_org, zoomFactor);
	SDL_BlitSurface (UnitsUiData.rubbleBig->shw.get(), &src, surface, &tmp);
}
else
{
	CHECK_SCALING (*UnitsUiData.rubbleSmall->shw, *UnitsUiData.rubbleSmall->shw_org, zoomFactor);
	SDL_BlitSurface (UnitsUiData.rubbleSmall->shw.get(), &src, surface, &tmp);
}

AutoSurface s (SDL_CreateRGBSurface (0, x, y, 32, 0, 0, 0, 0));

For effects:

SDL_SetSurfaceAlphaMod (ui.eff.get(), 10);

For building images:
			ui.img_org = LoadPCX (sTmpString);
			ui.img = CloneSDLSurface (*ui.img_org);
			SDL_SetColorKey (ui.img_org.get(), SDL_TRUE, 0xFFFFFF);
			SDL_SetColorKey (ui.img.get(), SDL_TRUE, 0xFFFFFF);

Restoring vanilla unit parameters

Restored vanilla unit parameters.
Moved overriden unit parameters to mods/max_rescaled folder.

Added proper cmake config for SDL2_image library.
