#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Scene.h"

#include <sol/sol.hpp>
#include "Scripting/Lua/LuaScriptComponent.h"
#include <entt/entt.hpp>

namespace Lumos
{
    void LumosPhysicsEngine::DispatchCollisionCallbacks(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!scene)
            return;

        auto& registry = scene->GetRegistry();
        auto view = registry.view<RigidBody3DComponent, LuaScriptComponent>();

        // Dispatch OnCollision3DBegin for current collision events
        for(auto& evt : m_CollisionEvents)
        {
            for(auto entity : view)
            {
                auto& rbComp = view.get<RigidBody3DComponent>(entity);
                auto& luaComp = view.get<LuaScriptComponent>(entity);

                if(rbComp.GetRigidBody() == evt.BodyA)
                    luaComp.OnCollision3DBegin(evt.InfoA);
                else if(rbComp.GetRigidBody() == evt.BodyB)
                    luaComp.OnCollision3DBegin(evt.InfoB);
            }
        }

        // Dispatch OnCollision3DEnd for pairs that were in prev frame but not current
        for(auto& prevPair : m_PrevCollisionPairs)
        {
            bool found = false;
            for(auto& currPair : m_CurrCollisionPairs)
            {
                if(prevPair == currPair)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                CollisionInfo3D infoA = { prevPair.B, Vec3(0.0f), Vec3(0.0f), 0.0f, false };
                CollisionInfo3D infoB = { prevPair.A, Vec3(0.0f), Vec3(0.0f), 0.0f, false };

                for(auto entity : view)
                {
                    auto& rbComp = view.get<RigidBody3DComponent>(entity);
                    auto& luaComp = view.get<LuaScriptComponent>(entity);

                    if(rbComp.GetRigidBody() == prevPair.A)
                        luaComp.OnCollision3DEnd(infoA);
                    else if(rbComp.GetRigidBody() == prevPair.B)
                        luaComp.OnCollision3DEnd(infoB);
                }
            }
        }
    }
}
