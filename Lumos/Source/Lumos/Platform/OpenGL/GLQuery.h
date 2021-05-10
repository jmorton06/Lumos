#pragma once

#include "Graphics/API/Query.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLQuery : public Query
        {
        public:
            explicit GLQuery(QueryType type);
            ~GLQuery();

            void Begin() override;
            uint32_t GetResult() override;
            bool GetResultReady() override;
            void End() override;

            static void MakeDefault();

        protected:
            static Query* CreateFuncGL(QueryType type);

        private:
            uint32_t m_Handle;
            uint32_t m_QueryType;
        };
    }
}
