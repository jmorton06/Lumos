#include "Precompiled.h"
#include "Maths/Vector2.h"

namespace Lumos::Maths
{
    const Vector2 Vector2::ZERO;
    const Vector2 Vector2::LEFT(-1.0f, 0.0f);
    const Vector2 Vector2::RIGHT(1.0f, 0.0f);
    const Vector2 Vector2::UP(0.0f, 1.0f);
    const Vector2 Vector2::DOWN(0.0f, -1.0f);
    const Vector2 Vector2::ONE(1.0f, 1.0f);

    const IntVector2 IntVector2::ZERO;
    const IntVector2 IntVector2::LEFT(-1, 0);
    const IntVector2 IntVector2::RIGHT(1, 0);
    const IntVector2 IntVector2::UP(0, 1);
    const IntVector2 IntVector2::DOWN(0, -1);
    const IntVector2 IntVector2::ONE(1, 1);
}
