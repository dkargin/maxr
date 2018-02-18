# dkargin's development notes #


My plan so far:

1. Increase map resolution x3. I just want to do it and see how MAX feels with such a change
	- Fix compiler errors here and there 			OK
	- Fix the rendeding of anysize units
		- fix sprite scaling, up to full size
		- fix rendering of the underlay. Maybe we should bring here some sort of tiling generator
	- Fix building process.  
		- fix selection of build position
	- Replace all XML fields
	- Make constructor 2x2
	- Fix pathfinder to deal with increased map size
	- Update unit sizes according to this change
1. Lua AI in screeps style
	- make lua bindings for
		- unit control
		- access to unit's state
		- map access
	- save AI state from lua
	- extend network API to get script updates from the player
1. Get rid of XML and replace it by LUA definitions. LUA is beautifull while XML is ugly
1. Slight rework for radars: separate direct vision and 'radar highlight' vision
1. Fuel storage for all units. That was a brilliant idea from rumaxclub.
1. Rework for heavy missiles:
	Large missile launchers (mobile, ship, building) should launch a special cruise missile unit. This unit should have a limited fuel storage just for one turn of flight. This unit can be shot down by AA. Cruise missiles should be built separately. Missile launchers should be loaded separately. This ammunition is not direct-conversible.

# Notable problems I've created #

 - Rendering connector's attachments to the buildings is broken now. I've cut this part of code from the buildings. Connectors themselves should deal with it, not the buildings
 - Unit's icons are rendered improperly in unit selection dialog. They are scaled too much
 - Pathfinder ignores cells, that are occupied by another units. Seems like regular units do not block the map properly
 - Pathfinder tries to cross the angles
 - Rendering pipeline hopes that size of unit's graphics is consistent with actual unit's size. In fact - it is not consistent, and should not.

Broken functions. Should be fixed:

 - `void cVehicle::render_smallClearing` - 
 - `void cVehicle::render_BuildingOrBigClearing` - building process is so tricky ...
 - `void cBuilding::render_rubble` - I've just disabled it
 - `bool cUnit::isInRange (const cPosition& position) const` does inproper range calculations
 - `bool cUnit::isNextTo (const cPosition& position) const` does inproper adjacency calculation
 - `void cActionInitNewGame::execute(cModel& model) const` - disabled check for credits. Players can overspend
 - broken the rendering of building's underlay tiles. This shit should be specified from XML, not the code!
 std::pair<bool, cPosition> cMouseModeSelectBuildPosition::findNextBuildPosition seems like broken


# Stuff that I do not like #

1. The way XML data is organized. We could just iterate through directories and eat all the files
2. No modder wants to deal with numeric IDs in XML. Just type text IDs and reference them there and there.
3. Data flow in rendering pipeline. Unit should not deal with camera scaling when it tries to render itself.

# Scaling rework #

Now mostly all the units are multi-tile ones. So we need to rework a lot of interactions. 
Building process should not need the unit to move to the center of building area. Unit could do its building stuff standing near construction site. It will simplify a lot of things for everyone


# Rendering pipeline #

void cGameMapWidget::draw (SDL_Surface& destination, const cBox<cPosition>& clipRect)
{
...
	drawBaseUnits();
	drawTopBuildings();
	drawShips();
	drawAboveSeaBaseUnits();
	drawVehicles();
	drawConnectors();
	drawPlanes();
...
}