#pragma once

namespace Lumos
{
    namespace Graphics
    {
        enum class LUMOS_EXPORT QueryType
        {
            SAMPLES_PASSED,
            ANY_SAMPLES_PASSED
        };

        class Query
        {
        public:
            virtual ~Query() = default;
            static Query* Create(QueryType type);

            virtual void Begin() = 0;
            virtual uint32_t GetResult() = 0;
            virtual bool GetResultReady() = 0;
            virtual void End() = 0;

            inline bool GetInUse() const { return m_InUse; }

            bool m_InUse;

        protected:
            static Query* (*CreateFunc)(QueryType);
        };
    }
}
