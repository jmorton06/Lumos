#include "Precompiled.h"
#include "MeshFactory.h"
#include "Mesh.h"
#include "Material.h"
#include "Maths/Maths.h"
#include "Terrain.h"

#include "Graphics/RHI/GraphicsContext.h"

namespace Lumos
{
    namespace Graphics
    {
        Mesh* CreateQuad(float x, float y, float width, float height)
        {
            LUMOS_PROFILE_FUNCTION();

            Vertex* data = new Vertex[4];

            data[0].Position = Maths::Vector3(x, y, 0.0f);
            data[0].TexCoords = Maths::Vector2(0.0f, 1.0f);

            data[1].Position = Maths::Vector3(x + width, y, 0.0f);
            data[1].TexCoords = Maths::Vector2(0, 0);

            data[2].Position = Maths::Vector3(x + width, y + height, 0.0f);
            data[2].TexCoords = Maths::Vector2(1, 0);

            data[3].Position = Maths::Vector3(x, y + height, 0.0f);
            data[3].TexCoords = Maths::Vector2(1, 1);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(sizeof(Vertex) * 4, data);

            delete[] data;

            uint32_t indices[6] = {
                0,
                1,
                2,
                2,
                3,
                0,
            };
            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 6));

            SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 4; i++)
            {
                boundingBox->Merge(data[i].Position);
            }
            return new Mesh(vb, ib, boundingBox);
        }

        Mesh* CreateQuad(const Maths::Vector2& position, const Maths::Vector2& size)
        {
            return CreateQuad(position.x, position.y, size.x, size.y);
        }

        Mesh* CreateQuad()
        {
            LUMOS_PROFILE_FUNCTION();
            Vertex* data = new Vertex[4];

            data[0].Position = Maths::Vector3(-1.0f, -1.0f, 0.0f);
            data[0].TexCoords = Maths::Vector2(0.0f, 0.0f);
            data[0].Colours = Lumos::Maths::Vector4(0.0f);

            data[1].Position = Maths::Vector3(1.0f, -1.0f, 0.0f);
            data[1].Colours = Lumos::Maths::Vector4(0.0f);
            data[1].TexCoords = Maths::Vector2(1.0f, 0.0f);

            data[2].Position = Maths::Vector3(1.0f, 1.0f, 0.0f);
            data[2].Colours = Lumos::Maths::Vector4(0.0f);
            data[2].TexCoords = Maths::Vector2(1.0f, 1.0f);

            data[3].Position = Maths::Vector3(-1.0f, 1.0f, 0.0f);
            data[3].Colours = Lumos::Maths::Vector4(0.0f);
            data[3].TexCoords = Maths::Vector2(0.0f, 1.0f);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(sizeof(Vertex) * 4, data);

            SharedRef<Maths::BoundingBox> BoundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 4; i++)
            {
                BoundingBox->Merge(data[i].Position);
            }

            delete[] data;

            uint32_t indices[6] = {
                0,
                1,
                2,
                2,
                3,
                0,
            };
            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 6));

            return new Mesh(vb, ib, BoundingBox);
        }

        Mesh* CreateScreenQuad()
        {
            LUMOS_PROFILE_FUNCTION();
            Vertex* data = new Vertex[4];

            data[0].Position = Maths::Vector3(-1.0f, -1.0f, 0.0f);
            data[0].TexCoords = Maths::Vector2(0.0f, 0.0f);

            data[1].Position = Maths::Vector3(1.0f, -1.0f, 0.0f);
            data[1].TexCoords = Maths::Vector2(1.0f, 0.0f);

            data[2].Position = Maths::Vector3(1.0f, 1.0f, 0.0f);
            data[2].TexCoords = Maths::Vector2(1.0f, 1.0f);

            data[3].Position = Maths::Vector3(-1.0f, 1.0f, 0.0f);
            data[3].TexCoords = Maths::Vector2(0.0f, 1.0f);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(sizeof(Vertex) * 4, data);

            SharedRef<Maths::BoundingBox> BoundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 4; i++)
            {
                BoundingBox->Merge(data[i].Position);
            }

            delete[] data;

            bool usingVulkan = false;
#ifdef LUMOS_RENDER_API_VULKAN
            if(GraphicsContext::GetRenderAPI() == RenderAPI::VULKAN)
                usingVulkan = true;
#endif

            SharedRef<IndexBuffer> ib;

            if(usingVulkan)
            {
                uint32_t indices[6] = {
                    0,
                    3,
                    2,
                    2,
                    1,
                    0,
                };

                ib.reset(IndexBuffer::Create(indices, 6));
            }
            else
            {
                uint32_t indices[6] = {
                    0,
                    1,
                    2,
                    2,
                    3,
                    0,
                };

                ib.reset(IndexBuffer::Create(indices, 6));
            }

            return new Mesh(vb, ib, BoundingBox);
        }

        Mesh* CreateCube()
        {
            //    v6----- v5
            //   /|      /|
            //  v1------v0|
            //  | |     | |
            //  | |v7---|-|v4
            //  |/      |/
            //  v2------v3
            LUMOS_PROFILE_FUNCTION();
            Vertex* data = new Vertex[24];

            data[0].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
            data[0].Colours = Maths::Vector4(0.0f);
            data[0].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

            data[1].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
            data[1].Colours = Maths::Vector4(0.0f);
            data[1].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

            data[2].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
            data[2].Colours = Maths::Vector4(0.0f);
            data[2].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

            data[3].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
            data[3].Colours = Maths::Vector4(0.0f);
            data[3].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

            data[4].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
            data[4].Colours = Maths::Vector4(0.0f);
            data[4].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

            data[5].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
            data[5].Colours = Maths::Vector4(0.0f);
            data[5].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

            data[6].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[6].Colours = Maths::Vector4(0.0f);
            data[6].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

            data[7].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[7].Colours = Maths::Vector4(0.0f);
            data[7].TexCoords = Maths::Vector2(0.0f, 1.0f);
            data[7].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

            data[8].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
            data[8].Colours = Maths::Vector4(0.0f);
            data[8].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

            data[9].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[9].Colours = Maths::Vector4(0.0f);
            data[9].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

            data[10].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[10].Colours = Maths::Vector4(0.0f);
            data[10].TexCoords = Maths::Vector2(0.0f, 1.0f);
            data[10].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

            data[11].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
            data[11].Colours = Maths::Vector4(0.0f);
            data[11].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

            data[12].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
            data[12].Colours = Maths::Vector4(0.0f);
            data[12].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

            data[13].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[13].Colours = Maths::Vector4(0.0f);
            data[13].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

            data[14].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[14].Colours = Maths::Vector4(0.0f);
            data[14].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

            data[15].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
            data[15].Colours = Maths::Vector4(0.0f);
            data[15].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

            data[16].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[16].Colours = Maths::Vector4(0.0f);
            data[16].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

            data[17].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[17].Colours = Maths::Vector4(0.0f);
            data[17].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

            data[18].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
            data[18].Colours = Maths::Vector4(0.0f);
            data[18].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

            data[19].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
            data[19].Colours = Maths::Vector4(0.0f);
            data[19].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

            data[20].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[20].Colours = Maths::Vector4(0.0f);
            data[20].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[21].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[21].Colours = Maths::Vector4(0.0f);
            data[21].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[22].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[22].Colours = Maths::Vector4(0.0f);
            data[22].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[23].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[23].Colours = Maths::Vector4(0.0f);
            data[23].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            for(int i = 0; i < 6; i++)
            {
                data[i * 4 + 0].TexCoords = Maths::Vector2(0.0f, 0.0f);
                data[i * 4 + 1].TexCoords = Maths::Vector2(1.0f, 0.0f);
                data[i * 4 + 2].TexCoords = Maths::Vector2(1.0f, 1.0f);
                data[i * 4 + 3].TexCoords = Maths::Vector2(0.0f, 1.0f);
            }

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(24 * sizeof(Vertex), data);

            SharedRef<Maths::BoundingBox> BoundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 8; i++)
            {
                BoundingBox->Merge(data[i].Position);
            }

            delete[] data;

            uint32_t indices[36] {
                0, 1, 2,
                0, 2, 3,
                4, 5, 6,
                4, 6, 7,
                8, 9, 10,
                8, 10, 11,
                12, 13, 14,
                12, 14, 15,
                16, 17, 18,
                16, 18, 19,
                20, 21, 22,
                20, 22, 23
            };

            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 36));

            return new Mesh(vb, ib, BoundingBox);
        }

        Mesh* CreatePyramid()
        {
            LUMOS_PROFILE_FUNCTION();
            Vertex* data = new Vertex[18];

            data[0].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[0].Colours = Maths::Vector4(0.0f);
            data[0].TexCoords = Maths::Vector2(0.24f, 0.20f);
            data[0].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

            data[1].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[1].Colours = Maths::Vector4(0.0f);
            data[1].TexCoords = Maths::Vector2(0.24f, 0.81f);
            data[1].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

            data[2].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
            data[2].Colours = Maths::Vector4(0.0f);
            data[2].TexCoords = Maths::Vector2(0.95f, 0.50f);
            data[2].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

            data[3].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[3].Colours = Maths::Vector4(0.0f);
            data[3].TexCoords = Maths::Vector2(0.24f, 0.21f);
            data[3].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

            data[4].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[4].Colours = Maths::Vector4(0.0f);
            data[4].TexCoords = Maths::Vector2(0.24f, 0.81f);
            data[4].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

            data[5].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
            data[5].Colours = Maths::Vector4(0.0f);
            data[5].TexCoords = Maths::Vector2(0.95f, 0.50f);
            data[5].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

            data[6].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[6].Colours = Maths::Vector4(0.0f);
            data[6].TexCoords = Maths::Vector2(0.24f, 0.81f);
            data[6].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

            data[7].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
            data[7].Colours = Maths::Vector4(0.0f);
            data[7].TexCoords = Maths::Vector2(0.95f, 0.50f);
            data[7].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

            data[8].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[8].Colours = Maths::Vector4(0.0f);
            data[8].TexCoords = Maths::Vector2(0.24f, 0.21f);
            data[8].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

            data[9].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[9].Colours = Maths::Vector4(0.0f);
            data[9].TexCoords = Maths::Vector2(0.24f, 0.21f);
            data[9].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

            data[10].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[10].Colours = Maths::Vector4(0.0f);
            data[10].TexCoords = Maths::Vector2(0.24f, 0.81f);
            data[10].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

            data[11].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
            data[11].Colours = Maths::Vector4(0.0f);
            data[11].TexCoords = Maths::Vector2(0.95f, 0.50f);
            data[11].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

            data[12].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
            data[12].Colours = Maths::Vector4(0.0f);
            data[12].TexCoords = Maths::Vector2(0.0f, 0.0f);
            data[12].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[13].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
            data[13].Colours = Maths::Vector4(0.0f);
            data[13].TexCoords = Maths::Vector2(0.0f, 1.0f);
            data[13].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[14].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
            data[14].Colours = Maths::Vector4(0.0f);
            data[14].TexCoords = Maths::Vector2(1.0f, 1.0f);
            data[14].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[15].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
            data[15].Colours = Maths::Vector4(0.0f);
            data[15].TexCoords = Maths::Vector2(0.96f, 0.0f);
            data[15].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

            data[16].Position = Maths::Vector3(0.0f, 0.0f, 0.0f);
            data[16].Colours = Maths::Vector4(0.0f);
            data[16].TexCoords = Maths::Vector2(0.0f, 0.0f);
            data[16].Normal = Maths::Vector3(0.0f, 0.0f, 0.0f);

            data[17].Position = Maths::Vector3(0.0f, 0.0f, 0.0f);
            data[17].Colours = Maths::Vector4(0.0f);
            data[17].TexCoords = Maths::Vector2(0.0f, 0.0f);
            data[17].Normal = Maths::Vector3(0.0f, 0.0f, 0.0f);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(18 * sizeof(Vertex), data);

            SharedRef<Maths::BoundingBox> BoundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 18; i++)
            {
                BoundingBox->Merge(data[i].Position);
            }
            delete[] data;

            uint32_t indices[18] {
                0, 1, 2,
                3, 4, 5,
                6, 7, 8,
                9, 10, 11,
                12, 13, 14,
                15, 12, 14
            };

            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices, 18));

            return new Mesh(vb, ib, BoundingBox);
        }

        Mesh* CreateSphere(uint32_t xSegments, uint32_t ySegments)
        {
            LUMOS_PROFILE_FUNCTION();
            auto data = std::vector<Vertex>();

            float sectorCount = static_cast<float>(xSegments);
            float stackCount = static_cast<float>(ySegments);
            float sectorStep = 2 * Maths::M_PI / sectorCount;
            float stackStep = Maths::M_PI / stackCount;
            float radius = 1.0f;

            for(int i = 0; i <= stackCount; ++i)
            {
                float stackAngle = Maths::M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
                float xy = radius * cos(stackAngle); // r * cos(u)
                float z = radius * sin(stackAngle); // r * sin(u)

                // add (sectorCount+1) vertices per stack
                // the first and last vertices have same position and normal, but different tex coords
                for(int j = 0; j <= sectorCount; ++j)
                {
                    float sectorAngle = j * sectorStep; // starting from 0 to 2pi

                    // vertex position (x, y, z)
                    float x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
                    float y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)

                    // vertex tex coord (s, t) range between [0, 1]
                    float s = static_cast<float>(j / sectorCount);
                    float t = static_cast<float>(i / stackCount);

                    Vertex vertex;
                    vertex.Position = Maths::Vector3(x, y, z);
                    vertex.TexCoords = Maths::Vector2(s, t);
                    vertex.Normal = Maths::Vector3(x, y, z).Normalised();

                    data.emplace_back(vertex);
                }
            }

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(int(data.size()) * sizeof(Vertex), data.data());

            std::vector<uint32_t> indices;
            uint32_t k1, k2;
            for(uint32_t i = 0; i < stackCount; ++i)
            {
                k1 = i * (static_cast<uint32_t>(sectorCount) + 1U); // beginning of current stack
                k2 = k1 + static_cast<uint32_t>(sectorCount) + 1U; // beginning of next stack

                for(uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
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
                    if(i != (stackCount - 1))
                    {
                        indices.push_back(k1 + 1);
                        indices.push_back(k2);
                        indices.push_back(k2 + 1);
                    }
                }
            }

            SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(size_t i = 0; i < data.size(); i++)
            {
                boundingBox->Merge(data[i].Position);
            }

            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size())));

            return new Mesh(vb, ib, boundingBox);
        }

        Mesh* CreateIcoSphere(uint32_t radius, uint32_t subdivision)
        {
            return nullptr;
        }

        Mesh* CreatePlane(float width, float height, const Maths::Vector3& normal)
        {
            LUMOS_PROFILE_FUNCTION();
            Maths::Vector3 vec = normal * 90.0f;
            Maths::Quaternion rotation = Maths::Quaternion(vec.z, Maths::Vector3(1.0f, 0.0f, 0.0f)) * Maths::Quaternion(vec.y, Maths::Vector3(0.0f, 1.0f, 0.0f)) * Maths::Quaternion(vec.x, Maths::Vector3(0.0f, 0.0f, 1.0f));

            Vertex* data = new Vertex[4];

            data[0].Position = rotation * Maths::Vector3(-width / 2.0f, -1.0f, -height / 2.0f);
            data[0].Normal = normal;
            data[0].TexCoords = Maths::Vector2(0.0f, 0.0f);

            data[1].Position = rotation * Maths::Vector3(-width / 2.0f, -1.0f, height / 2.0f);
            data[1].Normal = normal;
            data[1].TexCoords = Maths::Vector2(0.0f, 1.0f);

            data[2].Position = rotation * Maths::Vector3(width / 2.0f, 1.0f, height / 2.0f);
            data[2].Normal = normal;
            data[2].TexCoords = Maths::Vector2(1.0f, 1.0f);

            data[3].Position = rotation * Maths::Vector3(width / 2.0f, 1.0f, -height / 2.0f);
            data[3].Normal = normal;
            data[3].TexCoords = Maths::Vector2(1.0f, 0.0f);

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(4 * sizeof(Vertex), data);

            SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(int i = 0; i < 4; i++)
            {
                boundingBox->Merge(data[i].Position);
            }
            
            delete[] data;

            uint32_t indices[6] {
                0, 1, 2,
                2, 3, 0
            };

            SharedRef<IndexBuffer> ib = SharedRef<IndexBuffer>(IndexBuffer::Create(indices, 6));

            return new Mesh(vb, ib, boundingBox);
        }

        Mesh* CreateCapsule(float radius, float midHeight, int radialSegments, int rings)
        {
            LUMOS_PROFILE_FUNCTION();
            int i, j, prevrow, thisrow, point;
            float x, y, z, u, v, w;
            float onethird = 1.0f / 3.0f;
            float twothirds = 2.0f / 3.0f;

            std::vector<Vertex> data;
            std::vector<uint32_t> indices;

            point = 0;

            /* top hemisphere */
            thisrow = 0;
            prevrow = 0;
            for(j = 0; j <= (rings + 1); j++)
            {
                v = float(j);

                v /= (rings + 1);
                w = sin(0.5f * Maths::M_PI * v);
                z = radius * cos(0.5f * Maths::M_PI * v);

                for(i = 0; i <= radialSegments; i++)
                {
                    u = float(i);
                    u /= radialSegments;

                    x = sin(u * (Maths::M_PI * 2.0f));
                    y = -cos(u * (Maths::M_PI * 2.0f));

                    Maths::Vector3 p = Maths::Vector3(x * radius * w, y * radius * w, z);

                    Vertex vertex;
                    vertex.Position = p + Maths::Vector3(0.0f, 0.0f, 0.5f * midHeight);
                    vertex.Normal = (p + Maths::Vector3(0.0f, 0.0f, 0.5f * midHeight)).Normalised();
                    vertex.TexCoords = Maths::Vector2(u, onethird * v);
                    data.emplace_back(vertex);
                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.push_back(thisrow + i - 1);
                        indices.push_back(prevrow + i);
                        indices.push_back(prevrow + i - 1);

                        indices.push_back(thisrow + i - 1);
                        indices.push_back(thisrow + i);
                        indices.push_back(prevrow + i);
                    };
                };

                prevrow = thisrow;
                thisrow = point;
            };

            /* cylinder */
            thisrow = point;
            prevrow = 0;
            for(j = 0; j <= (rings + 1); j++)
            {
                v = float(j);
                v /= (rings + 1);

                z = midHeight * v;
                z = (midHeight * 0.5f) - z;

                for(i = 0; i <= radialSegments; i++)
                {
                    u = float(i);
                    u /= radialSegments;

                    x = sin(u * (Maths::M_PI * 2.0f));
                    y = -cos(u * (Maths::M_PI * 2.0f));

                    Maths::Vector3 p = Maths::Vector3(x * radius, y * radius, z);

                    Vertex vertex;
                    vertex.Position = p;
                    vertex.Normal = Maths::Vector3(x, 0.0f, z);
                    vertex.TexCoords = Maths::Vector2(u, onethird + (v * onethird));
                    data.emplace_back(vertex);

                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.push_back(thisrow + i - 1);
                        indices.push_back(prevrow + i);
                        indices.push_back(prevrow + i - 1);

                        indices.push_back(thisrow + i - 1);
                        indices.push_back(thisrow + i);
                        indices.push_back(prevrow + i);
                    };
                };

                prevrow = thisrow;
                thisrow = point;
            };

            /* bottom hemisphere */
            thisrow = point;
            prevrow = 0;

            for(j = 0; j <= (rings + 1); j++)
            {
                v = float(j);

                v /= (rings + 1);
                v += 1.0f;
                w = sin(0.5f * Maths::M_PI * v);
                z = radius * cos(0.5f * Maths::M_PI * v);

                for(i = 0; i <= radialSegments; i++)
                {
                    float u2 = float(i);
                    u2 /= radialSegments;

                    x = sin(u2 * (Maths::M_PI * 2.0f));
                    y = -cos(u2 * (Maths::M_PI * 2.0f));

                    Maths::Vector3 p = Maths::Vector3(x * radius * w, y * radius * w, z);

                    Vertex vertex;
                    vertex.Position = p + Maths::Vector3(0.0f, 0.0f, -0.5f * midHeight);
                    vertex.Normal = (p + Maths::Vector3(0.0f, 0.0f, -0.5f * midHeight)).Normalised();
                    vertex.TexCoords = Maths::Vector2(u2, twothirds + ((v - 1.0f) * onethird));
                    data.emplace_back(vertex);

                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.push_back(thisrow + i - 1);
                        indices.push_back(prevrow + i);
                        indices.push_back(prevrow + i - 1);

                        indices.push_back(thisrow + i - 1);
                        indices.push_back(thisrow + i);
                        indices.push_back(prevrow + i);
                    };
                };

                prevrow = thisrow;
                thisrow = point;
            }

            SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
            vb->SetData(static_cast<uint32_t>(data.size() * sizeof(Vertex)), data.data());

            SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();
            for(size_t i = 0; i < data.size(); i++)
            {
                boundingBox->Merge(data[i].Position);
            }

            SharedRef<IndexBuffer> ib;
            ib.reset(IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size())));

            return new Mesh(vb, ib, boundingBox);
        }
    }

    Graphics::Mesh* Graphics::CreateCylinder(float bottomRadius, float topRadius, float height, int radialSegments, int rings)
    {
        LUMOS_PROFILE_FUNCTION();
        int i, j, prevrow, thisrow, point = 0;
        float x, y, z, u, v, radius;

        std::vector<Vertex> data;
        std::vector<uint32_t> indices;

        thisrow = 0;
        prevrow = 0;
        for(j = 0; j <= (rings + 1); j++)
        {
            v = float(j);
            v /= (rings + 1);

            radius = topRadius + ((bottomRadius - topRadius) * v);

            y = height * v;
            y = (height * 0.5f) - y;

            for(i = 0; i <= radialSegments; i++)
            {
                u = float(i);
                u /= radialSegments;

                x = sin(u * (Maths::M_PI * 2.0f));
                z = cos(u * (Maths::M_PI * 2.0f));

                Maths::Vector3 p = Maths::Vector3(x * radius, y, z * radius);

                Vertex vertex;
                vertex.Position = p;
                vertex.Normal = Maths::Vector3(x, 0.0f, z);
                vertex.TexCoords = Maths::Vector2(u, v * 0.5f);
                data.emplace_back(vertex);

                point++;

                if(i > 0 && j > 0)
                {
                    indices.push_back(thisrow + i - 1);
                    indices.push_back(prevrow + i);
                    indices.push_back(prevrow + i - 1);

                    indices.push_back(thisrow + i - 1);
                    indices.push_back(thisrow + i);
                    indices.push_back(prevrow + i);
                };
            };

            prevrow = thisrow;
            thisrow = point;
        };

        // add top
        if(topRadius > 0.0f)
        {
            y = height * 0.5f;

            Vertex vertex;
            vertex.Position = Maths::Vector3(0.0f, y, 0.0f);
            vertex.Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);
            vertex.TexCoords = Maths::Vector2(0.25f, 0.75f);
            data.emplace_back(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = ((x + 1.0f) * 0.25f);
                v = 0.5f + ((z + 1.0f) * 0.25f);

                Maths::Vector3 p = Maths::Vector3(x * topRadius, y, z * topRadius);
                Vertex vertex;
                vertex.Position = p;
                vertex.Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);
                vertex.TexCoords = Maths::Vector2(u, v);
                data.emplace_back(vertex);
                point++;

                if(i > 0)
                {
                    indices.push_back(point - 2);
                    indices.push_back(point - 1);
                    indices.push_back(thisrow);
                };
            };
        };

        // add bottom
        if(bottomRadius > 0.0f)
        {
            y = height * -0.5f;

            thisrow = point;

            Vertex vertex;
            vertex.Position = Maths::Vector3(0.0f, y, 0.0f);
            vertex.Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);
            vertex.TexCoords = Maths::Vector2(0.75f, 0.75f);
            data.emplace_back(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = 0.5f + ((x + 1.0f) * 0.25f);
                v = 1.0f - ((z + 1.0f) * 0.25f);

                Maths::Vector3 p = Maths::Vector3(x * bottomRadius, y, z * bottomRadius);

                vertex.Position = p;
                vertex.Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);
                vertex.TexCoords = Maths::Vector2(u, v);
                data.emplace_back(vertex);
                point++;

                if(i > 0)
                {
                    indices.push_back(point - 1);
                    indices.push_back(point - 2);
                    indices.push_back(thisrow);
                };
            };
        };

        SharedRef<VertexBuffer> vb = SharedRef<VertexBuffer>(VertexBuffer::Create(BufferUsage::STATIC));
        vb->SetData(static_cast<uint32_t>(data.size() * sizeof(Vertex)), data.data());

        SharedRef<Maths::BoundingBox> boundingBox = CreateSharedRef<Maths::BoundingBox>();
        for(size_t i = 0; i < data.size(); i++)
        {
            boundingBox->Merge(data[i].Position);
        }

        SharedRef<IndexBuffer> ib;
        ib.reset(IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size())));

        return new Mesh(vb, ib, boundingBox);
    }

    Graphics::Mesh* Graphics::CreatePrimative(PrimitiveType type)
    {
        switch(type)
        {
        case Graphics::PrimitiveType::Cube:
            return Graphics::CreateCube();
        case Graphics::PrimitiveType::Plane:
            return Graphics::CreatePlane(1.0f, 1.0f, Maths::Vector3(0.0f, 1.0f, 0.0f));
        case Graphics::PrimitiveType::Quad:
            return Graphics::CreateQuad();
        case Graphics::PrimitiveType::Sphere:
            return Graphics::CreateSphere();
        case Graphics::PrimitiveType::Pyramid:
            return Graphics::CreatePyramid();
        case Graphics::PrimitiveType::Capsule:
            return Graphics::CreateCapsule();
        case Graphics::PrimitiveType::Cylinder:
            return Graphics::CreateCylinder();
        case Graphics::PrimitiveType::Terrain:
            return Graphics::CreateTerrain();
        case Graphics::PrimitiveType::File:
            LUMOS_LOG_WARN("Trying to create primitive of type File");
            return nullptr;
        }

        LUMOS_LOG_ERROR("Primitive not supported");
        return nullptr;
    };

    Graphics::Mesh* Graphics::CreateTerrain()
    {
        return new Terrain();
    }
}
