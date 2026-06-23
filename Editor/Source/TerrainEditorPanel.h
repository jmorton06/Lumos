#pragma once

#include "EditorPanel.h"
#include <Lumos/Maths/MathsFwd.h>
#include <Lumos/Maths/Vector3.h>
#include <Lumos/Maths/Ray.h>
#include <Lumos/Core/DataStructures/TDArray.h>
#include <entt/entt.hpp>

namespace Lumos
{
    class Terrain;
    struct TerrainComponent;
    class TerrainCollisionShape;

    enum class TerrainTool
    {
        None,
        Raise,
        Lower,
        Smooth,
        Flatten,
        Noise,
        // Reserved for the future splat-paint phase
        PaintLayer0,
        PaintLayer1,
        PaintLayer2,
        PaintLayer3,
        Count
    };

    enum class TerrainFalloff
    {
        Linear,
        Smooth,
        Sharp
    };

    // Modal sculpt/paint tool. Lives next to the existing inspector panels; activated
    // from the SceneView toolbar's Terrain toggle. Owns brush state + drives the
    // per-stroke heightmap edits through the Core/Undo system.
    class TerrainEditorPanel : public EditorPanel
    {
    public:
        TerrainEditorPanel();
        ~TerrainEditorPanel() override = default;

        void OnImGui() override;
        void OnNewScene(Scene* scene) override;

        // External hooks driven by SceneViewPanel.
        bool IsActive() const { return m_ToolMode && m_CurrentTool != TerrainTool::None; }
        void SetEnabled(bool enabled);

        // Apply one brush dab at a world-space hit. Returns true if any terrain was
        // affected (so SceneView knows to consume the input).
        bool ApplyBrushAt(const Vec3& worldHit, entt::entity terrainEntity, float dt);

        // Pick which terrain (if any) the ray hits. Returns the entity + hit point.
        bool RaycastTerrain(const Maths::Ray& ray, entt::entity& outEntity, Vec3& outHit) const;

        // Stroke lifecycle — bracket each LMB drag.
        void BeginStroke();
        void EndStroke();

        // Called once per frame by SceneViewPanel: rebuilds meshes + collision
        // for every entity touched this frame, then clears the dirty list.
        void FlushDirty();

        // Hooked into Core/Undo so we rebuild meshes / collision after a memory swap.
        static void RebuildAllAfterUndo(void* userdata);

        // Brush params (exposed so keyboard handlers in SceneView can nudge them).
        float& BrushRadius()   { return m_Radius; }
        float& BrushStrength() { return m_Strength; }

        TerrainTool CurrentTool() const { return m_CurrentTool; }
        void SetCurrentTool(TerrainTool t) { m_CurrentTool = t; }

        bool& ToolModeRef() { return m_ToolMode; }

        // Sample radius preview ring in world space — for SceneView overlay.
        // outRing must have room for ringCount points.
        void SampleBrushRing(const Vec3& centre, int ringCount, Vec3* outRing) const;

    private:
        void RebuildTerrain(entt::entity e);
        void MarkDirty(entt::entity e);

        Scene* m_CurrentScene = nullptr;
        bool m_ToolMode       = false;       // master toggle (T key / toolbar)
        TerrainTool m_CurrentTool = TerrainTool::Raise;
        TerrainFalloff m_Falloff  = TerrainFalloff::Smooth;

        float m_Radius      = 20.0f;
        float m_Strength    = 4.0f;
        float m_FlattenHeight  = 0.0f;       // captured on first dab of a Flatten stroke
        bool m_StrokeActive    = false;
        bool m_StrokeFirstDab  = true;

        // Per-stroke vertex tracker. Marks which cells have already been pushed
        // into the undo system so we never UndoPush the same vertex twice in a
        // stroke (without this we burn through MAX_UNDOS in a second of dragging).
        // Sized to GridW*GridH of the stroke's terrain; reset between strokes.
        TDArray<uint8_t> m_StrokePushed;
        entt::entity     m_StrokeEntity = entt::null;

        // Entities touched in the current frame — coalesce one mesh + collision
        // rebuild per terrain per frame instead of per-mouse-move.
        TDArray<entt::entity> m_DirtyEntities;
    };
}
