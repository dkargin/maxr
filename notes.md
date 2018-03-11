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


Notable defines to be fixed:

```
#ifdef FIX_THIS
#ifdef FUCK_THIS
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


Local plan:


The problems stopping from pull request:

1. Restore build/clear graphics to original look
	- Fix wrong scaling and sprite placement for large buildings
	buildSize = building->getCellSize();
1. Reimplement rubble graphics
	- Rubble is not created
	- Load rubble graphics to a common location
	- Draw proper rubble
	- Test it a bit
1. Reimplement walking animation, using sprite groups
	- implement cRenderableGroup class. We need a container for different sprites
	- need iterative XML parser as well
1. Air transport can not load units