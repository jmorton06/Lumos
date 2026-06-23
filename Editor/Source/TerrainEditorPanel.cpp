#include "TerrainEditorPanel.h"
#include "Editor.h"

#include <Lumos/Core/Undo.h>
#include <Lumos/Core/Application.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Scene/EntityManager.h>
#include <Lumos/Scene/Component/TerrainComponent.h>
#include <Lumos/Scene/Component/ModelComponent.h>
#include <Lumos/Scene/Component/RigidBody3DComponent.h>
#include <Lumos/Graphics/Terrain.h>
#include <Lumos/Graphics/Model.h>
#include <Lumos/Maths/Transform.h>
#include <Lumos/Maths/Ray.h>
#include <Lumos/Physics/LumosPhysicsEngine/CollisionShapes/TerrainCollisionShape.h>
#include <Lumos/Physics/LumosPhysicsEngine/RigidBody3D.h>
#include <imgui/imgui.h>
#include <stb/stb_perlin.h>
#include <cmath>

namespace Lumos
{
    namespace
    {
        const char* ToolName(TerrainTool t)
        {
            switch(t)
            {
                case TerrainTool::Raise:        return "Raise";
                case TerrainTool::Lower:        return "Lower";
                case TerrainTool::Smooth:       return "Smooth";
                case TerrainTool::Flatten:      return "Flatten";
                case TerrainTool::Noise:        return "Noise";
                case TerrainTool::PaintLayer0:  return "Paint L0";
                case TerrainTool::PaintLayer1:  return "Paint L1";
                case TerrainTool::PaintLayer2:  return "Paint L2";
                case TerrainTool::PaintLayer3:  return "Paint L3";
                default:                        return "None";
            }
        }

        float FalloffWeight(float t, TerrainFalloff f)
        {
            // t = 0 at brush centre, 1 at edge. Returns 1..0.
            t = std::clamp(t, 0.0f, 1.0f);
            float inv = 1.0f - t;
            switch(f)
            {
                case TerrainFalloff::Linear: return inv;
                case TerrainFalloff::Smooth: return inv * inv * (3.0f - 2.0f * inv); // smoothstep
                case TerrainFalloff::Sharp:  return inv * inv;
            }
            return inv;
        }

        // Returns the Terrain mesh + transform world position for an entity, or nullptrs
        // when the entity isn't a sculpt-eligible terrain.
        bool ResolveTerrain(entt::registry& reg, entt::entity e,
                            Terrain*& outMesh, TerrainComponent*& outComp, Vec3& outOrigin)
        {
            outMesh   = nullptr;
            outComp   = nullptr;
            outOrigin = Vec3(0.0f);

            auto tc = reg.try_get<TerrainComponent>(e);
            auto mc = reg.try_get<Graphics::ModelComponent>(e);
            auto tr = reg.try_get<Maths::Transform>(e);
            if(!tc || !mc || !tr) return false;
            if(!mc->ModelRef || mc->ModelRef->GetPrimitiveType() != Graphics::PrimitiveType::Terrain) return false;
            auto& meshes = mc->ModelRef->GetMeshes();
            if(meshes.Size() == 0) return false;

            outMesh   = static_cast<Terrain*>(meshes.Front().get());
            outComp   = tc;
            outOrigin = tr->GetWorldPosition();
            return outMesh && outComp;
        }
    }

    TerrainEditorPanel::TerrainEditorPanel()
    {
        m_Name       = "Terrain Editor###TerrainEditor";
        m_SimpleName = "Terrain";
        m_Active     = false; // off by default — opened from the SceneView toolbar
        SetUndoChangedCallback(&TerrainEditorPanel::RebuildAllAfterUndo, this);
    }

    void TerrainEditorPanel::OnNewScene(Scene* scene)
    {
        m_CurrentScene = scene;
        m_StrokeActive = false;
        m_DirtyEntities.Clear();
        UndoClear();
    }

    void TerrainEditorPanel::SetEnabled(bool enabled)
    {
        m_ToolMode = enabled;
        m_Active   = enabled || m_Active; // also pop the panel open when enabling
    }

    void TerrainEditorPanel::OnImGui()
    {
        if(!m_Active)
            return;

        if(!ImGuiUtilities::BeginPanel(m_Name.c_str(), &m_Active))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Tool Active (T)", &m_ToolMode);
        ImGui::Separator();

        ImGui::TextUnformatted("Tool");
        const TerrainTool tools[] = {
            TerrainTool::Raise, TerrainTool::Lower, TerrainTool::Smooth,
            TerrainTool::Flatten, TerrainTool::Noise
        };
        for(TerrainTool t : tools)
        {
            bool selected = (m_CurrentTool == t);
            if(ImGui::Selectable(ToolName(t), selected))
                m_CurrentTool = t;
        }

        ImGui::Separator();
        ImGui::TextDisabled("Paint Layer (splat phase — disabled)");
        ImGui::BeginDisabled();
        const TerrainTool paintTools[] = {
            TerrainTool::PaintLayer0, TerrainTool::PaintLayer1,
            TerrainTool::PaintLayer2, TerrainTool::PaintLayer3
        };
        for(TerrainTool t : paintTools)
        {
            bool selected = (m_CurrentTool == t);
            ImGui::Selectable(ToolName(t), selected);
        }
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::DragFloat("Radius",   &m_Radius,   0.5f, 0.5f,  500.0f);
        ImGui::DragFloat("Strength", &m_Strength, 0.1f, 0.01f,  50.0f);
        const char* falloffNames[] = { "Linear", "Smooth", "Sharp" };
        int falloff = (int)m_Falloff;
        if(ImGui::Combo("Falloff", &falloff, falloffNames, IM_ARRAYSIZE(falloffNames)))
            m_Falloff = (TerrainFalloff)falloff;

        ImGui::Separator();
        ImGui::TextDisabled("Hotkeys: G Raise · Shift+G Lower · S Smooth · F Flatten · N Noise");
        ImGui::TextDisabled("[ / ] Radius · Shift+[ / ] Strength · T toggle · Ctrl+Z / Ctrl+Shift+Z undo");

        ImGui::End();
    }

    bool TerrainEditorPanel::RaycastTerrain(const Maths::Ray& ray, entt::entity& outEntity, Vec3& outHit) const
    {
        outEntity = entt::null;
        outHit    = Vec3(0.0f);
        if(!m_CurrentScene) return false;

        auto& reg = m_CurrentScene->GetEntityManager()->GetRegistry();
        auto view = reg.view<TerrainComponent, Graphics::ModelComponent, Maths::Transform>();

        float bestT = std::numeric_limits<float>::max();
        bool hit    = false;

        for(auto e : view)
        {
            auto& tc = view.get<TerrainComponent>(e);
            auto& tr = view.get<Maths::Transform>(e);
            if(tc.Heights.Size() != (size_t)(tc.GridW * tc.GridH))
                continue;

            // Transform ray into terrain-local space. Terrain meshes are tile-local
            // (vertex 0 at origin), so we just subtract the transform world position.
            Vec3 origin = tr.GetWorldPosition();
            Vec3 ro     = ray.Origin - origin;
            Vec3 rd     = ray.Direction;

            // Walk in fixed steps along the ray within the terrain's XZ AABB
            // (cheap; precision is plenty for brush placement). Bisect when we
            // cross the surface.
            const float tMax   = 5000.0f;
            const float stepLen = std::max(0.5f, tc.ScaleXZ * 0.5f);

            auto Sample = [&](float x, float z) -> float
            {
                float gx = x / std::max(tc.ScaleXZ, 0.0001f);
                float gz = z / std::max(tc.ScaleXZ, 0.0001f);
                if(gx < 0 || gz < 0 || gx > (tc.GridW - 1) || gz > (tc.GridH - 1))
                    return -std::numeric_limits<float>::max();
                int x0 = (int)gx; int z0 = (int)gz;
                int x1 = std::min(x0 + 1, tc.GridW - 1);
                int z1 = std::min(z0 + 1, tc.GridH - 1);
                float fx = gx - x0; float fz = gz - z0;
                float h00 = tc.Heights[x0 * tc.GridW + z0];
                float h10 = tc.Heights[x1 * tc.GridW + z0];
                float h01 = tc.Heights[x0 * tc.GridW + z1];
                float h11 = tc.Heights[x1 * tc.GridW + z1];
                float h0  = h00 * (1.0f - fx) + h10 * fx;
                float h1  = h01 * (1.0f - fx) + h11 * fx;
                return (h0 * (1.0f - fz) + h1 * fz) * tc.ScaleY;
            };

            float prevT = 0.0f;
            float prevDiff = ro.y - Sample(ro.x, ro.z);
            for(float t = stepLen; t < tMax; t += stepLen)
            {
                Vec3 p = ro + rd * t;
                float surf = Sample(p.x, p.z);
                if(surf == -std::numeric_limits<float>::max())
                {
                    prevT = t; prevDiff = 1e30f; continue;
                }
                float diff = p.y - surf;
                if(prevDiff > 0 && diff <= 0)
                {
                    // crossed surface — refine via lerp
                    float k = prevDiff / (prevDiff - diff);
                    float hitT = prevT + (t - prevT) * k;
                    if(hitT < bestT)
                    {
                        bestT     = hitT;
                        outEntity = e;
                        outHit    = origin + (ro + rd * hitT);
                        hit       = true;
                    }
                    break;
                }
                prevT = t; prevDiff = diff;
            }
        }
        return hit;
    }

    void TerrainEditorPanel::BeginStroke()
    {
        m_StrokeActive   = true;
        m_StrokeFirstDab = true;
        m_StrokeEntity   = entt::null;
        m_StrokePushed.Clear();
    }

    void TerrainEditorPanel::EndStroke()
    {
        if(!m_StrokeActive)
            return;
        m_StrokeActive   = false;
        m_StrokeFirstDab = true;
        m_StrokeEntity   = entt::null;
        m_StrokePushed.Clear();
        UndoCommit();
    }

    bool TerrainEditorPanel::ApplyBrushAt(const Vec3& worldHit, entt::entity terrainEntity, float dt)
    {
        if(!m_CurrentScene || terrainEntity == entt::null)
            return false;

        auto& reg = m_CurrentScene->GetEntityManager()->GetRegistry();
        Terrain* mesh = nullptr;
        TerrainComponent* tc = nullptr;
        Vec3 origin;
        if(!ResolveTerrain(reg, terrainEntity, mesh, tc, origin))
            return false;

        // Capture flatten reference height on first dab of the stroke.
        if(m_CurrentTool == TerrainTool::Flatten && m_StrokeFirstDab)
            m_FlattenHeight = (worldHit.y - origin.y) / std::max(tc->ScaleY, 0.0001f);
        m_StrokeFirstDab = false;

        // World → terrain-local XZ.
        const float lx = worldHit.x - origin.x;
        const float lz = worldHit.z - origin.z;
        const float radius  = m_Radius;
        const float radius2 = radius * radius;

        const int gw = tc->GridW;
        const int gh = tc->GridH;
        const float sxz = std::max(tc->ScaleXZ, 0.0001f);

        // Vertex window (clamped) covering the brush footprint.
        const int gxMin = std::max(0,       (int)std::floor((lx - radius) / sxz));
        const int gxMax = std::min(gw - 1,  (int)std::ceil ((lx + radius) / sxz));
        const int gzMin = std::max(0,       (int)std::floor((lz - radius) / sxz));
        const int gzMax = std::min(gh - 1,  (int)std::ceil ((lz + radius) / sxz));
        if(gxMax < gxMin || gzMax < gzMin)
            return false;

        // Undo snapshot: push each touched cell only on its FIRST dab of the
        // stroke. Without this we hit MAX_UNDOS (65k entries) in a second of
        // drag-painting. We batch per row so each UndoPush covers a contiguous
        // span of floats — far cheaper than one push per vertex.
        if(m_StrokeEntity != terrainEntity)
        {
            m_StrokeEntity = terrainEntity;
            m_StrokePushed.Clear();
            m_StrokePushed.Resize((size_t)(gw * gh), uint8_t(0));
        }

        for(int x = gxMin; x <= gxMax; ++x)
        {
            // Walk the row, push contiguous unpushed spans in one UndoPush call.
            int spanStart = -1;
            for(int z = gzMin; z <= gzMax + 1; ++z)
            {
                bool isUnpushed = (z <= gzMax) && (m_StrokePushed[x * gw + z] == 0);
                if(isUnpushed)
                {
                    if(spanStart < 0) spanStart = z;
                }
                else if(spanStart >= 0)
                {
                    const int spanLen = z - spanStart;
                    UndoPush(&tc->Heights[x * gw + spanStart], (i64)(spanLen * sizeof(float)));
                    for(int zz = spanStart; zz < z; ++zz)
                        m_StrokePushed[x * gw + zz] = 1;
                    spanStart = -1;
                }
            }
        }

        for(int x = gxMin; x <= gxMax; ++x)
        {
            const float wx = x * sxz;
            for(int z = gzMin; z <= gzMax; ++z)
            {
                const float wz = z * sxz;
                const float dx = wx - lx;
                const float dz = wz - lz;
                const float d2 = dx * dx + dz * dz;
                if(d2 > radius2) continue;

                const float t = std::sqrt(d2) / std::max(radius, 0.0001f);
                const float w = FalloffWeight(t, m_Falloff);
                float& h = tc->Heights[x * gw + z];

                switch(m_CurrentTool)
                {
                    case TerrainTool::Raise:   h += m_Strength * w * dt; break;
                    case TerrainTool::Lower:   h -= m_Strength * w * dt; break;
                    case TerrainTool::Smooth:
                    {
                        // 3x3 average (clamped on edges).
                        float sum = 0.0f; int n = 0;
                        for(int ox = -1; ox <= 1; ++ox)
                            for(int oz = -1; oz <= 1; ++oz)
                            {
                                int nx = std::clamp(x + ox, 0, gw - 1);
                                int nz = std::clamp(z + oz, 0, gh - 1);
                                sum += tc->Heights[nx * gw + nz];
                                ++n;
                            }
                        const float avg = sum / float(n);
                        h += (avg - h) * std::min(1.0f, m_Strength * w * dt);
                        break;
                    }
                    case TerrainTool::Flatten:
                    {
                        h += (m_FlattenHeight - h) * std::min(1.0f, m_Strength * w * dt);
                        break;
                    }
                    case TerrainTool::Noise:
                    {
                        const float n = stb_perlin_noise3(
                            (wx + tc->TileOriginX * sxz) * 0.05f,
                            (wz + tc->TileOriginZ * sxz) * 0.05f,
                            0.0f, 0, 0, 0);
                        h += n * m_Strength * w * dt;
                        break;
                    }
                    default: break;
                }
            }
        }

        MarkDirty(terrainEntity);
        return true;
    }

    void TerrainEditorPanel::MarkDirty(entt::entity e)
    {
        for(auto cached : m_DirtyEntities)
            if(cached == e) return;
        m_DirtyEntities.PushBack(e);
    }

    void TerrainEditorPanel::RebuildTerrain(entt::entity e)
    {
        if(!m_CurrentScene) return;
        auto& reg = m_CurrentScene->GetEntityManager()->GetRegistry();
        Terrain* mesh = nullptr;
        TerrainComponent* tc = nullptr;
        Vec3 origin;
        if(!ResolveTerrain(reg, e, mesh, tc, origin))
            return;

        mesh->Rebuild(tc->Heights.Data());

        if(auto rb = reg.try_get<RigidBody3DComponent>(e))
        {
            if(auto* body = rb->GetRigidBody())
            {
                if(auto shape = body->GetCollisionShape())
                {
                    if(shape->GetType() == CollisionTerrain)
                        static_cast<TerrainCollisionShape*>(shape.get())->UpdateHeights(tc->Heights.Data());
                }
            }
        }

        tc->HasCustomEdits = true;
        tc->Dirty          = false;
    }

    void TerrainEditorPanel::SampleBrushRing(const Vec3& centre, int ringCount, Vec3* outRing) const
    {
        if(!m_CurrentScene || !outRing || ringCount <= 0) return;
        auto& reg = m_CurrentScene->GetEntityManager()->GetRegistry();
        auto view = reg.view<TerrainComponent, Maths::Transform>();
        // Pick the terrain whose AABB contains the brush centre (XZ).
        TerrainComponent* picked = nullptr;
        Vec3 origin(0.0f);
        for(auto e : view)
        {
            auto& tc = view.get<TerrainComponent>(e);
            auto& tr = view.get<Maths::Transform>(e);
            Vec3 o   = tr.GetWorldPosition();
            float maxX = o.x + (tc.GridW - 1) * tc.ScaleXZ;
            float maxZ = o.z + (tc.GridH - 1) * tc.ScaleXZ;
            if(centre.x >= o.x && centre.x <= maxX && centre.z >= o.z && centre.z <= maxZ)
            {
                picked = &tc;
                origin = o;
                break;
            }
        }

        auto SampleHeight = [&](float wx, float wz) -> float
        {
            if(!picked) return centre.y;
            float gx = (wx - origin.x) / std::max(picked->ScaleXZ, 0.0001f);
            float gz = (wz - origin.z) / std::max(picked->ScaleXZ, 0.0001f);
            gx = std::clamp(gx, 0.0f, (float)(picked->GridW - 1));
            gz = std::clamp(gz, 0.0f, (float)(picked->GridH - 1));
            int x0 = (int)gx; int z0 = (int)gz;
            int x1 = std::min(x0 + 1, picked->GridW - 1);
            int z1 = std::min(z0 + 1, picked->GridH - 1);
            float fx = gx - x0; float fz = gz - z0;
            float h00 = picked->Heights[x0 * picked->GridW + z0];
            float h10 = picked->Heights[x1 * picked->GridW + z0];
            float h01 = picked->Heights[x0 * picked->GridW + z1];
            float h11 = picked->Heights[x1 * picked->GridW + z1];
            float h0  = h00 * (1.0f - fx) + h10 * fx;
            float h1  = h01 * (1.0f - fx) + h11 * fx;
            return origin.y + (h0 * (1.0f - fz) + h1 * fz) * picked->ScaleY;
        };

        for(int i = 0; i < ringCount; ++i)
        {
            float a = (float)i / (float)ringCount * 2.0f * 3.14159265358979f;
            float wx = centre.x + std::cos(a) * m_Radius;
            float wz = centre.z + std::sin(a) * m_Radius;
            outRing[i] = Vec3(wx, SampleHeight(wx, wz) + 0.05f, wz);
        }
    }

    void TerrainEditorPanel::RebuildAllAfterUndo(void* userdata)
    {
        auto* self = static_cast<TerrainEditorPanel*>(userdata);
        if(!self || !self->m_CurrentScene) return;
        // After an undo memory-swap, every terrain entity's heights may have
        // changed in place — refresh all of them.
        auto& reg = self->m_CurrentScene->GetEntityManager()->GetRegistry();
        auto view = reg.view<TerrainComponent>();
        for(auto e : view)
            self->RebuildTerrain(e);
    }

    void TerrainEditorPanel::FlushDirty()
    {
        for(auto e : m_DirtyEntities)
            RebuildTerrain(e);
        m_DirtyEntities.Clear();
    }
}
