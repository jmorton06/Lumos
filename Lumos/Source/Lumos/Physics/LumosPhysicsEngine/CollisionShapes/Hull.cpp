#include "Precompiled.h"
#include "Hull.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    Hull::Hull()
    {
    }

    Hull::~Hull()
    {
    }

    void Hull::AddVertex(const Maths::Vector3& v)
    {
        HullVertex new_vertex;
        new_vertex.idx = static_cast<int>(m_Vertices.size());
        new_vertex.pos = v;

        m_Vertices.push_back(new_vertex);
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

        return -1; //Not Found
    }

    int Hull::ConstructNewEdge(int parent_face_idx, int vert_start, int vert_end)
    {
        int out_idx = FindEdge(vert_start, vert_end);

        //Edge not already within the Hull,
        if(out_idx == -1)
        {
            out_idx = static_cast<int>(m_Edges.size());

            HullEdge new_edge;
            new_edge.idx = static_cast<int>(m_Edges.size());
            new_edge.vStart = vert_start;
            new_edge.vEnd = vert_end;
            m_Edges.push_back(new_edge);

            HullEdge* new_edge_ptr = &m_Edges[new_edge.idx];

            //Find Adjacent Edges
            for(int i = 0; i < new_edge.idx; ++i)
            {
                if(m_Edges[i].vStart == vert_start
                    || m_Edges[i].vStart == vert_end
                    || m_Edges[i].vEnd == vert_start
                    || m_Edges[i].vEnd == vert_end)
                {
                    m_Edges[i].adjoining_edge_ids.push_back(new_edge.idx);
                    new_edge_ptr->adjoining_edge_ids.push_back(i);
                }
            }

            //Update Contained Vertices
            m_Vertices[vert_start].enclosing_edges.push_back(new_edge.idx);
            m_Vertices[vert_end].enclosing_edges.push_back(new_edge.idx);
        }

        m_Edges[out_idx].enclosing_faces.push_back(parent_face_idx);
        return out_idx;
    }

    void Hull::AddFace(const Maths::Vector3& normal, int nVerts, const int* verts)
    {
        HullFace new_face;
        new_face.idx = (int)m_Faces.size();
        new_face.normal = normal;
        new_face.normal.Normalise();

        m_Faces.push_back(new_face);
        HullFace* new_face_ptr = &m_Faces[new_face.idx];

        //Construct all contained edges
        int p0 = nVerts - 1;
        for(int p1 = 0; p1 < nVerts; ++p1)
        {
            new_face_ptr->vert_ids.push_back(verts[p1]);
            new_face_ptr->edge_ids.push_back(ConstructNewEdge(new_face.idx, verts[p0], verts[p1]));
            p0 = p1;
        }

        //Find Adjacent Faces
        for(int i = 0; i < new_face.idx; ++i)
        {
            HullFace& cFace = m_Faces[i];
            bool found = false;
            for(size_t j = 0; found == false && j < cFace.edge_ids.size(); ++j)
            {
                for(int k = 0; found == false && k < nVerts; ++k)
                {
                    if(new_face_ptr->edge_ids[k] == cFace.edge_ids[j])
                    {
                        found = true;
                        cFace.adjoining_face_ids.push_back(new_face.idx);
                        new_face_ptr->adjoining_face_ids.push_back(i);
                    }
                }
            }
        }

        //Update Contained Vertices
        for(int i = 0; i < nVerts; ++i)
        {
            HullVertex* cVertStart = &m_Vertices[m_Edges[new_face_ptr->edge_ids[i]].vStart];
            HullVertex* cVertEnd = &m_Vertices[m_Edges[new_face_ptr->edge_ids[i]].vEnd];

            const auto foundLocStart = std::find(cVertStart->enclosing_faces.begin(), cVertStart->enclosing_faces.end(), new_face.idx);
            if(foundLocStart == cVertStart->enclosing_faces.end())
            {
                cVertStart->enclosing_faces.push_back(new_face.idx);
            }

            const auto foundLocEnd = std::find(cVertEnd->enclosing_faces.begin(), cVertEnd->enclosing_faces.end(), new_face.idx);
            if(foundLocEnd == cVertEnd->enclosing_faces.end())
            {
                cVertEnd->enclosing_faces.push_back(new_face.idx);
            }
        }
    }

    void Hull::GetMinMaxVerticesInAxis(const Maths::Vector3& local_axis, int* out_min_vert, int* out_max_vert)
    {
        LUMOS_PROFILE_FUNCTION();
        int minVertex = 0, maxVertex = 0;

        float minCorrelation = FLT_MAX, maxCorrelation = -FLT_MAX;

        for(size_t i = 0; i < m_Vertices.size(); ++i)
        {
            const float cCorrelation = Maths::Vector3::Dot(local_axis, m_Vertices[i].pos);

            if(cCorrelation > maxCorrelation)
            {
                maxCorrelation = cCorrelation;
                maxVertex = static_cast<int>(i);
            }

            if(cCorrelation <= minCorrelation)
            {
                minCorrelation = cCorrelation;
                minVertex = static_cast<int>(i);
            }
        }

        if(out_min_vert)
            *out_min_vert = minVertex;
        if(out_max_vert)
            *out_max_vert = maxVertex;
    }

    void Hull::DebugDraw(const Maths::Matrix4& transform)
    {
        //Draw all Hull Polygons
        for(HullFace& face : m_Faces)
        {
            //Render Polygon as triangle fan
            if(face.vert_ids.size() > 2)
            {
                Maths::Vector3 polygon_start = transform * m_Vertices[face.vert_ids[0]].pos;
                Maths::Vector3 polygon_last = transform * m_Vertices[face.vert_ids[1]].pos;

                for(size_t idx = 2; idx < face.vert_ids.size(); ++idx)
                {
                    Maths::Vector3 polygon_next = transform * m_Vertices[face.vert_ids[idx]].pos;

                    DebugRenderer::DrawTriangle(polygon_start, polygon_last, polygon_next, Maths::Vector4(0.9f, 0.9f, 0.9f, 0.2f));
                    polygon_last = polygon_next;
                }
            }
        }

        //Draw all Hull Edges
        for(HullEdge& edge : m_Edges)
        {
            DebugRenderer::DrawThickLine(transform * m_Vertices[edge.vStart].pos, transform * m_Vertices[edge.vEnd].pos, 0.02f, Maths::Vector4(0.7f, 0.2f, 0.7f, 1.0f));
        }
    }
}
