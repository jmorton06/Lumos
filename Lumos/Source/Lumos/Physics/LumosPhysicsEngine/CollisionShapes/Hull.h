#pragma once

#include "Maths/Vector3.h"
#include "Maths/Matrix4.h"
#include "Maths/BoundingBox.h"
#include "Core/DataStructures/TDArray.h"

struct HullEdge;
struct HullFace;

namespace Lumos
{

    struct LUMOS_EXPORT HullVertex
    {
        int idx = 0;
        Vec3 pos;
        TDArray<int> enclosing_edges;
        TDArray<int> enclosing_faces;
    };

    struct LUMOS_EXPORT HullEdge
    {
        int idx    = 0;
        int vStart = 0, vEnd = 0;
        TDArray<int> adjoining_edge_ids;
        TDArray<int> enclosing_faces;
    };

    struct LUMOS_EXPORT HullFace
    {
        int idx = 0;
        Vec3 normal;
        TDArray<int> vert_ids;
        TDArray<int> edge_ids;
        TDArray<int> adjoining_face_ids;
    };

    class LUMOS_EXPORT Hull
    {
    public:
        Hull();
        ~Hull();

        void AddVertex(const Vec3& v);

        void AddFace(const Vec3& normal, int nVerts, const int* verts);
        void AddFace(const Vec3& normal, const TDArray<int>& vert_ids) { AddFace(normal, static_cast<int>(vert_ids.Size()), &vert_ids[0]); }

        int FindEdge(int v0_idx, int v1_idx);

        const HullVertex& GetVertex(int idx) { return m_Vertices[idx]; }
        const HullEdge& GetEdge(int idx) { return m_Edges[idx]; }
        const HullFace& GetFace(int idx) { return m_Faces[idx]; }

        size_t GetNumVertices() const { return m_Vertices.Size(); }
        size_t GetNumEdges() const { return m_Edges.Size(); }
        size_t GetNumFaces() const { return m_Faces.Size(); }

        void GetMinMaxVerticesInAxis(const Vec3& local_axis, int* out_min_vert, int* out_max_vert);

        void DebugDraw(const Mat4& transform);

    protected:
        int ConstructNewEdge(int parent_face_idx, int vert_start, int vert_end); // Called by AddFace

    protected:
        TDArray<HullVertex> m_Vertices;
        TDArray<HullEdge> m_Edges;
        TDArray<HullFace> m_Faces;
    };

    class BoundingBoxHull : public Maths::BoundingBox, public Hull
    {
    public:
        static const int FAR_FACE[];
        static const int NEAR_FACE[];
        static const int TOP_FACE[];
        static const int BOTTOM_FACE[];
        static const int RIGHT_FACE[];
        static const int LEFT_FACE[];

    public:
        BoundingBoxHull();
        virtual ~BoundingBoxHull();

        void UpdateHull();
    };

}
