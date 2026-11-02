#pragma once
#include "Core/DataStructures/TDArray.h"
#include "Core/Reference.h"
#include "Core/Function.h"

namespace Lumos::AI
{
    enum class BTStatus : uint8_t
    {
        Invalid = 0,
        Running,
        Success,
        Failure,
    };

    class LUMOS_EXPORT BTNode
    {
    public:
        virtual ~BTNode() = default;
        virtual BTStatus Tick(float dt) = 0;
        virtual void Reset() { }
    };

    class LUMOS_EXPORT BTComposite : public BTNode
    {
    public:
        void AddChild(SharedPtr<BTNode> child) { m_Children.PushBack(child); }
        const TDArray<SharedPtr<BTNode>>& GetChildren() const { return m_Children; }

        void Reset() override
        {
            for(auto& c : m_Children) if(c) c->Reset();
            m_CurrentChild = 0;
        }

    protected:
        TDArray<SharedPtr<BTNode>> m_Children;
        uint32_t m_CurrentChild = 0;
    };

    class LUMOS_EXPORT BTSequence : public BTComposite
    {
    public:
        BTStatus Tick(float dt) override
        {
            while(m_CurrentChild < m_Children.Size())
            {
                BTStatus s = m_Children[m_CurrentChild]->Tick(dt);
                if(s == BTStatus::Running) return BTStatus::Running;
                if(s == BTStatus::Failure) { Reset(); return BTStatus::Failure; }
                ++m_CurrentChild;
            }
            m_CurrentChild = 0;
            return BTStatus::Success;
        }
    };

    class LUMOS_EXPORT BTSelector : public BTComposite
    {
    public:
        BTStatus Tick(float dt) override
        {
            while(m_CurrentChild < m_Children.Size())
            {
                BTStatus s = m_Children[m_CurrentChild]->Tick(dt);
                if(s == BTStatus::Running) return BTStatus::Running;
                if(s == BTStatus::Success) { Reset(); return BTStatus::Success; }
                ++m_CurrentChild;
            }
            m_CurrentChild = 0;
            return BTStatus::Failure;
        }
    };

    // Inverter: flips Success <-> Failure on a single child; Running passes through.
    class LUMOS_EXPORT BTInverter : public BTNode
    {
    public:
        explicit BTInverter(SharedPtr<BTNode> child) : m_Child(child) { }

        BTStatus Tick(float dt) override
        {
            if(!m_Child) return BTStatus::Failure;
            BTStatus s = m_Child->Tick(dt);
            if(s == BTStatus::Success) return BTStatus::Failure;
            if(s == BTStatus::Failure) return BTStatus::Success;
            return s;
        }

        void Reset() override { if(m_Child) m_Child->Reset(); }

    private:
        SharedPtr<BTNode> m_Child;
    };

    class LUMOS_EXPORT BTAction : public BTNode
    {
    public:
        using Func = Function<BTStatus(float)>;
        explicit BTAction(Func fn) : m_Fn(std::move(fn)) { }

        BTStatus Tick(float dt) override
        {
            return m_Fn ? m_Fn(dt) : BTStatus::Failure;
        }

    private:
        Func m_Fn;
    };

    // Convenience condition leaf: wraps a bool predicate as Success/Failure.
    class LUMOS_EXPORT BTCondition : public BTNode
    {
    public:
        using Predicate = Function<bool()>;
        explicit BTCondition(Predicate p) : m_Pred(std::move(p)) { }

        BTStatus Tick(float /*dt*/) override
        {
            if(!m_Pred) return BTStatus::Failure;
            return m_Pred() ? BTStatus::Success : BTStatus::Failure;
        }

    private:
        Predicate m_Pred;
    };

    // Owns a root node and a per-Tick status. Reset on entity reuse.
    class LUMOS_EXPORT BehaviourTree
    {
    public:
        BehaviourTree() = default;
        explicit BehaviourTree(SharedPtr<BTNode> root) : m_Root(root) { }

        void SetRoot(SharedPtr<BTNode> root) { m_Root = root; }
        const SharedPtr<BTNode>& GetRoot() const { return m_Root; }

        BTStatus Tick(float dt)
        {
            if(!m_Root) return BTStatus::Failure;
            m_LastStatus = m_Root->Tick(dt);
            return m_LastStatus;
        }

        void Reset() { if(m_Root) m_Root->Reset(); m_LastStatus = BTStatus::Invalid; }
        BTStatus GetLastStatus() const { return m_LastStatus; }

    private:
        SharedPtr<BTNode> m_Root;
        BTStatus m_LastStatus = BTStatus::Invalid;
    };
}
