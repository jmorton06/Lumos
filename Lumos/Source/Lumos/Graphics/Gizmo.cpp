#include "Precompiled.h"
#include "Gizmo.h"

#include "Core/Undo.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Transform.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/SceneGraph.h"
#include "Scene/SceneUndo.h"
#include "Scene/Component/Components.h"
#include "Scene/Component/RigidBody2DComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/B2PhysicsEngine/RigidBody2D.h"

namespace Lumos
{
    namespace
    {
        enum Handle : i32
        {
            Handle_None = -1,
            Handle_MoveX,
            Handle_MoveY,
            Handle_MoveZ,
            Handle_MoveYZ,
            Handle_MoveZX,
            Handle_MoveXY,
            Handle_MoveScreen,
            Handle_RotateX,
            Handle_RotateY,
            Handle_RotateZ,
            Handle_RotateScreen,
            Handle_ScaleX,
            Handle_ScaleY,
            Handle_ScaleZ,
            Handle_ScaleUniform,
        };

        struct GizmoState
        {
            u64 ActiveId  = 0;
            i32 Active    = Handle_None;
            bool Over     = false;

            // captured when the drag starts, so it stays stable as the object moves
            Vec3 StartPivot;
            Vec3 Axes[3];
            Vec2 StartMouse;
            Vec3 Axis;
            Vec3 PlaneNormal;
            Vec3 StartHit;
            float StartT        = 0.0f;
            float HandleLength  = 1.0f;
            bool ScreenFallback = false; // handle is edge-on: drive it from pointer travel instead
            Vec2 ScreenDir;
            float WorldPerPixel = 0.0f;
            float Angle         = 0.0f;

            GizmoResult Result;
        };

        GizmoState s_Gizmo;

        const Vec3 kUnitAxes[3]   = { Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) };
        const Vec4 kAxisColour[3] = { Vec4(1.0f, 0.30f, 0.28f, 1.0f), Vec4(0.45f, 0.95f, 0.40f, 1.0f), Vec4(0.35f, 0.60f, 1.0f, 1.0f) };
        const Vec4 kActiveColour  = Vec4(1.0f, 0.85f, 0.25f, 1.0f);
        const Vec4 kScreenColour  = Vec4(1.0f, 1.0f, 1.0f, 0.9f);

        bool WorldToScreen(const GizmoView& view, const Vec3& world, Vec2& out)
        {
            const Vec4 clip = view.Projection * (view.View * Vec4(world, 1.0f));
            if(Maths::Abs(clip.w) < 0.00001f)
                return false;
            if(!view.Orthographic && clip.w < 0.0f)
                return false; // behind the camera

            const float nx = clip.x / clip.w;
            const float ny = clip.y / clip.w;
            out            = Vec2((nx * 0.5f + 0.5f) * view.ViewportSize.x, (0.5f - ny * 0.5f) * view.ViewportSize.y);
            return true;
        }

        void ScreenRay(const GizmoView& view, const Vec2& mouse, Vec3& origin, Vec3& direction)
        {
            const Mat4 inverse = (view.Projection * view.View).Inverse();
            const float x      = 2.0f * mouse.x / Maths::Max(view.ViewportSize.x, 1.0f) - 1.0f;
            const float y      = 1.0f - 2.0f * mouse.y / Maths::Max(view.ViewportSize.y, 1.0f);

            Vec4 n = inverse * Vec4(x, y, 0.0f, 1.0f);
            n /= n.w;
            Vec4 f = inverse * Vec4(x, y, 1.0f, 1.0f);
            f /= f.w;

            origin    = Vec3(n);
            direction = (Vec3(f) - origin).Normalised();
        }

        // Line, not ray: a hit behind the origin still counts (orthographic views).
        bool LinePlane(const Vec3& origin, const Vec3& direction, const Vec3& planePoint, const Vec3& normal, Vec3& out)
        {
            const float denom = Vec3::Dot(direction, normal);
            if(Maths::Abs(denom) < 0.00001f)
                return false;
            const float t = Vec3::Dot(planePoint - origin, normal) / denom;
            out           = origin + direction * t;
            return true;
        }

        float DistanceToSegment(const Vec2& p, const Vec2& a, const Vec2& b)
        {
            const Vec2 ab    = b - a;
            const float len2 = ab.x * ab.x + ab.y * ab.y;
            if(len2 < 0.0001f)
                return (p - a).Length();
            float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2;
            t       = Maths::Clamp(t, 0.0f, 1.0f);
            return (p - (a + ab * t)).Length();
        }

        bool PointInQuad(const Vec2& p, const Vec2 q[4])
        {
            bool positive = false;
            bool negative = false;
            for(int i = 0; i < 4; i++)
            {
                const Vec2 a      = q[i];
                const Vec2 b      = q[(i + 1) % 4];
                const float cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
                if(cross > 0.0f)
                    positive = true;
                else if(cross < 0.0f)
                    negative = true;
            }
            return !(positive && negative);
        }

        float SnapTo(float value, float step)
        {
            if(step <= 0.0f)
                return value;
            return floorf(value / step + 0.5f) * step;
        }

        // Two vectors spanning the plane a ring lies in.
        void RingBasis(const Vec3& axis, Vec3& u, Vec3& v)
        {
            const Vec3 other = Maths::Abs(axis.y) < 0.9f ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
            u                = Vec3::Cross(axis, other).Normalised();
            v                = Vec3::Cross(axis, u).Normalised();
        }

        constexpr int kRingSegments = 48;

        Mat4 ComposeTRS(const Vec3& t, const Quat& r, const Vec3& s)
        {
            return Mat4::Translation(t) * r.ToMatrix4() * Mat4::Scale(s);
        }
    }

    namespace TransformGizmo
    {
        GizmoView ViewFromCamera(Camera* camera, Maths::Transform* cameraTransform, const Vec2& viewportSize)
        {
            GizmoView view;
            view.ViewportSize = viewportSize;
            if(!camera || !cameraTransform)
                return view;

            view.View           = cameraTransform->GetWorldMatrix().Inverse();
            view.Projection     = camera->GetProjectionMatrix();
            view.CameraPosition = cameraTransform->GetWorldPosition();
            view.CameraRight    = cameraTransform->GetRightDirection();
            view.CameraUp       = cameraTransform->GetUpDirection();
            view.CameraForward  = cameraTransform->GetForwardDirection();
            view.Orthographic   = camera->IsOrthographic();
            return view;
        }

        bool IsUsing()
        {
            return s_Gizmo.ActiveId != 0;
        }

        bool IsOver()
        {
            return s_Gizmo.Over || s_Gizmo.ActiveId != 0;
        }

        GizmoResult Manipulate(u64 id, const GizmoView& view, const GizmoInput& input, u32 operation, GizmoSpace space,
                               const Vec3& pivotPosition, const Quat& pivotRotation, const GizmoSettings& settings)
        {
            GizmoState& s = s_Gizmo;
            GizmoResult result;
            s.Over = false;

            if(id == 0)
                id = 1;

            // Owner went away mid-drag (selection changed, panel closed).
            if(s.ActiveId != 0 && s.ActiveId != id && !input.Down)
            {
                s.ActiveId = 0;
                s.Active   = Handle_None;
            }

            if(operation == GizmoOperation_None || view.ViewportSize.x < 1.0f || view.ViewportSize.y < 1.0f)
                return result;

            const bool dragging = s.ActiveId == id && s.Active != Handle_None;
            const float dpi     = Maths::Max(settings.DPIScale, 0.1f);

            const Vec3 origin = dragging ? s.StartPivot + s.Result.Translation : pivotPosition;
            Vec3 axes[3];
            for(int i = 0; i < 3; i++)
                axes[i] = dragging ? s.Axes[i] : (space == GizmoSpace::Local ? (pivotRotation * kUnitAxes[i]).Normalised() : kUnitAxes[i]);

            Vec2 originScreen;
            Vec2 rightScreen;
            if(!WorldToScreen(view, origin, originScreen) || !WorldToScreen(view, origin + view.CameraRight, rightScreen))
            {
                if(dragging && !input.Down)
                {
                    result          = s.Result;
                    result.Finished = true;
                    result.Active   = false;
                    s.ActiveId      = 0;
                    s.Active        = Handle_None;
                }
                return result;
            }

            const float pixelsPerUnit = (rightScreen - originScreen).Length();
            if(pixelsPerUnit < 0.0001f)
                return result;

            const float length = dragging ? s.HandleLength : (settings.Size * dpi) / pixelsPerUnit; // constant on screen

            const bool doMove   = (operation & GizmoOperation_Translate) != 0;
            const bool doRotate = (operation & GizmoOperation_Rotate) != 0;
            const bool doScale  = (operation & GizmoOperation_Scale) != 0;

            const float moveLength  = length;
            const float ringRadius  = (doRotate && (doMove || doScale)) ? length * 0.78f : length;
            const float scaleLength = doMove ? length * 1.22f : length;
            const float planeNear   = length * 0.28f;
            const float planeFar    = length * 0.50f;

            Vec3 viewDir = view.Orthographic ? view.CameraForward : (origin - view.CameraPosition);
            if(viewDir.LengthSquared() < 0.000001f)
                viewDir = view.CameraForward;
            viewDir.Normalise();

            float facing[3];
            for(int i = 0; i < 3; i++)
                facing[i] = Maths::Abs(Vec3::Dot(axes[i], viewDir));

            const Vec2 mouse = input.Mouse;

            // ---- which handle is under the pointer ----
            i32 hovered = Handle_None;
            if(!dragging && input.Enabled && s.ActiveId == 0)
            {
                const float pick         = 8.0f * dpi;
                const float centreRadius = 9.0f * dpi;

                if((mouse - originScreen).Length() < centreRadius)
                {
                    if(doMove)
                        hovered = Handle_MoveScreen;
                    else if(doScale)
                        hovered = Handle_ScaleUniform;
                }

                if(hovered == Handle_None && doMove)
                {
                    for(int i = 0; i < 3; i++)
                    {
                        if(facing[i] < 0.2f)
                            continue; // plane is edge-on
                        const Vec3& a = axes[(i + 1) % 3];
                        const Vec3& b = axes[(i + 2) % 3];
                        Vec2 quad[4];
                        if(WorldToScreen(view, origin + a * planeNear + b * planeNear, quad[0])
                           && WorldToScreen(view, origin + a * planeFar + b * planeNear, quad[1])
                           && WorldToScreen(view, origin + a * planeFar + b * planeFar, quad[2])
                           && WorldToScreen(view, origin + a * planeNear + b * planeFar, quad[3])
                           && PointInQuad(mouse, quad))
                        {
                            hovered = Handle_MoveYZ + i;
                            break;
                        }
                    }
                }

                if(hovered == Handle_None && doScale)
                {
                    float best = pick * 1.2f;
                    for(int i = 0; i < 3; i++)
                    {
                        if(facing[i] > 0.985f)
                            continue; // pointing straight at the camera
                        Vec2 tip;
                        if(!WorldToScreen(view, origin + axes[i] * scaleLength, tip))
                            continue;
                        float d = (mouse - tip).Length();
                        // on its own, the whole shaft grabs
                        if(!doMove)
                            d = Maths::Min(d, DistanceToSegment(mouse, originScreen, tip));
                        if(d < best)
                        {
                            best    = d;
                            hovered = Handle_ScaleX + i;
                        }
                    }
                }

                if(hovered == Handle_None && doMove)
                {
                    float best = pick;
                    for(int i = 0; i < 3; i++)
                    {
                        if(facing[i] > 0.985f)
                            continue;
                        Vec2 tip;
                        if(!WorldToScreen(view, origin + axes[i] * moveLength, tip))
                            continue;
                        const float d = DistanceToSegment(mouse, originScreen, tip);
                        if(d < best)
                        {
                            best    = d;
                            hovered = Handle_MoveX + i;
                        }
                    }
                }

                if(hovered == Handle_None && doRotate)
                {
                    float best = pick;
                    for(int ring = 0; ring < 4; ring++)
                    {
                        const bool screenRing = ring == 3;
                        if(!screenRing && facing[ring] < 0.12f)
                            continue; // seen edge-on it is just a line
                        const Vec3 axis    = screenRing ? viewDir * -1.0f : axes[ring];
                        const float radius = screenRing ? ringRadius * 1.18f : ringRadius;
                        Vec3 u, v;
                        RingBasis(axis, u, v);

                        Vec2 previous;
                        bool hasPrevious = false;
                        for(int k = 0; k <= kRingSegments; k++)
                        {
                            const float angle = (float)k / (float)kRingSegments * Maths::M_PI * 2.0f;
                            const Vec3 offset = (u * cosf(angle) + v * sinf(angle)) * radius;
                            // the far side of an axis ring is not pickable
                            const bool front = screenRing || view.Orthographic || Vec3::Dot(offset, viewDir) <= radius * 0.08f;
                            Vec2 current;
                            const bool ok = front && WorldToScreen(view, origin + offset, current);
                            if(ok && hasPrevious)
                            {
                                const float d = DistanceToSegment(mouse, previous, current);
                                if(d < best)
                                {
                                    best    = d;
                                    hovered = screenRing ? (i32)Handle_RotateScreen : (i32)(Handle_RotateX + ring);
                                }
                            }
                            previous    = current;
                            hasPrevious = ok;
                        }
                    }
                }
            }

            // ---- start a drag ----
            if(hovered != Handle_None && input.Pressed && input.Enabled && s.ActiveId == 0)
            {
                s.ActiveId       = id;
                s.Active         = hovered;
                s.StartPivot     = origin;
                s.StartMouse     = mouse;
                s.HandleLength   = length;
                s.ScreenFallback = false;
                s.Angle          = 0.0f;
                s.StartT         = 0.0f;
                for(int i = 0; i < 3; i++)
                    s.Axes[i] = axes[i];

                s.Result           = GizmoResult();
                s.Result.Pivot     = origin;
                s.Result.Operation = hovered <= Handle_MoveScreen ? GizmoOperation_Translate
                    : (hovered <= Handle_RotateScreen ? GizmoOperation_Rotate : GizmoOperation_Scale);

                Vec3 rayOrigin, rayDir;
                ScreenRay(view, mouse, rayOrigin, rayDir);

                auto setupAxis = [&](const Vec3& axis)
                {
                    s.Axis = axis;
                    // the plane through the axis that faces the camera most squarely
                    Vec3 normal = Vec3::Cross(axis, Vec3::Cross(viewDir, axis));
                    Vec3 hit;
                    if(normal.LengthSquared() > 0.000001f)
                    {
                        normal.Normalise();
                        s.PlaneNormal = normal;
                        if(LinePlane(rayOrigin, rayDir, origin, normal, hit))
                        {
                            s.StartT = Vec3::Dot(hit - origin, axis);
                            return;
                        }
                    }
                    // degenerate: follow the axis as it appears on screen
                    s.ScreenFallback = true;
                    Vec2 tip;
                    if(WorldToScreen(view, origin + axis * length, tip))
                    {
                        const Vec2 dir    = tip - originScreen;
                        const float len   = dir.Length();
                        s.ScreenDir       = len > 0.0001f ? dir / len : Vec2(1.0f, 0.0f);
                        s.WorldPerPixel   = len > 0.0001f ? length / len : 0.0f;
                    }
                    else
                    {
                        s.ScreenDir     = Vec2(1.0f, 0.0f);
                        s.WorldPerPixel = 0.0f;
                    }
                };

                auto setupPlane = [&](const Vec3& normal)
                {
                    s.PlaneNormal = normal;
                    if(!LinePlane(rayOrigin, rayDir, origin, normal, s.StartHit))
                        s.StartHit = origin;
                };

                switch(hovered)
                {
                case Handle_MoveX:
                case Handle_MoveY:
                case Handle_MoveZ:
                    setupAxis(axes[hovered - Handle_MoveX]);
                    break;
                case Handle_MoveYZ:
                case Handle_MoveZX:
                case Handle_MoveXY:
                    setupPlane(axes[hovered - Handle_MoveYZ]);
                    break;
                case Handle_MoveScreen:
                    setupPlane(viewDir);
                    break;
                case Handle_RotateX:
                case Handle_RotateY:
                case Handle_RotateZ:
                case Handle_RotateScreen:
                {
                    s.Axis = hovered == Handle_RotateScreen ? viewDir * -1.0f : axes[hovered - Handle_RotateX];
                    setupPlane(s.Axis);
                    if(Maths::Abs(Vec3::Dot(s.Axis, viewDir)) < 0.2f || (s.StartHit - origin).LengthSquared() < 0.0000001f)
                    {
                        // ring nearly edge-on: turn by pointer travel across the axis
                        s.ScreenFallback = true;
                        Vec2 tip;
                        Vec2 dir = Vec2(0.0f, 1.0f);
                        if(WorldToScreen(view, origin + s.Axis * length, tip) && (tip - originScreen).Length() > 0.001f)
                            dir = (tip - originScreen) / (tip - originScreen).Length();
                        s.ScreenDir = Vec2(-dir.y, dir.x);
                    }
                }
                break;
                case Handle_ScaleX:
                case Handle_ScaleY:
                case Handle_ScaleZ:
                    setupAxis(axes[hovered - Handle_ScaleX]);
                    break;
                default:
                    break;
                }

                result.Started = true;
            }

            // ---- update the drag ----
            if(s.ActiveId == id && s.Active != Handle_None)
            {
                if(input.Down)
                {
                    Vec3 rayOrigin, rayDir;
                    ScreenRay(view, mouse, rayOrigin, rayDir);
                    const Vec2 moved = mouse - s.StartMouse;

                    auto alongAxis = [&]()
                    {
                        if(s.ScreenFallback)
                            return (moved.x * s.ScreenDir.x + moved.y * s.ScreenDir.y) * s.WorldPerPixel;
                        Vec3 hit;
                        if(!LinePlane(rayOrigin, rayDir, s.StartPivot, s.PlaneNormal, hit))
                            return 0.0f;
                        return Vec3::Dot(hit - s.StartPivot, s.Axis) - s.StartT;
                    };

                    switch(s.Active)
                    {
                    case Handle_MoveX:
                    case Handle_MoveY:
                    case Handle_MoveZ:
                    {
                        float t = alongAxis();
                        if(settings.Snap)
                            t = SnapTo(t, settings.SnapTranslate);
                        s.Result.Translation = s.Axis * t;
                    }
                    break;
                    case Handle_MoveYZ:
                    case Handle_MoveZX:
                    case Handle_MoveXY:
                    case Handle_MoveScreen:
                    {
                        Vec3 hit;
                        if(LinePlane(rayOrigin, rayDir, s.StartPivot, s.PlaneNormal, hit))
                        {
                            Vec3 delta = hit - s.StartHit;
                            if(settings.Snap && s.Active != Handle_MoveScreen)
                            {
                                const int n   = s.Active - Handle_MoveYZ;
                                const Vec3& a = s.Axes[(n + 1) % 3];
                                const Vec3& b = s.Axes[(n + 2) % 3];
                                delta         = a * SnapTo(Vec3::Dot(delta, a), settings.SnapTranslate)
                                    + b * SnapTo(Vec3::Dot(delta, b), settings.SnapTranslate);
                            }
                            s.Result.Translation = delta;
                        }
                    }
                    break;
                    case Handle_RotateX:
                    case Handle_RotateY:
                    case Handle_RotateZ:
                    case Handle_RotateScreen:
                    {
                        float angle = s.Angle;
                        if(s.ScreenFallback)
                        {
                            angle = (moved.x * s.ScreenDir.x + moved.y * s.ScreenDir.y) / (settings.Size * dpi) * 2.0f;
                        }
                        else
                        {
                            Vec3 hit;
                            if(LinePlane(rayOrigin, rayDir, s.StartPivot, s.PlaneNormal, hit))
                            {
                                Vec3 from = s.StartHit - s.StartPivot;
                                Vec3 to   = hit - s.StartPivot;
                                if(from.LengthSquared() > 0.0000001f && to.LengthSquared() > 0.0000001f)
                                {
                                    from.Normalise();
                                    to.Normalise();
                                    angle = atan2f(Vec3::Dot(Vec3::Cross(from, to), s.Axis), Vec3::Dot(from, to));
                                }
                            }
                        }
                        if(settings.Snap)
                            angle = SnapTo(angle, Maths::ToRadians(settings.SnapRotateDegrees));
                        s.Angle           = angle;
                        s.Result.Rotation = Quat::Rotation(angle, s.Axis);
                    }
                    break;
                    case Handle_ScaleX:
                    case Handle_ScaleY:
                    case Handle_ScaleZ:
                    {
                        float factor = 1.0f + alongAxis() / Maths::Max(s.HandleLength, 0.0001f);
                        if(settings.Snap)
                            factor = SnapTo(factor, settings.SnapScale);
                        factor         = Maths::Max(factor, 0.01f);
                        s.Result.Scale = Vec3(1.0f);
                        (&s.Result.Scale.x)[s.Active - Handle_ScaleX] = factor;
                    }
                    break;
                    case Handle_ScaleUniform:
                    {
                        float factor = 1.0f + (moved.x - moved.y) / (settings.Size * dpi);
                        if(settings.Snap)
                            factor = SnapTo(factor, settings.SnapScale);
                        s.Result.Scale = Vec3(Maths::Max(factor, 0.01f));
                    }
                    break;
                    default:
                        break;
                    }

                    const bool started = result.Started;
                    result             = s.Result;
                    result.Active      = true;
                    result.Started     = started;
                }
                else
                {
                    const bool started = result.Started;
                    result             = s.Result;
                    result.Active      = false;
                    result.Started     = started;
                    result.Finished    = true;
                    s.ActiveId         = 0;
                    s.Active           = Handle_None;
                }
            }

            const i32 lit  = (s.ActiveId == id) ? s.Active : hovered;
            s.Over         = lit != Handle_None;
            result.Hovered = hovered != Handle_None;

            // ---- draw ----
            const Vec3 drawOrigin = (s.ActiveId == id) ? s.StartPivot + s.Result.Translation : origin;
            const float thickness = length * 0.022f;
            DebugRenderer::RequestPass();

            auto colourFor = [&](i32 handle, const Vec4& base)
            {
                return lit == handle ? kActiveColour : base;
            };

            if(doRotate)
            {
                for(int ring = 0; ring < 4; ring++)
                {
                    const bool screenRing = ring == 3;
                    if(!screenRing && facing[ring] < 0.12f)
                        continue;
                    const Vec3 axis    = screenRing ? viewDir * -1.0f : axes[ring];
                    const float radius = screenRing ? ringRadius * 1.18f : ringRadius;
                    const i32 handle   = screenRing ? (i32)Handle_RotateScreen : (i32)(Handle_RotateX + ring);
                    Vec4 colour        = colourFor(handle, screenRing ? Vec4(1.0f, 1.0f, 1.0f, 0.45f) : kAxisColour[ring]);

                    Vec3 u, v;
                    RingBasis(axis, u, v);
                    Vec3 previous = drawOrigin + u * radius;
                    for(int k = 1; k <= kRingSegments; k++)
                    {
                        const float angle  = (float)k / (float)kRingSegments * Maths::M_PI * 2.0f;
                        const Vec3 offset  = (u * cosf(angle) + v * sinf(angle)) * radius;
                        const Vec3 current = drawOrigin + offset;
                        const bool front   = screenRing || view.Orthographic || Vec3::Dot(offset, viewDir) <= radius * 0.08f;
                        Vec4 c             = colour;
                        if(!front)
                            c.w *= 0.18f;
                        DebugRenderer::DrawThickLine(previous, current, thickness * 0.8f, false, c, 0.0f);
                        previous = current;
                    }
                }

                // spoke showing how far it has turned
                if(s.ActiveId == id && s.Active >= Handle_RotateX && s.Active <= Handle_RotateScreen && !s.ScreenFallback)
                {
                    Vec3 from = s.StartHit - s.StartPivot;
                    if(from.LengthSquared() > 0.0000001f)
                    {
                        from.Normalise();
                        const float radius = s.Active == Handle_RotateScreen ? ringRadius * 1.18f : ringRadius;
                        const Vec3 to      = Quat::Rotation(s.Angle, s.Axis) * from;
                        DebugRenderer::DrawThickLine(drawOrigin, drawOrigin + from * radius, thickness * 0.6f, false, Vec4(1.0f, 1.0f, 1.0f, 0.5f), 0.0f);
                        DebugRenderer::DrawThickLine(drawOrigin, drawOrigin + to * radius, thickness * 0.6f, false, kActiveColour, 0.0f);
                    }
                }
            }

            if(doMove)
            {
                for(int i = 0; i < 3; i++)
                {
                    // plane handles
                    if(facing[i] >= 0.2f)
                    {
                        const Vec3& a = axes[(i + 1) % 3];
                        const Vec3& b = axes[(i + 2) % 3];
                        Vec4 colour   = colourFor(Handle_MoveYZ + i, kAxisColour[i]);
                        colour.w      = lit == Handle_MoveYZ + i ? 0.75f : 0.35f;
                        const Vec3 p0 = drawOrigin + a * planeNear + b * planeNear;
                        const Vec3 p1 = drawOrigin + a * planeFar + b * planeNear;
                        const Vec3 p2 = drawOrigin + a * planeFar + b * planeFar;
                        const Vec3 p3 = drawOrigin + a * planeNear + b * planeFar;
                        DebugRenderer::DrawTriangle(p0, p1, p2, false, colour, 0.0f);
                        DebugRenderer::DrawTriangle(p0, p2, p3, false, colour, 0.0f);
                    }

                    if(facing[i] > 0.985f)
                        continue;
                    const Vec4 colour = colourFor(Handle_MoveX + i, kAxisColour[i]);
                    const Vec3 tip    = drawOrigin + axes[i] * moveLength;
                    DebugRenderer::DrawThickLine(drawOrigin, tip, thickness, false, colour, 0.0f);
                    DebugRenderer::DrawPoint(tip, length * 0.055f, false, colour, 0.0f);
                }
                DebugRenderer::DrawPoint(drawOrigin, length * 0.05f, false, colourFor(Handle_MoveScreen, kScreenColour), 0.0f);
            }

            if(doScale)
            {
                for(int i = 0; i < 3; i++)
                {
                    if(facing[i] > 0.985f)
                        continue;
                    const Vec4 colour = colourFor(Handle_ScaleX + i, kAxisColour[i]);
                    // the handle stretches with the drag
                    float stretch = 1.0f;
                    if(s.ActiveId == id && s.Active == Handle_ScaleX + i)
                        stretch = (&s.Result.Scale.x)[i];
                    const Vec3 tip = drawOrigin + axes[i] * (scaleLength * stretch);
                    if(!doMove)
                        DebugRenderer::DrawThickLine(drawOrigin, tip, thickness, false, colour, 0.0f);
                    else
                        DebugRenderer::DrawThickLine(drawOrigin + axes[i] * moveLength, tip, thickness * 0.6f, false, colour, 0.0f);
                    DebugRenderer::DrawPoint(tip, length * 0.07f, false, colour, 0.0f);
                }
                if(!doMove)
                    DebugRenderer::DrawPoint(drawOrigin, length * 0.07f, false, colourFor(Handle_ScaleUniform, kScreenColour), 0.0f);
            }

            return result;
        }

        // ---- entities ----

        namespace
        {
            constexpr u32 kMaxDragEntities = 64;

            struct EntityDrag
            {
                bool Active = false;
                u32 Count   = 0;
                entt::entity Entities[kMaxDragEntities];
                Mat4 StartWorld[kMaxDragEntities];
            };
            EntityDrag s_EntityDrag;

            Mat4 ParentWorld(entt::registry& registry, entt::entity entity, bool& hasParent)
            {
                hasParent = false;
                if(Hierarchy* hierarchy = registry.try_get<Hierarchy>(entity))
                {
                    const entt::entity parent = hierarchy->Parent();
                    if(parent != entt::null && registry.valid(parent))
                    {
                        if(Maths::Transform* parentTransform = registry.try_get<Maths::Transform>(parent))
                        {
                            hasParent = true;
                            return parentTransform->GetWorldMatrix();
                        }
                    }
                }
                return Mat4(1.0f);
            }
        }

        bool ManipulateEntities(Scene* scene, const entt::entity* entities, u32 count, const GizmoView& view, const GizmoInput& input,
                                u32 operation, GizmoSpace space, const GizmoSettings& settings)
        {
            if(!scene || !entities || count == 0)
            {
                s_EntityDrag.Active = false;
                return false;
            }

            entt::registry& registry = scene->GetRegistry();

            // what can actually be moved
            entt::entity valid[kMaxDragEntities];
            u32 validCount = 0;
            Vec3 median    = Vec3(0.0f);
            u64 id         = 0x47495A4D4Full;
            for(u32 i = 0; i < count && validCount < kMaxDragEntities; i++)
            {
                const entt::entity e = entities[i];
                if(!registry.valid(e) || registry.all_of<EditorLockComponent>(e))
                    continue;
                Maths::Transform* transform = registry.try_get<Maths::Transform>(e);
                if(!transform)
                    continue;
                valid[validCount++] = e;
                median += transform->GetWorldPosition();
                id = id * 1099511628211ull + (u64)entt::to_integral(e);
            }
            if(validCount == 0)
            {
                s_EntityDrag.Active = false;
                return false;
            }
            median /= (float)validCount;

            Quat pivotRotation = Quat(0.0f, 0.0f, 0.0f, 1.0f);
            if(validCount == 1)
                pivotRotation = registry.get<Maths::Transform>(valid[0]).GetWorldOrientation();

            // scaling is along the object's own axes whatever the space
            const GizmoSpace usedSpace = (validCount == 1 && operation == GizmoOperation_Scale) ? GizmoSpace::Local : space;

            const GizmoResult result = Manipulate(id, view, input, operation, usedSpace, median, pivotRotation, settings);

            if(result.Started)
            {
                s_EntityDrag.Active = true;
                s_EntityDrag.Count  = validCount;
                UndoSetLabel("Transform");
                for(u32 i = 0; i < validCount; i++)
                {
                    Maths::Transform& transform   = registry.get<Maths::Transform>(valid[i]);
                    s_EntityDrag.Entities[i]      = valid[i];
                    s_EntityDrag.StartWorld[i]    = transform.GetWorldMatrix();
                    SceneUndo::PushComponent<Maths::Transform>(Entity(valid[i], scene));
                }
            }

            if(s_EntityDrag.Active && (result.Active || result.Finished))
            {
                for(u32 i = 0; i < s_EntityDrag.Count; i++)
                {
                    const entt::entity e = s_EntityDrag.Entities[i];
                    if(!registry.valid(e))
                        continue;
                    Maths::Transform* transform = registry.try_get<Maths::Transform>(e);
                    if(!transform)
                        continue;

                    Vec3 position, scale;
                    Quat rotation;
                    s_EntityDrag.StartWorld[i].Decompose(position, rotation, scale);

                    if(result.Operation == GizmoOperation_Translate)
                    {
                        position += result.Translation;
                    }
                    else if(result.Operation == GizmoOperation_Rotate)
                    {
                        rotation = result.Rotation * rotation;
                        position = result.Pivot + result.Rotation * (position - result.Pivot);
                    }
                    else if(result.Operation == GizmoOperation_Scale)
                    {
                        scale = scale * result.Scale;
                        if(s_EntityDrag.Count > 1)
                            position = result.Pivot + (position - result.Pivot) * result.Scale;
                    }

                    bool hasParent    = false;
                    const Mat4 parent = ParentWorld(registry, e, hasParent);
                    Mat4 local        = ComposeTRS(position, rotation, scale);
                    if(hasParent)
                        local = parent.Inverse() * local;

                    Vec3 localPosition, localScale;
                    Quat localRotation;
                    local.Decompose(localPosition, localRotation, localScale);

                    // only what the handle changes: decomposing drifts the rest
                    if(result.Operation == GizmoOperation_Translate)
                    {
                        transform->SetLocalPosition(localPosition);
                    }
                    else if(result.Operation == GizmoOperation_Rotate)
                    {
                        transform->SetLocalOrientation(localRotation);
                        transform->SetLocalPosition(localPosition);
                    }
                    else if(result.Operation == GizmoOperation_Scale)
                    {
                        transform->SetLocalScale(localScale);
                        if(s_EntityDrag.Count > 1)
                            transform->SetLocalPosition(localPosition);
                    }

                    if(hasParent)
                        transform->SetWorldMatrix(parent);
                    else
                        transform->SetWorldMatrix();

                    if(RigidBody2DComponent* body2D = registry.try_get<RigidBody2DComponent>(e))
                    {
                        if(body2D->GetRigidBody())
                            body2D->GetRigidBody()->SetPosition(Vec2(position.x, position.y));
                    }
                    else if(RigidBody3DComponent* body3D = registry.try_get<RigidBody3DComponent>(e))
                    {
                        if(RigidBody3D* body = body3D->GetRigidBody())
                        {
                            body->SetPosition(position);
                            body->SetOrientation(rotation);
                        }
                    }
                }
            }

            if(result.Finished)
            {
                s_EntityDrag.Active = false;
                UndoCommit();
            }

            return result.Active;
        }
    }
}
