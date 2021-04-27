#include "Precompiled.h"
#include "Maths/Colour.h"

namespace Lumos::Maths
{
    unsigned Colour::ToUInt() const
    {
        auto r = (unsigned)Clamp(((int)(r_ * 255.0f)), 0, 255);
        auto g = (unsigned)Clamp(((int)(g_ * 255.0f)), 0, 255);
        auto b = (unsigned)Clamp(((int)(b_ * 255.0f)), 0, 255);
        auto a = (unsigned)Clamp(((int)(a_ * 255.0f)), 0, 255);
        return (a << 24u) | (b << 16u) | (g << 8u) | r;
    }

    unsigned Colour::ToUIntMask(ChannelMask mask) const
    {
        const auto max = static_cast<double>(M_MAX_UNSIGNED);
        const auto r = static_cast<unsigned>(Clamp(static_cast<double>(r_) * mask.red_, 0.0, max)) & mask.red_;
        const auto g = static_cast<unsigned>(Clamp(static_cast<double>(g_) * mask.green_, 0.0, max)) & mask.green_;
        const auto b = static_cast<unsigned>(Clamp(static_cast<double>(b_) * mask.blue_, 0.0, max)) & mask.blue_;
        const auto a = static_cast<unsigned>(Clamp(static_cast<double>(a_) * mask.alpha_, 0.0, max)) & mask.alpha_;
        return r | g | b | a;
    }

    Vector3 Colour::ToHSL() const
    {
        float min, max;
        Bounds(&min, &max, true);

        float h = Hue(min, max);
        float s = SaturationHSL(min, max);
        float l = (max + min) * 0.5f;

        return Vector3(h, s, l);
    }

    Vector3 Colour::ToHSV() const
    {
        float min, max;
        Bounds(&min, &max, true);

        float h = Hue(min, max);
        float s = SaturationHSV(min, max);
        float v = max;

        return Vector3(h, s, v);
    }

    void Colour::FromUInt(unsigned Colour)
    {
        a_ = ((Colour >> 24u) & 0xffu) / 255.0f;
        b_ = ((Colour >> 16u) & 0xffu) / 255.0f;
        g_ = ((Colour >> 8u) & 0xffu) / 255.0f;
        r_ = ((Colour >> 0u) & 0xffu) / 255.0f;
    }

    void Colour::FromUIntMask(unsigned Colour, ChannelMask mask)
    {
        // Channel offset is irrelevant during division, but double should be used to avoid precision loss.
        r_ = !mask.red_ ? 0.0f : static_cast<float>((Colour & mask.red_) / static_cast<double>(mask.red_));
        g_ = !mask.green_ ? 0.0f : static_cast<float>((Colour & mask.green_) / static_cast<double>(mask.green_));
        b_ = !mask.blue_ ? 0.0f : static_cast<float>((Colour & mask.blue_) / static_cast<double>(mask.blue_));
        a_ = !mask.alpha_ ? 1.0f : static_cast<float>((Colour & mask.alpha_) / static_cast<double>(mask.alpha_));
    }

    void Colour::FromHSL(float h, float s, float l, float a)
    {
        float c;
        if(l < 0.5f)
            c = (1.0f + (2.0f * l - 1.0f)) * s;
        else
            c = (1.0f - (2.0f * l - 1.0f)) * s;

        float m = l - 0.5f * c;

        FromHCM(h, c, m);

        a_ = a;
    }

    void Colour::FromHSV(float h, float s, float v, float a)
    {
        float c = v * s;
        float m = v - c;

        FromHCM(h, c, m);

        a_ = a;
    }

    float Colour::Chroma() const
    {
        float min, max;
        Bounds(&min, &max, true);

        return max - min;
    }

    float Colour::Hue() const
    {
        float min, max;
        Bounds(&min, &max, true);

        return Hue(min, max);
    }

    float Colour::SaturationHSL() const
    {
        float min, max;
        Bounds(&min, &max, true);

        return SaturationHSL(min, max);
    }

    float Colour::SaturationHSV() const
    {
        float min, max;
        Bounds(&min, &max, true);

        return SaturationHSV(min, max);
    }

    float Colour::Lightness() const
    {
        float min, max;
        Bounds(&min, &max, true);

        return (max + min) * 0.5f;
    }

    void Colour::Bounds(float* min, float* max, bool clipped) const
    {
        assert(min && max);

        if(r_ > g_)
        {
            if(g_ > b_) // r > g > b
            {
                *max = r_;
                *min = b_;
            }
            else // r > g && g <= b
            {
                *max = r_ > b_ ? r_ : b_;
                *min = g_;
            }
        }
        else
        {
            if(b_ > g_) // r <= g < b
            {
                *max = b_;
                *min = r_;
            }
            else // r <= g && b <= g
            {
                *max = g_;
                *min = r_ < b_ ? r_ : b_;
            }
        }

        if(clipped)
        {
            *max = *max > 1.0f ? 1.0f : (*max < 0.0f ? 0.0f : *max);
            *min = *min > 1.0f ? 1.0f : (*min < 0.0f ? 0.0f : *min);
        }
    }

    float Colour::MaxRGB() const
    {
        if(r_ > g_)
            return (r_ > b_) ? r_ : b_;
        else
            return (g_ > b_) ? g_ : b_;
    }

    float Colour::MinRGB() const
    {
        if(r_ < g_)
            return (r_ < b_) ? r_ : b_;
        else
            return (g_ < b_) ? g_ : b_;
    }

    float Colour::Range() const
    {
        float min, max;
        Bounds(&min, &max);
        return max - min;
    }

    void Colour::Clip(bool clipAlpha)
    {
        r_ = (r_ > 1.0f) ? 1.0f : ((r_ < 0.0f) ? 0.0f : r_);
        g_ = (g_ > 1.0f) ? 1.0f : ((g_ < 0.0f) ? 0.0f : g_);
        b_ = (b_ > 1.0f) ? 1.0f : ((b_ < 0.0f) ? 0.0f : b_);

        if(clipAlpha)
            a_ = (a_ > 1.0f) ? 1.0f : ((a_ < 0.0f) ? 0.0f : a_);
    }

    void Colour::Invert(bool invertAlpha)
    {
        r_ = 1.0f - r_;
        g_ = 1.0f - g_;
        b_ = 1.0f - b_;

        if(invertAlpha)
            a_ = 1.0f - a_;
    }

    Colour Colour::Lerp(const Colour& rhs, float t) const
    {
        float invT = 1.0f - t;
        return Colour(
            r_ * invT + rhs.r_ * t,
            g_ * invT + rhs.g_ * t,
            b_ * invT + rhs.b_ * t,
            a_ * invT + rhs.a_ * t);
    }

    float Colour::Hue(float min, float max) const
    {
        float chroma = max - min;

        // If chroma equals zero, hue is undefined
        if(chroma <= M_EPSILON)
            return 0.0f;

        // Calculate and return hue
        if(Lumos::Maths::Equals(g_, max))
            return (b_ + 2.0f * chroma - r_) / (6.0f * chroma);
        else if(Lumos::Maths::Equals(b_, max))
            return (4.0f * chroma - g_ + r_) / (6.0f * chroma);
        else
        {
            float r = (g_ - b_) / (6.0f * chroma);
            return (r < 0.0f) ? 1.0f + r : ((r >= 1.0f) ? r - 1.0f : r);
        }
    }

    float Colour::SaturationHSV(float min, float max) const
    {
        // Avoid div-by-zero: result undefined
        if(max <= M_EPSILON)
            return 0.0f;

        // Saturation equals chroma:value ratio
        return 1.0f - (min / max);
    }

    float Colour::SaturationHSL(float min, float max) const
    {
        // Avoid div-by-zero: result undefined
        if(max <= M_EPSILON || min >= 1.0f - M_EPSILON)
            return 0.0f;

        // Chroma = max - min, lightness = (max + min) * 0.5
        float hl = (max + min);
        if(hl <= 1.0f)
            return (max - min) / hl;
        else
            return (min - max) / (hl - 2.0f);
    }

    void Colour::FromHCM(float h, float c, float m)
    {
        if(h < 0.0f || h >= 1.0f)
            h -= floorf(h);

        float hs = h * 6.0f;
        float x = c * (1.0f - Lumos::Maths::Abs(fmodf(hs, 2.0f) - 1.0f));

        // Reconstruct r', g', b' from hue
        if(hs < 2.0f)
        {
            b_ = 0.0f;
            if(hs < 1.0f)
            {
                g_ = x;
                r_ = c;
            }
            else
            {
                g_ = c;
                r_ = x;
            }
        }
        else if(hs < 4.0f)
        {
            r_ = 0.0f;
            if(hs < 3.0f)
            {
                g_ = c;
                b_ = x;
            }
            else
            {
                g_ = x;
                b_ = c;
            }
        }
        else
        {
            g_ = 0.0f;
            if(hs < 5.0f)
            {
                r_ = x;
                b_ = c;
            }
            else
            {
                r_ = c;
                b_ = x;
            }
        }

        r_ += m;
        g_ += m;
        b_ += m;
    }

    const Colour::ChannelMask Colour::ABGR { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };
    const Colour::ChannelMask Colour::ARGB { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
    const Colour Colour::WHITE;
    const Colour Colour::GRAY(0.5f, 0.5f, 0.5f);
    const Colour Colour::BLACK(0.0f, 0.0f, 0.0f);
    const Colour Colour::RED(1.0f, 0.0f, 0.0f);
    const Colour Colour::GREEN(0.0f, 1.0f, 0.0f);
    const Colour Colour::BLUE(0.0f, 0.0f, 1.0f);
    const Colour Colour::CYAN(0.0f, 1.0f, 1.0f);
    const Colour Colour::MAGENTA(1.0f, 0.0f, 1.0f);
    const Colour Colour::YELLOW(1.0f, 1.0f, 0.0f);
    const Colour Colour::TRANSPARENT_BLACK(0.0f, 0.0f, 0.0f, 0.0f);
}
