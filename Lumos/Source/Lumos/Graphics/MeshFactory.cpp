#include "Precompiled.h"
#include "MeshFactory.h"
#include "Mesh.h"
#include "Material.h"
#include "Terrain.h"
#include "Maths/MathsUtilities.h"
#include "Graphics/RHI/GraphicsContext.h"

#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
    namespace Graphics
    {
        Mesh* CreateQuad(float x, float y, float width, float height)
        {
            LUMOS_PROFILE_FUNCTION();

            TDArray<Vertex> data(4);

            data[0].Position  = Vec3(x, y, 0.0f);
            data[0].TexCoords = Vec2(0.0f, 1.0f);

            data[1].Position  = Vec3(x + width, y, 0.0f);
            data[1].TexCoords = Vec2(0, 0);

            data[2].Position  = Vec3(x + width, y + height, 0.0f);
            data[2].TexCoords = Vec2(1, 0);

            data[3].Position  = Vec3(x, y + height, 0.0f);
            data[3].TexCoords = Vec2(1, 1);

            TDArray<uint32_t> indices = {
                0,
                1,
                2,
                2,
                3,
                0,
            };
            return new Mesh(indices, data);
        }

        Mesh* CreateQuad(const Vec2& position, const Vec2& size)
        {
            return CreateQuad(position.x, position.y, size.x, size.y);
        }

        Mesh* CreateQuad()
        {
            LUMOS_PROFILE_FUNCTION();
            TDArray<Vertex> data(4);

            data[0].Position  = Vec3(-1.0f, -1.0f, 0.0f);
            data[0].TexCoords = Vec2(0.0f, 0.0f);
            data[0].Colours   = Vec4(0.0f);

            data[1].Position  = Vec3(1.0f, -1.0f, 0.0f);
            data[1].Colours   = Vec4(0.0f);
            data[1].TexCoords = Vec2(1.0f, 0.0f);

            data[2].Position  = Vec3(1.0f, 1.0f, 0.0f);
            data[2].Colours   = Vec4(0.0f);
            data[2].TexCoords = Vec2(1.0f, 1.0f);

            data[3].Position  = Vec3(-1.0f, 1.0f, 0.0f);
            data[3].Colours   = Vec4(0.0f);
            data[3].TexCoords = Vec2(0.0f, 1.0f);

            TDArray<uint32_t> indices = TDArray<uint32_t>({
                0,
                1,
                2,
                2,
                3,
                0,
            });

            return new Mesh(indices, data);
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
            TDArray<Vertex> data(24);

            data[0].Position = Vec3(0.5f, 0.5f, 0.5f);
            data[0].Colours  = Vec4(0.0f);
            data[0].Normal   = Vec3(0.0f, 0.0f, 1.0f);

            data[1].Position = Vec3(-0.5f, 0.5f, 0.5f);
            data[1].Colours  = Vec4(0.0f);
            data[1].Normal   = Vec3(0.0f, 0.0f, 1.0f);

            data[2].Position = Vec3(-0.5f, -0.5f, 0.5f);
            data[2].Colours  = Vec4(0.0f);
            data[2].Normal   = Vec3(0.0f, 0.0f, 1.0f);

            data[3].Position = Vec3(0.5f, -0.5f, 0.5f);
            data[3].Colours  = Vec4(0.0f);
            data[3].Normal   = Vec3(0.0f, 0.0f, 1.0f);

            data[4].Position = Vec3(0.5f, 0.5f, 0.5f);
            data[4].Colours  = Vec4(0.0f);
            data[4].Normal   = Vec3(1.0f, 0.0f, 0.0f);

            data[5].Position = Vec3(0.5f, -0.5f, 0.5f);
            data[5].Colours  = Vec4(0.0f);
            data[5].Normal   = Vec3(1.0f, 0.0f, 0.0f);

            data[6].Position = Vec3(0.5f, -0.5f, -0.5f);
            data[6].Colours  = Vec4(0.0f);
            data[6].Normal   = Vec3(1.0f, 0.0f, 0.0f);

            data[7].Position = Vec3(0.5f, 0.5f, -0.5f);
            data[7].Colours  = Vec4(0.0f);
            data[7].Normal   = Vec3(1.0f, 0.0f, 0.0f);

            data[8].Position = Vec3(0.5f, 0.5f, 0.5f);
            data[8].Colours  = Vec4(0.0f);
            data[8].Normal   = Vec3(0.0f, 1.0f, 0.0f);

            data[9].Position = Vec3(0.5f, 0.5f, -0.5f);
            data[9].Colours  = Vec4(0.0f);
            data[9].Normal   = Vec3(0.0f, 1.0f, 0.0f);

            data[10].Position = Vec3(-0.5f, 0.5f, -0.5f);
            data[10].Colours  = Vec4(0.0f);
            data[10].Normal   = Vec3(0.0f, 1.0f, 0.0f);

            data[11].Position = Vec3(-0.5f, 0.5f, 0.5f);
            data[11].Colours  = Vec4(0.0f);
            data[11].Normal   = Vec3(0.0f, 1.0f, 0.0f);

            data[12].Position = Vec3(-0.5f, 0.5f, 0.5f);
            data[12].Colours  = Vec4(0.0f);
            data[12].Normal   = Vec3(-1.0f, 0.0f, 0.0f);

            data[13].Position = Vec3(-0.5f, 0.5f, -0.5f);
            data[13].Colours  = Vec4(0.0f);
            data[13].Normal   = Vec3(-1.0f, 0.0f, 0.0f);

            data[14].Position = Vec3(-0.5f, -0.5f, -0.5f);
            data[14].Colours  = Vec4(0.0f);
            data[14].Normal   = Vec3(-1.0f, 0.0f, 0.0f);

            data[15].Position = Vec3(-0.5f, -0.5f, 0.5f);
            data[15].Colours  = Vec4(0.0f);
            data[15].Normal   = Vec3(-1.0f, 0.0f, 0.0f);

            data[16].Position = Vec3(-0.5f, -0.5f, -0.5f);
            data[16].Colours  = Vec4(0.0f);
            data[16].Normal   = Vec3(0.0f, -1.0f, 0.0f);

            data[17].Position = Vec3(0.5f, -0.5f, -0.5f);
            data[17].Colours  = Vec4(0.0f);
            data[17].Normal   = Vec3(0.0f, -1.0f, 0.0f);

            data[18].Position = Vec3(0.5f, -0.5f, 0.5f);
            data[18].Colours  = Vec4(0.0f);
            data[18].Normal   = Vec3(0.0f, -1.0f, 0.0f);

            data[19].Position = Vec3(-0.5f, -0.5f, 0.5f);
            data[19].Colours  = Vec4(0.0f);
            data[19].Normal   = Vec3(0.0f, -1.0f, 0.0f);

            data[20].Position = Vec3(0.5f, -0.5f, -0.5f);
            data[20].Colours  = Vec4(0.0f);
            data[20].Normal   = Vec3(0.0f, 0.0f, -1.0f);

            data[21].Position = Vec3(-0.5f, -0.5f, -0.5f);
            data[21].Colours  = Vec4(0.0f);
            data[21].Normal   = Vec3(0.0f, 0.0f, -1.0f);

            data[22].Position = Vec3(-0.5f, 0.5f, -0.5f);
            data[22].Colours  = Vec4(0.0f);
            data[22].Normal   = Vec3(0.0f, 0.0f, -1.0f);

            data[23].Position = Vec3(0.5f, 0.5f, -0.5f);
            data[23].Colours  = Vec4(0.0f);
            data[23].Normal   = Vec3(0.0f, 0.0f, -1.0f);

            for(int i = 0; i < 6; i++)
            {
                data[i * 4 + 0].TexCoords = Vec2(0.0f, 0.0f);
                data[i * 4 + 1].TexCoords = Vec2(1.0f, 0.0f);
                data[i * 4 + 2].TexCoords = Vec2(1.0f, 1.0f);
                data[i * 4 + 3].TexCoords = Vec2(0.0f, 1.0f);
            }

            TDArray<uint32_t> indices = TDArray<uint32_t>({ 0, 1, 2,
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
                                                            20, 22, 23 });

            return new Mesh(indices, data);
        }

        Mesh* CreatePyramid()
        {
            LUMOS_PROFILE_FUNCTION();
            TDArray<Vertex> data(18);

            data[0].Position  = Vec3(1.0f, 1.0f, -1.0f);
            data[0].Colours   = Vec4(0.0f);
            data[0].TexCoords = Vec2(0.24f, 0.20f);
            data[0].Normal    = Vec3(0.0f, 0.8948f, 0.4464f);

            data[1].Position  = Vec3(-1.0f, 1.0f, -1.0f);
            data[1].Colours   = Vec4(0.0f);
            data[1].TexCoords = Vec2(0.24f, 0.81f);
            data[1].Normal    = Vec3(0.0f, 0.8948f, 0.4464f);

            data[2].Position  = Vec3(0.0f, 0.0f, 1.0f);
            data[2].Colours   = Vec4(0.0f);
            data[2].TexCoords = Vec2(0.95f, 0.50f);
            data[2].Normal    = Vec3(0.0f, 0.8948f, 0.4464f);

            data[3].Position  = Vec3(-1.0f, 1.0f, -1.0f);
            data[3].Colours   = Vec4(0.0f);
            data[3].TexCoords = Vec2(0.24f, 0.21f);
            data[3].Normal    = Vec3(-0.8948f, 0.0f, 0.4464f);

            data[4].Position  = Vec3(-1.0f, -1.0f, -1.0f);
            data[4].Colours   = Vec4(0.0f);
            data[4].TexCoords = Vec2(0.24f, 0.81f);
            data[4].Normal    = Vec3(-0.8948f, 0.0f, 0.4464f);

            data[5].Position  = Vec3(0.0f, 0.0f, 1.0f);
            data[5].Colours   = Vec4(0.0f);
            data[5].TexCoords = Vec2(0.95f, 0.50f);
            data[5].Normal    = Vec3(-0.8948f, 0.0f, 0.4464f);

            data[6].Position  = Vec3(1.0f, 1.0f, -1.0f);
            data[6].Colours   = Vec4(0.0f);
            data[6].TexCoords = Vec2(0.24f, 0.81f);
            data[6].Normal    = Vec3(0.8948f, 0.0f, 0.4475f);

            data[7].Position  = Vec3(0.0f, 0.0f, 1.0f);
            data[7].Colours   = Vec4(0.0f);
            data[7].TexCoords = Vec2(0.95f, 0.50f);
            data[7].Normal    = Vec3(0.8948f, 0.0f, 0.4475f);

            data[8].Position  = Vec3(1.0f, -1.0f, -1.0f);
            data[8].Colours   = Vec4(0.0f);
            data[8].TexCoords = Vec2(0.24f, 0.21f);
            data[8].Normal    = Vec3(0.8948f, 0.0f, 0.4475f);

            data[9].Position  = Vec3(-1.0f, -1.0f, -1.0f);
            data[9].Colours   = Vec4(0.0f);
            data[9].TexCoords = Vec2(0.24f, 0.21f);
            data[9].Normal    = Vec3(0.0f, -0.8948f, 0.448f);

            data[10].Position  = Vec3(1.0f, -1.0f, -1.0f);
            data[10].Colours   = Vec4(0.0f);
            data[10].TexCoords = Vec2(0.24f, 0.81f);
            data[10].Normal    = Vec3(0.0f, -0.8948f, 0.448f);

            data[11].Position  = Vec3(0.0f, 0.0f, 1.0f);
            data[11].Colours   = Vec4(0.0f);
            data[11].TexCoords = Vec2(0.95f, 0.50f);
            data[11].Normal    = Vec3(0.0f, -0.8948f, 0.448f);

            data[12].Position  = Vec3(-1.0f, 1.0f, -1.0f);
            data[12].Colours   = Vec4(0.0f);
            data[12].TexCoords = Vec2(0.0f, 0.0f);
            data[12].Normal    = Vec3(0.0f, 0.0f, -1.0f);

            data[13].Position  = Vec3(1.0f, 1.0f, -1.0f);
            data[13].Colours   = Vec4(0.0f);
            data[13].TexCoords = Vec2(0.0f, 1.0f);
            data[13].Normal    = Vec3(0.0f, 0.0f, -1.0f);

            data[14].Position  = Vec3(1.0f, -1.0f, -1.0f);
            data[14].Colours   = Vec4(0.0f);
            data[14].TexCoords = Vec2(1.0f, 1.0f);
            data[14].Normal    = Vec3(0.0f, 0.0f, -1.0f);

            data[15].Position  = Vec3(-1.0f, -1.0f, -1.0f);
            data[15].Colours   = Vec4(0.0f);
            data[15].TexCoords = Vec2(0.96f, 0.0f);
            data[15].Normal    = Vec3(0.0f, 0.0f, -1.0f);

            data[16].Position  = Vec3(0.0f, 0.0f, 0.0f);
            data[16].Colours   = Vec4(0.0f);
            data[16].TexCoords = Vec2(0.0f, 0.0f);
            data[16].Normal    = Vec3(0.0f, 0.0f, 0.0f);

            data[17].Position  = Vec3(0.0f, 0.0f, 0.0f);
            data[17].Colours   = Vec4(0.0f);
            data[17].TexCoords = Vec2(0.0f, 0.0f);
            data[17].Normal    = Vec3(0.0f, 0.0f, 0.0f);

            TDArray<uint32_t> indices = TDArray<uint32_t>({ 0, 1, 2,
                                                            3, 4, 5,
                                                            6, 7, 8,
                                                            9, 10, 11,
                                                            12, 13, 14,
                                                            15, 12, 14 });

            return new Mesh(indices, data);
        }

        Mesh* CreateSphere(uint32_t xSegments, uint32_t ySegments)
        {
            LUMOS_PROFILE_FUNCTION();
            auto data = TDArray<Vertex>();

            float sectorCount = static_cast<float>(xSegments);
            float stackCount  = static_cast<float>(ySegments);
            float sectorStep  = 2 * Maths::M_PI / sectorCount;
            float stackStep   = Maths::M_PI / stackCount;
            float radius      = 0.5f;

            for(int i = 0; i <= stackCount; ++i)
            {
                float stackAngle = Maths::M_PI / 2 - i * stackStep; // starting from pi/2 to -pi/2
                float xy         = radius * cos(stackAngle);        // r * cos(u)
                float z          = radius * sin(stackAngle);        // r * sin(u)

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
                    vertex.Position  = Vec3(x, y, z);
                    vertex.TexCoords = Vec2(s, t);
                    vertex.Normal    = vertex.Position.Normalised();

                    data.EmplaceBack(vertex);
                }
            }

            TDArray<uint32_t> indices;
            uint32_t k1, k2;
            for(uint32_t i = 0; i < stackCount; ++i)
            {
                k1 = i * (static_cast<uint32_t>(sectorCount) + 1U); // beginning of current stack
                k2 = k1 + static_cast<uint32_t>(sectorCount) + 1U;  // beginning of next stack

                for(uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
                {
                    // 2 triangles per sector excluding first and last stacks
                    // k1 => k2 => k1+1
                    if(i != 0)
                    {
                        indices.PushBack(k1);
                        indices.PushBack(k2);
                        indices.PushBack(k1 + 1);
                    }

                    // k1+1 => k2 => k2+1
                    if(i != (stackCount - 1))
                    {
                        indices.PushBack(k1 + 1);
                        indices.PushBack(k2);
                        indices.PushBack(k2 + 1);
                    }
                }
            }

            return new Mesh(indices, data);
        }

        Mesh* CreatePlane(float width, float height, const Vec3& normal)
        {
            LUMOS_PROFILE_FUNCTION();
            Vec3 vec      = normal * 90.0f;
            Quat rotation = Quat::Rotation(vec.z, Vec3(1.0f, 0.0f, 0.0f)) * Quat::Rotation(vec.y, Vec3(0.0f, 1.0f, 0.0f)) * Quat::Rotation(vec.x, Vec3(0.0f, 0.0f, 1.0f));

            TDArray<Vertex> data(4);

            data[0].Position  = rotation * Vec3(-width * 0.5f, -1.0f, -height * 0.5f);
            data[0].Normal    = normal;
            data[0].TexCoords = Vec2(0.0f, 0.0f);

            data[1].Position  = rotation * Vec3(-width * 0.5f, -1.0f, height * 0.5f);
            data[1].Normal    = normal;
            data[1].TexCoords = Vec2(0.0f, 1.0f);

            data[2].Position  = rotation * Vec3(width * 0.5f, 1.0f, height * 0.5f);
            data[2].Normal    = normal;
            data[2].TexCoords = Vec2(1.0f, 1.0f);

            data[3].Position  = rotation * Vec3(width * 0.5f, 1.0f, -height * 0.5f);
            data[3].Normal    = normal;
            data[3].TexCoords = Vec2(1.0f, 0.0f);

            TDArray<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
            return new Mesh(indices, data);
        }

        Mesh* CreateCapsule(float radius, float midHeight, int radialSegments, int rings)
        {
            LUMOS_PROFILE_FUNCTION();
            int i, j, prevrow, thisrow, point;
            float x, y, z, u, v, w;
            float onethird  = 1.0f / 3.0f;
            float twothirds = 2.0f / 3.0f;

            TDArray<Vertex> data;
            TDArray<uint32_t> indices;

            point = 0;

            /* top hemisphere */
            thisrow = 0;
            prevrow = 0;
            for(j = 0; j <= (rings + 1); j++)
            {
                v = float(j);

                v /= (rings + 1);
                w = sin(0.5f * Maths::M_PI * v);
                y = radius * cos(0.5f * Maths::M_PI * v);

                for(i = 0; i <= radialSegments; i++)
                {
                    u = float(i);
                    u /= radialSegments;

                    x = sin(u * (Maths::M_PI * 2.0f));
                    z = cos(u * (Maths::M_PI * 2.0f));

                    Vec3 p = Vec3(x * radius * w, y, z * radius * w);

                    Vertex vertex;
                    vertex.Position  = p + Vec3(0.0f, 0.5f * midHeight, 0.0f);
                    vertex.Normal    = p.Normalised();
                    vertex.TexCoords = Vec2(u, onethird * v);
                    data.EmplaceBack(vertex);
                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(prevrow + i);
                        indices.PushBack(prevrow + i - 1);

                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(thisrow + i);
                        indices.PushBack(prevrow + i);
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

                y = midHeight * v;
                y = (midHeight * 0.5f) - y;

                for(i = 0; i <= radialSegments; i++)
                {
                    u = float(i);
                    u /= radialSegments;

                    x = sin(u * (Maths::M_PI * 2.0f));
                    z = cos(u * (Maths::M_PI * 2.0f));

                    Vec3 p = Vec3(x * radius, y, z * radius);

                    Vertex vertex;
                    vertex.Position = p;
                    // vertex.Normal = Vec3(x, z, 0.0f);
                    vertex.Normal = Vec3(x, 0.0f, z);
                    // vertex.TexCoords = Vec2(u, onethird + (v * onethird));
                    vertex.TexCoords = Vec2(u, v * 0.5f);
                    data.EmplaceBack(vertex);

                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(prevrow + i);
                        indices.PushBack(prevrow + i - 1);

                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(thisrow + i);
                        indices.PushBack(prevrow + i);
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
                y = radius * cos(0.5f * Maths::M_PI * v);

                for(i = 0; i <= radialSegments; i++)
                {
                    float u2 = float(i);
                    u2 /= radialSegments;

                    x = sin(u2 * (Maths::M_PI * 2.0f));
                    z = cos(u2 * (Maths::M_PI * 2.0f));

                    Vec3 p = Vec3(x * radius * w, y, z * radius * w);

                    Vertex vertex;
                    vertex.Position  = p + Vec3(0.0f, -0.5f * midHeight, 0.0f);
                    vertex.Normal    = p.Normalised();
                    vertex.TexCoords = Vec2(u2, twothirds + ((v - 1.0f) * onethird));
                    data.EmplaceBack(vertex);

                    point++;

                    if(i > 0 && j > 0)
                    {
                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(prevrow + i);
                        indices.PushBack(prevrow + i - 1);

                        indices.PushBack(thisrow + i - 1);
                        indices.PushBack(thisrow + i);
                        indices.PushBack(prevrow + i);
                    };
                };

                prevrow = thisrow;
                thisrow = point;
            }

            return new Mesh(indices, data);
        }
    }

    Graphics::Mesh* Graphics::CreateCylinder(float bottomRadius, float topRadius, float height, int radialSegments, int rings)
    {
        LUMOS_PROFILE_FUNCTION();
        int i, j, prevrow, thisrow, point = 0;
        float x, y, z, u, v, radius;

        TDArray<Vertex> data;
        TDArray<uint32_t> indices;

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

                Vec3 p = Vec3(x * radius, y, z * radius);

                Vertex vertex;
                vertex.Position  = p;
                vertex.Normal    = Vec3(x, 0.0f, z);
                vertex.TexCoords = Vec2(u, v * 0.5f);
                data.EmplaceBack(vertex);

                point++;

                if(i > 0 && j > 0)
                {
                    indices.PushBack(thisrow + i - 1);
                    indices.PushBack(prevrow + i);
                    indices.PushBack(prevrow + i - 1);

                    indices.PushBack(thisrow + i - 1);
                    indices.PushBack(thisrow + i);
                    indices.PushBack(prevrow + i);
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
            vertex.Position  = Vec3(0.0f, y, 0.0f);
            vertex.Normal    = Vec3(0.0f, 1.0f, 0.0f);
            vertex.TexCoords = Vec2(0.25f, 0.75f);
            data.EmplaceBack(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = ((x + 1.0f) * 0.25f);
                v = 0.5f + ((z + 1.0f) * 0.25f);

                Vec3 p = Vec3(x * topRadius, y, z * topRadius);
                Vertex vertex;
                vertex.Position  = p;
                vertex.Normal    = Vec3(0.0f, 1.0f, 0.0f);
                vertex.TexCoords = Vec2(u, v);
                data.EmplaceBack(vertex);
                point++;

                if(i > 0)
                {
                    indices.PushBack(point - 2);
                    indices.PushBack(point - 1);
                    indices.PushBack(thisrow);
                };
            };
        };

        // add bottom
        if(bottomRadius > 0.0f)
        {
            y = height * -0.5f;

            thisrow = point;

            Vertex vertex;
            vertex.Position  = Vec3(0.0f, y, 0.0f);
            vertex.Normal    = Vec3(0.0f, -1.0f, 0.0f);
            vertex.TexCoords = Vec2(0.75f, 0.75f);
            data.EmplaceBack(vertex);
            point++;

            for(i = 0; i <= radialSegments; i++)
            {
                float r = float(i);
                r /= radialSegments;

                x = sin(r * (Maths::M_PI * 2.0f));
                z = cos(r * (Maths::M_PI * 2.0f));

                u = 0.5f + ((x + 1.0f) * 0.25f);
                v = 1.0f - ((z + 1.0f) * 0.25f);

                Vec3 p = Vec3(x * bottomRadius, y, z * bottomRadius);

                vertex.Position  = p;
                vertex.Normal    = Vec3(0.0f, -1.0f, 0.0f);
                vertex.TexCoords = Vec2(u, v);
                data.EmplaceBack(vertex);
                point++;

                if(i > 0)
                {
                    indices.PushBack(point - 1);
                    indices.PushBack(point - 2);
                    indices.PushBack(thisrow);
                };
            };
        };

        return new Mesh(indices, data);
    }

    Graphics::Mesh* Graphics::CreatePrimative(PrimitiveType type)
    {
        switch(type)
        {
        case Graphics::PrimitiveType::Cube:
            return Graphics::CreateCube();
        case Graphics::PrimitiveType::Plane:
            return Graphics::CreatePlane(1.0f, 1.0f, Vec3(0.0f, 1.0f, 0.0f));
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
            LWARN("Trying to create primitive of type File");
            return nullptr;
        }

        LERROR("Primitive not supported");
        return nullptr;
    };

    Graphics::Mesh* Graphics::CreateTerrain()
    {
        return new Terrain();
    }
}
