#pragma once
#include "Core.h"

namespace Lumos
{
    template <typename FuncType>
    struct Function;

    template <typename R, typename... Args>
    struct Function<R(Args...)>
    {
    private:
        enum class FunctionErasedOperation
        {
            Destruct,
            CopyConstruct,
            MoveConstruct
        };
        using StubFunction      = R (*)(const void* const*, typename AddPointer<Args>::type...);
        using OperationFunction = void (*)(FunctionErasedOperation operation, const void** other, const void* const*);

        static const int MAX_LAMBDA_SIZE = sizeof(void*) * 8;

        StubFunction functionStub;
        OperationFunction functionOperation;

        union
        {
            const void* classInstance;
            char lambdaMemory[MAX_LAMBDA_SIZE] = { 0 };
        };

        void ExecuteOperation(FunctionErasedOperation operation, const void** other) const
        {
            if(functionOperation)
                (*functionOperation)(operation, other, &classInstance);
        }

        Function(const void* instance, StubFunction stub, OperationFunction operation)
            : functionStub(stub)
            , functionOperation(operation)
        {
            classInstance = instance;
        }
        using FreeFunction = R (*)(Args...);

    public:
        Function()
        {
            functionStub      = nullptr;
            functionOperation = nullptr;
        }

        template <
            typename Lambda,
            typename = typename EnableIf<
                not IsSame<typename RemoveReference<Lambda>::type, Function>::value, void>::type>
        Function(Lambda&& lambda)
        {
            functionStub      = nullptr;
            functionOperation = nullptr;
            Bind(Forward<typename RemoveReference<Lambda>::type>(lambda));
        }

        ~Function() { ExecuteOperation(FunctionErasedOperation::Destruct, nullptr); }

        Function(Function&& other)
        {
            functionStub      = other.functionStub;
            functionOperation = other.functionOperation;
            classInstance     = other.classInstance;
            other.ExecuteOperation(FunctionErasedOperation::MoveConstruct, &classInstance);
            other.ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            other.functionStub      = nullptr;
            other.functionOperation = nullptr;
        }

        Function(const Function& other)
        {
            functionStub      = other.functionStub;
            functionOperation = other.functionOperation;
            other.ExecuteOperation(FunctionErasedOperation::CopyConstruct, &classInstance);
        }

        Function& operator=(const Function& other)
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            functionStub      = other.functionStub;
            functionOperation = other.functionOperation;
            other.ExecuteOperation(FunctionErasedOperation::CopyConstruct, &classInstance);
            return *this;
        }

        Function& operator=(Function&& other) noexcept
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            functionStub      = other.functionStub;
            functionOperation = other.functionOperation;
            other.ExecuteOperation(FunctionErasedOperation::MoveConstruct, &classInstance);
            other.ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            other.functionStub = nullptr;
            return *this;
        }

        bool IsValid() const { return functionStub != nullptr; }
        operator bool() { return IsValid(); }

        template <typename Lambda>
        void Bind(Lambda&& lambda)
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            functionStub      = nullptr;
            functionOperation = nullptr;

            new(&classInstance) Lambda(Forward<Lambda>(lambda));
            static_assert(sizeof(Lambda) <= sizeof(lambdaMemory), "Lambda is too big");
            functionStub = [](const void* const* p, typename AddPointer<Args>::type... args) -> R
            {
                Lambda& lambda = *reinterpret_cast<Lambda*>(const_cast<void**>(p));
                return lambda(*args...);
            };
            functionOperation = [](FunctionErasedOperation operation, const void** other, const void* const* p)
            {
                Lambda& lambda = *reinterpret_cast<Lambda*>(const_cast<void**>(p));
                if(operation == FunctionErasedOperation::Destruct)
                    lambda.~Lambda();
                else if(operation == FunctionErasedOperation::CopyConstruct)
                    new(other) Lambda(lambda);
                else if(operation == FunctionErasedOperation::MoveConstruct)
                    new(other) Lambda(Move(lambda));
                else
#if defined(_MSC_VER)
                    __assume(false);
#else
                    __builtin_unreachable();
#endif
            };
        }

        template <R (*FreeFunction)(Args...)>
        void Bind()
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            classInstance     = nullptr;
            functionStub      = &FunctionWrapper<FreeFunction>;
            functionOperation = &FunctionOperation;
        }

        template <typename Class, R (Class::*MemberFunction)(Args...) const>
        void Bind(const Class& c)
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            classInstance     = &c;
            functionStub      = &MemberWrapper<Class, MemberFunction>;
            functionOperation = &MemberOperation;
        }

        template <typename Class, R (Class::*MemberFunction)(Args...)>
        void Bind(Class& c)
        {
            ExecuteOperation(FunctionErasedOperation::Destruct, nullptr);
            classInstance     = &c;
            functionStub      = &MemberWrapper<Class, MemberFunction>;
            functionOperation = &MemberOperation;
        }

        template <typename Class, R (Class::*MemberFunction)(Args...)>
        static Function FromMember(Class& c)
        {
            return Function(&c, &MemberWrapper<Class, MemberFunction>, &MemberOperation);
        }

        template <typename Class, R (Class::*MemberFunction)(Args...) const>
        static Function FromMember(const Class& c)
        {
            return Function(&c, &MemberWrapper<Class, MemberFunction>, &MemberOperation);
        }

        [[nodiscard]] R operator()(Args... args) const { return (*functionStub)(&classInstance, &args...); }

    private:
        static void MemberOperation(FunctionErasedOperation operation, const void** other, const void* const* p)
        {
            if(operation == FunctionErasedOperation::CopyConstruct or operation == FunctionErasedOperation::MoveConstruct)
                *other = *p;
        }

        template <typename Class, R (Class::*MemberFunction)(Args...)>
        static R MemberWrapper(const void* const* p, typename RemoveReference<Args>::type*... args)
        {
            Class* cls = const_cast<Class*>(static_cast<const Class*>(*p));
            return (cls->*MemberFunction)(*args...);
        }

        template <typename Class, R (Class::*MemberFunction)(Args...) const>
        static R MemberWrapper(const void* const* p, typename RemoveReference<Args>::type*... args)
        {
            const Class* cls = static_cast<const Class*>(*p);
            return (cls->*MemberFunction)(*args...);
        }

        template <R (*FreeFunction)(Args...)>
        static R FunctionWrapper(const void* const* p, typename RemoveReference<Args>::type*... args)
        {
            LUMOS_UNUSED(p);
            return FreeFunction(*args...);
        }
        static void FunctionOperation(FunctionErasedOperation, const void**, const void* const*) { }
    };
}