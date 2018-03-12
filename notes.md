# dkargin's development notes #

## Notable problems I've created ##

 - Finish building job is broken:
 	- Markers are drawn in wrong places.
 	- Exit build area is broken
 - Pick building position
 - Building a series of structures, aka 'path building' should be reworked for new grid sizes
 - All exit points are broken
 - All range calculations are broken
 - UI interaction for move orders should be updated for multitile units.
 
Broken functions. Should be fixed:

 - `void cBuilding::render_rubble` - I've just disabled all rubble-related stuff
 - `void cActionInitNewGame::execute(cModel& model) const` - disabled check for credits. Players 
 can overspend
 - ActionFinishBuild - exit points are wrong
 - void cGameMapWidget::drawExitPoints() is not consistent with actual exit points. Rendering code should be coupled with actual logic code
 std::pair<bool, cPosition> cMouseModeSelectBuildPosition::findNextBuildPosition seems like broken for a very large buildings. 1x1 and 2x2 buildings are ok

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

1. Sometimes build graphics are glitchy
1. Reimplement rubble graphics
	- Rubble is not created
	- Load rubble graphics to a common location
	- Draw proper rubble
	- Test it a bit
1. Reimplement walking animation, using sprite groups
	- implement cRenderableGroup class. We need a container for different sprites
	- need iterative XML parser as well
1. Air transport can not load units
1. Restore cache for vehicle sprites. Hell knows when I reimplement rendering with opengl
1. Actual connector building does not pick proper graphics