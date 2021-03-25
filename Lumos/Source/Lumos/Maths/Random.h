#pragma once

namespace Lumos::Maths
{
    /// Set the random seed. The default seed is 1.
     void SetRandomSeed(unsigned seed);
    /// Return the current random seed.
     unsigned GetRandomSeed();
    /// Return a random number between 0-32767. Should operate similarly to MSVC rand().
     int Rand();
    /// Return a standard normal distributed number.
     float RandStandardNormalised();
}
