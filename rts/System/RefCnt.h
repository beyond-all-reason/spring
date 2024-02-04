#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

// adaptation and lots of fixes of https://github.com/fisnik-x/smart-ptr
// BSD-2-Clause License: https://github.com/fisnik-x/smart-ptr/blob/master/LICENSE

namespace recoil {
    template<typename T>
    class LightWeakPtr;
}

namespace recoil {
    struct ControlBlock {
        int ownCount{ 0 };
        int nwnCount{ 0 };
    };

    template<typename T>
    class LightSharedPtr {
    public:
        using Type = T;
        friend class LightWeakPtr<T>;
        constexpr LightSharedPtr()
            : LightSharedPtr(nullptr)
        {}

        template<typename ... Args>
        constexpr LightSharedPtr(Args&& ... args)
            : objPtr{ nullptr }
            , ctrBlk{ new ControlBlock }
        {
            objPtr = new T(std::forward<Args>(args)...);
            increment_reference();
        }

        constexpr LightSharedPtr(const std::nullptr_t) noexcept
            : objPtr{ nullptr }
            , ctrBlk{ nullptr }
        {}

        explicit LightSharedPtr(T* other)
            : objPtr{ other }
            , ctrBlk{ new ControlBlock }
        {
            increment_reference();
        }

        LightSharedPtr(LightSharedPtr&& other) noexcept
            : objPtr{ std::exchange(other.objPtr, nullptr) }
            , ctrBlk{ std::exchange(other.ctrBlk, nullptr) }
        {}

        explicit LightSharedPtr(LightWeakPtr<T>& other)
            : objPtr{ other.objPtr }
            , ctrBlk{ other.ctrBlk }
        {
            if (other.expired())
                throw std::runtime_error("LightWeakPtr has expired.");

            ++ctrBlk->ownCount;
        }

        ~LightSharedPtr() { decrement_reference(); }

        LightSharedPtr& operator=(const LightSharedPtr& other) noexcept
        {
            if (this == &other)
                return *this;

            decrement_reference();
            objPtr = other.objPtr;
            ctrBlk = other.ctrBlk;
            increment_reference();
            return *this;
        }

        LightSharedPtr& operator=(LightSharedPtr&& other) noexcept
        {
            if (this == &other)
                return *this;

            decrement_reference();
            ctrBlk = std::exchange(other.ctrBlk, nullptr);
            objPtr = std::exchange(other.objPtr, nullptr);

            return *this;
        }

        LightSharedPtr& operator=(std::nullptr_t)
        {
            if (ctrBlk == nullptr)
                return *this;

            decrement_reference();
            return *this;
        }

        [[nodiscard]] constexpr operator const LightWeakPtr<T>() const { return LightWeakPtr<T>(*this); }
        [[nodiscard]] constexpr operator       LightWeakPtr<T>()       { return LightWeakPtr<T>(*this); }

        T* get() const noexcept { return objPtr; }
        T* operator*() const noexcept { return *objPtr; }
        T* operator->() const noexcept { return objPtr; }
        explicit operator bool() const noexcept { return objPtr != nullptr && ctrBlk != nullptr; }

        int use_count() const noexcept { return ctrBlk ? ctrBlk->ownCount : 0; }
        void reset() noexcept { decrement_reference(); }

        void swap(LightSharedPtr<T>& rhs) noexcept
        {
            std::swap(ctrBlk, rhs.ctrBlk);
            std::swap(objPtr, rhs.objPtr);
        }

        friend auto operator<=>(const LightSharedPtr& lhs, const LightSharedPtr& rhs) = default;
        friend auto operator==(const LightSharedPtr& lhs, const LightSharedPtr& rhs)
        {
            if (lhs.get() != rhs.get())
                return false;

            return (lhs.get() <=> rhs.get()) == 0;
        }

    private:
        T* objPtr;
        ControlBlock* ctrBlk;

        void increment_reference()
        {
            ++ctrBlk->ownCount;
        }

        void decrement_reference()
        {
            if (objPtr) {
                if (--ctrBlk->ownCount == 0) {
                    delete objPtr;
                    if (ctrBlk->nwnCount == 0) {
                        delete ctrBlk;
                    }
                }

                objPtr = nullptr;
                ctrBlk = nullptr;
            }
        }
    };

    template<class T, class ...Args>
    LightSharedPtr<T> MakeLightShared(Args && ...args)
    {
        return LightSharedPtr<T>(new T(std::forward<Args>(args)...));
    }


    template<typename T>
    class LightWeakPtr {
        friend class LightSharedPtr<T>;
    public:
        constexpr LightWeakPtr() noexcept
            : objPtr{ nullptr }
            , ctrBlk{ nullptr }
        {}

        LightWeakPtr(T* other)
            : objPtr{ other }
            , ctrBlk{ new ControlBlock }
        {
            increment_weakptr();
        }

        LightWeakPtr(const LightWeakPtr<T>& other) noexcept
            : objPtr{ other.objPtr }
            , ctrBlk{ other.ctrBlk }
        {
            increment_weakptr();
        }

        LightWeakPtr(const LightSharedPtr<T>& other) { *this = other; }
        LightWeakPtr(      LightSharedPtr<T>& other) { *this = other; }

        ~LightWeakPtr() { decrement_weakptr(); }

        int use_count() const noexcept { return ctrBlk ? ctrBlk->nwnCount : 0; }
        void reset() noexcept { decrement_weakptr(); }

        bool expired() noexcept
        {
            if (ctrBlk == nullptr)
                return true;

            if (ctrBlk->ownCount == 0)
                --ctrBlk->nwnCount;

            if (ctrBlk->ownCount + ctrBlk->nwnCount == 0) {
                delete ctrBlk;
                objPtr = nullptr;
                ctrBlk = nullptr;
            }

            return !ctrBlk || ctrBlk->nwnCount == 0;
        }

        auto lock()
        {
            return expired() ? LightSharedPtr<T>() : LightSharedPtr<T>(*this);
        }

        LightWeakPtr& operator=(const LightWeakPtr& other)
        {
            if (this == &other)
                return *this; 

            decrement_weakptr();
            objPtr = other.objPtr;
            ctrBlk = other.ctrBlk;
            increment_weakptr();

            return *this;
        }

        LightWeakPtr& operator=(LightSharedPtr<T>& other)
        {
            objPtr = other.objPtr;
            ctrBlk = other.ctrBlk;

            if (objPtr != nullptr && ctrBlk != nullptr)
                ++ctrBlk->nwnCount;

            return *this;
        }
        LightWeakPtr& operator=(const LightSharedPtr<T>& other) { return operator=(const_cast<LightSharedPtr<T>&>(other)); }

        void swap(LightWeakPtr<T>& rhs) noexcept
        {
            std::swap(ctrBlk, rhs.ctrBlk);
            std::swap(objPtr, rhs.objPtr);
        }

    private:
        T* objPtr{ nullptr };
        ControlBlock* ctrBlk{ nullptr };

        void increment_weakptr() { ++ctrBlk->nwnCount; }
        void decrement_weakptr()
        {
            if (ctrBlk && objPtr) {
                if (--ctrBlk->nwnCount == 0 && ctrBlk->ownCount == 0) {
                    delete ctrBlk;
                }
                ctrBlk = nullptr;
                objPtr = nullptr;
            }
        }
    };

    template<class T>
    inline void swap(LightWeakPtr<T>& lhs, LightWeakPtr<T>& rhs) noexcept
    {
        T temp(std::move(lhs));
        lhs = std::move(rhs);
        rhs = std::move(temp);
    }
}
