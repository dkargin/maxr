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


```
#ifdef FIX_THIS
#ifdef OVERLAYS_TO_BE_FIXED
#ifdef FIX_BUILD_ANIMATION
#ifdef FIX_MINE_LAYER
#ifdef FIX_BUILDING_UNDERLAY
```

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

Connector

 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15|
   | | |   |   |   | | |   | | | | |   |   | | | | | | |   | | |
 o | o | o-| o |-o | o |-o-|-o | o-| o-|-o |-o |-o-| o-|-o-|-o-|
   |   |   | | |   | | |   |   |   | | | | | | |   | | | | | | |
Local plan:

1. Exit points are so borken. Fixed a bit, but still broken
1. Builder's graphics are broken. Need to restore it using new codebase.
	- When iterating visible map for a units: should check builders, cause they can occupy much more space.
	- Draw sprites where necessary
	- Fix sprite position and scaling
1. Fix sprite loading for rubble/wrecs/...

The problems stopping from pull request:

1. Reimplement rubble graphics
1. Restore build/clear graphics to original look
	- Fix wrong scaling
1. Reimplement connector graphics
1. Reimplement walking animation, using sprite groups
	- need iterative XML parser as well
1. 