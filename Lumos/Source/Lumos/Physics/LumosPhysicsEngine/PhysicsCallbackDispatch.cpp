#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Scene.h"
#include "Core/Thread.h"
#include "Core/DataStructures/Map.h"
#include "Core/DataStructures/Set.h"

#include <sol/sol.hpp>
#include "Scripting/Lua/LuaScriptComponent.h"
#include <entt/entt.hpp>

namespace Lumos
{
    // Order-independent key for a body pair (matches CollisionPairKey::operator==).
    static inline u64 BodyPairHash(const RigidBody3D* a, const RigidBody3D* b)
    {
        uintptr_t pa = (uintptr_t)a, pb = (uintptr_t)b;
        if(pa > pb)
        {
            uintptr_t t = pa;
            pa          = pb;
            pb          = t;
        }
        u64 k = (u64)pa;
        k ^= (u64)pb + 0x9e3779b97f4a7c15ULL + (k << 12) + (k >> 4);
        return k;
    }

    void LumosPhysicsEngine::DispatchCollisionCallbacks(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!scene)
            return;

        auto& registry = scene->GetRegistry();
        auto view      = registry.view<RigidBody3DComponent, LuaScriptComponent>();

        // Nothing scripted -> nothing to dispatch. Avoids touching every event.
        if(view.begin() == view.end())
            return;

        ArenaTemp scratch = ScratchBegin(nullptr, 0);

        HashMap(RigidBody3D*, LuaScriptComponent*) bodyToScript = { 0 };
        bodyToScript.arena                                      = scratch.arena;
        for(auto entity : view)
        {
            RigidBody3D* body = view.get<RigidBody3DComponent>(entity).GetRigidBody();
            if(body)
            {
                LuaScriptComponent* lua = &view.get<LuaScriptComponent>(entity);
                HashMapInsert(&bodyToScript, body, lua);
            }
        }

        // OnCollision3DBegin for this frame's collision events.
        for(auto& evt : m_CollisionEvents)
        {
            if(LuaScriptComponent** a = (LuaScriptComponent**)HashMapFindPtr(&bodyToScript, evt.BodyA))
                (*a)->OnCollision3DBegin(evt.InfoA);
            if(LuaScriptComponent** b = (LuaScriptComponent**)HashMapFindPtr(&bodyToScript, evt.BodyB))
                (*b)->OnCollision3DBegin(evt.InfoB);
        }

        // Set of this frame's pairs for O(1) "still colliding?" tests.
        HashSet(u64) currPairs = { 0 };
        currPairs.arena        = scratch.arena;
        for(auto& cp : m_CurrCollisionPairs)
        {
            u64 key = BodyPairHash(cp.A, cp.B);
            HashSetAdd(&currPairs, key);
        }

        // OnCollision3DEnd for pairs present last frame but gone this frame.
        for(auto& prevPair : m_PrevCollisionPairs)
        {
            u64 prevKey = BodyPairHash(prevPair.A, prevPair.B);
            if(HashSetContains(&currPairs, prevKey))
                continue;

            CollisionInfo3D infoA = { prevPair.B, Vec3(0.0f), Vec3(0.0f), 0.0f, false };
            CollisionInfo3D infoB = { prevPair.A, Vec3(0.0f), Vec3(0.0f), 0.0f, false };

            if(LuaScriptComponent** a = (LuaScriptComponent**)HashMapFindPtr(&bodyToScript, prevPair.A))
                (*a)->OnCollision3DEnd(infoA);
            if(LuaScriptComponent** b = (LuaScriptComponent**)HashMapFindPtr(&bodyToScript, prevPair.B))
                (*b)->OnCollision3DEnd(infoB);
        }

        HashSetDeinit(&currPairs);
        HashMapDeinit(&bodyToScript);
        ScratchEnd(scratch);
    }
}
