#include "Precompiled.h"
#include "Hull.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/Vector4.h"
#include "Maths/MathsUtilities.h"
#include "Core/Algorithms/Find.h"

namespace Lumos
{

    Hull::Hull()
    {
    }

    Hull::~Hull()
    {
    }

    void Hull::AddVertex(const Vec3& v)
    {
        HullVertex new_vertex;
        new_vertex.idx = static_cast<int>(m_Vertices.Size());
        new_vertex.pos = v;

        m_Vertices.PushBack(new_vertex);
    }

    int Hull::FindEdge(int v0_idx, int v1_idx)
    {
        for(const HullEdge& edge : m_Edges)
        {
            if((edge.vStart == v0_idx && edge.vEnd == v1_idx)
               || (edge.vStart == v1_idx && edge.vEnd == v0_idx))
            {
                return edge.idx;
            }
        }

        return -1; // Not Found
    }

    int Hull::ConstructNewEdge(int parent_face_idx, int vert_start, int vert_end)
    {
        int out_idx = FindEdge(vert_start, vert_end);

        // Edge not already within the Hull,
        if(out_idx == -1)
        {
            out_idx = static_cast<int>(m_Edges.Size());

            HullEdge new_edge;
            new_edge.idx    = static_cast<int>(m_Edges.Size());
            new_edge.vStart = vert_start;
            new_edge.vEnd   = vert_end;
            m_Edges.PushBack(new_edge);

            HullEdge* new_edge_ptr = &m_Edges[new_edge.idx];

            // Find Adjacent Edges
            for(int i = 0; i < new_edge.idx; ++i)
            {
                if(m_Edges[i].vStart == vert_start
                   || m_Edges[i].vStart == vert_end
                   || m_Edges[i].vEnd == vert_start
                   || m_Edges[i].vEnd == vert_end)
                {
                    m_Edges[i].adjoining_edge_ids.PushBack(new_edge.idx);
                    new_edge_ptr->adjoining_edge_ids.PushBack(i);
                }
            }

            // Update Contained Vertices
            m_Vertices[vert_start].enclosing_edges.PushBack(new_edge.idx);
            m_Vertices[vert_end].enclosing_edges.PushBack(new_edge.idx);
        }

        m_Edges[out_idx].enclosing_faces.PushBack(parent_face_idx);
        return out_idx;
    }

    void Hull::AddFace(const Vec3& normal, int nVerts, const int* verts)
    {
        HullFace new_face;
        new_face.idx    = (int)m_Faces.Size();
        new_face.normal = normal;
        new_face.normal.Normalise();

        m_Faces.PushBack(new_face);
        HullFace* new_face_ptr = &m_Faces[new_face.idx];

        // Construct all contained edges
        int p0 = nVerts - 1;
        for(int p1 = 0; p1 < nVerts; ++p1)
        {
            new_face_ptr->vert_ids.PushBack(verts[p1]);
            new_face_ptr->edge_ids.PushBack(ConstructNewEdge(new_face.idx, verts[p0], verts[p1]));
            p0 = p1;
        }

        // Find Adjacent Faces
        for(int i = 0; i < new_face.idx; ++i)
        {
            HullFace& cFace = m_Faces[i];
            bool found      = false;
            for(size_t j = 0; found == false && j < cFace.edge_ids.Size(); ++j)
            {
                for(int k = 0; found == false && k < nVerts; ++k)
                {
                    if(new_face_ptr->edge_ids[k] == cFace.edge_ids[j])
                    {
                        found = true;
                        cFace.adjoining_face_ids.PushBack(new_face.idx);
                        new_face_ptr->adjoining_face_ids.PushBack(i);
                    }
                }
            }
        }

        // Update Contained Vertices
        for(int i = 0; i < nVerts; ++i)
        {
            HullVertex* cVertStart = &m_Vertices[m_Edges[new_face_ptr->edge_ids[i]].vStart];
            HullVertex* cVertEnd   = &m_Vertices[m_Edges[new_face_ptr->edge_ids[i]].vEnd];

            const auto foundLocStart = Algorithms::FindIf(cVertStart->enclosing_faces.begin(), cVertStart->enclosing_faces.end(), new_face.idx);
            if(foundLocStart == cVertStart->enclosing_faces.end())
            {
                cVertStart->enclosing_faces.PushBack(new_face.idx);
            }

            const auto foundLocEnd = Algorithms::FindIf(cVertEnd->enclosing_faces.begin(), cVertEnd->enclosing_faces.end(), new_face.idx);
            if(foundLocEnd == cVertEnd->enclosing_faces.end())
            {
                cVertEnd->enclosing_faces.PushBack(new_face.idx);
            }
        }
    }

    void Hull::GetMinMaxVerticesInAxis(const Vec3& local_axis, int* out_min_vert, int* out_max_vert)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        int minVertex = 0, maxVertex = 0;

        float minCorrelation = FLT_MAX, maxCorrelation = -FLT_MAX;

        for(size_t i = 0; i < m_Vertices.Size(); ++i)
        {
            const float cCorrelation = Maths::Dot(local_axis, m_Vertices[i].pos);

            if(cCorrelation > maxCorrelation)
            {
                maxCorrelation = cCorrelation;
                maxVertex      = int(i);
            }

            if(cCorrelation <= minCorrelation)
            {
                minCorrelation = cCorrelation;
                minVertex      = int(i);
            }
        }

        if(out_min_vert)
            *out_min_vert = minVertex;
        if(out_max_vert)
            *out_max_vert = maxVertex;
    }

    void Hull::DebugDraw(const Mat4& transform)
    {
        // Draw all Hull Polygons
        for(HullFace& face : m_Faces)
        {
            // Render Polygon as triangle fan
            if(face.vert_ids.Size() > 2)
            {
                Vec3 polygon_start = transform * Vec4(m_Vertices[face.vert_ids[0]].pos, 1.0f);
                Vec3 polygon_last  = transform * Vec4(m_Vertices[face.vert_ids[1]].pos, 1.0f);

                for(size_t idx = 2; idx < face.vert_ids.Size(); ++idx)
                {
                    Vec3 polygon_next = transform * Vec4(m_Vertices[face.vert_ids[idx]].pos, 1.0f);

                    DebugRenderer::DrawTriangle(polygon_start, polygon_last, polygon_next, true, Vec4(0.9f, 0.9f, 0.9f, 0.2f));
                    polygon_last = polygon_next;
                }
            }

            DebugRenderer::DrawThickLine(transform * Vec4(face.normal, 1.0f), transform * Vec4(face.normal * 2.0f, 1.0f), 0.02f, true, Vec4(0.7f, 0.2f, 0.7f, 1.0f));
        }

        // Draw all Hull Edges
        for(HullEdge& edge : m_Edges)
        {
            DebugRenderer::DrawThickLine(transform * Vec4(m_Vertices[edge.vStart].pos, 1.0f), transform * Vec4(m_Vertices[edge.vEnd].pos, 1.0f), 0.02f, true, Vec4(0.7f, 0.2f, 0.7f, 1.0f));
        }
    }

    const int BoundingBoxHull::FAR_FACE[]    = { 0, 1, 2, 3 };
    const int BoundingBoxHull::NEAR_FACE[]   = { 7, 6, 5, 4 };
    const int BoundingBoxHull::TOP_FACE[]    = { 5, 6, 2, 1 };
    const int BoundingBoxHull::BOTTOM_FACE[] = { 0, 3, 7, 4 };
    const int BoundingBoxHull::RIGHT_FACE[]  = { 6, 7, 3, 2 };
    const int BoundingBoxHull::LEFT_FACE[]   = { 4, 5, 1, 0 };

    BoundingBoxHull::BoundingBoxHull()
    {
        // Vertices
        AddVertex(Vec3(-1.0f, -1.0f, -1.0f)); // 0 lll
        AddVertex(Vec3(-1.0f, 1.0f, -1.0f));  // 1 lul
        AddVertex(Vec3(1.0f, 1.0f, -1.0f));   // 2 uul
        AddVertex(Vec3(1.0f, -1.0f, -1.0f));  // 3 ull
        AddVertex(Vec3(-1.0f, -1.0f, 1.0f));  // 4 llu
        AddVertex(Vec3(-1.0f, 1.0f, 1.0f));   // 5 luu
        AddVertex(Vec3(1.0f, 1.0f, 1.0f));    // 6 uuu
        AddVertex(Vec3(1.0f, -1.0f, 1.0f));   // 7 ulu

        // Faces
        AddFace(Vec3(0.0f, 0.0f, -1.0f), 4, FAR_FACE);
        AddFace(Vec3(0.0f, 0.0f, 1.0f), 4, NEAR_FACE);
        AddFace(Vec3(0.0f, 1.0f, 0.0f), 4, TOP_FACE);
        AddFace(Vec3(0.0f, -1.0f, 0.0f), 4, BOTTOM_FACE);
        AddFace(Vec3(1.0f, 0.0f, 0.0f), 4, RIGHT_FACE);
        AddFace(Vec3(-1.0f, 0.0f, 0.0f), 4, LEFT_FACE);
    }

    BoundingBoxHull::~BoundingBoxHull()
    {
    }

    void BoundingBoxHull::UpdateHull()
    {
        if(m_Vertices.Size() != 8)
            return;

        // Lower
        m_Vertices[0].pos.x = m_Min.x;
        m_Vertices[0].pos.y = m_Min.y;
        m_Vertices[0].pos.z = m_Min.z;

        m_Vertices[1].pos.x = m_Min.x;
        m_Vertices[1].pos.z = m_Min.z;

        m_Vertices[2].pos.z = m_Min.z;

        m_Vertices[3].pos.y = m_Min.y;
        m_Vertices[3].pos.z = m_Min.z;

        m_Vertices[4].pos.x = m_Min.x;
        m_Vertices[4].pos.y = m_Min.y;

        m_Vertices[5].pos.x = m_Min.x;

        m_Vertices[7].pos.y = m_Min.y;

        // Upper
        m_Vertices[1].pos.y = m_Max.y;

        m_Vertices[2].pos.x = m_Max.x;
        m_Vertices[2].pos.y = m_Max.y;

        m_Vertices[3].pos.x = m_Max.x;

        m_Vertices[4].pos.z = m_Max.z;

        m_Vertices[5].pos.y = m_Max.y;
        m_Vertices[5].pos.z = m_Max.z;

        m_Vertices[6].pos.x = m_Max.x;
        m_Vertices[6].pos.y = m_Max.y;
        m_Vertices[6].pos.z = m_Max.z;

        m_Vertices[7].pos.x = m_Max.x;
        m_Vertices[7].pos.z = m_Max.z;
    }

}
