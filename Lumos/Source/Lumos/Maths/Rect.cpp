#include "Precompiled.h"
#include "Maths/Rect.h"

namespace Lumos::Maths
{
    const Rect Rect::FULL(-1.0f, -1.0f, 1.0f, 1.0f);
    const Rect Rect::POSITIVE(0.0f, 0.0f, 1.0f, 1.0f);
    const Rect Rect::ZERO(0.0f, 0.0f, 0.0f, 0.0f);

    const IntRect IntRect::ZERO(0, 0, 0, 0);

    void IntRect::Clip(const IntRect& rect)
    {
        if(rect.left_ > left_)
            left_ = rect.left_;
        if(rect.right_ < right_)
            right_ = rect.right_;
        if(rect.top_ > top_)
            top_ = rect.top_;
        if(rect.bottom_ < bottom_)
            bottom_ = rect.bottom_;

        if(left_ >= right_ || top_ >= bottom_)
            *this = IntRect();
    }

    void IntRect::Merge(const IntRect& rect)
    {
        if(Width() <= 0 || Height() <= 0)
        {
            *this = rect;
        }
        else if(rect.Width() > 0 && rect.Height() > 0)
        {
            if(rect.left_ < left_)
                left_ = rect.left_;
            if(rect.top_ < top_)
                top_ = rect.top_;
            if(rect.right_ > right_)
                right_ = rect.right_;
            if(rect.bottom_ > bottom_)
                bottom_ = rect.bottom_;
        }
    }

    void Rect::Clip(const Rect& rect)
    {
        if(rect.min_.x > min_.x)
            min_.x = rect.min_.x;
        if(rect.max_.x < max_.x)
            max_.x = rect.max_.x;
        if(rect.min_.y > min_.y)
            min_.y = rect.min_.y;
        if(rect.max_.y < max_.y)
            max_.y = rect.max_.y;

        if(min_.x > max_.x || min_.y > max_.y)
        {
            min_ = Vector2(M_INFINITY, M_INFINITY);
            max_ = Vector2(-M_INFINITY, -M_INFINITY);
        }
    }
}
