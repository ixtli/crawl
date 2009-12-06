/*
 * File:     l_featfeat.cc
 * Summary:  Boolean feat-related functions lua library "feat".
 */

#include "AppHdr.h"

#include "clua.h"
#include "cluautil.h"
#include "l_libs.h"

#include "coord.h"
#include "dungeon.h"
#include "env.h"
#include "terrain.h"

#define FEATF(name, val) \
    static int name(lua_State *ls) \
    { \
        if (lua_gettop(ls) == 2) \
        { \
            COORDS(c, 1, 2);\
            lua_pushboolean(ls, val(grd(c)));\
        } \
        else if (lua_isnumber(ls, 1)) \
             lua_pushboolean(ls, val(static_cast<dungeon_feature_type>( \
                                                luaL_checkint(ls, 1)))); \
        else if (lua_isstring(ls, 1)) \
            lua_pushboolean(ls, val(dungeon_feature_by_name(\
                                                luaL_checkstring(ls, 1))));\
        return (1); \
    }

FEATF(_feat_destroys_items, feat_destroys_items)

FEATF(_feat_is_wall, feat_is_wall)
FEATF(_feat_is_solid, feat_is_solid)
FEATF(_feat_is_opaque, feat_is_opaque)
FEATF(_feat_is_closed_door, feat_is_closed_door)
FEATF(_feat_is_secret_door, feat_is_secret_door)
FEATF(_feat_is_statue_or_idol, feat_is_statue_or_idol)
FEATF(_feat_is_rock, feat_is_rock)
FEATF(_feat_is_permarock, feat_is_permarock)
FEATF(_feat_is_stone_stair, feat_is_stone_stair)
FEATF(_feat_is_staircase, feat_is_staircase)
FEATF(_feat_is_escape_hatch, feat_is_escape_hatch)
FEATF(_feat_is_trap, feat_is_trap)
FEATF(_feat_is_sealable_portal, feat_sealable_portal)
FEATF(_feat_is_portal, feat_is_portal)
FEATF(_feat_is_stair, feat_is_stair)
FEATF(_feat_is_travelable_stair, feat_is_travelable_stair)
FEATF(_feat_is_gate, feat_is_gate)
FEATF(_feat_is_water, feat_is_water)
FEATF(_feat_is_watery, feat_is_watery)
FEATF(_feat_is_altar, feat_is_altar)
FEATF(_feat_is_player_altar, feat_is_player_altar)
FEATF(_feat_is_branch_stairs, feat_is_branch_stairs)
FEATF(_feat_is_critical, is_critical_feature)

const struct luaL_reg feat_dlib[] =
{
{ "destroys_items", _feat_destroys_items },
{ "is_wall", _feat_is_wall },
{ "is_solid", _feat_is_solid },
{ "is_opaque", _feat_is_opaque },
{ "is_closed_door", _feat_is_closed_door },
{ "is_secret_door", _feat_is_secret_door },
{ "is_statue_or_idol", _feat_is_statue_or_idol },
{ "is_rock", _feat_is_rock },
{ "is_permarock", _feat_is_permarock },
{ "is_stone_stair", _feat_is_stone_stair },
{ "is_staircase", _feat_is_staircase },
{ "is_escape_hatch", _feat_is_escape_hatch },
{ "is_trap", _feat_is_trap },
{ "is_sealable_portal", _feat_is_sealable_portal },
{ "is_portal", _feat_is_portal },
{ "is_stair", _feat_is_stair },
{ "is_travelable_stair", _feat_is_travelable_stair },
{ "is_gate", _feat_is_gate },
{ "is_water", _feat_is_water },
{ "is_watery", _feat_is_watery },
{ "is_altar", _feat_is_altar },
{ "is_player_altar", _feat_is_player_altar },
{ "is_branch_stairs", _feat_is_branch_stairs },
{ "is_critical", _feat_is_critical },

{ NULL, NULL }
};