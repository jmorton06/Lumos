#pragma once

namespace Lumos
{
    template<typename T>
    struct TScopedValue
    {
        TScopedValue(T& Value, const T& NewValue)
        : RefValue(Value), OriginalValue(Value)
        {
            RefValue = NewValue;
        }

        ~TScopedValue()
        {
            RefValue = OriginalValue;
        }

        T& RefValue;
        T OriginalValue;
    };
}
