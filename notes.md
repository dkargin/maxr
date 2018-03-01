# dkargin's development notes #

## Notable problems I've created ##

 - Rendering connector's attachments to the buildings is broken now. I've cut this part of code from the buildings. Connectors themselves should deal with it, not the buildings
 - Pathfinder ignores cells, that are occupied by another units. Seems like regular units do not block the map properly.
 - Pathfinder tries to cross the angles
 - Rendering pipeline hopes that size of unit's graphics is consistent with actual unit's size. In fact - it is not consistent, and should not.
 - Save game is broken. We have not properly loaded graphics!
 - Finish building job is broken. Markers are drawn in wrong places.
 - Pick building position
 - Building a series of structures, aka 'path building' should be reworked for new grid sizes
 - All exit points are broken
 - All range calculations are broken
 - UI interaction for move orders should be updated for multitile units.

Broken functions. Should be fixed:

 - `void cVehicle::render_smallClearing` - 
 - `void cVehicle::render_BuildingOrBigClearing` - building process is so tricky ...
 - `void cBuilding::render_rubble` - I've just disabled all rubble-related stuff
 - `void cActionInitNewGame::execute(cModel& model) const` - disabled check for credits. Players 
 can overspend
 - ActionFinishBuild - exit points are wrong
 - broken the rendering of building's underlay tiles. This shit should be specified from XML, not the code!
 - void cGameMapWidget::drawExitPoints() generates wrong exit points
 - Unit selection is broken
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