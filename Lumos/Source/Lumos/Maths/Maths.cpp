#include "Precompiled.h"
#include "Maths.h"

namespace Lumos
{
    namespace Maths
    {
        void SetScale(glm::mat4& transform, float scale)
        {
            transform[0][0] = scale;
            transform[1][1] = scale;
            transform[2][2] = scale;
        }

        void SetScale(glm::mat4& transform, const glm::vec3& scale)
        {
            transform[0][0] = scale.x;
            transform[1][1] = scale.y;
            transform[2][2] = scale.z;
        }

        void SetRotation(glm::mat4& transform, const glm::vec3& rotation)
        {
            transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        void SetTranslation(glm::mat4& transform, const glm::vec3& translation)
        {
            transform[3][0] = translation.x;
            transform[3][1] = translation.y;
            transform[3][2] = translation.z;
        }

        glm::vec3 GetScale(const glm::mat4& transform)
        {
            glm::vec3 scale = glm::vec3(transform[0][0], transform[1][1], transform[2][2]);
            return scale;
        }

        glm::vec3 GetRotation(const glm::mat4& transform)
        {
            glm::vec3 rotation = glm::eulerAngles(glm::quat_cast(transform));
            return rotation;
        }

        glm::mat4 Mat4FromTRS(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
        {
            glm::mat4 transform = glm::mat4(1.0f);
            SetScale(transform, scale);
            SetRotation(transform, rotation);
            SetTranslation(transform, translation);
            return transform;
        }

    }

}

namespace glm
{
    glm::vec3 operator*(const glm::mat4& a, const glm::vec3& b)
    {
        return glm::vec3(a * glm::vec4(b, 1.0f));
    }
}
