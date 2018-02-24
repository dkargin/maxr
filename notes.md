# dkargin's development notes #


My plan so far:

1. Increase map resolution x3. I just want to do it and see how MAX feels with such a change
	- Fix the rendeding of anysize units
		- fix sprite scaling, up to full size 		OK
		- fix rendering of the underlay. Maybe we should bring here some sort of tiling generator
	- Fix building process.  
		- fix selection of build position. Need a proper sprite
	- Replace all XML fields 						OK
	- Make constructor 2x2
	- Fix pathfinder to deal with increased map size
		- fix collision checking
		- fix unit projection
	- Update unit sizes according to this change 	OK
	- Update loading process to support multiple 'data' folders. Yeah, that means proper modding support.
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
 - `void cActionInitNewGame::execute(cModel& model) const` - disabled check for credits. Players 
 can overspend
 - ActionFinishBuild
 - broken the rendering of building's underlay tiles. This shit should be specified from XML, not the code!
 std::pair<bool, cPosition> cMouseModeSelectBuildPosition::findNextBuildPosition seems like broken


# Stuff that I do not like #

1. The way XML data is organized. We could just iterate through directories and eat all the files
2. No modder wants to deal with numeric IDs in XML. Just type text IDs and reference them there and there.
3. Data flow in rendering pipeline. Unit should not deal with camera scaling when it tries to render itself.

# Scaling rework #

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

# Fiddling with cropped sprites #
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

// Shared pointer to SDL surface
SurfacePtr surface;
// Area of source surface. We copy data from that area
SDL_Rect srcRect;
// Desired size. In 'world' tile coordinates. That's static data from XML
cVector2 size;

We could recalculate size, according to refSize and srcRect:
```
size.x() *= float(refSize.x()) / float(srcRect.x())
size.y() *= float(refSize.y()) / float(srcRect.y())
```


TODO right now:

1. Fix image for mine
	Add xml tag for selectable sprite sheet
2. Fix oversized and undersized images (ifder FIX_SHADOW)
	Add xml tag

// Wrapper for drawing sprites from world coordinates
struct cSprite: public cRenderable
{
    // Shared pointer to SDL surface
    SurfacePtr surface;
    // Area of source surface. We copy data from that area
    SDL_Rect srcRect;
    // Rendering mode. That's static data from XML
    FitMode mode;
public:
    cSprite();

    virtual cBox<cVector2> getRect() const override;
    // @param scale - multiplier between world coordinates and screen coordinates
    // usually it is like tileSize*zoom
    virtual void updateScale(float scale) override;

    operator SDL_Surface*();

    void setColorKey(int key);
    void setAlphaKey(int alpha = -1);
    // Blit this sprite to output surface
    // Will do rescaling, if necessary
    void blit_and_cache(SDL_Surface* surface, SDL_Rect rect) override;
protected:
    // Cached surface. Will be updated at 'updateScale' invocation
    SurfaceUPtr cache;
    // Last used scale
    cVector2 lastSize;
};

hasFlag(UnitFlag::C

#ifdef FIX_THIS
#ifdef OVERLAYS_TO_BE_FIXED
#ifdef FIX_BUILD_ANIMATION
#ifdef FIX_MINE_LAYER
#ifdef FIX_BUILDING_UNDERLAY