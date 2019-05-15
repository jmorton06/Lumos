#include "LM.h"
#include "MeshFactory.h"
#include "Maths/BoundingSphere.h"
#include "Mesh.h"
#include "Material.h"
#include "API/VertexArray.h"
#include "Maths/Maths.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace MeshFactory
    {
        Mesh* CreateQuad(float x, float y, float width, float height, std::shared_ptr<Material>  material)
        {
            struct QuadVertex
            {
                maths::Vector3 position;
                maths::Vector2 uv;
            };

            QuadVertex* data = new QuadVertex[4];

            data[0].position = maths::Vector3(x, y, 0.0f);
            data[0].uv = maths::Vector2(0.0f, 1.0f);

            data[1].position = maths::Vector3(x + width, y, 0.0f);
            data[1].uv = maths::Vector2(0, 0);

            data[2].position = maths::Vector3(x + width, y + height, 0.0f);
            data[2].uv = maths::Vector2(1, 0);

            data[3].position = maths::Vector3(x, y + height, 0.0f);
            data[3].uv = maths::Vector2(1, 1);

            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());

            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(sizeof(QuadVertex) * 4, data);

            delete[] data;

            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("position");
            layout.Push<maths::Vector2>("texCoord");
            buffer->SetLayout(layout);

            va->PushBuffer(buffer);
            uint indices[6] = { 0, 1, 2, 2, 3, 0, };
            std::shared_ptr<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 6));

            std::shared_ptr<maths::BoundingSphere> boundingBox = std::make_shared<maths::BoundingSphere>();
            for(int i = 0; i < 4; i++)
            {
                boundingBox->ExpandToFit(data[i].position);
            }
            return new Mesh(va, ib, material, boundingBox);
        }

        Mesh* CreateQuad(const maths::Vector2& position, const maths::Vector2& size, std::shared_ptr<Material>  material)
        {
            return CreateQuad(position.GetX(), position.GetY(), size.GetX(), size.GetY(), std::move(material));
        }

        Mesh* CreateQuad()
        {
            Vertex* data = new Vertex[4];

            data[0].Position = maths::Vector3(-1.0f, -1.0f, 0.0f);
            data[0].TexCoords = maths::Vector2(0.0f, 0.0f);
			data[0].Colours = Lumos::maths::Vector4(0.0f);

            data[1].Position = maths::Vector3(1.0f, -1.0f, 0.0f);
			data[1].Colours = Lumos::maths::Vector4(0.0f);
            data[1].TexCoords = maths::Vector2(1.0f, 0.0f);

            data[2].Position = maths::Vector3(1.0f, 1.0f, 0.0f);
			data[2].Colours = Lumos::maths::Vector4(0.0f);
            data[2].TexCoords = maths::Vector2(1.0f, 1.0f);

            data[3].Position = maths::Vector3(-1.0f, 1.0f, 0.0f);
			data[3].Colours = Lumos::maths::Vector4(0.0f);
            data[3].TexCoords = maths::Vector2(0.0f, 1.0f);

            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());

            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(sizeof(Vertex) * 4, data);

            delete[] data;

            graphics::BufferLayout layout;
			layout.Push<maths::Vector3>("position");
			layout.Push<maths::Vector4>("colour");
			layout.Push<maths::Vector2>("texCoord");
			layout.Push<maths::Vector3>("normal");
			layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);

            va->PushBuffer(buffer);
            uint32 indices[6] = { 0, 1, 2, 2, 3, 0, };
            std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 6));

            std::shared_ptr<Material> empty;
            std::shared_ptr<maths::BoundingSphere> boundingSphere = std::make_shared<maths::BoundingSphere>();
            for(int i = 0; i < 4; i++)
            {
                boundingSphere->ExpandToFit(data[i].Position);
            }
            return new Mesh(va, ib, empty, boundingSphere);
        }

        Mesh* CreateCube(float size, std::shared_ptr<Material> material)
        {

            //    v6----- v5
            //   /|      /|
            //  v1------v0|
            //  | |     | |
            //  | |v7---|-|v4
            //  |/      |/
            //  v2------v3

            Vertex* data = new Vertex[24];

            data[0].Position  = maths::Vector3(1.0f,  1.0f,  1.0f);
            data[0].Colours   = maths::Vector4(0.0f);
            data[0].TexCoords = maths::Vector2(1.0f,  1.0f);
            data[0].Normal    = maths::Vector3(0.0f,  0.0f,  1.0f);

            data[1].Position  = maths::Vector3(-1.0f,  1.0f,  1.0f);
            data[1].Colours   = maths::Vector4(0.0f);
            data[1].TexCoords = maths::Vector2(1.0f,  0.0f);
            data[1].Normal    = maths::Vector3(0.0f,  0.0f,  1.0f);

            data[2].Position  = maths::Vector3(-1.0f,  -1.0f,  1.0f);
            data[2].Colours   = maths::Vector4(0.0f);
            data[2].TexCoords = maths::Vector2(0.0f,  0.0f);
            data[2].Normal    = maths::Vector3(0.0f,  0.0f,  1.0f);

            data[3].Position  = maths::Vector3(1.0f,  -1.0f,  1.0f);
            data[3].Colours   = maths::Vector4(0.0f);
            data[3].TexCoords = maths::Vector2(1.0f,  0.0f);
            data[3].Normal    = maths::Vector3(0.0f,  0.0f,  1.0f);

            data[4].Position  = maths::Vector3(1.0f,  1.0f,  1.0f);
            data[4].Colours   = maths::Vector4(0.0f);
            data[4].TexCoords = maths::Vector2(1.0f,  1.0f);
            data[4].Normal    = maths::Vector3(1.0f,  0.0f,  0.0f);

            data[5].Position  = maths::Vector3(1.0f,  -1.0f,  1.0f);
            data[5].Colours   = maths::Vector4(0.0f);
            data[5].TexCoords = maths::Vector2(1.0f,  0.0f);
            data[5].Normal    = maths::Vector3(1.0f,  0.0f,  0.0f);

            data[6].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[6].Colours   = maths::Vector4(0.0f);
            data[6].TexCoords = maths::Vector2(0.0f,  0.0f);
            data[6].Normal    = maths::Vector3(1.0f,  0.0f,  0.0f);

            data[7].Position  = maths::Vector3(1.0f,  1.0f,  -1.0f);
            data[7].Colours   = maths::Vector4(0.0f);
            data[7].TexCoords = maths::Vector2(0.0f,  1.0f);
            data[7].Normal    = maths::Vector3(1.0f,  0.0f,  0.0f);

            data[8].Position  = maths::Vector3(1.0f,  1.0f,  1.0f);
            data[8].Colours   = maths::Vector4(0.0f);
            data[8].Normal    = maths::Vector3(0.0f,  1.0f,  0.0f);

            data[9].Position  = maths::Vector3(1.0f, 1.0f,  -1.0f);
            data[9].Colours   = maths::Vector4(0.0f);
            data[9].Normal    = maths::Vector3(0.0f,  1.0f,  0.0f);

            data[10].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[10].Colours   = maths::Vector4(0.0f);
            data[10].TexCoords = maths::Vector2(0.0f,  1.0f);
            data[10].Normal    = maths::Vector3(0.0f,  1.0f,  0.0f);

            data[11].Position  = maths::Vector3(-1.0f,  1.0f,  1.0f);
            data[11].Colours   = maths::Vector4(0.0f);
            data[11].Normal    = maths::Vector3(0.0f,  1.0f,  0.0f);

            data[12].Position  = maths::Vector3(-1.0f,  1.0f,  1.0f);
            data[12].Colours   = maths::Vector4(0.0f);
            data[12].Normal    = maths::Vector3(-1.0f,  0.0f,  0.0f);

            data[13].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[13].Colours   = maths::Vector4(0.0f);
            data[13].Normal    = maths::Vector3(-1.0f,  0.0f,  0.0f);

            data[14].Position  = maths::Vector3(-1.0f,  -1.0f,  -1.0f);
            data[14].Colours   = maths::Vector4(0.0f);
            data[14].Normal    = maths::Vector3(-1.0f,  0.0f,  0.0f);

            data[15].Position  = maths::Vector3(-1.0f,  -1.0f,  1.0f);
            data[15].Colours   = maths::Vector4(0.0f);
            data[15].Normal    = maths::Vector3(-1.0f,  0.0f,  0.0f);

            data[16].Position  = maths::Vector3(-1.0f,  -1.0f,  -1.0f);
            data[16].Colours   = maths::Vector4(0.0f);
            data[16].Normal    = maths::Vector3(0.0f,  -1.0f,  0.0f);

            data[17].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[17].Colours   = maths::Vector4(0.0f);
            data[17].Normal    = maths::Vector3(0.0f,  -1.0f,  0.0f);

            data[18].Position  = maths::Vector3(1.0f,  -1.0f,  1.0f);
            data[18].Colours   = maths::Vector4(0.0f);
            data[18].Normal    = maths::Vector3(0.0f,  -1.0f,  0.0f);

            data[19].Position  = maths::Vector3(-1.0f,  -1.0f,  1.0f);
            data[19].Colours   = maths::Vector4(0.0f);
            data[19].Normal    = maths::Vector3(0.0f,  -1.0f,  0.0f);

            data[20].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[20].Colours   = maths::Vector4(0.0f);
            data[20].Normal    = maths::Vector3(0.0f,  0.0f,  -1.0f);

            data[21].Position  = maths::Vector3(-1.0f,  -1.0f,  -1.0f);
            data[21].Colours   = maths::Vector4(0.0f);
            data[21].Normal    = maths::Vector3(0.0f,  0.0f,  -1.0f);

            data[22].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[22].Colours   = maths::Vector4(0.0f);
            data[22].Normal    = maths::Vector3(0.0f,  0.0f,  -1.0f);

            data[23].Position  = maths::Vector3(1.0f,  1.0f,  -1.0f);
            data[23].Colours   = maths::Vector4(0.0f);
            data[23].Normal    = maths::Vector3(0.0f,  0.0f,  -1.0f);

            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());

            for(int i = 0; i < 6; i++)
            {
                data[i*4 + 0].TexCoords = maths::Vector2(0.0f,0.0f);
                data[i*4 + 1].TexCoords = maths::Vector2(1.0f,0.0f);
                data[i*4 + 2].TexCoords = maths::Vector2(1.0f,1.0f);
                data[i*4 + 3].TexCoords = maths::Vector2(0.0f,1.0f);

            }

            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(24 * sizeof(Vertex), data);

            delete[] data;

            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("position");
            layout.Push<maths::Vector4>("colour");
            layout.Push<maths::Vector2>("texCoord");
            layout.Push<maths::Vector3>("normal");
            layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);

            va->Bind();
            va->PushBuffer(buffer);

            uint indices[36]
            {
                0,1,2,
                0,2,3,
                4,5,6,
                4,6,7,
                8,9,10,
                8,10,11,
                12,13,14,
                12,14,15,
                16,17,18,
                16,18,19,
                20,21,22,
                20,22,23
            };

            std::shared_ptr<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 36));
            std::shared_ptr<maths::BoundingSphere> boundingSphere = std::make_shared<maths::BoundingSphere>();
            for(int i = 0; i < 4; i++)
            {
                boundingSphere->ExpandToFit(data[i].Position);
            }
            return new Mesh(va, ib, material, boundingSphere);
        }
        
        Mesh* CreatePyramid(float size, std::shared_ptr<Material> material)
        {
            Vertex* data = new Vertex[18];
            
            data[0].Position  = maths::Vector3(1.0f,  1.0f,  -1.0f);
            data[0].Colours   = maths::Vector4(0.0f);
            data[0].TexCoords = maths::Vector2(0.24f,0.20f);
            data[0].Normal    = maths::Vector3(0.0f,  0.8948f,  0.4464f);
            
            data[1].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[1].Colours   = maths::Vector4(0.0f);
            data[1].TexCoords = maths::Vector2(0.24f,0.81f);
            data[1].Normal    = maths::Vector3(0.0f,  0.8948f,  0.4464f);
            
            data[2].Position  = maths::Vector3(0.0f,  0.0f,  1.0f);
            data[2].Colours   = maths::Vector4(0.0f);
            data[2].TexCoords = maths::Vector2(0.95f,0.50f);
            data[2].Normal    = maths::Vector3(0.0f,  0.8948f,  0.4464f);
            
            data[3].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[3].Colours   = maths::Vector4(0.0f);
            data[3].TexCoords = maths::Vector2(0.24f,0.21f);
            data[3].Normal    = maths::Vector3(-0.8948f,  0.0f,  0.4464f);
            
            data[4].Position  = maths::Vector3(-1.0f,  -1.0f,  -1.0f);
            data[4].Colours   = maths::Vector4(0.0f);
            data[4].TexCoords = maths::Vector2(0.24f, 0.81f);
            data[4].Normal    = maths::Vector3(-0.8948f,  0.0f,  0.4464f);
            
            data[5].Position  = maths::Vector3(0.0f,  0.0f,  1.0f);
            data[5].Colours   = maths::Vector4(0.0f);
            data[5].TexCoords = maths::Vector2(0.95f ,0.50f);
            data[5].Normal    = maths::Vector3(-0.8948f,  0.0f,  0.4464f);
            
            data[6].Position  = maths::Vector3(1.0f,  1.0f,  -1.0f);
            data[6].Colours   = maths::Vector4(0.0f);
            data[6].TexCoords = maths::Vector2(0.24f,0.81f);
            data[6].Normal    = maths::Vector3(0.8948f,  0.0f,  0.4475f);
            
            data[7].Position  = maths::Vector3(0.0f,  0.0f,  1.0f);
            data[7].Colours   = maths::Vector4(0.0f);
            data[7].TexCoords = maths::Vector2(0.95f,0.50f);
            data[7].Normal    = maths::Vector3(0.8948f,  0.0f,  0.4475f);
            
            data[8].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[8].Colours   = maths::Vector4(0.0f);
            data[8].TexCoords = maths::Vector2(0.24f,0.21f);
            data[8].Normal    = maths::Vector3(0.8948f,  0.0f,  0.4475f);
            
            data[9].Position  = maths::Vector3(-1.0f, -1.0f,  -1.0f);
            data[9].Colours   = maths::Vector4(0.0f);
            data[9].TexCoords = maths::Vector2(0.24f,0.21f);
            data[9].Normal    = maths::Vector3(0.0f,  -0.8948f,  0.448f);
            
            data[10].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[10].Colours   = maths::Vector4(0.0f);
            data[10].TexCoords = maths::Vector2(0.24f,0.81f);
            data[10].Normal    = maths::Vector3(0.0f,  -0.8948f,  0.448f);
            
            data[11].Position  = maths::Vector3(0.0f,  0.0f,  1.0f);
            data[11].Colours   = maths::Vector4(0.0f);
            data[11].TexCoords = maths::Vector2(0.95f,0.50f);
            data[11].Normal    = maths::Vector3(0.0f,  -0.8948f,  0.448f);
            
            data[12].Position  = maths::Vector3(-1.0f,  1.0f,  -1.0f);
            data[12].Colours   = maths::Vector4(0.0f);
            data[12].TexCoords = maths::Vector2(0.0f, 0.0f);
            data[12].Normal    = maths::Vector3(0.0f, 0.0f, -1.0f);
            
            data[13].Position  = maths::Vector3(1.0f,  1.0f,  -1.0f);
            data[13].Colours   = maths::Vector4(0.0f);
            data[13].TexCoords = maths::Vector2(0.0f, 1.0f);
            data[13].Normal    = maths::Vector3(0.0f, 0.0f,  -1.0f);
            
            data[14].Position  = maths::Vector3(1.0f,  -1.0f,  -1.0f);
            data[14].Colours   = maths::Vector4(0.0f);
            data[14].TexCoords = maths::Vector2(1.0f, 1.0f);
            data[14].Normal    = maths::Vector3(0.0f, 0.0f,  -1.0f);
            
            data[15].Position  = maths::Vector3(-1.0f,  -1.0f,  -1.0f);
            data[15].Colours   = maths::Vector4(0.0f);
            data[15].TexCoords = maths::Vector2(0.96f, 0.0f);
            data[15].Normal    = maths::Vector3(0.0f,  0.0f,  -1.0f);
            
            data[16].Position  = maths::Vector3(0.0f,  0.0f,  0.0f);
            data[16].Colours   = maths::Vector4(0.0f);
            data[16].TexCoords = maths::Vector2(0.0f,  0.0f);
            data[16].Normal    = maths::Vector3(0.0f,  0.0f,  0.0f);
            
            data[17].Position  = maths::Vector3(0.0f,  0.0f,  0.0f);
            data[17].Colours   = maths::Vector4(0.0f);
            data[17].TexCoords = maths::Vector2(0.0f,  0.0f);
            data[17].Normal    = maths::Vector3(0.0f,  0.0f,  0.0f);
            
            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());
            
            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(18 * sizeof(Vertex), data);
            
            delete[] data;
            
            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("position");
            layout.Push<maths::Vector4>("colour");
            layout.Push<maths::Vector2>("texCoord");
            layout.Push<maths::Vector3>("normal");
            layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);
            
            va->Bind();
            va->PushBuffer(buffer);
            
            uint indices[18]
            {
                0,1,2,
                3,4,5,
                6,7,8,
                9,10,11,
                12,13,14,
                15,12,14
            };
            
            std::shared_ptr<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 18));
            std::shared_ptr<maths::BoundingSphere> boundingSphere = std::make_shared<maths::BoundingSphere>();
            for(int i = 0; i < 4; i++)
            {
                boundingSphere->ExpandToFit(data[i].Position);
            }
            return new Mesh(va, ib, material, boundingSphere);
        }
        
        Mesh* CreateSphere(uint xSegments, uint ySegments, std::shared_ptr<Material> material)
        {
            auto data = std::vector<Vertex>();
            
            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());
            
            float sectorCount = xSegments;
            float stackCount = ySegments;
            float sectorStep = 2 * maths::PI / sectorCount;
            float stackStep = maths::PI / stackCount;
            float sectorAngle, stackAngle;
            float radius = 1.0f;
            float x, y, z, xy;
            float s, t;
            
            for(int i = 0; i <= stackCount; ++i)
            {
                stackAngle = maths::PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
                xy = radius * cos(stackAngle);             // r * cos(u)
                z = radius * sin(stackAngle);              // r * sin(u)
                
                // add (sectorCount+1) vertices per stack
                // the first and last vertices have same position and normal, but different tex coords
                for(int j = 0; j <= sectorCount; ++j)
                {
                    sectorAngle = j * sectorStep;           // starting from 0 to 2pi
                    
                    // vertex position (x, y, z)
                    x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                    y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
                    
                    // vertex tex coord (s, t) range between [0, 1]
                    s = (float)j / sectorCount;
                    t = (float)i / stackCount;
                    
                    Vertex vertex;
                    vertex.Position = maths::Vector3(x,y,z);
                    vertex.TexCoords = maths::Vector2(s,t);
                    vertex.Normal = maths::Vector3(x,y,z).Normal();
                    
                    data.emplace_back(vertex);
                }
            }
        
            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(int(data.size()) * sizeof(Vertex), data.data());
            
            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("position");
            layout.Push<maths::Vector4>("colour");
            layout.Push<maths::Vector2>("texCoord");
            layout.Push<maths::Vector3>("normal");
            layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);
            
            va->Bind();
            va->PushBuffer(buffer);
            
            std::vector<uint> indices;
            uint k1, k2;
            for(uint i = 0; i < stackCount; ++i)
            {
                k1 = i * (sectorCount + 1);     // beginning of current stack
                k2 = k1 + sectorCount + 1;      // beginning of next stack
                
                for(uint j = 0; j < sectorCount; ++j, ++k1, ++k2)
                {
                    // 2 triangles per sector excluding first and last stacks
                    // k1 => k2 => k1+1
                    if(i != 0)
                    {
                        indices.push_back(k1);
                        indices.push_back(k2);
                        indices.push_back(k1 + 1);
                    }
                    
                    // k1+1 => k2 => k2+1
                    if(i != (stackCount-1))
                    {
                        indices.push_back(k1 + 1);
                        indices.push_back(k2);
                        indices.push_back(k2 + 1);
                    }
                }
            }
            
            std::shared_ptr<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices.data(), static_cast<uint>(indices.size())));
            std::shared_ptr<maths::BoundingSphere> boundingSphere = std::make_shared<maths::BoundingSphere>();

            return new Mesh(va, ib, material, boundingSphere);
        }

        Mesh* CreatePlane(float width, float height, const maths::Vector3& normal, std::shared_ptr<Material> material)
        {
            maths::Vector3 vec = normal * 90.0f;
            maths::Matrix4 rotation = maths::Matrix4::Rotation(vec.z, maths::Vector3(1.0f, 0.0f, 0.0f)) * maths::Matrix4::Rotation(vec.y, maths::Vector3(0.0f, 1.0f, 0.0f)) * maths::Matrix4::Rotation(vec.x, maths::Vector3(0.0f, 0.0f, 1.0f));

            Vertex data[4];
            memset(data, 0, 4 * sizeof(Vertex));

            data[0].Position  = rotation * maths::Vector3(-width / 2.0f, 0.0f, -height / 2.0f);
            data[0].Normal    = normal;
            data[0].TexCoords = maths::Vector2(0.0f, 0.0f);
            data[0].Tangent   = maths::Matrix4::Rotation(90.0f, maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

            data[1].Position  = rotation * maths::Vector3(-width / 2.0f, 0.0f,  height / 2.0f);
            data[1].Normal    = normal;
            data[1].TexCoords = maths::Vector2(0.0f, 1.0f);
            data[1].Tangent   = maths::Matrix4::Rotation(90.0f, maths::Vector3(0, 0, 1)) * normal;

            data[2].Position  = rotation * maths::Vector3( width / 2.0f, 0.0f,  height / 2.0f);
            data[2].Normal    = normal;
            data[2].TexCoords = maths::Vector2(1.0f, 1.0f);
            data[2].Tangent   = maths::Matrix4::Rotation(90.0f, maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

            data[3].Position  = rotation * maths::Vector3( width / 2.0f, 0.0f, -height / 2.0f);
            data[3].Normal    = normal;
            data[3].TexCoords = maths::Vector2(1.0f, 0.0f);
            data[3].Tangent   = maths::Matrix4::Rotation(90.0f, maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

            VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
            buffer->SetData(8 * sizeof(Vertex), data);

            //delete[] data;

            graphics::BufferLayout layout;
            layout.Push<maths::Vector3>("postion");
            layout.Push<maths::Vector4>("colours");
            layout.Push<maths::Vector2>("texCoord");
            layout.Push<maths::Vector3>("normal");
            layout.Push<maths::Vector3>("tangent");
            buffer->SetLayout(layout);

            std::shared_ptr<VertexArray> va;
            va.reset(VertexArray::Create());
            va->PushBuffer(buffer);

            uint indices[6]
            {
                    0, 1, 2,
                    2, 3, 0
            };

            std::shared_ptr<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 6));
            std::shared_ptr<maths::BoundingSphere> boundingBox = std::make_shared<maths::BoundingSphere>();
            for(int i = 0; i < 4; i++)
            {
                boundingBox->ExpandToFit(data[i].Position);
            }
            return new Mesh(va, ib, material, boundingBox);
        }
    }
}
