#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "SamplingContext.h"

namespace Lumos
{
    namespace Graphics
    {
        SamplingContext::SamplingContext()
        {
        }

        SamplingContext::SamplingContext(const SamplingContext& other)
            : LocalTranslations(other.LocalTranslations)
            , LocalScales(other.LocalScales)
            , LocalRotations(other.LocalRotations)
            , m_LocalSpaceSoaTransforms(other.m_LocalSpaceSoaTransforms)
        {
            m_Context.Resize(other.m_Context.max_tracks());
        }

        SamplingContext::~SamplingContext()
        {
        }

        SamplingContext& SamplingContext::operator=(const SamplingContext& other)
        {
            LocalTranslations         = other.LocalTranslations;
            LocalScales               = other.LocalScales;
            LocalRotations            = other.LocalRotations;
            m_LocalSpaceSoaTransforms = other.m_LocalSpaceSoaTransforms;

            m_Context.Resize(other.m_Context.max_tracks());
            return *this;
        }

        void SamplingContext::resize(uint32_t size)
        {
            if(m_Size != size)
            {
                m_Size = size;
                m_Context.Resize(size);
                LocalTranslations.resize(size);
                LocalScales.resize(size);
                LocalRotations.resize(size);
            }
        }

        void SamplingContext::resizeSao(uint32_t size)
        {
            if(m_SaoSize != size)
            {
                m_SaoSize = size;
                m_LocalSpaceSoaTransforms.resize(size);
            }
        }
    }
}
