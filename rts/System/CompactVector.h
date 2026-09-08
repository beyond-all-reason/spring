/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMPACT_VECTOR_HDR
#define COMPACT_VECTOR_HDR

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <compare>
#include <stdexcept>
#include <iterator>
#include <initializer_list>
#include <limits>
#include <memory>       // std::uninitialized_copy_n, std::uninitialized_fill_n
#include <type_traits>

namespace recoil {

/**
 * @brief Low-overhead vector implementation optimized for memory efficiency.
 *
 * Standard std::vector uses 24 bytes on 64-bit systems (3 pointers for data, size, capacity).
 * CompactVector uses only 16 bytes (1 pointer + 2 x uint32_t), saving 8 bytes per instance.
 *
 * This is a significant improvement when dealing with millions of vector instances.
 * The tradeoff is that this vector is limited to uint32_t max elements (~4 billion),
 * which is not a practical limitation for most use cases.
 *
 * @note Requires C++20 or later (uses if constexpr, std::enable_if_t, is_integral_v, and <=>).
 * @note Assumes destructors are non-throwing (consistent with std::vector).
 * @note insert(pos, T&&) with self-moved values is not supported (use insert(pos, T) instead).
 *
 * @tparam T The type of elements stored in the vector
 */
template<typename T>
class CompactVector {
public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;
    using const_iterator  = const T*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type       = uint32_t;
    using difference_type = std::ptrdiff_t;

private:
    pointer   m_data     = nullptr;
    size_type m_size     = 0;
    size_type m_capacity = 0;

    // Minimum allocation on first growth. Reduces early reallocation churn for
    // small vectors without touching the struct layout or wasting memory when the
    // vector is never pushed to.
    static constexpr size_type kMinCapacity = 8;

public:
    // ============ Constructors ============

    /// Default constructor - creates empty vector
    CompactVector() = default;

    /// Constructs vector with count copies of value
    explicit CompactVector(size_type count, const T& value = T()) {
        if (count > 0) {
            reserve(count);
            uninit_fill(m_data, count, value);
            m_size = count;
        }
    }

    /// Copy constructor
    CompactVector(const CompactVector& other) {
        if (!other.empty()) {
            reserve(other.m_size);
            uninit_copy(other.m_data, other.m_size, m_data);
            m_size = other.m_size;
        }
    }

    /// Move constructor
    CompactVector(CompactVector&& other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
        other.m_data     = nullptr;
        other.m_size     = 0;
        other.m_capacity = 0;
    }

    /// Constructs from iterator range (SFINAE to avoid matching on integral types)
    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    CompactVector(InputIt first, InputIt last) {
        for (InputIt it = first; it != last; ++it)
            push_back(*it);
    }

    /// Constructs from initializer list
    CompactVector(std::initializer_list<T> init) {
        const size_type count = static_cast<size_type>(init.size());
        reserve(count);
        uninit_copy(init.begin(), count, m_data);
        m_size = count;
    }

    // ============ Destructor ============

    ~CompactVector() {
        destroy_range(m_data, m_size);
        deallocate();
    }

    // ============ Assignment Operators ============

    /// Copy assignment
    CompactVector& operator=(const CompactVector& other) {
        if (this != &other) {
            destroy_range(m_data, m_size);
            // Reset m_size immediately after destroying elements. If uninit_copy
            // throws (T's copy constructor can throw), the destructor must not
            // call destroy_range on the already-destroyed objects again.
            m_size = 0;
            // Reuse allocation if it is large enough
            if (other.m_size > m_capacity) {
                deallocate();
                m_data     = allocate(other.m_size);
                m_capacity = other.m_size;
            }
            uninit_copy(other.m_data, other.m_size, m_data);
            m_size = other.m_size;
        }
        return *this;
    }

    /// Move assignment
    CompactVector& operator=(CompactVector&& other) noexcept {
        if (this != &other) {
            destroy_range(m_data, m_size);
            deallocate();
            m_data           = other.m_data;
            m_size           = other.m_size;
            m_capacity       = other.m_capacity;
            other.m_data     = nullptr;
            other.m_size     = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /// Assign from initializer list
    CompactVector& operator=(std::initializer_list<T> ilist) {
        const size_type count = static_cast<size_type>(ilist.size());
        destroy_range(m_data, m_size);
        // Reset m_size immediately — same exception-safety reason as copy assignment.
        m_size = 0;
        if (count > m_capacity) {
            deallocate();
            m_data     = allocate(count);
            m_capacity = count;
        }
        uninit_copy(ilist.begin(), count, m_data);
        m_size = count;
        return *this;
    }

    /// Assign count copies of value
    void assign(size_type count, const T& value) {
        // Copy value before destroy to handle self-reference (e.g., v.assign(5, v[2]))
        T value_copy = value;
        destroy_range(m_data, m_size);
        // Reset m_size immediately after destroying elements. If uninit_fill throws
        // (T's copy constructor can throw), the destructor must not call destroy_range
        // on the already-destroyed objects again.
        m_size = 0;
        if (count > m_capacity) {
            deallocate();
            m_data     = allocate(count);
            m_capacity = count;
        }
        uninit_fill(m_data, count, value_copy);
        m_size = count;
    }

    /// Assign from iterator range (SFINAE to avoid matching on integral types)
    template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
    void assign(InputIt first, InputIt last) {
        bool self_range = false;
        if constexpr (std::is_same_v<InputIt, iterator> || std::is_same_v<InputIt, const_iterator>) {
            self_range = (first >= cbegin() && first < cend());
        }

        if (self_range) {
            CompactVector temp(first, last);
            swap(temp);
        } else {
            destroy_range(m_data, m_size);
            m_size = 0;
            for (InputIt it = first; it != last; ++it)
                push_back(*it);
        }
    }

    // ============ Element Access ============

    [[nodiscard]] reference operator[](size_type index) noexcept {
        return m_data[index];
    }

    [[nodiscard]] const_reference operator[](size_type index) const noexcept {
        return m_data[index];
    }

    [[nodiscard]] reference at(size_type index) {
        if (index >= m_size)
            throw std::out_of_range("CompactVector::at");
        return m_data[index];
    }

    [[nodiscard]] const_reference at(size_type index) const {
        if (index >= m_size)
            throw std::out_of_range("CompactVector::at");
        return m_data[index];
    }

    [[nodiscard]] reference       front() noexcept       { return m_data[0]; }
    [[nodiscard]] const_reference front() const noexcept { return m_data[0]; }
    [[nodiscard]] reference       back()  noexcept       { return m_data[m_size - 1]; }
    [[nodiscard]] const_reference back()  const noexcept { return m_data[m_size - 1]; }

    [[nodiscard]] pointer       data() noexcept       { return m_data; }
    [[nodiscard]] const_pointer data() const noexcept { return m_data; }

    // ============ Iterators ============

    iterator       begin()  noexcept       { return m_data; }
    const_iterator begin()  const noexcept { return m_data; }
    const_iterator cbegin() const noexcept { return m_data; }
    iterator       end()    noexcept       { return m_data + m_size; }
    const_iterator end()    const noexcept { return m_data + m_size; }
    const_iterator cend()   const noexcept { return m_data + m_size; }

    reverse_iterator       rbegin()  noexcept       { return reverse_iterator(end()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    reverse_iterator       rend()    noexcept       { return reverse_iterator(begin()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(begin()); }

    // ============ Capacity ============

    [[nodiscard]] bool      empty()    const noexcept { return m_size == 0; }
    [[nodiscard]] size_type size()     const noexcept { return m_size; }
    [[nodiscard]] size_type capacity() const noexcept { return m_capacity; }
    [[nodiscard]] size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }

    void reserve(size_type new_capacity) {
        if (new_capacity <= m_capacity)
            return;
        relocate(new_capacity);
    }

    void shrink_to_fit() {
        if (m_size < m_capacity) {
            if (m_size == 0) {
                deallocate();
                m_capacity = 0;
            } else {
                relocate(m_size);
            }
        }
    }

    // ============ Modifiers ============

    void clear() noexcept {
        destroy_range(m_data, m_size);
        m_size = 0;
    }

    /// Adds element to the end.
    /// Safe when value is a reference to an element already in this vector.
    void push_back(const T& value) {
        if (m_size == m_capacity) [[unlikely]] {
            // Defensive copy: value may alias our buffer (e.g. v.push_back(v[0])).
            // Must be taken before grow() frees the old allocation.
            // Cost is paid only on the cold reallocation path.
            T value_copy = value;
            grow(add_sizes(m_size, 1));
            new (m_data + m_size) T(std::move(value_copy));
        } else {
            // Fast path: no reallocation — value reference stays valid.
            new (m_data + m_size) T(value);
        }
        ++m_size;
    }

    /// Adds element to the end (move semantics).
    /// The caller must not pass an element of this vector (undefined behaviour, same as std::vector).
    void push_back(T&& value) {
        if (m_size == m_capacity) [[unlikely]]
            grow(add_sizes(m_size, 1));
        new (m_data + m_size) T(std::move(value));
        ++m_size;
    }

    /// Constructs element in-place at the end.
    template<typename... Args>
    reference emplace_back(Args&&... args) {
        if (m_size == m_capacity) [[unlikely]]
            grow(add_sizes(m_size, 1));
        new (m_data + m_size) T(std::forward<Args>(args)...);
        return m_data[m_size++];
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            m_data[m_size].~T();
        }
    }

    void resize(size_type count, const T& value = T()) {
        if (count > m_size) {
            T value_copy = value;   // guard against self-reference e.g. v.resize(n, v[0])
            reserve(count);
            // For scalar T (int, float, pointer, enum — no padding, no custom ctor),
            // T{} is guaranteed all-zero bytes, so we can use memset which gives the
            // compiler/CRT the best chance to exploit zero-initialised OS pages.
            // We restrict to std::is_scalar_v because structs may have padding bytes
            // that are not zeroed by the copy constructor; reading them via memcmp
            // would be undefined behaviour (uninitialised memory read).
            if constexpr (std::is_scalar_v<T>) {
                if (value_copy == T{}) {
                    std::memset(m_data + m_size, 0, (count - m_size) * sizeof(T));
                } else {
                    uninit_fill(m_data + m_size, count - m_size, value_copy);
                }
            } else {
                uninit_fill(m_data + m_size, count - m_size, value_copy);
            }
            m_size = count;
        } else if (count < m_size) {
            destroy_range(m_data + count, m_size - count);
            m_size = count;
        }
    }

    /// Erases element at pos. Returns iterator to the element that followed pos.
    iterator erase(const_iterator pos) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx < m_size) {
            // FIX: don't pre-destroy then placement-new over live slots.
            // shift_left handles the full move+destroy sequence correctly.
            shift_left(idx, 1);
            --m_size;
        }
        return m_data + idx;
    }

    /// Erases elements in [first, last). Returns iterator to the element that followed last.
    iterator erase(const_iterator first, const_iterator last) {
        size_type start_idx = static_cast<size_type>(first - cbegin());
        size_type end_idx   = static_cast<size_type>(last  - cbegin());

        // Guard: end_idx < start_idx would underflow to a huge count (uint32_t wrap).
        // Passing first > last is UB per the standard, but we defend against it cheaply.
        if (end_idx > start_idx && end_idx <= m_size) {
            size_type count = end_idx - start_idx;
            shift_left(start_idx, count);
            m_size -= count;
        }
        return m_data + start_idx;
    }

    iterator insert(const_iterator pos, const T& value) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx <= m_size) {
            T value_copy = value;   // guard against self-reference before potential grow()
            if (m_size == m_capacity) [[unlikely]]
                grow(add_sizes(m_size, 1));
            shift_right(idx, 1);
            new (m_data + idx) T(std::move(value_copy));
            ++m_size;
        }
        return m_data + idx;
    }

    iterator insert(const_iterator pos, T&& value) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx <= m_size) {
            if (m_size == m_capacity) [[unlikely]]
                grow(add_sizes(m_size, 1));
            shift_right(idx, 1);
            new (m_data + idx) T(std::move(value));
            ++m_size;
        }
        return m_data + idx;
    }

    iterator insert(const_iterator pos, size_type count, const T& value) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (idx <= m_size && count > 0) {
            T value_copy = value;
            size_type new_size = add_sizes(m_size, count);
            if (new_size > m_capacity) [[unlikely]]
                grow(new_size);
            shift_right(idx, count);
            uninit_fill(m_data + idx, count, value_copy);
            m_size += count;
        }
        return m_data + idx;
    }

    template<typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        size_type idx = static_cast<size_type>(pos - cbegin());

        size_type count = 0;
        for (InputIt it = first; it != last; ++it)
            ++count;

        if (count == 0)
            return m_data + idx;

        bool self_insert = false;
        if constexpr (std::is_same_v<InputIt, iterator> || std::is_same_v<InputIt, const_iterator>)
            self_insert = (first >= cbegin() && first < cend());

        CompactVector<T> temp;
        if (self_insert) {
            for (InputIt it = first; it != last; ++it)
                temp.push_back(*it);
        }

        size_type new_size = add_sizes(m_size, count);
        if (new_size > m_capacity) [[unlikely]]
            grow(new_size);

        shift_right(idx, count);

        size_type insert_idx = idx;
        if (self_insert) {
            for (const auto& item : temp)
                new (m_data + insert_idx++) T(item);
        } else {
            for (InputIt it = first; it != last; ++it)
                new (m_data + insert_idx++) T(*it);
        }

        m_size += count;
        return m_data + idx;
    }

    iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
        size_type idx   = static_cast<size_type>(pos - cbegin());
        size_type count = static_cast<size_type>(ilist.size());

        if (count == 0)
            return m_data + idx;

        size_type new_size = add_sizes(m_size, count);
        if (new_size > m_capacity) [[unlikely]]
            grow(new_size);

        shift_right(idx, count);

        size_type insert_idx = idx;
        for (const auto& item : ilist)
            new (m_data + insert_idx++) T(item);

        m_size += count;
        return m_data + idx;
    }

    void swap(CompactVector& other) noexcept {
        std::swap(m_data,     other.m_data);
        std::swap(m_size,     other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    // ============ Comparison Operators ============

    [[nodiscard]] bool operator==(const CompactVector& other) const {
        return m_size == other.m_size &&
               std::equal(m_data, m_data + m_size, other.m_data);
    }

    [[nodiscard]] bool operator!=(const CompactVector& other) const { return !operator==(other); }

    [[nodiscard]] auto operator<=>(const CompactVector& other) const {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /// Overflow-safe addition, capped at UINT32_MAX
    static size_type add_sizes(size_type a, size_type b) noexcept {
        if (b > std::numeric_limits<size_type>::max() - a)
            return std::numeric_limits<size_type>::max();
        return a + b;
    }

    /// Next capacity for growth: at least kMinCapacity on first alloc, then double,
    /// and always at least min_required.
    size_type next_capacity(size_type min_required) const noexcept {
        size_type doubled = (m_capacity == 0) ? kMinCapacity
            : (m_capacity <= std::numeric_limits<size_type>::max() / 2)
                ? m_capacity * 2 : std::numeric_limits<size_type>::max();
        return std::max(doubled, min_required);
    }

    /// Grow to accommodate at least min_required elements.
    /// Separated from push_back so the growth path is out-of-line.
    void grow(size_type min_required) {
        relocate(next_capacity(min_required));
    }

    /// Allocate raw memory for count elements (alignment-aware).
    pointer allocate(size_type count) {
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            return static_cast<pointer>(::operator new(count * sizeof(T), std::align_val_t(alignof(T))));
        else
            return static_cast<pointer>(::operator new(count * sizeof(T)));
    }

    /// Deallocate raw memory (alignment-aware). Safe to call with m_data == nullptr.
    void deallocate() noexcept {
        if (m_data == nullptr) return;
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
            ::operator delete(m_data, std::align_val_t(alignof(T)));
        else
            ::operator delete(m_data);
        m_data = nullptr;
    }

    /// Reallocate to new_capacity, moving/copying existing elements.
    /// For trivially-copyable T a single memcpy replaces the element-wise loop.
    void relocate(size_type new_capacity) {
        pointer new_data = allocate(new_capacity);
        if (m_data != nullptr) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(new_data, m_data, m_size * sizeof(T));
            } else {
                for (size_type i = 0; i < m_size; ++i) {
                    new (new_data + i) T(std::move(m_data[i]));
                    m_data[i].~T();
                }
            }
            deallocate();
        }
        m_data     = new_data;
        m_capacity = new_capacity;
    }

    /// Destroy n elements starting at ptr.
    /// For trivially-destructible T this compiles down to nothing.
    static void destroy_range(pointer ptr, size_type n) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_type i = 0; i < n; ++i)
                ptr[i].~T();
        }
    }

    /// Construct n copies of value into uninitialized memory at dst.
    static void uninit_fill(pointer dst, size_type n, const T& value) {
        std::uninitialized_fill_n(dst, n, value);
    }

    /// Copy n elements from src into uninitialized memory at dst.
    template<typename SrcIt>
    static void uninit_copy(SrcIt src, size_type n, pointer dst) {
        std::uninitialized_copy_n(src, n, dst);
    }

    /// Shift elements [idx, m_size) rightward by count slots to open a gap at [idx, idx+count).
    /// The gap slots are left as raw memory — caller must construct into them.
    /// Precondition: capacity >= m_size + count (caller must have reserved).
    void shift_right(size_type idx, size_type count) {
        if (idx == m_size) return; // appending — nothing to move

        if constexpr (std::is_trivially_copyable_v<T>) {
            // memmove handles overlapping regions correctly.
            std::memmove(m_data + idx + count,
                         m_data + idx,
                         (m_size - idx) * sizeof(T));
        } else {
            // Construct new objects at the tail end, moving backwards to avoid
            // overwriting source elements before they are moved.
            for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(m_size) - 1;
                 i >= static_cast<std::ptrdiff_t>(idx); --i) {
                new (m_data + i + count) T(std::move(m_data[i]));
                m_data[i].~T();
            }
        }
    }

    /// Shift elements [idx+count, m_size) leftward by count slots, closing the gap at
    /// [idx, idx+count) and destroying the vacated tail [m_size-count, m_size).
    ///
    /// FIX: The previous version called destroy_range on the erased slots before calling
    /// shift_left, then used placement-new inside shift_left over still-live objects —
    /// undefined behaviour for non-trivial T.  This version owns the full sequence:
    ///  - move-assign over the live elements in [idx, m_size-count)   (no destruct needed)
    ///  - destroy the now-duplicate tail in [m_size-count, m_size)
    /// This matches what std::vector implementations do.
    void shift_left(size_type idx, size_type count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            // For trivial types memmove is both correct and the fastest option.
            // The tail bytes are simply abandoned (destroy_range is a no-op for trivial T).
            std::memmove(m_data + idx,
                         m_data + idx + count,
                         (m_size - idx - count) * sizeof(T));
        } else {
            // Move-assign source elements leftward over existing (live) destination objects.
            // Move-assign is correct here because both source and destination slots are live.
            const size_type new_end = m_size - count;
            for (size_type i = idx; i < new_end; ++i)
                m_data[i] = std::move(m_data[i + count]);
            // Destroy the now-redundant tail objects.
            destroy_range(m_data + new_end, count);
        }
    }
};

/// ADL swap
template<typename T>
inline void swap(CompactVector<T>& lhs, CompactVector<T>& rhs) noexcept {
    lhs.swap(rhs);
}

}

#endif