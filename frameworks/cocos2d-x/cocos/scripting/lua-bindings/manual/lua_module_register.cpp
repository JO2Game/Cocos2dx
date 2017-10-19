#include "scripting/lua-bindings/manual/lua_module_register.h"

#include "scripting/lua-bindings/manual/cocosdenshion/lua_cocos2dx_cocosdenshion_manual.h"
#include "scripting/lua-bindings/manual/network/lua_cocos2dx_network_manual.h"
#include "scripting/lua-bindings/manual/cocosbuilder/lua_cocos2dx_cocosbuilder_manual.h"
#include "scripting/lua-bindings/manual/cocostudio/lua_cocos2dx_coco_studio_manual.hpp"
#include "scripting/lua-bindings/manual/extension/lua_cocos2dx_extension_manual.h"
#include "scripting/lua-bindings/manual/ui/lua_cocos2dx_ui_manual.hpp"
#include "scripting/lua-bindings/manual/spine/lua_cocos2dx_spine_manual.hpp"
#include "scripting/lua-bindings/manual/3d/lua_cocos2dx_3d_manual.h"
#include "scripting/lua-bindings/manual/audioengine/lua_cocos2dx_audioengine_manual.h"
#include "scripting/lua-bindings/manual/physics3d/lua_cocos2dx_physics3d_manual.h"
#include "scripting/lua-bindings/manual/navmesh/lua_cocos2dx_navmesh_manual.h"

#ifndef CC_USE_CCB
#define CC_USE_CCB 1
#endif

#ifndef CC_USE_SPINE
#define CC_USE_SPINE 1
#endif

#ifndef CC_USE_3D
#define CC_USE_3D 1
#endif

int lua_module_register(lua_State* L)
{
    //Dont' change the module register order unless you know what your are doing
    register_cocosdenshion_module(L);
    register_network_module(L);
#if CC_USE_CCB
    register_cocosbuilder_module(L);
#endif
    register_cocostudio_module(L);
    register_ui_moudle(L);
    register_extension_module(L);
#if CC_USE_SPINE
    register_spine_module(L);
#endif
#if CC_USE_3D
    register_cocos3d_module(L);
#endif
    register_audioengine_module(L);

#if CC_USE_3D_PHYSICS && CC_ENABLE_BULLET_INTEGRATION
    register_physics3d_module(L);
#endif
#if CC_USE_NAVMESH
    register_navmesh_module(L);
#endif
    return 1;
}

