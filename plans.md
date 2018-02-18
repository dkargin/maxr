1. Increase map resolution x3
	- Fix compiler errors here and there
	- Fix building process. 
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

Notable problems:

1. void cActionStartBuild::execute(cModel& model) const uses old object size

Broken stuff:
 - `void cVehicle::render_smallClearing`
 - `void cVehicle::render_BuildingOrBigClearing`
 - `void cBuilding::render_rubble`
 - `bool cUnit::isInRange (const cPosition& position) const` does inproper range calculations
 - `bool cUnit::isNextTo (const cPosition& position) const` does inproper adjacency calculation
 - `void cActionInitNewGame::execute(cModel& model) const`
 - `void cActionInitNewGame::makeLanding(cPlayer& player, const sLandingConfig& landingConfig, cModel& model) const` seems not getting proper landing position

# Scaling rework #

Now mostly all the units are multi-tile ones. So we need to rework a lot of interactions. 

Building process should not need the unit to move to the center of building area. Unit could do its building stuff standing near construction site. It will simplify a lot of things for everyone
