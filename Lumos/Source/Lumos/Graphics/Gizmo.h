#pragma once
#include "Core/Core.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"
#include <entt/fwd.hpp>

namespace Lumos
{
    class Scene;
    class Camera;
    namespace Maths
    {
        class Transform;
    }

    enum GizmoOperation : u32
    {
        GizmoOperation_None      = 0,
        GizmoOperation_Translate = 1 << 0,
        GizmoOperation_Rotate    = 1 << 1,
        GizmoOperation_Scale     = 1 << 2,
        GizmoOperation_Universal = GizmoOperation_Translate | GizmoOperation_Rotate | GizmoOperation_Scale,
    };

    enum class GizmoSpace : u8
    {
        World,
        Local,
    };

    struct GizmoView
    {
        Mat4 View;
        Mat4 Projection;
        Vec3 CameraPosition;
        Vec3 CameraRight;
        Vec3 CameraUp;
        Vec3 CameraForward;
        Vec2 ViewportSize; // same units as the mouse position
        bool Orthographic = false;
    };

    struct GizmoSettings
    {
        float Size              = 90.0f; // handle length on screen, logical px
        float DPIScale          = 1.0f;
        bool Snap               = false;
        float SnapTranslate     = 0.5f;
        float SnapRotateDegrees = 15.0f;
        float SnapScale         = 0.1f;
    };

    struct GizmoInput
    {
        Vec2 Mouse;           // relative to the viewport's top-left
        bool Down    = false; // left button held
        bool Pressed = false; // went down this frame
        bool Enabled = true;  // false: draw only (pointer is over UI, another tool has it...)
    };

    // Change since the drag began, in world space.
    struct GizmoResult
    {
        bool Active   = false; // a handle is being dragged
        bool Started  = false;
        bool Finished = false;
        bool Hovered  = false;
        u32 Operation = GizmoOperation_None;
        Vec3 Translation = Vec3(0.0f);
        Quat Rotation    = Quat(0.0f, 0.0f, 0.0f, 1.0f);
        Vec3 Scale       = Vec3(1.0f);
        Vec3 Pivot       = Vec3(0.0f); // where the drag started
    };

    // Transform handles drawn in the scene (debug renderer) and dragged with the mouse.
    namespace TransformGizmo
    {
        GizmoView ViewFromCamera(Camera* camera, Maths::Transform* cameraTransform, const Vec2& viewportSize);

        // One gizmo at a pivot. `id` ties a drag to its owner across frames.
        GizmoResult Manipulate(u64 id, const GizmoView& view, const GizmoInput& input, u32 operation, GizmoSpace space,
                               const Vec3& pivotPosition, const Quat& pivotRotation, const GizmoSettings& settings);

        // The whole job for scene entities: gizmo at their median point, one undo step
        // per drag, physics bodies moved with them. Returns true while dragging.
        bool ManipulateEntities(Scene* scene, const entt::entity* entities, u32 count, const GizmoView& view, const GizmoInput& input,
                                u32 operation, GizmoSpace space, const GizmoSettings& settings);

        bool IsUsing();
        bool IsOver();
    }
}
