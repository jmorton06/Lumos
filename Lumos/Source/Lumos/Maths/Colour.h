#pragma once
#include "Maths/Vector4.h"

namespace Lumos::Maths
{
    /// RGBA Colour.
    class Colour
    {
    public:
        /// Mask describing Colour channels.
        struct ChannelMask
        {
            /// Red channel mask. If zero, red channel is set to 0.
            unsigned red_;
            /// Green channel mask. If zero, green channel is set to 0.
            unsigned green_;
            /// Blue channel mask. If zero, blue channel is set to 0.
            unsigned blue_;
            /// Alpha channel mask. If zero, alpha channel is set to 1.
            unsigned alpha_;
        };
        /// Mask for 0xAABBGGRR layout.
        static const ChannelMask ABGR;
        /// Mask for 0xAARRGGBB layout.
        static const ChannelMask ARGB;

        /// Construct with default values (opaque white.)
        Colour() noexcept
            : r_(1.0f)
            , g_(1.0f)
            , b_(1.0f)
            , a_(1.0f)
        {
        }

        /// Copy-construct from another Colour.
        Colour(const Colour& Colour) noexcept = default;

        /// Construct from another Colour and modify the alpha.
        Colour(const Colour& Colour, float a) noexcept
            : r_(Colour.r_)
            , g_(Colour.g_)
            , b_(Colour.b_)
            , a_(a)
        {
        }

        /// Construct from RGB values and set alpha fully opaque.
        Colour(float r, float g, float b) noexcept
            : r_(r)
            , g_(g)
            , b_(b)
            , a_(1.0f)
        {
        }

        /// Construct from RGBA values.
        Colour(float r, float g, float b, float a) noexcept
            : r_(r)
            , g_(g)
            , b_(b)
            , a_(a)
        {
        }

        /// Construct from a float array.
        explicit Colour(const float* data) noexcept
            : r_(data[0])
            , g_(data[1])
            , b_(data[2])
            , a_(data[3])
        {
        }

        /// Construct from 32-bit integer. Default format is 0xAABBGGRR.
        explicit Colour(unsigned Colour, ChannelMask mask = ABGR) { FromUIntMask(Colour, mask); }

        /// Assign from another Colour.
        Colour& operator=(const Colour& rhs) noexcept = default;

        /// Test for equality with another Colour without epsilon.
        bool operator==(const Colour& rhs) const { return r_ == rhs.r_ && g_ == rhs.g_ && b_ == rhs.b_ && a_ == rhs.a_; }

        /// Test for inequality with another Colour without epsilon.
        bool operator!=(const Colour& rhs) const { return r_ != rhs.r_ || g_ != rhs.g_ || b_ != rhs.b_ || a_ != rhs.a_; }

        /// Multiply with a scalar.
        Colour operator*(float rhs) const { return Colour(r_ * rhs, g_ * rhs, b_ * rhs, a_ * rhs); }

        /// Add a Colour.
        Colour operator+(const Colour& rhs) const { return Colour(r_ + rhs.r_, g_ + rhs.g_, b_ + rhs.b_, a_ + rhs.a_); }

        /// Return negation.
        Colour operator-() const { return Colour(-r_, -g_, -b_, -a_); }

        /// Subtract a Colour.
        Colour operator-(const Colour& rhs) const { return Colour(r_ - rhs.r_, g_ - rhs.g_, b_ - rhs.b_, a_ - rhs.a_); }

        /// Add-assign a Colour.
        Colour& operator+=(const Colour& rhs)
        {
            r_ += rhs.r_;
            g_ += rhs.g_;
            b_ += rhs.b_;
            a_ += rhs.a_;
            return *this;
        }

        /// Return float data.
        const float* Data() const { return &r_; }

        /// Return Colour packed to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
        unsigned ToUInt() const;
        /// Return Colour packed to a 32-bit integer with arbitrary channel mask. Components are clamped to [0, 1] range.
        unsigned ToUIntMask(ChannelMask mask) const;
        /// Return HSL Colour-space representation as a Vector3; the RGB values are clipped before conversion but not changed in the process.
        Vector3 ToHSL() const;
        /// Return HSV Colour-space representation as a Vector3; the RGB values are clipped before conversion but not changed in the process.
        Vector3 ToHSV() const;
        /// Set RGBA values from packed 32-bit integer, with R component in the lowest 8 bits (format 0xAABBGGRR).
        void FromUInt(unsigned Colour);
        /// Set RGBA values from packed 32-bit integer with arbitrary channel mask.
        void FromUIntMask(unsigned Colour, ChannelMask mask);
        /// Set RGBA values from specified HSL values and alpha.
        void FromHSL(float h, float s, float l, float a = 1.0f);
        /// Set RGBA values from specified HSV values and alpha.
        void FromHSV(float h, float s, float v, float a = 1.0f);

        /// Return RGB as a three-dimensional vector.
        Vector3 ToVector3() const { return Vector3(r_, g_, b_); }

        /// Return RGBA as a four-dimensional vector.
        Vector4 ToVector4() const { return Vector4(r_, g_, b_, a_); }

        /// Return sum of RGB components.
        float SumRGB() const { return r_ + g_ + b_; }

        /// Return average value of the RGB channels.
        float Average() const { return (r_ + g_ + b_) / 3.0f; }

        /// Return the 'grayscale' representation of RGB values, as used by JPEG and PAL/NTSC among others.
        float Luma() const { return r_ * 0.299f + g_ * 0.587f + b_ * 0.114f; }

        /// Return the Colourfulness relative to the brightness of a similarly illuminated white.
        float Chroma() const;
        /// Return hue mapped to range [0, 1.0).
        float Hue() const;
        /// Return saturation as defined for HSL.
        float SaturationHSL() const;
        /// Return saturation as defined for HSV.
        float SaturationHSV() const;

        /// Return value as defined for HSV: largest value of the RGB components. Equivalent to calling MinRGB().
        float Value() const { return MaxRGB(); }

        /// Return lightness as defined for HSL: average of the largest and smallest values of the RGB components.
        float Lightness() const;

        /// Stores the values of least and greatest RGB component at specified pointer addresses, optionally clipping those values to [0, 1] range.
        void Bounds(float* min, float* max, bool clipped = false) const;
        /// Return the largest value of the RGB components.
        float MaxRGB() const;
        /// Return the smallest value of the RGB components.
        float MinRGB() const;
        /// Return range, defined as the difference between the greatest and least RGB component.
        float Range() const;

        /// Clip to [0, 1.0] range.
        void Clip(bool clipAlpha = false);
        /// Inverts the RGB channels and optionally the alpha channel as well.
        void Invert(bool invertAlpha = false);
        /// Return linear interpolation of this Colour with another Colour.
        Colour Lerp(const Colour& rhs, float t) const;

        /// Return Colour with absolute components.
        Colour Abs() const { return Colour(Lumos::Maths::Abs(r_), Lumos::Maths::Abs(g_), Lumos::Maths::Abs(b_), Lumos::Maths::Abs(a_)); }

        /// Test for equality with another Colour with epsilon.
        bool Equals(const Colour& rhs, float eps = M_EPSILON) const
        {
            return Lumos::Maths::Equals(r_, rhs.r_, eps)
                && Lumos::Maths::Equals(g_, rhs.g_, eps)
                && Lumos::Maths::Equals(b_, rhs.b_, eps)
                && Lumos::Maths::Equals(a_, rhs.a_, eps);
        }

        static Colour Hex(uint32_t hexValue)
        {
            uint8_t red = (hexValue >> 16) & 255;
            uint8_t green = (hexValue >> 8) & 255;
            uint8_t blue = hexValue & 255;
            return Colour(red, green, blue);
        }

        /// Return as string.

        /// Return Colour packed to a 32-bit integer, with B component in the lowest 8 bits. Components are clamped to [0, 1] range.
        unsigned ToUIntArgb() const { return ToUIntMask(ARGB); }

        /// Return hash value for HashSet & HashMap.
        unsigned ToHash() const { return ToUInt(); }

        /// Red value.
        float r_;
        /// Green value.
        float g_;
        /// Blue value.
        float b_;
        /// Alpha value.
        float a_;

        /// Opaque white Colour.
        static const Colour WHITE;
        /// Opaque gray Colour.
        static const Colour GRAY;
        /// Opaque black Colour.
        static const Colour BLACK;
        /// Opaque red Colour.
        static const Colour RED;
        /// Opaque green Colour.
        static const Colour GREEN;
        /// Opaque blue Colour.
        static const Colour BLUE;
        /// Opaque cyan Colour.
        static const Colour CYAN;
        /// Opaque magenta Colour.
        static const Colour MAGENTA;
        /// Opaque yellow Colour.
        static const Colour YELLOW;
        /// Transparent black Colour (black with no alpha).
        static const Colour TRANSPARENT_BLACK;

    protected:
        /// Return hue value given greatest and least RGB component, value-wise.
        float Hue(float min, float max) const;
        /// Return saturation (HSV) given greatest and least RGB component, value-wise.
        float SaturationHSV(float min, float max) const;
        /// Return saturation (HSL) given greatest and least RGB component, value-wise.
        float SaturationHSL(float min, float max) const;
        /// Calculate and set RGB values. Convenience function used by FromHSV and FromHSL to avoid code duplication.
        void FromHCM(float h, float c, float m);
    };

    /// Multiply Colour with a scalar.
    inline Colour operator*(float lhs, const Colour& rhs) { return rhs * lhs; }
}
