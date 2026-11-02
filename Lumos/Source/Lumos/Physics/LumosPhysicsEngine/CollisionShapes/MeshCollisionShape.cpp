#include "Precompiled.h"
#include "MeshCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/SceneRenderer.h"
#include "Core/Application.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Maths/MathsUtilities.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"

#include <ModelLoaders/meshoptimizer/src/meshoptimizer.h>

namespace Lumos
{
    MeshCollisionShape::MeshCollisionShape()
        : m_BoundsMin(0.0f)
        , m_BoundsMax(0.0f)
    {
        m_Type = CollisionShapeType::CollisionMesh;
    }

    MeshCollisionShape::MeshCollisionShape(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices)
        : m_BoundsMin(0.0f)
        , m_BoundsMax(0.0f)
    {
        m_Type = CollisionShapeType::CollisionMesh;
        SetMeshData(vertices, indices);
    }

    MeshCollisionShape::~MeshCollisionShape()
    {
    }

    void MeshCollisionShape::SetMeshData(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices)
    {
        m_Vertices = vertices;
        m_Triangles.Clear();

        // Build triangles from indices
        for(uint32_t i = 0; i + 2 < indices.Size(); i += 3)
        {
            MeshTriangle tri;
            tri.v0 = vertices[indices[i]];
            tri.v1 = vertices[indices[i + 1]];
            tri.v2 = vertices[indices[i + 2]];

            Vec3 edge1  = tri.v1 - tri.v0;
            Vec3 edge2  = tri.v2 - tri.v0;
            tri.Normal = Maths::Cross(edge1, edge2).Normalised();

            m_Triangles.PushBack(tri);
        }

        ComputeBounds();
        BuildBVH();
    }

    bool MeshCollisionShape::BuildFromModel(Graphics::Model* model, const Mat4& bake)
    {
        if(!model)
            return false;

        m_Vertices.Clear();
        m_Triangles.Clear();
        m_Nodes.Clear();

        uint32_t total = 0;
        for(auto& mesh : model->GetMeshes())
            if(mesh)
                total += (uint32_t)mesh->GetCPUIndices().Size() / 3;
        if(total == 0)
            return false;
        m_Triangles.Reserve(total);

        for(auto& mesh : model->GetMeshes())
        {
            if(!mesh)
                continue;
            const TDArray<Vec3>& positions    = mesh->GetCPUPositions();
            const TDArray<uint32_t>& allIndices = mesh->GetCPUIndices();
            const uint32_t vertCount          = (uint32_t)positions.Size();

            // fewer triangles: meshoptimizer keeps the shape within a small error of the original
            TDArray<uint32_t> simplified;
            if(m_Detail < 0.999f && allIndices.Size() > 3 && vertCount > 0)
            {
                size_t target = (size_t)(allIndices.Size() * m_Detail) / 3 * 3;
                if(target < 3)
                    target = 3;
                simplified.Resize((uint32_t)allIndices.Size());
                const size_t count = meshopt_simplify(simplified.Data(), allIndices.Data(), allIndices.Size(), &positions[0].x, vertCount, sizeof(Vec3), target, 0.05f, 0, nullptr);
                simplified.Resize((uint32_t)count);
            }
            const TDArray<uint32_t>& indices = simplified.Empty() ? allIndices : simplified;

            for(uint32_t i = 0; i + 2 < indices.Size(); i += 3)
            {
                if(indices[i] >= vertCount || indices[i + 1] >= vertCount || indices[i + 2] >= vertCount)
                    continue;
                MeshTriangle tri;
                tri.v0 = Vec3(bake * Vec4(positions[indices[i]], 1.0f));
                tri.v1 = Vec3(bake * Vec4(positions[indices[i + 1]], 1.0f));
                tri.v2 = Vec3(bake * Vec4(positions[indices[i + 2]], 1.0f));

                const Vec3 n     = Maths::Cross(tri.v1 - tri.v0, tri.v2 - tri.v0);
                const float len2 = Maths::Length2(n);
                if(len2 < 1e-14f) // zero area
                    continue;
                tri.Normal = n / Maths::Sqrt(len2);
                m_Triangles.PushBack(tri);
            }
        }

        ComputeBounds();
        BuildBVH();
        return !m_Triangles.Empty();
    }

    static inline Vec3 MinV(const Vec3& a, const Vec3& b) { return Vec3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z); }
    static inline Vec3 MaxV(const Vec3& a, const Vec3& b) { return Vec3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z); }

    void MeshCollisionShape::BuildBVH()
    {
        m_Nodes.Clear();
        const uint32_t triCount = (uint32_t)m_Triangles.Size();
        if(triCount == 0)
            return;

        static constexpr uint32_t kLeafSize = 4;
        m_Nodes.Reserve(triCount); // upper bound is about 2 * triCount / kLeafSize
        m_Nodes.PushBack({ Vec3(0.0f), Vec3(0.0f), 0, triCount });

        // Median split on the widest axis of the triangle centres; triangles are reordered in place.
        uint32_t stack[128];
        uint32_t top = 0;
        stack[top++] = 0;
        while(top > 0)
        {
            const uint32_t nodeIndex = stack[--top];
            const uint32_t first     = m_Nodes[nodeIndex].First;
            const uint32_t count     = m_Nodes[nodeIndex].Count;

            Vec3 lo(FLT_MAX), hi(-FLT_MAX), cLo(FLT_MAX), cHi(-FLT_MAX);
            for(uint32_t i = first; i < first + count; i++)
            {
                const MeshTriangle& t = m_Triangles[i];
                lo                    = MinV(lo, MinV(t.v0, MinV(t.v1, t.v2)));
                hi                    = MaxV(hi, MaxV(t.v0, MaxV(t.v1, t.v2)));
                const Vec3 c          = (t.v0 + t.v1 + t.v2) * (1.0f / 3.0f);
                cLo                   = MinV(cLo, c);
                cHi                   = MaxV(cHi, c);
            }
            m_Nodes[nodeIndex].Min = lo;
            m_Nodes[nodeIndex].Max = hi;

            if(count <= kLeafSize || top + 2 > 128)
                continue;

            const Vec3 extent = cHi - cLo;
            int axis          = 0;
            if(extent.y > extent.x)
                axis = 1;
            if(extent.z > extent[axis])
                axis = 2;
            if(extent[axis] <= 0.0f)
                continue; // all centres coincide: leave as one leaf

            // quickselect: everything before mid ends up no further along the axis than everything after
            const uint32_t mid = first + count / 2;
            auto key           = [axis](const MeshTriangle& t)
            { return t.v0[axis] + t.v1[axis] + t.v2[axis]; };
            uint32_t lowIndex = first, highIndex = first + count - 1;
            while(lowIndex < highIndex)
            {
                const float pivot = key(m_Triangles[(lowIndex + highIndex) / 2]);
                uint32_t i = lowIndex, j = highIndex;
                while(i <= j)
                {
                    while(key(m_Triangles[i]) < pivot)
                        i++;
                    while(key(m_Triangles[j]) > pivot)
                        j--;
                    if(i <= j)
                    {
                        const MeshTriangle tmp = m_Triangles[i];
                        m_Triangles[i]         = m_Triangles[j];
                        m_Triangles[j]         = tmp;
                        i++;
                        if(j == 0)
                            break;
                        j--;
                    }
                }
                if(mid <= j)
                    highIndex = j;
                else if(mid >= i)
                    lowIndex = i;
                else
                    break;
            }

            const uint32_t left = (uint32_t)m_Nodes.Size();
            m_Nodes.PushBack({ Vec3(0.0f), Vec3(0.0f), first, mid - first });
            m_Nodes.PushBack({ Vec3(0.0f), Vec3(0.0f), mid, first + count - mid });
            m_Nodes[nodeIndex].First = left;
            m_Nodes[nodeIndex].Count = 0;
            stack[top++]             = left;
            stack[top++]             = left + 1;
        }
    }

    uint32_t MeshCollisionShape::QueryAABB(const Vec3& boxMin, const Vec3& boxMax, uint32_t* outTriangles, uint32_t maxTriangles) const
    {
        if(m_Nodes.Empty())
            return 0;

        uint32_t found = 0;
        uint32_t stack[64];
        uint32_t top = 0;
        stack[top++] = 0;
        while(top > 0)
        {
            const MeshBVHNode& node = m_Nodes[stack[--top]];
            if(node.Min.x > boxMax.x || node.Max.x < boxMin.x || node.Min.y > boxMax.y || node.Max.y < boxMin.y || node.Min.z > boxMax.z || node.Max.z < boxMin.z)
                continue;

            if(node.Count == 0)
            {
                if(top + 2 <= 64)
                {
                    stack[top++] = node.First;
                    stack[top++] = node.First + 1;
                }
                continue;
            }

            for(uint32_t i = node.First; i < node.First + node.Count; i++)
            {
                const MeshTriangle& t = m_Triangles[i];
                const Vec3 lo         = MinV(t.v0, MinV(t.v1, t.v2));
                const Vec3 hi         = MaxV(t.v0, MaxV(t.v1, t.v2));
                if(lo.x > boxMax.x || hi.x < boxMin.x || lo.y > boxMax.y || hi.y < boxMin.y || lo.z > boxMax.z || hi.z < boxMin.z)
                    continue;
                if(found == maxTriangles)
                    return found;
                outTriangles[found++] = i;
            }
        }
        return found;
    }

    bool MeshCollisionShape::RayCast(const Vec3& origin, const Vec3& direction, float maxDist, float radius, float& outT, Vec3& outNormal) const
    {
        if(m_Nodes.Empty())
            return false;

        const Vec3 invDir(1.0f / (Maths::Abs(direction.x) > 1e-12f ? direction.x : 1e-12f),
            1.0f / (Maths::Abs(direction.y) > 1e-12f ? direction.y : 1e-12f),
            1.0f / (Maths::Abs(direction.z) > 1e-12f ? direction.z : 1e-12f));

        bool hit    = false;
        float bestT = maxDist;

        uint32_t stack[64];
        uint32_t top = 0;
        stack[top++] = 0;
        while(top > 0)
        {
            const MeshBVHNode& node = m_Nodes[stack[--top]];

            // slab test against the node grown by the cast radius
            float tNear = 0.0f, tFar = bestT;
            bool miss = false;
            for(int a = 0; a < 3 && !miss; a++)
            {
                float t0 = (node.Min[a] - radius - origin[a]) * invDir[a];
                float t1 = (node.Max[a] + radius - origin[a]) * invDir[a];
                if(t0 > t1)
                {
                    const float tmp = t0;
                    t0              = t1;
                    t1              = tmp;
                }
                tNear = Maths::Max(tNear, t0);
                tFar  = Maths::Min(tFar, t1);
                miss  = tNear > tFar;
            }
            if(miss)
                continue;

            if(node.Count == 0)
            {
                if(top + 2 <= 64)
                {
                    stack[top++] = node.First;
                    stack[top++] = node.First + 1;
                }
                continue;
            }

            for(uint32_t i = node.First; i < node.First + node.Count; i++)
            {
                const MeshTriangle& tri = m_Triangles[i];
                Vec3 normal             = tri.Normal;
                float denom             = Maths::Dot(normal, direction);
                if(Maths::Abs(denom) < 1e-8f)
                    continue;
                if(denom > 0.0f) // hit from behind: face the ray
                {
                    normal = normal * -1.0f;
                    denom  = -denom;
                }

                // a sphere cast moves the plane a radius toward the ray: exact on faces, generous on edges
                const float t = (Maths::Dot(normal, tri.v0 - origin) + radius) / denom;
                if(t < 0.0f || t >= bestT)
                    continue;

                const Vec3 contact = origin + direction * t - normal * radius;
                const Vec3& n      = tri.Normal;
                if(Maths::Dot(Maths::Cross(tri.v1 - tri.v0, contact - tri.v0), n) < 0.0f
                    || Maths::Dot(Maths::Cross(tri.v2 - tri.v1, contact - tri.v1), n) < 0.0f
                    || Maths::Dot(Maths::Cross(tri.v0 - tri.v2, contact - tri.v2), n) < 0.0f)
                    continue;

                bestT     = t;
                outT      = t;
                outNormal = normal;
                hit       = true;
            }
        }
        return hit;
    }

    void MeshCollisionShape::BuildFromMesh(Graphics::Mesh* mesh)
    {
        if(!mesh)
            return;

        const TDArray<Vec3>& positions    = mesh->GetCPUPositions();
        const TDArray<uint32_t>& indices  = mesh->GetCPUIndices();
        if(positions.Empty() || indices.Empty())
        {
            LWARN("MeshCollisionShape::BuildFromMesh: mesh has no CPU-side position/index data; build the mesh from a TDArray<Vertex>/TDArray<uint32_t> pair so it retains positions, or call SetMeshData() directly.");
            return;
        }

        SetMeshData(positions, indices);
    }

    void MeshCollisionShape::ComputeBounds()
    {
        if(m_Triangles.Empty())
        {
            m_BoundsMin = Vec3(0.0f);
            m_BoundsMax = Vec3(0.0f);
            return;
        }

        m_BoundsMin = Vec3(FLT_MAX);
        m_BoundsMax = Vec3(-FLT_MAX);
        for(const auto& t : m_Triangles)
        {
            m_BoundsMin = MinV(m_BoundsMin, MinV(t.v0, MinV(t.v1, t.v2)));
            m_BoundsMax = MaxV(m_BoundsMax, MaxV(t.v0, MaxV(t.v1, t.v2)));
        }
    }

    Mat3 MeshCollisionShape::BuildInverseInertia(float invMass) const
    {
        // Approximate using bounding box inertia
        Vec3 size = m_BoundsMax - m_BoundsMin;
        if(invMass <= 0.0f || m_Triangles.Empty())
            return Mat3(0.0f);
        float mass = 1.0f / invMass;
        float factor = mass / 12.0f;

        Mat3 inertia(1.0f);
        inertia.SetDiagonal(Vec3(
            1.0f / (factor * (size.y * size.y + size.z * size.z)),
            1.0f / (factor * (size.x * size.x + size.z * size.z)),
            1.0f / (factor * (size.x * size.x + size.y * size.y))));

        return inertia;
    }

    float MeshCollisionShape::GetSize() const
    {
        return Maths::Length(m_BoundsMax - m_BoundsMin);
    }

    void MeshCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        if(m_Triangles.Empty())
            return;

        const Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);

        // What the line batch can take in a frame alongside everything else.
        const uint32_t kBudget = 20000;
        ArenaTemp scratch      = ScratchBegin(0, 0);
        uint32_t* picked       = nullptr;
        uint32_t count         = (uint32_t)m_Triangles.Size();

        bool haveEye  = false;
        Vec3 eyeWorld = Vec3(0.0f);
        Vec3 lookWorld = Vec3(0.0f, 0.0f, -1.0f);
        if(Graphics::SceneRenderer* renderer = Application::Get().GetSceneRenderer())
        {
            if(Maths::Transform* view = renderer->GetViewTransform())
            {
                eyeWorld  = view->GetWorldPosition();
                lookWorld = view->GetForwardDirection();
                haveEye  = true;
            }
        }

        // A big mesh: the triangles nearest the camera, among those it is looking towards.
        if(count > kBudget)
        {
            const Vec3 eye = haveEye ? Vec3(Mat4::Inverse(transform) * Vec4(eyeWorld, 1.0f)) : (m_BoundsMin + m_BoundsMax) * 0.5f;

            // everything in a box round it (shrunk if that is still too much to look through)...
            const uint32_t kCandidates = 120000;
            uint32_t* candidates       = PushArrayNoZero(scratch.arena, uint32_t, kCandidates + 1);
            float reach                = Maths::Max(Maths::Length(m_BoundsMax - m_BoundsMin) * 0.4f, 1.0f);
            uint32_t found             = 0;
            for(int attempt = 0; attempt < 10; attempt++)
            {
                found = QueryAABB(eye - Vec3(reach), eye + Vec3(reach), candidates, kCandidates + 1);
                if(found <= kCandidates)
                    break;
                reach *= 0.7f;
            }
            found = Maths::Min(found, kCandidates);

            // ...then the nearest of those. A histogram of distances gives the cut-off without a sort.
            // (Shrinking the box alone leaves a bare floor: one detailed prop is over the budget by itself.)
            const Mat4 toLocal      = Mat4::Inverse(transform);
            const Vec3 look         = haveEye ? Vec3(toLocal * Vec4(lookWorld, 0.0f)).Normalised() : Vec3(0.0f);
            const uint32_t kBuckets = 128;
            uint32_t buckets[kBuckets] = {};
            float* distance            = PushArrayNoZero(scratch.arena, float, found + 1);
            const float span           = reach * 1.75f;
            for(uint32_t i = 0; i < found; i++)
            {
                const MeshTriangle& tri = m_Triangles[candidates[i]];
                // to its bounds, not its centre: a floor is a few huge triangles whose centres are far off
                // (per component by hand: Maths::Min / Max on a Vec3 are not component-wise)
                float gap[3];
                for(int axis = 0; axis < 3; axis++)
                {
                    const float lo = Maths::Min(tri.v0[axis], Maths::Min(tri.v1[axis], tri.v2[axis]));
                    const float hi = Maths::Max(tri.v0[axis], Maths::Max(tri.v1[axis], tri.v2[axis]));
                    gap[axis]      = Maths::Max(lo - eye[axis], Maths::Max(eye[axis] - hi, 0.0f));
                }
                distance[i] = sqrtf(gap[0] * gap[0] + gap[1] * gap[1] + gap[2] * gap[2]);
                // off to the side or behind (past ~65 degrees from where the camera points), unless it is right here:
                // it is not on screen, and a dense prop back there would use up the whole budget
                const Vec3 to = (tri.v0 + tri.v1 + tri.v2) * (1.0f / 3.0f) - eye;
                if(haveEye && distance[i] > 1.5f && Maths::Dot(to, look) < 0.42f * Maths::Length(to))
                    distance[i] = span * 2.0f;
                if(distance[i] < span)
                    buckets[Maths::Min((uint32_t)(distance[i] / span * (float)kBuckets), kBuckets - 1)]++;
            }
            uint32_t kept = 0, lastBucket = 0;
            for(; lastBucket < kBuckets && kept + buckets[lastBucket] <= kBudget; lastBucket++)
                kept += buckets[lastBucket];
            const float cutoff = span * (float)lastBucket / (float)kBuckets;

            picked = PushArrayNoZero(scratch.arena, uint32_t, kBudget + 1);
            count  = 0;
            for(uint32_t i = 0; i < found && count < kBudget; i++)
                if(distance[i] < cutoff || (lastBucket == 0 && distance[i] < span))
                    picked[count++] = candidates[i];
        }

        // Ground you can stand on in one colour, walls and ceilings in another.
        const Vec4 walkable = Vec4(0.35f, 0.9f, 0.5f, 1.0f);
        const Vec4 steep    = Vec4(0.95f, 0.55f, 0.3f, 1.0f);
        const Vec3 up       = Vec3(transform * Vec4(0.0f, 1.0f, 0.0f, 0.0f)).Normalised();

        for(uint32_t i = 0; i < count; i++)
        {
            const MeshTriangle& tri = m_Triangles[picked ? picked[i] : i];
            const Vec3 normal       = Vec3(transform * Vec4(tri.Normal, 0.0f)).Normalised();
            // pulled a little towards the eye so the render mesh under it does not fight the lines
            // (not along the normal: the collider is two-sided and half of those point into the wall)
            auto place = [&](const Vec3& local)
            {
                const Vec3 world = Vec3(transform * Vec4(local, 1.0f));
                return haveEye ? world + (eyeWorld - world) * 0.02f : world;
            };
            const Vec3 v0 = place(tri.v0);
            const Vec3 v1 = place(tri.v1);
            const Vec3 v2 = place(tri.v2);

            const Vec4& colour = Maths::Abs(Maths::Dot(normal, up)) > 0.7f ? walkable : steep;
            DebugRenderer::DrawHairLine(v0, v1, true, colour);
            DebugRenderer::DrawHairLine(v1, v2, true, colour);
            DebugRenderer::DrawHairLine(v2, v0, true, colour);
        }
        ScratchEnd(scratch);
    }

    TDArray<Vec3>& MeshCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        m_Axes.Clear();

        // Add unique face normals (limited for performance)
        uint32_t maxAxes = 32;

        for(const auto& tri : m_Triangles)
        {
            if(m_Axes.Size() >= maxAxes)
                break;

            bool isDuplicate = false;
            for(const auto& axis : m_Axes)
            {
                if(Maths::Abs(Maths::Dot(axis, tri.Normal)) > 0.999f)
                {
                    isDuplicate = true;
                    break;
                }
            }

            if(!isDuplicate)
                m_Axes.PushBack(tri.Normal);
        }

        return m_Axes;
    }

    TDArray<CollisionEdge>& MeshCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        m_Edges.Clear();

        // Return edges (limited for performance)
        uint32_t maxEdges = 64;

        for(const auto& tri : m_Triangles)
        {
            if(m_Edges.Size() >= maxEdges)
                break;

            m_Edges.PushBack(CollisionEdge(tri.v0, tri.v1));
            m_Edges.PushBack(CollisionEdge(tri.v1, tri.v2));
            m_Edges.PushBack(CollisionEdge(tri.v2, tri.v0));
        }

        return m_Edges;
    }

    void MeshCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        if(m_Triangles.Empty())
        {
            if(out_min) *out_min = Vec3(0.0f);
            if(out_max) *out_max = Vec3(0.0f);
            return;
        }

        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);

        float minProj = 1e30f;
        float maxProj = -1e30f;
        Vec3 minVert, maxVert;

        // corners of the bounds: only used to size the body's AABB
        for(int i = 0; i < 8; i++)
        {
            const Vec3 v((i & 1) ? m_BoundsMax.x : m_BoundsMin.x, (i & 2) ? m_BoundsMax.y : m_BoundsMin.y, (i & 4) ? m_BoundsMax.z : m_BoundsMin.z);
            Vec3 worldV = Vec3(transform * Vec4(v, 1.0f));
            float proj = Maths::Dot(worldV, axis);

            if(proj < minProj) { minProj = proj; minVert = worldV; }
            if(proj > maxProj) { maxProj = proj; maxVert = worldV; }
        }

        if(out_min) *out_min = minVert;
        if(out_max) *out_max = maxVert;
    }

    void MeshCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const
    {
        if(m_Triangles.Empty())
        {
            refPolygon.FaceCount = 0;
            return;
        }

        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);
        Mat3 normalTransform = Mat3(transform);

        // Find triangle most aligned with axis
        float bestDot = -1e30f;
        int bestTriIdx = 0;

        for(uint32_t i = 0; i < m_Triangles.Size(); i++)
        {
            Vec3 worldNormal = (normalTransform * m_Triangles[i].Normal).Normalised();
            float dot = Maths::Dot(worldNormal, axis);

            if(dot > bestDot)
            {
                bestDot = dot;
                bestTriIdx = (int)i;
            }
        }

        const auto& tri = m_Triangles[bestTriIdx];
        refPolygon.Faces[0] = Vec3(transform * Vec4(tri.v0, 1.0f));
        refPolygon.Faces[1] = Vec3(transform * Vec4(tri.v1, 1.0f));
        refPolygon.Faces[2] = Vec3(transform * Vec4(tri.v2, 1.0f));
        refPolygon.FaceCount = 3;
        refPolygon.Normal = (normalTransform * tri.Normal).Normalised();
    }

    // ---- .lcol ---------------------------------------------------------------------
    // Header, then TriangleCount * LColTriangle, then NodeCount * LColNode. Little endian.
    static const char kLColMagic[4]  = { 'L', 'C', 'O', 'L' };
    static const uint32_t kLColVersion = 1;

    struct LColHeader
    {
        char Magic[4];
        uint32_t Version;
        uint32_t TriangleCount;
        uint32_t NodeCount;
        float BoundsMin[3];
        float BoundsMax[3];
    };

    struct LColTriangle
    {
        float v[9]; // normals are recomputed
    };

    struct LColNode
    {
        float Min[3];
        float Max[3];
        uint32_t First;
        uint32_t Count;
    };

    void MeshCollisionShape::SetFilePath(const char* path)
    {
        m_FilePath[0] = 0;
        m_LoadFile    = true;
        if(!path)
            return;
        const size_t length = strlen(path);
        if(length >= sizeof(m_FilePath))
        {
            LWARN("MeshCollisionShape: path too long, ignored: %s", path);
            return;
        }
        memcpy(m_FilePath, path, length + 1);
    }

    bool MeshCollisionShape::SaveToFile(const String8& path) const
    {
        if(m_Triangles.Empty() || m_Nodes.Empty())
            return false;

        const uint64_t size = sizeof(LColHeader) + (uint64_t)m_Triangles.Size() * sizeof(LColTriangle) + (uint64_t)m_Nodes.Size() * sizeof(LColNode);
        if(size > 0xFFFFFFFFull)
            return false;

        Arena* arena = ArenaAlloc(size + Kilobytes(4));
        uint8_t* out = (uint8_t*)ArenaPushNoZero(arena, size);

        LColHeader header;
        memcpy(header.Magic, kLColMagic, 4);
        header.Version       = kLColVersion;
        header.TriangleCount = (uint32_t)m_Triangles.Size();
        header.NodeCount     = (uint32_t)m_Nodes.Size();
        for(int a = 0; a < 3; a++)
        {
            header.BoundsMin[a] = m_BoundsMin[a];
            header.BoundsMax[a] = m_BoundsMax[a];
        }
        memcpy(out, &header, sizeof(header));

        LColTriangle* triangles = (LColTriangle*)(out + sizeof(LColHeader));
        for(uint32_t i = 0; i < header.TriangleCount; i++)
        {
            const MeshTriangle& t = m_Triangles[i];
            const Vec3* corners[3] = { &t.v0, &t.v1, &t.v2 };
            for(int c = 0; c < 3; c++)
                for(int a = 0; a < 3; a++)
                    triangles[i].v[c * 3 + a] = (*corners[c])[a];
        }

        LColNode* nodes = (LColNode*)(triangles + header.TriangleCount);
        for(uint32_t i = 0; i < header.NodeCount; i++)
        {
            const MeshBVHNode& n = m_Nodes[i];
            for(int a = 0; a < 3; a++)
            {
                nodes[i].Min[a] = n.Min[a];
                nodes[i].Max[a] = n.Max[a];
            }
            nodes[i].First = n.First;
            nodes[i].Count = n.Count;
        }

        // the file may not exist yet, so only the resolved path is wanted, not whether it was found
        ArenaTemp scratch = ScratchBegin(nullptr, 0);
        String8 physical;
        FileSystem::Get().ResolvePhysicalPath(scratch.arena, path, &physical);
        bool ok = physical.size > 0;
        if(ok)
        {
            const String8 folder = StringUtilities::Str8PathChopLastSlash(physical);
            FileSystem::CreateFolderIfDoesntExist(PushStr8F(scratch.arena, "%.*s", (int)folder.size, (char*)folder.str));
            ok = FileSystem::WriteFile(physical, out, (uint32_t)size);
        }
        ScratchEnd(scratch);
        ArenaRelease(arena);
        return ok;
    }

    bool MeshCollisionShape::LoadFromFile(const String8& path)
    {
        const int64_t fileSize = FileSystem::Get().GetFileSizeVFS(path);
        if(fileSize < (int64_t)sizeof(LColHeader))
            return false;

        Arena* arena        = ArenaAlloc((uint64_t)fileSize + Kilobytes(4));
        const uint8_t* data = FileSystem::Get().ReadFileVFS(arena, path);
        bool ok             = data != nullptr;

        LColHeader header;
        if(ok)
        {
            memcpy(&header, data, sizeof(header));
            const uint64_t expected = sizeof(LColHeader) + (uint64_t)header.TriangleCount * sizeof(LColTriangle) + (uint64_t)header.NodeCount * sizeof(LColNode);
            ok                      = memcmp(header.Magic, kLColMagic, 4) == 0 && header.Version == kLColVersion
                && header.TriangleCount > 0 && header.NodeCount > 0 && expected == (uint64_t)fileSize;
        }

        if(ok)
        {
            const LColTriangle* triangles = (const LColTriangle*)(data + sizeof(LColHeader));
            const LColNode* nodes         = (const LColNode*)(triangles + header.TriangleCount);

            // a node pointing outside the arrays would be read out of bounds later
            for(uint32_t i = 0; i < header.NodeCount && ok; i++)
            {
                if(nodes[i].Count > 0)
                    ok = (uint64_t)nodes[i].First + nodes[i].Count <= header.TriangleCount;
                else
                    ok = nodes[i].First > i && (uint64_t)nodes[i].First + 1 < header.NodeCount;
            }

            if(ok)
            {
                m_Vertices.Clear();
                m_Triangles.Resize(header.TriangleCount);
                for(uint32_t i = 0; i < header.TriangleCount; i++)
                {
                    const float* v  = triangles[i].v;
                    MeshTriangle& t = m_Triangles[i];
                    t.v0            = Vec3(v[0], v[1], v[2]);
                    t.v1            = Vec3(v[3], v[4], v[5]);
                    t.v2            = Vec3(v[6], v[7], v[8]);
                    t.Normal        = Maths::Cross(t.v1 - t.v0, t.v2 - t.v0).Normalised();
                }

                m_Nodes.Resize(header.NodeCount);
                for(uint32_t i = 0; i < header.NodeCount; i++)
                {
                    m_Nodes[i].Min   = Vec3(nodes[i].Min[0], nodes[i].Min[1], nodes[i].Min[2]);
                    m_Nodes[i].Max   = Vec3(nodes[i].Max[0], nodes[i].Max[1], nodes[i].Max[2]);
                    m_Nodes[i].First = nodes[i].First;
                    m_Nodes[i].Count = nodes[i].Count;
                }
                m_BoundsMin = Vec3(header.BoundsMin[0], header.BoundsMin[1], header.BoundsMin[2]);
                m_BoundsMax = Vec3(header.BoundsMax[0], header.BoundsMax[1], header.BoundsMax[2]);
            }
        }

        ArenaRelease(arena);
        return ok;
    }

    bool MeshCollisionShape::RayIntersect(const Vec3& origin, const Vec3& direction, float maxDist, float& outT, Vec3& outNormal) const
    {
        bool hit    = false;
        float bestT = maxDist;

        for(const auto& tri : m_Triangles)
        {
            // Moller-Trumbore intersection
            Vec3 edge1 = tri.v1 - tri.v0;
            Vec3 edge2 = tri.v2 - tri.v0;
            Vec3 h     = Maths::Cross(direction, edge2);
            float a    = Maths::Dot(edge1, h);

            if(Maths::Abs(a) < Maths::M_EPSILON)
                continue;

            float f = 1.0f / a;
            Vec3 s  = origin - tri.v0;
            float u = f * Maths::Dot(s, h);

            if(u < 0.0f || u > 1.0f)
                continue;

            Vec3 q  = Maths::Cross(s, edge1);
            float v = f * Maths::Dot(direction, q);

            if(v < 0.0f || u + v > 1.0f)
                continue;

            float t = f * Maths::Dot(edge2, q);

            if(t > Maths::M_EPSILON && t < bestT)
            {
                bestT     = t;
                outT      = t;
                outNormal = tri.Normal;
                hit       = true;
            }
        }

        return hit;
    }
}
