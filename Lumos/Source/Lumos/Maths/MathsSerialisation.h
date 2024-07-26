#pragma once
#include "Core/Core.h"
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix3.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    namespace Maths
    {

        template <class Archive>
        void save(Archive& archive, const Vector2& v)
        {
            archive(v.x, v.y);
        }

        template <class Archive>
        void load(Archive& archive, Vector2& v)
        {
            archive(v.x, v.y);
        }

        template <class Archive>
        void save(Archive& archive, const Vector3& v)
        {
            archive(v.x, v.y, v.z);
        }

        template <class Archive>
        void load(Archive& archive, Vector3& v)
        {
            archive(v.x, v.y, v.z);
        }

        template <class Archive>
        void save(Archive& archive, const Vector4& v)
        {
            archive(v.x, v.y, v.z, v.w);
        }

        template <class Archive>
        void load(Archive& archive, Vector4& v)
        {
            archive(v.x, v.y, v.z, v.w);
        }

        template <class Archive>
        void save(Archive& archive, const Matrix3& m)
        {
            for(u32 i = 0; i < 9; i++)
                archive(m.values[i]);
        }
        template <class Archive>
        void save(Archive& archive, const Matrix4& m)
        {
            for(u32 i = 0; i < 16; i++)
                archive(m.values[i]);
        }

        template <class Archive>
        void load(Archive& archive, Matrix3& m)
        {
            for(u32 i = 0; i < 9; i++)
                archive(m.values[i]);
        }
        template <class Archive>
        void load(Archive& archive, Matrix4& m)
        {
            for(u32 i = 0; i < 16; i++)
                archive(m.values[i]);
        }

        template <class Archive>
        void save(Archive& archive, const Quaternion& q)
        {
            archive(q.x, q.y, q.z, q.w);
        }

        template <class Archive>
        void load(Archive& archive, Quaternion& q)
        {
            archive(q.x, q.y, q.z, q.w);
        }
    }
}
