#pragma once

#include <memory>
#include <vector>

namespace spring {
    template <typename R, typename... Args>
    class TypedQueuedFunction;

    class QueuedFunction {
    public:
        virtual ~QueuedFunction() = default;
        virtual void Execute() const = 0;


        template<typename F, typename... Args>
        static void Enqueue(F f, Args&&... args) {
            using R = decltype(f(std::forward<Args>(args)...));
            functions.emplace_back(std::make_unique<TypedQueuedFunction<R, Args...>>(f, std::forward<Args>(args)...));
        }
        template<typename F, typename... Args>
        static void Enqueue(F f, Args... args) {
            using R = decltype(f(args...));
            functions.emplace_back(std::make_unique<TypedQueuedFunction<R, Args...>>(f, args...));
        }

        static void Clear() { functions.clear(); }
        static bool Empty() { return functions.empty(); }
        static const auto& GetQueuedFunctions() { return functions; }
    private:
        inline static std::vector<std::unique_ptr<QueuedFunction>> functions = {};
    };

    template <typename R, typename... Args>
    class TypedQueuedFunction : public QueuedFunction {
    public:
        using ReturnType = R;
        using FunctionType = R(*)(Args...);


        TypedQueuedFunction(FunctionType func, Args&&... args)
            : storedFunc(func)
            , storedArgs(std::forward<Args>(args)...)
        {}
        TypedQueuedFunction(FunctionType func, Args... args)
            : storedFunc(func)
            , storedArgs(args...)
        {}

        void Execute() const override
        {
            std::apply(storedFunc, storedArgs);
        }
    private:
        std::tuple<std::decay_t<Args>...> storedArgs;
        FunctionType storedFunc;
    };
}