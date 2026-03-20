#pragma once

namespace Lumos
{
    enum class PhysicsMaterialCombineMode : uint8_t
    {
        Average  = 0,
        Minimum  = 1,
        Maximum  = 2,
        Multiply = 3
    };

    struct PhysicsMaterial
    {
        float Friction              = 0.5f;
        float Restitution           = 0.5f;
        PhysicsMaterialCombineMode FrictionCombine    = PhysicsMaterialCombineMode::Average;
        PhysicsMaterialCombineMode RestitutionCombine = PhysicsMaterialCombineMode::Average;

        static float CombineValues(float a, float b, PhysicsMaterialCombineMode mode)
        {
            switch(mode)
            {
            case PhysicsMaterialCombineMode::Average:
                return (a + b) * 0.5f;
            case PhysicsMaterialCombineMode::Minimum:
                return a < b ? a : b;
            case PhysicsMaterialCombineMode::Maximum:
                return a > b ? a : b;
            case PhysicsMaterialCombineMode::Multiply:
                return a * b;
            default:
                return (a + b) * 0.5f;
            }
        }

        // Common presets
        static PhysicsMaterial Default() { return { 0.5f, 0.5f }; }
        static PhysicsMaterial Bouncy() { return { 0.2f, 0.9f }; }
        static PhysicsMaterial Ice() { return { 0.05f, 0.1f }; }
        static PhysicsMaterial Rubber() { return { 0.8f, 0.8f }; }
        static PhysicsMaterial Metal() { return { 0.4f, 0.3f }; }
        static PhysicsMaterial Wood() { return { 0.5f, 0.4f }; }
        static PhysicsMaterial Concrete() { return { 0.6f, 0.2f }; }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(Friction, Restitution, (uint8_t)FrictionCombine, (uint8_t)RestitutionCombine);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            uint8_t fc, rc;
            archive(Friction, Restitution, fc, rc);
            FrictionCombine = (PhysicsMaterialCombineMode)fc;
            RestitutionCombine = (PhysicsMaterialCombineMode)rc;
        }
    };
}
