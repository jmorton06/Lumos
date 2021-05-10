#pragma once

namespace Lumos
{
    class AINode
    {
    public:
        AINode() = default;
        virtual ~AINode() = default;

        void Update(float dt) {};
    };
}
